#pragma once

#include "exercise_manager/cli/IConsole.h"

#include <optional>
#include <string>
#include <string_view>

namespace exercise_manager::cli
{

/// @brief Small input helpers on top of IConsole.
///
/// When the input ends, every helper returns an empty result (or false), so a loop
/// that uses them can stop instead of waiting forever.
class ConsoleUtils
{
  public:
    /// @brief Asks for a whole number inside a range, and asks again on bad input.
    /// @param console The console to use.
    /// @param prompt The text shown before reading.
    /// @param minValue Smallest allowed number.
    /// @param maxValue Largest allowed number.
    /// @return The number, or no value if the input ended.
    static std::optional<int> readNumber(IConsole& console, std::string_view prompt, int minValue,
                                         int maxValue);

    /// @brief Asks a yes/no question (y or n, any case), and asks again on bad input.
    /// @param console The console to use.
    /// @param question The question, without the "(y/n)" part.
    /// @return true for yes, false for no, or no value if the input ended.
    static std::optional<bool> askYesNo(IConsole& console, std::string_view question);

    /// @brief Reads one line of text exactly as typed (nothing is trimmed).
    /// @param console The console to use.
    /// @param prompt The text shown before reading.
    /// @return The line, or no value if the input ended.
    static std::optional<std::string> readText(IConsole& console, std::string_view prompt);

    /// @brief Shows "Press Enter to continue..." and waits for Enter.
    /// @param console The console to use.
    /// @return false if the input ended, otherwise true.
    static bool waitForEnter(IConsole& console);
};

} // namespace exercise_manager::cli
