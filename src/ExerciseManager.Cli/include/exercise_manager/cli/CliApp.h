#pragma once

#include "exercise_manager/Course.h"
#include "exercise_manager/CourseService.h"
#include "exercise_manager/DefaultsStore.h"
#include "exercise_manager/Errors.h"
#include "exercise_manager/ExerciseService.h"
#include "exercise_manager/IBuildRunner.h"
#include "exercise_manager/ICourseRepository.h"
#include "exercise_manager/Workspace.h"
#include "exercise_manager/cli/IConsole.h"

#include <vector>

namespace exercise_manager::cli
{

/// @brief The interactive program: the menus and the navigation between the screens.
///
/// Everything it needs is passed in the constructor, so the whole program can be tested
/// with a fake console and a fake build runner. Navigation uses loops and returns, never
/// nested calls. Errors thrown by the Core are caught here and shown as messages.
class CliApp
{
  public:
    /// @brief Creates the program. All the objects must outlive it.
    /// @param console The console to read from and write to.
    /// @param repository The course storage (used to check that courses exist).
    /// @param courses The course service.
    /// @param exercises The exercise service.
    /// @param defaults The store of the default course and the default exercises.
    /// @param runner The runner that builds and runs one exercise.
    /// @param workspace The workspace (copied), used to show real folder and file paths.
    CliApp(IConsole& console, ICourseRepository& repository, CourseService& courses,
           ExerciseService& exercises, DefaultsStore& defaults, IBuildRunner& runner,
           const Workspace& workspace);

    /// @brief Runs the program until the user exits or the input ends.
    /// @return 0 for a normal exit, or 1 if the courses cannot be loaded.
    int run();

  private:
    // The courses screen: the list of courses and its menu. Returns when the user exits.
    void runCoursesScreen();

    // Prints the courses, numbered 1..N for display only.
    void showCourses(const std::vector<Course>& courses);

    // Shows "Error: <message>" and waits for Enter, so the user can read it.
    void showError(const ExerciseManagerError& error);

    IConsole& m_console;
    ICourseRepository& m_repository;
    CourseService& m_courses;
    ExerciseService& m_exercises;
    DefaultsStore& m_defaults;
    IBuildRunner& m_runner;
    Workspace m_workspace;
};

} // namespace exercise_manager::cli
