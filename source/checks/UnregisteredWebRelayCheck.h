#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace unregistered_web_relay
    {
        class UnregisteredWebRelayCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(cxxRecordDecl(isDefinition(),
                                                 unless(isLambda()),
                                                 unless(isTemplateInstantiation()),
                                                 has(fieldDecl()))
                                       .bind("record"),
                                   this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* record = result.Nodes.getNodeAs<CXXRecordDecl>("record");
                std::vector<const FieldDecl*> relays;

                for (const auto* field : record->fields())
                {
                    if (isRelay(field->getType()) && !field->getLocation().isMacroID())
                        relays.push_back(field);
                }

                if (relays.empty())
                    return;

                auto statements = utils::getClassStatements(*record);

                if (!statements)
                    return;

                for (const auto* field : record->fields())
                {
                    if (const auto* initialiser = field->getInClassInitializer())
                        statements->push_back(initialiser);
                }

                for (const auto* relay : relays)
                {
                    if (mayBeRegistered(*relay, *statements, *result.Context))
                        continue;

                    diag(relay->getLocation(),
                         "%0 is never registered with a WebBrowserComponent")
                        << relay;
                }
            }

        private:
            [[nodiscard]] static bool isRelay(QualType type)
            {
                static const std::vector<std::string> relays{
                    "juce::WebSliderRelay",
                    "juce::WebToggleButtonRelay",
                    "juce::WebComboBoxRelay",
                    "juce::WebControlParameterIndexReceiver",
                };

                return utils::isOrOwnsAnyOf(type, relays);
            }

            [[nodiscard]] static bool mayBeRegistered(const FieldDecl& relay,
                                                      ArrayRef<const Stmt*> statements,
                                                      ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto references = [&relay](const Expr* expression) {
                    return utils::getReferencedField(*expression) == &relay;
                };

                const auto attachment = cxxRecordDecl(hasAnyName("::juce::WebSliderParameterAttachment",
                                                                 "::juce::WebToggleButtonParameterAttachment",
                                                                 "::juce::WebComboBoxParameterAttachment"));
                const auto nonRegisteringCall = expr(anyOf(cxxOperatorCallExpr(),
                                                           cxxConstructExpr(hasType(attachment)),
                                                           callExpr(callee(functionDecl(hasName("::std::make_unique"),
                                                                                        hasTemplateArgument(0,
                                                                                                            refersToType(hasUnqualifiedDesugaredType(recordType(hasDeclaration(attachment))))))))));

                for (const auto* statement : statements)
                {
                    for (const auto& nodes : match(findAll(expr(anyOf(callExpr(),
                                                                      cxxConstructExpr()),
                                                                unless(nonRegisteringCall))
                                                               .bind("call")),
                                                   *statement,
                                                   context))
                    {
                        if (llvm::any_of(utils::getCallArguments(*nodes.getNodeAs<Expr>("call")), references))
                            return true;
                    }
                }

                return false;
            }
        };
    } // namespace unregistered_web_relay

    using unregistered_web_relay::UnregisteredWebRelayCheck;
} // namespace clang::tidy::juce
