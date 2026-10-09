#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace scoped_no_denormals
    {
        class ScopedNoDenormalsCheck : public JuceTidyCheck
        {
        public:
            ScopedNoDenormalsCheck(StringRef name, ClangTidyContext* context)
                : JuceTidyCheck{ name, context }
                , rawAudioCallbacks{ Options.get("AudioCallbacks", defaultAudioCallbacks) }
                , audioCallbacks{ utils::parseQualifiedNames(rawAudioCallbacks) }
            {
            }

            void storeOptions(ClangTidyOptions::OptionMap& options) override
            {
                Options.store(options, "AudioCallbacks", rawAudioCallbacks);
            }

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto disablesDenormals = stmt(anyOf(cxxConstructExpr(hasDeclaration(cxxConstructorDecl(ofClass(hasName("::juce::ScopedNoDenormals"))))),
                                                          callExpr(callee(functionDecl(hasName("::juce::FloatVectorOperations::disableDenormalisedNumberSupport"))))));
                const auto callsFunctionThatDisablesDenormals = callExpr(callee(functionDecl(hasBody(hasDescendant(disablesDenormals)))));

                finder->addMatcher(
                    functionDecl(isDefinition(),
                                 unless(isInstantiated()),
                                 utils::isOrOverridesAnyOf(audioCallbacks),
                                 hasBody(compoundStmt(unless(hasDescendant(disablesDenormals)),
                                                      unless(hasDescendant(callsFunctionThatDisablesDenormals)))
                                             .bind("body")))
                        .bind("callback"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* callback = result.Nodes.getNodeAs<FunctionDecl>("callback");
                const auto* body = result.Nodes.getNodeAs<CompoundStmt>("body");
                const auto& sourceManager = *result.SourceManager;

                const auto diagnostic = diag(callback->getLocation(),
                                             "missing "
                                             "'juce::ScopedNoDenormals' in "
                                             "'%0()'")
                                     << callback->getName();

                if (body->body_empty() || body->body_front()->getBeginLoc().isMacroID())
                    return;

                const auto firstStatement = body->body_front()->getBeginLoc();
                const auto indentation = utils::getLineIndentation(firstStatement, sourceManager).str();

                diagnostic << FixItHint::CreateInsertion(firstStatement, "juce::ScopedNoDenormals noDenormals;\n\n" + indentation);
            }

        private:
            static inline constexpr auto defaultAudioCallbacks = "::juce::AudioProcessor::processBlock";

            const StringRef rawAudioCallbacks;
            const std::vector<std::string> audioCallbacks;
        };
    } // namespace scoped_no_denormals

    using scoped_no_denormals::ScopedNoDenormalsCheck;
} // namespace clang::tidy::juce
