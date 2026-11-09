#pragma once

#include "exercise_manager/Workspace.h"

namespace exercise_manager
{

/// @brief Generates the source files (`.hpp` and `.cpp`) of an exercise.
///
/// The text comes from Templates, filled in by TemplateEngine. All names come from
/// FileNaming (numbers only), and every path is checked by the workspace guard.
/// Only the files are created here; the exercise data is stored elsewhere.
class CodeGenerator
{
  public:
    /// @brief Creates a generator that writes into a workspace.
    /// @param workspace The workspace where the files are created (copied).
    explicit CodeGenerator(const Workspace& workspace);

    /// @brief Creates the header and source file of an exercise in its course folder.
    ///
    /// The course folder is created if it does not exist. Existing files are never
    /// overwritten: if either file already exists, nothing is written.
    /// @param courseId The course number.
    /// @param exerciseNumber The exercise number.
    /// @throws InvalidExerciseNameError if a number is out of range.
    /// @throws WorkspaceError if a file path is outside the workspace or a file already exists.
    /// @throws StorageError if a file cannot be written.
    void generateExerciseFiles(int courseId, int exerciseNumber) const;

  private:
    Workspace m_workspace;
};

} // namespace exercise_manager
