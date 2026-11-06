#include "exercise_manager/Workspace.h"

#include "exercise_manager/Errors.h"
#include "exercise_manager/FileNaming.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <system_error>

namespace exercise_manager
{

namespace fs = std::filesystem;

namespace
{

// Reads an environment variable as a path. On Windows the wide version is used,
// so paths with Arabic or other non-ASCII characters are read correctly.
fs::path readEnvironmentPath(const char* name)
{
#ifdef _WIN32
    const std::wstring wideName(name, name + std::strlen(name));
    wchar_t* buffer = nullptr;
    std::size_t size = 0;
    if (_wdupenv_s(&buffer, &size, wideName.c_str()) != 0 || buffer == nullptr)
    {
        return fs::path();
    }
    fs::path value(buffer);
    std::free(buffer);
    return value;
#else
    const char* value = std::getenv(name);
    return value != nullptr ? fs::path(value) : fs::path();
#endif
}

void createFolder(const fs::path& folder)
{
    std::error_code error;
    fs::create_directories(folder, error);
    if (error)
    {
        throw WorkspaceError("Could not create workspace folder: " + folder.string());
    }
}

} // namespace

Workspace::Workspace(const fs::path& root) : m_guard(root)
{
}

Workspace Workspace::fromEnvironment()
{
    const fs::path root = readEnvironmentPath(kEnvironmentVariable);
    if (root.empty())
    {
        throw WorkspaceError(std::string("Environment variable ") + kEnvironmentVariable +
                             " is missing or empty.");
    }
    return Workspace(root);
}

void Workspace::initialize() const
{
    createFolder(root());
    createFolder(coursesDir());
    createFolder(sharedDir());
    createFolder(deletedDir());
    createFolder(buildDir());
}

const fs::path& Workspace::root() const noexcept
{
    return m_guard.root();
}

fs::path Workspace::coursesDir() const
{
    return root() / kCoursesFolder;
}

fs::path Workspace::sharedDir() const
{
    return root() / kSharedFolder;
}

fs::path Workspace::deletedDir() const
{
    return root() / kDeletedFolder;
}

fs::path Workspace::buildDir() const
{
    return root() / kBuildFolder;
}

fs::path Workspace::courseDir(int courseId) const
{
    return coursesDir() / FileNaming::courseFolderName(courseId);
}

fs::path Workspace::courseFile(int courseId) const
{
    return courseDir(courseId) / kCourseFileName;
}

fs::path Workspace::coursesFile() const
{
    return coursesDir() / kCoursesFileName;
}

fs::path Workspace::exercisesFile(int courseId) const
{
    return courseDir(courseId) / kExercisesFileName;
}

const PathGuard& Workspace::guard() const noexcept
{
    return m_guard;
}

} // namespace exercise_manager
