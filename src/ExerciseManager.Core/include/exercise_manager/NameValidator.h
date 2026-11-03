#pragma once

#include <cstddef>
#include <string_view>

namespace exercise_manager
{

/// @brief Validates exercise names before they are stored.
///
/// A name is rejected if it is empty, too long, contains control characters,
/// contains any of `| < > " ? * : / \` or the sequence `..`, is a reserved
/// Windows device name (CON, PRN, AUX, NUL, COM1..COM9, LPT1..LPT9), or ends
/// with a dot or a space.
class NameValidator
{
public:
    /// @brief Maximum allowed length of an exercise name, in characters.
    static constexpr std::size_t kMaxNameLength = 64;

    /// @brief Checks that a name is allowed.
    /// @param name The exercise name to check.
    /// @throws InvalidExerciseNameError if the name is not allowed.
    static void validate(std::string_view name);
};

} // namespace exercise_manager