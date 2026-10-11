#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/StringMap.h>

namespace clang::tidy::juce
{
    namespace duplicate_webview_name
    {
        class DuplicateWebviewNameCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(expr(unless(isInTemplateInstantiation()),
                                        utils::webBrowserConstruction())
                                       .bind("browser"),
                                   this);

                finder->addMatcher(cxxRecordDecl(isDefinition(),
                                                 unless(isLambda()),
                                                 unless(isTemplateInstantiation()),
                                                 has(fieldDecl()))
                                       .bind("record"),
                                   this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                if (const auto* record = result.Nodes.getNodeAs<CXXRecordDecl>("record"))
                {
                    checkRelays(*record);
                    return;
                }

                const auto* options = utils::getWebBrowserOptionsArgument(*result
                                                                               .Nodes
                                                                               .getNodeAs<Expr>("browser"));

                if (options != nullptr)
                {
                    checkNativeFunctions(utils::findWebBrowserOptionsCalls(*options,
                                                                           *result.Context)
                                             .calls,
                                         *result.SourceManager);
                }
            }

        private:
            void checkNativeFunctions(std::vector<const CXXMemberCallExpr*> calls,
                                      const SourceManager& sourceManager)
            {
                llvm::sort(calls,
                           [&sourceManager](const CXXMemberCallExpr* a,
                                            const CXXMemberCallExpr* b) {
                               return sourceManager.isBeforeInTranslationUnit(a->getExprLoc(),
                                                                              b->getExprLoc());
                           });

                llvm::StringMap<SourceLocation> registered;

                for (const auto* call : calls)
                {
                    const auto* name = (utils::getMethodName(*call) == "withNativeFunction"
                                        && call->getNumArgs() > 0)
                                         ? utils::getStringLiteral(*call->getArg(0))
                                         : nullptr;

                    if (name == nullptr || !name->isOrdinary())
                        continue;

                    const auto [first, inserted] = registered.try_emplace(name->getString(),
                                                                          name->getBeginLoc());

                    if (inserted || first->second == name->getBeginLoc())
                        continue;

                    diag(name->getBeginLoc(),
                         "native function '%0' is already registered. Give "
                         "each native function a unique name")
                        << name->getString();

                    diag(first->second,
                         "'%0' is first registered here",
                         DiagnosticIDs::Note)
                        << name->getString();
                }
            }

            void checkRelays(const CXXRecordDecl& record)
            {
                struct Relay
                {
                    const CXXRecordDecl* type;
                    const FieldDecl* field;
                    const StringLiteral* name;
                };

                std::vector<Relay> relays;

                for (const auto* field : record.fields())
                {
                    const auto* type = utils::getRecord(field->getType());
                    const auto* name = isRelay(type) ? findRelayName(*field, record) : nullptr;

                    if (name == nullptr || !name->isOrdinary())
                        continue;

                    const auto first = llvm::find_if(relays, [type, name](const Relay& relay) {
                        return relay.type == type && relay.name->getString() == name->getString();
                    });

                    if (first == relays.end())
                    {
                        relays.push_back({ .type = type, .field = field, .name = name });
                        continue;
                    }

                    diag(name->getBeginLoc(),
                         "relay name '%0' is already used by %1, so both share "
                         "the same frontend state. Give each relay a unique name")
                        << name->getString()
                        << first->field;

                    diag(first->name->getBeginLoc(),
                         "'%0' is first used here",
                         DiagnosticIDs::Note)
                        << name->getString();
                }
            }

            [[nodiscard]] static bool isRelay(const CXXRecordDecl* type)
            {
                if (type == nullptr)
                    return false;

                const auto name = utils::qualifiedName(*type);

                return name == "juce::WebSliderRelay"
                    || name == "juce::WebToggleButtonRelay"
                    || name == "juce::WebComboBoxRelay";
            }

            [[nodiscard]] static const StringLiteral* findRelayName(const FieldDecl& field, const CXXRecordDecl& record)
            {
                if (const auto* initialiser = field.getInClassInitializer())
                    return getConstructedName(*initialiser);

                for (const auto* constructor : record.ctors())
                {
                    const auto* definition = dyn_cast_or_null<CXXConstructorDecl>(constructor->getDefinition());

                    if (definition == nullptr)
                        continue;

                    for (const auto* initialiser : definition->inits())
                    {
                        if (initialiser->getMember() == &field && initialiser->isWritten())
                            return getConstructedName(*initialiser->getInit());
                    }
                }

                return nullptr;
            }

            [[nodiscard]] static const StringLiteral* getConstructedName(const Expr& initialiser)
            {
                const auto* construction = dyn_cast<CXXConstructExpr>(initialiser.IgnoreImplicit());

                return construction != nullptr && construction->getNumArgs() > 0
                         ? utils::getStringLiteral(*construction->getArg(0))
                         : nullptr;
            }
        };
    } // namespace duplicate_webview_name

    using duplicate_webview_name::DuplicateWebviewNameCheck;
} // namespace clang::tidy::juce
