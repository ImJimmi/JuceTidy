#include "JuceTidyUtils.h"

#include <clang-tidy/utils/OptionsUtils.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/StmtCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallPtrSet.h>
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

bool isOrOwnsAnyOf(QualType type, llvm::ArrayRef<std::string> qualifiedNames)
{
    if (isSameOrDerivedFrom(type->getAsCXXRecordDecl(), qualifiedNames))
        return true;

    const auto* specialisation = dyn_cast_or_null<ClassTemplateSpecializationDecl>(type->getAsCXXRecordDecl());

    return specialisation != nullptr && qualifiedName(*specialisation) == "std::unique_ptr" && isSameOrDerivedFrom(specialisation->getTemplateArgs().get(0).getAsType()->getAsCXXRecordDecl(), qualifiedNames);
}

bool isComponentOrOwnedComponent(QualType type)
{
    static const std::vector<std::string> component{"juce::Component"};

    return isOrOwnsAnyOf(type, component);
}

bool isInJuceNamespace(const NamedDecl& decl)
{
    return llvm::StringRef{qualifiedName(decl)}.starts_with("juce::");
}

const CXXMethodDecl* findAccessibilityHandlerOverride(const CXXRecordDecl& record)
{
    const auto* definition = record.getDefinition();

    if (definition == nullptr)
        return nullptr;

    for (const auto* method : definition->methods())
        if (method->getDeclName().isIdentifier() && method->getName() == "createAccessibilityHandler" && method->size_overridden_methods() > 0)
            return method;

    for (const auto& base : definition->bases())
        if (const auto* baseRecord = getRecord(base.getType()))
            if (const auto* method = findAccessibilityHandlerOverride(*baseRecord))
                return method;

    return nullptr;
}

llvm::ArrayRef<const Expr*> getCallArguments(const Expr& call)
{
    if (const auto* construction = dyn_cast<CXXConstructExpr>(&call))
        return {construction->getArgs(), construction->getNumArgs()};

    const auto& callExpression = cast<CallExpr>(call);
    return {callExpression.getArgs(), callExpression.getNumArgs()};
}

const StringLiteral* getStringLiteral(const Expr& expression)
{
    const auto* current = expression.IgnoreImplicit()->IgnoreParens();

    while (true)
    {
        if (const auto* literal = dyn_cast<StringLiteral>(current))
            return literal;

        if (const auto* cast = dyn_cast<CXXFunctionalCastExpr>(current))
            current = cast->getSubExpr();
        else if (const auto* construction = dyn_cast<CXXConstructExpr>(current); construction != nullptr && construction->getNumArgs() == 1)
            current = construction->getArg(0);
        else
            return nullptr;

        current = current->IgnoreImplicit()->IgnoreParens();
    }
}

const Stmt* findInnermostLoop(const Stmt& statement, ASTContext& context)
{
    auto parents = context.getParents(statement);

    while (!parents.empty())
    {
        const auto& parent = *parents.begin();

        if (parent.get<FunctionDecl>() != nullptr || parent.get<LambdaExpr>() != nullptr)
            return nullptr;

        if (const auto* loop = parent.get<Stmt>(); loop != nullptr && isa<ForStmt, CXXForRangeStmt>(loop))
            return loop;

        parents = context.getParents(parent);
    }

    return nullptr;
}

llvm::SmallPtrSet<const Decl*, 4> getLoopVariables(const Stmt& loop, ASTContext& context)
{
    using namespace ast_matchers;

    llvm::SmallPtrSet<const Decl*, 4> variables;

    if (const auto* rangeLoop = dyn_cast<CXXForRangeStmt>(&loop))
    {
        variables.insert(rangeLoop->getLoopVariable());
        return variables;
    }

    const auto& forLoop = cast<ForStmt>(loop);

    for (const auto* part : {forLoop.getInit(), static_cast<const Stmt*>(forLoop.getInc())})
    {
        if (part == nullptr)
            continue;

        for (const auto& nodes : match(findAll(stmt(anyOf(declStmt(forEach(varDecl().bind("variable"))), declRefExpr(to(varDecl().bind("variable")))))), *part, context))
            variables.insert(nodes.getNodeAs<VarDecl>("variable"));
    }

    return variables;
}

bool referencesAny(const Expr& expression, const llvm::SmallPtrSetImpl<const Decl*>& declarations, ASTContext& context)
{
    using namespace ast_matchers;

    return llvm::any_of(match(findAll(declRefExpr().bind("reference")), expression, context),
                        [&declarations](const BoundNodes& nodes) { return declarations.contains(nodes.getNodeAs<DeclRefExpr>("reference")->getDecl()); });
}

ast_matchers::internal::BindableMatcher<Stmt> webBrowserConstruction()
{
    using namespace ast_matchers;

    const auto browser = hasUnqualifiedDesugaredType(recordType(hasDeclaration(cxxRecordDecl(ast_matchers::isSameOrDerivedFrom(hasName("::juce::WebBrowserComponent"))))));

    return expr(anyOf(cxxConstructExpr(hasType(browser)),
                      callExpr(callee(functionDecl(hasAnyName("::std::make_unique", "::std::make_shared"),
                                                   hasTemplateArgument(0, refersToType(browser)))))));
}

static bool isWebBrowserOptions(const CXXRecordDecl* record)
{
    if (record == nullptr)
        return false;

    const auto name = qualifiedName(*record);
    return name == "juce::WebBrowserComponent::Options" || name == "juce::WebBrowserComponent::Options::WinWebView2";
}

const Expr* getWebBrowserOptionsArgument(const Expr& construction)
{
    const auto arguments = getCallArguments(construction);
    const auto* options = llvm::find_if(arguments, [](const Expr* argument)
                                        {
        const auto* record = argument->getType()->getAsCXXRecordDecl();
        return record != nullptr && qualifiedName(*record) == "juce::WebBrowserComponent::Options"; });

    return options != arguments.end() ? *options : nullptr;
}

llvm::StringRef getMethodName(const CXXMemberCallExpr& call)
{
    const auto* method = call.getMethodDecl();

    if (method == nullptr || !method->getDeclName().isIdentifier())
        return {};

    return method->getName();
}

namespace
{
class WebBrowserOptionsFinder
{
public:
    explicit WebBrowserOptionsFinder(ASTContext& context)
        : context(context)
    {
    }

    WebBrowserOptionsCalls find(const Expr& options)
    {
        visit(options);
        return result;
    }

private:
    static const Expr* skipCopies(const Expr& expression)
    {
        const auto* current = expression.IgnoreImplicit()->IgnoreParens();

        while (true)
        {
            if (const auto* cast = dyn_cast<CXXFunctionalCastExpr>(current))
                current = cast->getSubExpr();
            else if (const auto* construction = dyn_cast<CXXConstructExpr>(current); construction != nullptr && construction->getConstructor()->isCopyOrMoveConstructor())
                current = construction->getArg(0);
            else if (const auto* call = dyn_cast<CallExpr>(current); call != nullptr && call->isCallToStdMove())
                current = call->getArg(0);
            else
                return current;

            current = current->IgnoreImplicit()->IgnoreParens();
        }
    }

    void visit(const Expr& expression)
    {
        const auto* current = skipCopies(expression);

        if (const auto* call = dyn_cast<CXXMemberCallExpr>(current); call != nullptr && call->getMethodDecl() != nullptr && isWebBrowserOptions(call->getMethodDecl()->getParent()))
        {
            result.calls.push_back(call);
            visit(*call->getImplicitObjectArgument());

            if (getMethodName(*call) == "withWinWebView2Options" && call->getNumArgs() == 1)
                visit(*call->getArg(0));

            return;
        }

        if (const auto* construction = dyn_cast<CXXConstructExpr>(current); construction != nullptr && construction->getNumArgs() == 0)
            return;

        if (const auto* list = dyn_cast<InitListExpr>(current); list != nullptr && list->getNumInits() == 0)
            return;

        if (const auto* conditional = dyn_cast<AbstractConditionalOperator>(current))
        {
            visit(*conditional->getTrueExpr());
            visit(*conditional->getFalseExpr());
            return;
        }

        if (const auto* reference = dyn_cast<DeclRefExpr>(current))
        {
            if (const auto* variable = dyn_cast<VarDecl>(reference->getDecl()); variable != nullptr && !isa<ParmVarDecl>(variable))
            {
                visitVariable(*variable);
                return;
            }
        }

        if (const auto* call = dyn_cast<CallExpr>(current))
        {
            const FunctionDecl* definition = nullptr;
            const auto* callee = call->getDirectCallee();
            const auto* method = dyn_cast_or_null<CXXMethodDecl>(callee);

            if (callee != nullptr && callee->hasBody(definition) && definition != nullptr && (method == nullptr || !method->isVirtual()))
            {
                visitFunction(*definition);
                return;
            }
        }

        result.isComplete = false;
    }

    void visitVariable(const VarDecl& variable)
    {
        if (!visited.insert(&variable).second)
            return;

        if (const auto* initialiser = variable.getInit())
            visit(*initialiser);
        else
            result.isComplete = false;

        const auto* function = dyn_cast_or_null<FunctionDecl>(variable.getParentFunctionOrMethod());

        if (function == nullptr || function->getBody() == nullptr)
        {
            if (!variable.getType().isConstQualified())
                result.isComplete = false;

            return;
        }

        using namespace ast_matchers;

        const auto reference = ignoringParenImpCasts(declRefExpr(to(varDecl(equalsNode(&variable)))));
        const auto& body = *function->getBody();

        for (const auto& nodes : match(findAll(cxxOperatorCallExpr(hasOverloadedOperatorName("="),
                                                                   hasArgument(0, reference),
                                                                   hasArgument(1, expr().bind("value")))),
                                       body,
                                       context))
        {
            visit(*nodes.getNodeAs<Expr>("value"));
        }

        const auto modification = expr(anyOf(callExpr(unless(callee(functionDecl(hasAnyName("::std::move", "::std::forward", "::std::make_unique", "::std::make_shared")))),
                                                      forEachArgumentWithParam(reference, parmVarDecl(hasType(references(qualType(unless(isConstQualified()))))))),
                                             unaryOperator(hasOperatorName("&"), hasUnaryOperand(reference))));

        if (!match(findAll(modification), body, context).empty())
            result.isComplete = false;
    }

    void visitFunction(const FunctionDecl& function)
    {
        if (!visited.insert(&function).second)
            return;

        using namespace ast_matchers;

        for (const auto& nodes : match(findAll(returnStmt(forCallable(equalsNode(&function)),
                                                          hasReturnValue(expr().bind("value")))),
                                       *function.getBody(),
                                       context))
        {
            visit(*nodes.getNodeAs<Expr>("value"));
        }
    }

    ASTContext& context;
    WebBrowserOptionsCalls result;
    llvm::SmallPtrSet<const Decl*, 8> visited;
};
} // namespace

WebBrowserOptionsCalls findWebBrowserOptionsCalls(const Expr& options, ASTContext& context)
{
    return WebBrowserOptionsFinder{context}.find(options);
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
