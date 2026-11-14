#include "exercise_manager/DefaultsStore.h"

#include "exercise_manager/Errors.h"
#include "exercise_manager/FileNaming.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

namespace exercise_manager
{

namespace fs = std::filesystem;
using nlohmann::json;

namespace
{

void checkCourseId(int courseId)
{
    if (courseId < 1 || courseId > FileNaming::kMaxCourseId)
    {
        throw InvalidExerciseNameError("Course number is out of range.");
    }
}

void checkExerciseNumber(int exerciseNumber)
{
    if (exerciseNumber < 1 || exerciseNumber > FileNaming::kMaxExerciseNumber)
    {
        throw InvalidExerciseNameError("Exercise number is out of range.");
    }
}

// Reads one number from a defaults file. A missing file means "no default".
std::optional<int> readNumber(const fs::path& file, const char* key, int minValue, int maxValue)
{
    std::error_code ec;
    if (!fs::is_regular_file(file, ec))
    {
        return std::nullopt;
    }

    std::ifstream in(file, std::ios::binary);
    if (!in)
    {
        throw StorageError("Could not open a defaults file.");
    }

    try
    {
        const json root = json::parse(in);

        if (!root.is_object() || !root.contains("schema_version") ||
            !root["schema_version"].is_number_integer() ||
            root["schema_version"].get<long long>() != DefaultsStore::kSchemaVersion)
        {
            throw StorageError("Defaults file has an unsupported schema version.");
        }

        if (!root.contains(key) || !root[key].is_number_integer())
        {
            throw StorageError(std::string("Defaults file is invalid: missing or bad field '") +
                               key + "'.");
        }

        const long long value = root[key].get<long long>();
        if (value < minValue || value > maxValue)
        {
            throw StorageError("Defaults file is invalid: the number is out of range.");
        }
        return static_cast<int>(value);
    }
    catch (const json::exception&)
    {
        throw StorageError("Defaults file is not valid JSON.");
    }
}

// Writes to "<file>.tmp" first, then replaces the real file in one step.
void writeNumber(const fs::path& file, const char* key, int value)
{
    const json data{{"schema_version", DefaultsStore::kSchemaVersion}, {key, value}};

    fs::path temp = file;
    temp += ".tmp";

    std::error_code ec;
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            throw StorageError("Could not create the temporary defaults file.");
        }
        out << data.dump(2) << '\n';
        out.flush();
        if (!out)
        {
            fs::remove(temp, ec);
            throw StorageError("Could not write the defaults file.");
        }
    }

    // On Windows, rename() replaces an existing file in one step.
    fs::rename(temp, file, ec);
    if (ec)
    {
        std::error_code ignored;
        fs::remove(temp, ignored);
        throw StorageError("Could not replace the defaults file.");
    }
}

void removeFile(const fs::path& file)
{
    std::error_code ec;
    fs::remove(file, ec); // a missing file is not an error
    if (ec)
    {
        throw StorageError("Could not remove a defaults file.");
    }
}

} // namespace

DefaultsStore::DefaultsStore(const Workspace& workspace) : m_workspace(workspace)
{
}

std::optional<int> DefaultsStore::loadDefaultCourse() const
{
    const fs::path file = m_workspace.guard().ensureInside(m_workspace.defaultCourseFile());
    return readNumber(file, "course_id", 1, FileNaming::kMaxCourseId);
}

void DefaultsStore::saveDefaultCourse(int courseId)
{
    checkCourseId(courseId);
    const fs::path file = m_workspace.guard().ensureInside(m_workspace.defaultCourseFile());
    writeNumber(file, "course_id", courseId);
}

void DefaultsStore::clearDefaultCourse()
{
    removeFile(m_workspace.guard().ensureInside(m_workspace.defaultCourseFile()));
}

std::optional<int> DefaultsStore::loadDefaultExercise(int courseId) const
{
    const fs::path file =
        m_workspace.guard().ensureInside(m_workspace.defaultExerciseFile(courseId));
    return readNumber(file, "exercise_number", 1, FileNaming::kMaxExerciseNumber);
}

void DefaultsStore::saveDefaultExercise(int courseId, int exerciseNumber)
{
    checkExerciseNumber(exerciseNumber);
    const fs::path file =
        m_workspace.guard().ensureInside(m_workspace.defaultExerciseFile(courseId));

    // Do not create a stray course folder just to hold a default.
    std::error_code ec;
    if (!fs::is_directory(m_workspace.courseDir(courseId), ec))
    {
        throw WorkspaceError("The course folder does not exist.");
    }

    writeNumber(file, "exercise_number", exerciseNumber);
}

void DefaultsStore::clearDefaultExercise(int courseId)
{
    removeFile(m_workspace.guard().ensureInside(m_workspace.defaultExerciseFile(courseId)));
}

} // namespace exercise_manager
