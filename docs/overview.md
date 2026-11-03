# ExerciseManager



A personal C++ console tool for managing coding exercises across a series of

courses inside a single project, instead of one project per exercise.



\## Idea



Each exercise is a small pair of source files (`.hpp` / `.cpp`) stored in a

separate personal workspace. The tool adds, renames and deletes exercises,

numbers them automatically, and builds and runs only the selected exercise on

demand.



\## Architecture



The code is split into layers:



\- \*\*Domain\*\*: exercises, courses and the error types.

\- \*\*Application\*\*: services that implement the use cases.

\- \*\*Infrastructure\*\*: JSON storage, the workspace on disk, and the compiler.

\- \*\*App\*\*: the console user interface (menus and input handling only).



\## Testing strategy



Tests use GoogleTest. Each test works on its own temporary workspace, so tests

never touch real personal data.



\## Privacy



Courses, exercises and shared libraries live in a separate personal workspace

outside this repository. They are never part of the source code or the

documentation.

