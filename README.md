# lockdep

A lockdep (lock dependency) implementation in C that provides deadlock detection capabilities for any pthread-based program, inspired by the Linux kernel's lockdep system.

## Quick Start

- **Compilation**

    First, setup the `build` directory with:

    ```bash
    cmake -S . -B build
    ```

    Then, you can compile the project with:

    ```bash
    cmake --build build
    ```

- **Usage:**

    Once the build is complete, you can use the lockdep library by preloading it with your pthread-based program. For example:

    ```bash
    # Run simple lock ordering test
    LD_PRELOAD=./build/liblockdep_interpose.so ./build/simple_test

    # Run classic AB-BA deadlock test
    LD_PRELOAD=./build/liblockdep_interpose.so ./build/deadlock_test

    # Test with any existing pthread program
    LD_PRELOAD=./build/liblockdep_interpose.so /path/to/your/program
    ```

- **Environment variables:**

    You can disable the lockdep system during runtime by setting the `LOCKDEP_DISABLE` environment variable to `1`. For example:

    ```bash
    LOCKDEP_DISABLE=1 LD_PRELOAD=./build/liblockdep_interpose.so ./your_program
    ```

## CONTRIBUTING

### Code Formatting

Make sure to align with the project's code style. The project uses `clang-format` for code formatting. You can check if your code is formatted correctly by running:

```bash
clang-format --dry-run --Werror src/*.c src/include/*.h
```

You can also format everything by running this:

```bash
find . -name "*.c" -o -name "*.h" | xargs clang-format -i
```

### Git

- Refer to [TODO.md](./TODO.md) to see what you can do. (We may improve this model later on to a issue-kanban-like system)
- When developing a new functionality, create a new branch from `main` with a descriptive name, for example `feat/lockdep-debug-system`. When the functionality is ready, create a pull request to `main` with a description of the changes made.
- Preferably, rebase your branch with the latest changes from `main` before creating the pull request.
- Make sure to make use of [conventional commits standard](https://www.conventionalcommits.org/en/v1.0.0/).
