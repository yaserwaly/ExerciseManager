#include "exercise_manager/JsonCourseRepository.h"

#include "exercise_manager/Errors.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
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

// Reads "key" as an int, or throws StorageError with a clear message.
int requireInt(const json& object, const char* key)
{
    if (!object.is_object() || !object.contains(key) || !object[key].is_number_integer())
    {
        throw StorageError(std::string("Course file is invalid: missing or bad field '") + key +
                           "'.");
    }
    return object[key].get<int>();
}

std::string requireString(const json& object, const char* key)
{
    if (!object.is_object() || !object.contains(key) || !object[key].is_string())
    {
        throw StorageError(std::string("Course file is invalid: missing or bad field '") + key +
                           "'.");
    }
    return object[key].get<std::string>();
}

Course fromJson(const json& root, int expectedId)
{
    const int version = requireInt(root, "schema_version");
    if (version != JsonCourseRepository::kSchemaVersion)
    {
        throw StorageError("Course file has an unsupported schema version.");
    }

    if (!root.contains("course") || !root["course"].is_object())
    {
        throw StorageError("Course file is invalid: missing 'course' object.");
    }

    Course course;
    course.id = requireInt(root["course"], "id");
    course.name = requireString(root["course"], "name");
    course.nextExerciseNumber = requireInt(root, "next_exercise_number");

    if (course.id != expectedId)
    {
        throw StorageError("Course file does not match its folder.");
    }

    if (!root.contains("exercises") || !root["exercises"].is_array())
    {
        throw StorageError("Course file is invalid: missing 'exercises' array.");
    }

    for (const json& item : root["exercises"])
    {
        Exercise exercise;
        exercise.number = requireInt(item, "number");
        exercise.name = requireString(item, "name");
        exercise.description = requireString(item, "description");
        course.exercises.push_back(std::move(exercise));
    }

    return course;
}

json toJson(const Course& course)
{
    json exercises = json::array();
    for (const Exercise& exercise : course.exercises)
    {
        exercises.push_back({{"number", exercise.number},
                             {"name", exercise.name},
                             {"description", exercise.description}});
    }

    return json{{"schema_version", JsonCourseRepository::kSchemaVersion},
                {"course", {{"id", course.id}, {"name", course.name}}},
                {"next_exercise_number", course.nextExerciseNumber},
                {"exercises", exercises}};
}

} // namespace

JsonCourseRepository::JsonCourseRepository(const Workspace& workspace) : m_workspace(workspace)
{
}

bool JsonCourseRepository::exists(int courseId) const
{
    std::error_code ec;
    return fs::is_regular_file(m_workspace.courseFile(courseId), ec);
}

Course JsonCourseRepository::load(int courseId) const
{
    const fs::path file = m_workspace.courseFile(courseId);

    if (!exists(courseId))
    {
        throw CourseNotFoundError("Course does not exist.");
    }

    std::ifstream in(file, std::ios::binary);
    if (!in)
    {
        throw StorageError("Could not open the course file.");
    }

    json root;
    try
    {
        root = json::parse(in);
    }
    catch (const json::parse_error&)
    {
        throw StorageError("Course file is not valid JSON.");
    }

    try
    {
        return fromJson(root, courseId);
    }
    catch (const json::exception&)
    {
        throw StorageError("Course file has unexpected data.");
    }
}

void JsonCourseRepository::save(const Course& course)
{
    const fs::path file = m_workspace.courseFile(course.id);
    const fs::path folder = file.parent_path();
    fs::path temp = file;
    temp += ".tmp";

    std::error_code ec;
    fs::create_directories(folder, ec);
    if (ec)
    {
        throw StorageError("Could not create the course folder.");
    }

    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            throw StorageError("Could not create the temporary course file.");
        }
        out << toJson(course).dump(2) << '\n';
        out.flush();
        if (!out)
        {
            fs::remove(temp, ec);
            throw StorageError("Could not write the course file.");
        }
    }

    // On Windows, rename() replaces an existing file in one step.
    fs::rename(temp, file, ec);
    if (ec)
    {
        std::error_code ignored;
        fs::remove(temp, ignored);
        throw StorageError("Could not replace the course file.");
    }
}

std::vector<int> JsonCourseRepository::listCourseIds() const
{
    std::vector<int> ids;

    std::error_code ec;
    if (!fs::is_directory(m_workspace.coursesDir(), ec))
    {
        return ids;
    }

    for (const fs::directory_entry& entry : fs::directory_iterator(m_workspace.coursesDir(), ec))
    {
        const std::string name = entry.path().filename().string();

        // Folder names look like "c01": a 'c' followed by digits only.
        if (name.size() < 2 || name[0] != 'c' ||
            !std::all_of(name.begin() + 1, name.end(),
                         [](unsigned char ch) { return std::isdigit(ch) != 0; }))
        {
            continue;
        }

        const int id = std::stoi(name.substr(1));
        if (exists(id))
        {
            ids.push_back(id);
        }
    }

    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}

} // namespace exercise_manager
