#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace buffer_pointer_in_sample_loop
    {
        class BufferPointerInSampleLoopCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(hasAnyName("getWritePointer",
                                                                      "getReadPointer",
                                                                      "getSample",
                                                                      "setSample",
                                                                      "addSample"),
                                                           ofClass(hasName("::juce::AudioBuffer")))
                                                 .bind("method")))
                        .bind("call"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
                const auto* method = result.Nodes.getNodeAs<CXXMethodDecl>("method");
                auto& context = *result.Context;
                const auto* loop = utils::findInnermostLoop(*call, context);

                if (loop == nullptr
                    || utils::isInMacroBody(call->getExprLoc(),
                                            *result.SourceManager))
                {
                    return;
                }

                const auto variables = utils::getLoopVariables(*loop, context);

                if (method->getName().ends_with("Pointer"))
                {
                    if (llvm::any_of(call->arguments(),
                                     [&](const Expr* argument) {
                                         return utils::referencesAny(*argument, variables, context);
                                     }))
                    {
                        return;
                    }

                    diag(call->getExprLoc(),
                         "%0 is called on every iteration of the loop with the "
                         "same arguments. Get the pointer once before the loop")
                        << method;

                    return;
                }

                if (call->getNumArgs() < 2
                    || !utils::referencesAny(*call->getArg(1),
                                             variables,
                                             context))
                {
                    return;
                }

                diag(call->getExprLoc(),
                     "calling %0 for every sample is slower than using the "
                     "pointer from %1")
                    << method
                    << (method->getNameAsString() == "getSample"
                            ? "'getReadPointer'"
                            : "'getWritePointer'");
            }
        };

    } // namespace buffer_pointer_in_sample_loop

    using buffer_pointer_in_sample_loop::BufferPointerInSampleLoopCheck;
} // namespace clang::tidy::juce
