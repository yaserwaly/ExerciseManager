#include "exercise_manager/DefaultsStore.h"

#include "exercise_manager/Errors.h"
#include "exercise_manager/FileNaming.h"
#include "exercise_manager/Workspace.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

namespace fs = std::filesystem;

using exercise_manager::DefaultsStore;
using exercise_manager::FileNaming;
using exercise_manager::InvalidExerciseNameError;
using exercise_manager::StorageError;
using exercise_manager::Workspace;
using exercise_manager::WorkspaceError;

namespace
{

// Creates a fresh temporary workspace and deletes it when the test ends.
class DefaultsStoreTests : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        m_root = fs::temp_directory_path() / (std::string("em_defaults_") + info->name());
        fs::remove_all(m_root);
        m_workspace = std::make_unique<Workspace>(m_root);
        m_workspace->initialize();
        m_store = std::make_unique<DefaultsStore>(*m_workspace);
    }

    void TearDown() override
    {
        m_store.reset();
        m_workspace.reset();
        fs::remove_all(m_root);
    }

    static void writeRawFile(const fs::path& file, const std::string& text)
    {
        fs::create_directories(file.parent_path());
        std::ofstream out(file, std::ios::binary);
        out << text;
    }

    void createCourseFolder(int courseId) const
    {
        fs::create_directories(m_workspace->courseDir(courseId));
    }

    fs::path m_root;
    std::unique_ptr<Workspace> m_workspace;
    std::unique_ptr<DefaultsStore> m_store;
};

} // namespace

// ---------- default course ----------

TEST_F(DefaultsStoreTests, LoadDefaultCourseIsEmptyWhenNotSet)
{
    EXPECT_FALSE(m_store->loadDefaultCourse().has_value());
}

TEST_F(DefaultsStoreTests, SaveThenLoadDefaultCourse)
{
    m_store->saveDefaultCourse(10);

    const auto loaded = m_store->loadDefaultCourse();
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(*loaded, 10);
    EXPECT_TRUE(fs::is_regular_file(m_workspace->defaultCourseFile()));
}

TEST_F(DefaultsStoreTests, SaveDefaultCourseOverwritesOldValue)
{
    m_store->saveDefaultCourse(3);
    m_store->saveDefaultCourse(7);

    EXPECT_EQ(m_store->loadDefaultCourse(), 7);
}

TEST_F(DefaultsStoreTests, SaveDefaultCourseLeavesNoTemporaryFile)
{
    m_store->saveDefaultCourse(1);

    fs::path temp = m_workspace->defaultCourseFile();
    temp += ".tmp";
    EXPECT_FALSE(fs::exists(temp));
}

TEST_F(DefaultsStoreTests, ClearDefaultCourseRemovesIt)
{
    m_store->saveDefaultCourse(2);

    m_store->clearDefaultCourse();

    EXPECT_FALSE(m_store->loadDefaultCourse().has_value());
    EXPECT_FALSE(fs::exists(m_workspace->defaultCourseFile()));
}

TEST_F(DefaultsStoreTests, ClearDefaultCourseDoesNothingWhenNotSet)
{
    EXPECT_NO_THROW(m_store->clearDefaultCourse());
}

TEST_F(DefaultsStoreTests, SaveDefaultCourseRejectsNumberOutOfRange)
{
    EXPECT_THROW(m_store->saveDefaultCourse(0), InvalidExerciseNameError);
    EXPECT_THROW(m_store->saveDefaultCourse(FileNaming::kMaxCourseId + 1),
                 InvalidExerciseNameError);
    EXPECT_FALSE(fs::exists(m_workspace->defaultCourseFile()));
}

TEST_F(DefaultsStoreTests, LoadDefaultCourseInvalidJsonThrowsStorageError)
{
    writeRawFile(m_workspace->defaultCourseFile(), "this is not json");
    EXPECT_THROW(m_store->loadDefaultCourse(), StorageError);
}

TEST_F(DefaultsStoreTests, LoadDefaultCourseMissingFieldThrowsStorageError)
{
    writeRawFile(m_workspace->defaultCourseFile(), R"({"schema_version":1})");
    EXPECT_THROW(m_store->loadDefaultCourse(), StorageError);
}

TEST_F(DefaultsStoreTests, LoadDefaultCourseUnsupportedSchemaThrowsStorageError)
{
    writeRawFile(m_workspace->defaultCourseFile(), R"({"schema_version":99,"course_id":1})");
    EXPECT_THROW(m_store->loadDefaultCourse(), StorageError);
}

TEST_F(DefaultsStoreTests, LoadDefaultCourseNumberOutOfRangeThrowsStorageError)
{
    writeRawFile(m_workspace->defaultCourseFile(), R"({"schema_version":1,"course_id":500})");
    EXPECT_THROW(m_store->loadDefaultCourse(), StorageError);
}

// ---------- default exercise ----------

TEST_F(DefaultsStoreTests, LoadDefaultExerciseIsEmptyWhenNotSet)
{
    EXPECT_FALSE(m_store->loadDefaultExercise(1).has_value());
}

TEST_F(DefaultsStoreTests, SaveThenLoadDefaultExercise)
{
    createCourseFolder(10);

    m_store->saveDefaultExercise(10, 4);

    const auto loaded = m_store->loadDefaultExercise(10);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(*loaded, 4);
    EXPECT_TRUE(fs::is_regular_file(m_workspace->defaultExerciseFile(10)));
}

TEST_F(DefaultsStoreTests, DefaultExercisesOfDifferentCoursesAreIndependent)
{
    createCourseFolder(1);
    createCourseFolder(2);

    m_store->saveDefaultExercise(1, 5);
    m_store->saveDefaultExercise(2, 9);

    EXPECT_EQ(m_store->loadDefaultExercise(1), 5);
    EXPECT_EQ(m_store->loadDefaultExercise(2), 9);
}

TEST_F(DefaultsStoreTests, ClearDefaultExerciseRemovesItAndKeepsOtherCourses)
{
    createCourseFolder(1);
    createCourseFolder(2);
    m_store->saveDefaultExercise(1, 5);
    m_store->saveDefaultExercise(2, 9);

    m_store->clearDefaultExercise(1);

    EXPECT_FALSE(m_store->loadDefaultExercise(1).has_value());
    EXPECT_EQ(m_store->loadDefaultExercise(2), 9);
}

TEST_F(DefaultsStoreTests, ClearDefaultExerciseDoesNothingWhenNotSet)
{
    EXPECT_NO_THROW(m_store->clearDefaultExercise(1));
}

TEST_F(DefaultsStoreTests, SaveDefaultExerciseThrowsWhenCourseFolderIsMissing)
{
    EXPECT_THROW(m_store->saveDefaultExercise(3, 1), WorkspaceError);
    EXPECT_FALSE(fs::exists(m_workspace->courseDir(3)));
}

TEST_F(DefaultsStoreTests, SaveDefaultExerciseRejectsExerciseNumberOutOfRange)
{
    createCourseFolder(1);

    EXPECT_THROW(m_store->saveDefaultExercise(1, 0), InvalidExerciseNameError);
    EXPECT_THROW(m_store->saveDefaultExercise(1, FileNaming::kMaxExerciseNumber + 1),
                 InvalidExerciseNameError);
    EXPECT_FALSE(fs::exists(m_workspace->defaultExerciseFile(1)));
}

TEST_F(DefaultsStoreTests, DefaultExerciseRejectsCourseNumberOutOfRange)
{
    EXPECT_THROW(m_store->loadDefaultExercise(0), InvalidExerciseNameError);
    EXPECT_THROW(m_store->saveDefaultExercise(0, 1), InvalidExerciseNameError);
    EXPECT_THROW(m_store->clearDefaultExercise(FileNaming::kMaxCourseId + 1),
                 InvalidExerciseNameError);
}

TEST_F(DefaultsStoreTests, SaveDefaultExerciseLeavesNoTemporaryFile)
{
    createCourseFolder(1);

    m_store->saveDefaultExercise(1, 2);

    fs::path temp = m_workspace->defaultExerciseFile(1);
    temp += ".tmp";
    EXPECT_FALSE(fs::exists(temp));
}

TEST_F(DefaultsStoreTests, LoadDefaultExerciseInvalidJsonThrowsStorageError)
{
    writeRawFile(m_workspace->defaultExerciseFile(1), "this is not json");
    EXPECT_THROW(m_store->loadDefaultExercise(1), StorageError);
}

TEST_F(DefaultsStoreTests, CourseAndExerciseDefaultsDoNotAffectEachOther)
{
    createCourseFolder(1);
    m_store->saveDefaultCourse(1);
    m_store->saveDefaultExercise(1, 3);

    m_store->clearDefaultCourse();

    EXPECT_FALSE(m_store->loadDefaultCourse().has_value());
    EXPECT_EQ(m_store->loadDefaultExercise(1), 3);
}
