#pragma once

#include "exercise_manager/cli/IConsole.h"

#include <optional>
#include <string>
#include <string_view>

namespace exercise_manager::cli
{

/// @brief How a question that can be cancelled ended.
enum class InputStatus
{
    Value,     ///< The user typed an answer.
    Cancelled, ///< The user typed the cancel word.
    EndOfInput ///< There is no more input.
};

/// @brief The result of a question that can be cancelled.
///
/// `value` has a meaning only when `status` is InputStatus::Value.
/// @tparam T The type of the answer.
template <typename T> struct CancellableInput
{
    InputStatus status = InputStatus::EndOfInput;
    T value{};
};

/// @brief Small input helpers on top of IConsole.
///
/// When the input ends, every helper returns an empty result (or false), so a loop
/// that uses them can stop instead of waiting forever.
///
/// The helpers whose name ends with `OrCancel` also understand the cancel word
/// (kCancelInput). They tell "cancelled" and "input ended" apart. The other helpers
/// do not know the cancel word at all: for them it is normal text (or bad input).
class ConsoleUtils
{
  public:
    /// @brief The word the user types to cancel an operation. Changed in one place only.
    /// It is accepted in any letter case, with spaces around it.
    static constexpr std::string_view kCancelInput = "/c";

    /// @brief The hint shown to the user, e.g. `(/c to cancel)`.
    ///
    /// It is built from kCancelInput, so the hint and the word can never differ.
    static std::string cancelHint();

    /// @brief A line that tells the user how to cancel, e.g. `Type /c at any question to cancel.`
    ///
    /// Shown once at the top of an operation screen. It is built from kCancelInput.
    static std::string cancelNotice();

    /// @brief Asks for a whole number inside a range, and asks again on bad input.
    /// @param console The console to use.
    /// @param prompt The text shown before reading.
    /// @param minValue Smallest allowed number.
    /// @param maxValue Largest allowed number.
    /// @return The number, or no value if the input ended.
    static std::optional<int> readNumber(IConsole& console, std::string_view prompt, int minValue,
                                         int maxValue);

    /// @brief Like readNumber, but the cancel word cancels the question.
    /// @return The number, or Cancelled, or EndOfInput.
    static CancellableInput<int> readNumberOrCancel(IConsole& console, std::string_view prompt,
                                                    int minValue, int maxValue);

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

    /// @brief Like readText, but the cancel word cancels the question.
    /// @return The line as typed, or Cancelled, or EndOfInput.
    static CancellableInput<std::string> readTextOrCancel(IConsole& console,
                                                          std::string_view prompt);

    /// @brief Reads a line of text, and keeps the current value if nothing is typed.
    ///
    /// The prompt is shown with the current value in brackets, e.g. `Name [old]: `.
    /// An empty line (or a line with only spaces) keeps the current value.
    /// @param console The console to use.
    /// @param prompt The text shown before the current value.
    /// @param current The current value.
    /// @return The new text, the current value if nothing was typed, or no value if the
    ///         input ended.
    static std::optional<std::string> readTextOrKeep(IConsole& console, std::string_view prompt,
                                                     std::string_view current);

    /// @brief Like readTextOrKeep, but the cancel word cancels the question.
    /// @return The new text (or the current value), or Cancelled, or EndOfInput.
    static CancellableInput<std::string> readTextOrKeepOrCancel(IConsole& console,
                                                                std::string_view prompt,
                                                                std::string_view current);

    /// @brief Shows an empty line and "Press Enter to continue...", then waits for Enter.
    /// @param console The console to use.
    /// @return false if the input ended, otherwise true.
    static bool waitForEnter(IConsole& console);
};

} // namespace exercise_manager::cli
