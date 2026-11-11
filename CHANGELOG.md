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

### Changed
- Exercise number comment updated: deleted numbers are now reused.
- Removed the unused `_deleted` folder from `Workspace` (`kDeletedFolder`, `deletedDir()` and its creation in `initialize()`), because exercises are now deleted permanently.