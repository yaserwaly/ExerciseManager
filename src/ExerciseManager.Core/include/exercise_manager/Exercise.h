#pragma once

#include <string>

namespace exercise_manager
{

/// @brief One exercise inside a course.
///
/// The exercise number is permanent and decides the file names, so renaming an
/// exercise never touches its files.
struct Exercise
{
    /// @brief Exercise number inside its course (starts at 1, never reused).
    int number = 0;

    /// @brief Short name chosen by the user (validated by NameValidator).
    std::string name;

    /// @brief Free-text description.
    std::string description;
};

} // namespace exercise_manager