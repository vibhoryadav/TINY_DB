# TinyDB Developer Tooling Guide

This guide documents the verified C++20 build, sanitizer, debugging, and formatting workflow for TinyDB on macOS (AppleClang + Homebrew CMake/Ninja/GoogleTest).

---

## 1. Standard Debug Build (`-Werror`)

Configure with Ninja, strict C++20 compiler warnings (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion`), and warnings treated as errors:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DTINYDB_WARNINGS_AS_ERRORS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

*(Omit `-G Ninja` if using the default Unix Makefiles generator.)*

---

## 2. Sanitizer Build (ASan + Non-Recoverable UBSan)

Enable AddressSanitizer and UndefinedBehaviorSanitizer (`-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -g`) in a dedicated build directory:

```bash
cmake -S . -B build-asan -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DTINYDB_ENABLE_SANITIZERS=ON \
  -DTINYDB_WARNINGS_AS_ERRORS=ON
cmake --build build-asan -j
ctest --test-dir build-asan --output-on-failure
```

Using `-fno-sanitize-recover=all` ensures any runtime Undefined Behavior immediately aborts the test instead of printing a non-fatal warning.

---

## 3. Release Build

Use `-O3` optimizations with sanitizers disabled for benchmarks and release verification:

```bash
cmake -S . -B build-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DTINYDB_WARNINGS_AS_ERRORS=ON
cmake --build build-release -j
ctest --test-dir build-release --output-on-failure
```

---

## 4. Formatting Checks (`clang-format`)

Project formatting follows `.clang-format` (Google C++20 base, 2-space indent, 100-column limit).

### Via CMake Target

```bash
cmake --build build --target format-check
```

### Direct CLI Invocation

On macOS with Xcode Command Line Tools, `clang-format` lives at `/Library/Developer/CommandLineTools/usr/bin/clang-format` if not already on `PATH`:

```bash
/Library/Developer/CommandLineTools/usr/bin/clang-format --dry-run --Werror \
  include/tinydb/*.hpp src/core/*.cpp src/cli/*.cpp tests/unit/*.cpp
```

To format files in place, replace `--dry-run --Werror` with `-i`.

---

## 5. Debugging with LLDB

Launch an interactive LLDB session on the Debug test binary or `tinydb` executable:

```bash
lldb ./build/tinydb_unit_tests
```

Common LLDB commands:

```text
(lldb) breakpoint set -n GetVersionBanner
(lldb) run
(lldb) bt
(lldb) frame variable
(lldb) continue
(lldb) quit
```

Quick non-interactive batch verification:

```bash
lldb --batch \
  -o "breakpoint set -n GetVersionBanner" \
  -o "run" \
  -o "bt" \
  -o "continue" \
  ./build/tinydb_unit_tests
```

---

## 6. Pre-Commit Verification Checklist

Before committing any daily session to `develop`:

1. `cmake --build build -j && ctest --test-dir build --output-on-failure`
2. `cmake --build build-asan -j && ctest --test-dir build-asan --output-on-failure`
3. `cmake --build build --target format-check`
4. `git diff --check`
