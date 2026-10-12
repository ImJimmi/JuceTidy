#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace nullable_dereference
    {
        class NullableDereferenceCheck : public JuceTidyCheck
        {
        public:
            NullableDereferenceCheck(StringRef name, ClangTidyContext* context)
                : JuceTidyCheck{ name, context }
                , rawNullableFunctions{ Options.get("NullableFunctions", defaultNullableFunctions) }
                , nullableFunctions{ utils::parseQualifiedNames(rawNullableFunctions) }
                , checkDynamicCasts{ Options.get("CheckDynamicCasts", true) }
            {
            }

            void storeOptions(ClangTidyOptions::OptionMap& options) override
            {
                Options.store(options, "NullableFunctions", rawNullableFunctions);
                Options.store(options, "CheckDynamicCasts", checkDynamicCasts);
            }

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto nullableFunctionCall = callExpr(callee(functionDecl(utils::isOrOverridesAnyOf(nullableFunctions))
                                                                      .bind("function")))
                                                      .bind("call");
                const auto nullableCast = cxxDynamicCastExpr(hasDestinationType(pointerType())).bind("cast");
                const auto nullableCall = checkDynamicCasts ? expr(anyOf(nullableFunctionCall, nullableCast))
                                                            : expr(nullableFunctionCall);

                finder->addMatcher(
                    expr(unless(isInTemplateInstantiation()),
                         anyOf(memberExpr(isArrow(),
                                          hasObjectExpression(ignoringParenImpCasts(nullableCall))),
                               unaryOperator(hasOperatorName("*"),
                                             hasUnaryOperand(ignoringParenImpCasts(nullableCall))),
                               cxxOperatorCallExpr(hasAnyOverloadedOperatorName("->", "*"),
                                                   hasArgument(0, ignoringImplicit(nullableCall))))),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                if (const auto* cast = result.Nodes.getNodeAs<CXXDynamicCastExpr>("cast"))
                {
                    diag(cast->getExprLoc(),
                         "'dynamic_cast' returns nullptr if the object isn't "
                         "the requested type. Check the result before "
                         "dereferencing it");

                    return;
                }

                const auto* call = result.Nodes.getNodeAs<CallExpr>("call");
                const auto* function = result.Nodes.getNodeAs<FunctionDecl>("function");

                if (isGuardedByTypeCheck(*call, *function, *result.Context))
                    return;

                diag(call->getExprLoc(),
                     "%0 may return nullptr. Check the result before "
                     "dereferencing it")
                    << function;
            }

        private:
            [[nodiscard]] static bool isGuardedByTypeCheck(const CallExpr& call, const FunctionDecl& function, ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto* memberCall = dyn_cast<CXXMemberCallExpr>(&call);
                const auto* object = memberCall != nullptr ? dyn_cast<DeclRefExpr>(memberCall->getImplicitObjectArgument()->IgnoreParenImpCasts()) : nullptr;

                if (object == nullptr || !function.getDeclName().isIdentifier() || !function.getName().starts_with("get"))
                    return false;

                const auto typeName = function.getName().drop_front(3);
                const auto guardName = "is" + (typeName == "DynamicObject" ? StringRef{ "Object" } : typeName).str();
                const auto guard = cxxMemberCallExpr(callee(cxxMethodDecl(hasName(guardName))),
                                                     on(ignoringParenImpCasts(declRefExpr(to(equalsNode(object->getDecl()))))));
                const Stmt* child = &call;

                for (auto parents = context.getParents(call); parents.size() == 1;)
                {
                    if (const auto* variable = parents.begin()->get<VarDecl>(); variable != nullptr && variable->isLocalVarDecl())
                    {
                        parents = context.getParents(*variable);
                        continue;
                    }

                    const auto* parent = parents.begin()->get<Stmt>();

                    if (parent == nullptr)
                        return false;

                    if (const auto* ifStatement = dyn_cast<IfStmt>(parent);
                        ifStatement != nullptr
                        && ifStatement->getThen() == child
                        && utils::findFirst(guard, *ifStatement->getCond(), context) != nullptr)
                    {
                        return true;
                    }

                    child = parent;
                    parents = context.getParents(*child);
                }

                return false;
            }

            static inline constexpr auto defaultNullableFunctions =
                "::juce::AudioProcessor::getPlayHead;"
                "::juce::AudioProcessor::getActiveEditor;"
                "::juce::AudioProcessorValueTreeState::getParameter;"
                "::juce::AudioProcessorValueTreeState::getRawParameterValue;"
                "::juce::Component::getParentComponent;"
                "::juce::Component::findParentComponentOfClass;"
                "::juce::Component::getPeer;"
                "::juce::MessageManager::getInstanceWithoutCreating;"
                "::juce::AudioDeviceManager::getCurrentAudioDevice;"
                "::juce::Displays::getPrimaryDisplay;"
                "::juce::AudioProcessor::getXmlFromBinary;"
                "::juce::parseXML;"
                "::juce::parseXMLIfTagMatches;"
                "::juce::XmlDocument::parse;"
                "::juce::XmlDocument::getDocumentElement;"
                "::juce::ValueTree::createXml;"
                "::juce::WebBrowserComponent::EvaluationResult::getResult;"
                "::juce::WebBrowserComponent::EvaluationResult::getError;"
                "::juce::var::getArray;"
                "::juce::var::getBinaryData;"
                "::juce::var::getObject;"
                "::juce::var::getDynamicObject;"
                "::juce::XmlElement::getChildByName;"
                "::juce::XmlElement::getChildByAttribute;"
                "::juce::XmlElement::getChildElement;"
                "::juce::XmlElement::getFirstChildElement;"
                "::juce::XmlElement::getNextElement;"
                "::juce::XmlElement::getNextElementWithTagName;"
                "::juce::Component::getChildComponent;"
                "::juce::Component::getComponentAt;"
                "::juce::AudioProcessor::getBus;"
                "::juce::AudioFormatManager::createReaderFor;"
                "::juce::AudioFormatManager::findFormatForFileExtension";

            const StringRef rawNullableFunctions;
            const std::vector<std::string> nullableFunctions;
            const bool checkDynamicCasts;
        };
    } // namespace nullable_dereference

    using nullable_dereference::NullableDereferenceCheck;
} // namespace clang::tidy::juce
