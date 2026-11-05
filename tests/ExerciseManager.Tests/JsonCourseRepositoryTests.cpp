#include "exercise_manager/JsonCourseRepository.h"

#include "exercise_manager/Errors.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

using exercise_manager::Course;
using exercise_manager::CourseNotFoundError;
using exercise_manager::Exercise;
using exercise_manager::JsonCourseRepository;
using exercise_manager::StorageError;
using exercise_manager::Workspace;

namespace
{

// Creates a fresh temporary workspace and deletes it when the test ends.
class JsonCourseRepositoryTests : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        m_root = fs::temp_directory_path() / (std::string("em_jsonrepo_") + info->name());
        fs::remove_all(m_root);
        m_workspace = std::make_unique<Workspace>(m_root);
        m_workspace->initialize();
        m_repository = std::make_unique<JsonCourseRepository>(*m_workspace);
    }

    void TearDown() override
    {
        m_repository.reset();
        m_workspace.reset();
        fs::remove_all(m_root);
    }

    static Course makeCourse(int id)
    {
        Course course;
        course.id = id;
        course.name = "C++ Basics";
        course.nextExerciseNumber = 4;
        course.exercises.push_back(Exercise{1, "adder", "Add two numbers"});
        course.exercises.push_back(Exercise{3, "جمع رقمين", "وصف بالعربي"});
        return course;
    }

    void writeRawCourseFile(int id, const std::string& text)
    {
        fs::create_directories(m_workspace->courseDir(id));
        std::ofstream out(m_workspace->courseFile(id), std::ios::binary);
        out << text;
    }

    fs::path m_root;
    std::unique_ptr<Workspace> m_workspace;
    std::unique_ptr<JsonCourseRepository> m_repository;
};

} // namespace

TEST_F(JsonCourseRepositoryTests, ExistsIsFalseForMissingCourse)
{
    EXPECT_FALSE(m_repository->exists(1));
}

TEST_F(JsonCourseRepositoryTests, SaveThenLoadReturnsSameData)
{
    m_repository->save(makeCourse(1));

    const Course loaded = m_repository->load(1);
    EXPECT_EQ(loaded.id, 1);
    EXPECT_EQ(loaded.name, "C++ Basics");
    EXPECT_EQ(loaded.nextExerciseNumber, 4);
    ASSERT_EQ(loaded.exercises.size(), 2u);
    EXPECT_EQ(loaded.exercises[0].number, 1);
    EXPECT_EQ(loaded.exercises[0].name, "adder");
    EXPECT_EQ(loaded.exercises[0].description, "Add two numbers");
    EXPECT_EQ(loaded.exercises[1].number, 3);
    EXPECT_EQ(loaded.exercises[1].name, "جمع رقمين");
    EXPECT_EQ(loaded.exercises[1].description, "وصف بالعربي");
}

TEST_F(JsonCourseRepositoryTests, SaveCreatesCourseFile)
{
    m_repository->save(makeCourse(2));

    EXPECT_TRUE(m_repository->exists(2));
    EXPECT_TRUE(fs::is_regular_file(m_workspace->courseFile(2)));
}

TEST_F(JsonCourseRepositoryTests, SaveLeavesNoTemporaryFile)
{
    m_repository->save(makeCourse(1));

    fs::path temp = m_workspace->courseFile(1);
    temp += ".tmp";
    EXPECT_FALSE(fs::exists(temp));
}

TEST_F(JsonCourseRepositoryTests, SaveOverwritesExistingCourse)
{
    m_repository->save(makeCourse(1));

    Course changed = makeCourse(1);
    changed.name = "Renamed";
    changed.exercises.clear();
    m_repository->save(changed);

    const Course loaded = m_repository->load(1);
    EXPECT_EQ(loaded.name, "Renamed");
    EXPECT_TRUE(loaded.exercises.empty());
}

TEST_F(JsonCourseRepositoryTests, LoadMissingCourseThrowsCourseNotFound)
{
    EXPECT_THROW(m_repository->load(5), CourseNotFoundError);
}

TEST_F(JsonCourseRepositoryTests, LoadInvalidJsonThrowsStorageError)
{
    writeRawCourseFile(1, "this is not json");
    EXPECT_THROW(m_repository->load(1), StorageError);
}

TEST_F(JsonCourseRepositoryTests, LoadMissingFieldThrowsStorageError)
{
    writeRawCourseFile(1, R"({"schema_version":1,"course":{"id":1,"name":"x"},"exercises":[]})");
    EXPECT_THROW(m_repository->load(1), StorageError);
}

TEST_F(JsonCourseRepositoryTests, LoadUnsupportedSchemaVersionThrowsStorageError)
{
    writeRawCourseFile(
        1, R"({"schema_version":99,"course":{"id":1,"name":"x"},"next_exercise_number":1,"exercises":[]})");
    EXPECT_THROW(m_repository->load(1), StorageError);
}

TEST_F(JsonCourseRepositoryTests, LoadCourseThatDoesNotMatchFolderThrowsStorageError)
{
    writeRawCourseFile(
        1, R"({"schema_version":1,"course":{"id":2,"name":"x"},"next_exercise_number":1,"exercises":[]})");
    EXPECT_THROW(m_repository->load(1), StorageError);
}

TEST_F(JsonCourseRepositoryTests, ListCourseIdsIsEmptyForNewWorkspace)
{
    EXPECT_TRUE(m_repository->listCourseIds().empty());
}

TEST_F(JsonCourseRepositoryTests, ListCourseIdsReturnsSortedIds)
{
    m_repository->save(makeCourse(3));
    m_repository->save(makeCourse(1));
    m_repository->save(makeCourse(10));

    const std::vector<int> ids = m_repository->listCourseIds();
    ASSERT_EQ(ids.size(), 3u);
    EXPECT_EQ(ids[0], 1);
    EXPECT_EQ(ids[1], 3);
    EXPECT_EQ(ids[2], 10);
}

TEST_F(JsonCourseRepositoryTests, ListCourseIdsIgnoresFoldersWithoutCourseFile)
{
    m_repository->save(makeCourse(1));
    fs::create_directories(m_workspace->coursesDir() / "Course02");
    fs::create_directories(m_workspace->coursesDir() / "notacourse");

    const std::vector<int> ids = m_repository->listCourseIds();
    ASSERT_EQ(ids.size(), 1u);
    EXPECT_EQ(ids[0], 1);
}
