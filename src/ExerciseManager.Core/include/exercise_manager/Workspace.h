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
///   courses/c01/course.json, c01Problem001.hpp, ...
///   shared/      shared libraries for exercises
///   _deleted/    deleted exercises (never wiped)
///   build/       build output
/// @endcode
class Workspace
{
public:
    /// @brief Name of the environment variable that holds the workspace path.
    static constexpr const char* kEnvironmentVariable = "EXERCISE_WORKSPACE";

    /// @brief Folder names inside the workspace.
    static constexpr const char* kCoursesFolder = "courses";
    static constexpr const char* kSharedFolder = "shared";
    static constexpr const char* kDeletedFolder = "_deleted";
    static constexpr const char* kBuildFolder = "build";

    /// @brief File name of the course data file inside a course folder.
    static constexpr const char* kCourseFileName = "course.json";

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

    /// @brief Folder where deleted exercises are moved.
    std::filesystem::path deletedDir() const;

    /// @brief Folder for build output.
    std::filesystem::path buildDir() const;

    /// @brief Folder of one course, e.g. `<workspace>/courses/c01`.
    /// @throws InvalidExerciseNameError if the course number is out of range.
    std::filesystem::path courseDir(int courseId) const;

    /// @brief Data file of one course, e.g. `<workspace>/courses/c01/course.json`.
    /// @throws InvalidExerciseNameError if the course number is out of range.
    std::filesystem::path courseFile(int courseId) const;

    /// @brief The guard that checks paths stay inside this workspace.
    const PathGuard& guard() const noexcept;

private:
    PathGuard m_guard;
};

} // namespace exercise_manager