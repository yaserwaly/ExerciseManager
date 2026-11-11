#include "exercise_manager/MsvcBuildRunner.h"
#include "exercise_manager/Errors.h"
#include "exercise_manager/FileNaming.h"
#include "exercise_manager/TemplateEngine.h"
#include "exercise_manager/Templates.h"

#include <algorithm>
#include <fstream>
#include <vector>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>

namespace exercise_manager
{

namespace fs = std::filesystem;

namespace
{

// Runs a command line and returns everything it prints (trailing line breaks removed).
// The command must already be a complete, quoted string.
std::string runAndCapture(const std::string& command)
{
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"), _pclose);
    if (!pipe)
    {
        return "";
    }

    std::array<char, 256> buffer{};
    std::string result;
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }

    while (!result.empty() &&
           (result.back() == '\n' || result.back() == '\r' || result.back() == ' '))
    {
        result.pop_back();
    }
    return result;
}

// Reads %ProgramFiles(x86)% as a path (wide version, so non-ASCII paths are safe).
fs::path programFilesX86()
{
    wchar_t* buffer = nullptr;
    std::size_t size = 0;
    if (_wdupenv_s(&buffer, &size, L"ProgramFiles(x86)") != 0 || buffer == nullptr)
    {
        return fs::path();
    }
    fs::path value(buffer);
    std::free(buffer);
    return value;
}

// Runs a command and captures its output, and also returns its exit code.
std::string runAndCaptureWithExitCode(const std::string& command, int& exitCode)
{
    FILE* pipe = _popen(command.c_str(), "r");
    if (pipe == nullptr)
    {
        exitCode = -1;
        return "";
    }

    std::array<char, 256> buffer{};
    std::string result;
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
    {
        result += buffer.data();
    }
    exitCode = _pclose(pipe);
    return result;
}

// All .cpp files directly inside the shared folder (empty if the folder is missing).
std::vector<fs::path> sharedSources(const fs::path& sharedDir)
{
    std::vector<fs::path> sources;
    std::error_code error;
    if (!fs::is_directory(sharedDir, error))
    {
        return sources;
    }
    for (const fs::directory_entry& entry : fs::directory_iterator(sharedDir, error))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".cpp")
        {
            sources.push_back(entry.path());
        }
    }
    std::sort(sources.begin(), sources.end());
    return sources;
}

void writeLauncher(const fs::path& file, const std::string& text)
{
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    out << text;
    out.flush();
    if (!out)
    {
        throw BuildError("Could not write the launcher file.");
    }
}

std::string quoted(const fs::path& path)
{
    return "\"" + path.string() + "\"";
}

} // namespace

MsvcBuildRunner::MsvcBuildRunner(const Workspace& workspace) : m_workspace(workspace)
{
}

fs::path MsvcBuildRunner::findVcvarsall()
{
    const fs::path programFiles = programFilesX86();
    if (programFiles.empty())
    {
        throw BuildError("Could not find the Program Files (x86) folder.");
    }

    const fs::path vswhere = programFiles / "Microsoft Visual Studio" / "Installer" / "vswhere.exe";
    std::error_code error;
    if (!fs::exists(vswhere, error))
    {
        throw BuildError("Could not find vswhere.exe. Is Visual Studio installed?");
    }

    // The whole command is wrapped in one extra pair of quotes. The path contains
    // "(x86)", and without this cmd.exe loses the quotes around it.
    const std::string inner = "\"" + vswhere.string() +
                              "\""
                              " -latest -products * "
                              "-requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 "
                              "-property installationPath";
    const std::string installPath = runAndCapture("\"" + inner + "\"");
    if (installPath.empty())
    {
        throw BuildError("Visual Studio with the C++ tools was not found. "
                         "Install the \"Desktop development with C++\" workload.");
    }

    const fs::path vcvarsall =
        fs::path(installPath) / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat";
    if (!fs::exists(vcvarsall, error))
    {
        throw BuildError("Could not find vcvarsall.bat in the Visual Studio folder.");
    }
    return vcvarsall;
}

BuildResult MsvcBuildRunner::buildAndRun(int courseId, int exerciseNumber)
{
    // These throw InvalidExerciseNameError for numbers out of range.
    const fs::path courseDir = m_workspace.courseDir(courseId);
    const std::string baseName = FileNaming::baseName(courseId, exerciseNumber);

    const fs::path sourceFile = m_workspace.guard().ensureInside(
        courseDir / FileNaming::sourceFileName(courseId, exerciseNumber));
    std::error_code error;
    if (!fs::is_regular_file(sourceFile, error))
    {
        throw WorkspaceError("The exercise source file does not exist.");
    }

    const fs::path buildDir = m_workspace.guard().ensureInside(m_workspace.buildDir());
    fs::create_directories(buildDir, error);
    if (error)
    {
        throw BuildError("Could not create the build folder.");
    }

    const fs::path launcherFile = buildDir / "launcher.cpp";
    const fs::path exeFile = buildDir / "exercise.exe";

    const TemplateEngine::Values values{
        {"Namespace", baseName}, {"ClassName", FileNaming::className(courseId, exerciseNumber)}};
    writeLauncher(launcherFile, TemplateEngine::render(Templates::kLauncherSource, values));

    // A stale exe must never be run if the build fails.
    fs::remove(exeFile, error);

    const fs::path vcvarsall = findVcvarsall();

    std::string inner = "cd /d " + quoted(buildDir) + " && ";
    inner += quoted(vcvarsall) + " x64 >nul && cl /nologo /std:c++20 /EHsc";
    inner += " /I" + quoted(courseDir) + " /I" + quoted(m_workspace.sharedDir());
    inner += " /Fe:" + quoted(exeFile);
    inner += " " + quoted(launcherFile) + " " + quoted(sourceFile);
    for (const fs::path& shared : sharedSources(m_workspace.sharedDir()))
    {
        inner += " " + quoted(shared);
    }

    // Extra outer quotes, same reason as in findVcvarsall().
    int buildExit = 0;
    const std::string compilerOutput =
        runAndCaptureWithExitCode("\"" + inner + "\" 2>&1", buildExit);
    if (buildExit != 0 || !fs::is_regular_file(exeFile, error))
    {
        throw BuildError("Build failed:\n" + compilerOutput);
    }

    const int exitCode = std::system(("\"" + quoted(exeFile) + "\"").c_str());
    return BuildResult{exitCode};
}

} // namespace exercise_manager
