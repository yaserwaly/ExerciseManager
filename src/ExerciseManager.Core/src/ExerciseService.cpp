#include "exercise_manager/ExerciseService.h"

#include "exercise_manager/Errors.h"
#include "exercise_manager/FileNaming.h"
#include "exercise_manager/NameValidator.h"

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

int ExerciseService::addExercise(int courseId, const std::string& name,
                                 const std::string& description, const std::string& fullDescription)
{
    NameValidator::validate(name);

    Course course = m_repository.load(courseId);

    const bool duplicate =
        std::any_of(course.exercises.begin(), course.exercises.end(),
                    [&name](const Exercise& exercise) { return exercise.name == name; });
    if (duplicate)
    {
        throw DuplicateExerciseError("An exercise with this name already exists.");
    }

    const int number = nextExerciseNumber(course);
    course.exercises.push_back(Exercise{number, name, description, fullDescription});
    m_repository.save(course);
    return number;
}

} // namespace exercise_manager
