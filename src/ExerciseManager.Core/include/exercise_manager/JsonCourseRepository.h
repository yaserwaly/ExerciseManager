#pragma once

#include "exercise_manager/ICourseRepository.h"
#include "exercise_manager/Workspace.h"

#include <vector>

namespace exercise_manager
{

/// @brief Stores each course as a JSON file (`course.json`) inside the workspace.
///
/// File format (version 1):
/// @code
/// {
///   "schema_version": 1,
///   "course": { "id": 1, "name": "C++ Basics" },
///   "next_exercise_number": 4,
///   "exercises": [ { "number": 1, "name": "adder", "description": "Add two numbers" } ]
/// }
/// @endcode
///
/// Writing is safe: the data goes to a temporary file first, which then replaces
/// the real file, so a crash in the middle never corrupts an existing course.
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