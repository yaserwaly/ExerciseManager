#pragma once

#include "exercise_manager/Exercise.h"

#include <string>
#include <vector>

namespace exercise_manager
{

/// @brief A course: a numbered group of exercises.
///
/// This is plain data. The rules (numbering, adding, deleting) live in the
/// services, and the storage format lives in the repository.
struct Course
{
    /// @brief Course number (1..10 and so on). Used in file names, e.g. c01.
    int id = 0;

    /// @brief Display name of the course.
    std::string name;

    /// @brief Number given to the next exercise. Numbers of deleted exercises are never reused.
    int nextExerciseNumber = 1;

    /// @brief The exercises of this course.
    std::vector<Exercise> exercises;

    /// @brief Short description of the course.
    std::string description;
};

} // namespace exercise_manager
