#pragma once

#include <map>
#include <string>
#include <string_view>

namespace exercise_manager
{

/// @brief Replaces `{{Key}}` placeholders in a text with values.
///
/// Used to generate the source files of an exercise from a template. The text is
/// scanned once from left to right, so a value that itself contains `{{Key}}` is
/// copied as it is and never expanded again. Single braces are left untouched,
/// so normal C++ code can be used inside a template.
class TemplateEngine
{
  public:
    /// @brief Placeholder names mapped to their values, e.g. `{"Namespace", "C01Exercise001"}`.
    using Values = std::map<std::string, std::string>;

    /// @brief Fills a template with values.
    /// @param templateText The template text, with `{{Key}}` placeholders.
    /// @param values The value of every placeholder used in the template.
    /// @return The template with every placeholder replaced.
    /// @throws StorageError if a placeholder has no value or is not closed.
    static std::string render(std::string_view templateText, const Values& values);
};

} // namespace exercise_manager
