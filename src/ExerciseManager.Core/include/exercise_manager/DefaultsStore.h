#pragma once

#include "exercise_manager/Workspace.h"

#include <optional>

namespace exercise_manager
{

/// @brief Stores the default course and the default exercise of each course.
///
/// The defaults live in the user's workspace, never in the program's repository:
/// @code
/// Courses/DefaultCourse.json            { "schema_version": 1, "course_id": 10 }
/// Courses/Course10/DefaultExercise.json { "schema_version": 1, "exercise_number": 1 }
/// @endcode
///
/// This class stores numbers only (the real course and exercise numbers). It does not
/// check that the course or the exercise exists, because that belongs to the caller.
/// A missing file means "no default". Writing is safe: data goes to a temporary file
/// first, which then replaces the real file.
class DefaultsStore
{
  public:
    /// @brief Current version of the defaults file format.
    static constexpr int kSchemaVersion = 1;

    /// @brief Creates a store that works inside a workspace.
    /// @param workspace The workspace to read from and write to (copied).
    explicit DefaultsStore(const Workspace& workspace);

    /// @brief Reads the default course number.
    /// @return The course number, or no value if no default course is set.
    /// @throws StorageError if the file cannot be read or is invalid.
    std::optional<int> loadDefaultCourse() const;

    /// @brief Sets the default course.
    /// @param courseId The course number (1..FileNaming::kMaxCourseId).
    /// @throws InvalidExerciseNameError if the number is out of range.
    /// @throws StorageError if the file cannot be written.
    void saveDefaultCourse(int courseId);

    /// @brief Removes the default course. Does nothing if there is none.
    /// @throws StorageError if the file cannot be removed.
    void clearDefaultCourse();

    /// @brief Reads the default exercise number of a course.
    /// @param courseId The course number.
    /// @return The exercise number, or no value if the course has no default exercise.
    /// @throws InvalidExerciseNameError if the course number is out of range.
    /// @throws StorageError if the file cannot be read or is invalid.
    std::optional<int> loadDefaultExercise(int courseId) const;

    /// @brief Sets the default exercise of a course.
    /// @param courseId The course number.
    /// @param exerciseNumber The exercise number (1..FileNaming::kMaxExerciseNumber).
    /// @throws InvalidExerciseNameError if a number is out of range.
    /// @throws WorkspaceError if the course folder does not exist.
    /// @throws StorageError if the file cannot be written.
    void saveDefaultExercise(int courseId, int exerciseNumber);

    /// @brief Removes the default exercise of a course. Does nothing if there is none.
    /// @param courseId The course number.
    /// @throws InvalidExerciseNameError if the course number is out of range.
    /// @throws StorageError if the file cannot be removed.
    void clearDefaultExercise(int courseId);

  private:
    Workspace m_workspace;
};

} // namespace exercise_manager
