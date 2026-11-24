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

    /// @brief Removes a course from the list of stored courses.
    ///
    /// Only the stored data is changed. Files and folders of the course are not touched.
    /// @param courseId The course number.
    /// @throws CourseNotFoundError if the course does not exist.
    /// @throws StorageError if the stored data cannot be read or written.
    virtual void remove(int courseId) = 0;

    /// @brief Lists the numbers of all stored courses, in the order they are stored.
    ///
    /// The stored order is the display order: a new course goes to the end, and saving
    /// an existing course keeps its position.
    virtual std::vector<int> listCourseIds() const = 0;

    /// @brief Stores a new order of the courses.
    ///
    /// The list must contain exactly the numbers of the stored courses, each one once, in
    /// the new order. Only the order changes: the data and the files of the courses are
    /// not touched.
    /// @param orderedIds The course numbers in the new order.
    /// @throws StorageError if the list does not match the stored courses (a missing,
    ///         unknown or repeated number), or if the stored data cannot be read or written.
    virtual void reorder(const std::vector<int>& orderedIds) = 0;
};

} // namespace exercise_manager
