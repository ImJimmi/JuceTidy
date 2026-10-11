#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace atomic_in_sample_loop
    {
        class AtomicInSampleLoopCheck : public JuceTidyCheck
        {
        public:
            AtomicInSampleLoopCheck(StringRef name, ClangTidyContext* context)
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

                const auto atomic = cxxRecordDecl(isSameOrDerivedFrom(hasName("::std::atomic")));
                const auto atomicRead = cxxMemberCallExpr(callee(cxxMethodDecl(anyOf(hasName("load"),
                                                                                     cxxConversionDecl()))),
                                                          on(ignoringParenImpCasts(anyOf(hasType(atomic),
                                                                                         hasType(pointsTo(atomic))))));
                const auto parameterRead = cxxMemberCallExpr(callee(cxxMethodDecl(anyOf(hasName("get"),
                                                                                        cxxConversionDecl()),
                                                                                  ofClass(hasAnyName("::juce::AudioParameterFloat",
                                                                                                     "::juce::AudioParameterInt",
                                                                                                     "::juce::AudioParameterBool",
                                                                                                     "::juce::AudioParameterChoice")))));

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      anyOf(atomicRead.bind("atomic"),
                                            parameterRead),
                                      forCallable(functionDecl(utils::isOrOverridesAnyOf(audioCallbacks))))
                        .bind("read"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* read = result.Nodes.getNodeAs<CXXMemberCallExpr>("read");
                const auto isAtomic = result.Nodes.getNodeAs<Expr>("atomic") != nullptr;
                auto& context = *result.Context;
                const auto* loop = utils::findInnermostLoop(*read, context);

                if (loop == nullptr
                    || utils::isInMacroBody(read->getExprLoc(),
                                            *result.SourceManager))
                {
                    return;
                }

                const auto& object = getReadObject(*read);

                if (utils::referencesAny(object,
                                         utils::getLoopVariables(*loop, context),
                                         context))
                {
                    return;
                }

                const auto name = utils::getSourceText(object.getSourceRange(),
                                                       *result.SourceManager,
                                                       context.getLangOpts());

                diag(read->getBeginLoc(),
                     "'%1' is read for every sample in the buffer. Reading an "
                     "atomic is expensive so read it once per block and reuse "
                     "the value instead")
                    << isAtomic
                    << name.value_or("value");
            }

        private:
            [[nodiscard]] static const Expr& getReadObject(const CXXMemberCallExpr& read)
            {
                const auto* object = read.getImplicitObjectArgument()->IgnoreParenImpCasts();

                if (const auto* dereference = dyn_cast<UnaryOperator>(object);
                    dereference != nullptr
                    && dereference->getOpcode() == UO_Deref)
                {
                    return *dereference->getSubExpr()->IgnoreParenImpCasts();
                }

                return *object;
            }

            static inline constexpr auto defaultAudioCallbacks =
                "::juce::AudioProcessor::processBlock;"
                "::juce::AudioProcessor::processBlockBypassed;"
                "::juce::AudioSource::getNextAudioBlock;"
                "::juce::AudioIODeviceCallback::audioDeviceIOCallbackWithContext;"
                "::juce::SynthesiserVoice::renderNextBlock;"
                "::juce::MPESynthesiserVoice::renderNextBlock;"
                "::juce::dsp::ProcessorBase::process";

            const StringRef rawAudioCallbacks;
            const std::vector<std::string> audioCallbacks;
        };
    } // namespace atomic_in_sample_loop

    using atomic_in_sample_loop::AtomicInSampleLoopCheck;
} // namespace clang::tidy::juce
