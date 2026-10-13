#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>

#include <algorithm>

namespace clang::tidy::juce
{
    namespace prefer_logger
    {
        class PreferLoggerCheck : public JuceTidyCheck
        {
        public:
            PreferLoggerCheck(StringRef name, ClangTidyContext* context)
                : JuceTidyCheck{ name, context }
                , rawStreams{ Options.get("Streams", defaultStreams) }
                , streams{ utils::parseQualifiedNames(rawStreams) }
                , rawFunctions{ Options.get("Functions", defaultFunctions) }
                , functions{ utils::parseQualifiedNames(rawFunctions) }
            {
            }

            void storeOptions(ClangTidyOptions::OptionMap& options) override
            {
                Options.store(options, "Streams", rawStreams);
                Options.store(options, "Functions", rawFunctions);
            }

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto qualified = [](const std::vector<std::string>& names) {
                    std::vector<std::string> result;
                    result.reserve(names.size());

                    for (const auto& name : names)
                        result.push_back("::" + name);

                    return result;
                };

                const auto inLogger = hasAncestor(cxxMethodDecl(ofClass(cxxRecordDecl(isDerivedFrom(hasName("::juce::Logger"))))));

                finder->addMatcher(
                    declRefExpr(unless(isInTemplateInstantiation()),
                                unless(isExpansionInSystemHeader()),
                                unless(inLogger),
                                to(varDecl(hasAnyName(utils::toStringRefs(qualified(streams)))).bind("output")))
                        .bind("reference"),
                    this);

                finder->addMatcher(
                    callExpr(unless(isInTemplateInstantiation()),
                             unless(isExpansionInSystemHeader()),
                             unless(inLogger),
                             callee(functionDecl(hasAnyName(utils::toStringRefs(qualified(functions)))).bind("output")))
                        .bind("reference"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* reference = result.Nodes.getNodeAs<Expr>("reference");
                const auto* output = result.Nodes.getNodeAs<NamedDecl>("output");

                if (utils::isInMacroBody(reference->getBeginLoc(), *result.SourceManager))
                    return;

                if (const auto* call = dyn_cast<CallExpr>(reference); call != nullptr && writesToOtherFile(*call))
                    return;

                diag(reference->getBeginLoc(),
                     "use 'juce::Logger::writeToLog()' instead of '%0'")
                    << utils::qualifiedName(*output);
            }

        private:
            [[nodiscard]] static bool writesToOtherFile(const CallExpr& call)
            {
                const auto* function = call.getDirectCallee();

                for (unsigned int index = 0; index < std::min(call.getNumArgs(), function->getNumParams()); ++index)
                {
                    if (isFilePointer(function->getParamDecl(index)->getType()))
                        return !isStandardStream(*call.getArg(index));
                }

                return false;
            }

            [[nodiscard]] static bool isFilePointer(QualType type)
            {
                if (!type->isPointerType())
                    return false;

                const auto pointee = type->getPointeeType().getUnqualifiedType();
                const auto* typedefType = pointee->getAs<TypedefType>();

                return typedefType != nullptr && typedefType->getDecl()->getName() == "FILE";
            }

            [[nodiscard]] static bool isStandardStream(const Expr& argument)
            {
                const auto* expression = argument.IgnoreParenImpCasts();

                if (const auto* reference = dyn_cast<DeclRefExpr>(expression))
                    return llvm::is_contained({ "stdout", "stderr", "__stdoutp", "__stderrp" }, reference->getDecl()->getName());

                if (const auto* call = dyn_cast<CallExpr>(expression); call != nullptr && call->getDirectCallee() != nullptr && call->getNumArgs() == 1)
                {
                    const auto* index = dyn_cast<IntegerLiteral>(call->getArg(0)->IgnoreParenImpCasts());

                    return call->getDirectCallee()->getName() == "__acrt_iob_func"
                        && index != nullptr
                        && (index->getValue() == 1 || index->getValue() == 2);
                }

                return false;
            }

            static inline constexpr auto defaultStreams =
                "::std::cout;"
                "::std::cerr;"
                "::std::clog;"
                "::std::wcout;"
                "::std::wcerr;"
                "::std::wclog";
            static inline constexpr auto defaultFunctions =
                "::printf;"
                "::vprintf;"
                "::puts;"
                "::putchar;"
                "::fprintf;"
                "::vfprintf;"
                "::fputs;"
                "::fputc;"
                "::putc;"
                "::fwrite;"
                "::std::print;"
                "::std::println;"
                "::std::vprint_unicode;"
                "::std::vprint_nonunicode";

            const StringRef rawStreams;
            const std::vector<std::string> streams;
            const StringRef rawFunctions;
            const std::vector<std::string> functions;
        };
    } // namespace prefer_logger

    using prefer_logger::PreferLoggerCheck;
} // namespace clang::tidy::juce
