#include "exercise_manager/Workspace.h"

#include "exercise_manager/Errors.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

using exercise_manager::InvalidExerciseNameError;
using exercise_manager::Workspace;
using exercise_manager::WorkspaceError;

namespace
{

// Gives every test its own temporary folder (not created yet) and deletes it afterwards.
class WorkspaceTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        m_root = fs::temp_directory_path() / (std::string("em_workspace_") + info->name());
        fs::remove_all(m_root);
    }

    void TearDown() override
    {
        // Removes the variable so a test never leaks it into the next one.
        _putenv_s(Workspace::kEnvironmentVariable, "");
        fs::remove_all(m_root);
    }

    fs::path m_root;
};

} // namespace

TEST_F(WorkspaceTests, RejectsEmptyPath)
{
    EXPECT_THROW(Workspace{fs::path()}, WorkspaceError);
}

TEST_F(WorkspaceTests, ConstructorDoesNotCreateAnything)
{
    const Workspace workspace(m_root);
    EXPECT_FALSE(fs::exists(m_root));
}

TEST_F(WorkspaceTests, InitializeCreatesFolderLayout)
{
    const Workspace workspace(m_root);
    workspace.initialize();

    EXPECT_TRUE(fs::is_directory(workspace.root()));
    EXPECT_TRUE(fs::is_directory(workspace.coursesDir()));
    EXPECT_TRUE(fs::is_directory(workspace.sharedDir()));
    EXPECT_TRUE(fs::is_directory(workspace.deletedDir()));
    EXPECT_TRUE(fs::is_directory(workspace.buildDir()));
}

TEST_F(WorkspaceTests, InitializeTwiceKeepsExistingFiles)
{
    const Workspace workspace(m_root);
    workspace.initialize();

    const fs::path marker = workspace.sharedDir() / "marker.txt";
    std::ofstream(marker) << "keep me";

    EXPECT_NO_THROW(workspace.initialize());
    EXPECT_TRUE(fs::exists(marker));
}

TEST_F(WorkspaceTests, FoldersAreInsideTheRoot)
{
    const Workspace workspace(m_root);
    EXPECT_EQ(workspace.coursesDir(), workspace.root() / "courses");
    EXPECT_EQ(workspace.sharedDir(), workspace.root() / "shared");
    EXPECT_EQ(workspace.deletedDir(), workspace.root() / "_deleted");
    EXPECT_EQ(workspace.buildDir(), workspace.root() / "build");
}

TEST_F(WorkspaceTests, BuildsCourseFolderAndFilePaths)
{
    const Workspace workspace(m_root);
    EXPECT_EQ(workspace.courseDir(1), workspace.root() / "courses" / "Course01");
    EXPECT_EQ(workspace.courseFile(10), workspace.root() / "courses" / "Course10" / "course.json");
}

TEST_F(WorkspaceTests, RejectsCourseNumberOutOfRange)
{
    const Workspace workspace(m_root);
    EXPECT_THROW(workspace.courseDir(0), InvalidExerciseNameError);
    EXPECT_THROW(workspace.courseFile(100), InvalidExerciseNameError);
}

TEST_F(WorkspaceTests, GuardAcceptsCoursePathsAndRejectsOutsidePaths)
{
    const Workspace workspace(m_root);
    EXPECT_NO_THROW(workspace.guard().ensureInside(workspace.courseFile(1)));
    EXPECT_THROW(workspace.guard().ensureInside("../outside.txt"), WorkspaceError);
}

TEST_F(WorkspaceTests, FromEnvironmentReadsTheVariable)
{
    ASSERT_EQ(_putenv_s(Workspace::kEnvironmentVariable, m_root.string().c_str()), 0);

    const Workspace workspace = Workspace::fromEnvironment();
    EXPECT_EQ(workspace.root(), Workspace(m_root).root());
}

TEST_F(WorkspaceTests, FromEnvironmentFailsWhenVariableIsMissing)
{
    _putenv_s(Workspace::kEnvironmentVariable, "");
    EXPECT_THROW(Workspace::fromEnvironment(), WorkspaceError);
}
