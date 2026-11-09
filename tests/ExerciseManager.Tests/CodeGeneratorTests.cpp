#include "exercise_manager/CodeGenerator.h"

#include "exercise_manager/Errors.h"
#include "exercise_manager/FileNaming.h"
#include "exercise_manager/Workspace.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>

namespace fs = std::filesystem;

using exercise_manager::CodeGenerator;
using exercise_manager::FileNaming;
using exercise_manager::InvalidExerciseNameError;
using exercise_manager::Workspace;
using exercise_manager::WorkspaceError;

namespace
{

// Gives every test its own temporary workspace and deletes it afterwards.
class CodeGeneratorTests : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        m_root = fs::temp_directory_path() / (std::string("em_codegen_") + info->name());
        fs::remove_all(m_root);
        m_workspace = std::make_unique<Workspace>(m_root);
        m_workspace->initialize();
        m_generator = std::make_unique<CodeGenerator>(*m_workspace);
    }

    void TearDown() override
    {
        m_generator.reset();
        m_workspace.reset();
        fs::remove_all(m_root);
    }

    fs::path headerPath(int courseId, int number) const
    {
        return m_workspace->courseDir(courseId) / FileNaming::headerFileName(courseId, number);
    }

    fs::path sourcePath(int courseId, int number) const
    {
        return m_workspace->courseDir(courseId) / FileNaming::sourceFileName(courseId, number);
    }

    static std::string readFile(const fs::path& file)
    {
        std::ifstream in(file, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }

    static void writeFile(const fs::path& file, const std::string& text)
    {
        fs::create_directories(file.parent_path());
        std::ofstream out(file, std::ios::binary);
        out << text;
    }

    static bool contains(const std::string& text, const std::string& part)
    {
        return text.find(part) != std::string::npos;
    }

    fs::path m_root;
    std::unique_ptr<Workspace> m_workspace;
    std::unique_ptr<CodeGenerator> m_generator;
};

} // namespace

TEST_F(CodeGeneratorTests, CreatesHeaderAndSourceFiles)
{
    m_generator->generateExerciseFiles(1, 1);

    EXPECT_TRUE(fs::is_regular_file(headerPath(1, 1)));
    EXPECT_TRUE(fs::is_regular_file(sourcePath(1, 1)));
}

TEST_F(CodeGeneratorTests, CreatesCourseFolderWhenMissing)
{
    ASSERT_FALSE(fs::exists(m_workspace->courseDir(2)));

    m_generator->generateExerciseFiles(2, 1);

    EXPECT_TRUE(fs::is_directory(m_workspace->courseDir(2)));
}

TEST_F(CodeGeneratorTests, HeaderContainsNamespaceAndClassName)
{
    m_generator->generateExerciseFiles(2, 13);

    const std::string header = readFile(headerPath(2, 13));
    EXPECT_TRUE(contains(header, "#pragma once"));
    EXPECT_TRUE(contains(header, "namespace C02Exercise013 {"));
    EXPECT_TRUE(contains(header, "class ClsC02Exercise013 {"));
    EXPECT_TRUE(contains(header, "static void Run();"));
}

TEST_F(CodeGeneratorTests, SourceIncludesItsHeaderAndDefinesRun)
{
    m_generator->generateExerciseFiles(2, 13);

    const std::string source = readFile(sourcePath(2, 13));
    EXPECT_TRUE(contains(source, "#include \"C02Exercise013.hpp\""));
    EXPECT_TRUE(contains(source, "namespace C02Exercise013 {"));
    EXPECT_TRUE(contains(source, "void ClsC02Exercise013::Run()"));
    EXPECT_TRUE(contains(source, "Hello from Exercise 13!"));
}

TEST_F(CodeGeneratorTests, GeneratedFilesHaveNoLeftoverPlaceholders)
{
    m_generator->generateExerciseFiles(1, 1);

    EXPECT_FALSE(contains(readFile(headerPath(1, 1)), "{{"));
    EXPECT_FALSE(contains(readFile(sourcePath(1, 1)), "{{"));
}

TEST_F(CodeGeneratorTests, ThrowsWhenHeaderAlreadyExistsAndKeepsIt)
{
    writeFile(headerPath(1, 1), "my own code");

    EXPECT_THROW(m_generator->generateExerciseFiles(1, 1), WorkspaceError);
    EXPECT_EQ(readFile(headerPath(1, 1)), "my own code");
    EXPECT_FALSE(fs::exists(sourcePath(1, 1)));
}

TEST_F(CodeGeneratorTests, ThrowsWhenSourceAlreadyExistsAndKeepsIt)
{
    writeFile(sourcePath(1, 1), "my own code");

    EXPECT_THROW(m_generator->generateExerciseFiles(1, 1), WorkspaceError);
    EXPECT_EQ(readFile(sourcePath(1, 1)), "my own code");
    EXPECT_FALSE(fs::exists(headerPath(1, 1)));
}

TEST_F(CodeGeneratorTests, DoesNotTouchOtherExercises)
{
    m_generator->generateExerciseFiles(1, 1);
    const std::string firstHeader = readFile(headerPath(1, 1));

    m_generator->generateExerciseFiles(1, 2);

    EXPECT_EQ(readFile(headerPath(1, 1)), firstHeader);
    EXPECT_TRUE(fs::is_regular_file(headerPath(1, 2)));
    EXPECT_TRUE(fs::is_regular_file(sourcePath(1, 2)));
}

TEST_F(CodeGeneratorTests, RejectsCourseNumberOutOfRange)
{
    EXPECT_THROW(m_generator->generateExerciseFiles(0, 1), InvalidExerciseNameError);
    EXPECT_THROW(m_generator->generateExerciseFiles(FileNaming::kMaxCourseId + 1, 1),
                 InvalidExerciseNameError);
}

TEST_F(CodeGeneratorTests, RejectsExerciseNumberOutOfRange)
{
    EXPECT_THROW(m_generator->generateExerciseFiles(1, 0), InvalidExerciseNameError);
    EXPECT_THROW(m_generator->generateExerciseFiles(1, FileNaming::kMaxExerciseNumber + 1),
                 InvalidExerciseNameError);
    EXPECT_FALSE(fs::exists(m_workspace->courseDir(1)));
}
