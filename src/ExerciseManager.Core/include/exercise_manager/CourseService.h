#pragma once

#include "exercise_manager/Course.h"
#include "exercise_manager/ICourseRepository.h"
#include "exercise_manager/Workspace.h"

#include <string>
#include <vector>

namespace exercise_manager
{

/// @brief Business rules for the courses of a workspace.
///
/// The service talks to the storage through ICourseRepository only, so it can be
/// tested with an in-memory fake and does not care about the storage format.
class CourseService
{
  public:
    /// @brief Creates a service that works on top of a repository and a workspace.
    /// @param repository The storage to use. It must outlive the service.
    /// @param workspace The workspace where the course folders live (copied).
    CourseService(ICourseRepository& repository, const Workspace& workspace);

    /// @brief Finds the first free course number.
    ///
    /// The number of a deleted course is free again, so the smallest unused
    /// number is returned (1 when there are no courses).
    /// @param usedIds The numbers of the existing courses, in any order.
    /// @return The first unused number, starting from 1.
    /// @throws InvalidExerciseNameError if all numbers up to FileNaming::kMaxCourseId are used.
    static int nextCourseId(const std::vector<int>& usedIds);

    /// @brief Adds a new, empty course.
    ///
    /// The new course gets the first free number and has no exercises.
    /// @param name Course name (checked by NameValidator).
    /// @param description Short description.
    /// @param fullDescription Detailed description.
    /// @return The number given to the new course.
    /// @throws InvalidExerciseNameError if the name is not allowed or there is no free number.
    /// @throws DuplicateCourseError if another course already uses this name.
    /// @throws StorageError if the data cannot be read or written.
    int addCourse(const std::string& name, const std::string& description,
                  const std::string& fullDescription);

    /// @brief Changes the name and descriptions of an existing course.
    ///
    /// The course number and its exercises are not touched.
    /// @param courseId The course number.
    /// @param name New name (checked by NameValidator).
    /// @param description New short description.
    /// @param fullDescription New detailed description.
    /// @throws InvalidExerciseNameError if the name is not allowed.
    /// @throws CourseNotFoundError if the course does not exist.
    /// @throws DuplicateCourseError if another course already uses this name.
    /// @throws StorageError if the data cannot be read or written.
    void updateCourse(int courseId, const std::string& name, const std::string& description,
                      const std::string& fullDescription);

    /// @brief Permanently deletes a course: its stored data and its whole folder.
    ///
    /// The number of the deleted course becomes free again. There is no undo, so the
    /// workspace Git repository is the only safety net. The confirmation question
    /// belongs to the CLI, not to this class.
    /// @param courseId The course number.
    /// @throws CourseNotFoundError if the course does not exist.
    /// @throws WorkspaceError if the folder path is outside the workspace or cannot be removed.
    /// @throws StorageError if the stored data cannot be read or written.
    void deleteCourse(int courseId);

    /// @brief Lists all courses, sorted by number.
    /// @return The stored courses, each with its exercises.
    /// @throws StorageError if the stored data cannot be read or is invalid.
    std::vector<Course> listCourses() const;

  private:
    ICourseRepository& m_repository;
    Workspace m_workspace;
};

} // namespace exercise_manager
