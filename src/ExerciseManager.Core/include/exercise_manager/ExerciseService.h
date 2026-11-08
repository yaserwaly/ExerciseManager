#pragma once

#include "exercise_manager/Course.h"
#include "exercise_manager/ICourseRepository.h"

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
    /// @brief Creates a service that works on top of a repository.
    /// @param repository The storage to use. It must outlive the service.
    explicit ExerciseService(ICourseRepository& repository);

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

  private:
    ICourseRepository& m_repository;
};

} // namespace exercise_manager
