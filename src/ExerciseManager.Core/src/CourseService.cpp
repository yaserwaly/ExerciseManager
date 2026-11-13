#include "exercise_manager/CourseService.h"

#include "exercise_manager/Errors.h"
#include "exercise_manager/FileNaming.h"
#include "exercise_manager/NameValidator.h"

#include <algorithm>
#include <vector>
#include <filesystem>
#include <system_error>

namespace exercise_manager
{

namespace
{

// True if a course other than "exceptId" already uses this name.
// Pass 0 as exceptId when no course should be skipped (adding a new course).
bool nameUsedByAnotherCourse(const ICourseRepository& repository, const std::vector<int>& ids,
                             const std::string& name, int exceptId)
{
    for (const int id : ids)
    {
        if (id != exceptId && repository.load(id).name == name)
        {
            return true;
        }
    }
    return false;
}

} // namespace

CourseService::CourseService(ICourseRepository& repository, const Workspace& workspace)
    : m_repository(repository), m_workspace(workspace)
{
}

int CourseService::nextCourseId(const std::vector<int>& usedIds)
{
    std::vector<int> used = usedIds;
    std::sort(used.begin(), used.end());

    int candidate = 1;
    for (const int id : used)
    {
        if (id == candidate)
        {
            ++candidate;
        }
        else if (id > candidate)
        {
            break;
        }
    }

    if (candidate > FileNaming::kMaxCourseId)
    {
        throw InvalidExerciseNameError("There is no free course number.");
    }
    return candidate;
}

int CourseService::addCourse(const std::string& name, const std::string& description,
                             const std::string& fullDescription)
{
    NameValidator::validate(name);

    const std::vector<int> ids = m_repository.listCourseIds();
    if (nameUsedByAnotherCourse(m_repository, ids, name, 0))
    {
        throw DuplicateCourseError("A course with this name already exists.");
    }

    Course course;
    course.id = nextCourseId(ids);
    course.name = name;
    course.description = description;
    course.fullDescription = fullDescription;

    m_repository.save(course);
    return course.id;
}

void CourseService::updateCourse(int courseId, const std::string& name,
                                 const std::string& description, const std::string& fullDescription)
{
    NameValidator::validate(name);

    // Throws CourseNotFoundError if the course does not exist.
    Course course = m_repository.load(courseId);

    if (nameUsedByAnotherCourse(m_repository, m_repository.listCourseIds(), name, courseId))
    {
        throw DuplicateCourseError("Another course already uses this name.");
    }

    course.name = name;
    course.description = description;
    course.fullDescription = fullDescription;
    m_repository.save(course);
}

void CourseService::deleteCourse(int courseId)
{
    if (!m_repository.exists(courseId))
    {
        throw CourseNotFoundError("Course does not exist.");
    }

    // Check the path before changing anything.
    const std::filesystem::path courseDir =
        m_workspace.guard().ensureInside(m_workspace.courseDir(courseId));

    // Data first: a leftover folder is better than a listed course without files.
    m_repository.remove(courseId);

    std::error_code error;
    std::filesystem::remove_all(courseDir, error); // a missing folder is not an error
    if (error)
    {
        throw WorkspaceError("Could not delete the course folder.");
    }
}

std::vector<Course> CourseService::listCourses() const
{
    std::vector<int> ids = m_repository.listCourseIds();
    std::sort(ids.begin(), ids.end());

    std::vector<Course> courses;
    courses.reserve(ids.size());
    for (const int id : ids)
    {
        courses.push_back(m_repository.load(id));
    }
    return courses;
}

} // namespace exercise_manager
