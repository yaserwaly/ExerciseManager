#include "exercise_manager/cli/ConsoleUtils.h"

#include "FakeConsole.h"

#include <gtest/gtest.h>

#include <string>

using exercise_manager::cli::ConsoleUtils;
using exercise_manager::cli::test::FakeConsole;

namespace
{

bool contains(const std::string& text, const std::string& part)
{
    return text.find(part) != std::string::npos;
}

} // namespace

// ---------- readNumber ----------

TEST(ConsoleUtilsTests, ReadNumberReturnsNumberInsideRange)
{
    FakeConsole console{"3"};

    EXPECT_EQ(ConsoleUtils::readNumber(console, "Choose: ", 1, 5), 3);
}

TEST(ConsoleUtilsTests, ReadNumberAcceptsLimits)
{
    FakeConsole low{"1"};
    FakeConsole high{"5"};

    EXPECT_EQ(ConsoleUtils::readNumber(low, "Choose: ", 1, 5), 1);
    EXPECT_EQ(ConsoleUtils::readNumber(high, "Choose: ", 1, 5), 5);
}

TEST(ConsoleUtilsTests, ReadNumberIgnoresSpacesAroundNumber)
{
    FakeConsole console{"  4  "};

    EXPECT_EQ(ConsoleUtils::readNumber(console, "Choose: ", 1, 5), 4);
}

TEST(ConsoleUtilsTests, ReadNumberAsksAgainAfterText)
{
    FakeConsole console{"abc", "2"};

    EXPECT_EQ(ConsoleUtils::readNumber(console, "Choose: ", 1, 5), 2);
    EXPECT_TRUE(
        contains(console.output(), "Invalid input. Please enter a number between 1 and 5.\n"));
}

TEST(ConsoleUtilsTests, ReadNumberAsksAgainAfterNumberOutOfRange)
{
    FakeConsole console{"0", "6", "5"};

    EXPECT_EQ(ConsoleUtils::readNumber(console, "Choose: ", 1, 5), 5);
}

TEST(ConsoleUtilsTests, ReadNumberRejectsPartialNumbers)
{
    FakeConsole console{"3x", "2.5", "", "2"};

    EXPECT_EQ(ConsoleUtils::readNumber(console, "Choose: ", 1, 5), 2);
}

TEST(ConsoleUtilsTests, ReadNumberReturnsNothingWhenInputEnds)
{
    FakeConsole console;

    EXPECT_FALSE(ConsoleUtils::readNumber(console, "Choose: ", 1, 5).has_value());
}

TEST(ConsoleUtilsTests, ReadNumberReturnsNothingWhenInputEndsAfterBadInput)
{
    FakeConsole console{"abc"};

    EXPECT_FALSE(ConsoleUtils::readNumber(console, "Choose: ", 1, 5).has_value());
}

TEST(ConsoleUtilsTests, ReadNumberShowsThePrompt)
{
    FakeConsole console{"1"};

    ConsoleUtils::readNumber(console, "Choose: ", 1, 5);

    EXPECT_TRUE(contains(console.output(), "Choose: "));
}

// ---------- askYesNo ----------

TEST(ConsoleUtilsTests, AskYesNoAcceptsYesInAnyCase)
{
    FakeConsole lower{"y"};
    FakeConsole upper{"Y"};

    EXPECT_EQ(ConsoleUtils::askYesNo(lower, "Delete?"), true);
    EXPECT_EQ(ConsoleUtils::askYesNo(upper, "Delete?"), true);
}

TEST(ConsoleUtilsTests, AskYesNoAcceptsNoInAnyCase)
{
    FakeConsole lower{"n"};
    FakeConsole upper{"N"};

    EXPECT_EQ(ConsoleUtils::askYesNo(lower, "Delete?"), false);
    EXPECT_EQ(ConsoleUtils::askYesNo(upper, "Delete?"), false);
}

TEST(ConsoleUtilsTests, AskYesNoAsksAgainOnBadInput)
{
    FakeConsole console{"maybe", "y"};

    EXPECT_EQ(ConsoleUtils::askYesNo(console, "Delete?"), true);
    EXPECT_TRUE(contains(console.output(), "Invalid input. Please enter 'y' or 'n'.\n"));
}

TEST(ConsoleUtilsTests, AskYesNoShowsTheQuestionWithChoices)
{
    FakeConsole console{"n"};

    ConsoleUtils::askYesNo(console, "Delete?");

    EXPECT_TRUE(contains(console.output(), "Delete? (y/n): "));
}

TEST(ConsoleUtilsTests, AskYesNoReturnsNothingWhenInputEnds)
{
    FakeConsole console;

    EXPECT_FALSE(ConsoleUtils::askYesNo(console, "Delete?").has_value());
}

// ---------- readText ----------

TEST(ConsoleUtilsTests, ReadTextReturnsTheLineAsTyped)
{
    FakeConsole console{"  my name "};

    EXPECT_EQ(ConsoleUtils::readText(console, "Name: "), "  my name ");
}

TEST(ConsoleUtilsTests, ReadTextAcceptsEmptyLine)
{
    FakeConsole console{""};

    const auto text = ConsoleUtils::readText(console, "Name: ");

    ASSERT_TRUE(text.has_value());
    EXPECT_TRUE(text->empty());
}

TEST(ConsoleUtilsTests, ReadTextReturnsNothingWhenInputEnds)
{
    FakeConsole console;

    EXPECT_FALSE(ConsoleUtils::readText(console, "Name: ").has_value());
}

// ---------- readTextOrKeep ----------

TEST(ConsoleUtilsTests, ReadTextOrKeepReturnsTheTypedText)
{
    FakeConsole console{"new"};

    EXPECT_EQ(ConsoleUtils::readTextOrKeep(console, "Name", "old"), "new");
}

TEST(ConsoleUtilsTests, ReadTextOrKeepKeepsTheCurrentValueOnAnEmptyLine)
{
    FakeConsole console{""};

    EXPECT_EQ(ConsoleUtils::readTextOrKeep(console, "Name", "old"), "old");
}

TEST(ConsoleUtilsTests, ReadTextOrKeepKeepsTheCurrentValueWhenOnlySpacesAreTyped)
{
    FakeConsole console{"   "};

    EXPECT_EQ(ConsoleUtils::readTextOrKeep(console, "Name", "old"), "old");
}

TEST(ConsoleUtilsTests, ReadTextOrKeepShowsTheCurrentValueInThePrompt)
{
    FakeConsole console{""};

    ConsoleUtils::readTextOrKeep(console, "Name", "old");

    EXPECT_TRUE(contains(console.output(), "Name [old]: "));
}

TEST(ConsoleUtilsTests, ReadTextOrKeepReturnsNothingWhenInputEnds)
{
    FakeConsole console;

    EXPECT_FALSE(ConsoleUtils::readTextOrKeep(console, "Name", "old").has_value());
}

// ---------- waitForEnter ----------

TEST(ConsoleUtilsTests, WaitForEnterReturnsTrueWhenALineIsRead)
{
    FakeConsole console{""};

    EXPECT_TRUE(ConsoleUtils::waitForEnter(console));
    EXPECT_TRUE(contains(console.output(), "Press Enter to continue..."));
}

TEST(ConsoleUtilsTests, WaitForEnterReturnsFalseWhenInputEnds)
{
    FakeConsole console;

    EXPECT_FALSE(ConsoleUtils::waitForEnter(console));
}

TEST(ConsoleUtilsTests, WaitForEnterShowsAnEmptyLineBeforeThePrompt)
{
    FakeConsole console{""};

    ConsoleUtils::waitForEnter(console);

    EXPECT_EQ(console.output(), "\nPress Enter to continue...");
}

// ---------- cancel word (opt-in) ----------

using exercise_manager::cli::CancellableInput;
using exercise_manager::cli::InputStatus;

TEST(ConsoleUtilsTests, ReadNumberOrCancelReturnsNumberInsideRange)
{
    FakeConsole console{"3"};

    const auto result = ConsoleUtils::readNumberOrCancel(console, "Choose: ", 1, 5);

    EXPECT_EQ(result.status, InputStatus::Value);
    EXPECT_EQ(result.value, 3);
}

TEST(ConsoleUtilsTests, ReadNumberOrCancelReturnsCancelledForTheCancelWord)
{
    FakeConsole console{"/c"};

    EXPECT_EQ(ConsoleUtils::readNumberOrCancel(console, "Choose: ", 1, 5).status,
              InputStatus::Cancelled);
}

TEST(ConsoleUtilsTests, ReadNumberOrCancelAcceptsTheCancelWordInAnyCase)
{
    FakeConsole console{"/C"};

    EXPECT_EQ(ConsoleUtils::readNumberOrCancel(console, "Choose: ", 1, 5).status,
              InputStatus::Cancelled);
}

TEST(ConsoleUtilsTests, ReadNumberOrCancelIgnoresSpacesAroundTheCancelWord)
{
    FakeConsole console{"  /c  "};

    EXPECT_EQ(ConsoleUtils::readNumberOrCancel(console, "Choose: ", 1, 5).status,
              InputStatus::Cancelled);
}

TEST(ConsoleUtilsTests, ReadNumberOrCancelReturnsEndOfInputWhenInputEnds)
{
    FakeConsole console;

    EXPECT_EQ(ConsoleUtils::readNumberOrCancel(console, "Choose: ", 1, 5).status,
              InputStatus::EndOfInput);
}

TEST(ConsoleUtilsTests, ReadNumberOrCancelAsksAgainAfterBadInput)
{
    FakeConsole console{"abc", "2"};

    const auto result = ConsoleUtils::readNumberOrCancel(console, "Choose: ", 1, 5);

    EXPECT_EQ(result.status, InputStatus::Value);
    EXPECT_EQ(result.value, 2);
    EXPECT_TRUE(
        contains(console.output(), "Invalid input. Please enter a number between 1 and 5.\n"));
}

TEST(ConsoleUtilsTests, ReadNumberOrCancelCancelsAfterBadInput)
{
    FakeConsole console{"abc", "/c"};

    EXPECT_EQ(ConsoleUtils::readNumberOrCancel(console, "Choose: ", 1, 5).status,
              InputStatus::Cancelled);
}

TEST(ConsoleUtilsTests, ReadNumberOrCancelDoesNotShowAnErrorForTheCancelWord)
{
    FakeConsole console{"/c"};

    ConsoleUtils::readNumberOrCancel(console, "Choose: ", 1, 5);

    EXPECT_FALSE(contains(console.output(), "Invalid input"));
}

TEST(ConsoleUtilsTests, ReadNumberOrCancelDoesNotTreatLongerTextAsTheCancelWord)
{
    FakeConsole console{"/cx", "/c"};

    EXPECT_EQ(ConsoleUtils::readNumberOrCancel(console, "Choose: ", 1, 5).status,
              InputStatus::Cancelled);
    EXPECT_TRUE(contains(console.output(), "Invalid input."));
}

TEST(ConsoleUtilsTests, ReadNumberStillTreatsTheCancelWordAsBadInput)
{
    FakeConsole console{"/c", "2"};

    EXPECT_EQ(ConsoleUtils::readNumber(console, "Choose: ", 1, 5), 2);
    EXPECT_TRUE(contains(console.output(), "Invalid input."));
}

TEST(ConsoleUtilsTests, ReadTextOrCancelReturnsTheLineAsTyped)
{
    FakeConsole console{"  my name "};

    const auto result = ConsoleUtils::readTextOrCancel(console, "Name: ");

    EXPECT_EQ(result.status, InputStatus::Value);
    EXPECT_EQ(result.value, "  my name ");
}

TEST(ConsoleUtilsTests, ReadTextOrCancelAcceptsAnEmptyLine)
{
    FakeConsole console{""};

    const auto result = ConsoleUtils::readTextOrCancel(console, "Name: ");

    EXPECT_EQ(result.status, InputStatus::Value);
    EXPECT_TRUE(result.value.empty());
}

TEST(ConsoleUtilsTests, ReadTextOrCancelReturnsCancelledForTheCancelWord)
{
    FakeConsole console{" /C "};

    EXPECT_EQ(ConsoleUtils::readTextOrCancel(console, "Name: ").status, InputStatus::Cancelled);
}

TEST(ConsoleUtilsTests, ReadTextOrCancelReturnsEndOfInputWhenInputEnds)
{
    FakeConsole console;

    EXPECT_EQ(ConsoleUtils::readTextOrCancel(console, "Name: ").status, InputStatus::EndOfInput);
}

TEST(ConsoleUtilsTests, ReadTextStillReturnsTheCancelWordAsText)
{
    FakeConsole console{"/c"};

    EXPECT_EQ(ConsoleUtils::readText(console, "Name: "), "/c");
}

TEST(ConsoleUtilsTests, ReadTextOrKeepOrCancelReturnsTheTypedText)
{
    FakeConsole console{"new"};

    const auto result = ConsoleUtils::readTextOrKeepOrCancel(console, "Name", "old");

    EXPECT_EQ(result.status, InputStatus::Value);
    EXPECT_EQ(result.value, "new");
}

TEST(ConsoleUtilsTests, ReadTextOrKeepOrCancelKeepsTheCurrentValueOnAnEmptyLine)
{
    FakeConsole console{""};

    const auto result = ConsoleUtils::readTextOrKeepOrCancel(console, "Name", "old");

    EXPECT_EQ(result.status, InputStatus::Value);
    EXPECT_EQ(result.value, "old");
}

TEST(ConsoleUtilsTests, ReadTextOrKeepOrCancelReturnsCancelledForTheCancelWord)
{
    FakeConsole console{"/c"};

    EXPECT_EQ(ConsoleUtils::readTextOrKeepOrCancel(console, "Name", "old").status,
              InputStatus::Cancelled);
}

TEST(ConsoleUtilsTests, ReadTextOrKeepOrCancelReturnsEndOfInputWhenInputEnds)
{
    FakeConsole console;

    EXPECT_EQ(ConsoleUtils::readTextOrKeepOrCancel(console, "Name", "old").status,
              InputStatus::EndOfInput);
}

TEST(ConsoleUtilsTests, ReadTextOrKeepOrCancelShowsTheCurrentValueInThePrompt)
{
    FakeConsole console{""};

    ConsoleUtils::readTextOrKeepOrCancel(console, "Name", "old");

    EXPECT_TRUE(contains(console.output(), "Name [old]: "));
}

TEST(ConsoleUtilsTests, ReadTextOrKeepStillReturnsTheCancelWordAsText)
{
    FakeConsole console{"/c"};

    EXPECT_EQ(ConsoleUtils::readTextOrKeep(console, "Name", "old"), "/c");
}

TEST(ConsoleUtilsTests, CancelHintContainsTheCancelWord)
{
    EXPECT_TRUE(contains(ConsoleUtils::cancelHint(), std::string(ConsoleUtils::kCancelInput)));
}

TEST(ConsoleUtilsTests, CancelHintHasTheAgreedFormat)
{
    EXPECT_EQ(ConsoleUtils::cancelHint(), "(/c to cancel)");
}

TEST(ConsoleUtilsTests, CancelNoticeContainsTheCancelWord)
{
    EXPECT_TRUE(contains(ConsoleUtils::cancelNotice(), std::string(ConsoleUtils::kCancelInput)));
}

TEST(ConsoleUtilsTests, CancelNoticeHasTheAgreedFormat)
{
    EXPECT_EQ(ConsoleUtils::cancelNotice(), "Type /c at any question to cancel.");
}
