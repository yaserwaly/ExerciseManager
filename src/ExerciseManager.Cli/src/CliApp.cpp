#include "exercise_manager/cli/CliApp.h"

#include "exercise_manager/cli/ConsoleUtils.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace exercise_manager::cli
{

namespace
{

// The options of the courses screen. The numbers are what the user types.
enum class CoursesOption
{
    Open = 1,
    SetDefault = 2,
    Add = 3,
    Update = 4,
    Delete = 5,
    Exit = 6
};

constexpr int kCoursesOptionCount = static_cast<int>(CoursesOption::Exit);

constexpr const char* kLine = "========================================";

} // namespace

CliApp::CliApp(IConsole& console, ICourseRepository& repository, CourseService& courses,
               ExerciseService& exercises, DefaultsStore& defaults, IBuildRunner& runner,
               const Workspace& workspace)
    : m_console(console), m_repository(repository), m_courses(courses), m_exercises(exercises),
      m_defaults(defaults), m_runner(runner), m_workspace(workspace)
{
}

int CliApp::run()
{
    try
    {
        runCoursesScreen();
        return 0;
    }
    catch (const ExerciseManagerError& error)
    {
        // Only an error that no screen handled reaches this point, for example a
        // courses file that cannot be read. There is nothing useful left to do.
        showError(error);
        return 1;
    }
}

void CliApp::showError(const ExerciseManagerError& error)
{
    m_console.writeLine(std::string("Error: ") + error.what());
    ConsoleUtils::waitForEnter(m_console);
}

void CliApp::showCourses(const std::vector<Course>& courses)
{
    m_console.writeLine(kLine);
    m_console.writeLine("                Courses");
    m_console.writeLine(kLine);

    if (courses.empty())
    {
        m_console.writeLine("No courses yet.");
    }
    for (std::size_t i = 0; i < courses.size(); ++i)
    {
        m_console.writeLine("[" + std::to_string(i + 1) + "] " + courses[i].name + " -- " +
                            courses[i].description);
    }

    m_console.writeLine(kLine);
}

void CliApp::runCoursesScreen()
{
    while (true)
    {
        const std::vector<Course> courses = m_courses.listCourses();

        m_console.clearScreen();
        showCourses(courses);
        m_console.writeLine("[1] Open course");
        m_console.writeLine("[2] Set default course");
        m_console.writeLine("[3] Add course");
        m_console.writeLine("[4] Update course");
        m_console.writeLine("[5] Delete course");
        m_console.writeLine("[6] Exit");
        m_console.writeLine(kLine);

        const std::optional<int> choice = ConsoleUtils::readNumber(
            m_console, "Choose option [1-" + std::to_string(kCoursesOptionCount) + "]: ", 1,
            kCoursesOptionCount);
        if (!choice.has_value())
        {
            return; // the input ended
        }

        switch (static_cast<CoursesOption>(*choice))
        {
        case CoursesOption::Exit:
            return;
        default:
            break; // the other options are added in the next steps
        }
    }
}

} // namespace exercise_manager::cli
