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
