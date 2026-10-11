#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Lex/Preprocessor.h>

#include <array>

namespace clang::tidy::juce
{
    namespace webview_dev_server_in_release
    {
        class DebugRegionListener : public PPCallbacks
        {
        public:
            DebugRegionListener(const SourceManager& sourceManagerIn,
                                const LangOptions& langOptionsIn,
                                ArrayRef<std::string> debugMacrosIn,
                                std::vector<SourceRange>& regionsIn)
                : sourceManager{ sourceManagerIn }
                , langOptions{ langOptionsIn }
                , debugMacros{ debugMacrosIn }
                , regions{ regionsIn }
            {
            }

            void If(SourceLocation location, SourceRange, ConditionValueKind) override
            {
                push(location, getPolarity(location));
            }

            void Ifdef(SourceLocation location, const Token& name, const MacroDefinition&) override
            {
                push(location, isDebugMacro(name) ? Polarity::debug : Polarity::other);
            }

            void Ifndef(SourceLocation location, const Token& name, const MacroDefinition&) override
            {
                push(location, isDebugMacro(name) ? Polarity::release : Polarity::other);
            }

            void Elif(SourceLocation location, SourceRange, ConditionValueKind, SourceLocation) override
            {
                startBranch(location, getPolarity(location) == Polarity::debug);
            }

            void Elifdef(SourceLocation location, const Token& name, const MacroDefinition&) override
            {
                startBranch(location, isDebugMacro(name));
            }

            void Elifndef(SourceLocation location, const Token&, const MacroDefinition&) override
            {
                startBranch(location, false);
            }

            void Else(SourceLocation location, SourceLocation) override
            {
                if (!conditionals.empty())
                    startBranch(location, conditionals.back().polarity == Polarity::release);
            }

            void Endif(SourceLocation location, SourceLocation) override
            {
                if (conditionals.empty())
                    return;

                endBranch(location);
                conditionals.pop_back();
            }

        private:
            enum class Polarity
            {
                debug,
                release,
                other,
            };

            struct Conditional
            {
                Polarity polarity;
                SourceLocation branchStart;
                bool branchIsDebug;
            };

            void push(SourceLocation location, Polarity polarity)
            {
                conditionals.push_back({ .polarity = polarity, .branchStart = location, .branchIsDebug = polarity == Polarity::debug });
            }

            void startBranch(SourceLocation location, bool isDebug)
            {
                if (conditionals.empty())
                    return;

                endBranch(location);
                conditionals.back().branchStart = location;
                conditionals.back().branchIsDebug = isDebug;
            }

            void endBranch(SourceLocation location)
            {
                if (conditionals.back().branchIsDebug)
                    regions.emplace_back(conditionals.back().branchStart, location);
            }

            [[nodiscard]] bool isDebugMacro(const Token& name) const
            {
                const auto* identifier = name.getIdentifierInfo();
                return identifier != nullptr && llvm::is_contained(debugMacros, identifier->getName());
            }

            [[nodiscard]] Polarity getPolarity(SourceLocation directive) const
            {
                const auto tokens = lexCondition(directive);

                const auto isDebugToken = [this](const Token& token) {
                    return token.is(tok::raw_identifier) && llvm::is_contained(debugMacros, token.getRawIdentifier());
                };
                const auto isNegation = [](const Token& token) {
                    return token.is(tok::exclaim) || utils::isRawIdentifier(token, "not");
                };

                if (tokens.size() == 2 && isNegation(tokens.front()) && isDebugToken(tokens.back()))
                    return Polarity::release;

                for (auto index = 0UZ; index < tokens.size(); ++index)
                {
                    if (isDebugToken(tokens.at(index)) && (index == 0 || !isNegation(tokens.at(index - 1))))
                        return Polarity::debug;
                }

                return Polarity::other;
            }

            [[nodiscard]] std::vector<Token> lexCondition(SourceLocation directive) const
            {
                if (directive.isInvalid() || directive.isMacroID())
                    return {};

                const auto [file, offset] = sourceManager.getDecomposedLoc(directive);
                const auto buffer = sourceManager.getBufferData(file);
                Lexer lexer(sourceManager.getLocForStartOfFile(file),
                            langOptions,
                            buffer.begin(),
                            buffer.data() + offset,
                            buffer.end());

                std::vector<Token> tokens;
                Token token;

                lexer.LexFromRawLexer(token);

                for (lexer.LexFromRawLexer(token);
                     token.isNot(tok::eof) && !token.isAtStartOfLine();
                     lexer.LexFromRawLexer(token))
                {
                    if (!token.isOneOf(tok::l_paren, tok::r_paren) && !utils::isRawIdentifier(token, "defined"))
                        tokens.push_back(token);
                }

                return tokens;
            }

            const SourceManager& sourceManager;
            const LangOptions& langOptions;
            const ArrayRef<std::string> debugMacros;
            std::vector<SourceRange>& regions;
            std::vector<Conditional> conditionals;
        };

        class WebviewDevServerInReleaseCheck : public JuceTidyCheck
        {
        public:
            WebviewDevServerInReleaseCheck(StringRef name, ClangTidyContext* context)
                : JuceTidyCheck{ name, context }
                , rawDebugMacros{ Options.get("DebugMacros", defaultDebugMacros) }
                , debugMacros{ utils::parseQualifiedNames(rawDebugMacros) }
            {
            }

            void storeOptions(ClangTidyOptions::OptionMap& options) override
            {
                Options.store(options, "DebugMacros", rawDebugMacros);
            }

            void registerPPCallbacks(const SourceManager& sourceManager,
                                     Preprocessor* preprocessor,
                                     Preprocessor*) override
            {
                debugRegions.clear();
                preprocessor->addPPCallbacks(std::make_unique<DebugRegionListener>(sourceManager,
                                                                                   getLangOpts(),
                                                                                   debugMacros,
                                                                                   debugRegions));
            }

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const std::vector<std::string> pageAboutToLoad{ "juce::WebBrowserComponent::pageAboutToLoad" };

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(hasName("withResourceProvider"),
                                                           ofClass(hasName("::juce::WebBrowserComponent::Options")))),
                                      hasArgument(1, expr(unless(cxxDefaultArgExpr())).bind("origin"))),
                    this);

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(hasName("goToURL"),
                                                           ofClass(hasName("::juce::WebBrowserComponent")))),
                                      hasArgument(0, expr().bind("url"))),
                    this);

                finder->addMatcher(
                    cxxMethodDecl(hasName("pageAboutToLoad"),
                                  isDefinition(),
                                  unless(isInstantiated()),
                                  utils::isOrOverridesAnyOf(pageAboutToLoad))
                        .bind("pageAboutToLoad"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                using namespace clang::ast_matchers;

                const auto& sourceManager = *result.SourceManager;

                if (const auto* origin = result.Nodes.getNodeAs<Expr>("origin"))
                {
                    if (isEmptyOptional(*origin) || isInDebugRegion(origin->getBeginLoc(), sourceManager))
                        return;

                    diag(origin->getBeginLoc(),
                         "pages from this origin can read your resources in "
                         "release builds too. Only allow it in debug builds");
                    return;
                }

                if (const auto* method = result.Nodes.getNodeAs<CXXMethodDecl>("pageAboutToLoad"))
                {
                    for (const auto& nodes : match(findAll(expr(anyOf(stringLiteral(),
                                                                      declRefExpr(to(varDecl(hasInitializer(expr()))))))
                                                               .bind("url")),
                                                   *method->getBody(),
                                                   *result.Context))
                    {
                        checkUrl(*nodes.getNodeAs<Expr>("url"), sourceManager);
                    }

                    return;
                }

                checkUrl(*result.Nodes.getNodeAs<Expr>("url"), sourceManager);
            }

        private:
            static inline constexpr auto defaultDebugMacros = "JUCE_DEBUG";

            static inline constexpr std::array developmentServers{
                StringRef{ "http://localhost" },
                StringRef{ "https://localhost" },
                StringRef{ "http://127.0.0.1" },
                StringRef{ "https://127.0.0.1" },
            };

            void checkUrl(const Expr& url, const SourceManager& sourceManager)
            {
                const auto* server = findDevelopmentServer(url);

                if (server == nullptr
                    || isInDebugRegion(url.getBeginLoc(), sourceManager)
                    || isInDebugRegion(server->getBeginLoc(), sourceManager))
                {
                    return;
                }

                diag(url.getBeginLoc(),
                     "local development servers should only be used in Debug builds, but '%0' is being used in Release builds too.")
                    << server->getString();
            }

            [[nodiscard]] bool isInDebugRegion(SourceLocation location, const SourceManager& sourceManager) const
            {
                const auto fileLocation = sourceManager.getExpansionLoc(location);

                return llvm::any_of(debugRegions, [&](SourceRange region) {
                    return sourceManager.isBeforeInTranslationUnit(region.getBegin(), fileLocation)
                        && sourceManager.isBeforeInTranslationUnit(fileLocation, region.getEnd());
                });
            }

            [[nodiscard]] static const StringLiteral* findDevelopmentServer(const Expr& url)
            {
                const auto* value = url.IgnoreImplicit()->IgnoreParens();

                for (const auto* construction = dyn_cast<CXXConstructExpr>(value);
                     construction != nullptr && construction->getNumArgs() == 1;
                     construction = dyn_cast<CXXConstructExpr>(value))
                {
                    value = construction->getArg(0)->IgnoreImplicit()->IgnoreParens();
                }

                const auto* literal = dyn_cast<StringLiteral>(value);

                if (const auto* reference = dyn_cast<DeclRefExpr>(value))
                {
                    const auto* variable = dyn_cast<VarDecl>(reference->getDecl());
                    literal = variable != nullptr && variable->getAnyInitializer() != nullptr
                                ? utils::getStringLiteral(*variable->getAnyInitializer())
                                : nullptr;
                }

                return literal != nullptr && isDevelopmentServer(*literal) ? literal : nullptr;
            }

            [[nodiscard]] static bool isDevelopmentServer(const StringLiteral& literal)
            {
                if (!literal.isOrdinary())
                    return false;

                const auto url = literal.getString();

                return llvm::any_of(developmentServers, [url](StringRef server) {
                    const auto rest = url.substr(server.size());
                    return url.starts_with(server)
                        && (rest.empty() || rest.front() == ':' || rest.front() == '/');
                });
            }

            [[nodiscard]] static bool isEmptyOptional(const Expr& origin)
            {
                const auto* construction = dyn_cast<CXXConstructExpr>(origin.IgnoreImplicit());

                if (construction == nullptr)
                    return false;

                if (construction->getNumArgs() == 0)
                    return true;

                const auto* argument = construction->getArg(0)->getType()->getAsCXXRecordDecl();
                return construction->getNumArgs() == 1
                    && argument != nullptr
                    && utils::qualifiedName(*argument) == "std::nullopt_t";
            }

            const StringRef rawDebugMacros;
            const std::vector<std::string> debugMacros;
            std::vector<SourceRange> debugRegions;
        };
    } // namespace webview_dev_server_in_release

    using webview_dev_server_in_release::WebviewDevServerInReleaseCheck;
} // namespace clang::tidy::juce
