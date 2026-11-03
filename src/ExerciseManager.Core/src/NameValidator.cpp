#include "exercise_manager/NameValidator.h"

#include "exercise_manager/Errors.h"

#include <cctype>
#include <string>
#include <string_view>

namespace exercise_manager
{

namespace
{

bool isControlChar(char c)
{
    const auto code = static_cast<unsigned char>(c);
    return code < 0x20 || code == 0x7F;
}

bool isForbiddenChar(char c)
{
    constexpr std::string_view kForbidden = "|<>\"?*:/\\";
    return kForbidden.find(c) != std::string_view::npos;
}

std::string toUpperAscii(std::string_view text)
{
    std::string result(text);
    for (char& c : result)
    {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return result;
}

bool isReservedDeviceName(std::string_view name)
{
    // Windows also reserves names like "CON.txt", so only the part before
    // the first dot is compared.
    const std::string stem = toUpperAscii(name.substr(0, name.find('.')));

    if (stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL")
    {
        return true;
    }

    if (stem.size() == 4 && (stem.compare(0, 3, "COM") == 0 || stem.compare(0, 3, "LPT") == 0))
    {
        return stem[3] >= '1' && stem[3] <= '9';
    }

    return false;
}

} // namespace

void NameValidator::validate(std::string_view name)
{
    if (name.empty())
    {
        throw InvalidExerciseNameError("Exercise name must not be empty.");
    }

    if (name.size() > kMaxNameLength)
    {
        throw InvalidExerciseNameError("Exercise name is too long.");
    }

    for (const char c : name)
    {
        if (isControlChar(c))
        {
            throw InvalidExerciseNameError("Exercise name contains a control character.");
        }
        if (isForbiddenChar(c))
        {
            throw InvalidExerciseNameError("Exercise name contains a forbidden character.");
        }
    }

    if (name.find("..") != std::string_view::npos)
    {
        throw InvalidExerciseNameError("Exercise name must not contain '..'.");
    }

    if (name.back() == '.' || name.back() == ' ')
    {
        throw InvalidExerciseNameError("Exercise name must not end with a dot or a space.");
    }

    if (isReservedDeviceName(name))
    {
        throw InvalidExerciseNameError("Exercise name is a reserved Windows device name.");
    }
}

} // namespace exercise_manager