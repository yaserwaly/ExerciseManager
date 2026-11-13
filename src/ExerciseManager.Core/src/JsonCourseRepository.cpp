#include "exercise_manager/JsonCourseRepository.h"

#include "exercise_manager/Errors.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>

namespace exercise_manager
{

namespace fs = std::filesystem;
using nlohmann::json;

namespace
{

// A course as listed in Courses.json (without its exercises).
struct CourseEntry
{
    int id = 0;
    std::string name;
    std::string description;
    std::string fullDescription;
};

// Reads "key" as an int, or throws StorageError with a clear message.
int requireInt(const json& object, const char* key)
{
    if (!object.is_object() || !object.contains(key) || !object[key].is_number_integer())
    {
        throw StorageError(std::string("Data file is invalid: missing or bad field '") + key +
                           "'.");
    }
    return object[key].get<int>();
}

std::string requireString(const json& object, const char* key)
{
    if (!object.is_object() || !object.contains(key) || !object[key].is_string())
    {
        throw StorageError(std::string("Data file is invalid: missing or bad field '") + key +
                           "'.");
    }
    return object[key].get<std::string>();
}

void requireSchemaVersion(const json& root)
{
    if (requireInt(root, "schema_version") != JsonCourseRepository::kSchemaVersion)
    {
        throw StorageError("Data file has an unsupported schema version.");
    }
}

json readJsonFile(const fs::path& file)
{
    std::ifstream in(file, std::ios::binary);
    if (!in)
    {
        throw StorageError("Could not open a data file.");
    }

    try
    {
        return json::parse(in);
    }
    catch (const json::parse_error&)
    {
        throw StorageError("Data file is not valid JSON.");
    }
}

// Writes to "<file>.tmp" first, then replaces the real file in one step.
void writeJsonFile(const fs::path& file, const json& data)
{
    std::error_code ec;
    fs::create_directories(file.parent_path(), ec);
    if (ec)
    {
        throw StorageError("Could not create the data folder.");
    }

    fs::path temp = file;
    temp += ".tmp";

    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            throw StorageError("Could not create the temporary data file.");
        }
        out << data.dump(2) << '\n';
        out.flush();
        if (!out)
        {
            fs::remove(temp, ec);
            throw StorageError("Could not write the data file.");
        }
    }

    // On Windows, rename() replaces an existing file in one step.
    fs::rename(temp, file, ec);
    if (ec)
    {
        std::error_code ignored;
        fs::remove(temp, ignored);
        throw StorageError("Could not replace the data file.");
    }
}

// Reads Courses.json. A missing file means "no courses yet".
std::vector<CourseEntry> readCourseEntries(const Workspace& workspace)
{
    const fs::path file = workspace.coursesFile();

    std::error_code ec;
    if (!fs::is_regular_file(file, ec))
    {
        return {};
    }

    const json root = readJsonFile(file);

    try
    {
        requireSchemaVersion(root);

        if (!root.contains("courses") || !root["courses"].is_array())
        {
            throw StorageError("Courses file is invalid: missing 'courses' array.");
        }

        std::vector<CourseEntry> entries;
        for (const json& item : root["courses"])
        {
            CourseEntry entry;
            entry.id = requireInt(item, "id");
            entry.name = requireString(item, "name");
            entry.description = requireString(item, "description");
            entry.fullDescription = requireString(item, "full_description");

            const bool duplicate =
                std::any_of(entries.begin(), entries.end(),
                            [&entry](const CourseEntry& other) { return other.id == entry.id; });
            if (duplicate)
            {
                throw StorageError("Courses file is invalid: duplicate course number.");
            }

            entries.push_back(std::move(entry));
        }
        return entries;
    }
    catch (const json::exception&)
    {
        throw StorageError("Courses file has unexpected data.");
    }
}

json coursesToJson(const std::vector<CourseEntry>& entries)
{
    json courses = json::array();
    for (const CourseEntry& entry : entries)
    {
        courses.push_back({{"id", entry.id},
                           {"name", entry.name},
                           {"description", entry.description},
                           {"full_description", entry.fullDescription}});
    }

    return json{{"schema_version", JsonCourseRepository::kSchemaVersion}, {"courses", courses}};
}

// Reads Exercises.json of one course. A listed course must have this file.
std::vector<Exercise> readExercises(const Workspace& workspace, int courseId)
{
    const fs::path file = workspace.exercisesFile(courseId);

    std::error_code ec;
    if (!fs::is_regular_file(file, ec))
    {
        throw StorageError("Exercises file is missing.");
    }

    const json root = readJsonFile(file);

    try
    {
        requireSchemaVersion(root);

        if (requireInt(root, "course_id") != courseId)
        {
            throw StorageError("Exercises file does not match its folder.");
        }

        if (!root.contains("exercises") || !root["exercises"].is_array())
        {
            throw StorageError("Exercises file is invalid: missing 'exercises' array.");
        }

        std::vector<Exercise> exercises;
        for (const json& item : root["exercises"])
        {
            Exercise exercise;
            exercise.number = requireInt(item, "number");
            exercise.name = requireString(item, "name");
            exercise.description = requireString(item, "description");
            exercise.fullDescription = requireString(item, "full_description");
            exercises.push_back(std::move(exercise));
        }
        return exercises;
    }
    catch (const json::exception&)
    {
        throw StorageError("Exercises file has unexpected data.");
    }
}

json exercisesToJson(const Course& course)
{
    json exercises = json::array();
    for (const Exercise& exercise : course.exercises)
    {
        exercises.push_back({{"number", exercise.number},
                             {"name", exercise.name},
                             {"description", exercise.description},
                             {"full_description", exercise.fullDescription}});
    }

    return json{{"schema_version", JsonCourseRepository::kSchemaVersion},
                {"course_id", course.id},
                {"exercises", exercises}};
}

} // namespace

JsonCourseRepository::JsonCourseRepository(const Workspace& workspace) : m_workspace(workspace)
{
}

bool JsonCourseRepository::exists(int courseId) const
{
    const std::vector<CourseEntry> entries = readCourseEntries(m_workspace);
    return std::any_of(entries.begin(), entries.end(),
                       [courseId](const CourseEntry& entry) { return entry.id == courseId; });
}

Course JsonCourseRepository::load(int courseId) const
{
    const std::vector<CourseEntry> entries = readCourseEntries(m_workspace);
    const auto it =
        std::find_if(entries.begin(), entries.end(),
                     [courseId](const CourseEntry& entry) { return entry.id == courseId; });
    if (it == entries.end())
    {
        throw CourseNotFoundError("Course does not exist.");
    }

    Course course;
    course.id = it->id;
    course.name = it->name;
    course.description = it->description;
    course.fullDescription = it->fullDescription;
    course.exercises = readExercises(m_workspace, courseId);
    return course;
}

void JsonCourseRepository::save(const Course& course)
{
    // Throws InvalidExerciseNameError for a bad course number, before anything is written.
    const fs::path exercisesFile = m_workspace.exercisesFile(course.id);

    std::vector<CourseEntry> entries = readCourseEntries(m_workspace);

    const CourseEntry entry{course.id, course.name, course.description, course.fullDescription};
    const auto it = std::find_if(entries.begin(), entries.end(), [&course](const CourseEntry& other)
                                 { return other.id == course.id; });
    if (it != entries.end())
    {
        *it = entry;
    }
    else
    {
        entries.push_back(entry);
    }

    std::sort(entries.begin(), entries.end(),
              [](const CourseEntry& a, const CourseEntry& b) { return a.id < b.id; });

    // Exercises first: if something fails in between, the course is just not listed yet.
    writeJsonFile(exercisesFile, exercisesToJson(course));
    writeJsonFile(m_workspace.coursesFile(), coursesToJson(entries));
}

void JsonCourseRepository::remove(int courseId)
{
    std::vector<CourseEntry> entries = readCourseEntries(m_workspace);

    const auto it =
        std::find_if(entries.begin(), entries.end(),
                     [courseId](const CourseEntry& entry) { return entry.id == courseId; });
    if (it == entries.end())
    {
        throw CourseNotFoundError("Course does not exist.");
    }

    entries.erase(it);
    writeJsonFile(m_workspace.coursesFile(), coursesToJson(entries));
}

std::vector<int> JsonCourseRepository::listCourseIds() const
{
    const std::vector<CourseEntry> entries = readCourseEntries(m_workspace);

    std::vector<int> ids;
    ids.reserve(entries.size());
    for (const CourseEntry& entry : entries)
    {
        ids.push_back(entry.id);
    }

    std::sort(ids.begin(), ids.end());
    return ids;
}

} // namespace exercise_manager
