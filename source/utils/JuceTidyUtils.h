#pragma once

#include <clang-tidy/ClangTidyCheck.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclCXX.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/PPCallbacks.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/StringRef.h>

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace clang::tidy::juce
{
class JuceTidyCheck : public ClangTidyCheck
{
public:
    using ClangTidyCheck::ClangTidyCheck;

    [[nodiscard]] bool isLanguageVersionSupported(const LangOptions& langOptions) const override
    {
        return langOptions.CPlusPlus14;
    }
};
} // namespace clang::tidy::juce

namespace clang::tidy::juce::utils
{
std::vector<std::string> parseQualifiedNames(llvm::StringRef option);
std::vector<llvm::StringRef> toStringRefs(const std::vector<std::string>& strings);

std::string qualifiedName(const NamedDecl& decl);

const CXXRecordDecl* getRecord(QualType type);
bool isSameOrDerivedFrom(const CXXRecordDecl* record, llvm::ArrayRef<std::string> qualifiedNames);
bool hasMethodNamed(const CXXRecordDecl* record, llvm::StringRef name);
bool isOrOverridesAnyOf(const FunctionDecl& function, llvm::ArrayRef<std::string> qualifiedNames);

std::optional<CharSourceRange> getFileRange(SourceRange range, const SourceManager& sourceManager, const LangOptions& langOptions);
std::optional<llvm::StringRef> getSourceText(SourceRange range, const SourceManager& sourceManager, const LangOptions& langOptions);

bool isInMacroBody(SourceLocation location, const SourceManager& sourceManager);

llvm::StringRef getLineIndentation(SourceLocation location, const SourceManager& sourceManager);
bool isFirstOnLine(SourceLocation location, const SourceManager& sourceManager);

template <typename Node>
std::optional<DynTypedNode> getSingleParent(const Node& node, ASTContext& context)
{
    const auto parents = context.getParents(node);

    if (parents.size() != 1)
        return std::nullopt;

    return *parents.begin();
}

const Stmt* findFirst(const ast_matchers::internal::Matcher<Stmt>& matcher, const Stmt& root, ASTContext& context);

enum class DestructorSearch
{
    found,
    notFound,
    unknown,
};

DestructorSearch searchDestructor(const CXXRecordDecl& record, const ast_matchers::internal::Matcher<Stmt>& matcher, ASTContext& context);

std::vector<const Stmt*> getFunctionStatements(const FunctionDecl& function);
std::optional<std::vector<const Stmt*>> getClassStatements(const CXXRecordDecl& record);
const FieldDecl* getReferencedField(const Expr& expression);
bool isOrOwnsAnyOf(QualType type, llvm::ArrayRef<std::string> qualifiedNames);
bool isComponentOrOwnedComponent(QualType type);
bool isInJuceNamespace(const NamedDecl& decl);
const CXXMethodDecl* findAccessibilityHandlerOverride(const CXXRecordDecl& record);
llvm::ArrayRef<const Expr*> getCallArguments(const Expr& call);
const StringLiteral* getStringLiteral(const Expr& expression);
const Stmt* findInnermostLoop(const Stmt& statement, ASTContext& context);
llvm::SmallPtrSet<const Decl*, 4> getLoopVariables(const Stmt& loop, ASTContext& context);
bool referencesAny(const Expr& expression, const llvm::SmallPtrSetImpl<const Decl*>& declarations, ASTContext& context);

struct WebBrowserOptionsCalls
{
    std::vector<const CXXMemberCallExpr*> calls;
    bool isComplete = true;
};

ast_matchers::internal::BindableMatcher<Stmt> webBrowserConstruction();
const Expr* getWebBrowserOptionsArgument(const Expr& construction);
WebBrowserOptionsCalls findWebBrowserOptionsCalls(const Expr& options, ASTContext& context);
llvm::StringRef getMethodName(const CXXMemberCallExpr& call);

class MacroExpansionListener : public PPCallbacks
{
public:
    using Callback = std::function<void(const Token& name, SourceRange range, const MacroArgs* arguments)>;

    MacroExpansionListener(std::vector<std::string> macroNames, Callback callback);

    void MacroExpands(const Token& name, const MacroDefinition& definition, SourceRange range, const MacroArgs* arguments) override;

private:
    const std::vector<std::string> macroNames;
    const Callback callback;
};

struct ConditionalDirective
{
    std::vector<Token> tokens;

    [[nodiscard]] llvm::StringRef keyword() const;
    [[nodiscard]] bool isDefinedCheck() const;
    [[nodiscard]] llvm::ArrayRef<Token> condition() const;
};

class ConditionalDirectiveListener : public PPCallbacks
{
public:
    using Callback = std::function<void(const ConditionalDirective& directive)>;

    ConditionalDirectiveListener(const SourceManager& sourceManager, const LangOptions& langOptions, Callback callback);

    void If(SourceLocation location, SourceRange conditionRange, ConditionValueKind value) override;
    void Elif(SourceLocation location, SourceRange conditionRange, ConditionValueKind value, SourceLocation ifLocation) override;
    void Ifdef(SourceLocation location, const Token& name, const MacroDefinition& definition) override;
    void Ifndef(SourceLocation location, const Token& name, const MacroDefinition& definition) override;
    void Elifdef(SourceLocation location, const Token& name, const MacroDefinition& definition) override;
    void Elifndef(SourceLocation location, const Token& name, const MacroDefinition& definition) override;

private:
    void lexDirective(SourceLocation keyword);

    const SourceManager& sourceManager;
    const LangOptions& langOptions;
    const Callback callback;
};

struct DefinedOperator
{
    std::size_t name;
    std::size_t last;
};

llvm::StringRef getRawSpelling(const Token& token);
bool isRawIdentifier(const Token& token, llvm::StringRef name);
std::optional<DefinedOperator> matchDefinedOperator(const std::vector<Token>& tokens, std::size_t index);
bool isFollowedByDefinitionOf(const ConditionalDirective& directive, llvm::StringRef macroName, const SourceManager& sourceManager);
CharSourceRange getTokensRange(const Token& first, const Token& last);

AST_MATCHER_P(FunctionDecl, isOrOverridesAnyOf, std::vector<std::string>, qualifiedNames)
{
    return utils::isOrOverridesAnyOf(Node, qualifiedNames);
}

AST_MATCHER_P(CXXRecordDecl, isSameOrDerivedFromAnyOf, std::vector<std::string>, qualifiedNames)
{
    return utils::isSameOrDerivedFrom(&Node, qualifiedNames);
}
} // namespace clang::tidy::juce::utils
