#pragma once

#include "exercise_manager/Errors.h"
#include "exercise_manager/IBuildRunner.h"

#include <string>
#include <utility>
#include <vector>

namespace exercise_manager::cli::test
{

// Build runner for tests: it builds nothing, it only records the calls.
// Set errorMessage to make it throw BuildError, like a failed build.
class FakeBuildRunner : public IBuildRunner
{
  public:
    BuildResult buildAndRun(int courseId, int exerciseNumber) override
    {
        calls.emplace_back(courseId, exerciseNumber);
        if (!errorMessage.empty())
        {
            throw BuildError(errorMessage);
        }
        return BuildResult{exitCode};
    }

    // Every (courseId, exerciseNumber) pair that was run, in order.
    std::vector<std::pair<int, int>> calls;

    // The exit code returned when the build "succeeds".
    int exitCode = 0;

    // If not empty, buildAndRun throws BuildError with this message.
    std::string errorMessage;
};

} // namespace exercise_manager::cli::test
