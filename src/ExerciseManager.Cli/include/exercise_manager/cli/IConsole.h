#pragma once

#include <string>
#include <string_view>

namespace exercise_manager::cli
{

/// @brief Abstract console used by the CLI.
///
/// The navigation logic talks to this interface only, so it can be tested with an
/// in-memory fake instead of a real console.
class IConsole
{
  public:
    virtual ~IConsole() = default;

    /// @brief Writes text without adding a line break.
    /// @param text The text to write.
    virtual void write(std::string_view text) = 0;

    /// @brief Writes text followed by a line break.
    /// @param text The text to write.
    virtual void writeLine(std::string_view text) = 0;

    /// @brief Reads one line, without its line break.
    /// @param line Receives the line that was read.
    /// @return false if there is no more input (end of input), otherwise true.
    virtual bool readLine(std::string& line) = 0;

    /// @brief Clears the screen.
    virtual void clearScreen() = 0;
};

} // namespace exercise_manager::cli
