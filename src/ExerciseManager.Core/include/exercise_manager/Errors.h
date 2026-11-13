#pragma once

#include <stdexcept>

namespace exercise_manager
{

/// @brief Base class for all errors thrown by ExerciseManager.Core.
class ExerciseManagerError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

/// @brief Thrown when an exercise does not exist in a course.
class ExerciseNotFoundError : public ExerciseManagerError
{
public:
    using ExerciseManagerError::ExerciseManagerError;
};

/// @brief Thrown when a course does not exist.
class CourseNotFoundError : public ExerciseManagerError
{
public:
    using ExerciseManagerError::ExerciseManagerError;
};

/// @brief Thrown when an exercise with the same identity already exists.
class DuplicateExerciseError : public ExerciseManagerError
{
public:
    using ExerciseManagerError::ExerciseManagerError;
};

/// @brief Thrown when a course with the same name already exists.
class DuplicateCourseError : public ExerciseManagerError
{
  public:
    using ExerciseManagerError::ExerciseManagerError;
};

/// @brief Thrown when an exercise name is not allowed.
class InvalidExerciseNameError : public ExerciseManagerError
{
public:
    using ExerciseManagerError::ExerciseManagerError;
};

/// @brief Thrown when a path or the workspace is invalid or unsafe.
class WorkspaceError : public ExerciseManagerError
{
public:
    using ExerciseManagerError::ExerciseManagerError;
};

/// @brief Thrown when reading or writing stored data fails.
class StorageError : public ExerciseManagerError
{
public:
    using ExerciseManagerError::ExerciseManagerError;
};

/// @brief Thrown when building or running an exercise fails.
class BuildError : public ExerciseManagerError
{
public:
    using ExerciseManagerError::ExerciseManagerError;
};

} // namespace exercise_manager
