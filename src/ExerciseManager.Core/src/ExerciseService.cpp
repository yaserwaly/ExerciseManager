#include "exercise_manager/ExerciseService.h"

#include "exercise_manager/Errors.h"
#include "exercise_manager/FileNaming.h"

#include <algorithm>
#include <vector>

namespace exercise_manager
{

ExerciseService::ExerciseService(ICourseRepository& repository) : m_repository(repository)
{
}

int ExerciseService::nextExerciseNumber(const Course& course)
{
    std::vector<int> used;
    used.reserve(course.exercises.size());
    for (const Exercise& exercise : course.exercises)
    {
        used.push_back(exercise.number);
    }
    std::sort(used.begin(), used.end());

    int candidate = 1;
    for (const int number : used)
    {
        if (number == candidate)
        {
            ++candidate;
        }
        else if (number > candidate)
        {
            break;
        }
    }

    if (candidate > FileNaming::kMaxExerciseNumber)
    {
        throw InvalidExerciseNameError("The course is full: no free exercise number.");
    }
    return candidate;
}

} // namespace exercise_manager
