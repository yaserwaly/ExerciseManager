#pragma once

#include "exercise_manager/cli/IConsole.h"

#include <deque>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>

namespace exercise_manager::cli::test
{

// In-memory console for tests: the input lines are queued, and everything written is collected.
class FakeConsole : public IConsole
{
  public:
    explicit FakeConsole(std::initializer_list<std::string> inputLines = {}) : m_input(inputLines)
    {
    }

    void addInput(std::string line)
    {
        m_input.push_back(std::move(line));
    }

    void write(std::string_view text) override
    {
        m_output.append(text);
    }

    void writeLine(std::string_view text) override
    {
        m_output.append(text);
        m_output.push_back('\n');
    }

    // Returns false when no input line is left (the input has ended).
    bool readLine(std::string& line) override
    {
        if (m_input.empty())
        {
            return false;
        }
        line = std::move(m_input.front());
        m_input.pop_front();
        return true;
    }

    void clearScreen() override
    {
        ++m_clearCount;
    }

    const std::string& output() const
    {
        return m_output;
    }

    int clearCount() const
    {
        return m_clearCount;
    }

  private:
    std::deque<std::string> m_input;
    std::string m_output;
    int m_clearCount = 0;
};

} // namespace exercise_manager::cli::test
