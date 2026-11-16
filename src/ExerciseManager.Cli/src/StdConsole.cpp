#include "exercise_manager/cli/StdConsole.h"

#include <cstdlib>
#include <iostream>

namespace exercise_manager::cli
{

void StdConsole::write(std::string_view text)
{
    std::cout << text;
}

void StdConsole::writeLine(std::string_view text)
{
    std::cout << text << '\n';
}

bool StdConsole::readLine(std::string& line)
{
    // std::cin is tied to std::cout, so a prompt written with write() is shown first.
    return static_cast<bool>(std::getline(std::cin, line));
}

void StdConsole::clearScreen()
{
    std::system("cls");
}

} // namespace exercise_manager::cli
