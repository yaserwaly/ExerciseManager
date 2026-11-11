#include "exercise_manager/TemplateEngine.h"
#include "exercise_manager/Errors.h"
#include "exercise_manager/Templates.h"

#include <gtest/gtest.h>

#include <string>

using exercise_manager::StorageError;
using exercise_manager::TemplateEngine;
using exercise_manager::Templates;

TEST(TemplateEngineTests, ReplacesOnePlaceholder)
{
    EXPECT_EQ(
        TemplateEngine::render("namespace {{Namespace}} {", {{"Namespace", "C01Exercise001"}}),
        "namespace C01Exercise001 {");
}

TEST(TemplateEngineTests, ReplacesDifferentPlaceholders)
{
    const TemplateEngine::Values values{{"Namespace", "C02Exercise013"},
                                        {"ClassName", "ClsC02Exercise013"}};
    EXPECT_EQ(TemplateEngine::render("{{Namespace}}::{{ClassName}}::Run();", values),
              "C02Exercise013::ClsC02Exercise013::Run();");
}

TEST(TemplateEngineTests, ReplacesSamePlaceholderTwice)
{
    EXPECT_EQ(TemplateEngine::render("{{A}}-{{A}}", {{"A", "x"}}), "x-x");
}

TEST(TemplateEngineTests, KeepsTextWithoutPlaceholders)
{
    EXPECT_EQ(TemplateEngine::render("int main() { return 0; }", {}), "int main() { return 0; }");
}

TEST(TemplateEngineTests, EmptyTemplateGivesEmptyText)
{
    EXPECT_EQ(TemplateEngine::render("", {}), "");
}

TEST(TemplateEngineTests, UnusedValuesAreIgnored)
{
    EXPECT_EQ(TemplateEngine::render("{{A}}", {{"A", "1"}, {"B", "2"}}), "1");
}

TEST(TemplateEngineTests, KeepsNewlinesAndSingleBraces)
{
    const std::string text = "namespace {{N}} {\n    class C {};\n}\n";
    EXPECT_EQ(TemplateEngine::render(text, {{"N", "X"}}), "namespace X {\n    class C {};\n}\n");
}

TEST(TemplateEngineTests, ValueIsNotExpandedAgain)
{
    EXPECT_EQ(TemplateEngine::render("{{A}}", {{"A", "{{B}}"}, {"B", "boom"}}), "{{B}}");
}

TEST(TemplateEngineTests, ThrowsForUnknownPlaceholder)
{
    EXPECT_THROW(TemplateEngine::render("{{Missing}}", {}), StorageError);
}

TEST(TemplateEngineTests, ThrowsForUnclosedPlaceholder)
{
    EXPECT_THROW(TemplateEngine::render("namespace {{Namespace", {{"Namespace", "x"}}),
                 StorageError);
}

TEST(TemplateEngineTests, LauncherTemplateIncludesHeaderAndCallsRun)
{
    const TemplateEngine::Values values{{"Namespace", "C02Exercise013"},
                                        {"ClassName", "ClsC02Exercise013"}};
    const std::string text = TemplateEngine::render(Templates::kLauncherSource, values);

    EXPECT_NE(text.find("#include \"C02Exercise013.hpp\""), std::string::npos);
    EXPECT_NE(text.find("int main()"), std::string::npos);
    EXPECT_NE(text.find("C02Exercise013::ClsC02Exercise013::Run();"), std::string::npos);
    EXPECT_EQ(text.find("{{"), std::string::npos);
}

TEST(TemplateEngineTests, LauncherTemplateNeedsNoNumber)
{
    const TemplateEngine::Values values{{"Namespace", "C01Exercise001"},
                                        {"ClassName", "ClsC01Exercise001"}};
    EXPECT_NO_THROW(TemplateEngine::render(Templates::kLauncherSource, values));
}
