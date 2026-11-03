#include "exercise_manager/FileNaming.h"

#include "exercise_manager/Errors.h"

#include <format>

namespace exercise_manager
{

namespace
{

void checkCourseId(int courseId)
{
    if (courseId < 1 || courseId > FileNaming::kMaxCourseId)
    {
        throw InvalidExerciseNameError("Course number is out of range.");
    }
}

void checkExerciseNumber(int exerciseNumber)
{
    if (exerciseNumber < 1 || exerciseNumber > FileNaming::kMaxExerciseNumber)
    {
        throw InvalidExerciseNameError("Exercise number is out of range.");
    }
}

} // namespace

std::string FileNaming::courseFolderName(int courseId)
{
    checkCourseId(courseId);
    return std::format("c{:02}", courseId);
}

std::string FileNaming::baseName(int courseId, int exerciseNumber)
{
    checkCourseId(courseId);
    checkExerciseNumber(exerciseNumber);
    return std::format("c{:02}{}{:03}", courseId, kFilePrefix, exerciseNumber);
}

std::string FileNaming::headerFileName(int courseId, int exerciseNumber)
{
    return baseName(courseId, exerciseNumber) + ".hpp";
}

std::string FileNaming::sourceFileName(int courseId, int exerciseNumber)
{
    return baseName(courseId, exerciseNumber) + ".cpp";
}

std::string FileNaming::className(int exerciseNumber)
{
    checkExerciseNumber(exerciseNumber);
    return std::format("cls{}{:03}", kFilePrefix, exerciseNumber);
}

} // namespace exercise_manager