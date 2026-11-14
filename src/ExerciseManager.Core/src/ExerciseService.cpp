#include "exercise_manager/ExerciseService.h"

#include "exercise_manager/Errors.h"
#include "exercise_manager/FileNaming.h"
#include "exercise_manager/NameValidator.h"

#include <algorithm>
#include <vector>
#include <filesystem>
#include <system_error>

namespace exercise_manager
{

ExerciseService::ExerciseService(ICourseRepository& repository, const Workspace& workspace)
    : m_repository(repository), m_workspace(workspace), m_generator(workspace),
      m_defaults(workspace)
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

    // Files first: if this fails, the stored data is still unchanged.
    m_generator.generateExerciseFiles(courseId, number);

    course.exercises.push_back(Exercise{number, name, description, fullDescription});
    try
    {
        m_repository.save(course);
    }
    catch (...)
    {
        // Do not leave files of an exercise that was never stored.
        const std::filesystem::path courseDir = m_workspace.courseDir(courseId);
        std::error_code ignored;
        std::filesystem::remove(courseDir / FileNaming::headerFileName(courseId, number), ignored);
        std::filesystem::remove(courseDir / FileNaming::sourceFileName(courseId, number), ignored);
        throw;
    }
    return number;
}

void ExerciseService::updateExercise(int courseId, int exerciseNumber, const std::string& name,
                                     const std::string& description,
                                     const std::string& fullDescription)
{
    NameValidator::validate(name);

    Course course = m_repository.load(courseId);

    const auto target = std::find_if(course.exercises.begin(), course.exercises.end(),
                                     [exerciseNumber](const Exercise& exercise)
                                     { return exercise.number == exerciseNumber; });
    if (target == course.exercises.end())
    {
        throw ExerciseNotFoundError("Exercise does not exist in this course.");
    }

    const bool duplicate =
        std::any_of(course.exercises.begin(), course.exercises.end(), [&](const Exercise& other)
                    { return other.number != exerciseNumber && other.name == name; });
    if (duplicate)
    {
        throw DuplicateExerciseError("Another exercise already uses this name.");
    }

    target->name = name;
    target->description = description;
    target->fullDescription = fullDescription;
    m_repository.save(course);
}

void ExerciseService::deleteExercise(int courseId, int exerciseNumber)
{
    Course course = m_repository.load(courseId);

    const auto target = std::find_if(course.exercises.begin(), course.exercises.end(),
                                     [exerciseNumber](const Exercise& exercise)
                                     { return exercise.number == exerciseNumber; });
    if (target == course.exercises.end())
    {
        throw ExerciseNotFoundError("Exercise does not exist in this course.");
    }

    // Check both paths before changing anything.
    const std::filesystem::path courseDir = m_workspace.courseDir(courseId);
    const std::filesystem::path headerFile = m_workspace.guard().ensureInside(
        courseDir / FileNaming::headerFileName(courseId, exerciseNumber));
    const std::filesystem::path sourceFile = m_workspace.guard().ensureInside(
        courseDir / FileNaming::sourceFileName(courseId, exerciseNumber));

    // Data first: a leftover file is better than a listed exercise without files.
    course.exercises.erase(target);
    m_repository.save(course);

    // The number of a deleted exercise is reused, so it must not stay the default.
    // A corrupt defaults file is ignored here: the CLI asks the user to reset it.
    std::optional<int> defaultExercise;
    try
    {
        defaultExercise = m_defaults.loadDefaultExercise(courseId);
    }
    catch (const StorageError&)
    {
        defaultExercise.reset();
    }
    if (defaultExercise == exerciseNumber)
    {
        m_defaults.clearDefaultExercise(courseId);
    }

    for (const std::filesystem::path& file : {headerFile, sourceFile})
    {
        std::error_code error;
        std::filesystem::remove(file, error); // a missing file is not an error
        if (error)
        {
            throw WorkspaceError("Could not delete an exercise file.");
        }
    }
}

} // namespace exercise_manager
