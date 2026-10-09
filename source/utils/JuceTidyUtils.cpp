#include "JuceTidyUtils.h"

#include <clang-tidy/utils/OptionsUtils.h>
#include <clang/AST/DeclTemplate.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/Support/Regex.h>

#include <ranges>

namespace clang::tidy::juce::utils
{
std::vector<std::string> parseQualifiedNames(llvm::StringRef option)
{
    return tidy::utils::options::parseStringList(option) | std::views::transform([](llvm::StringRef name)
                                                                                 {
               name.consume_front("::");
               return name.str(); }) |
        std::ranges::to<std::vector>();
}

std::vector<llvm::StringRef> toStringRefs(const std::vector<std::string>& strings)
{
    return {strings.begin(), strings.end()};
}

std::string qualifiedName(const NamedDecl& decl)
{
    std::vector<std::string> parts{decl.getNameAsString()};

    for (const auto* context = decl.getDeclContext(); context != nullptr; context = context->getParent())
    {
        const auto* named = dyn_cast<NamedDecl>(context);

        if (named == nullptr || !named->getDeclName().isIdentifier() || named->getName().empty())
            continue;

        if (const auto* ns = dyn_cast<NamespaceDecl>(named); ns != nullptr && ns->isInline())
            continue;

        parts.push_back(named->getName().str());
    }

    return llvm::join(llvm::reverse(parts), "::");
}

const CXXRecordDecl* getRecord(QualType type)
{
    if (const auto* record = type->getAsCXXRecordDecl())
        return record;

    if (const auto* specialisation = type->getAs<TemplateSpecializationType>())
        if (const auto* templateDecl = specialisation->getTemplateName().getAsTemplateDecl())
            return dyn_cast_or_null<CXXRecordDecl>(templateDecl->getTemplatedDecl());

    return nullptr;
}

bool isSameOrDerivedFrom(const CXXRecordDecl* record, llvm::ArrayRef<std::string> qualifiedNames)
{
    if (record == nullptr)
        return false;

    if (llvm::is_contained(qualifiedNames, qualifiedName(*record)))
        return true;

    const auto* definition = record->getDefinition();

    if (definition == nullptr)
        return false;

    return llvm::any_of(definition->bases(), [&](const CXXBaseSpecifier& base)
                        { return isSameOrDerivedFrom(getRecord(base.getType()), qualifiedNames); });
}

bool hasMethodNamed(const CXXRecordDecl* record, llvm::StringRef name)
{
    const auto* definition = record != nullptr ? record->getDefinition() : nullptr;

    if (definition == nullptr)
        return false;

    const auto isNamed = [name](const CXXMethodDecl* method)
    { return method->getDeclName().isIdentifier() && method->getName() == name; };

    return llvm::any_of(definition->methods(), isNamed) || llvm::any_of(definition->bases(), [name](const CXXBaseSpecifier& base)
                                                                        { return hasMethodNamed(getRecord(base.getType()), name); });
}

bool isOrOverridesAnyOf(const FunctionDecl& function, llvm::ArrayRef<std::string> qualifiedNames)
{
    if (llvm::is_contained(qualifiedNames, qualifiedName(function)))
        return true;

    const auto* method = dyn_cast<CXXMethodDecl>(&function);

    if (method == nullptr)
        return false;

    return llvm::any_of(method->overridden_methods(), [&](const CXXMethodDecl* overridden)
                        { return isOrOverridesAnyOf(*overridden, qualifiedNames); });
}

std::optional<CharSourceRange> getFileRange(SourceRange range, const SourceManager& sourceManager, const LangOptions& langOptions)
{
    const auto fileRange = Lexer::makeFileCharRange(CharSourceRange::getTokenRange(range), sourceManager, langOptions);

    if (fileRange.isInvalid())
        return std::nullopt;

    return fileRange;
}

std::optional<llvm::StringRef> getSourceText(SourceRange range, const SourceManager& sourceManager, const LangOptions& langOptions)
{
    const auto fileRange = getFileRange(range, sourceManager, langOptions);

    if (!fileRange)
        return std::nullopt;

    return Lexer::getSourceText(*fileRange, sourceManager, langOptions);
}

bool isInMacroBody(SourceLocation location, const SourceManager& sourceManager)
{
    return location.isMacroID() && !sourceManager.isMacroArgExpansion(location);
}

static llvm::StringRef getLineUpTo(SourceLocation location, const SourceManager& sourceManager)
{
    const auto [file, offset] = sourceManager.getDecomposedLoc(sourceManager.getSpellingLoc(location));
    const auto buffer = sourceManager.getBufferData(file);
    const auto lineStart = buffer.rfind('\n', offset);

    return buffer.slice(lineStart == llvm::StringRef::npos ? 0 : lineStart + 1, offset);
}

llvm::StringRef getLineIndentation(SourceLocation location, const SourceManager& sourceManager)
{
    const auto line = getLineUpTo(location, sourceManager);
    return line.take_while([](char character)
                           { return character == ' ' || character == '\t'; });
}

bool isFirstOnLine(SourceLocation location, const SourceManager& sourceManager)
{
    return getLineUpTo(location, sourceManager).trim().empty();
}

const Stmt* findFirst(const ast_matchers::internal::Matcher<Stmt>& matcher, const Stmt& root, ASTContext& context)
{
    using namespace ast_matchers;
    return selectFirst<Stmt>("match", match(findAll(stmt(matcher).bind("match")), root, context));
}

DestructorSearch searchDestructor(const CXXRecordDecl& record, const ast_matchers::internal::Matcher<Stmt>& matcher, ASTContext& context)
{
    const auto* destructor = record.getDestructor();

    if (destructor == nullptr || !destructor->isUserProvided())
        return DestructorSearch::notFound;

    const auto* definition = destructor->getDefinition();

    if (definition == nullptr || definition->getBody() == nullptr)
        return DestructorSearch::unknown;

    return findFirst(matcher, *definition->getBody(), context) != nullptr ? DestructorSearch::found : DestructorSearch::notFound;
}

std::vector<const Stmt*> getFunctionStatements(const FunctionDecl& function)
{
    std::vector<const Stmt*> statements;

    if (const auto* constructor = dyn_cast<CXXConstructorDecl>(&function))
        for (const auto* initialiser : constructor->inits())
            if (initialiser->isWritten())
                statements.push_back(initialiser->getInit());

    if (function.getBody() != nullptr)
        statements.push_back(function.getBody());

    return statements;
}

std::optional<std::vector<const Stmt*>> getClassStatements(const CXXRecordDecl& record)
{
    std::vector<const Stmt*> statements;

    for (const auto* method : record.methods())
    {
        if (method->isImplicit() || method->isPureVirtual() || method->isDeleted() || method->isDefaulted())
            continue;

        const FunctionDecl* definition = nullptr;

        if (!method->hasBody(definition) || definition == nullptr)
            return std::nullopt;

        llvm::append_range(statements, getFunctionStatements(*definition));
    }

    return statements;
}

const FieldDecl* getReferencedField(const Expr& expression)
{
    const auto* current = expression.IgnoreParenImpCasts();

    if (const auto* unary = dyn_cast<UnaryOperator>(current); unary != nullptr && (unary->getOpcode() == UO_AddrOf || unary->getOpcode() == UO_Deref))
        current = unary->getSubExpr()->IgnoreParenImpCasts();

    if (const auto* call = dyn_cast<CXXOperatorCallExpr>(current); call != nullptr && (call->getOperator() == OO_Star || call->getOperator() == OO_Arrow))
        current = call->getArg(0)->IgnoreParenImpCasts();

    if (const auto* call = dyn_cast<CXXMemberCallExpr>(current); call != nullptr && call->getMethodDecl() != nullptr && call->getMethodDecl()->getDeclName().isIdentifier() && call->getMethodDecl()->getName() == "get")
        current = call->getImplicitObjectArgument()->IgnoreParenImpCasts();

    const auto* member = dyn_cast<MemberExpr>(current);

    if (member == nullptr || !isa<CXXThisExpr>(member->getBase()->IgnoreParenImpCasts()))
        return nullptr;

    return dyn_cast<FieldDecl>(member->getMemberDecl());
}

bool isComponentOrOwnedComponent(QualType type)
{
    static const std::vector<std::string> component{"juce::Component"};

    if (isSameOrDerivedFrom(type->getAsCXXRecordDecl(), component))
        return true;

    const auto* specialisation = dyn_cast_or_null<ClassTemplateSpecializationDecl>(type->getAsCXXRecordDecl());

    return specialisation != nullptr && qualifiedName(*specialisation) == "std::unique_ptr" && isSameOrDerivedFrom(specialisation->getTemplateArgs().get(0).getAsType()->getAsCXXRecordDecl(), component);
}

llvm::ArrayRef<const Expr*> getCallArguments(const Expr& call)
{
    if (const auto* construction = dyn_cast<CXXConstructExpr>(&call))
        return {construction->getArgs(), construction->getNumArgs()};

    const auto& callExpression = cast<CallExpr>(call);
    return {callExpression.getArgs(), callExpression.getNumArgs()};
}

MacroExpansionListener::MacroExpansionListener(std::vector<std::string> macroNames, Callback callback)
    : macroNames(std::move(macroNames)), callback(std::move(callback))
{
}

void MacroExpansionListener::MacroExpands(const Token& name, const MacroDefinition&, SourceRange range, const MacroArgs* arguments)
{
    if (llvm::is_contained(macroNames, name.getIdentifierInfo()->getName()))
        callback(name, range, arguments);
}

llvm::StringRef ConditionalDirective::keyword() const
{
    return getRawSpelling(tokens.front());
}

bool ConditionalDirective::isDefinedCheck() const
{
    return keyword().ends_with("def");
}

llvm::ArrayRef<Token> ConditionalDirective::condition() const
{
    return llvm::ArrayRef(tokens).drop_front();
}

ConditionalDirectiveListener::ConditionalDirectiveListener(const SourceManager& sourceManager, const LangOptions& langOptions, Callback callback)
    : sourceManager(sourceManager), langOptions(langOptions), callback(std::move(callback))
{
}

void ConditionalDirectiveListener::If(SourceLocation location, SourceRange, ConditionValueKind)
{
    lexDirective(location);
}

void ConditionalDirectiveListener::Elif(SourceLocation location, SourceRange, ConditionValueKind, SourceLocation)
{
    lexDirective(location);
}

void ConditionalDirectiveListener::Ifdef(SourceLocation location, const Token&, const MacroDefinition&)
{
    lexDirective(location);
}

void ConditionalDirectiveListener::Ifndef(SourceLocation location, const Token&, const MacroDefinition&)
{
    lexDirective(location);
}

void ConditionalDirectiveListener::Elifdef(SourceLocation location, const Token&, const MacroDefinition&)
{
    lexDirective(location);
}

void ConditionalDirectiveListener::Elifndef(SourceLocation location, const Token&, const MacroDefinition&)
{
    lexDirective(location);
}

void ConditionalDirectiveListener::lexDirective(SourceLocation keyword)
{
    if (keyword.isInvalid() || keyword.isMacroID())
        return;

    const auto [file, offset] = sourceManager.getDecomposedLoc(keyword);
    const auto buffer = sourceManager.getBufferData(file);
    Lexer lexer(sourceManager.getLocForStartOfFile(file), langOptions, buffer.begin(), buffer.data() + offset, buffer.end());

    ConditionalDirective directive;
    Token token;

    for (lexer.LexFromRawLexer(token); token.isNot(tok::eof) && (directive.tokens.empty() || !token.isAtStartOfLine()); lexer.LexFromRawLexer(token))
        directive.tokens.push_back(token);

    if (directive.tokens.size() > 1)
        callback(directive);
}

llvm::StringRef getRawSpelling(const Token& token)
{
    if (token.is(tok::raw_identifier))
        return token.getRawIdentifier();

    if (token.isLiteral() && token.getLiteralData() != nullptr)
        return {token.getLiteralData(), token.getLength()};

    return {};
}

bool isRawIdentifier(const Token& token, llvm::StringRef name)
{
    return token.is(tok::raw_identifier) && token.getRawIdentifier() == name;
}

std::optional<DefinedOperator> matchDefinedOperator(const std::vector<Token>& tokens, std::size_t index)
{
    if (!isRawIdentifier(tokens.at(index), "defined"))
        return std::nullopt;

    if (index + 1 < tokens.size() && tokens.at(index + 1).is(tok::raw_identifier))
        return DefinedOperator{.name = index + 1, .last = index + 1};

    if (index + 3 < tokens.size() && tokens.at(index + 1).is(tok::l_paren) && tokens.at(index + 2).is(tok::raw_identifier) && tokens.at(index + 3).is(tok::r_paren))
        return DefinedOperator{.name = index + 2, .last = index + 3};

    return std::nullopt;
}

bool isFollowedByDefinitionOf(const ConditionalDirective& directive, llvm::StringRef macroName, const SourceManager& sourceManager)
{
    static const llvm::Regex definition("^[ \t]*#[ \t]*(define|undef)[ \t]+([A-Za-z_][A-Za-z0-9_]*)");

    const auto [file, offset] = sourceManager.getDecomposedLoc(directive.tokens.back().getEndLoc());
    auto remaining = sourceManager.getBufferData(file).substr(offset).split('\n').second;

    while (!remaining.empty())
    {
        const auto [line, rest] = remaining.split('\n');
        const auto trimmed = line.trim();
        remaining = rest;

        if (trimmed.empty() || trimmed.starts_with("//") || trimmed.starts_with("/*") || trimmed.starts_with("*"))
            continue;

        llvm::SmallVector<llvm::StringRef, 3> matches;
        return definition.match(line, &matches) && matches.back() == macroName;
    }

    return false;
}

CharSourceRange getTokensRange(const Token& first, const Token& last)
{
    return CharSourceRange::getCharRange(first.getLocation(), last.getEndLoc());
}
} // namespace clang::tidy::juce::utils
