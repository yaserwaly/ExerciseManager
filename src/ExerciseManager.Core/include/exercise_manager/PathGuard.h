#pragma once

#include <filesystem>

namespace exercise_manager
{

/// @brief Makes sure every path used for reading or writing stays inside the workspace.
///
/// Paths are resolved with `std::filesystem::weakly_canonical`, so `..` segments
/// and relative paths are collapsed before the check. The path does not have to
/// exist yet.
class PathGuard
{
public:
    /// @brief Creates a guard for a workspace folder.
    /// @param workspaceRoot The workspace folder. It is resolved once, at construction.
    explicit PathGuard(const std::filesystem::path& workspaceRoot);

    /// @brief Resolves a path and checks that it is inside the workspace.
    /// @param path The path to check (absolute, or relative to the workspace).
    /// @return The resolved absolute path.
    /// @throws WorkspaceError if the resolved path is outside the workspace.
    std::filesystem::path ensureInside(const std::filesystem::path& path) const;

    /// @brief Returns the resolved workspace folder.
    const std::filesystem::path& root() const noexcept;

private:
    std::filesystem::path m_root;
};

} // namespace exercise_manager