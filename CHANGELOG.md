# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project follows [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added
- Visual Studio solution with Core, App and Tests projects.
- Shared `Directory.Build.props` (C++20, `/W4`, central output paths).
- GitHub templates for pull requests and issues.
- Contribution guidelines.
- Exercise service: add, update and permanently delete exercises (data and source files).
- Exercise numbering: a new exercise takes the first free number, and a deleted number is reused.
- Exercise templates (header and source) stored in code, filled in by `TemplateEngine`.
- Code generator: creates the `.hpp` and `.cpp` files of an exercise and never overwrites existing files.
- `ExerciseService::addExercise` now generates the exercise files, and removes them again if storing the data fails.
- Build runner interface (`IBuildRunner`) and `MsvcBuildRunner`: builds and runs one exercise on demand with `cl.exe`. Visual Studio is found with `vswhere.exe` and its environment is loaded with `vcvarsall.bat`, so no Developer Command Prompt is needed.
- Launcher template (`Templates::kLauncherSource`), generated into the workspace `build` folder for every run.
- A failed build throws `BuildError` with the compiler output, and a stale exe is never run.
- Shared sources: every `.cpp` directly inside the workspace `shared` folder is compiled with the exercise.
- Course service (`CourseService`): add, update, list and permanently delete courses (stored data and the course folder).
- Course numbering: a new course takes the first free number (1 to 99), and a deleted number is reused.
- Course names are checked by `NameValidator`, and a name already used by another course is rejected with the new `DuplicateCourseError`.
- `ICourseRepository::remove` and `JsonCourseRepository::remove`: remove a course from `Courses.json` (the course folder is deleted by the service).
- Defaults store (`DefaultsStore`): saves the default course in `Courses/DefaultCourse.json` and the default exercise of each course in `CourseNN/DefaultExercise.json`. It stores numbers only, writes safely through a temporary file, and a missing file means "no default".
- `Workspace::defaultCourseFile()` and `Workspace::defaultExerciseFile()`, with the file name constants.
- Deleting a course or an exercise also clears it as the default, because deleted numbers are reused: `CourseService::deleteCourse` clears the default course, and `ExerciseService::deleteExercise` clears the default exercise of its course.
- CLI project (`ExerciseManager.Cli`, a static library): the `IConsole` interface with the real `StdConsole`, input helpers (`ConsoleUtils`: number in a range, yes/no, text, text that keeps the current value) and `CliApp` with the menus and the navigation. Navigation uses loops and returns, never nested calls, so the whole program is tested with a fake console and a fake build runner.
- Courses screen: list, open, add, update, delete (with y/n confirmation) and set or reset the default course.
- Course screen: the exercises list, with open (builds and runs only that exercise on demand), add, update, delete (with y/n confirmation) and set or reset the default exercise.
- Lists are numbered 1 to N for display only. The number typed by the user is converted to the real course or exercise number, which is never shown as the list number.
- The default course opens when the program starts, and the default exercise opens once when arriving at a course. A default that is missing or damaged is reported, and the user is asked whether to reset it.
- Errors thrown by the Core are shown as `Error: <message>`, and a failed build shows the full compiler output. Every operation waits for Enter before the menu comes back.
- The console app starts the CLI. It reads the workspace folder from `EXERCISE_WORKSPACE`, shows a clear message with an example when it is missing, and creates the workspace folders on start.
- List order: courses and exercises are listed in creation order (the newest item is last), not by their real number. A new item that takes a free number is listed last. The stored order is the display order.
- Move: `ExerciseService::moveExercise` and `CourseService::moveCourse` move an item to another position (1 to N) and the other items shift. The numbers, folders, files and defaults do not change, and moving an item to its own position writes nothing.
- `ICourseRepository::reorder` and `JsonCourseRepository::reorder`: store a new order of the courses. The list must match the stored courses exactly, otherwise `StorageError` is thrown and nothing is written.
- New `InvalidPositionError`, thrown when a position is not between 1 and the number of items.
- CLI: a `Move` option in the courses screen and in the course screen. It asks for the item and its new position, shows the current position, and shows a message when the item is already there.

### Changed
- Exercise number comment updated: deleted numbers are now reused.
- Removed the unused `_deleted` folder from `Workspace` (`kDeletedFolder`, `deletedDir()` and its creation in `initialize()`), because exercises are now deleted permanently.
- `Directory.Build.props` also adds the CLI include folder to every project.
- `JsonCourseRepository::save` no longer sorts `Courses.json` by number: a new course goes to the end and an existing course keeps its position. `listCourseIds` and `CourseService::listCourses` return the stored order, and the CLI no longer sorts the exercises.
- Menu option numbers changed: the courses screen has `Move` = 6 and `Exit` = 7, and the course screen has `Move` = 6, `Back to courses` = 7 and `Exit` = 8.