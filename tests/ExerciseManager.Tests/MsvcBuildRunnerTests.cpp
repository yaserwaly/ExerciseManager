#include "exercise_manager/MsvcBuildRunner.h"

#include "exercise_manager/CodeGenerator.h"
#include "exercise_manager/Errors.h"
#include "exercise_manager/FileNaming.h"
#include "exercise_manager/Workspace.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

namespace fs = std::filesystem;

using exercise_manager::BuildError;
using exercise_manager::BuildResult;
using exercise_manager::CodeGenerator;
using exercise_manager::FileNaming;
using exercise_manager::InvalidExerciseNameError;
using exercise_manager::MsvcBuildRunner;
using exercise_manager::Workspace;
using exercise_manager::WorkspaceError;

namespace
{

// Gives every test its own temporary workspace and deletes it afterwards.
class MsvcBuildRunnerTests : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        m_root = fs::temp_directory_path() / (std::string("em_buildrunner_") + info->name());
        fs::remove_all(m_root);
        m_workspace = std::make_unique<Workspace>(m_root);
        m_workspace->initialize();
        m_runner = std::make_unique<MsvcBuildRunner>(*m_workspace);
    }

    void TearDown() override
    {
        m_runner.reset();
        m_workspace.reset();
        fs::remove_all(m_root);
    }

    fs::path sourcePath(int courseId, int number) const
    {
        return m_workspace->courseDir(courseId) / FileNaming::sourceFileName(courseId, number);
    }

    fs::path m_root;
    std::unique_ptr<Workspace> m_workspace;
    std::unique_ptr<MsvcBuildRunner> m_runner;
};

} // namespace

TEST_F(MsvcBuildRunnerTests, FindVcvarsallReturnsExistingFile)
{
    const fs::path vcvarsall = MsvcBuildRunner::findVcvarsall();
    EXPECT_TRUE(fs::is_regular_file(vcvarsall));
}

TEST_F(MsvcBuildRunnerTests, BuildAndRunReturnsExitCodeOfExercise)
{
    CodeGenerator(*m_workspace).generateExerciseFiles(1, 1);

    const BuildResult result = m_runner->buildAndRun(1, 1);

    EXPECT_EQ(result.exitCode, 0);
    EXPECT_TRUE(fs::is_regular_file(m_workspace->buildDir() / "exercise.exe"));
}

TEST_F(MsvcBuildRunnerTests, BuildAndRunThrowsBuildErrorWithCompilerOutputWhenBuildFails)
{
    CodeGenerator(*m_workspace).generateExerciseFiles(1, 1);
    std::ofstream(sourcePath(1, 1), std::ios::app) << "this is a syntax error\n";

    try
    {
        m_runner->buildAndRun(1, 1);
        FAIL() << "Expected BuildError";
    }
    catch (const BuildError& error)
    {
        EXPECT_NE(std::string(error.what()).find("error C"), std::string::npos);
    }
    EXPECT_FALSE(fs::exists(m_workspace->buildDir() / "exercise.exe"));
}

TEST_F(MsvcBuildRunnerTests, BuildAndRunThrowsWorkspaceErrorWhenSourceIsMissing)
{
    EXPECT_THROW(m_runner->buildAndRun(1, 1), WorkspaceError);
}

TEST_F(MsvcBuildRunnerTests, BuildAndRunRejectsNumberOutOfRange)
{
    EXPECT_THROW(m_runner->buildAndRun(0, 1), InvalidExerciseNameError);
    EXPECT_THROW(m_runner->buildAndRun(1, FileNaming::kMaxExerciseNumber + 1),
                 InvalidExerciseNameError);
}
