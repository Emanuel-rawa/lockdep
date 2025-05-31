# lockdep project - Operating Systems class UFRN

## One-time setup

Make sure you have the `clang` compiler installed on your system and `cmake`. You can install it using your package manager, for example:

```bash
sudo apt install clang cmake
```

## Compiling and usage

First, setup the `build` directory with:

```bash
cmake -S . -B build
```

Then, you can compile the project with:

```bash
cmake --build build
```

The compiled binary will be available at `build/lockdep`.

## CONTRIBUTING

- Refer to [TODO.md](./TODO.md) to see what you can do. (We may improve this model later on to a issue-kanban-like system)
- When developing a new functionality, create a new branch from `main` with a descriptive name, for example `feat/lockdep-debug-system`. When the functionality is ready, create a pull request to `main` with a description of the changes made.
- Preferably, rebase your branch with the latest changes from `main` before creating the pull request.
- Make sure to make use of [conventional commits standard](https://www.conventionalcommits.org/en/v1.0.0/).
