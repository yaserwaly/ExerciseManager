#include "exercise_manager/ExerciseService.h"

#include "exercise_manager/Errors.h"
#include "exercise_manager/FileNaming.h"
#include "exercise_manager/ICourseRepository.h"

#include <algorithm>
#include <map>

#include <gtest/gtest.h>

#include <vector>

using exercise_manager::Course;
using exercise_manager::Exercise;
using exercise_manager::ExerciseService;
using exercise_manager::FileNaming;
using exercise_manager::InvalidExerciseNameError;
using exercise_manager::CourseNotFoundError;
using exercise_manager::DuplicateExerciseError;
using exercise_manager::ICourseRepository;
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
        m_courses[course.id] = course;
    }

    bool exists(int courseId) const override
    {
        return m_courses.count(courseId) > 0;
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
};

} // namespace

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

TEST(ExerciseServiceTests, AddExerciseGivesNumberOneToFirstExercise)
{
    FakeCourseRepository repository;
    repository.save(makeCourseWithNumbers({}));
    ExerciseService service(repository);

    EXPECT_EQ(service.addExercise(1, "adder", "short", "full"), 1);

    const Course course = repository.load(1);
    ASSERT_EQ(course.exercises.size(), 1u);
    EXPECT_EQ(course.exercises[0].number, 1);
    EXPECT_EQ(course.exercises[0].name, "adder");
    EXPECT_EQ(course.exercises[0].description, "short");
    EXPECT_EQ(course.exercises[0].fullDescription, "full");
}

TEST(ExerciseServiceTests, AddExerciseTakesFirstFreeNumber)
{
    FakeCourseRepository repository;
    repository.save(makeCourseWithNumbers({1, 3}));
    ExerciseService service(repository);

    EXPECT_EQ(service.addExercise(1, "gap filler", "", ""), 2);
}

TEST(ExerciseServiceTests, AddExerciseThrowsForMissingCourse)
{
    FakeCourseRepository repository;
    ExerciseService service(repository);

    EXPECT_THROW(service.addExercise(5, "adder", "", ""), CourseNotFoundError);
}

TEST(ExerciseServiceTests, AddExerciseRejectsInvalidName)
{
    FakeCourseRepository repository;
    repository.save(makeCourseWithNumbers({}));
    ExerciseService service(repository);

    EXPECT_THROW(service.addExercise(1, "bad/name", "", ""), InvalidExerciseNameError);
    EXPECT_TRUE(repository.load(1).exercises.empty());
}

TEST(ExerciseServiceTests, AddExerciseRejectsDuplicateName)
{
    FakeCourseRepository repository;
    repository.save(makeCourseWithNumbers({}));
    ExerciseService service(repository);

    service.addExercise(1, "adder", "", "");
    EXPECT_THROW(service.addExercise(1, "adder", "", ""), DuplicateExerciseError);
    EXPECT_EQ(repository.load(1).exercises.size(), 1u);
}
