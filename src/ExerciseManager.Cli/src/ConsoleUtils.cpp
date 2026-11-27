#include "exercise_manager/cli/ConsoleUtils.h"

#include <cctype>
#include <charconv>
#include <cstddef>
#include <string>
#include <system_error>
#include <utility>

namespace exercise_manager::cli
{

namespace
{

// Removes spaces, tabs and line breaks from both ends.
std::string_view trim(std::string_view text)
{
    constexpr std::string_view kSpaces = " \t\r\n";

    const std::size_t first = text.find_first_not_of(kSpaces);
    if (first == std::string_view::npos)
    {
        return {};
    }
    const std::size_t last = text.find_last_not_of(kSpaces);
    return text.substr(first, last - first + 1);
}

// True if the line is the cancel word: any letter case, spaces around it are ignored.
bool isCancelWord(std::string_view line)
{
    const std::string_view word = trim(line);
    if (word.size() != ConsoleUtils::kCancelInput.size())
    {
        return false;
    }
    for (std::size_t i = 0; i < word.size(); ++i)
    {
        const int typed = std::tolower(static_cast<unsigned char>(word[i]));
        const int expected =
            std::tolower(static_cast<unsigned char>(ConsoleUtils::kCancelInput[i]));
        if (typed != expected)
        {
            return false;
        }
    }
    return true;
}

// The shared code of readNumber and readNumberOrCancel.
// allowCancel == false: the cancel word is just bad input, like any other text.
CancellableInput<int> readNumberImpl(IConsole& console, std::string_view prompt, int minValue,
                                     int maxValue, bool allowCancel)
{
    while (true)
    {
        console.write(prompt);

        std::string line;
        if (!console.readLine(line))
        {
            return {InputStatus::EndOfInput, 0};
        }

        if (allowCancel && isCancelWord(line))
        {
            return {InputStatus::Cancelled, 0};
        }

        const std::string_view text = trim(line);
        int number = 0;
        const auto result = std::from_chars(text.data(), text.data() + text.size(), number);
        const bool wholeTextRead =
            result.ec == std::errc() && result.ptr == text.data() + text.size();

        if (wholeTextRead && number >= minValue && number <= maxValue)
        {
            return {InputStatus::Value, number};
        }

        console.writeLine("Invalid input. Please enter a number between " +
                          std::to_string(minValue) + " and " + std::to_string(maxValue) + ".");
    }
}

// The shared code of readText and readTextOrCancel.
CancellableInput<std::string> readTextImpl(IConsole& console, std::string_view prompt,
                                           bool allowCancel)
{
    console.write(prompt);

    std::string line;
    if (!console.readLine(line))
    {
        return {InputStatus::EndOfInput, {}};
    }
    if (allowCancel && isCancelWord(line))
    {
        return {InputStatus::Cancelled, {}};
    }
    return {InputStatus::Value, std::move(line)};
}

// The shared code of readTextOrKeep and readTextOrKeepOrCancel.
CancellableInput<std::string> readTextOrKeepImpl(IConsole& console, std::string_view prompt,
                                                 std::string_view current, bool allowCancel)
{
    console.write(std::string(prompt) + " [" + std::string(current) + "]: ");

    std::string line;
    if (!console.readLine(line))
    {
        return {InputStatus::EndOfInput, {}};
    }
    // The cancel word is checked before the "keep" rule, so it never keeps a value.
    if (allowCancel && isCancelWord(line))
    {
        return {InputStatus::Cancelled, {}};
    }
    if (trim(line).empty())
    {
        return {InputStatus::Value, std::string(current)};
    }
    return {InputStatus::Value, std::move(line)};
}

// Turns a result into the old "value or nothing" form. Cancelled never happens here,
// because the old functions never allow cancelling.
template <typename T> std::optional<T> toOptional(CancellableInput<T> input)
{
    if (input.status == InputStatus::Value)
    {
        return std::move(input.value);
    }
    return std::nullopt;
}

} // namespace

std::string ConsoleUtils::cancelHint()
{
    return "(" + std::string(kCancelInput) + " to cancel)";
}

std::optional<int> ConsoleUtils::readNumber(IConsole& console, std::string_view prompt,
                                            int minValue, int maxValue)
{
    return toOptional(readNumberImpl(console, prompt, minValue, maxValue, false));
}

CancellableInput<int> ConsoleUtils::readNumberOrCancel(IConsole& console, std::string_view prompt,
                                                       int minValue, int maxValue)
{
    return readNumberImpl(console, prompt, minValue, maxValue, true);
}

std::optional<bool> ConsoleUtils::askYesNo(IConsole& console, std::string_view question)
{
    while (true)
    {
        console.write(std::string(question) + " (y/n): ");

        std::string line;
        if (!console.readLine(line))
        {
            return std::nullopt;
        }

        const std::string_view text = trim(line);
        if (text == "y" || text == "Y")
        {
            return true;
        }
        if (text == "n" || text == "N")
        {
            return false;
        }

        console.writeLine("Invalid input. Please enter 'y' or 'n'.");
    }
}

std::optional<std::string> ConsoleUtils::readText(IConsole& console, std::string_view prompt)
{
    return toOptional(readTextImpl(console, prompt, false));
}

CancellableInput<std::string> ConsoleUtils::readTextOrCancel(IConsole& console,
                                                             std::string_view prompt)
{
    return readTextImpl(console, prompt, true);
}

std::optional<std::string> ConsoleUtils::readTextOrKeep(IConsole& console, std::string_view prompt,
                                                        std::string_view current)
{
    return toOptional(readTextOrKeepImpl(console, prompt, current, false));
}

CancellableInput<std::string> ConsoleUtils::readTextOrKeepOrCancel(IConsole& console,
                                                                   std::string_view prompt,
                                                                   std::string_view current)
{
    return readTextOrKeepImpl(console, prompt, current, true);
}

bool ConsoleUtils::waitForEnter(IConsole& console)
{
    console.writeLine("");
    console.write("Press Enter to continue...");

    std::string line;
    return console.readLine(line);
}

} // namespace exercise_manager::cli
