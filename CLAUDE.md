# MobileNetworkOperatorDemo

## Domain
Simplified billing library for a mobile network operator. Calculates call cost given start/end
datetime, dialed number, and subscriber account state.

## Project Structure
```
MobileNetworkOperatorDemo/
├── CMakeLists.txt          ← root; C++17; FetchContent for doctest
├── lib/billing/            ← static library `billing` — all pricing logic
├── app/demo/               ← CLI demo executable `demo`
├── tests/                  ← doctest unit-test executable `test_billing`
└── docs/plans/             ← spec (*.spec.md) and implementation plan (*.md)
```

## Targets

| CMake target   | Type       | Description                          |
|----------------|------------|--------------------------------------|
| `billing`      | STATIC lib | Core billing logic; no runtime deps  |
| `demo`         | executable | Demo app; links `billing`            |
| `test_billing` | executable | All unit tests; links `billing`      |

## C++ Standard & Compilers
- **C++17** (`CMAKE_CXX_STANDARD 17`, extensions OFF)
- GCC ≥ 9, Clang ≥ 10, MSVC 2019+
- No platform-specific headers; no Boost/Qt

## Compiler Warnings
- GCC/Clang: `-Wall -Wextra -Wpedantic -Werror` (applied to `billing` target)
- MSVC: `/W4 /WX`

## Dependencies

| Dependency      | How fetched          | Used by                |
|-----------------|----------------------|------------------------|
| doctest v2.4.11 | CMake `FetchContent` | `test_billing` only    |

Tests are built by default. Set `-DBUILD_TESTS=OFF` to skip.

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Run Tests

```bash
ctest --test-dir build --output-on-failure
# or directly:
./build/tests/test_billing
```

## Run Demo

```bash
./build/app/demo/demo
```

## Sanitizer Build

```bash
cmake -B build-asan -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
cmake --build build-asan --target test_billing
./build-asan/tests/test_billing
```

## Disable Tests

```bash
cmake -B build -DBUILD_TESTS=OFF
cmake --build build
```
