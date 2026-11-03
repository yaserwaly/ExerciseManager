#include "exercise_manager/PathGuard.h"

#include "exercise_manager/Errors.h"

#include <system_error>

namespace exercise_manager
{

namespace fs = std::filesystem;

namespace
{

fs::path resolve(const fs::path& path)
{
    try
    {
        return fs::weakly_canonical(path);
    }
    catch (const fs::filesystem_error&)
    {
        throw WorkspaceError("Could not resolve a path inside the workspace.");
    }
}

} // namespace

PathGuard::PathGuard(const fs::path& workspaceRoot) : m_root(fs::path())
{
    if (workspaceRoot.empty())
    {
        throw WorkspaceError("Workspace path must not be empty.");
    }

    m_root = resolve(workspaceRoot);
}

fs::path PathGuard::ensureInside(const fs::path& path) const
{
    // If "path" is absolute, operator/ replaces the root, so absolute paths
    // are checked as they are and relative paths are taken from the workspace.
    const fs::path resolved = resolve(m_root / path);
    const fs::path relative = resolved.lexically_relative(m_root);

    // Rejected: an empty result (different drive), "." (the workspace root itself),
    // or a path whose first part is ".." (outside the workspace).
    if (relative.empty() || relative == fs::path(".") || *relative.begin() == fs::path(".."))
    {
        throw WorkspaceError("Path is outside the workspace.");
    }

    return resolved;
}

const fs::path& PathGuard::root() const noexcept
{
    return m_root;
}

} // namespace exercise_manager
