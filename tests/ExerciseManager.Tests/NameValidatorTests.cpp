#include "exercise_manager/NameValidator.h"

#include "exercise_manager/Errors.h"

#include <gtest/gtest.h>

#include <string>

using exercise_manager::InvalidExerciseNameError;
using exercise_manager::NameValidator;

TEST(NameValidatorTests, AcceptsSimpleName)
{
    EXPECT_NO_THROW(NameValidator::validate("adder"));
}

TEST(NameValidatorTests, AcceptsNameWithSpacesAndDigits)
{
    EXPECT_NO_THROW(NameValidator::validate("add two numbers 2"));
}

TEST(NameValidatorTests, AcceptsArabicName)
{
    EXPECT_NO_THROW(NameValidator::validate("جمع رقمين"));
}

TEST(NameValidatorTests, RejectsEmptyName)
{
    EXPECT_THROW(NameValidator::validate(""), InvalidExerciseNameError);
}

TEST(NameValidatorTests, AcceptsNameAtMaxLength)
{
    const std::string name(NameValidator::kMaxNameLength, 'a');
    EXPECT_NO_THROW(NameValidator::validate(name));
}

TEST(NameValidatorTests, RejectsNameLongerThanMax)
{
    const std::string name(NameValidator::kMaxNameLength + 1, 'a');
    EXPECT_THROW(NameValidator::validate(name), InvalidExerciseNameError);
}

TEST(NameValidatorTests, RejectsControlCharacters)
{
    EXPECT_THROW(NameValidator::validate("bad\nname"), InvalidExerciseNameError);
    EXPECT_THROW(NameValidator::validate(std::string("bad\0name", 8)), InvalidExerciseNameError);
}

TEST(NameValidatorTests, RejectsForbiddenCharacters)
{
    const char* names[] = {"a|b", "a<b", "a>b", "a\"b", "a?b", "a*b", "a:b", "a/b", "a\\b"};
    for (const char* name : names)
    {
        EXPECT_THROW(NameValidator::validate(name), InvalidExerciseNameError) << name;
    }
}

TEST(NameValidatorTests, RejectsParentDirectorySequence)
{
    EXPECT_THROW(NameValidator::validate("a..b"), InvalidExerciseNameError);
    EXPECT_THROW(NameValidator::validate(".."), InvalidExerciseNameError);
}

TEST(NameValidatorTests, RejectsTrailingDotOrSpace)
{
    EXPECT_THROW(NameValidator::validate("name."), InvalidExerciseNameError);
    EXPECT_THROW(NameValidator::validate("name "), InvalidExerciseNameError);
}

TEST(NameValidatorTests, RejectsReservedWindowsNames)
{
    const char* names[] = {"CON", "con", "PRN", "AUX", "NUL", "COM1", "com9", "LPT1", "lpt9", "CON.txt"};
    for (const char* name : names)
    {
        EXPECT_THROW(NameValidator::validate(name), InvalidExerciseNameError) << name;
    }
}

TEST(NameValidatorTests, AcceptsNamesThatOnlyLookReserved)
{
    EXPECT_NO_THROW(NameValidator::validate("COM0"));
    EXPECT_NO_THROW(NameValidator::validate("COM10"));
    EXPECT_NO_THROW(NameValidator::validate("console"));
}