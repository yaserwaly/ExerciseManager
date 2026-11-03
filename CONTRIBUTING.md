\# Contributing to ExerciseManager



\## Requirements



\- Windows

\- Visual Studio 2026 (C++20, `/std:c++20`)



\## Workflow



1\. Create a branch from `main` using one of these prefixes:

&#x20;  `feature/`, `fix/`, `docs/`, `chore/`, `test/`, `ci/`

2\. Make small, focused commits.

3\. Open a Pull Request and fill in the template.

4\. The PR is merged with \*\*Squash and merge\*\*, then the branch is deleted.



\## Commit messages



This project uses \[Conventional Commits](https://www.conventionalcommits.org/),

in English, imperative mood:



```

feat: add name validator

fix: reject paths that escape the workspace

docs: describe workspace setup

```



Types: `feat`, `fix`, `docs`, `test`, `chore`, `ci`, `refactor`.



\## Code style



\- Namespaces: `snake\_case`

\- Classes and files: `PascalCase`

\- Functions and variables: `camelCase`

\- Member variables: `m` prefix

\- Constants: `kPascalCase`



\## Privacy rule



Never commit personal data. Courses, exercises, and shared libraries live in a

separate workspace outside this repository.



\## Before every commit



\- Run `git status` and check that no personal or generated files are staged.

\- Make sure the solution builds and the tests pass.

