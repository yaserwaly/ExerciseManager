// ExerciseManager.App.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "exercise_manager/CourseService.h"
#include "exercise_manager/DefaultsStore.h"
#include "exercise_manager/Errors.h"
#include "exercise_manager/ExerciseService.h"
#include "exercise_manager/JsonCourseRepository.h"
#include "exercise_manager/MsvcBuildRunner.h"
#include "exercise_manager/Workspace.h"
#include "exercise_manager/cli/CliApp.h"
#include "exercise_manager/cli/StdConsole.h"

#include <iostream>
#include <optional>

using namespace exercise_manager;

namespace
{

// Explains how to set the workspace folder, when it is missing or cannot be used.
void printWorkspaceHelp(const WorkspaceError& error)
{
    std::cerr << "Error: " << error.what() << "\n\n"
              << "Set " << Workspace::kEnvironmentVariable
              << " to the folder of your exercises workspace.\n"
              << "In PowerShell, for example:\n"
              << "    $env:" << Workspace::kEnvironmentVariable
              << " = \"C:\\Projects\\my-exercises\"\n"
              << "In Visual Studio: App project > Properties > Debugging > Environment:\n"
              << "    " << Workspace::kEnvironmentVariable << "=C:\\Projects\\my-exercises\n";
}

} // namespace

int main()
{
    std::optional<Workspace> workspace;
    try
    {
        workspace.emplace(Workspace::fromEnvironment());
        workspace->initialize();
    }
    catch (const WorkspaceError& error)
    {
        printWorkspaceHelp(error);
        return 1;
    }
    catch (const ExerciseManagerError& error)
    {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    try
    {
        JsonCourseRepository repository(*workspace);
        CourseService courses(repository, *workspace);
        ExerciseService exercises(repository, *workspace);
        DefaultsStore defaults(*workspace);
        MsvcBuildRunner runner(*workspace);
        cli::StdConsole console;

        cli::CliApp app(console, repository, courses, exercises, defaults, runner, *workspace);
        return app.run();
    }
    catch (const ExerciseManagerError& error)
    {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
