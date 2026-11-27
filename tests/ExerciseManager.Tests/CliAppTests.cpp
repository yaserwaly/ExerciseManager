#include "exercise_manager/cli/CliApp.h"

#include "FakeBuildRunner.h"
#include "FakeConsole.h"
#include "exercise_manager/CourseService.h"
#include "exercise_manager/DefaultsStore.h"
#include "exercise_manager/ExerciseService.h"
#include "exercise_manager/FileNaming.h"
#include "exercise_manager/JsonCourseRepository.h"
#include "exercise_manager/Workspace.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <cstddef>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using exercise_manager::CourseService;
using exercise_manager::DefaultsStore;
using exercise_manager::ExerciseService;
using exercise_manager::FileNaming;
using exercise_manager::JsonCourseRepository;
using exercise_manager::Workspace;
using exercise_manager::cli::CliApp;
using exercise_manager::cli::test::FakeBuildRunner;
using exercise_manager::cli::test::FakeConsole;

namespace
{

// Gives every test its own temporary workspace, with the real JSON storage and services.
class CliAppTests : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        m_root = fs::temp_directory_path() / (std::string("em_cliapp_") + info->name());
        fs::remove_all(m_root);
        m_workspace = std::make_unique<Workspace>(m_root);
        m_workspace->initialize();
        m_repository = std::make_unique<JsonCourseRepository>(*m_workspace);
        m_courses = std::make_unique<CourseService>(*m_repository, *m_workspace);
        m_exercises = std::make_unique<ExerciseService>(*m_repository, *m_workspace);
        m_defaults = std::make_unique<DefaultsStore>(*m_workspace);
    }

    void TearDown() override
    {
        m_defaults.reset();
        m_exercises.reset();
        m_courses.reset();
        m_repository.reset();
        m_workspace.reset();
        fs::remove_all(m_root);
    }

    // Runs the program on a console and returns its exit code.
    int runApp(FakeConsole& console)
    {
        CliApp app(console, *m_repository, *m_courses, *m_exercises, *m_defaults, m_runner,
                   *m_workspace);
        return app.run();
    }

    static bool contains(const std::string& text, const std::string& part)
    {
        return text.find(part) != std::string::npos;
    }

    // Counts how many times a text appears inside another text.
    static int countOccurrences(const std::string& text, const std::string& part)
    {
        int count = 0;
        for (std::size_t position = text.find(part); position != std::string::npos;
             position = text.find(part, position + part.size()))
        {
            ++count;
        }
        return count;
    }

    fs::path m_root;
    std::unique_ptr<Workspace> m_workspace;
    std::unique_ptr<JsonCourseRepository> m_repository;
    std::unique_ptr<CourseService> m_courses;
    std::unique_ptr<ExerciseService> m_exercises;
    std::unique_ptr<DefaultsStore> m_defaults;
    FakeBuildRunner m_runner;
};

} // namespace

// ---------- courses screen ----------

TEST_F(CliAppTests, ExitOptionEndsTheProgramWithCodeZero)
{
    FakeConsole console{"7"};

    EXPECT_EQ(runApp(console), 0);
}

TEST_F(CliAppTests, EndsWhenTheInputEnds)
{
    FakeConsole console;

    EXPECT_EQ(runApp(console), 0);
    EXPECT_TRUE(contains(console.output(), "Choose option [1-7]: "));
}

TEST_F(CliAppTests, ShowsAMessageWhenThereAreNoCourses)
{
    FakeConsole console{"7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "No courses yet."));
}

TEST_F(CliAppTests, ShowsTheCoursesWithNameAndShortDescription)
{
    m_courses->addCourse("C++ Basics", "Basics", "full");
    m_courses->addCourse("Algorithms", "Sorting", "full");
    FakeConsole console{"7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "[1] C++ Basics -- Basics\n"));
    EXPECT_TRUE(contains(console.output(), "[2] Algorithms -- Sorting\n"));
}

TEST_F(CliAppTests, CourseDisplayNumbersAreNotCourseNumbers)
{
    m_courses->addCourse("first", "a", "full");
    m_courses->addCourse("second", "b", "full");
    m_courses->deleteCourse(1);
    FakeConsole console{"7"};

    runApp(console);

    // The real course number is 2, but it is shown as 1.
    EXPECT_TRUE(contains(console.output(), "[1] second -- b\n"));
}

TEST_F(CliAppTests, ShowsTheMenuOptions)
{
    FakeConsole console{"7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "[1] Open course\n"));
    EXPECT_TRUE(contains(console.output(), "[2] Set default course\n"));
    EXPECT_TRUE(contains(console.output(), "[3] Add course\n"));
    EXPECT_TRUE(contains(console.output(), "[4] Update course\n"));
    EXPECT_TRUE(contains(console.output(), "[5] Delete course\n"));
    EXPECT_TRUE(contains(console.output(), "[6] Move course\n"));
    EXPECT_TRUE(contains(console.output(), "[7] Exit\n"));
}

TEST_F(CliAppTests, AsksAgainWhenTheOptionIsNotValid)
{
    FakeConsole console{"abc", "9", "7"};

    EXPECT_EQ(runApp(console), 0);
    EXPECT_TRUE(
        contains(console.output(), "Invalid input. Please enter a number between 1 and 7.\n"));
}

TEST_F(CliAppTests, ClearsTheScreenBeforeShowingTheMenu)
{
    FakeConsole console{"7"};

    runApp(console);

    EXPECT_EQ(console.clearCount(), 1);
}

// ---------- errors ----------

TEST_F(CliAppTests, UnreadableCoursesFileShowsTheErrorAndEndsWithCodeOne)
{
    std::ofstream(m_workspace->coursesFile(), std::ios::binary) << "this is not json";
    FakeConsole console{""};

    EXPECT_EQ(runApp(console), 1);
    EXPECT_TRUE(contains(console.output(), "Error: Data file is not valid JSON.\n"));
    EXPECT_TRUE(contains(console.output(), "Press Enter to continue..."));
    EXPECT_FALSE(contains(console.output(), "[1] Open course"));
}

TEST_F(CliAppTests, UnreadableCoursesFileEndsEvenWhenTheInputEnds)
{
    std::ofstream(m_workspace->coursesFile(), std::ios::binary) << "this is not json";
    FakeConsole console;

    EXPECT_EQ(runApp(console), 1);
}

// ---------- add course ----------

TEST_F(CliAppTests, AddCourseCreatesTheCourseAndWaitsForEnter)
{
    FakeConsole console{"3", "C++ Basics", "Basics", "Basics in full", "", "7"};

    EXPECT_EQ(runApp(console), 0);

    const auto courses = m_courses->listCourses();
    ASSERT_EQ(courses.size(), 1u);
    EXPECT_EQ(courses[0].name, "C++ Basics");
    EXPECT_EQ(courses[0].description, "Basics");
    EXPECT_EQ(courses[0].fullDescription, "Basics in full");
    EXPECT_TRUE(contains(console.output(), "Course created successfully. Name: C++ Basics\n"));
    EXPECT_TRUE(contains(console.output(), "Press Enter to continue..."));
}

TEST_F(CliAppTests, AddCourseShowsTheNewCourseInTheList)
{
    FakeConsole console{"3", "C++ Basics", "Basics", "full", "", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "[1] C++ Basics -- Basics\n"));
}

TEST_F(CliAppTests, AddCourseShowsTheErrorForAnInvalidName)
{
    FakeConsole console{"3", "bad/name", "short", "full", "", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(
        contains(console.output(), "Error: Exercise name contains a forbidden character.\n"));
    EXPECT_TRUE(m_courses->listCourses().empty());
}

TEST_F(CliAppTests, AddCourseShowsTheErrorForADuplicateName)
{
    m_courses->addCourse("same", "a", "b");
    FakeConsole console{"3", "same", "x", "y", "", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(contains(console.output(), "Error: A course with this name already exists.\n"));
    EXPECT_EQ(m_courses->listCourses().size(), 1u);
}

TEST_F(CliAppTests, AddCourseAddsNothingWhenTheInputEndsInTheMiddle)
{
    FakeConsole console{"3", "only a name"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(m_courses->listCourses().empty());
}

// ---------- delete course ----------

TEST_F(CliAppTests, DeleteCourseWithNoCoursesShowsAMessage)
{
    FakeConsole console{"5", "", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(contains(console.output(), "There are no courses.\n"));
}

TEST_F(CliAppTests, DeleteCourseRemovesTheCourseAndItsFolder)
{
    m_courses->addCourse("first", "a", "b");
    ASSERT_TRUE(fs::exists(m_workspace->courseDir(1)));
    FakeConsole console{"5", "1", "y", "", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(m_courses->listCourses().empty());
    EXPECT_FALSE(fs::exists(m_workspace->courseDir(1)));
    EXPECT_TRUE(contains(console.output(), "Course deleted successfully. Name: first\n"));
}

TEST_F(CliAppTests, DeleteCourseCancelledKeepsTheCourse)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"5", "1", "n", "", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(m_courses->listCourses().size(), 1u);
    EXPECT_TRUE(contains(console.output(), "Deletion cancelled.\n"));
}

TEST_F(CliAppTests, DeleteCourseAsksAgainWhenTheNumberIsNotInTheList)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"5", "2", "1", "n", "", "7"};

    runApp(console);

    EXPECT_TRUE(
        contains(console.output(), "Invalid input. Please enter a number between 1 and 1.\n"));
    EXPECT_EQ(m_courses->listCourses().size(), 1u);
}

TEST_F(CliAppTests, DeleteCourseUsesTheRealCourseNumber)
{
    m_courses->addCourse("first", "a", "b");
    m_courses->addCourse("second", "c", "d");
    m_courses->deleteCourse(1);
    // "second" has the real number 2 but it is shown (and typed) as 1.
    FakeConsole console{"5", "1", "y", "", "7"};

    runApp(console);

    EXPECT_TRUE(m_courses->listCourses().empty());
    EXPECT_TRUE(contains(console.output(), "Course deleted successfully. Name: second\n"));
}

TEST_F(CliAppTests, DeleteCourseShowsTheCourseInfoBeforeAsking)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"5", "1", "n", "", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "Name            : first\n"));
    EXPECT_TRUE(contains(console.output(), "Description     : a\n"));
    EXPECT_TRUE(contains(console.output(), "Full description: b\n"));
    EXPECT_TRUE(contains(console.output(),
                         "Folder          : " + m_workspace->courseDir(1).string() + "\n"));
    EXPECT_TRUE(contains(console.output(), "(y/n): "));
}

TEST_F(CliAppTests, DeleteCourseDoesNothingWhenTheInputEndsAtTheQuestion)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"5", "1"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(m_courses->listCourses().size(), 1u);
}

// ---------- update course ----------

TEST_F(CliAppTests, UpdateCourseWithNoCoursesShowsAMessage)
{
    FakeConsole console{"4", "", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(contains(console.output(), "There are no courses.\n"));
}

TEST_F(CliAppTests, UpdateCourseChangesAllFields)
{
    m_courses->addCourse("old", "old short", "old full");
    FakeConsole console{"4", "1", "new", "new short", "new full", "", "7"};

    EXPECT_EQ(runApp(console), 0);

    const auto courses = m_courses->listCourses();
    ASSERT_EQ(courses.size(), 1u);
    EXPECT_EQ(courses[0].name, "new");
    EXPECT_EQ(courses[0].description, "new short");
    EXPECT_EQ(courses[0].fullDescription, "new full");
    EXPECT_TRUE(contains(console.output(), "Course updated successfully. Name: new\n"));
}

TEST_F(CliAppTests, UpdateCourseKeepsEveryValueWhenEnterIsPressed)
{
    m_courses->addCourse("old", "old short", "old full");
    FakeConsole console{"4", "1", "", "", "", "", "7"};

    runApp(console);

    const auto courses = m_courses->listCourses();
    ASSERT_EQ(courses.size(), 1u);
    EXPECT_EQ(courses[0].name, "old");
    EXPECT_EQ(courses[0].description, "old short");
    EXPECT_EQ(courses[0].fullDescription, "old full");
}

TEST_F(CliAppTests, UpdateCourseChangesOnlyTheTypedField)
{
    m_courses->addCourse("old", "old short", "old full");
    FakeConsole console{"4", "1", "", "new short", "", "", "7"};

    runApp(console);

    const auto courses = m_courses->listCourses();
    ASSERT_EQ(courses.size(), 1u);
    EXPECT_EQ(courses[0].name, "old");
    EXPECT_EQ(courses[0].description, "new short");
    EXPECT_EQ(courses[0].fullDescription, "old full");
}

TEST_F(CliAppTests, UpdateCourseShowsTheCurrentValuesInThePrompts)
{
    m_courses->addCourse("old", "old short", "old full");
    FakeConsole console{"4", "1", "", "", "", "", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "Press Enter to keep the current value.\n"));
    EXPECT_TRUE(contains(console.output(), "Course name [old]: "));
    EXPECT_TRUE(contains(console.output(), "Short description [old short]: "));
    EXPECT_TRUE(contains(console.output(), "Full description [old full]: "));
}

TEST_F(CliAppTests, UpdateCourseShowsTheCourseInfoFirst)
{
    m_courses->addCourse("old", "old short", "old full");
    FakeConsole console{"4", "1", "", "", "", "", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "Name            : old\n"));
    EXPECT_TRUE(contains(console.output(), "Description     : old short\n"));
    EXPECT_TRUE(contains(console.output(), "Full description: old full\n"));
}

TEST_F(CliAppTests, UpdateCourseShowsTheErrorForAnInvalidName)
{
    m_courses->addCourse("old", "old short", "old full");
    FakeConsole console{"4", "1", "bad/name", "", "", "", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(
        contains(console.output(), "Error: Exercise name contains a forbidden character.\n"));
    EXPECT_EQ(m_courses->listCourses()[0].name, "old");
}

TEST_F(CliAppTests, UpdateCourseShowsTheErrorForTheNameOfAnotherCourse)
{
    m_courses->addCourse("first", "a", "b");
    m_courses->addCourse("second", "c", "d");
    FakeConsole console{"4", "2", "first", "", "", "", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "Error: Another course already uses this name.\n"));
    EXPECT_EQ(m_courses->listCourses()[1].name, "second");
}

TEST_F(CliAppTests, UpdateCourseChangesNothingWhenTheInputEndsInTheMiddle)
{
    m_courses->addCourse("old", "old short", "old full");
    FakeConsole console{"4", "1", "new name"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(m_courses->listCourses()[0].name, "old");
}

// ---------- move course ----------

TEST_F(CliAppTests, MoveCourseWithNoCoursesShowsAMessage)
{
    FakeConsole console{"6", "", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(contains(console.output(), "There are no courses.\n"));
}

TEST_F(CliAppTests, MoveCourseMovesTheCourseAndSavesTheOrder)
{
    m_courses->addCourse("a", "da", "x"); // number 1
    m_courses->addCourse("b", "db", "x"); // number 2
    m_courses->addCourse("c", "dc", "x"); // number 3
    FakeConsole console{"6", "3", "1", "", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(m_repository->listCourseIds(), (std::vector<int>{3, 1, 2}));
    EXPECT_TRUE(contains(console.output(), "Course moved successfully. Name: c\n"));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
}

TEST_F(CliAppTests, MoveCourseShowsTheNewOrderInTheList)
{
    m_courses->addCourse("a", "da", "x");
    m_courses->addCourse("b", "db", "x");
    m_courses->addCourse("c", "dc", "x");
    FakeConsole console{"6", "1", "3", "", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "[1] b -- db\n[2] c -- dc\n[3] a -- da\n"));
}

TEST_F(CliAppTests, MoveCourseUsesTheRealCourseNumberNotThePosition)
{
    m_courses->addCourse("first", "a", "x");  // number 1
    m_courses->addCourse("second", "b", "x"); // number 2
    m_courses->addCourse("third", "c", "x");  // number 3
    m_courses->deleteCourse(1);
    m_courses->addCourse("fourth", "d", "x"); // takes the free number 1, stored last: 2, 3, 1
    // The list shows second, third, fourth. Typing 3 picks "fourth", whose real number is 1.
    FakeConsole console{"6", "3", "1", "", "7"};

    runApp(console);

    EXPECT_EQ(m_repository->listCourseIds(), (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(m_repository->load(1).name, "fourth");
    EXPECT_TRUE(contains(console.output(), "Course moved successfully. Name: fourth\n"));
}

TEST_F(CliAppTests, MoveCourseAsksAgainWhenThePositionIsNotInTheList)
{
    m_courses->addCourse("a", "da", "x");
    m_courses->addCourse("b", "db", "x");
    FakeConsole console{"6", "1", "3", "2", "", "7"};

    runApp(console);

    EXPECT_TRUE(
        contains(console.output(), "Invalid input. Please enter a number between 1 and 2.\n"));
    EXPECT_EQ(m_repository->listCourseIds(), (std::vector<int>{2, 1}));
}

TEST_F(CliAppTests, MoveCourseToItsOwnPositionChangesNothing)
{
    m_courses->addCourse("a", "da", "x");
    m_courses->addCourse("b", "db", "x");
    FakeConsole console{"6", "2", "2", "", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(m_repository->listCourseIds(), (std::vector<int>{1, 2}));
}

TEST_F(CliAppTests, MoveCourseMovesNothingWhenTheInputEndsAtThePosition)
{
    m_courses->addCourse("a", "da", "x");
    m_courses->addCourse("b", "db", "x");
    FakeConsole console{"6", "1"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(m_repository->listCourseIds(), (std::vector<int>{1, 2}));
}

TEST_F(CliAppTests, MoveCourseShowsTheCurrentPositionNotTheRealNumber)
{
    m_courses->addCourse("first", "a", "x");  // number 1
    m_courses->addCourse("second", "b", "x"); // number 2
    m_courses->addCourse("third", "c", "x");  // number 3
    m_courses->deleteCourse(1);
    m_courses->addCourse("fourth", "d", "x"); // takes the free number 1, stored last: 2, 3, 1
    // The list shows second, third, fourth. Typing 3 picks "fourth" (real number 1).
    FakeConsole console{"6", "3", "3", "", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "Current position: 3 of 3\n"));
}

TEST_F(CliAppTests, MoveCourseToItsOwnPositionShowsAMessageAndDoesNotMove)
{
    m_courses->addCourse("a", "da", "x");
    m_courses->addCourse("b", "db", "x");
    m_courses->addCourse("c", "dc", "x");
    FakeConsole console{"6", "2", "2", "", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(
        contains(console.output(), "The course is already at position 2. Nothing changed.\n"));
    EXPECT_FALSE(contains(console.output(), "Course moved successfully"));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
    EXPECT_EQ(m_repository->listCourseIds(), (std::vector<int>{1, 2, 3}));
}

// ---------- Enter is asked once per operation ----------

TEST_F(CliAppTests, AnErrorInAnOperationAsksForEnterOnlyOnce)
{
    FakeConsole console{"3", "bad/name", "short", "full", "", "7"};

    runApp(console);

    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
}

TEST_F(CliAppTests, ASuccessfulOperationAsksForEnterOnlyOnce)
{
    FakeConsole console{"3", "C++ Basics", "short", "full", "", "7"};

    runApp(console);

    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
}

TEST_F(CliAppTests, ALoadingErrorAsksForEnterOnlyOnce)
{
    std::ofstream(m_workspace->coursesFile(), std::ios::binary) << "this is not json";
    FakeConsole console{""};

    runApp(console);

    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
}

// ---------- set default course ----------

TEST_F(CliAppTests, SetDefaultCourseSavesTheRealCourseNumber)
{
    m_courses->addCourse("first", "a", "b");
    m_courses->addCourse("second", "c", "d");
    m_courses->deleteCourse(1);
    // "second" has the real number 2 but it is shown (and typed) as 1.
    FakeConsole console{"2", "1", "1", "", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(m_defaults->loadDefaultCourse(), 2);
    EXPECT_TRUE(contains(console.output(), "Default course set. Name: second\n"));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
}

TEST_F(CliAppTests, SetDefaultCourseShowsTheCurrentDefaultAndResetRemovesIt)
{
    m_courses->addCourse("first", "a", "b");
    m_defaults->saveDefaultCourse(1);
    // The program starts in the default course, so "6" goes back to the courses screen first.
    FakeConsole console{"7", "2", "2", "", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "Default course: first\n"));
    EXPECT_TRUE(contains(console.output(), "Default course removed.\n"));
    EXPECT_FALSE(m_defaults->loadDefaultCourse().has_value());
}

TEST_F(CliAppTests, SetDefaultCourseShowsWhenThereIsNoDefault)
{
    FakeConsole console{"2", "2", "", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "No default course is set.\n"));
}

TEST_F(CliAppTests, SetDefaultCourseShowsWhenTheDefaultDoesNotExist)
{
    m_defaults->saveDefaultCourse(5);
    // At the start the program offers a reset, so the first answer is "n".
    FakeConsole console{"n", "2", "2", "", "7"};

    runApp(console);

    // The message is shown at the start and again on the set default screen.
    EXPECT_EQ(countOccurrences(console.output(), "The default course does not exist anymore.\n"),
              2);
    EXPECT_FALSE(m_defaults->loadDefaultCourse().has_value());
}

TEST_F(CliAppTests, SetDefaultCourseCanFixAnUnreadableDefaultFile)
{
    m_courses->addCourse("first", "a", "b");
    std::ofstream(m_workspace->defaultCourseFile(), std::ios::binary) << "this is not json";
    // At the start the program offers a reset, so the first answer is "n".
    FakeConsole console{"n", "2", "1", "1", "", "7"};

    EXPECT_EQ(runApp(console), 0);

    // The message is shown at the start and again on the set default screen.
    EXPECT_EQ(countOccurrences(console.output(), "The default course file cannot be read.\n"), 2);
    EXPECT_EQ(m_defaults->loadDefaultCourse(), 1);
}

TEST_F(CliAppTests, SetDefaultCourseWithNoCoursesShowsAMessage)
{
    FakeConsole console{"2", "1", "", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "There are no courses.\n"));
    EXPECT_FALSE(m_defaults->loadDefaultCourse().has_value());
}

TEST_F(CliAppTests, SetDefaultCourseAsksAgainWhenTheOptionIsNotValid)
{
    FakeConsole console{"2", "3", "2", "", "7"};

    runApp(console);

    EXPECT_TRUE(
        contains(console.output(), "Invalid input. Please enter a number between 1 and 2.\n"));
}

TEST_F(CliAppTests, ResettingTheDefaultCourseKeepsTheDefaultExerciseOfThatCourse)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "", "");
    m_defaults->saveDefaultCourse(1);
    m_defaults->saveDefaultExercise(1, 1);
    // The program starts in the default course and opens its default exercise ("n"), then
    // "6" goes back to the courses screen.
    FakeConsole console{"n", "7", "2", "2", "", "7"};

    runApp(console);

    EXPECT_FALSE(m_defaults->loadDefaultCourse().has_value());
    EXPECT_EQ(m_defaults->loadDefaultExercise(1), 1);
}

TEST_F(CliAppTests, SetDefaultCourseSavesNothingWhenTheInputEndsInTheMiddle)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"2", "1"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_FALSE(m_defaults->loadDefaultCourse().has_value());
}

// ---------- course screen ----------

TEST_F(CliAppTests, OpenCourseShowsTheCourseInfoAndItsExercises)
{
    m_courses->addCourse("C++ Basics", "Basics", "Basics in full");
    m_exercises->addExercise(1, "adder", "Adds", "Adds numbers");
    m_exercises->addExercise(1, "sum", "Sums", "Sums numbers");
    FakeConsole console{"1", "1", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(contains(console.output(), "Name            : C++ Basics\n"));
    EXPECT_TRUE(contains(console.output(), "Description     : Basics\n"));
    EXPECT_TRUE(contains(console.output(), "Full description: Basics in full\n"));
    EXPECT_TRUE(contains(console.output(),
                         "Folder          : " + m_workspace->courseDir(1).string() + "\n"));
    EXPECT_TRUE(contains(console.output(), "[1] adder -- Adds\n"));
    EXPECT_TRUE(contains(console.output(), "[2] sum -- Sums\n"));
}

TEST_F(CliAppTests, ExercisesAreShownInCreationOrderAndNeverShowTheRealNumbers)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "a", "ad", "x"); // real number 1
    m_exercises->addExercise(1, "b", "bd", "x"); // real number 2
    m_exercises->addExercise(1, "c", "cd", "x"); // real number 3
    m_exercises->addExercise(1, "d", "dd", "x"); // real number 4
    m_exercises->deleteExercise(1, 1);
    m_exercises->deleteExercise(1, 3);
    m_exercises->addExercise(1, "e", "ed", "x"); // takes the free number 1, stored last
    FakeConsole console{"1", "1", "7", "7"};

    runApp(console);

    // Real numbers: b = 2, d = 4, e = 1. Shown in creation order as 1, 2, 3.
    EXPECT_TRUE(contains(console.output(), "[1] b -- bd\n[2] d -- dd\n[3] e -- ed\n"));
    // There are only three exercises, so no exercise line is numbered 4.
    EXPECT_FALSE(contains(console.output(), "[4] b -- "));
    EXPECT_FALSE(contains(console.output(), "[4] d -- "));
    EXPECT_FALSE(contains(console.output(), "[4] e -- "));
}

TEST_F(CliAppTests, CourseScreenShowsTheMenuOptions)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "1", "7", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "[1] Open exercise\n"));
    EXPECT_TRUE(contains(console.output(), "[2] Set default exercise\n"));
    EXPECT_TRUE(contains(console.output(), "[3] Add exercise\n"));
    EXPECT_TRUE(contains(console.output(), "[4] Update exercise\n"));
    EXPECT_TRUE(contains(console.output(), "[5] Delete exercise\n"));
    EXPECT_TRUE(contains(console.output(), "[6] Move exercise\n"));
    EXPECT_TRUE(contains(console.output(), "[7] Back to courses\n"));
    EXPECT_TRUE(contains(console.output(), "[8] Exit\n"));
}

TEST_F(CliAppTests, CourseScreenShowsAMessageWhenThereAreNoExercises)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "1", "7", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "No exercises yet.\n"));
}

TEST_F(CliAppTests, BackReturnsToTheCoursesScreen)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "1", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    // The courses screen is shown before opening the course and again after going back.
    EXPECT_EQ(countOccurrences(console.output(), "[1] Open course\n"), 2);
}

TEST_F(CliAppTests, ExitFromTheCourseScreenEndsTheProgram)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "1", "8"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(countOccurrences(console.output(), "[1] Open course\n"), 1);
}

TEST_F(CliAppTests, OpenCourseWithNoCoursesShowsAMessage)
{
    FakeConsole console{"1", "", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "There are no courses.\n"));
    EXPECT_FALSE(contains(console.output(), "Exercises:"));
}

TEST_F(CliAppTests, OpenCourseAsksAgainWhenTheNumberIsNotInTheList)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "2", "1", "7", "7"};

    runApp(console);

    EXPECT_TRUE(
        contains(console.output(), "Invalid input. Please enter a number between 1 and 1.\n"));
    EXPECT_TRUE(contains(console.output(), "Exercises:"));
}

TEST_F(CliAppTests, CourseScreenEndsTheProgramWhenTheInputEnds)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "1"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(contains(console.output(), "Exercises:"));
}

TEST_F(CliAppTests, CourseScreenAsksAgainWhenTheOptionIsNotValid)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "1", "9", "7", "7"};

    runApp(console);

    EXPECT_TRUE(
        contains(console.output(), "Invalid input. Please enter a number between 1 and 8.\n"));
}

// ---------- add exercise ----------

TEST_F(CliAppTests, AddExerciseCreatesTheExerciseAndItsFiles)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "1", "3", "adder", "Adds", "Adds numbers", "", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    const auto exercises = m_repository->load(1).exercises;
    ASSERT_EQ(exercises.size(), 1u);
    EXPECT_EQ(exercises[0].number, 1);
    EXPECT_EQ(exercises[0].name, "adder");
    EXPECT_EQ(exercises[0].description, "Adds");
    EXPECT_EQ(exercises[0].fullDescription, "Adds numbers");
    EXPECT_TRUE(fs::is_regular_file(m_workspace->courseDir(1) / FileNaming::headerFileName(1, 1)));
    EXPECT_TRUE(fs::is_regular_file(m_workspace->courseDir(1) / FileNaming::sourceFileName(1, 1)));
    EXPECT_TRUE(contains(console.output(), "Exercise created successfully. Name: adder\n"));
    // The course screen shows the new exercise right away.
    EXPECT_TRUE(contains(console.output(), "[1] adder -- Adds\n"));
}

TEST_F(CliAppTests, AddExerciseTakesTheFirstFreeRealNumber)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "a", "ad", "x"); // real number 1
    m_exercises->addExercise(1, "b", "bd", "x"); // real number 2
    m_exercises->addExercise(1, "c", "cd", "x"); // real number 3
    m_exercises->deleteExercise(1, 2);
    FakeConsole console{"1", "1", "3", "d", "dd", "x", "", "7", "7"};

    runApp(console);

    // The free real number 2 is reused. The list shows 1, 2, 3 for numbers 1, 2, 3.
    for (const auto& exercise : m_repository->load(1).exercises)
    {
        if (exercise.name == "d")
        {
            EXPECT_EQ(exercise.number, 2);
        }
    }
    EXPECT_EQ(m_repository->load(1).exercises.size(), 3u);
}

TEST_F(CliAppTests, AddExerciseAddsItToTheOpenedCourseOnly)
{
    m_courses->addCourse("first", "a", "b");
    m_courses->addCourse("second", "c", "d");
    // The second course is shown (and typed) as 2.
    FakeConsole console{"1", "2", "3", "adder", "Adds", "full", "", "7", "7"};

    runApp(console);

    EXPECT_TRUE(m_repository->load(1).exercises.empty());
    ASSERT_EQ(m_repository->load(2).exercises.size(), 1u);
    EXPECT_EQ(m_repository->load(2).exercises[0].name, "adder");
}

TEST_F(CliAppTests, AddExerciseShowsTheErrorForAnInvalidName)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "1", "3", "bad/name", "short", "full", "", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(
        contains(console.output(), "Error: Exercise name contains a forbidden character.\n"));
    EXPECT_TRUE(m_repository->load(1).exercises.empty());
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
}

TEST_F(CliAppTests, AddExerciseShowsTheErrorForADuplicateName)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "same", "a", "b");
    FakeConsole console{"1", "1", "3", "same", "x", "y", "", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(contains(console.output(), "Error: An exercise with this name already exists.\n"));
    EXPECT_EQ(m_repository->load(1).exercises.size(), 1u);
}

TEST_F(CliAppTests, AddExerciseAddsNothingWhenTheInputEndsInTheMiddle)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "1", "3", "only a name"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(m_repository->load(1).exercises.empty());
}

// ---------- delete exercise ----------

TEST_F(CliAppTests, DeleteExerciseWithNoExercisesShowsAMessage)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "1", "5", "", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(contains(console.output(), "There are no exercises.\n"));
}

TEST_F(CliAppTests, DeleteExerciseRemovesTheExerciseAndItsFiles)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    FakeConsole console{"1", "1", "5", "1", "y", "", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(m_repository->load(1).exercises.empty());
    EXPECT_FALSE(fs::exists(m_workspace->courseDir(1) / FileNaming::headerFileName(1, 1)));
    EXPECT_FALSE(fs::exists(m_workspace->courseDir(1) / FileNaming::sourceFileName(1, 1)));
    EXPECT_TRUE(contains(console.output(), "Exercise deleted successfully. Name: adder\n"));
}

TEST_F(CliAppTests, DeleteExerciseUsesTheRealExerciseNumberNotThePosition)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "a", "ad", "x"); // real number 1
    m_exercises->addExercise(1, "b", "bd", "x"); // real number 2
    m_exercises->addExercise(1, "c", "cd", "x"); // real number 3
    m_exercises->addExercise(1, "d", "dd", "x"); // real number 4
    m_exercises->deleteExercise(1, 1);
    m_exercises->deleteExercise(1, 3);
    // The list shows b as 1 and d as 2. Typing 2 deletes d, whose real number is 4.
    FakeConsole console{"1", "1", "5", "2", "y", "", "7", "7"};

    runApp(console);

    const auto exercises = m_repository->load(1).exercises;
    ASSERT_EQ(exercises.size(), 1u);
    EXPECT_EQ(exercises[0].name, "b");
    EXPECT_EQ(exercises[0].number, 2);
    EXPECT_TRUE(fs::exists(m_workspace->courseDir(1) / FileNaming::headerFileName(1, 2)));
    EXPECT_FALSE(fs::exists(m_workspace->courseDir(1) / FileNaming::headerFileName(1, 4)));
    EXPECT_TRUE(contains(console.output(), "Exercise deleted successfully. Name: d\n"));
}

TEST_F(CliAppTests, DeleteExerciseCancelledKeepsTheExerciseAndItsFiles)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    FakeConsole console{"1", "1", "5", "1", "n", "", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(m_repository->load(1).exercises.size(), 1u);
    EXPECT_TRUE(fs::exists(m_workspace->courseDir(1) / FileNaming::headerFileName(1, 1)));
    EXPECT_TRUE(contains(console.output(), "Deletion cancelled.\n"));
}

TEST_F(CliAppTests, DeleteExerciseAsksAgainWhenTheNumberIsNotInTheList)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    FakeConsole console{"1", "1", "5", "2", "1", "n", "", "7", "7"};

    runApp(console);

    EXPECT_TRUE(
        contains(console.output(), "Invalid input. Please enter a number between 1 and 1.\n"));
    EXPECT_EQ(m_repository->load(1).exercises.size(), 1u);
}

TEST_F(CliAppTests, DeleteExerciseShowsTheRealClassNameAndFilePathsBeforeAsking)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "a", "ad", "x");              // real number 1
    m_exercises->addExercise(1, "b", "Adds", "Adds numbers"); // real number 2
    m_exercises->deleteExercise(1, 1);
    // b is shown as 1, but its real number is 2.
    FakeConsole console{"1", "1", "5", "1", "n", "", "7", "7"};

    runApp(console);

    const fs::path courseDir = m_workspace->courseDir(1);
    EXPECT_TRUE(contains(console.output(), "Name            : b\n"));
    EXPECT_TRUE(contains(console.output(), "Description     : Adds\n"));
    EXPECT_TRUE(contains(console.output(), "Full description: Adds numbers\n"));
    EXPECT_TRUE(contains(console.output(), "Class           : ClsC01Exercise002\n"));
    EXPECT_TRUE(contains(
        console.output(),
        "Header          : " + (courseDir / FileNaming::headerFileName(1, 2)).string() + "\n"));
    EXPECT_TRUE(contains(
        console.output(),
        "Source          : " + (courseDir / FileNaming::sourceFileName(1, 2)).string() + "\n"));
    EXPECT_TRUE(contains(console.output(), "(y/n): "));
}

TEST_F(CliAppTests, DeleteExerciseClearsItsDefaultExercise)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    m_defaults->saveDefaultExercise(1, 1);
    // Arriving at the course opens its default exercise first, so the first answer is "n".
    FakeConsole console{"1", "1", "n", "5", "1", "y", "", "7", "7"};

    runApp(console);

    EXPECT_FALSE(m_defaults->loadDefaultExercise(1).has_value());
}

TEST_F(CliAppTests, DeleteExerciseDoesNothingWhenTheInputEndsAtTheQuestion)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    FakeConsole console{"1", "1", "5", "1"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(m_repository->load(1).exercises.size(), 1u);
}

// ---------- update exercise ----------

TEST_F(CliAppTests, UpdateExerciseWithNoExercisesShowsAMessage)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "1", "4", "", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(contains(console.output(), "There are no exercises.\n"));
}

TEST_F(CliAppTests, UpdateExerciseChangesAllFields)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "old", "old short", "old full");
    FakeConsole console{"1", "1", "4", "1", "new", "new short", "new full", "", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    const auto exercises = m_repository->load(1).exercises;
    ASSERT_EQ(exercises.size(), 1u);
    EXPECT_EQ(exercises[0].number, 1);
    EXPECT_EQ(exercises[0].name, "new");
    EXPECT_EQ(exercises[0].description, "new short");
    EXPECT_EQ(exercises[0].fullDescription, "new full");
    EXPECT_TRUE(contains(console.output(), "Exercise updated successfully. Name: new\n"));
}

TEST_F(CliAppTests, UpdateExerciseKeepsEveryValueWhenEnterIsPressed)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "old", "old short", "old full");
    FakeConsole console{"1", "1", "4", "1", "", "", "", "", "7", "7"};

    runApp(console);

    const auto exercises = m_repository->load(1).exercises;
    ASSERT_EQ(exercises.size(), 1u);
    EXPECT_EQ(exercises[0].name, "old");
    EXPECT_EQ(exercises[0].description, "old short");
    EXPECT_EQ(exercises[0].fullDescription, "old full");
}

TEST_F(CliAppTests, UpdateExerciseChangesOnlyTheTypedField)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "old", "old short", "old full");
    FakeConsole console{"1", "1", "4", "1", "", "new short", "", "", "7", "7"};

    runApp(console);

    const auto exercises = m_repository->load(1).exercises;
    ASSERT_EQ(exercises.size(), 1u);
    EXPECT_EQ(exercises[0].name, "old");
    EXPECT_EQ(exercises[0].description, "new short");
    EXPECT_EQ(exercises[0].fullDescription, "old full");
}

TEST_F(CliAppTests, UpdateExerciseUsesTheRealExerciseNumberNotThePosition)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "a", "ad", "x"); // real number 1
    m_exercises->addExercise(1, "b", "bd", "x"); // real number 2
    m_exercises->addExercise(1, "c", "cd", "x"); // real number 3
    m_exercises->addExercise(1, "d", "dd", "x"); // real number 4
    m_exercises->deleteExercise(1, 1);
    m_exercises->deleteExercise(1, 3);
    // The list shows b as 1 and d as 2. Typing 2 updates d, whose real number is 4.
    FakeConsole console{"1", "1", "4", "2", "renamed", "", "", "", "7", "7"};

    runApp(console);

    std::string nameOfNumber2;
    std::string nameOfNumber4;
    for (const auto& exercise : m_repository->load(1).exercises)
    {
        if (exercise.number == 2)
        {
            nameOfNumber2 = exercise.name;
        }
        if (exercise.number == 4)
        {
            nameOfNumber4 = exercise.name;
        }
    }
    EXPECT_EQ(nameOfNumber2, "b");
    EXPECT_EQ(nameOfNumber4, "renamed");
}

TEST_F(CliAppTests, UpdateExerciseShowsTheCurrentValuesInThePrompts)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "old", "old short", "old full");
    FakeConsole console{"1", "1", "4", "1", "", "", "", "", "7", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "Press Enter to keep the current value.\n"));
    EXPECT_TRUE(contains(console.output(), "Exercise name [old]: "));
    EXPECT_TRUE(contains(console.output(), "Short description [old short]: "));
    EXPECT_TRUE(contains(console.output(), "Full description [old full]: "));
}

TEST_F(CliAppTests, UpdateExerciseShowsTheExerciseInfoFirst)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "old", "old short", "old full");
    FakeConsole console{"1", "1", "4", "1", "", "", "", "", "7", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "Name            : old\n"));
    EXPECT_TRUE(contains(console.output(), "Class           : ClsC01Exercise001\n"));
}

TEST_F(CliAppTests, UpdateExerciseShowsTheErrorForAnInvalidName)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "old", "old short", "old full");
    FakeConsole console{"1", "1", "4", "1", "bad/name", "", "", "", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(
        contains(console.output(), "Error: Exercise name contains a forbidden character.\n"));
    EXPECT_EQ(m_repository->load(1).exercises[0].name, "old");
}

TEST_F(CliAppTests, UpdateExerciseShowsTheErrorForTheNameOfAnotherExercise)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "a", "ad", "x");
    m_exercises->addExercise(1, "b", "bd", "x");
    FakeConsole console{"1", "1", "4", "2", "a", "", "", "", "7", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "Error: Another exercise already uses this name.\n"));
    EXPECT_EQ(m_repository->load(1).exercises[1].name, "b");
}

TEST_F(CliAppTests, UpdateExerciseChangesNothingWhenTheInputEndsInTheMiddle)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "old", "old short", "old full");
    FakeConsole console{"1", "1", "4", "1", "new name"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(m_repository->load(1).exercises[0].name, "old");
}

// ---------- move exercise ----------

TEST_F(CliAppTests, MoveExerciseWithNoExercisesShowsAMessage)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "1", "6", "", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(contains(console.output(), "There are no exercises.\n"));
}

TEST_F(CliAppTests, MoveExerciseMovesTheExerciseAndSavesTheOrder)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "a", "ad", "x");
    m_exercises->addExercise(1, "b", "bd", "x");
    m_exercises->addExercise(1, "c", "cd", "x");
    FakeConsole console{"1", "1", "6", "3", "1", "", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    const auto exercises = m_repository->load(1).exercises;
    ASSERT_EQ(exercises.size(), 3u);
    EXPECT_EQ(exercises[0].name, "c");
    EXPECT_EQ(exercises[1].name, "a");
    EXPECT_EQ(exercises[2].name, "b");
    EXPECT_TRUE(contains(console.output(), "Exercise moved successfully. Name: c\n"));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
}

TEST_F(CliAppTests, MoveExerciseShowsTheNewOrderInTheList)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "a", "ad", "x");
    m_exercises->addExercise(1, "b", "bd", "x");
    m_exercises->addExercise(1, "c", "cd", "x");
    FakeConsole console{"1", "1", "6", "1", "3", "", "7", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "[1] b -- bd\n[2] c -- cd\n[3] a -- ad\n"));
}

TEST_F(CliAppTests, MoveExerciseUsesTheRealExerciseNumberNotThePosition)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "a", "ad", "x"); // real number 1
    m_exercises->addExercise(1, "b", "bd", "x"); // real number 2
    m_exercises->addExercise(1, "c", "cd", "x"); // real number 3
    m_exercises->addExercise(1, "d", "dd", "x"); // real number 4
    m_exercises->deleteExercise(1, 1);
    m_exercises->deleteExercise(1, 3);
    m_exercises->addExercise(1, "e", "ed", "x"); // takes the free number 1, stored last
    // The list shows b, d, e. Typing 3 picks e, whose real number is 1.
    FakeConsole console{"1", "1", "6", "3", "1", "", "7", "7"};

    runApp(console);

    const auto exercises = m_repository->load(1).exercises;
    ASSERT_EQ(exercises.size(), 3u);
    EXPECT_EQ(exercises[0].name, "e");
    EXPECT_EQ(exercises[0].number, 1);
    EXPECT_EQ(exercises[1].name, "b");
    EXPECT_EQ(exercises[2].name, "d");
    EXPECT_TRUE(contains(console.output(), "Exercise moved successfully. Name: e\n"));
}

TEST_F(CliAppTests, MoveExerciseAsksAgainWhenThePositionIsNotInTheList)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "a", "ad", "x");
    m_exercises->addExercise(1, "b", "bd", "x");
    FakeConsole console{"1", "1", "6", "1", "3", "2", "", "7", "7"};

    runApp(console);

    EXPECT_TRUE(
        contains(console.output(), "Invalid input. Please enter a number between 1 and 2.\n"));
    const auto exercises = m_repository->load(1).exercises;
    ASSERT_EQ(exercises.size(), 2u);
    EXPECT_EQ(exercises[0].name, "b");
    EXPECT_EQ(exercises[1].name, "a");
}

TEST_F(CliAppTests, MoveExerciseToItsOwnPositionChangesNothing)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "a", "ad", "x");
    m_exercises->addExercise(1, "b", "bd", "x");
    FakeConsole console{"1", "1", "6", "2", "2", "", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    const auto exercises = m_repository->load(1).exercises;
    ASSERT_EQ(exercises.size(), 2u);
    EXPECT_EQ(exercises[0].name, "a");
    EXPECT_EQ(exercises[1].name, "b");
}

TEST_F(CliAppTests, MoveExerciseMovesNothingWhenTheInputEndsAtThePosition)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "a", "ad", "x");
    m_exercises->addExercise(1, "b", "bd", "x");
    FakeConsole console{"1", "1", "6", "1"};

    EXPECT_EQ(runApp(console), 0);

    const auto exercises = m_repository->load(1).exercises;
    ASSERT_EQ(exercises.size(), 2u);
    EXPECT_EQ(exercises[0].name, "a");
    EXPECT_EQ(exercises[1].name, "b");
}

TEST_F(CliAppTests, MoveExerciseKeepsNumbersFilesAndTheDefaultExercise)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "a", "ad", "x");
    m_exercises->addExercise(1, "b", "bd", "x");
    m_exercises->addExercise(1, "c", "cd", "x");
    m_defaults->saveDefaultExercise(1, 3);
    // Arriving at the course opens its default exercise first, so the first answer is "n".
    FakeConsole console{"1", "1", "n", "6", "3", "1", "", "7", "7"};

    runApp(console);

    const auto exercises = m_repository->load(1).exercises;
    ASSERT_EQ(exercises.size(), 3u);
    EXPECT_EQ(exercises[0].name, "c");
    EXPECT_EQ(exercises[0].number, 3);
    EXPECT_EQ(exercises[1].number, 1);
    EXPECT_EQ(exercises[2].number, 2);
    for (int number = 1; number <= 3; ++number)
    {
        EXPECT_TRUE(
            fs::is_regular_file(m_workspace->courseDir(1) / FileNaming::headerFileName(1, number)));
        EXPECT_TRUE(
            fs::is_regular_file(m_workspace->courseDir(1) / FileNaming::sourceFileName(1, number)));
    }
    EXPECT_EQ(m_defaults->loadDefaultExercise(1), 3);
}

TEST_F(CliAppTests, MoveExerciseShowsTheCurrentPositionNotTheRealNumber)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "a", "ad", "x"); // real number 1
    m_exercises->addExercise(1, "b", "bd", "x"); // real number 2
    m_exercises->addExercise(1, "c", "cd", "x"); // real number 3
    m_exercises->deleteExercise(1, 1);
    m_exercises->addExercise(1, "d", "dd", "x"); // takes the free number 1, stored last: b, c, d
    // The list shows b, c, d. Typing 3 picks d (real number 1).
    FakeConsole console{"1", "1", "6", "3", "3", "", "7", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "Current position: 3 of 3\n"));
}

TEST_F(CliAppTests, MoveExerciseToItsOwnPositionShowsAMessageAndDoesNotMove)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "a", "ad", "x");
    m_exercises->addExercise(1, "b", "bd", "x");
    FakeConsole console{"1", "1", "6", "2", "2", "", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(
        contains(console.output(), "The exercise is already at position 2. Nothing changed.\n"));
    EXPECT_FALSE(contains(console.output(), "Exercise moved successfully"));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
    const auto exercises = m_repository->load(1).exercises;
    ASSERT_EQ(exercises.size(), 2u);
    EXPECT_EQ(exercises[0].name, "a");
    EXPECT_EQ(exercises[1].name, "b");
}

// ---------- open exercise ----------

TEST_F(CliAppTests, OpenExerciseWithNoExercisesShowsAMessage)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "1", "1", "", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(contains(console.output(), "There are no exercises.\n"));
    EXPECT_TRUE(m_runner.calls.empty());
}

TEST_F(CliAppTests, OpenExerciseShowsTheInfoAndDoesNotRunWhenTheAnswerIsNo)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "Adds numbers");
    FakeConsole console{"1", "1", "1", "1", "n", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(contains(console.output(), "Name            : adder\n"));
    EXPECT_TRUE(contains(console.output(), "Class           : ClsC01Exercise001\n"));
    EXPECT_TRUE(contains(console.output(), "Run the exercise? (y/n): "));
    EXPECT_TRUE(m_runner.calls.empty());
    // Answering no goes straight back to the exercises list, with no Enter to press.
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, OpenExerciseRunsTheExerciseWithItsRealNumber)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "a", "ad", "x"); // real number 1
    m_exercises->addExercise(1, "b", "bd", "x"); // real number 2
    m_exercises->addExercise(1, "c", "cd", "x"); // real number 3
    m_exercises->addExercise(1, "d", "dd", "x"); // real number 4
    m_exercises->deleteExercise(1, 1);
    m_exercises->deleteExercise(1, 3);
    // The list shows b as 1 and d as 2. Typing 2 runs d, whose real number is 4.
    FakeConsole console{"1", "1", "1", "2", "y", "", "7", "7"};

    runApp(console);

    ASSERT_EQ(m_runner.calls.size(), 1u);
    EXPECT_EQ(m_runner.calls[0].first, 1);
    EXPECT_EQ(m_runner.calls[0].second, 4);
}

TEST_F(CliAppTests, OpenExerciseRunsAReusedNumberThatIsListedLast)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "a", "ad", "x"); // real number 1
    m_exercises->addExercise(1, "b", "bd", "x"); // real number 2
    m_exercises->deleteExercise(1, 1);
    m_exercises->addExercise(1, "c", "cd", "x"); // takes the free number 1, stored last
    // The list shows b as 1 and c as 2. Typing 2 runs c, whose real number is 1.
    FakeConsole console{"1", "1", "1", "2", "y", "", "7", "7"};

    runApp(console);

    ASSERT_EQ(m_runner.calls.size(), 1u);
    EXPECT_EQ(m_runner.calls[0].first, 1);
    EXPECT_EQ(m_runner.calls[0].second, 1);
}

TEST_F(CliAppTests, OpenExerciseShowsTheExitCodeAndAsksForEnterOnce)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    m_runner.exitCode = 3;
    FakeConsole console{"1", "1", "1", "1", "y", "", "7", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "Building the exercise...\n"));
    EXPECT_TRUE(contains(console.output(), "Exercise finished with exit code 3.\n"));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
}

TEST_F(CliAppTests, OpenExerciseShowsTheBuildErrorAndGoesBackToTheList)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    m_runner.errorMessage = "Build failed:\nerror C2065: x";
    FakeConsole console{"1", "1", "1", "1", "y", "", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(contains(console.output(), "Error: Build failed:\nerror C2065: x\n"));
    EXPECT_FALSE(contains(console.output(), "Exercise finished"));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
}

TEST_F(CliAppTests, AfterRunningTheExercisesListIsShownAgainAndTheExerciseIsNotReopened)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    FakeConsole console{"1", "1", "1", "1", "y", "", "7", "7"};

    runApp(console);

    // The list is shown before and after running. The exercise details are shown only once.
    EXPECT_EQ(countOccurrences(console.output(), "Exercises:"), 2);
    EXPECT_EQ(countOccurrences(console.output(), "Class           : "), 1);
}

TEST_F(CliAppTests, OpenExerciseAsksAgainWhenTheNumberIsNotInTheList)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    FakeConsole console{"1", "1", "1", "2", "1", "n", "7", "7"};

    runApp(console);

    EXPECT_TRUE(
        contains(console.output(), "Invalid input. Please enter a number between 1 and 1.\n"));
}

TEST_F(CliAppTests, OpenExerciseAsksAgainWhenTheAnswerIsNotYesOrNo)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    FakeConsole console{"1", "1", "1", "1", "maybe", "n", "7", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "Invalid input. Please enter 'y' or 'n'.\n"));
    EXPECT_TRUE(m_runner.calls.empty());
}

TEST_F(CliAppTests, OpenExerciseRunsNothingWhenTheInputEndsAtTheQuestion)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    FakeConsole console{"1", "1", "1", "1"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(m_runner.calls.empty());
}

// ---------- set default exercise ----------

TEST_F(CliAppTests, SetDefaultExerciseSavesTheRealExerciseNumberNotThePosition)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "a", "ad", "x"); // real number 1
    m_exercises->addExercise(1, "b", "bd", "x"); // real number 2
    m_exercises->addExercise(1, "c", "cd", "x"); // real number 3
    m_exercises->addExercise(1, "d", "dd", "x"); // real number 4
    m_exercises->deleteExercise(1, 1);
    m_exercises->deleteExercise(1, 3);
    // The list shows b as 1 and d as 2. Typing 2 chooses d, whose real number is 4.
    FakeConsole console{"1", "1", "2", "1", "2", "", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(m_defaults->loadDefaultExercise(1), 4);
    EXPECT_TRUE(contains(console.output(), "Default exercise set. Name: d\n"));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
}

TEST_F(CliAppTests, SetDefaultExerciseShowsTheCurrentDefaultAndResetRemovesIt)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    m_defaults->saveDefaultExercise(1, 1);
    // Arriving at the course opens its default exercise first, so the first answer is "n".
    FakeConsole console{"1", "1", "n", "2", "2", "", "7", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "Default exercise: adder\n"));
    EXPECT_TRUE(contains(console.output(), "Default exercise removed.\n"));
    EXPECT_FALSE(m_defaults->loadDefaultExercise(1).has_value());
}

TEST_F(CliAppTests, SetDefaultExerciseShowsWhenThereIsNoDefault)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "1", "2", "2", "", "7", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "No default exercise is set.\n"));
}

TEST_F(CliAppTests, SetDefaultExerciseShowsWhenTheDefaultDoesNotExist)
{
    m_courses->addCourse("first", "a", "b");
    m_defaults->saveDefaultExercise(1, 5);
    // Arriving at the course offers a reset first, so the first answer is "n".
    FakeConsole console{"1", "1", "n", "2", "2", "", "7", "7"};

    runApp(console);

    // The message is shown on arrival and again on the set default screen.
    EXPECT_EQ(countOccurrences(console.output(), "The default exercise does not exist anymore.\n"),
              2);
    EXPECT_FALSE(m_defaults->loadDefaultExercise(1).has_value());
}

TEST_F(CliAppTests, SetDefaultExerciseCanFixAnUnreadableDefaultFile)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    std::ofstream(m_workspace->defaultExerciseFile(1), std::ios::binary) << "this is not json";
    // Arriving at the course offers to reset the damaged file first, so the first answer is "n".
    FakeConsole console{"1", "1", "n", "2", "1", "1", "", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    // The message is shown on arrival and again on the set default screen.
    EXPECT_EQ(countOccurrences(console.output(), "The default exercise file cannot be read.\n"), 2);
    EXPECT_EQ(m_defaults->loadDefaultExercise(1), 1);
}

TEST_F(CliAppTests, SetDefaultExerciseWithNoExercisesShowsAMessage)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "1", "2", "1", "", "7", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "There are no exercises.\n"));
    EXPECT_FALSE(m_defaults->loadDefaultExercise(1).has_value());
}

TEST_F(CliAppTests, SetDefaultExerciseAsksAgainWhenTheOptionIsNotValid)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "1", "2", "3", "2", "", "7", "7"};

    runApp(console);

    EXPECT_TRUE(
        contains(console.output(), "Invalid input. Please enter a number between 1 and 2.\n"));
}

TEST_F(CliAppTests, EachCourseHasItsOwnDefaultExercise)
{
    m_courses->addCourse("first", "a", "b");
    m_courses->addCourse("second", "c", "d");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    m_exercises->addExercise(2, "sum", "Sums", "full");
    FakeConsole console{"1", "1", "2", "1", "1", "", "7", "7"};

    runApp(console);

    EXPECT_EQ(m_defaults->loadDefaultExercise(1), 1);
    EXPECT_FALSE(m_defaults->loadDefaultExercise(2).has_value());
}

TEST_F(CliAppTests, SetDefaultExerciseSavesNothingWhenTheInputEndsInTheMiddle)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    FakeConsole console{"1", "1", "2", "1"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_FALSE(m_defaults->loadDefaultExercise(1).has_value());
}

// ---------- default exercise on arrival ----------

TEST_F(CliAppTests, ArrivingAtACourseOpensItsDefaultExerciseBeforeTheList)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    m_defaults->saveDefaultExercise(1, 1);
    FakeConsole console{"1", "1", "n", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    const std::string& output = console.output();
    EXPECT_TRUE(contains(output, "Class           : ClsC01Exercise001\n"));
    ASSERT_NE(output.find("Run the exercise?"), std::string::npos);
    ASSERT_NE(output.find("Exercises:"), std::string::npos);
    EXPECT_LT(output.find("Run the exercise?"), output.find("Exercises:"));
    EXPECT_TRUE(m_runner.calls.empty());
}

TEST_F(CliAppTests, TheDefaultExerciseRunsAndThenTheListIsShownWithoutReopeningIt)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    m_defaults->saveDefaultExercise(1, 1);
    FakeConsole console{"1", "1", "y", "", "7", "7"};

    runApp(console);

    ASSERT_EQ(m_runner.calls.size(), 1u);
    EXPECT_EQ(m_runner.calls[0].first, 1);
    EXPECT_EQ(m_runner.calls[0].second, 1);
    EXPECT_EQ(countOccurrences(console.output(), "Run the exercise?"), 1);
    EXPECT_EQ(countOccurrences(console.output(), "Exercises:"), 1);
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
}

TEST_F(CliAppTests, RefreshingTheListDoesNotOpenTheDefaultExerciseAgain)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    m_defaults->saveDefaultExercise(1, 1);
    FakeConsole console{"1", "1", "n", "3", "second", "s", "f", "", "7", "7"};

    runApp(console);

    EXPECT_EQ(countOccurrences(console.output(), "Run the exercise?"), 1);
    EXPECT_EQ(m_repository->load(1).exercises.size(), 2u);
}

TEST_F(CliAppTests, OpeningTheCourseAgainChecksTheDefaultExerciseAgain)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    m_defaults->saveDefaultExercise(1, 1);
    FakeConsole console{"1", "1", "n", "7", "1", "1", "n", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(countOccurrences(console.output(), "Run the exercise?"), 2);
}

TEST_F(CliAppTests, TheDefaultExerciseOfAnotherCourseIsNotOpened)
{
    m_courses->addCourse("first", "a", "b");
    m_courses->addCourse("second", "c", "d");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    m_defaults->saveDefaultExercise(1, 1);
    FakeConsole console{"1", "2", "7", "7"};

    runApp(console);

    EXPECT_EQ(countOccurrences(console.output(), "Run the exercise?"), 0);
    EXPECT_EQ(countOccurrences(console.output(), "Exercises:"), 1);
}

TEST_F(CliAppTests, WithoutADefaultExerciseTheListIsShownDirectly)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    FakeConsole console{"1", "1", "7", "7"};

    runApp(console);

    EXPECT_EQ(countOccurrences(console.output(), "Run the exercise?"), 0);
    EXPECT_EQ(countOccurrences(console.output(), "Exercises:"), 1);
}

TEST_F(CliAppTests, ADefaultExerciseThatDoesNotExistOffersAResetAndYesRemovesIt)
{
    m_courses->addCourse("first", "a", "b");
    m_defaults->saveDefaultExercise(1, 5);
    FakeConsole console{"1", "1", "y", "", "7", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "The default exercise does not exist anymore.\n"));
    EXPECT_TRUE(contains(console.output(), "Reset the default exercise? (y/n): "));
    EXPECT_TRUE(contains(console.output(), "Default exercise removed.\n"));
    EXPECT_FALSE(m_defaults->loadDefaultExercise(1).has_value());
    EXPECT_EQ(countOccurrences(console.output(), "Exercises:"), 1);
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
}

TEST_F(CliAppTests, ADefaultExerciseThatDoesNotExistKeepsItWhenTheAnswerIsNo)
{
    m_courses->addCourse("first", "a", "b");
    m_defaults->saveDefaultExercise(1, 5);
    FakeConsole console{"1", "1", "n", "7", "7"};

    runApp(console);

    EXPECT_EQ(m_defaults->loadDefaultExercise(1), 5);
    EXPECT_EQ(countOccurrences(console.output(), "Exercises:"), 1);
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, AnUnreadableDefaultExerciseFileOffersAReset)
{
    m_courses->addCourse("first", "a", "b");
    std::ofstream(m_workspace->defaultExerciseFile(1), std::ios::binary) << "this is not json";
    FakeConsole console{"1", "1", "y", "", "7", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "The default exercise file cannot be read.\n"));
    EXPECT_FALSE(m_defaults->loadDefaultExercise(1).has_value());
}

TEST_F(CliAppTests, TheProgramEndsWhenTheInputEndsAtTheResetQuestion)
{
    m_courses->addCourse("first", "a", "b");
    m_defaults->saveDefaultExercise(1, 5);
    FakeConsole console{"1", "1"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(m_defaults->loadDefaultExercise(1), 5);
}

// ---------- default course at the start ----------

TEST_F(CliAppTests, TheDefaultCourseOpensAtTheStartBeforeTheCoursesScreen)
{
    m_courses->addCourse("first", "a", "b");
    m_defaults->saveDefaultCourse(1);
    FakeConsole console{"7", "7"};

    EXPECT_EQ(runApp(console), 0);

    const std::string& output = console.output();
    ASSERT_NE(output.find("Exercises:"), std::string::npos);
    ASSERT_NE(output.find("[1] Open course\n"), std::string::npos);
    EXPECT_LT(output.find("Exercises:"), output.find("[1] Open course\n"));
}

TEST_F(CliAppTests, ExitFromTheDefaultCourseEndsTheProgramWithoutTheCoursesScreen)
{
    m_courses->addCourse("first", "a", "b");
    m_defaults->saveDefaultCourse(1);
    FakeConsole console{"8"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(countOccurrences(console.output(), "[1] Open course\n"), 0);
}

TEST_F(CliAppTests, TheDefaultCourseOpensItsDefaultExerciseAtTheStart)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    m_defaults->saveDefaultCourse(1);
    m_defaults->saveDefaultExercise(1, 1);
    FakeConsole console{"n", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(countOccurrences(console.output(), "Run the exercise?"), 1);
}

TEST_F(CliAppTests, BackFromTheDefaultCourseShowsTheCoursesScreenAndDoesNotReopenIt)
{
    m_courses->addCourse("first", "a", "b");
    m_defaults->saveDefaultCourse(1);
    FakeConsole console{"7", "7"};

    runApp(console);

    EXPECT_EQ(countOccurrences(console.output(), "[1] Open course\n"), 1);
    EXPECT_EQ(countOccurrences(console.output(), "Exercises:"), 1);
}

TEST_F(CliAppTests, WithoutADefaultCourseTheCoursesScreenIsShownFirst)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"7"};

    runApp(console);

    EXPECT_EQ(countOccurrences(console.output(), "Exercises:"), 0);
    EXPECT_EQ(countOccurrences(console.output(), "[1] Open course\n"), 1);
}

TEST_F(CliAppTests, ADefaultCourseThatDoesNotExistOffersAResetAndYesRemovesIt)
{
    m_defaults->saveDefaultCourse(5);
    FakeConsole console{"y", "", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(contains(console.output(), "The default course does not exist anymore.\n"));
    EXPECT_TRUE(contains(console.output(), "Reset the default course? (y/n): "));
    EXPECT_TRUE(contains(console.output(), "Default course removed.\n"));
    EXPECT_FALSE(m_defaults->loadDefaultCourse().has_value());
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
    EXPECT_EQ(countOccurrences(console.output(), "[1] Open course\n"), 1);
}

TEST_F(CliAppTests, ADefaultCourseThatDoesNotExistKeepsItWhenTheAnswerIsNo)
{
    m_defaults->saveDefaultCourse(5);
    FakeConsole console{"n", "7"};

    runApp(console);

    EXPECT_EQ(m_defaults->loadDefaultCourse(), 5);
    EXPECT_EQ(countOccurrences(console.output(), "[1] Open course\n"), 1);
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, AnUnreadableDefaultCourseFileOffersAReset)
{
    std::ofstream(m_workspace->defaultCourseFile(), std::ios::binary) << "this is not json";
    FakeConsole console{"y", "", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "The default course file cannot be read.\n"));
    EXPECT_FALSE(m_defaults->loadDefaultCourse().has_value());
}

TEST_F(CliAppTests, TheProgramEndsWhenTheInputEndsAtTheDefaultCourseResetQuestion)
{
    m_defaults->saveDefaultCourse(5);
    FakeConsole console;

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(m_defaults->loadDefaultCourse(), 5);
}

TEST_F(CliAppTests, SettingTheDefaultCourseDoesNotOpenItUntilTheNextStart)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"2", "1", "1", "", "7"};

    runApp(console);

    EXPECT_EQ(m_defaults->loadDefaultCourse(), 1);
    // The courses screen is shown again after setting it. The course screen is not opened.
    EXPECT_EQ(countOccurrences(console.output(), "[1] Open course\n"), 2);
    EXPECT_EQ(countOccurrences(console.output(), "Exercises:"), 0);
}

// ---------- spacing ----------

TEST_F(CliAppTests, TheEnterPromptIsPrecededByAnEmptyLine)
{
    FakeConsole console{"3", "C++ Basics", "short", "full", "", "7"};

    runApp(console);

    EXPECT_TRUE(
        contains(console.output(),
                 "Course created successfully. Name: C++ Basics\n\nPress Enter to continue..."));
}

TEST_F(CliAppTests, TheBuildingMessageIsFollowedByAnEmptyLine)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    FakeConsole console{"1", "1", "1", "1", "y", "", "7", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "Building the exercise...\n\n"));
}

// ---------- cancel: choosing an item ----------

TEST_F(CliAppTests, CancelWhenOpeningACourseGoesBackToTheCoursesScreen)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "/c", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(countOccurrences(console.output(), "[1] Open course\n"), 2);
    EXPECT_EQ(countOccurrences(console.output(), "Exercises:"), 0);
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, CancelWhenUpdatingACourseChangesNothing)
{
    m_courses->addCourse("old", "old short", "old full");
    FakeConsole console{"4", "/c", "7"};

    EXPECT_EQ(runApp(console), 0);

    const auto courses = m_courses->listCourses();
    ASSERT_EQ(courses.size(), 1u);
    EXPECT_EQ(courses[0].name, "old");
    EXPECT_EQ(courses[0].description, "old short");
    EXPECT_FALSE(contains(console.output(), "Press Enter to keep the current value."));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, CancelWhenDeletingACourseKeepsTheCourseAndAsksNothing)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"5", "/c", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(m_courses->listCourses().size(), 1u);
    EXPECT_TRUE(fs::exists(m_workspace->courseDir(1)));
    EXPECT_FALSE(contains(console.output(), "(y/n): "));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, CancelAfterAnInvalidNumberWhenDeletingACourseKeepsTheCourse)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"5", "9", "/c", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(
        contains(console.output(), "Invalid input. Please enter a number between 1 and 1.\n"));
    EXPECT_EQ(m_courses->listCourses().size(), 1u);
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, CancelWhenMovingACourseChangesNothing)
{
    m_courses->addCourse("a", "da", "x");
    m_courses->addCourse("b", "db", "x");
    m_courses->addCourse("c", "dc", "x");
    FakeConsole console{"6", "/c", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(m_repository->listCourseIds(), (std::vector<int>{1, 2, 3}));
    EXPECT_FALSE(contains(console.output(), "Current position:"));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, CancelWhenChoosingTheDefaultCourseSavesNothing)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"2", "1", "/c", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_FALSE(m_defaults->loadDefaultCourse().has_value());
    EXPECT_FALSE(contains(console.output(), "Default course set."));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, CancelWhenOpeningAnExerciseShowsTheListAgainAndRunsNothing)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    FakeConsole console{"1", "1", "1", "/c", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(m_runner.calls.empty());
    EXPECT_FALSE(contains(console.output(), "Run the exercise?"));
    // The list is shown before the cancelled question and again after it.
    EXPECT_EQ(countOccurrences(console.output(), "Exercises:"), 2);
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, CancelWhenUpdatingAnExerciseChangesNothing)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "old", "old short", "old full");
    FakeConsole console{"1", "1", "4", "/c", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    const auto exercises = m_repository->load(1).exercises;
    ASSERT_EQ(exercises.size(), 1u);
    EXPECT_EQ(exercises[0].name, "old");
    EXPECT_EQ(exercises[0].description, "old short");
    EXPECT_FALSE(contains(console.output(), "Press Enter to keep the current value."));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, CancelWhenDeletingAnExerciseKeepsItAndItsFiles)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    FakeConsole console{"1", "1", "5", "/c", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(m_repository->load(1).exercises.size(), 1u);
    EXPECT_TRUE(fs::exists(m_workspace->courseDir(1) / FileNaming::headerFileName(1, 1)));
    EXPECT_TRUE(fs::exists(m_workspace->courseDir(1) / FileNaming::sourceFileName(1, 1)));
    EXPECT_FALSE(contains(console.output(), "(y/n): "));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, CancelWhenMovingAnExerciseChangesNothing)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "a", "ad", "x");
    m_exercises->addExercise(1, "b", "bd", "x");
    m_exercises->addExercise(1, "c", "cd", "x");
    FakeConsole console{"1", "1", "6", "/c", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    const auto exercises = m_repository->load(1).exercises;
    ASSERT_EQ(exercises.size(), 3u);
    EXPECT_EQ(exercises[0].name, "a");
    EXPECT_EQ(exercises[1].name, "b");
    EXPECT_EQ(exercises[2].name, "c");
    EXPECT_FALSE(contains(console.output(), "Current position:"));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, CancelWhenChoosingTheDefaultExerciseSavesNothing)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    FakeConsole console{"1", "1", "2", "1", "/c", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_FALSE(m_defaults->loadDefaultExercise(1).has_value());
    EXPECT_FALSE(contains(console.output(), "Default exercise set."));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

// ---------- cancel: adding ----------

TEST_F(CliAppTests, CancelWhenAddingACourseAtTheNameAddsNothing)
{
    FakeConsole console{"3", "/c", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(m_courses->listCourses().empty());
    EXPECT_FALSE(contains(console.output(), "Course created successfully."));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
    // The courses screen is shown before the add screen and again after the cancel.
    EXPECT_EQ(countOccurrences(console.output(), "[1] Open course\n"), 2);
}

TEST_F(CliAppTests, CancelWhenAddingACourseAtTheShortDescriptionAddsNothing)
{
    FakeConsole console{"3", "C++ Basics", "/c", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(m_courses->listCourses().empty());
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, CancelWhenAddingACourseAtTheFullDescriptionAddsNothing)
{
    FakeConsole console{"3", "C++ Basics", "short", "/c", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(m_courses->listCourses().empty());
    EXPECT_FALSE(fs::exists(m_workspace->courseDir(1)));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, ACourseCanBeAddedAfterACancelledAdd)
{
    FakeConsole console{"3", "/c", "3", "C++ Basics", "short", "full", "", "7"};

    EXPECT_EQ(runApp(console), 0);

    const auto courses = m_courses->listCourses();
    ASSERT_EQ(courses.size(), 1u);
    EXPECT_EQ(courses[0].id, 1);
    EXPECT_EQ(courses[0].name, "C++ Basics");
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
}

TEST_F(CliAppTests, CancelWhenAddingAnExerciseAtTheNameAddsNothing)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "1", "3", "/c", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(m_repository->load(1).exercises.empty());
    EXPECT_FALSE(contains(console.output(), "Exercise created successfully."));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
    // The course screen is shown before the add screen and again after the cancel.
    EXPECT_EQ(countOccurrences(console.output(), "Exercises:"), 2);
}

TEST_F(CliAppTests, CancelWhenAddingAnExerciseAtTheShortDescriptionAddsNothing)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "1", "3", "adder", "/c", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(m_repository->load(1).exercises.empty());
    EXPECT_FALSE(fs::exists(m_workspace->courseDir(1) / FileNaming::headerFileName(1, 1)));
    EXPECT_FALSE(fs::exists(m_workspace->courseDir(1) / FileNaming::sourceFileName(1, 1)));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, CancelWhenAddingAnExerciseAtTheFullDescriptionAddsNothingAndCreatesNoFiles)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "1", "3", "adder", "short", "/c", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(m_repository->load(1).exercises.empty());
    EXPECT_FALSE(fs::exists(m_workspace->courseDir(1) / FileNaming::headerFileName(1, 1)));
    EXPECT_FALSE(fs::exists(m_workspace->courseDir(1) / FileNaming::sourceFileName(1, 1)));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, AnExerciseCanBeAddedAfterACancelledAddAndTakesNumberOne)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "1", "3", "/c", "3", "adder", "short", "full", "", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    const auto exercises = m_repository->load(1).exercises;
    ASSERT_EQ(exercises.size(), 1u);
    EXPECT_EQ(exercises[0].number, 1);
    EXPECT_EQ(exercises[0].name, "adder");
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
}

// ---------- cancel: updating ----------

TEST_F(CliAppTests, CancelWhenUpdatingACourseAtTheNameChangesNothing)
{
    m_courses->addCourse("old", "old short", "old full");
    FakeConsole console{"4", "1", "/c", "7"};

    EXPECT_EQ(runApp(console), 0);

    const auto courses = m_courses->listCourses();
    ASSERT_EQ(courses.size(), 1u);
    EXPECT_EQ(courses[0].name, "old");
    EXPECT_EQ(courses[0].description, "old short");
    EXPECT_EQ(courses[0].fullDescription, "old full");
    EXPECT_TRUE(contains(console.output(), "Course name [old]: "));
    EXPECT_FALSE(contains(console.output(), "Short description [old short]: "));
    EXPECT_FALSE(contains(console.output(), "Course updated successfully."));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, CancelWhenUpdatingACourseAtTheShortDescriptionDropsTheTypedName)
{
    m_courses->addCourse("old", "old short", "old full");
    FakeConsole console{"4", "1", "new", "/c", "7"};

    EXPECT_EQ(runApp(console), 0);

    const auto courses = m_courses->listCourses();
    ASSERT_EQ(courses.size(), 1u);
    EXPECT_EQ(courses[0].name, "old");
    EXPECT_EQ(courses[0].description, "old short");
    EXPECT_EQ(courses[0].fullDescription, "old full");
    EXPECT_FALSE(contains(console.output(), "Full description [old full]: "));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, CancelWhenUpdatingACourseAtTheFullDescriptionDropsEveryTypedField)
{
    m_courses->addCourse("old", "old short", "old full");
    FakeConsole console{"4", "1", "new", "new short", "/c", "7"};

    EXPECT_EQ(runApp(console), 0);

    const auto courses = m_courses->listCourses();
    ASSERT_EQ(courses.size(), 1u);
    EXPECT_EQ(courses[0].name, "old");
    EXPECT_EQ(courses[0].description, "old short");
    EXPECT_EQ(courses[0].fullDescription, "old full");
    EXPECT_FALSE(contains(console.output(), "Course updated successfully."));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, ACourseCanBeUpdatedAfterACancelledUpdate)
{
    m_courses->addCourse("old", "old short", "old full");
    FakeConsole console{"4", "1", "/c", "4", "1", "new", "", "", "", "7"};

    EXPECT_EQ(runApp(console), 0);

    const auto courses = m_courses->listCourses();
    ASSERT_EQ(courses.size(), 1u);
    EXPECT_EQ(courses[0].name, "new");
    EXPECT_EQ(courses[0].description, "old short");
    EXPECT_EQ(courses[0].fullDescription, "old full");
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
}

TEST_F(CliAppTests, CancelWhenUpdatingAnExerciseAtTheNameChangesNothing)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "old", "old short", "old full");
    FakeConsole console{"1", "1", "4", "1", "/c", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    const auto exercises = m_repository->load(1).exercises;
    ASSERT_EQ(exercises.size(), 1u);
    EXPECT_EQ(exercises[0].name, "old");
    EXPECT_EQ(exercises[0].description, "old short");
    EXPECT_EQ(exercises[0].fullDescription, "old full");
    EXPECT_TRUE(contains(console.output(), "Exercise name [old]: "));
    EXPECT_FALSE(contains(console.output(), "Short description [old short]: "));
    EXPECT_FALSE(contains(console.output(), "Exercise updated successfully."));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, CancelWhenUpdatingAnExerciseAtTheShortDescriptionDropsTheTypedName)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "old", "old short", "old full");
    FakeConsole console{"1", "1", "4", "1", "new", "/c", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    const auto exercises = m_repository->load(1).exercises;
    ASSERT_EQ(exercises.size(), 1u);
    EXPECT_EQ(exercises[0].name, "old");
    EXPECT_EQ(exercises[0].description, "old short");
    EXPECT_EQ(exercises[0].fullDescription, "old full");
    EXPECT_FALSE(contains(console.output(), "Full description [old full]: "));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, CancelWhenUpdatingAnExerciseAtTheFullDescriptionDropsEveryTypedField)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "old", "old short", "old full");
    FakeConsole console{"1", "1", "4", "1", "new", "new short", "/c", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    const auto exercises = m_repository->load(1).exercises;
    ASSERT_EQ(exercises.size(), 1u);
    EXPECT_EQ(exercises[0].name, "old");
    EXPECT_EQ(exercises[0].description, "old short");
    EXPECT_EQ(exercises[0].fullDescription, "old full");
    EXPECT_FALSE(contains(console.output(), "Exercise updated successfully."));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, AnExerciseCanBeUpdatedAfterACancelledUpdate)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "old", "old short", "old full");
    FakeConsole console{"1", "1", "4", "1", "/c", "4", "1", "new", "", "", "", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    const auto exercises = m_repository->load(1).exercises;
    ASSERT_EQ(exercises.size(), 1u);
    EXPECT_EQ(exercises[0].name, "new");
    EXPECT_EQ(exercises[0].description, "old short");
    EXPECT_EQ(exercises[0].fullDescription, "old full");
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
}

// ---------- cancel: move and set default ----------

TEST_F(CliAppTests, CancelWhenTypingTheNewPositionOfACourseMovesNothing)
{
    m_courses->addCourse("a", "da", "x");
    m_courses->addCourse("b", "db", "x");
    m_courses->addCourse("c", "dc", "x");
    FakeConsole console{"6", "3", "/c", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(m_repository->listCourseIds(), (std::vector<int>{1, 2, 3}));
    EXPECT_TRUE(contains(console.output(), "Current position: 3 of 3\n"));
    EXPECT_FALSE(contains(console.output(), "Course moved successfully"));
    EXPECT_FALSE(contains(console.output(), "Nothing changed."));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, ACourseCanBeMovedAfterACancelledMove)
{
    m_courses->addCourse("a", "da", "x");
    m_courses->addCourse("b", "db", "x");
    m_courses->addCourse("c", "dc", "x");
    FakeConsole console{"6", "3", "/c", "6", "3", "1", "", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(m_repository->listCourseIds(), (std::vector<int>{3, 1, 2}));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
}

TEST_F(CliAppTests, CancelWhenTypingTheNewPositionOfAnExerciseMovesNothing)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "a", "ad", "x");
    m_exercises->addExercise(1, "b", "bd", "x");
    m_exercises->addExercise(1, "c", "cd", "x");
    FakeConsole console{"1", "1", "6", "3", "/c", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    const auto exercises = m_repository->load(1).exercises;
    ASSERT_EQ(exercises.size(), 3u);
    EXPECT_EQ(exercises[0].name, "a");
    EXPECT_EQ(exercises[1].name, "b");
    EXPECT_EQ(exercises[2].name, "c");
    EXPECT_TRUE(contains(console.output(), "Current position: 3 of 3\n"));
    EXPECT_FALSE(contains(console.output(), "Exercise moved successfully"));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, AnExerciseCanBeMovedAfterACancelledMove)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "a", "ad", "x");
    m_exercises->addExercise(1, "b", "bd", "x");
    m_exercises->addExercise(1, "c", "cd", "x");
    FakeConsole console{"1", "1", "6", "3", "/c", "6", "3", "1", "", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    const auto exercises = m_repository->load(1).exercises;
    ASSERT_EQ(exercises.size(), 3u);
    EXPECT_EQ(exercises[0].name, "c");
    EXPECT_EQ(exercises[1].name, "a");
    EXPECT_EQ(exercises[2].name, "b");
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
}

TEST_F(CliAppTests, CancelAtTheDefaultCourseMenuSavesNothing)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"2", "/c", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_FALSE(m_defaults->loadDefaultCourse().has_value());
    EXPECT_TRUE(contains(console.output(), "Choose option [1-2]: "));
    EXPECT_FALSE(contains(console.output(), "Default course set."));
    EXPECT_FALSE(contains(console.output(), "Default course removed."));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, CancelAtTheDefaultCourseMenuKeepsTheCurrentDefault)
{
    m_courses->addCourse("first", "a", "b");
    m_defaults->saveDefaultCourse(1);
    // The program starts in the default course, so "7" goes back to the courses screen first.
    FakeConsole console{"7", "2", "/c", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(m_defaults->loadDefaultCourse(), 1);
    EXPECT_FALSE(contains(console.output(), "Default course removed."));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, CancelAtTheDefaultExerciseMenuSavesNothing)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    FakeConsole console{"1", "1", "2", "/c", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_FALSE(m_defaults->loadDefaultExercise(1).has_value());
    EXPECT_TRUE(contains(console.output(), "Choose option [1-2]: "));
    EXPECT_FALSE(contains(console.output(), "Default exercise set."));
    EXPECT_FALSE(contains(console.output(), "Default exercise removed."));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

TEST_F(CliAppTests, CancelAtTheDefaultExerciseMenuKeepsTheCurrentDefault)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    m_defaults->saveDefaultExercise(1, 1);
    // Arriving at the course opens its default exercise first, so the first answer is "n".
    FakeConsole console{"1", "1", "n", "2", "/c", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(m_defaults->loadDefaultExercise(1), 1);
    EXPECT_FALSE(contains(console.output(), "Default exercise removed."));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 0);
}

// ---------- cancel hint in the item questions ----------

TEST_F(CliAppTests, ThePickCourseQuestionShowsTheCancelHint)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "/c", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "Enter course number (/c to cancel): "));
}

TEST_F(CliAppTests, ThePickExerciseQuestionShowsTheCancelHint)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    FakeConsole console{"1", "1", "1", "/c", "7", "7"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "Enter exercise number (/c to cancel): "));
}

TEST_F(CliAppTests, ThePickQuestionShowsTheCancelHintEveryTimeItIsAsked)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"1", "9", "/c", "7"};

    runApp(console);

    EXPECT_EQ(countOccurrences(console.output(), "Enter course number (/c to cancel): "), 2);
    EXPECT_TRUE(
        contains(console.output(), "Invalid input. Please enter a number between 1 and 1.\n"));
}

TEST_F(CliAppTests, ThePickQuestionsAreTheOnlyQuestionsWithTheCancelHintForNow)
{
    m_courses->addCourse("first", "a", "b");
    m_exercises->addExercise(1, "adder", "Adds", "full");
    // Open the course, delete an exercise, answer "n" to the yes/no question, go back, exit.
    FakeConsole console{"1", "1", "5", "1", "n", "", "7", "7"};

    EXPECT_EQ(runApp(console), 0);

    // One hint in the course question and one in the exercise question, nowhere else.
    EXPECT_EQ(countOccurrences(console.output(), "(/c to cancel)"), 2);
    EXPECT_TRUE(contains(console.output(), "Choose option [1-7]: "));
    EXPECT_TRUE(contains(console.output(), "Choose option [1-8]: "));
    EXPECT_TRUE(contains(console.output(),
                         "Delete this exercise and its files? This cannot be undone. (y/n): "));
}
