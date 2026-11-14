#pragma once

#include "exercise_manager/PathGuard.h"

#include <filesystem>

namespace exercise_manager
{

/// @brief The personal workspace folder that holds courses, shared libraries and build output.
///
/// The workspace lives outside the program's repository. Its location comes from
/// the `EXERCISE_WORKSPACE` environment variable. Layout:
/// @code
/// <workspace>/
///   Courses/Courses.json              all courses (id, name, description)
///   Courses/Course01/Exercises.json   exercises of one course
///   Courses/DefaultCourse.json        the default course (optional)
///   Courses/Course01/DefaultExercise.json   default exercise of one course (optional)
///   Courses/Course01/C01Exercise001.hpp, ...
///   shared/      shared libraries for exercises
///   build/       build output
/// @endcode
class Workspace
{
public:
    /// @brief Name of the environment variable that holds the workspace path.
    static constexpr const char* kEnvironmentVariable = "EXERCISE_WORKSPACE";

    /// @brief Folder names inside the workspace.
    static constexpr const char* kCoursesFolder = "Courses";
    static constexpr const char* kSharedFolder = "shared";
    static constexpr const char* kBuildFolder = "build";

    /// @brief File name of the list of all courses, inside the Courses folder.
    static constexpr const char* kCoursesFileName = "Courses.json";

    /// @brief File name of the exercises list, inside a course folder.
    static constexpr const char* kExercisesFileName = "Exercises.json";

    /// @brief File name of the default course, inside the Courses folder.
    static constexpr const char* kDefaultCourseFileName = "DefaultCourse.json";

    /// @brief File name of the default exercise, inside a course folder.
    static constexpr const char* kDefaultExerciseFileName = "DefaultExercise.json";

    /// @brief Creates a workspace object for a folder. Nothing is created on disk yet.
    /// @param root The workspace folder (it may not exist yet).
    /// @throws WorkspaceError if the path is empty or cannot be resolved.
    explicit Workspace(const std::filesystem::path& root);

    /// @brief Creates a workspace from the `EXERCISE_WORKSPACE` environment variable.
    /// @throws WorkspaceError if the variable is missing or empty.
    static Workspace fromEnvironment();

    /// @brief Creates the workspace folder and its sub-folders if they do not exist.
    /// @throws WorkspaceError if a folder cannot be created.
    void initialize() const;

    /// @brief The resolved workspace folder.
    const std::filesystem::path& root() const noexcept;

    /// @brief Folder that holds all course folders.
    std::filesystem::path coursesDir() const;

    /// @brief Folder of the shared libraries.
    std::filesystem::path sharedDir() const;

    /// @brief Folder for build output.
    std::filesystem::path buildDir() const;

    /// @brief Folder of one course, e.g. `<workspace>/Courses/Course01`.
    /// @throws InvalidExerciseNameError if the course number is out of range.
    std::filesystem::path courseDir(int courseId) const;

    /// @brief File that lists all courses, e.g. `<workspace>/Courses/Courses.json`.
    std::filesystem::path coursesFile() const;

    /// @brief Exercises file of one course, e.g. `<workspace>/Courses/Course01/Exercises.json`.
    /// @throws InvalidExerciseNameError if the course number is out of range.
    std::filesystem::path exercisesFile(int courseId) const;

    /// @brief File that stores the default course, e.g. `<workspace>/Courses/DefaultCourse.json`.
    std::filesystem::path defaultCourseFile() const;

    /// @brief File that stores the default exercise of one course,
    /// e.g. `<workspace>/Courses/Course01/DefaultExercise.json`.
    /// @throws InvalidExerciseNameError if the course number is out of range.
    std::filesystem::path defaultExerciseFile(int courseId) const;

    /// @brief The guard that checks paths stay inside this workspace.
    const PathGuard& guard() const noexcept;

private:
    PathGuard m_guard;
};

} // namespace exercise_manager
