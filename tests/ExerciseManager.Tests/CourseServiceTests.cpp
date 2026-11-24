#include "exercise_manager/CourseService.h"

#include "exercise_manager/Errors.h"
#include "exercise_manager/FileNaming.h"
#include "exercise_manager/ICourseRepository.h"
#include "exercise_manager/Workspace.h"
#include "exercise_manager/DefaultsStore.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

namespace fs = std::filesystem;

using exercise_manager::Course;
using exercise_manager::CourseNotFoundError;
using exercise_manager::CourseService;
using exercise_manager::DuplicateCourseError;
using exercise_manager::Exercise;
using exercise_manager::FileNaming;
using exercise_manager::ICourseRepository;
using exercise_manager::InvalidExerciseNameError;
using exercise_manager::Workspace;
using exercise_manager::DefaultsStore;
using exercise_manager::InvalidPositionError;
using exercise_manager::StorageError;

namespace
{

// In-memory repository, so the service is tested without touching the disk.
class FakeCourseRepository : public ICourseRepository
{
  public:
    Course load(int courseId) const override
    {
        const auto it = find(courseId);
        if (it == m_courses.end())
        {
            throw CourseNotFoundError("Course does not exist.");
        }
        return *it;
    }

    void save(const Course& course) override
    {
        const auto it = find(course.id);
        if (it != m_courses.end())
        {
            *it = course;
        }
        else
        {
            m_courses.push_back(course);
        }
    }

    bool exists(int courseId) const override
    {
        return find(courseId) != m_courses.end();
    }

    void remove(int courseId) override
    {
        const auto it = find(courseId);
        if (it == m_courses.end())
        {
            throw CourseNotFoundError("Course does not exist.");
        }
        m_courses.erase(it);
    }

    std::vector<int> listCourseIds() const override
    {
        std::vector<int> ids;
        for (const Course& course : m_courses)
        {
            ids.push_back(course.id);
        }
        return ids;
    }

        void reorder(const std::vector<int>& orderedIds) override
    {
        if (orderedIds.size() != m_courses.size())
        {
            throw StorageError("The new order does not match the stored courses.");
        }

        std::vector<Course> reordered;
        for (const int id : orderedIds)
        {
            const auto stored = find(id);
            const bool repeated =
                std::any_of(reordered.begin(), reordered.end(),
                            [id](const Course& course) { return course.id == id; });
            if (stored == m_courses.end() || repeated)
            {
                throw StorageError("The new order does not match the stored courses.");
            }
            reordered.push_back(*stored);
        }
        m_courses = std::move(reordered);
    }

  private:
    std::vector<Course>::iterator find(int courseId)
    {
        return std::find_if(m_courses.begin(), m_courses.end(),
                            [courseId](const Course& course) { return course.id == courseId; });
    }

    std::vector<Course>::const_iterator find(int courseId) const
    {
        return std::find_if(m_courses.begin(), m_courses.end(),
                            [courseId](const Course& course) { return course.id == courseId; });
    }

    std::vector<Course> m_courses;
};

Course makeCourse(int id, const std::string& name)
{
    Course course;
    course.id = id;
    course.name = name;
    course.description = "short";
    course.fullDescription = "full";
    return course;
}

// Gives every test its own temporary workspace and an in-memory repository.
class CourseServiceFixture : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        m_root = fs::temp_directory_path() / (std::string("em_courseservice_") + info->name());
        fs::remove_all(m_root);
        m_workspace = std::make_unique<Workspace>(m_root);
        m_workspace->initialize();
        m_service = std::make_unique<CourseService>(m_repository, *m_workspace);
    }

    void TearDown() override
    {
        m_service.reset();
        m_workspace.reset();
        fs::remove_all(m_root);
    }

    // Creates a course folder with a few files, like a real course would have.
    void createCourseFolder(int courseId) const
    {
        fs::create_directories(m_workspace->courseDir(courseId));
        std::ofstream(m_workspace->courseDir(courseId) / "C01Exercise001.hpp") << "// header";
        std::ofstream(m_workspace->courseDir(courseId) / "C01Exercise001.cpp") << "// source";
    }

    fs::path m_root;
    FakeCourseRepository m_repository;
    std::unique_ptr<Workspace> m_workspace;
    std::unique_ptr<CourseService> m_service;
};

} // namespace

// ---------- nextCourseId (static, no service needed) ----------

TEST(CourseServiceTests, NextIdIsOneWhenThereAreNoCourses)
{
    EXPECT_EQ(CourseService::nextCourseId({}), 1);
}

TEST(CourseServiceTests, NextIdContinuesAfterLast)
{
    EXPECT_EQ(CourseService::nextCourseId({1, 2, 3}), 4);
}

TEST(CourseServiceTests, NextIdFillsFirstGap)
{
    EXPECT_EQ(CourseService::nextCourseId({1, 2, 4, 5}), 3);
}

TEST(CourseServiceTests, NextIdReusesNumberOneWhenFirstIsMissing)
{
    EXPECT_EQ(CourseService::nextCourseId({2, 3}), 1);
}

TEST(CourseServiceTests, NextIdIgnoresOrderOfInput)
{
    EXPECT_EQ(CourseService::nextCourseId({5, 1, 3, 2}), 4);
}

TEST(CourseServiceTests, NextIdThrowsWhenAllNumbersAreUsed)
{
    std::vector<int> all;
    for (int i = 1; i <= FileNaming::kMaxCourseId; ++i)
    {
        all.push_back(i);
    }
    EXPECT_THROW(CourseService::nextCourseId(all), InvalidExerciseNameError);
}

// ---------- addCourse ----------

TEST_F(CourseServiceFixture, AddCourseGivesNumberOneToFirstCourse)
{
    EXPECT_EQ(m_service->addCourse("C++ Basics", "short", "full"), 1);

    const Course course = m_repository.load(1);
    EXPECT_EQ(course.id, 1);
    EXPECT_EQ(course.name, "C++ Basics");
    EXPECT_EQ(course.description, "short");
    EXPECT_EQ(course.fullDescription, "full");
    EXPECT_TRUE(course.exercises.empty());
}

TEST_F(CourseServiceFixture, AddCourseTakesFirstFreeNumber)
{
    m_repository.save(makeCourse(1, "one"));
    m_repository.save(makeCourse(3, "three"));

    EXPECT_EQ(m_service->addCourse("two", "", ""), 2);
}

TEST_F(CourseServiceFixture, AddCourseKeepsOtherCourses)
{
    m_repository.save(makeCourse(1, "one"));

    m_service->addCourse("two", "", "");

    EXPECT_EQ(m_repository.load(1).name, "one");
    EXPECT_EQ(m_repository.listCourseIds(), (std::vector<int>{1, 2}));
}

TEST_F(CourseServiceFixture, AddCourseRejectsInvalidName)
{
    EXPECT_THROW(m_service->addCourse("bad/name", "", ""), InvalidExerciseNameError);
    EXPECT_TRUE(m_repository.listCourseIds().empty());
}

TEST_F(CourseServiceFixture, AddCourseRejectsDuplicateName)
{
    m_service->addCourse("C++ Basics", "", "");

    EXPECT_THROW(m_service->addCourse("C++ Basics", "", ""), DuplicateCourseError);
    EXPECT_EQ(m_repository.listCourseIds().size(), 1u);
}

// ---------- updateCourse ----------

TEST_F(CourseServiceFixture, UpdateCourseChangesNameAndDescriptions)
{
    m_service->addCourse("old", "old short", "old full");

    m_service->updateCourse(1, "new", "new short", "new full");

    const Course course = m_repository.load(1);
    EXPECT_EQ(course.id, 1);
    EXPECT_EQ(course.name, "new");
    EXPECT_EQ(course.description, "new short");
    EXPECT_EQ(course.fullDescription, "new full");
}

TEST_F(CourseServiceFixture, UpdateCourseKeepsItsExercises)
{
    Course course = makeCourse(1, "old");
    course.exercises.push_back(Exercise{1, "adder", "a", "b"});
    m_repository.save(course);

    m_service->updateCourse(1, "new", "", "");

    const Course loaded = m_repository.load(1);
    ASSERT_EQ(loaded.exercises.size(), 1u);
    EXPECT_EQ(loaded.exercises[0].name, "adder");
}

TEST_F(CourseServiceFixture, UpdateCourseAllowsKeepingOwnName)
{
    m_service->addCourse("same", "old", "old");

    EXPECT_NO_THROW(m_service->updateCourse(1, "same", "new", "new"));
    EXPECT_EQ(m_repository.load(1).description, "new");
}

TEST_F(CourseServiceFixture, UpdateCourseThrowsForMissingCourse)
{
    EXPECT_THROW(m_service->updateCourse(5, "name", "", ""), CourseNotFoundError);
}

TEST_F(CourseServiceFixture, UpdateCourseRejectsInvalidName)
{
    m_service->addCourse("good", "", "");

    EXPECT_THROW(m_service->updateCourse(1, "bad/name", "", ""), InvalidExerciseNameError);
    EXPECT_EQ(m_repository.load(1).name, "good");
}

TEST_F(CourseServiceFixture, UpdateCourseRejectsNameOfAnotherCourse)
{
    m_service->addCourse("first", "", "");
    m_service->addCourse("second", "", "");

    EXPECT_THROW(m_service->updateCourse(2, "first", "", ""), DuplicateCourseError);
    EXPECT_EQ(m_repository.load(2).name, "second");
}

// ---------- listCourses ----------

TEST_F(CourseServiceFixture, ListCoursesIsEmptyWhenThereAreNoCourses)
{
    EXPECT_TRUE(m_service->listCourses().empty());
}

TEST_F(CourseServiceFixture, ListCoursesReturnsCoursesInStorageOrder)
{
    m_repository.save(makeCourse(3, "three"));
    m_repository.save(makeCourse(1, "one"));
    m_repository.save(makeCourse(10, "ten"));

    const std::vector<Course> courses = m_service->listCourses();
    ASSERT_EQ(courses.size(), 3u);
    EXPECT_EQ(courses[0].id, 3);
    EXPECT_EQ(courses[1].id, 1);
    EXPECT_EQ(courses[2].id, 10);
    EXPECT_EQ(courses[2].name, "ten");
}

TEST_F(CourseServiceFixture, ANewCourseThatReusesAFreeNumberIsListedLast)
{
    m_service->addCourse("first", "", "");  // number 1
    m_service->addCourse("second", "", ""); // number 2
    m_service->addCourse("third", "", "");  // number 3
    m_service->deleteCourse(1);

    EXPECT_EQ(m_service->addCourse("fourth", "", ""), 1);

    const std::vector<Course> courses = m_service->listCourses();
    ASSERT_EQ(courses.size(), 3u);
    EXPECT_EQ(courses[0].id, 2);
    EXPECT_EQ(courses[1].id, 3);
    EXPECT_EQ(courses[2].id, 1);
    EXPECT_EQ(courses[2].name, "fourth");
}

// ---------- deleteCourse ----------

TEST_F(CourseServiceFixture, DeleteCourseRemovesDataAndFolder)
{
    m_service->addCourse("C++ Basics", "", "");
    createCourseFolder(1);

    m_service->deleteCourse(1);

    EXPECT_FALSE(m_repository.exists(1));
    EXPECT_FALSE(fs::exists(m_workspace->courseDir(1)));
}

TEST_F(CourseServiceFixture, DeleteCourseWorksWhenFolderDoesNotExist)
{
    m_service->addCourse("C++ Basics", "", "");

    EXPECT_NO_THROW(m_service->deleteCourse(1));
    EXPECT_FALSE(m_repository.exists(1));
}

TEST_F(CourseServiceFixture, DeleteCourseKeepsOtherCoursesAndFolders)
{
    m_service->addCourse("first", "", "");
    m_service->addCourse("second", "", "");
    createCourseFolder(1);
    createCourseFolder(2);

    m_service->deleteCourse(1);

    EXPECT_EQ(m_repository.listCourseIds(), (std::vector<int>{2}));
    EXPECT_TRUE(fs::is_directory(m_workspace->courseDir(2)));
    EXPECT_TRUE(fs::is_regular_file(m_workspace->courseDir(2) / "C01Exercise001.hpp"));
}

TEST_F(CourseServiceFixture, DeletedNumberIsReusedByNextCourse)
{
    m_service->addCourse("first", "", "");
    m_service->addCourse("second", "", "");

    m_service->deleteCourse(1);

    EXPECT_EQ(m_service->addCourse("third", "", ""), 1);
}

TEST_F(CourseServiceFixture, DeleteCourseThrowsForMissingCourse)
{
    EXPECT_THROW(m_service->deleteCourse(5), CourseNotFoundError);
}

TEST_F(CourseServiceFixture, DeleteCourseClearsItsDefaultCourse)
{
    m_service->addCourse("first", "", "");
    DefaultsStore(*m_workspace).saveDefaultCourse(1);

    m_service->deleteCourse(1);

    EXPECT_FALSE(DefaultsStore(*m_workspace).loadDefaultCourse().has_value());
}

TEST_F(CourseServiceFixture, DeleteCourseKeepsDefaultOfAnotherCourse)
{
    m_service->addCourse("first", "", "");
    m_service->addCourse("second", "", "");
    DefaultsStore(*m_workspace).saveDefaultCourse(2);

    m_service->deleteCourse(1);

    EXPECT_EQ(DefaultsStore(*m_workspace).loadDefaultCourse(), 2);
}

TEST_F(CourseServiceFixture, DeleteCourseRemovesItsDefaultExercise)
{
    m_service->addCourse("first", "", "");
    createCourseFolder(1);
    DefaultsStore(*m_workspace).saveDefaultExercise(1, 1);

    m_service->deleteCourse(1);

    EXPECT_FALSE(fs::exists(m_workspace->defaultExerciseFile(1)));
}

TEST_F(CourseServiceFixture, DeletedCourseNumberDoesNotInheritTheDefault)
{
    m_service->addCourse("first", "", "");
    DefaultsStore(*m_workspace).saveDefaultCourse(1);
    m_service->deleteCourse(1);

    EXPECT_EQ(m_service->addCourse("third", "", ""), 1);

    EXPECT_FALSE(DefaultsStore(*m_workspace).loadDefaultCourse().has_value());
}

TEST_F(CourseServiceFixture, DeleteCourseWorksWhenDefaultFileIsCorrupt)
{
    m_service->addCourse("first", "", "");
    std::ofstream(m_workspace->defaultCourseFile()) << "this is not json";

    EXPECT_NO_THROW(m_service->deleteCourse(1));
    EXPECT_FALSE(m_repository.exists(1));
}

// ---------- moveCourse ----------

TEST_F(CourseServiceFixture, MoveCourseToTheFirstPosition)
{
    m_service->addCourse("a", "", "");
    m_service->addCourse("b", "", "");
    m_service->addCourse("c", "", "");
    m_service->addCourse("d", "", "");

    m_service->moveCourse(4, 1);

    EXPECT_EQ(m_repository.listCourseIds(), (std::vector<int>{4, 1, 2, 3}));
}

TEST_F(CourseServiceFixture, MoveCourseToTheLastPosition)
{
    m_service->addCourse("a", "", "");
    m_service->addCourse("b", "", "");
    m_service->addCourse("c", "", "");
    m_service->addCourse("d", "", "");

    m_service->moveCourse(1, 4);

    EXPECT_EQ(m_repository.listCourseIds(), (std::vector<int>{2, 3, 4, 1}));
}

TEST_F(CourseServiceFixture, MoveCourseToTheMiddle)
{
    m_service->addCourse("a", "", "");
    m_service->addCourse("b", "", "");
    m_service->addCourse("c", "", "");
    m_service->addCourse("d", "", "");

    m_service->moveCourse(4, 2);

    EXPECT_EQ(m_repository.listCourseIds(), (std::vector<int>{1, 4, 2, 3}));
}

TEST_F(CourseServiceFixture, MoveCourseToItsOwnPositionChangesNothing)
{
    m_service->addCourse("a", "", "");
    m_service->addCourse("b", "", "");
    m_service->addCourse("c", "", "");

    EXPECT_NO_THROW(m_service->moveCourse(2, 2));

    EXPECT_EQ(m_repository.listCourseIds(), (std::vector<int>{1, 2, 3}));
}

TEST_F(CourseServiceFixture, MoveCourseRejectsPositionOutOfRange)
{
    m_service->addCourse("a", "", "");
    m_service->addCourse("b", "", "");

    EXPECT_THROW(m_service->moveCourse(1, 0), InvalidPositionError);
    EXPECT_THROW(m_service->moveCourse(1, 3), InvalidPositionError);
    EXPECT_THROW(m_service->moveCourse(1, -2), InvalidPositionError);
    EXPECT_EQ(m_repository.listCourseIds(), (std::vector<int>{1, 2}));
}

TEST_F(CourseServiceFixture, MoveCourseThrowsForMissingCourse)
{
    m_service->addCourse("a", "", "");

    EXPECT_THROW(m_service->moveCourse(5, 1), CourseNotFoundError);
    EXPECT_EQ(m_repository.listCourseIds(), (std::vector<int>{1}));
}

TEST_F(CourseServiceFixture, MoveCourseUsesTheRealNumberNotThePosition)
{
    m_service->addCourse("first", "", "");  // number 1
    m_service->addCourse("second", "", ""); // number 2
    m_service->addCourse("third", "", "");  // number 3
    m_service->deleteCourse(1);
    m_service->addCourse("fourth", "", ""); // takes the free number 1, stored last: 2, 3, 1

    // Number 1 is "fourth", which is at position 3. Moving it to position 1 must pick it.
    m_service->moveCourse(1, 1);

    EXPECT_EQ(m_repository.listCourseIds(), (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(m_repository.load(1).name, "fourth");
}

TEST_F(CourseServiceFixture, MoveCourseKeepsFoldersExercisesAndTheDefaultCourse)
{
    m_service->addCourse("a", "", "");
    m_service->addCourse("b", "", "");
    m_service->addCourse("c", "", "");
    createCourseFolder(1);
    Course first = m_repository.load(1);
    first.exercises.push_back(Exercise{1, "adder", "a", "b"});
    m_repository.save(first);
    DefaultsStore(*m_workspace).saveDefaultCourse(3);

    m_service->moveCourse(3, 1);

    EXPECT_EQ(m_repository.listCourseIds(), (std::vector<int>{3, 1, 2}));
    EXPECT_TRUE(fs::is_directory(m_workspace->courseDir(1)));
    EXPECT_EQ(m_repository.load(1).exercises.size(), 1u);
    EXPECT_EQ(DefaultsStore(*m_workspace).loadDefaultCourse(), 3);
}
