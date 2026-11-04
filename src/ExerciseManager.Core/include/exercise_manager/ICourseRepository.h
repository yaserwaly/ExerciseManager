#pragma once

#include "exercise_manager/Course.h"

#include <vector>

namespace exercise_manager
{

/// @brief Abstract storage for courses.
///
/// The services talk to this interface only, so the storage format (JSON today)
/// can change without touching the rules, and tests can use an in-memory fake.
class ICourseRepository
{
  public:
    virtual ~ICourseRepository() = default;

    /// @brief Loads one course.
    /// @param courseId The course number.
    /// @return The stored course.
    /// @throws CourseNotFoundError if the course does not exist.
    /// @throws StorageError if the stored data cannot be read or is invalid.
    virtual Course load(int courseId) const = 0;

    /// @brief Saves a course, creating it if it does not exist yet.
    /// @param course The course to store.
    /// @throws StorageError if the data cannot be written.
    virtual void save(const Course& course) = 0;

    /// @brief Checks whether a course exists in the storage.
    /// @param courseId The course number.
    virtual bool exists(int courseId) const = 0;

    /// @brief Lists the numbers of all stored courses, sorted ascending.
    virtual std::vector<int> listCourseIds() const = 0;
};

} // namespace exercise_manager
