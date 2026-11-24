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

#include <cstddef>
#include <functional>
#include <optional>
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
    // How the course screen ends: back to the courses screen, or leave the program.
    enum class CourseScreenResult
    {
        BackToCourses,
        Exit
    };

    // The courses screen: the list of courses and its menu. Returns when the user exits.
    void runCoursesScreen();

    // Prints the courses, numbered 1..N for display only.
    void showCourses(const std::vector<Course>& courses);

    // Shows "Error: <message>".
    void showError(const ExerciseManagerError& error);

    // Runs one operation. If the Core throws an error, it is shown. In both cases the
    // user must press Enter before the menu comes back.
    void runGuarded(const std::function<void()>& action);

    // The "Add course" option: asks for the data and adds the course.
    void addCourse();
    
    // Asks for one of the shown courses and returns its position in the list, or no value
    // if there are no courses or the input ended.
    std::optional<std::size_t> pickCourse(const std::vector<Course>& courses);

    // Prints the details of one course: name, descriptions and its real folder.
    void showCourseInfo(const Course& course);

    // The "Delete course" option: asks which course, asks to confirm, and deletes it.
    void deleteCourse(const std::vector<Course>& courses);
    
    // The "Update course" option: asks which course, then each field with its current value.
    void updateCourse(const std::vector<Course>& courses);

    // The "Move course" option: asks which course and its new position, and moves it.
    void moveCourse(const std::vector<Course>& courses);

    // The "Set default course" option: shows the current default, then chooses or resets it.
    void setDefaultCourse(const std::vector<Course>& courses);

    // The course screen: course info, its exercises and their menu. Returns when the user
    // goes back to the courses screen or exits.
    CourseScreenResult runCourseScreen(int courseId);

    // Prints the exercises of a course, numbered 1..N for display only.
    void showExercises(const Course& course);

    // The "Add exercise" option: asks for the data and adds the exercise to the course.
    void addExercise(int courseId);

    // Asks for one of the shown exercises and returns its position in the list (the exercises
    // must be in the order they are shown), or no value if there are none or the input ended.
    std::optional<std::size_t> pickExercise(const Course& course);

    // Prints the details of one exercise: name, descriptions, class name and its real files.
    void showExerciseInfo(int courseId, const Exercise& exercise);

    // The "Delete exercise" option: asks which exercise, asks to confirm, and deletes it.
    void deleteExercise(const Course& course);

    // The "Update exercise" option: asks which exercise, then each field with its current value.
    void updateExercise(const Course& course);

    // The "Open exercise" option: asks which exercise, shows its details and, if the user
    // agrees, builds and runs it.
    void openExercise(const Course& course);

    // The exercise screen: shows the details of one exercise and, if the user agrees,
    // builds and runs it. Afterwards the caller shows the exercises list again.
    void runExerciseScreen(const Course& course, const Exercise& exercise);

    // Called once when the user arrives at a course: opens the default exercise if it is
    // valid, or offers to reset it if it is not.
    void openDefaultExercise(const Course& course);

    // Called once when the program starts: opens the default course if it is valid, or
    // offers to reset it if it is not. Returns Exit if the user left the program from the
    // default course, otherwise BackToCourses (go on to the courses screen).
    CourseScreenResult openDefaultCourse();

    // The "Set default exercise" option: shows the current default exercise of this course,
    // then chooses or resets it.
    void setDefaultExercise(const Course& course);

    IConsole& m_console;
    ICourseRepository& m_repository;
    CourseService& m_courses;
    ExerciseService& m_exercises;
    DefaultsStore& m_defaults;
    IBuildRunner& m_runner;
    Workspace m_workspace;
};

} // namespace exercise_manager::cli
