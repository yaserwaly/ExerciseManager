#include "exercise_manager/cli/CliApp.h"

#include "FakeBuildRunner.h"
#include "FakeConsole.h"
#include "exercise_manager/CourseService.h"
#include "exercise_manager/DefaultsStore.h"
#include "exercise_manager/ExerciseService.h"
#include "exercise_manager/JsonCourseRepository.h"
#include "exercise_manager/Workspace.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <cstddef>
#include <fstream>
#include <memory>
#include <string>

namespace fs = std::filesystem;

using exercise_manager::CourseService;
using exercise_manager::DefaultsStore;
using exercise_manager::ExerciseService;
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
    FakeConsole console{"6"};

    EXPECT_EQ(runApp(console), 0);
}

TEST_F(CliAppTests, EndsWhenTheInputEnds)
{
    FakeConsole console;

    EXPECT_EQ(runApp(console), 0);
    EXPECT_TRUE(contains(console.output(), "Choose option [1-6]: "));
}

TEST_F(CliAppTests, ShowsAMessageWhenThereAreNoCourses)
{
    FakeConsole console{"6"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "No courses yet."));
}

TEST_F(CliAppTests, ShowsTheCoursesWithNameAndShortDescription)
{
    m_courses->addCourse("C++ Basics", "Basics", "full");
    m_courses->addCourse("Algorithms", "Sorting", "full");
    FakeConsole console{"6"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "[1] C++ Basics -- Basics\n"));
    EXPECT_TRUE(contains(console.output(), "[2] Algorithms -- Sorting\n"));
}

TEST_F(CliAppTests, CourseDisplayNumbersAreNotCourseNumbers)
{
    m_courses->addCourse("first", "a", "full");
    m_courses->addCourse("second", "b", "full");
    m_courses->deleteCourse(1);
    FakeConsole console{"6"};

    runApp(console);

    // The real course number is 2, but it is shown as 1.
    EXPECT_TRUE(contains(console.output(), "[1] second -- b\n"));
}

TEST_F(CliAppTests, ShowsTheMenuOptions)
{
    FakeConsole console{"6"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "[1] Open course\n"));
    EXPECT_TRUE(contains(console.output(), "[2] Set default course\n"));
    EXPECT_TRUE(contains(console.output(), "[3] Add course\n"));
    EXPECT_TRUE(contains(console.output(), "[4] Update course\n"));
    EXPECT_TRUE(contains(console.output(), "[5] Delete course\n"));
    EXPECT_TRUE(contains(console.output(), "[6] Exit\n"));
}

TEST_F(CliAppTests, AsksAgainWhenTheOptionIsNotValid)
{
    FakeConsole console{"abc", "9", "6"};

    EXPECT_EQ(runApp(console), 0);
    EXPECT_TRUE(
        contains(console.output(), "Invalid input. Please enter a number between 1 and 6.\n"));
}

TEST_F(CliAppTests, ClearsTheScreenBeforeShowingTheMenu)
{
    FakeConsole console{"6"};

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
    FakeConsole console{"3", "C++ Basics", "Basics", "Basics in full", "", "6"};

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
    FakeConsole console{"3", "C++ Basics", "Basics", "full", "", "6"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "[1] C++ Basics -- Basics\n"));
}

TEST_F(CliAppTests, AddCourseShowsTheErrorForAnInvalidName)
{
    FakeConsole console{"3", "bad/name", "short", "full", "", "6"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(
        contains(console.output(), "Error: Exercise name contains a forbidden character.\n"));
    EXPECT_TRUE(m_courses->listCourses().empty());
}

TEST_F(CliAppTests, AddCourseShowsTheErrorForADuplicateName)
{
    m_courses->addCourse("same", "a", "b");
    FakeConsole console{"3", "same", "x", "y", "", "6"};

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
    FakeConsole console{"5", "", "6"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(contains(console.output(), "There are no courses.\n"));
}

TEST_F(CliAppTests, DeleteCourseRemovesTheCourseAndItsFolder)
{
    m_courses->addCourse("first", "a", "b");
    ASSERT_TRUE(fs::exists(m_workspace->courseDir(1)));
    FakeConsole console{"5", "1", "y", "", "6"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(m_courses->listCourses().empty());
    EXPECT_FALSE(fs::exists(m_workspace->courseDir(1)));
    EXPECT_TRUE(contains(console.output(), "Course deleted successfully. Name: first\n"));
}

TEST_F(CliAppTests, DeleteCourseCancelledKeepsTheCourse)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"5", "1", "n", "", "6"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(m_courses->listCourses().size(), 1u);
    EXPECT_TRUE(contains(console.output(), "Deletion cancelled.\n"));
}

TEST_F(CliAppTests, DeleteCourseAsksAgainWhenTheNumberIsNotInTheList)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"5", "2", "1", "n", "", "6"};

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
    FakeConsole console{"5", "1", "y", "", "6"};

    runApp(console);

    EXPECT_TRUE(m_courses->listCourses().empty());
    EXPECT_TRUE(contains(console.output(), "Course deleted successfully. Name: second\n"));
}

TEST_F(CliAppTests, DeleteCourseShowsTheCourseInfoBeforeAsking)
{
    m_courses->addCourse("first", "a", "b");
    FakeConsole console{"5", "1", "n", "", "6"};

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
    FakeConsole console{"4", "", "6"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(contains(console.output(), "There are no courses.\n"));
}

TEST_F(CliAppTests, UpdateCourseChangesAllFields)
{
    m_courses->addCourse("old", "old short", "old full");
    FakeConsole console{"4", "1", "new", "new short", "new full", "", "6"};

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
    FakeConsole console{"4", "1", "", "", "", "", "6"};

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
    FakeConsole console{"4", "1", "", "new short", "", "", "6"};

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
    FakeConsole console{"4", "1", "", "", "", "", "6"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "Press Enter to keep the current value.\n"));
    EXPECT_TRUE(contains(console.output(), "Course name [old]: "));
    EXPECT_TRUE(contains(console.output(), "Short description [old short]: "));
    EXPECT_TRUE(contains(console.output(), "Full description [old full]: "));
}

TEST_F(CliAppTests, UpdateCourseShowsTheCourseInfoFirst)
{
    m_courses->addCourse("old", "old short", "old full");
    FakeConsole console{"4", "1", "", "", "", "", "6"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "Name            : old\n"));
    EXPECT_TRUE(contains(console.output(), "Description     : old short\n"));
    EXPECT_TRUE(contains(console.output(), "Full description: old full\n"));
}

TEST_F(CliAppTests, UpdateCourseShowsTheErrorForAnInvalidName)
{
    m_courses->addCourse("old", "old short", "old full");
    FakeConsole console{"4", "1", "bad/name", "", "", "", "6"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(
        contains(console.output(), "Error: Exercise name contains a forbidden character.\n"));
    EXPECT_EQ(m_courses->listCourses()[0].name, "old");
}

TEST_F(CliAppTests, UpdateCourseShowsTheErrorForTheNameOfAnotherCourse)
{
    m_courses->addCourse("first", "a", "b");
    m_courses->addCourse("second", "c", "d");
    FakeConsole console{"4", "2", "first", "", "", "", "6"};

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

// ---------- Enter is asked once per operation ----------

TEST_F(CliAppTests, AnErrorInAnOperationAsksForEnterOnlyOnce)
{
    FakeConsole console{"3", "bad/name", "short", "full", "", "6"};

    runApp(console);

    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
}

TEST_F(CliAppTests, ASuccessfulOperationAsksForEnterOnlyOnce)
{
    FakeConsole console{"3", "C++ Basics", "short", "full", "", "6"};

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
    FakeConsole console{"2", "1", "1", "", "6"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_EQ(m_defaults->loadDefaultCourse(), 2);
    EXPECT_TRUE(contains(console.output(), "Default course set. Name: second\n"));
    EXPECT_EQ(countOccurrences(console.output(), "Press Enter to continue..."), 1);
}

TEST_F(CliAppTests, SetDefaultCourseShowsTheCurrentDefaultAndResetRemovesIt)
{
    m_courses->addCourse("first", "a", "b");
    m_defaults->saveDefaultCourse(1);
    FakeConsole console{"2", "2", "", "6"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "Default course: first\n"));
    EXPECT_TRUE(contains(console.output(), "Default course removed.\n"));
    EXPECT_FALSE(m_defaults->loadDefaultCourse().has_value());
}

TEST_F(CliAppTests, SetDefaultCourseShowsWhenThereIsNoDefault)
{
    FakeConsole console{"2", "2", "", "6"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "No default course is set.\n"));
}

TEST_F(CliAppTests, SetDefaultCourseShowsWhenTheDefaultDoesNotExist)
{
    m_defaults->saveDefaultCourse(5);
    FakeConsole console{"2", "2", "", "6"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "The default course does not exist anymore.\n"));
    EXPECT_FALSE(m_defaults->loadDefaultCourse().has_value());
}

TEST_F(CliAppTests, SetDefaultCourseCanFixAnUnreadableDefaultFile)
{
    m_courses->addCourse("first", "a", "b");
    std::ofstream(m_workspace->defaultCourseFile(), std::ios::binary) << "this is not json";
    FakeConsole console{"2", "1", "1", "", "6"};

    EXPECT_EQ(runApp(console), 0);

    EXPECT_TRUE(contains(console.output(), "The default course file cannot be read.\n"));
    EXPECT_EQ(m_defaults->loadDefaultCourse(), 1);
}

TEST_F(CliAppTests, SetDefaultCourseWithNoCoursesShowsAMessage)
{
    FakeConsole console{"2", "1", "", "6"};

    runApp(console);

    EXPECT_TRUE(contains(console.output(), "There are no courses.\n"));
    EXPECT_FALSE(m_defaults->loadDefaultCourse().has_value());
}

TEST_F(CliAppTests, SetDefaultCourseAsksAgainWhenTheOptionIsNotValid)
{
    FakeConsole console{"2", "3", "2", "", "6"};

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
    FakeConsole console{"2", "2", "", "6"};

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
