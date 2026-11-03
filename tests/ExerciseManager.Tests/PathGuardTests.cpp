#include "exercise_manager/PathGuard.h"

#include "exercise_manager/Errors.h"

#include <gtest/gtest.h>

#include <filesystem>

namespace fs = std::filesystem;

using exercise_manager::PathGuard;
using exercise_manager::WorkspaceError;

namespace
{

// Creates a fresh temporary workspace folder and deletes it when the test ends.
class PathGuardTests : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        m_workspace = fs::temp_directory_path() / (std::string("em_pathguard_") + info->name());
        fs::remove_all(m_workspace);
        fs::create_directories(m_workspace);
    }

    void TearDown() override
    {
        fs::remove_all(m_workspace);
    }

    fs::path m_workspace;
};

} // namespace

TEST_F(PathGuardTests, RejectsEmptyWorkspacePath)
{
    EXPECT_THROW(PathGuard guard{fs::path()}, WorkspaceError);
}

TEST_F(PathGuardTests, AcceptsRelativePathInsideWorkspace)
{
    const PathGuard guard(m_workspace);
    const fs::path result = guard.ensureInside("courses/c01/c01Problem001.cpp");
    EXPECT_EQ(result, guard.root() / "courses" / "c01" / "c01Problem001.cpp");
}

TEST_F(PathGuardTests, AcceptsAbsolutePathInsideWorkspace)
{
    const PathGuard guard(m_workspace);
    const fs::path inside = guard.root() / "shared" / "lib.hpp";
    EXPECT_EQ(guard.ensureInside(inside), inside);
}

TEST_F(PathGuardTests, AcceptsPathThatDoesNotExistYet)
{
    const PathGuard guard(m_workspace);
    EXPECT_NO_THROW(guard.ensureInside("courses/c99/new.cpp"));
}

TEST_F(PathGuardTests, AcceptsDotDotThatStaysInside)
{
    const PathGuard guard(m_workspace);
    const fs::path result = guard.ensureInside("courses/c01/../c02/file.cpp");
    EXPECT_EQ(result, guard.root() / "courses" / "c02" / "file.cpp");
}

TEST_F(PathGuardTests, RejectsRelativePathThatEscapes)
{
    const PathGuard guard(m_workspace);
    EXPECT_THROW(guard.ensureInside("../outside.txt"), WorkspaceError);
    EXPECT_THROW(guard.ensureInside("courses/../../outside.txt"), WorkspaceError);
}

TEST_F(PathGuardTests, RejectsAbsolutePathOutsideWorkspace)
{
    const PathGuard guard(m_workspace);
    const fs::path outside = guard.root().parent_path() / "somewhere_else" / "file.txt";
    EXPECT_THROW(guard.ensureInside(outside), WorkspaceError);
}

TEST_F(PathGuardTests, RejectsSiblingFolderWithSamePrefix)
{
    // "<workspace>_evil" starts with the workspace path as text but is a different folder.
    const PathGuard guard(m_workspace);
    const fs::path sibling = fs::path(guard.root().string() + "_evil") / "file.txt";
    EXPECT_THROW(guard.ensureInside(sibling), WorkspaceError);
}

TEST_F(PathGuardTests, WorkspaceRootItselfIsRejected)
{
    // The root is not a file or folder "inside" the workspace.
    const PathGuard guard(m_workspace);
    EXPECT_THROW(guard.ensureInside(guard.root()), WorkspaceError);
}
