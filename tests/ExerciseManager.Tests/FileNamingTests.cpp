#include "exercise_manager/FileNaming.h"

#include "exercise_manager/Errors.h"

#include <gtest/gtest.h>

using exercise_manager::FileNaming;
using exercise_manager::InvalidExerciseNameError;

TEST(FileNamingTests, BuildsBaseNameWithPaddedNumbers)
{
    EXPECT_EQ(FileNaming::baseName(1, 1), "c01Problem001");
    EXPECT_EQ(FileNaming::baseName(2, 13), "c02Problem013");
    EXPECT_EQ(FileNaming::baseName(10, 100), "c10Problem100");
}

TEST(FileNamingTests, AcceptsLimits)
{
    EXPECT_EQ(FileNaming::baseName(FileNaming::kMaxCourseId, FileNaming::kMaxExerciseNumber),
              "c99Problem999");
}

TEST(FileNamingTests, BuildsFileNames)
{
    EXPECT_EQ(FileNaming::headerFileName(2, 13), "c02Problem013.hpp");
    EXPECT_EQ(FileNaming::sourceFileName(2, 13), "c02Problem013.cpp");
}

TEST(FileNamingTests, BuildsCourseFolderName)
{
    EXPECT_EQ(FileNaming::courseFolderName(1), "c01");
    EXPECT_EQ(FileNaming::courseFolderName(10), "c10");
}

TEST(FileNamingTests, BuildsClassName)
{
    EXPECT_EQ(FileNaming::className(1), "clsProblem001");
    EXPECT_EQ(FileNaming::className(100), "clsProblem100");
}

TEST(FileNamingTests, RejectsCourseOutOfRange)
{
    EXPECT_THROW(FileNaming::courseFolderName(0), InvalidExerciseNameError);
    EXPECT_THROW(FileNaming::courseFolderName(-1), InvalidExerciseNameError);
    EXPECT_THROW(FileNaming::baseName(FileNaming::kMaxCourseId + 1, 1), InvalidExerciseNameError);
}

TEST(FileNamingTests, RejectsExerciseOutOfRange)
{
    EXPECT_THROW(FileNaming::baseName(1, 0), InvalidExerciseNameError);
    EXPECT_THROW(FileNaming::baseName(1, -5), InvalidExerciseNameError);
    EXPECT_THROW(FileNaming::baseName(1, FileNaming::kMaxExerciseNumber + 1),
                 InvalidExerciseNameError);
    EXPECT_THROW(FileNaming::className(0), InvalidExerciseNameError);
}

TEST(FileNamingTests, AlphabeticalOrderMatchesNumericOrder)
{
    EXPECT_LT(FileNaming::baseName(1, 9), FileNaming::baseName(1, 10));
    EXPECT_LT(FileNaming::baseName(1, 99), FileNaming::baseName(1, 100));
    EXPECT_LT(FileNaming::baseName(2, 999), FileNaming::baseName(10, 1));
}