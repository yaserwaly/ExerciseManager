#pragma once

#include "exercise_manager/Course.h"
#include "exercise_manager/ICourseRepository.h"

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

  private:
    ICourseRepository& m_repository;
};

} // namespace exercise_manager
