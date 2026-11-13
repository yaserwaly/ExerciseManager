#include "exercise_manager/JsonCourseRepository.h"

#include "exercise_manager/Errors.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using exercise_manager::Course;
using exercise_manager::CourseNotFoundError;
using exercise_manager::Exercise;
using exercise_manager::InvalidExerciseNameError;
using exercise_manager::JsonCourseRepository;
using exercise_manager::StorageError;
using exercise_manager::Workspace;

namespace
{

const char* const kValidCoursesJson =
    R"({"schema_version":1,"courses":[{"id":1,"name":"x","description":"y","full_description":"z"}]})";

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
        course.description = "Basics of C++";
        course.fullDescription = "Basics of C++: variables, loops and functions";
        course.exercises.push_back(
            Exercise{1, "adder", "Add two numbers", "Adds two numbers and prints the sum"});
        course.exercises.push_back(Exercise{3, "جمع رقمين", "وصف بالعربي", "وصف كامل بالعربي"});
        return course;
    }

    static void writeRawFile(const fs::path& file, const std::string& text)
    {
        fs::create_directories(file.parent_path());
        std::ofstream out(file, std::ios::binary);
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
    EXPECT_EQ(loaded.description, "Basics of C++");
    EXPECT_EQ(loaded.fullDescription, "Basics of C++: variables, loops and functions");
    ASSERT_EQ(loaded.exercises.size(), 2u);
    EXPECT_EQ(loaded.exercises[0].number, 1);
    EXPECT_EQ(loaded.exercises[0].name, "adder");
    EXPECT_EQ(loaded.exercises[0].description, "Add two numbers");
    EXPECT_EQ(loaded.exercises[0].fullDescription, "Adds two numbers and prints the sum");
    EXPECT_EQ(loaded.exercises[1].number, 3);
    EXPECT_EQ(loaded.exercises[1].name, "جمع رقمين");
    EXPECT_EQ(loaded.exercises[1].description, "وصف بالعربي");
    EXPECT_EQ(loaded.exercises[1].fullDescription, "وصف كامل بالعربي");
}

TEST_F(JsonCourseRepositoryTests, SaveCreatesCoursesFileAndExercisesFile)
{
    m_repository->save(makeCourse(2));

    EXPECT_TRUE(m_repository->exists(2));
    EXPECT_TRUE(fs::is_regular_file(m_workspace->coursesFile()));
    EXPECT_TRUE(fs::is_regular_file(m_workspace->exercisesFile(2)));
}

TEST_F(JsonCourseRepositoryTests, SaveLeavesNoTemporaryFile)
{
    m_repository->save(makeCourse(1));

    fs::path tempCourses = m_workspace->coursesFile();
    tempCourses += ".tmp";
    fs::path tempExercises = m_workspace->exercisesFile(1);
    tempExercises += ".tmp";
    EXPECT_FALSE(fs::exists(tempCourses));
    EXPECT_FALSE(fs::exists(tempExercises));
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

TEST_F(JsonCourseRepositoryTests, SaveKeepsOtherCourses)
{
    m_repository->save(makeCourse(1));
    m_repository->save(makeCourse(2));

    Course changed = makeCourse(1);
    changed.name = "Renamed";
    m_repository->save(changed);

    EXPECT_EQ(m_repository->load(1).name, "Renamed");
    EXPECT_EQ(m_repository->load(2).name, "C++ Basics");
    EXPECT_EQ(m_repository->listCourseIds(), (std::vector<int>{1, 2}));
}

TEST_F(JsonCourseRepositoryTests, SaveRejectsCourseNumberOutOfRange)
{
    EXPECT_THROW(m_repository->save(makeCourse(0)), InvalidExerciseNameError);
    EXPECT_FALSE(fs::exists(m_workspace->coursesFile()));
}

TEST_F(JsonCourseRepositoryTests, LoadMissingCourseThrowsCourseNotFound)
{
    EXPECT_THROW(m_repository->load(5), CourseNotFoundError);
}

TEST_F(JsonCourseRepositoryTests, LoadInvalidCoursesJsonThrowsStorageError)
{
    writeRawFile(m_workspace->coursesFile(), "this is not json");
    EXPECT_THROW(m_repository->load(1), StorageError);
}

TEST_F(JsonCourseRepositoryTests, LoadCoursesMissingFieldThrowsStorageError)
{
    writeRawFile(m_workspace->coursesFile(),
                 R"({"schema_version":1,"courses":[{"id":1,"name":"x","description":"y"}]})");
    EXPECT_THROW(m_repository->load(1), StorageError);
}

TEST_F(JsonCourseRepositoryTests, LoadCoursesUnsupportedSchemaVersionThrowsStorageError)
{
    writeRawFile(m_workspace->coursesFile(), R"({"schema_version":99,"courses":[]})");
    EXPECT_THROW(m_repository->load(1), StorageError);
}

TEST_F(JsonCourseRepositoryTests, LoadCoursesWithDuplicateIdsThrowsStorageError)
{
    writeRawFile(m_workspace->coursesFile(),
                 R"({"schema_version":1,"courses":[)"
                 R"({"id":1,"name":"a","description":"a","full_description":"a"},)"
                 R"({"id":1,"name":"b","description":"b","full_description":"b"}]})");
    EXPECT_THROW(m_repository->load(1), StorageError);
}

TEST_F(JsonCourseRepositoryTests, LoadMissingExercisesFileThrowsStorageError)
{
    writeRawFile(m_workspace->coursesFile(), kValidCoursesJson);
    EXPECT_THROW(m_repository->load(1), StorageError);
}

TEST_F(JsonCourseRepositoryTests, LoadInvalidExercisesJsonThrowsStorageError)
{
    writeRawFile(m_workspace->coursesFile(), kValidCoursesJson);
    writeRawFile(m_workspace->exercisesFile(1), "this is not json");
    EXPECT_THROW(m_repository->load(1), StorageError);
}

TEST_F(JsonCourseRepositoryTests, LoadExercisesMissingFieldThrowsStorageError)
{
    writeRawFile(m_workspace->coursesFile(), kValidCoursesJson);
    writeRawFile(m_workspace->exercisesFile(1),
                 R"({"schema_version":1,"course_id":1,)"
                 R"("exercises":[{"number":1,"name":"a","description":"b"}]})");
    EXPECT_THROW(m_repository->load(1), StorageError);
}

TEST_F(JsonCourseRepositoryTests, LoadExercisesThatDoNotMatchFolderThrowsStorageError)
{
    writeRawFile(m_workspace->coursesFile(), kValidCoursesJson);
    writeRawFile(m_workspace->exercisesFile(1),
                 R"({"schema_version":1,"course_id":2,"exercises":[]})");
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

TEST_F(JsonCourseRepositoryTests, ListCourseIdsIgnoresFoldersThatAreNotListed)
{
    m_repository->save(makeCourse(1));
    fs::create_directories(m_workspace->coursesDir() / "Course02");
    fs::create_directories(m_workspace->coursesDir() / "notacourse");

    const std::vector<int> ids = m_repository->listCourseIds();
    ASSERT_EQ(ids.size(), 1u);
    EXPECT_EQ(ids[0], 1);
}

TEST_F(JsonCourseRepositoryTests, RemoveDeletesCourseFromList)
{
    m_repository->save(makeCourse(1));

    m_repository->remove(1);

    EXPECT_FALSE(m_repository->exists(1));
    EXPECT_TRUE(m_repository->listCourseIds().empty());
    EXPECT_THROW(m_repository->load(1), CourseNotFoundError);
}

TEST_F(JsonCourseRepositoryTests, RemoveKeepsOtherCourses)
{
    m_repository->save(makeCourse(1));
    m_repository->save(makeCourse(2));
    m_repository->save(makeCourse(3));

    m_repository->remove(2);

    EXPECT_EQ(m_repository->listCourseIds(), (std::vector<int>{1, 3}));
    EXPECT_EQ(m_repository->load(1).name, "C++ Basics");
    EXPECT_EQ(m_repository->load(3).exercises.size(), 2u);
}

TEST_F(JsonCourseRepositoryTests, RemoveMissingCourseThrowsCourseNotFound)
{
    m_repository->save(makeCourse(1));

    EXPECT_THROW(m_repository->remove(5), CourseNotFoundError);
    EXPECT_TRUE(m_repository->exists(1));
}

TEST_F(JsonCourseRepositoryTests, RemoveLeavesNoTemporaryFile)
{
    m_repository->save(makeCourse(1));

    m_repository->remove(1);

    fs::path tempCourses = m_workspace->coursesFile();
    tempCourses += ".tmp";
    EXPECT_FALSE(fs::exists(tempCourses));
}

TEST_F(JsonCourseRepositoryTests, RemoveDoesNotTouchTheCourseFolderFiles)
{
    m_repository->save(makeCourse(1));

    m_repository->remove(1);

    // The repository only changes the list. Deleting the folder is the service's job.
    EXPECT_TRUE(fs::is_regular_file(m_workspace->exercisesFile(1)));
}
