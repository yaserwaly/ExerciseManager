#include "exercise_manager/CodeGenerator.h"

#include "exercise_manager/Errors.h"
#include "exercise_manager/FileNaming.h"
#include "exercise_manager/TemplateEngine.h"
#include "exercise_manager/Templates.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

namespace exercise_manager
{

namespace fs = std::filesystem;

namespace
{

// Writes a text file in binary mode, so the line endings stay exactly as rendered.
void writeTextFile(const fs::path& file, const std::string& text)
{
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        throw StorageError("Could not create an exercise file.");
    }
    out << text;
    out.flush();
    if (!out)
    {
        throw StorageError("Could not write an exercise file.");
    }
}

} // namespace

CodeGenerator::CodeGenerator(const Workspace& workspace) : m_workspace(workspace)
{
}

void CodeGenerator::generateExerciseFiles(int courseId, int exerciseNumber) const
{
    // These throw InvalidExerciseNameError for numbers out of range, before anything is written.
    const fs::path courseDir = m_workspace.courseDir(courseId);
    const std::string baseName = FileNaming::baseName(courseId, exerciseNumber);

    const fs::path headerFile = m_workspace.guard().ensureInside(
        courseDir / FileNaming::headerFileName(courseId, exerciseNumber));
    const fs::path sourceFile = m_workspace.guard().ensureInside(
        courseDir / FileNaming::sourceFileName(courseId, exerciseNumber));

    // Check both files before writing anything, so we never leave half an exercise.
    std::error_code error;
    if (fs::exists(headerFile, error) || fs::exists(sourceFile, error))
    {
        throw WorkspaceError("An exercise file already exists.");
    }

    // Render both texts first: if a template is broken, nothing is written.
    const TemplateEngine::Values values{
        {"Namespace", baseName},
        {"ClassName", FileNaming::className(courseId, exerciseNumber)},
        {"Number", std::to_string(exerciseNumber)}};
    const std::string headerText = TemplateEngine::render(Templates::kExerciseHeader, values);
    const std::string sourceText = TemplateEngine::render(Templates::kExerciseSource, values);

    fs::create_directories(courseDir, error);
    if (error)
    {
        throw StorageError("Could not create the course folder.");
    }

    writeTextFile(headerFile, headerText);
    writeTextFile(sourceFile, sourceText);
}

} // namespace exercise_manager
