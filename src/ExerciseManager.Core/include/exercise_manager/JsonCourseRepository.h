#pragma once

#include "exercise_manager/ICourseRepository.h"
#include "exercise_manager/Workspace.h"

#include <vector>

namespace exercise_manager
{

/// @brief Stores courses as JSON files inside the workspace.
///
/// `Courses/Courses.json` lists all courses (schema version 1):
/// @code
/// {
///   "schema_version": 1,
///   "courses": [ { "id": 1, "name": "C++ Basics", "description": "...", "full_description": "..." } ]
/// }
/// @endcode
///
/// `Courses/Course01/Exercises.json` lists the exercises of one course:
/// @code
/// {
///   "schema_version": 1,
///   "course_id": 1,
///   "exercises": [
///     { "number": 1, "name": "adder", "description": "...", "full_description": "..." }
///   ]
/// }
/// @endcode
///
/// Writing is safe: data goes to a temporary file first, which then replaces the
/// real file. The exercises file is written before Courses.json, so a crash in
/// the middle leaves the course unlisted instead of corrupting anything.

class JsonCourseRepository : public ICourseRepository
{
public:
    /// @brief Current version of the JSON format.
    static constexpr int kSchemaVersion = 1;

    /// @brief Creates a repository that works inside a workspace.
    /// @param workspace The workspace to read from and write to (copied).
    explicit JsonCourseRepository(const Workspace& workspace);

    /// @copydoc ICourseRepository::load
    Course load(int courseId) const override;

    /// @copydoc ICourseRepository::save
    void save(const Course& course) override;

    /// @copydoc ICourseRepository::exists
    bool exists(int courseId) const override;

    /// @copydoc ICourseRepository::listCourseIds
    std::vector<int> listCourseIds() const override;

private:
    Workspace m_workspace;
};

} // namespace exercise_manager
