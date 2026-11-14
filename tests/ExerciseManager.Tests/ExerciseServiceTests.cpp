#include "exercise_manager/ExerciseService.h"

#include "exercise_manager/Errors.h"
#include "exercise_manager/FileNaming.h"
#include "exercise_manager/ICourseRepository.h"
#include "exercise_manager/Workspace.h"
#include "exercise_manager/DefaultsStore.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using exercise_manager::Course;
using exercise_manager::CourseNotFoundError;
using exercise_manager::DuplicateExerciseError;
using exercise_manager::Exercise;
using exercise_manager::ExerciseNotFoundError;
using exercise_manager::ExerciseService;
using exercise_manager::FileNaming;
using exercise_manager::ICourseRepository;
using exercise_manager::InvalidExerciseNameError;
using exercise_manager::Workspace;
using exercise_manager::StorageError;
using exercise_manager::WorkspaceError;
using exercise_manager::DefaultsStore;

namespace
{

Course makeCourseWithNumbers(const std::vector<int>& numbers)
{
    Course course;
    course.id = 1;
    for (const int number : numbers)
    {
        Exercise exercise;
        exercise.number = number;
        course.exercises.push_back(exercise);
    }
    return course;
}

// In-memory repository, so the service is tested without touching the disk.
class FakeCourseRepository : public ICourseRepository
{
  public:
    Course load(int courseId) const override
    {
        const auto it = m_courses.find(courseId);
        if (it == m_courses.end())
        {
            throw CourseNotFoundError("Course does not exist.");
        }
        return it->second;
    }

    void save(const Course& course) override
    {
        if (m_failOnSave)
        {
            throw StorageError("Could not write.");
        }
        m_courses[course.id] = course;
    }

    void setFailOnSave(bool fail)
    {
        m_failOnSave = fail;
    }

    bool exists(int courseId) const override
    {
        return m_courses.count(courseId) > 0;
    }

    void remove(int courseId) override
    {
        if (m_courses.erase(courseId) == 0)
        {
            throw CourseNotFoundError("Course does not exist.");
        }
    }

    std::vector<int> listCourseIds() const override
    {
        std::vector<int> ids;
        for (const auto& entry : m_courses)
        {
            ids.push_back(entry.first);
        }
        return ids;
    }

  private:
    std::map<int, Course> m_courses;
    bool m_failOnSave = false;
};

// Gives every test its own temporary workspace and an in-memory repository.
class ExerciseServiceFixture : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        m_root = fs::temp_directory_path() / (std::string("em_service_") + info->name());
        fs::remove_all(m_root);
        m_workspace = std::make_unique<Workspace>(m_root);
        m_workspace->initialize();
        m_service = std::make_unique<ExerciseService>(m_repository, *m_workspace);
    }

    void TearDown() override
    {
        m_service.reset();
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

    void createFiles(int courseId, int number) const
    {
        fs::create_directories(m_workspace->courseDir(courseId));
        std::ofstream(headerPath(courseId, number)) << "// header";
        std::ofstream(sourcePath(courseId, number)) << "// source";
    }

    fs::path m_root;
    FakeCourseRepository m_repository;
    std::unique_ptr<Workspace> m_workspace;
    std::unique_ptr<ExerciseService> m_service;
};

} // namespace

// ---------- nextExerciseNumber (static, no service needed) ----------

TEST(ExerciseServiceTests, NextNumberIsOneForEmptyCourse)
{
    EXPECT_EQ(ExerciseService::nextExerciseNumber(makeCourseWithNumbers({})), 1);
}

TEST(ExerciseServiceTests, NextNumberContinuesAfterLast)
{
    EXPECT_EQ(ExerciseService::nextExerciseNumber(makeCourseWithNumbers({1, 2, 3})), 4);
}

TEST(ExerciseServiceTests, NextNumberFillsFirstGap)
{
    EXPECT_EQ(ExerciseService::nextExerciseNumber(makeCourseWithNumbers({1, 2, 4, 5})), 3);
}

TEST(ExerciseServiceTests, NextNumberReusesNumberOneWhenFirstIsMissing)
{
    EXPECT_EQ(ExerciseService::nextExerciseNumber(makeCourseWithNumbers({2, 3})), 1);
}

TEST(ExerciseServiceTests, NextNumberIgnoresOrderOfExercises)
{
    EXPECT_EQ(ExerciseService::nextExerciseNumber(makeCourseWithNumbers({5, 1, 3, 2})), 4);
}

TEST(ExerciseServiceTests, NextNumberThrowsWhenCourseIsFull)
{
    std::vector<int> all;
    for (int i = 1; i <= FileNaming::kMaxExerciseNumber; ++i)
    {
        all.push_back(i);
    }
    EXPECT_THROW(ExerciseService::nextExerciseNumber(makeCourseWithNumbers(all)),
                 InvalidExerciseNameError);
}

// ---------- addExercise ----------

TEST_F(ExerciseServiceFixture, AddExerciseGivesNumberOneToFirstExercise)
{
    m_repository.save(makeCourseWithNumbers({}));

    EXPECT_EQ(m_service->addExercise(1, "adder", "short", "full"), 1);

    const Course course = m_repository.load(1);
    ASSERT_EQ(course.exercises.size(), 1u);
    EXPECT_EQ(course.exercises[0].number, 1);
    EXPECT_EQ(course.exercises[0].name, "adder");
    EXPECT_EQ(course.exercises[0].description, "short");
    EXPECT_EQ(course.exercises[0].fullDescription, "full");
}

TEST_F(ExerciseServiceFixture, AddExerciseTakesFirstFreeNumber)
{
    m_repository.save(makeCourseWithNumbers({1, 3}));

    EXPECT_EQ(m_service->addExercise(1, "gap filler", "", ""), 2);
}

TEST_F(ExerciseServiceFixture, AddExerciseThrowsForMissingCourse)
{
    EXPECT_THROW(m_service->addExercise(5, "adder", "", ""), CourseNotFoundError);
}

TEST_F(ExerciseServiceFixture, AddExerciseRejectsInvalidName)
{
    m_repository.save(makeCourseWithNumbers({}));

    EXPECT_THROW(m_service->addExercise(1, "bad/name", "", ""), InvalidExerciseNameError);
    EXPECT_TRUE(m_repository.load(1).exercises.empty());
}

TEST_F(ExerciseServiceFixture, AddExerciseRejectsDuplicateName)
{
    m_repository.save(makeCourseWithNumbers({}));

    m_service->addExercise(1, "adder", "", "");
    EXPECT_THROW(m_service->addExercise(1, "adder", "", ""), DuplicateExerciseError);
    EXPECT_EQ(m_repository.load(1).exercises.size(), 1u);
}

// ---------- updateExercise ----------

TEST_F(ExerciseServiceFixture, UpdateExerciseChangesNameAndDescriptions)
{
    m_repository.save(makeCourseWithNumbers({}));
    m_service->addExercise(1, "adder", "old short", "old full");

    m_service->updateExercise(1, 1, "sum", "new short", "new full");

    const Course course = m_repository.load(1);
    ASSERT_EQ(course.exercises.size(), 1u);
    EXPECT_EQ(course.exercises[0].number, 1);
    EXPECT_EQ(course.exercises[0].name, "sum");
    EXPECT_EQ(course.exercises[0].description, "new short");
    EXPECT_EQ(course.exercises[0].fullDescription, "new full");
}

TEST_F(ExerciseServiceFixture, UpdateExerciseAllowsKeepingOwnName)
{
    m_repository.save(makeCourseWithNumbers({}));
    m_service->addExercise(1, "adder", "old", "old");

    EXPECT_NO_THROW(m_service->updateExercise(1, 1, "adder", "new", "new"));
    EXPECT_EQ(m_repository.load(1).exercises[0].description, "new");
}

TEST_F(ExerciseServiceFixture, UpdateExerciseThrowsForMissingCourse)
{
    EXPECT_THROW(m_service->updateExercise(5, 1, "adder", "", ""), CourseNotFoundError);
}

TEST_F(ExerciseServiceFixture, UpdateExerciseThrowsForMissingExercise)
{
    m_repository.save(makeCourseWithNumbers({}));

    EXPECT_THROW(m_service->updateExercise(1, 7, "adder", "", ""), ExerciseNotFoundError);
}

TEST_F(ExerciseServiceFixture, UpdateExerciseRejectsInvalidName)
{
    m_repository.save(makeCourseWithNumbers({}));
    m_service->addExercise(1, "adder", "", "");

    EXPECT_THROW(m_service->updateExercise(1, 1, "bad/name", "", ""), InvalidExerciseNameError);
    EXPECT_EQ(m_repository.load(1).exercises[0].name, "adder");
}

TEST_F(ExerciseServiceFixture, UpdateExerciseRejectsNameOfAnotherExercise)
{
    m_repository.save(makeCourseWithNumbers({}));
    m_service->addExercise(1, "adder", "", "");
    m_service->addExercise(1, "sum", "", "");

    EXPECT_THROW(m_service->updateExercise(1, 2, "adder", "", ""), DuplicateExerciseError);
    EXPECT_EQ(m_repository.load(1).exercises[1].name, "sum");
}

// ---------- deleteExercise ----------

TEST_F(ExerciseServiceFixture, DeleteExerciseRemovesDataAndFiles)
{
    m_repository.save(makeCourseWithNumbers({}));
    m_service->addExercise(1, "adder", "", "");
    createFiles(1, 1);

    m_service->deleteExercise(1, 1);

    EXPECT_TRUE(m_repository.load(1).exercises.empty());
    EXPECT_FALSE(fs::exists(headerPath(1, 1)));
    EXPECT_FALSE(fs::exists(sourcePath(1, 1)));
}

TEST_F(ExerciseServiceFixture, DeleteExerciseWorksWhenFilesDoNotExist)
{
    m_repository.save(makeCourseWithNumbers({}));
    m_service->addExercise(1, "adder", "", "");

    EXPECT_NO_THROW(m_service->deleteExercise(1, 1));
    EXPECT_TRUE(m_repository.load(1).exercises.empty());
}

TEST_F(ExerciseServiceFixture, DeleteExerciseKeepsOtherExercisesAndFiles)
{
    m_repository.save(makeCourseWithNumbers({}));
    m_service->addExercise(1, "adder", "", "");
    m_service->addExercise(1, "sum", "", "");
    createFiles(1, 1);
    createFiles(1, 2);

    m_service->deleteExercise(1, 1);

    const Course course = m_repository.load(1);
    ASSERT_EQ(course.exercises.size(), 1u);
    EXPECT_EQ(course.exercises[0].number, 2);
    EXPECT_TRUE(fs::exists(headerPath(1, 2)));
    EXPECT_TRUE(fs::exists(sourcePath(1, 2)));
}

TEST_F(ExerciseServiceFixture, DeletedNumberIsReusedByNextExercise)
{
    m_repository.save(makeCourseWithNumbers({}));
    m_service->addExercise(1, "adder", "", "");
    m_service->addExercise(1, "sum", "", "");

    m_service->deleteExercise(1, 1);

    EXPECT_EQ(m_service->addExercise(1, "product", "", ""), 1);
}

TEST_F(ExerciseServiceFixture, DeleteExerciseThrowsForMissingCourse)
{
    EXPECT_THROW(m_service->deleteExercise(5, 1), CourseNotFoundError);
}

TEST_F(ExerciseServiceFixture, DeleteExerciseThrowsForMissingExercise)
{
    m_repository.save(makeCourseWithNumbers({}));

    EXPECT_THROW(m_service->deleteExercise(1, 7), ExerciseNotFoundError);
}

// ---------- addExercise generates the source files ----------

TEST_F(ExerciseServiceFixture, AddExerciseGeneratesSourceFiles)
{
    m_repository.save(makeCourseWithNumbers({}));

    m_service->addExercise(1, "adder", "", "");

    EXPECT_TRUE(fs::is_regular_file(headerPath(1, 1)));
    EXPECT_TRUE(fs::is_regular_file(sourcePath(1, 1)));
}

TEST_F(ExerciseServiceFixture, AddExerciseGeneratesFilesForTheNewNumber)
{
    m_repository.save(makeCourseWithNumbers({1, 3}));

    EXPECT_EQ(m_service->addExercise(1, "gap filler", "", ""), 2);

    EXPECT_TRUE(fs::is_regular_file(headerPath(1, 2)));
    EXPECT_TRUE(fs::is_regular_file(sourcePath(1, 2)));
}

TEST_F(ExerciseServiceFixture, AddExerciseKeepsExistingFileAndDataWhenFileExists)
{
    m_repository.save(makeCourseWithNumbers({}));
    createFiles(1, 1);

    EXPECT_THROW(m_service->addExercise(1, "adder", "", ""), WorkspaceError);

    EXPECT_TRUE(m_repository.load(1).exercises.empty());
    std::ifstream in(headerPath(1, 1));
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content, "// header");
}

TEST_F(ExerciseServiceFixture, AddExerciseGeneratesNoFilesForInvalidName)
{
    m_repository.save(makeCourseWithNumbers({}));

    EXPECT_THROW(m_service->addExercise(1, "bad/name", "", ""), InvalidExerciseNameError);

    EXPECT_FALSE(fs::exists(headerPath(1, 1)));
    EXPECT_FALSE(fs::exists(sourcePath(1, 1)));
}

TEST_F(ExerciseServiceFixture, AddExerciseGeneratesNoFilesForDuplicateName)
{
    m_repository.save(makeCourseWithNumbers({}));
    m_service->addExercise(1, "adder", "", "");

    EXPECT_THROW(m_service->addExercise(1, "adder", "", ""), DuplicateExerciseError);

    EXPECT_FALSE(fs::exists(headerPath(1, 2)));
    EXPECT_FALSE(fs::exists(sourcePath(1, 2)));
}

TEST_F(ExerciseServiceFixture, AddExerciseRemovesNewFilesWhenSavingFails)
{
    m_repository.save(makeCourseWithNumbers({}));
    m_repository.setFailOnSave(true);

    EXPECT_THROW(m_service->addExercise(1, "adder", "", ""), StorageError);

    EXPECT_FALSE(fs::exists(headerPath(1, 1)));
    EXPECT_FALSE(fs::exists(sourcePath(1, 1)));
    m_repository.setFailOnSave(false);
    EXPECT_TRUE(m_repository.load(1).exercises.empty());
}

TEST_F(ExerciseServiceFixture, DeleteExerciseClearsItsDefaultExercise)
{
    m_repository.save(makeCourseWithNumbers({}));
    m_service->addExercise(1, "adder", "", "");
    DefaultsStore(*m_workspace).saveDefaultExercise(1, 1);

    m_service->deleteExercise(1, 1);

    EXPECT_FALSE(DefaultsStore(*m_workspace).loadDefaultExercise(1).has_value());
}

TEST_F(ExerciseServiceFixture, DeleteExerciseKeepsDefaultOfAnotherExercise)
{
    m_repository.save(makeCourseWithNumbers({}));
    m_service->addExercise(1, "adder", "", "");
    m_service->addExercise(1, "sum", "", "");
    DefaultsStore(*m_workspace).saveDefaultExercise(1, 2);

    m_service->deleteExercise(1, 1);

    EXPECT_EQ(DefaultsStore(*m_workspace).loadDefaultExercise(1), 2);
}

TEST_F(ExerciseServiceFixture, DeletedExerciseNumberDoesNotInheritTheDefault)
{
    m_repository.save(makeCourseWithNumbers({}));
    m_service->addExercise(1, "adder", "", "");
    m_service->addExercise(1, "sum", "", "");
    DefaultsStore(*m_workspace).saveDefaultExercise(1, 1);
    m_service->deleteExercise(1, 1);

    EXPECT_EQ(m_service->addExercise(1, "product", "", ""), 1);

    EXPECT_FALSE(DefaultsStore(*m_workspace).loadDefaultExercise(1).has_value());
}

TEST_F(ExerciseServiceFixture, DeleteExerciseWorksWhenDefaultFileIsCorrupt)
{
    m_repository.save(makeCourseWithNumbers({}));
    m_service->addExercise(1, "adder", "", "");
    std::ofstream(m_workspace->defaultExerciseFile(1)) << "this is not json";

    EXPECT_NO_THROW(m_service->deleteExercise(1, 1));
    EXPECT_TRUE(m_repository.load(1).exercises.empty());
}
