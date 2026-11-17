#include "exercise_manager/cli/ConsoleUtils.h"

#include <charconv>
#include <cstddef>
#include <string>
#include <system_error>

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

} // namespace

std::optional<int> ConsoleUtils::readNumber(IConsole& console, std::string_view prompt,
                                            int minValue, int maxValue)
{
    while (true)
    {
        console.write(prompt);

        std::string line;
        if (!console.readLine(line))
        {
            return std::nullopt;
        }

        const std::string_view text = trim(line);
        int number = 0;
        const auto result = std::from_chars(text.data(), text.data() + text.size(), number);
        const bool wholeTextRead =
            result.ec == std::errc() && result.ptr == text.data() + text.size();

        if (wholeTextRead && number >= minValue && number <= maxValue)
        {
            return number;
        }

        console.writeLine("Invalid input. Please enter a number between " +
                          std::to_string(minValue) + " and " + std::to_string(maxValue) + ".");
    }
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
    console.write(prompt);

    std::string line;
    if (!console.readLine(line))
    {
        return std::nullopt;
    }
    return line;
}

std::optional<std::string> ConsoleUtils::readTextOrKeep(IConsole& console, std::string_view prompt,
                                                        std::string_view current)
{
    console.write(std::string(prompt) + " [" + std::string(current) + "]: ");

    std::string line;
    if (!console.readLine(line))
    {
        return std::nullopt;
    }
    if (trim(line).empty())
    {
        return std::string(current);
    }
    return line;
}

bool ConsoleUtils::waitForEnter(IConsole& console)
{
    console.write("Press Enter to continue...");

    std::string line;
    return console.readLine(line);
}

} // namespace exercise_manager::cli
