#pragma once

#include "exercise_manager/IBuildRunner.h"
#include "exercise_manager/Workspace.h"

namespace exercise_manager
{

/// @brief Builds and runs an exercise with the Microsoft C++ compiler (`cl.exe`).
///
/// Visual Studio is found with `vswhere.exe`, and its environment is loaded with
/// `vcvarsall.bat` in the same command line that runs `cl`. So the program does not
/// need to be started from a Developer Command Prompt.
class MsvcBuildRunner : public IBuildRunner
{
  public:
    /// @brief Creates a runner that builds inside a workspace.
    /// @param workspace The workspace with the courses and the build folder (copied).
    explicit MsvcBuildRunner(const Workspace& workspace);

    /// @brief Finds `vcvarsall.bat` of the newest Visual Studio with the C++ tools.
    /// @return The full path of `vcvarsall.bat`.
    /// @throws BuildError if `vswhere.exe` or `vcvarsall.bat` cannot be found.
    static std::filesystem::path findVcvarsall();

    /// @copydoc IBuildRunner::buildAndRun
    BuildResult buildAndRun(int courseId, int exerciseNumber) override;

  private:
    Workspace m_workspace;
};

} // namespace exercise_manager
