#pragma once

#include "exercise_manager/Course.h"
#include "exercise_manager/ICourseRepository.h"
#include "exercise_manager/Workspace.h"

#include <string>

namespace exercise_manager
{

/// @brief Business rules for the exercises of a course.
///
/// The service talks to the storage through ICourseRepository only, so it can be
/// tested with an in-memory fake and does not care about the storage format.
class ExerciseService
{
  public:
    /// @brief Creates a service that works on top of a repository and a workspace.
    /// @param repository The storage to use. It must outlive the service.
    /// @param workspace The workspace where the exercise files live (copied).
    ExerciseService(ICourseRepository& repository, const Workspace& workspace);

    /// @brief Finds the first free exercise number in a course.
    ///
    /// The number of a deleted exercise is free again, so the smallest unused
    /// number is returned (1 for an empty course).
    /// @param course The course to look at.
    /// @return The first unused number, starting from 1.
    /// @throws InvalidExerciseNameError if the course is full (no free number up to
    ///         FileNaming::kMaxExerciseNumber).
    static int nextExerciseNumber(const Course& course);

    /// @brief Adds a new exercise to a course.
    ///
    /// The new exercise gets the first free number. Only the stored data is
    /// updated here; the source files are generated elsewhere.
    /// @param courseId The course number.
    /// @param name Exercise name (checked by NameValidator).
    /// @param description Short description.
    /// @param fullDescription Detailed description.
    /// @return The number given to the new exercise.
    /// @throws InvalidExerciseNameError if the name is not allowed or the course is full.
    /// @throws CourseNotFoundError if the course does not exist.
    /// @throws DuplicateExerciseError if the course already has an exercise with this name.
    /// @throws StorageError if the data cannot be read or written.
    int addExercise(int courseId, const std::string& name, const std::string& description,
                    const std::string& fullDescription);

    /// @brief Changes the name and descriptions of an existing exercise.
    ///
    /// The exercise is found by its number, and the number never changes, so the
    /// source files are not touched.
    /// @param courseId The course number.
    /// @param exerciseNumber The number of the exercise to change.
    /// @param name New name (checked by NameValidator).
    /// @param description New short description.
    /// @param fullDescription New detailed description.
    /// @throws InvalidExerciseNameError if the name is not allowed.
    /// @throws CourseNotFoundError if the course does not exist.
    /// @throws ExerciseNotFoundError if the course has no exercise with this number.
    /// @throws DuplicateExerciseError if another exercise already uses this name.
    /// @throws StorageError if the data cannot be read or written.
    void updateExercise(int courseId, int exerciseNumber, const std::string& name,
                        const std::string& description, const std::string& fullDescription);

    /// @brief Permanently deletes an exercise: its stored data and its source files.
    ///
    /// The number of the deleted exercise becomes free again. There is no undo,
    /// so the workspace Git repository is the only safety net.
    /// @param courseId The course number.
    /// @param exerciseNumber The number of the exercise to delete.
    /// @throws CourseNotFoundError if the course does not exist.
    /// @throws ExerciseNotFoundError if the course has no exercise with this number.
    /// @throws WorkspaceError if a file path is outside the workspace or a file cannot be removed.
    /// @throws StorageError if the data cannot be read or written.
    void deleteExercise(int courseId, int exerciseNumber);

  private:
    ICourseRepository& m_repository;
    Workspace m_workspace;
};

} // namespace exercise_manager
