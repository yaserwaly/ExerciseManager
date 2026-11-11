#pragma once

namespace exercise_manager
{

/// @brief Result of building and running an exercise.
///
/// It is only returned when the build succeeded. A failed build throws BuildError
/// instead, with the compiler output in its message.
struct BuildResult
{
    /// @brief Exit code of the exercise program.
    int exitCode = 0;
};

/// @brief Abstract builder that compiles and runs one exercise on demand.
///
/// Only the chosen exercise (plus the shared libraries it needs) is built, so the
/// main solution never grows with the number of exercises. The exercise talks to
/// the console directly, because it may read input, so its output is not captured.
class IBuildRunner
{
  public:
    virtual ~IBuildRunner() = default;

    /// @brief Builds one exercise and runs it.
    /// @param courseId The course number.
    /// @param exerciseNumber The exercise number.
    /// @return The exit code of the exercise program.
    /// @throws InvalidExerciseNameError if a number is out of range.
    /// @throws WorkspaceError if a path is outside the workspace or a source file is missing.
    /// @throws BuildError if the compiler cannot be found or the build fails
    ///         (the message contains the compiler output).
    virtual BuildResult buildAndRun(int courseId, int exerciseNumber) = 0;
};

} // namespace exercise_manager
