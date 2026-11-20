#include "exercise_manager/cli/CliApp.h"
#include "exercise_manager/FileNaming.h"
#include "exercise_manager/cli/ConsoleUtils.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
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

// The options of the course screen (the exercises menu). The numbers are what the user types.
enum class ExercisesOption
{
    Open = 1,
    SetDefault = 2,
    Add = 3,
    Update = 4,
    Delete = 5,
    BackToCourses = 6,
    Exit = 7
};

constexpr int kExercisesOptionCount = static_cast<int>(ExercisesOption::Exit);

constexpr const char* kLine = "========================================";

// What the stored default course points to.
enum class DefaultState
{
    NotSet,    // there is no default course
    Valid,     // the default course exists
    Missing,   // a number is stored, but that course does not exist
    Unreadable // the defaults file is damaged
};

struct DefaultCourseCheck
{
    DefaultState state = DefaultState::NotSet;
    int courseId = 0;
};

// Looks at the stored default course and checks it against the existing courses.
DefaultCourseCheck checkDefaultCourse(const DefaultsStore& defaults,
                                      const std::vector<Course>& courses)
{
    try
    {
        const std::optional<int> stored = defaults.loadDefaultCourse();
        if (!stored.has_value())
        {
            return {DefaultState::NotSet, 0};
        }

        const bool exists =
            std::any_of(courses.begin(), courses.end(),
                        [&stored](const Course& course) { return course.id == *stored; });
        return {exists ? DefaultState::Valid : DefaultState::Missing, *stored};
    }
    catch (const StorageError&)
    {
        return {DefaultState::Unreadable, 0};
    }
}

struct DefaultExerciseCheck
{
    DefaultState state = DefaultState::NotSet;
    int exerciseNumber = 0;
};

// Looks at the stored default exercise of a course and checks it against its exercises.
DefaultExerciseCheck checkDefaultExercise(const DefaultsStore& defaults, const Course& course)
{
    try
    {
        const std::optional<int> stored = defaults.loadDefaultExercise(course.id);
        if (!stored.has_value())
        {
            return {DefaultState::NotSet, 0};
        }

        const bool exists =
            std::any_of(course.exercises.begin(), course.exercises.end(),
                        [&stored](const Exercise& exercise) { return exercise.number == *stored; });
        return {exists ? DefaultState::Valid : DefaultState::Missing, *stored};
    }
    catch (const StorageError&)
    {
        return {DefaultState::Unreadable, 0};
    }
}

// Asks for a position in a list that is shown numbered 1..N. The number typed is only the
// position in the list, never the real course or exercise number.
// Returns the position as a 0-based index, or no value if the list is empty or the input ended.
std::optional<std::size_t> pickPosition(IConsole& console, std::size_t count,
                                        std::string_view emptyMessage, std::string_view prompt)
{
    if (count == 0)
    {
        console.writeLine(emptyMessage);
        ConsoleUtils::waitForEnter(console);
        return std::nullopt;
    }

    const std::optional<int> number =
        ConsoleUtils::readNumber(console, prompt, 1, static_cast<int>(count));
    if (!number.has_value())
    {
        return std::nullopt;
    }
    return static_cast<std::size_t>(*number - 1);
}

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
        // The default course is checked once, when the program starts. Going back to the
        // courses screen later never opens it again.
        if (openDefaultCourse() == CourseScreenResult::Exit)
        {
            return 0;
        }
        runCoursesScreen();
        return 0;
    }
    catch (const ExerciseManagerError& error)
    {
        // Only an error that no screen handled reaches this point, for example a
        // courses file that cannot be read. There is nothing useful left to do.
        showError(error);
        ConsoleUtils::waitForEnter(m_console);
        return 1;
    }
}

void CliApp::showError(const ExerciseManagerError& error)
{
    m_console.writeLine(std::string("Error: ") + error.what());
}

void CliApp::runGuarded(const std::function<void()>& action)
{
    try
    {
        action();
    }
    catch (const ExerciseManagerError& error)
    {
        showError(error);
    }
    ConsoleUtils::waitForEnter(m_console);
}

void CliApp::addCourse()
{
    m_console.clearScreen();
    m_console.writeLine(kLine);
    m_console.writeLine("               Add course");
    m_console.writeLine(kLine);

    // If the input ends in the middle, nothing is added.
    const std::optional<std::string> name = ConsoleUtils::readText(m_console, "Course name: ");
    if (!name.has_value())
    {
        return;
    }
    const std::optional<std::string> description =
        ConsoleUtils::readText(m_console, "Short description: ");
    if (!description.has_value())
    {
        return;
    }
    const std::optional<std::string> fullDescription =
        ConsoleUtils::readText(m_console, "Full description: ");
    if (!fullDescription.has_value())
    {
        return;
    }

    runGuarded(
        [&]()
        {
            m_courses.addCourse(*name, *description, *fullDescription);
            m_console.writeLine("Course created successfully. Name: " + *name);
        });
}

std::optional<std::size_t> CliApp::pickCourse(const std::vector<Course>& courses)
{
    return pickPosition(m_console, courses.size(), "There are no courses.",
                        "Enter course number: ");
}

void CliApp::showCourseInfo(const Course& course)
{
    m_console.writeLine("Name            : " + course.name);
    m_console.writeLine("Description     : " + course.description);
    m_console.writeLine("Full description: " + course.fullDescription);
    m_console.writeLine("Folder          : " + m_workspace.courseDir(course.id).string());
    m_console.writeLine(kLine);
}

void CliApp::deleteCourse(const std::vector<Course>& courses)
{
    const std::optional<std::size_t> index = pickCourse(courses);
    if (!index.has_value())
    {
        return;
    }
    const Course& course = courses[*index];

    m_console.clearScreen();
    m_console.writeLine(kLine);
    m_console.writeLine("              Delete course");
    m_console.writeLine(kLine);
    showCourseInfo(course);

    const std::optional<bool> confirmed = ConsoleUtils::askYesNo(
        m_console, "Delete this course and all its exercises? This cannot be undone.");
    if (!confirmed.has_value())
    {
        return;
    }
    if (!*confirmed)
    {
        m_console.writeLine("Deletion cancelled.");
        ConsoleUtils::waitForEnter(m_console);
        return;
    }

    runGuarded(
        [&]()
        {
            m_courses.deleteCourse(course.id);
            m_console.writeLine("Course deleted successfully. Name: " + course.name);
        });
}

void CliApp::updateCourse(const std::vector<Course>& courses)
{
    const std::optional<std::size_t> index = pickCourse(courses);
    if (!index.has_value())
    {
        return;
    }
    const Course& course = courses[*index];

    m_console.clearScreen();
    m_console.writeLine(kLine);
    m_console.writeLine("              Update course");
    m_console.writeLine(kLine);
    showCourseInfo(course);
    m_console.writeLine("Press Enter to keep the current value.");

    // If the input ends in the middle, nothing is changed.
    const std::optional<std::string> name =
        ConsoleUtils::readTextOrKeep(m_console, "Course name", course.name);
    if (!name.has_value())
    {
        return;
    }
    const std::optional<std::string> description =
        ConsoleUtils::readTextOrKeep(m_console, "Short description", course.description);
    if (!description.has_value())
    {
        return;
    }
    const std::optional<std::string> fullDescription =
        ConsoleUtils::readTextOrKeep(m_console, "Full description", course.fullDescription);
    if (!fullDescription.has_value())
    {
        return;
    }

    runGuarded(
        [&]()
        {
            m_courses.updateCourse(course.id, *name, *description, *fullDescription);
            m_console.writeLine("Course updated successfully. Name: " + *name);
        });
}

void CliApp::setDefaultCourse(const std::vector<Course>& courses)
{
    m_console.clearScreen();
    m_console.writeLine(kLine);
    m_console.writeLine("           Set default course");
    m_console.writeLine(kLine);

    const DefaultCourseCheck check = checkDefaultCourse(m_defaults, courses);
    switch (check.state)
    {
    case DefaultState::NotSet:
        m_console.writeLine("No default course is set.");
        break;
    case DefaultState::Valid:
        for (const Course& course : courses)
        {
            if (course.id == check.courseId)
            {
                m_console.writeLine("Default course: " + course.name);
            }
        }
        break;
    case DefaultState::Missing:
        m_console.writeLine("The default course does not exist anymore.");
        break;
    case DefaultState::Unreadable:
        m_console.writeLine("The default course file cannot be read.");
        break;
    }

    m_console.writeLine("[1] Choose the default course");
    m_console.writeLine("[2] Reset (no default course)");
    m_console.writeLine(kLine);

    const std::optional<int> choice =
        ConsoleUtils::readNumber(m_console, "Choose option [1-2]: ", 1, 2);
    if (!choice.has_value())
    {
        return;
    }

    if (*choice == 2)
    {
        runGuarded(
            [&]()
            {
                m_defaults.clearDefaultCourse();
                m_console.writeLine("Default course removed.");
            });
        return;
    }

    const std::optional<std::size_t> index = pickCourse(courses);
    if (!index.has_value())
    {
        return;
    }
    const Course& course = courses[*index];

    runGuarded(
        [&]()
        {
            m_defaults.saveDefaultCourse(course.id);
            m_console.writeLine("Default course set. Name: " + course.name);
        });
}

void CliApp::showExercises(const Course& course)
{
    m_console.writeLine("Exercises:");

    if (course.exercises.empty())
    {
        m_console.writeLine("No exercises yet.");
    }
    // The numbers shown are positions in the list (1..N), not the real exercise numbers.
    for (std::size_t i = 0; i < course.exercises.size(); ++i)
    {
        m_console.writeLine("[" + std::to_string(i + 1) + "] " + course.exercises[i].name + " -- " +
                            course.exercises[i].description);
    }

    m_console.writeLine(kLine);
}

void CliApp::addExercise(int courseId)
{
    m_console.clearScreen();
    m_console.writeLine(kLine);
    m_console.writeLine("              Add exercise");
    m_console.writeLine(kLine);

    // If the input ends in the middle, nothing is added.
    const std::optional<std::string> name = ConsoleUtils::readText(m_console, "Exercise name: ");
    if (!name.has_value())
    {
        return;
    }
    const std::optional<std::string> description =
        ConsoleUtils::readText(m_console, "Short description: ");
    if (!description.has_value())
    {
        return;
    }
    const std::optional<std::string> fullDescription =
        ConsoleUtils::readText(m_console, "Full description: ");
    if (!fullDescription.has_value())
    {
        return;
    }

    runGuarded(
        [&]()
        {
            m_exercises.addExercise(courseId, *name, *description, *fullDescription);
            m_console.writeLine("Exercise created successfully. Name: " + *name);
        });
}

std::optional<std::size_t> CliApp::pickExercise(const Course& course)
{
    return pickPosition(m_console, course.exercises.size(), "There are no exercises.",
                        "Enter exercise number: ");
}

void CliApp::showExerciseInfo(int courseId, const Exercise& exercise)
{
    const std::filesystem::path courseDir = m_workspace.courseDir(courseId);
    const std::filesystem::path header =
        courseDir / FileNaming::headerFileName(courseId, exercise.number);
    const std::filesystem::path source =
        courseDir / FileNaming::sourceFileName(courseId, exercise.number);

    m_console.writeLine("Name            : " + exercise.name);
    m_console.writeLine("Description     : " + exercise.description);
    m_console.writeLine("Full description: " + exercise.fullDescription);
    m_console.writeLine("Class           : " + FileNaming::className(courseId, exercise.number));
    m_console.writeLine("Header          : " + header.string());
    m_console.writeLine("Source          : " + source.string());
    m_console.writeLine(kLine);
}

void CliApp::deleteExercise(const Course& course)
{
    const std::optional<std::size_t> index = pickExercise(course);
    if (!index.has_value())
    {
        return;
    }
    const Exercise& exercise = course.exercises[*index];

    m_console.clearScreen();
    m_console.writeLine(kLine);
    m_console.writeLine("             Delete exercise");
    m_console.writeLine(kLine);
    showExerciseInfo(course.id, exercise);

    const std::optional<bool> confirmed = ConsoleUtils::askYesNo(
        m_console, "Delete this exercise and its files? This cannot be undone.");
    if (!confirmed.has_value())
    {
        return;
    }
    if (!*confirmed)
    {
        m_console.writeLine("Deletion cancelled.");
        ConsoleUtils::waitForEnter(m_console);
        return;
    }

    runGuarded(
        [&]()
        {
            // The real exercise number is used here, not the position in the list.
            m_exercises.deleteExercise(course.id, exercise.number);
            m_console.writeLine("Exercise deleted successfully. Name: " + exercise.name);
        });
}

void CliApp::updateExercise(const Course& course)
{
    const std::optional<std::size_t> index = pickExercise(course);
    if (!index.has_value())
    {
        return;
    }
    const Exercise& exercise = course.exercises[*index];

    m_console.clearScreen();
    m_console.writeLine(kLine);
    m_console.writeLine("             Update exercise");
    m_console.writeLine(kLine);
    showExerciseInfo(course.id, exercise);
    m_console.writeLine("Press Enter to keep the current value.");

    // If the input ends in the middle, nothing is changed.
    const std::optional<std::string> name =
        ConsoleUtils::readTextOrKeep(m_console, "Exercise name", exercise.name);
    if (!name.has_value())
    {
        return;
    }
    const std::optional<std::string> description =
        ConsoleUtils::readTextOrKeep(m_console, "Short description", exercise.description);
    if (!description.has_value())
    {
        return;
    }
    const std::optional<std::string> fullDescription =
        ConsoleUtils::readTextOrKeep(m_console, "Full description", exercise.fullDescription);
    if (!fullDescription.has_value())
    {
        return;
    }

    runGuarded(
        [&]()
        {
            // The real exercise number is used here, not the position in the list.
            m_exercises.updateExercise(course.id, exercise.number, *name, *description,
                                       *fullDescription);
            m_console.writeLine("Exercise updated successfully. Name: " + *name);
        });
}

void CliApp::openExercise(const Course& course)
{
    const std::optional<std::size_t> index = pickExercise(course);
    if (!index.has_value())
    {
        return;
    }
    runExerciseScreen(course, course.exercises[*index]);
}

void CliApp::runExerciseScreen(const Course& course, const Exercise& exercise)
{
    m_console.clearScreen();
    m_console.writeLine(kLine);
    m_console.writeLine("                Exercise");
    m_console.writeLine(kLine);
    showExerciseInfo(course.id, exercise);

    const std::optional<bool> run = ConsoleUtils::askYesNo(m_console, "Run the exercise?");
    if (!run.has_value() || !*run)
    {
        return; // back to the exercises list
    }

    m_console.clearScreen();
    m_console.writeLine("Building the exercise...");
    runGuarded(
        [&]()
        {
            // The real exercise number is used here, not the position in the list.
            const BuildResult result = m_runner.buildAndRun(course.id, exercise.number);
            m_console.writeLine("");
            m_console.writeLine("Exercise finished with exit code " +
                                std::to_string(result.exitCode) + ".");
        });
}

void CliApp::setDefaultExercise(const Course& course)
{
    m_console.clearScreen();
    m_console.writeLine(kLine);
    m_console.writeLine("          Set default exercise");
    m_console.writeLine(kLine);

    const DefaultExerciseCheck check = checkDefaultExercise(m_defaults, course);
    switch (check.state)
    {
    case DefaultState::NotSet:
        m_console.writeLine("No default exercise is set.");
        break;
    case DefaultState::Valid:
        for (const Exercise& exercise : course.exercises)
        {
            if (exercise.number == check.exerciseNumber)
            {
                m_console.writeLine("Default exercise: " + exercise.name);
            }
        }
        break;
    case DefaultState::Missing:
        m_console.writeLine("The default exercise does not exist anymore.");
        break;
    case DefaultState::Unreadable:
        m_console.writeLine("The default exercise file cannot be read.");
        break;
    }

    m_console.writeLine("[1] Choose the default exercise");
    m_console.writeLine("[2] Reset (no default exercise)");
    m_console.writeLine(kLine);

    const std::optional<int> choice =
        ConsoleUtils::readNumber(m_console, "Choose option [1-2]: ", 1, 2);
    if (!choice.has_value())
    {
        return;
    }

    if (*choice == 2)
    {
        runGuarded(
            [&]()
            {
                m_defaults.clearDefaultExercise(course.id);
                m_console.writeLine("Default exercise removed.");
            });
        return;
    }

    const std::optional<std::size_t> index = pickExercise(course);
    if (!index.has_value())
    {
        return;
    }
    const Exercise& exercise = course.exercises[*index];

    runGuarded(
        [&]()
        {
            // The real exercise number is stored, not the position in the list.
            m_defaults.saveDefaultExercise(course.id, exercise.number);
            m_console.writeLine("Default exercise set. Name: " + exercise.name);
        });
}

void CliApp::openDefaultExercise(const Course& course)
{
    const DefaultExerciseCheck check = checkDefaultExercise(m_defaults, course);
    switch (check.state)
    {
    case DefaultState::NotSet:
        return;
    case DefaultState::Valid:
        for (const Exercise& exercise : course.exercises)
        {
            if (exercise.number == check.exerciseNumber)
            {
                runExerciseScreen(course, exercise);
                return;
            }
        }
        return;
    case DefaultState::Missing:
        m_console.clearScreen();
        m_console.writeLine("The default exercise does not exist anymore.");
        break;
    case DefaultState::Unreadable:
        m_console.clearScreen();
        m_console.writeLine("The default exercise file cannot be read.");
        break;
    }

    // The stored default is not valid. If the user says no, the normal list is shown.
    const std::optional<bool> reset =
        ConsoleUtils::askYesNo(m_console, "Reset the default exercise?");
    if (reset.has_value() && *reset)
    {
        runGuarded(
            [&]()
            {
                m_defaults.clearDefaultExercise(course.id);
                m_console.writeLine("Default exercise removed.");
            });
    }
}

CliApp::CourseScreenResult CliApp::openDefaultCourse()
{
    const std::vector<Course> courses = m_courses.listCourses();
    const DefaultCourseCheck check = checkDefaultCourse(m_defaults, courses);
    switch (check.state)
    {
    case DefaultState::NotSet:
        return CourseScreenResult::BackToCourses;
    case DefaultState::Valid:
        return runCourseScreen(check.courseId);
    case DefaultState::Missing:
        m_console.clearScreen();
        m_console.writeLine("The default course does not exist anymore.");
        break;
    case DefaultState::Unreadable:
        m_console.clearScreen();
        m_console.writeLine("The default course file cannot be read.");
        break;
    }

    // The stored default is not valid. If the user says no, the courses screen is shown.
    const std::optional<bool> reset =
        ConsoleUtils::askYesNo(m_console, "Reset the default course?");
    if (reset.has_value() && *reset)
    {
        runGuarded(
            [&]()
            {
                m_defaults.clearDefaultCourse();
                m_console.writeLine("Default course removed.");
            });
    }
    return CourseScreenResult::BackToCourses;
}

CliApp::CourseScreenResult CliApp::runCourseScreen(int courseId)
{
    bool justArrived = true;
    while (true)
    {
        // The course is loaded again every time, so changes are always shown.
        Course course;
        try
        {
            course = m_repository.load(courseId);
        }
        catch (const ExerciseManagerError& error)
        {
            showError(error);
            ConsoleUtils::waitForEnter(m_console);
            return CourseScreenResult::BackToCourses;
        }
        // The list is ordered by the real exercise number, but it is shown as 1..N.
        std::sort(course.exercises.begin(), course.exercises.end(),
                  [](const Exercise& a, const Exercise& b) { return a.number < b.number; });

        // Right after arriving at the course, its default exercise is checked once. Later
        // refreshes of the list (after an operation) never open it again.
        if (justArrived)
        {
            justArrived = false;
            openDefaultExercise(course);
        }

        m_console.clearScreen();
        m_console.writeLine(kLine);
        m_console.writeLine("                Course");
        m_console.writeLine(kLine);
        showCourseInfo(course);
        showExercises(course);
        m_console.writeLine("[1] Open exercise");
        m_console.writeLine("[2] Set default exercise");
        m_console.writeLine("[3] Add exercise");
        m_console.writeLine("[4] Update exercise");
        m_console.writeLine("[5] Delete exercise");
        m_console.writeLine("[6] Back to courses");
        m_console.writeLine("[7] Exit");
        m_console.writeLine(kLine);

        const std::optional<int> choice = ConsoleUtils::readNumber(
            m_console, "Choose option [1-" + std::to_string(kExercisesOptionCount) + "]: ", 1,
            kExercisesOptionCount);
        if (!choice.has_value())
        {
            return CourseScreenResult::Exit; // the input ended
        }

        switch (static_cast<ExercisesOption>(*choice))
        {
        case ExercisesOption::Open:
            openExercise(course);
            break;
        case ExercisesOption::SetDefault:
            setDefaultExercise(course);
            break;
        case ExercisesOption::Add:
            addExercise(courseId);
            break;
        case ExercisesOption::Update:
            updateExercise(course);
            break;
        case ExercisesOption::Delete:
            deleteExercise(course);
            break;
        case ExercisesOption::BackToCourses:
            return CourseScreenResult::BackToCourses;
        case ExercisesOption::Exit:
            return CourseScreenResult::Exit;
        default:
            break; // the other options are added in the next steps
        }
    }
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
        case CoursesOption::Add:
            addCourse();
            break;
        case CoursesOption::Open:
        {
            const std::optional<std::size_t> index = pickCourse(courses);
            if (index.has_value() &&
                runCourseScreen(courses[*index].id) == CourseScreenResult::Exit)
            {
                return;
            }
            break;
        }
        case CoursesOption::SetDefault:
            setDefaultCourse(courses);
            break;
        case CoursesOption::Update:
            updateCourse(courses);
            break;
        case CoursesOption::Delete:
            deleteCourse(courses);
            break;
        case CoursesOption::Exit:
            return;
        default:
            break; // the other options are added in the next steps
        }
    }
}

} // namespace exercise_manager::cli
