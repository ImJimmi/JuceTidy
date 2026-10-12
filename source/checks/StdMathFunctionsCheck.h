#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang-tidy/utils/IncludeInserter.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringSwitch.h>

#include <algorithm>

namespace clang::tidy::juce
{
    namespace std_math_functions
    {
        class StdMathFunctionsCheck : public JuceTidyCheck
        {
        public:
            StdMathFunctionsCheck(StringRef name, ClangTidyContext* context)
                : JuceTidyCheck{ name, context }
                , preferJuceFunctions{ Options.get("PreferJuceFunctions", false) }
                , includeInserter{ Options.getLocalOrGlobal("IncludeStyle", tidy::utils::IncludeSorter::IS_LLVM), areDiagsSelfContained() }
            {
            }

            void storeOptions(ClangTidyOptions::OptionMap& options) override
            {
                Options.store(options, "PreferJuceFunctions", preferJuceFunctions);
                Options.store(options, "IncludeStyle", includeInserter.getStyle());
            }

            void registerPPCallbacks(const SourceManager&, Preprocessor* preprocessor, Preprocessor*) override
            {
                includeInserter.registerPreprocessor(preprocessor);
            }

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto keys = getStandardNames().keys();
                std::vector<StringRef> standardNames(keys.begin(), keys.end());
                std::vector<std::string> juceNames;

                for (const auto& equivalent : juceEquivalents)
                {
                    standardNames.push_back(equivalent.standardName);

                    if (equivalent.form != Form::clamp || getLangOpts().CPlusPlus17)
                        juceNames.push_back(("::juce::" + equivalent.juceName).str());
                }

                const auto callTo = [](const auto& function) {
                    return callExpr(unless(isInTemplateInstantiation()),
                                    callee(expr(ignoringImpCasts(declRefExpr(to(function)).bind("reference")))))
                        .bind("call");
                };

                finder->addMatcher(callTo(functionDecl(hasAnyName(standardNames))), this);
                finder->addMatcher(callTo(functionDecl(hasAnyName(utils::toStringRefs(juceNames))).bind("juce")), this);

                if (getLangOpts().CPlusPlus20 && !preferJuceFunctions)
                {
                    const auto constants = classTemplateSpecializationDecl(hasName("::juce::MathConstants"),
                                                                           hasTemplateArgument(0, refersToType(qualType(realFloatingPointType()).bind("floatType"))));

                    finder->addMatcher(
                        declRefExpr(unless(isInTemplateInstantiation()),
                                    to(varDecl(hasAnyName("pi", "twoPi", "halfPi", "euler", "sqrt2"),
                                               hasDeclContext(constants))))
                            .bind("constant"),
                        this);
                }
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                if (const auto* constant = result.Nodes.getNodeAs<DeclRefExpr>("constant"))
                {
                    reportMathConstant(*constant, *result.Nodes.getNodeAs<QualType>("floatType"), *result.SourceManager);
                    return;
                }

                const auto* call = result.Nodes.getNodeAs<CallExpr>("call");
                const auto* reference = result.Nodes.getNodeAs<DeclRefExpr>("reference");
                const auto function = getFunction(*call, *reference, result.Nodes.getNodeAs<FunctionDecl>("juce") != nullptr);

                if (!function || utils::isInMacroBody(reference->getBeginLoc(), *result.SourceManager))
                    return;

                const auto toJuce = preferJuceFunctions && function->equivalent != nullptr;

                if (toJuce ? function->isJuce : function->isStandard)
                    return;

                const auto replacement = toJuce
                                           ? ("juce::" + function->equivalent->juceName).str()
                                           : "std::" + function->standardName;
                const auto diagnostic = diag(reference->getBeginLoc(),
                                             "use '%0' rather than '%1'")
                                     << replacement
                                     << function->spelling;

                if (!isSafeToReplace(*call, *reference, *function, toJuce))
                    return;

                if (const auto fix = getFix(*call,
                                            *reference,
                                            *function,
                                            toJuce,
                                            replacement,
                                            *result.SourceManager,
                                            result.Context->getLangOpts()))
                {
                    diagnostic << *fix;
                }
            }

        private:
            void reportMathConstant(const DeclRefExpr& constant, QualType floatType, const SourceManager& sourceManager)
            {
                if (utils::isInMacroBody(constant.getBeginLoc(), sourceManager))
                    return;

                const auto name = constant.getDecl()->getName();
                const auto suffix = floatType->isSpecificBuiltinType(BuiltinType::Double)
                                      ? std::string{}
                                      : "_v<" + floatType.getAsString(PrintingPolicy{ getLangOpts() }) + ">";
                const auto standardConstant = [&suffix](StringRef standardName) {
                    return "std::numbers::" + standardName.str() + suffix;
                };
                const auto replacement = llvm::StringSwitch<std::string>(name)
                                             .Case("pi", standardConstant("pi"))
                                             .Case("twoPi", "(2 * " + standardConstant("pi") + ")")
                                             .Case("halfPi", "(" + standardConstant("pi") + " / 2)")
                                             .Case("euler", standardConstant("e"))
                                             .Default(standardConstant("sqrt2"));
                const auto spelling = utils::getSourceText(constant.getSourceRange(), sourceManager, getLangOpts());

                const auto diagnostic = diag(constant.getBeginLoc(),
                                             "use '%0' rather than '%1'")
                                     << StringRef{ replacement }.trim("()")
                                     << spelling.value_or(("juce::MathConstants::" + name).str())
                                     << FixItHint::CreateReplacement(constant.getSourceRange(), replacement);

                if (auto include = includeInserter.createIncludeInsertion(sourceManager.getFileID(sourceManager.getExpansionLoc(constant.getBeginLoc())), "<numbers>"))
                    diagnostic << *include;
            }

            enum class Form
            {
                maths,
                variadic,
                clamp,
            };

            struct JuceEquivalent
            {
                StringRef juceName;
                StringRef standardName;
                unsigned numArguments;
                Form form;
            };

            struct Function
            {
                std::string spelling;
                std::string standardName;
                const JuceEquivalent* equivalent;
                bool isJuce;
                bool isStandard;
            };

            static inline constexpr std::array juceEquivalents{
                JuceEquivalent{
                    .juceName = "jmax",
                    .standardName = "max",
                    .numArguments = 2,
                    .form = Form::variadic,
                },
                JuceEquivalent{
                    .juceName = "jmin",
                    .standardName = "min",
                    .numArguments = 2,
                    .form = Form::variadic,
                },
                JuceEquivalent{
                    .juceName = "jlimit",
                    .standardName = "clamp",
                    .numArguments = 3,
                    .form = Form::clamp,
                },
                JuceEquivalent{
                    .juceName = "juce_hypot",
                    .standardName = "hypot",
                    .numArguments = 2,
                    .form = Form::maths,
                },
                JuceEquivalent{
                    .juceName = "juce_isfinite",
                    .standardName = "isfinite",
                    .numArguments = 1,
                    .form = Form::maths,
                },
            };

            [[nodiscard]] static const llvm::StringMap<std::string>& getStandardNames()
            {
                static const auto names = [] {
                    static constexpr std::array functions{
                        "acos",
                        "acosh",
                        "asin",
                        "asinh",
                        "atan",
                        "atan2",
                        "atanh",
                        "cbrt",
                        "ceil",
                        "copysign",
                        "cos",
                        "cosh",
                        "erf",
                        "erfc",
                        "exp",
                        "exp2",
                        "expm1",
                        "fdim",
                        "floor",
                        "fma",
                        "fmax",
                        "fmin",
                        "fmod",
                        "frexp",
                        "hypot",
                        "ldexp",
                        "lgamma",
                        "llrint",
                        "llround",
                        "log",
                        "log10",
                        "log1p",
                        "log2",
                        "lrint",
                        "lround",
                        "modf",
                        "nearbyint",
                        "pow",
                        "remainder",
                        "rint",
                        "round",
                        "sin",
                        "sinh",
                        "sqrt",
                        "tan",
                        "tanh",
                        "tgamma",
                        "trunc",
                    };

                    llvm::StringMap<std::string> result;

                    for (const std::string function : functions)
                    {
                        result.try_emplace(function, function);
                        result.try_emplace(function + "f", function);
                        result.try_emplace(function + "l", function);
                    }

                    for (const auto* absolute : {
                             "abs",
                             "labs",
                             "llabs",
                             "fabs",
                             "fabsf",
                             "fabsl",
                         })
                    {
                        result.try_emplace(absolute, "abs");
                    }

                    return result;
                }();

                return names;
            }

            [[nodiscard]] static const JuceEquivalent* findEquivalent(StringRef JuceEquivalent::* name, StringRef value)
            {
                const auto* const found = std::ranges::find(juceEquivalents, value, name);
                return found != juceEquivalents.end() ? found : nullptr;
            }

            [[nodiscard]] static bool isCLibraryFunction(const FunctionDecl& function)
            {
                return function.getDeclContext()->getRedeclContext()->isTranslationUnit()
                    || function.isInStdNamespace();
            }

            [[nodiscard]] static bool isFromStd(const NamedDecl& found)
            {
                if (found.isInStdNamespace())
                    return true;

                const auto* shadow = dyn_cast<UsingShadowDecl>(&found);
                const auto* introducer = shadow != nullptr
                                           ? dyn_cast<UsingDecl>(shadow->getIntroducer())
                                           : nullptr;

                if (introducer == nullptr
                    || introducer->getQualifier().getKind() != NestedNameSpecifier::Kind::Namespace)
                {
                    return false;
                }

                const auto* ns = dyn_cast_or_null<NamespaceDecl>(introducer->getQualifier().getAsNamespaceAndPrefix().Namespace);
                return ns != nullptr && ns->isStdNamespace();
            }

            [[nodiscard]] static bool promotesFloatToDouble(const CallExpr& call, const FunctionDecl& function)
            {
                return llvm::any_of(llvm::zip(call.arguments(), function.parameters()),
                                    [](const auto& pair) {
                                        const auto& [argument, parameter] = pair;

                                        return argument->IgnoreParenImpCasts()->getType()->isSpecificBuiltinType(BuiltinType::Float)
                                            && parameter->getType()->isSpecificBuiltinType(BuiltinType::Double);
                                    });
            }

            [[nodiscard]] static bool argumentsMatchParameters(const CallExpr& call, const FunctionDecl& function)
            {
                if (call.getNumArgs() != function.getNumParams())
                    return false;

                return llvm::all_of(llvm::zip(call.arguments(), function.parameters()),
                                    [](const auto& pair) {
                                        const auto& [argument, parameter] = pair;

                                        return argument->IgnoreParenImpCasts()->getType().getCanonicalType().getUnqualifiedType()
                                            == parameter->getType().getCanonicalType().getUnqualifiedType();
                                    });
            }

            [[nodiscard]] static bool hasMatchingFloatingArguments(const CallExpr& call, const FunctionDecl& function)
            {
                const auto type = function.getParamDecl(0)->getType().getCanonicalType().getUnqualifiedType();

                return argumentsMatchParameters(call, function)
                    && type->isRealFloatingType()
                    && llvm::all_of(function.parameters(),
                                    [&type](const ParmVarDecl* parameter) {
                                        return parameter->getType().getCanonicalType().getUnqualifiedType() == type;
                                    });
            }

            [[nodiscard]] static const InitListExpr* getInitialiserList(const Expr& argument)
            {
                const auto* list = dyn_cast<CXXStdInitializerListExpr>(argument.IgnoreImplicit());

                return list != nullptr
                         ? dyn_cast<InitListExpr>(list->getSubExpr()->IgnoreImplicit())
                         : nullptr;
            }

            [[nodiscard]] static const InitListExpr* getOnlyInitialiserList(const CallExpr& call)
            {
                return call.getNumArgs() == 1 ? getInitialiserList(*call.getArg(0)) : nullptr;
            }

            [[nodiscard]] static std::vector<const Expr*> getArguments(const CallExpr& call)
            {
                if (const auto* list = getOnlyInitialiserList(call))
                    return { list->inits().begin(), list->inits().end() };

                return { call.arguments().begin(), call.arguments().end() };
            }

            [[nodiscard]] static bool acceptsArguments(const JuceEquivalent& equivalent, const CallExpr& call)
            {
                if (const auto* list = getOnlyInitialiserList(call))
                {
                    return equivalent.form == Form::variadic
                        && list->getNumInits() >= 2
                        && list->getNumInits() <= 4;
                }

                return call.getNumArgs() == equivalent.numArguments
                    && llvm::none_of(call.arguments(),
                                     [](const Expr* argument) {
                                         return getInitialiserList(*argument) != nullptr;
                                     });
            }

            [[nodiscard]] static std::optional<Function> getFunction(const CallExpr& call,
                                                                     const DeclRefExpr& reference,
                                                                     bool isJuce)
            {
                const auto& function = *cast<FunctionDecl>(reference.getDecl());
                const auto name = function.getName();

                if (isJuce)
                {
                    const auto* equivalent = findEquivalent(&JuceEquivalent::juceName, name);

                    return Function{
                        .spelling = ("juce::" + name).str(),
                        .standardName = equivalent->standardName.str(),
                        .equivalent = equivalent,
                        .isJuce = true,
                        .isStandard = false,
                    };
                }

                const auto& cNames = getStandardNames();
                const auto cName = cNames.find(name);
                const auto writtenAsStd = isFromStd(*reference.getFoundDecl());
                const auto isCFunction = cName != cNames.end() && isCLibraryFunction(function);

                if ((!isCFunction && !writtenAsStd)
                    || promotesFloatToDouble(call, function))
                {
                    return std::nullopt;
                }

                const auto standardName = cName != cNames.end() ? cName->second : name.str();
                const auto* equivalent = findEquivalent(&JuceEquivalent::standardName, standardName);

                return Function{
                    .spelling = ((writtenAsStd ? "std::" : "") + name).str(),
                    .standardName = standardName,
                    .equivalent = equivalent != nullptr && acceptsArguments(*equivalent, call)
                                    ? equivalent
                                    : nullptr,
                    .isJuce = false,
                    .isStandard = name == standardName && writtenAsStd,
                };
            }

            [[nodiscard]] static bool isSafeToReplace(const CallExpr& call,
                                                      const DeclRefExpr& reference,
                                                      const Function& function,
                                                      bool toJuce)
            {
                const auto& declaration = *cast<FunctionDecl>(reference.getDecl());

                if (function.equivalent == nullptr || (!function.isJuce && !toJuce))
                    return argumentsMatchParameters(call, declaration);

                if (function.equivalent->form == Form::maths)
                    return !reference.hasExplicitTemplateArgs() && hasMatchingFloatingArguments(call, declaration);

                const auto addsBraces = !toJuce && call.getNumArgs() > function.equivalent->numArguments;
                return !addsBraces || !reference.hasExplicitTemplateArgs();
            }

            [[nodiscard]] static std::optional<std::string> joinArguments(ArrayRef<const Expr*> arguments,
                                                                          bool braced,
                                                                          const SourceManager& sourceManager,
                                                                          const LangOptions& langOptions)
            {
                std::vector<StringRef> texts;

                for (const auto* argument : arguments)
                {
                    const auto text = utils::getSourceText(argument->getSourceRange(), sourceManager, langOptions);

                    if (!text)
                        return std::nullopt;

                    texts.push_back(*text);
                }

                const auto joined = llvm::join(texts, ", ");
                return braced ? "{ " + joined + " }" : joined;
            }

            [[nodiscard]] static std::optional<FixItHint> getFix(const CallExpr& call,
                                                                 const DeclRefExpr& reference,
                                                                 const Function& function,
                                                                 bool toJuce,
                                                                 StringRef replacement,
                                                                 const SourceManager& sourceManager,
                                                                 const LangOptions& langOptions)
            {
                const auto name = utils::getFileRange({ reference.getBeginLoc(), reference.getNameInfo().getEndLoc() },
                                                      sourceManager,
                                                      langOptions);

                if (!name)
                    return std::nullopt;

                auto rename = FixItHint::CreateReplacement(*name, replacement);

                if (function.equivalent == nullptr)
                    return rename;

                auto arguments = getArguments(call);
                const auto braced = !toJuce && arguments.size() > function.equivalent->numArguments;
                const auto isClamp = function.equivalent->form == Form::clamp;

                if (braced == (getOnlyInitialiserList(call) != nullptr) && !isClamp)
                    return rename;

                if (isClamp)
                    std::ranges::rotate(arguments, toJuce ? arguments.begin() + 1 : arguments.end() - 1);

                const auto text = joinArguments(arguments, braced, sourceManager, langOptions);
                const auto written = utils::getFileRange({ call.getArg(0)->getBeginLoc(), call.getArg(call.getNumArgs() - 1)->getEndLoc() },
                                                         sourceManager,
                                                         langOptions);
                const auto whole = utils::getFileRange({ reference.getBeginLoc(), call.getRParenLoc() },
                                                       sourceManager,
                                                       langOptions);

                if (!text || !written || !whole)
                    return std::nullopt;

                const auto between = [&](SourceLocation begin, SourceLocation end) {
                    return Lexer::getSourceText(CharSourceRange::getCharRange(begin, end), sourceManager, langOptions);
                };

                return FixItHint::CreateReplacement(*whole,
                                                    (replacement + between(name->getEnd(), written->getBegin()) + *text + between(written->getEnd(), whole->getEnd())).str());
            }

            const bool preferJuceFunctions;
            tidy::utils::IncludeInserter includeInserter;
        };
    } // namespace std_math_functions

    using std_math_functions::StdMathFunctionsCheck;
} // namespace clang::tidy::juce
