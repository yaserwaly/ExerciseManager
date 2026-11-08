#include "exercise_manager/ExerciseService.h"

#include "exercise_manager/Errors.h"
#include "exercise_manager/FileNaming.h"

#include <gtest/gtest.h>

#include <vector>

using exercise_manager::Course;
using exercise_manager::Exercise;
using exercise_manager::ExerciseService;
using exercise_manager::FileNaming;
using exercise_manager::InvalidExerciseNameError;

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
