#pragma once

#include <string>
#include <string_view>

namespace exercise_manager
{

/// @brief Builds file and code names for exercises from their numbers.
///
/// Names come from numbers only, never from the user's text, so a name can
/// never break a path or an `#include`. Example: course 1, exercise 1 gives
/// `c01Problem001`.
class FileNaming
{
public:
    /// @brief Fixed prefix placed before the exercise number. Changed in one place only.
    static constexpr std::string_view kFilePrefix = "Exercise";

    /// @brief Fixed prefix of a course folder name, e.g. `Course01`.
    static constexpr std::string_view kCourseFolderPrefix = "Course";

    /// @brief Largest allowed course number (two digits in the file name).
    static constexpr int kMaxCourseId = 99;

    /// @brief Largest allowed exercise number (three digits in the file name).
    static constexpr int kMaxExerciseNumber = 999;

    /// @brief Base name of an exercise: `c{course:02}{Prefix}{number:03}`.
    /// @param courseId Course number (1..kMaxCourseId).
    /// @param exerciseNumber Exercise number (1..kMaxExerciseNumber).
    /// @return For example `c02Problem013`.
    /// @throws InvalidExerciseNameError if a number is out of range.
    static std::string baseName(int courseId, int exerciseNumber);

    /// @brief Header file name, e.g. `c02Problem013.hpp`.
    static std::string headerFileName(int courseId, int exerciseNumber);

    /// @brief Source file name, e.g. `c02Problem013.cpp`.
    static std::string sourceFileName(int courseId, int exerciseNumber);

    /// @brief Folder name of a course, e.g. `c02`.
    static std::string courseFolderName(int courseId);

    /// @brief Class name used inside the generated code, e.g. `ClsC01Exercise001`.
    static std::string className(int courseId, int exerciseNumber);
};

} // namespace exercise_manager
