#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Lex/Preprocessor.h>

namespace clang::tidy::juce
{
    namespace plugin_global_state
    {
        class PluginGlobalStateCheck : public JuceTidyCheck
        {
        public:
            PluginGlobalStateCheck(StringRef name, ClangTidyContext* context)
                : JuceTidyCheck{ name, context }
                , rawSingletonMacros{
                    Options.get("SingletonMacros",
                                defaultSingletonMacros),
                }
                , singletonMacros{
                    utils::parseQualifiedNames(rawSingletonMacros),
                }
            {
            }

            void storeOptions(ClangTidyOptions::OptionMap& options) override
            {
                Options.store(options, "SingletonMacros", rawSingletonMacros);
            }

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    varDecl(unless(isExpansionInSystemHeader()),
                            unless(isInstantiated()),
                            anyOf(isStaticLocal(),
                                  isStaticStorageClass()),
                            hasDeclContext(anyOf(functionDecl(),
                                                 cxxRecordDecl())))
                        .bind("variable"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* variable = result.Nodes.getNodeAs<VarDecl>("variable");

                if (!variable->isCanonicalDecl()
                    || variable->getType().getNonReferenceType().isConstQualified()
                    || utils::isInMacroBody(variable->getLocation(),
                                            *result.SourceManager)
                    || utils::isInJuceNamespace(*variable))
                {
                    return;
                }

                diag(variable->getLocation(),
                     "%0 is shared by every instance of the plugin loaded in "
                     "the same process. Store it per-instance, or use a "
                     "juce::SharedResourcePointer if it should be shared")
                    << variable;
            }

            void registerPPCallbacks(const SourceManager& sourceManager,
                                     Preprocessor* preprocessor,
                                     Preprocessor*) override
            {
                preprocessor->addPPCallbacks(std::make_unique<utils::MacroExpansionListener>(
                    singletonMacros,
                    [this, &sourceManager](const Token& name, SourceRange, const MacroArgs*) {
                        const auto location = sourceManager.getExpansionLoc(name.getLocation());

                        if (sourceManager.isInSystemHeader(location))
                            return;

                        diag(location,
                             "singletons are shared by every instance of the "
                             "plugin loaded in the same instance. Store the "
                             "object per instance, or use a "
                             "juce::SharedResourcePointer if it should be "
                             "shared");
                    }));
            }

        private:
            static inline constexpr auto defaultSingletonMacros =
                "JUCE_DECLARE_SINGLETON;"
                "JUCE_DECLARE_SINGLETON_SINGLETHREADED;"
                "JUCE_DECLARE_SINGLETON_SINGLETHREADED_MINIMAL;"
                "JUCE_DECLARE_SINGLETON_INLINE";

            const StringRef rawSingletonMacros;
            const std::vector<std::string> singletonMacros;
        };
    } // namespace plugin_global_state

    using plugin_global_state::PluginGlobalStateCheck;
} // namespace clang::tidy::juce
