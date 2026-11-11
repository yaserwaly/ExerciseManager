#pragma once

#include <string_view>

namespace exercise_manager
{

/// @brief Source templates of a generated exercise.
///
/// The templates live in code, so the program needs no template files at run time.
/// They contain `{{Key}}` placeholders that TemplateEngine fills in:
/// `{{Namespace}}`, `{{ClassName}}` and `{{Number}}`.
class Templates
{
  public:
    /// @brief Template of the exercise header file (e.g. `C01Exercise001.hpp`).
    static constexpr std::string_view kExerciseHeader = R"(#pragma once

namespace {{Namespace}} {

    class {{ClassName}} {
    public:
        static void Run();
    };

}
)";

    /// @brief Template of the exercise source file (e.g. `C01Exercise001.cpp`).
    static constexpr std::string_view kExerciseSource = R"(#include "{{Namespace}}.hpp"

#include <iostream>

namespace {{Namespace}} {

    void {{ClassName}}::Run() {
        std::cout << "Hello from Exercise {{Number}}!\n";
    }

}
)";

    
    /// @brief Template of the temporary launcher (`build/launcher.cpp`).
    ///
    /// It only includes the exercise header and calls `Run()`. The file is generated
    /// for every run, written into the build folder and never committed.
    /// Placeholders: `{{Namespace}}` and `{{ClassName}}`.
    static constexpr std::string_view kLauncherSource = R"(#include "{{Namespace}}.hpp"

int main()
{
    {{Namespace}}::{{ClassName}}::Run();
    return 0;
}
)";
};

} // namespace exercise_manager
