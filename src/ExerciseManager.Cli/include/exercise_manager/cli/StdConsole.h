#pragma once

#include "exercise_manager/cli/IConsole.h"

#include <string>
#include <string_view>

namespace exercise_manager::cli
{

/// @brief The real console: reads from `std::cin` and writes to `std::cout`.
class StdConsole : public IConsole
{
  public:
    /// @copydoc IConsole::write
    void write(std::string_view text) override;

    /// @copydoc IConsole::writeLine
    void writeLine(std::string_view text) override;

    /// @copydoc IConsole::readLine
    bool readLine(std::string& line) override;

    /// @brief Clears the console window (Windows only, uses the `cls` command).
    void clearScreen() override;
};

} // namespace exercise_manager::cli
