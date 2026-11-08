#include "exercise_manager/TemplateEngine.h"

#include "exercise_manager/Errors.h"

#include <cstddef>

namespace exercise_manager
{

std::string TemplateEngine::render(std::string_view templateText, const Values& values)
{
    constexpr std::string_view kOpen = "{{";
    constexpr std::string_view kClose = "}}";

    std::string result;
    result.reserve(templateText.size());

    std::size_t position = 0;
    while (position < templateText.size())
    {
        const std::size_t open = templateText.find(kOpen, position);
        if (open == std::string_view::npos)
        {
            result.append(templateText.substr(position));
            break;
        }

        result.append(templateText.substr(position, open - position));

        const std::size_t keyStart = open + kOpen.size();
        const std::size_t close = templateText.find(kClose, keyStart);
        if (close == std::string_view::npos)
        {
            throw StorageError("Template is invalid: a placeholder is not closed.");
        }

        const std::string key(templateText.substr(keyStart, close - keyStart));
        const auto value = values.find(key);
        if (value == values.end())
        {
            throw StorageError("Template is invalid: no value for placeholder '" + key + "'.");
        }

        result += value->second;
        position = close + kClose.size();
    }

    return result;
}

} // namespace exercise_manager
