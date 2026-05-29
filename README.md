# testftw

> Header-only C++ testing framework for the win

```

   ░██                             ░██        ░████    ░██
   ░██                             ░██       ░██       ░██
░████████  ░███████   ░███████  ░████████ ░████████ ░████████ ░██    ░██    ░██
   ░██    ░██    ░██ ░██           ░██       ░██       ░██    ░██    ░██    ░██
   ░██    ░█████████  ░███████     ░██       ░██       ░██     ░██  ░████  ░██
   ░██    ░██               ░██    ░██       ░██       ░██      ░██░██ ░██░██
    ░████  ░███████   ░███████      ░████    ░██        ░████    ░███   ░███

```

[![CI](https://github.com/notweerdmonk/testftw/actions/workflows/ci.yml/badge.svg)](https://github.com/notweerdmonk/testftw/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++11](https://img.shields.io/badge/C%2B%2B-11-blue.svg)](https://en.cppreference.com/w/cpp/11)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)

## Overview

**testftw** is a lightweight, header-only C++ testing framework designed for simplicity and cross-standard compatibility. It provides composable fixtures, testcase wrappers, testsuites, and benchmarking helpers in a single `testftw.hpp` header.

The framework is **dogfooded** — the repository includes a test suite that uses testftw itself as the runner, ensuring the framework works correctly across C++11, C++14, C++17, and C++23.

### Design Philosophy

- **Header-only**: Include `testftw.hpp` and start testing. No build system integration required.
- **Cross-standard**: Supports C++11 through C++23 with automatic feature detection.
- **Self-contained**: The test suite validates the framework using the framework itself.
- **Minimal dependencies**: Only requires the C++ standard library.

## Features

- **Type-erased fixtures** with setup/call/teardown lifecycle
- **Dual implementation paths**: `std::any` for C++17+, polymorphic wrappers for pre-C++17
- **Timed testcases** with automatic iteration and duration measurement
- **Testsuite management** with add/remove operations and suite-level fixtures
- **Ownership-transfer casting** via `unique_ptr_dynamic_cast`
- **Scoped benchmarking macros** (`BENCHMARK_START`/`BENCHMARK_END`)
- **Null-safe fixture handling** with automatic pointer argument detection
- **Compile-time configuration** through template parameters

## Installation

**testftw** is a single-header library. Copy `testftw.hpp` into your project:

```bash
cp testftw.hpp /path/to/your/project/
```

Or include it directly from this repository:

```cpp
#include <testftw.hpp>

using namespace testftw;
```

No build system configuration is required. The header is self-contained and uses only standard library headers.

## Quick Start

```cpp
#include <testftw.hpp>
#include <cassert>
#include <iostream>

using namespace testftw;

// Define a fixture with setup/call/teardown lifecycle
class counter_fixture : public fixture_base<int, int> {
  int count = 0;

  int setup(int initial) override {
    count = initial;
    return count;
  }

  int operator()(int increment) override {
    count += increment;
    return count;
  }

  int teardown(int) override {
    int final_count = count;
    count = 0;
    return final_count;
  }
};

// Define a testcase body
int add_ten(int value, std::vector<fixture_interface*>) {
  return value + 10;
}

int main() {
  // Create fixture and bind argument
  counter_fixture counter;
  auto arg = counter_fixture::makearg(5);

  // Create testcase with fixture
  testcase<false, 1, int, int> tc(add_ten, {{&counter, arg}});

  // Run and verify
  int result = tc.run(0);
  assert(result == 10);

  std::cout << "All tests passed!" << std::endl;
  return 0;
}
```

Compile and run:

```bash
g++ -std=c++17 -o example example.cpp && ./example
```

## Examples

### Basic Fixture Usage

Demonstrates the fixture lifecycle with direct and bound calls:

```cpp
#include <testftw.hpp>
#include <cassert>
#include <vector>

using namespace testftw;

class log_fixture : public fixture_base<int, int> {
  std::vector<int> *events;

  public:
  explicit log_fixture(std::vector<int> *e) : events(e) {}

  int setup(int value) override {
    events->push_back(1000 + value);
    return value;
  }

  int operator()(int value) override {
    events->push_back(2000 + value);
    return value;
  }

  int teardown(int value) override {
    events->push_back(3000 + value);
    return value;
  }
};

int main() {
  std::vector<int> events;
  log_fixture fixture(&events);

  // Direct calls (C++17+)
  #if __cplusplus >= 201703L
  assert(std::any_cast<int>(fixture.call_setup(std::any(7))) == 7);
  assert(std::any_cast<int>(fixture.call(std::any(8))) == 8);
  assert(std::any_cast<int>(fixture.call_teardown(std::any(9))) == 9);
  #endif

  // Verify lifecycle events
  assert((events == std::vector<int>{1007, 2008, 3009}));
  return 0;
}
```

### Timed Testcase with Benchmarking

Create testcases that measure execution time:

```cpp
#include <testftw.hpp>
#include <cassert>
#include <iostream>

using namespace testftw;

int fibonacci(int n, std::vector<fixture_interface*>) {
  if (n <= 1) return n;
  return fibonacci(n - 1, {}) + fibonacci(n - 2, {});
}

int main() {
  // Timed testcase: 100 iterations
  testcase<true, 100, int, int> tc(fibonacci);

  std::chrono::nanoseconds total, average;
  int result = tc.run(20, &total, &average);

  std::cout << "fibonacci(20) = " << result << std::endl;
  std::cout << "Total: " << total.count() << " ns" << std::endl;
  std::cout << "Average: " << average.count() << " ns" << std::endl;

  assert(total.count() > 0);
  assert(average.count() > 0);
  assert(average <= total);
  return 0;
}
```

### Testsuite with Suite-Level Fixtures

Group testcases into a suite with shared fixtures:

```cpp
#include <testftw.hpp>
#include <cassert>

using namespace testftw;

int test_add(int, std::vector<fixture_interface*>) {
  assert(1 + 1 == 2);
  return 0;
}

int test_subtract(int, std::vector<fixture_interface*>) {
  assert(5 - 3 == 2);
  return 0;
}

int main() {
  testcase<false, 1, int, int> t1(test_add);
  testcase<false, 1, int, int> t2(test_subtract);

  testsuite<int, int> suite;
  suite.add(&t1, 0);
  suite.add(&t2, 0);
  suite.run(0);

  std::cout << "All suite tests passed!" << std::endl;
  return 0;
}
```

### Pointer Fixture with Null Handling

Use pointer arguments with automatic null detection:

```cpp
#include <testftw.hpp>
#include <cassert>

using namespace testftw;

class ptr_fixture : public fixture_base<int, void*> {
  bool was_null = false;

  int setup(void *value) override {
    was_null = (value == nullptr);
    return was_null ? 0 : 1;
  }

  int operator()(void *value) override {
    return value ? 1 : 0;
  }

  int teardown(void *) override {
    return was_null ? 0 : 1;
  }
};

int body(void*, std::vector<fixture_interface*>) {
  return 0;
}

int main() {
  ptr_fixture fixture;
  auto arg = ptr_fixture::makearg(nullptr);

  testcase<false, 1, int, void*> tc(body, {{&fixture, arg}});
  int result = tc.run(reinterpret_cast<void*>(0x1));

  assert(result == 0);
  return 0;
}
```

## API Reference

### `unique_ptr_dynamic_cast`

```cpp
template<class derived, class base>
std::unique_ptr<derived> unique_ptr_dynamic_cast(std::unique_ptr<base>&& p);

template<class derived, class base>
std::unique_ptr<derived> unique_ptr_dynamic_cast(std::unique_ptr<base>& p);
```

Dynamically casts ownership from a base `unique_ptr` to a derived `unique_ptr`.

**Parameters:**
- `p` — Source unique_ptr (rvalue or lvalue reference)

**Returns:**
- `std::unique_ptr<derived>` on successful cast (ownership transferred)
- `nullptr` on failed cast (source retains ownership)

**Example:**

```cpp
std::unique_ptr<base_type> base_ptr(new derived_type());
std::unique_ptr<derived_type> derived_ptr =
    unique_ptr_dynamic_cast<derived_type>(std::move(base_ptr));

assert(derived_ptr.get() != nullptr);  // Cast succeeded
assert(base_ptr.get() == nullptr);      // Ownership transferred
```

---

### `BENCHMARK_START` / `BENCHMARK_END`

```cpp
#define BENCHMARK_START(runs)
#define BENCHMARK_END(totalns)
```

Scoped benchmarking macros that execute a block multiple times and measure elapsed time.

**Parameters:**
- `runs` — Number of iterations to execute
- `totalns` — `std::chrono::nanoseconds` variable receiving elapsed time

**Example:**

```cpp
std::chrono::nanoseconds total;
int counter = 0;

BENCHMARK_START(1000)
  ++counter;
BENCHMARK_END(total)

assert(counter == 1000);
std::cout << "Elapsed: " << total.count() << " ns" << std::endl;
```

---

### `fixture_interface`

Abstract base class for type-erased fixtures.

#### C++17+ Interface

```cpp
class fixture_interface {
public:
  using fixture_interface_argtype = std::shared_ptr<std::any>;

  virtual std::any call_setup(const std::any&) = 0;
  virtual std::any bindcall_setup() = 0;
  virtual std::any call(const std::any&) = 0;
  virtual std::any bindcall() = 0;
  virtual std::any call_teardown(const std::any&) = 0;
  virtual std::any bindcall_teardown() = 0;
  virtual void setarg(const fixture_interface_argtype&) = 0;
  virtual ~fixture_interface() = default;

protected:
  fixture_interface_argtype arg;
};
```

#### Pre-C++17 Interface

```cpp
class fixture_interface {
public:
  struct fixture_interface_ret { virtual ~fixture_interface_ret() = default; };
  struct fixture_interface_arg { virtual ~fixture_interface_arg() = default; };

  using fixture_interface_returntype = std::shared_ptr<fixture_interface_ret>;
  using fixture_interface_argtype = std::shared_ptr<fixture_interface_arg>;

  virtual fixture_interface_returntype call_setup(const fixture_interface_argtype&) = 0;
  virtual fixture_interface_returntype call(const fixture_interface_argtype&) = 0;
  virtual fixture_interface_returntype call_teardown(const fixture_interface_argtype&) = 0;
  virtual void setarg(const fixture_interface_argtype&) = 0;
  virtual fixture_interface_returntype bindcall_setup() = 0;
  virtual fixture_interface_returntype bindcall() = 0;
  virtual fixture_interface_returntype bindcall_teardown() = 0;
  virtual ~fixture_interface() = default;

protected:
  fixture_interface_argtype arg;
};
```

**Key Methods:**
- `call_*()` — Invoke with an explicit argument
- `bindcall_*()` — Invoke with the bound argument (set via `setarg()`)
- `setarg()` — Bind an argument for subsequent `bindcall_*` invocations

---

### `fixture_base<returntype, argtype>`

Typed fixture implementation. Derive from this class and implement `setup()`, `operator()()`, and `teardown()`.

```cpp
template<typename returntype, typename argtype>
class fixture_base : public fixture_interface {
public:
  // Create a bound argument object
  static fixture_interface_argtype makearg(argtype a);

  // Pure virtual hooks to implement
  virtual returntype setup(argtype a) = 0;
  virtual returntype operator()(argtype a) = 0;
  virtual returntype teardown(argtype a) = 0;

  // Bind an argument for bindcall_* operations
  void setarg(const fixture_interface_argtype& a) override;
};
```

**Template Parameters:**
- `returntype` — Return type for setup, body, and teardown operations
- `argtype` — Argument type accepted by the fixture

**Static Methods:**

```cpp
// Create a bound argument object for this fixture type
static fixture_interface_argtype makearg(argtype a);

// (Pre-C++17 only) Cast type-erased return wrapper to typed wrapper
static fixture_base_returntype retval(const fixture_interface_returntype& r);
```

- `makearg(argtype a)` — Create a bound argument object for this fixture type. Returns `fixture_interface_argtype` (a `std::shared_ptr<std::any>` in C++17+ or a shared polymorphic wrapper in pre-C++17).
- `retval(const fixture_interface_returntype& r)` — *(Pre-C++17 only)* Cast a type-erased return wrapper to this fixture's typed return wrapper. Returns `fixture_base_returntype` or `nullptr` on type mismatch.

**Example:**

```cpp
class my_fixture : public fixture_base<int, int> {
  int setup(int value) override {
    // Initialization logic
    return value;
  }

  int operator()(int value) override {
    // Test body logic
    return value * 2;
  }

  int teardown(int value) override {
    // Cleanup logic
    return value;
  }
};
```

---

### `testcase_base<return_type, arg_type>`

Abstract testcase interface stored by `testsuite`.

```cpp
template<typename return_type = int, typename arg_type = void*>
class testcase_base {
public:
  #if __cplusplus >= 201703L
  virtual std::optional<return_type> operator()(
    arg_type data,
    std::chrono::nanoseconds *ptotal = nullptr,
    std::chrono::nanoseconds *pns = nullptr
  ) = 0;
  #else
  virtual return_type operator()(
    arg_type data,
    std::chrono::nanoseconds *ptotal = nullptr,
    std::chrono::nanoseconds *pns = nullptr
  ) = 0;
  #endif

  virtual ~testcase_base() = default;
};
```

---

### `testcase<timed, runs, return_type, arg_type, fn_type>`

Concrete testcase wrapper with optional timing and iteration.

```cpp
template<
  bool timed = false,
  std::size_t runs = 1,
  typename return_type = int,
  typename arg_type = void*,
  typename fn_type = std::function<return_type(arg_type, std::vector<fixture_interface*>)>
>
class testcase final : public testcase_base<return_type, arg_type> {
public:
  // Fixture pointer plus bound argument for construction
  struct fixture_pack {
    fixture_interface *fixture;
    const fixture_interface::fixture_interface_argtype &arg;
  };

  // Construct from callable and fixture packs
  testcase(fn_type fn_, std::initializer_list<fixture_pack> fixtures_ = {});

  // Construct from callable and varargs fixture packs
  testcase(fn_type fn_, std::size_t nfixtures_, ...);

  // Construct from existing fixture pointer vector
  testcase(fn_type fn_, const std::vector<fixture_interface*>& fixtures_);
  testcase(fn_type fn_, std::vector<fixture_interface*>&& fixtures_);

  // Run the testcase
  return_type run(
    arg_type data,
    std::chrono::nanoseconds *ptotal = nullptr,
    std::chrono::nanoseconds *pavg = nullptr
  );
};
```

**Template Parameters:**
| Parameter | Default | Description |
|-----------|---------|-------------|
| `timed` | `false` | Enable timing and iteration |
| `runs` | `1` | Number of iterations (when timed) |
| `return_type` | `int` | Testcase return type |
| `arg_type` | `void*` | Testcase argument type |
| `fn_type` | `std::function<...>` | Callable signature |

**Constructors:**
- Initializer list: `testcase(fn, {{&fixture1, arg1}, {&fixture2, arg2}})`
- Varargs: `testcase(fn, nfixtures, fixture_ptr1, fixture_arg1, ...)`
- Vector: `testcase(fn, std::vector<fixture_interface*>{...})`

---

### `testsuite<return_type, arg_type>`

Collection of testcases with optional suite-level fixtures.

```cpp
template<typename return_type = int, typename arg_type = void*>
class testsuite {
public:
  using times_type = std::pair<std::chrono::nanoseconds, std::chrono::nanoseconds>;

  // Fixture pointer plus bound argument
  struct fixture_pack {
    fixture_interface *fixture;
    const fixture_interface::fixture_interface_argtype &arg;
  };

  // Testcase pointer plus argument
  struct testcase_pack {
    testcase_type *testcase;
    arg_type arg;
  };

  // Constructors
  testsuite(std::initializer_list<testcase_pack> testcases_,
            std::initializer_list<fixture_pack> fixtures_ = {});
  testsuite(std::initializer_list<testcase_pack> testcases_);
  testsuite(std::initializer_list<fixture_pack> fixtures_ = {});
  testsuite(const std::vector<testcase_pack>& testcases_,
            const std::vector<fixture_interface*>& fixtures_ = {});
  testsuite(std::vector<testcase_pack>&& testcases_,
            std::vector<fixture_interface*>&& fixtures_ = {});

  // Add a testcase to the suite
  void add(testcase_type *tcptr, arg_type arg);

  // Remove matching testcase from the suite
  void remove(testcase_type *tcptr, arg_type arg);

  // Run all testcases
  void run(arg_type data, std::vector<times_type> *ptimes = nullptr);
};
```

**Key Methods:**
- `add(tcptr, arg)` — Add a testcase with its argument
- `remove(tcptr, arg)` — Remove a matching testcase
- `run(data, ptimes)` — Execute all testcases; suite fixtures run setup before and teardown after

**Example:**

```cpp
testcase<false, 1, int, int> t1(test_func);
testsuite<int, int> suite(
  {{&t1, 42}},              // testcase packs
  {{&fixture, fixture_arg}} // suite-level fixtures
);
suite.run(0);
```

## Architecture

### Type Erasure Strategy

testftw uses type erasure to store fixtures and testcases with heterogeneous types in containers. The framework provides two implementations:

| Standard | Mechanism | Trade-offs |
|----------|-----------|------------|
| C++17+ | `std::any` | Simpler code, requires `<any>` header |
| Pre-C++17 | Polymorphic wrappers | More code, no extra headers |

### C++17+ vs Pre-C++17 Code Paths

The header uses `#if __cplusplus >= 201703L` to select implementations:

```
testftw.hpp
├── std::make_unique polyfill (C++11 only)
├── fixture_interface
│   ├── C++17+: std::any-based interface
│   └── Pre-C++17: Polymorphic wrapper interface
├── fixture_base<returntype, argtype>
│   ├── C++17+: Direct std::any_cast operations
│   └── Pre-C++17: Wrapper-based type conversion
├── testcase<timed, runs, ...>
│   ├── C++17+: if constexpr for timed/untimed paths
│   └── Pre-C++17: SFINAE template specialization
└── testsuite<return_type, arg_type>
    └── Shared implementation (uses testcase_base)
```

### Fixture Lifecycle

```
       ┌──────────────────────┐
       │  testcase.run(data)  │
       └──────────┬───────────┘
                  │
                  ▼
    ┌────────────────────────────┐
    │  For each fixture:         │
    │  fixture.bindcall_setup()  │
    └─────────────┬──────────────┘
                  │
                  ▼
     ┌─────────────────────────┐
     │  Execute testcase body  │
     │  fn(data, fixtures)     │
     └────────────┬────────────┘
                  │
                  ▼
  ┌───────────────────────────────┐
  │  For each fixture:            │
  │  fixture.bindcall_teardown()  │
  └───────────────┬───────────────┘
                  │
                  ▼
         ┌─────────────────┐
         │  Return result  │
         └─────────────────┘
```

**Suite-Level Fixtures:**

When fixtures are added to a `testsuite`, they execute their lifecycle around **all** testcases:

```
suite.run()
├── suite fixture setup (once)
├── testcase 1
├── testcase 2
├── ...
└── suite fixture teardown (once)
```

## Building & Testing

All commands are run from the `tests/` directory:

```bash
cd tests
```

### Check (Default)

Build and run the test suite with the default C++ standard (C++23):

```bash
make check
```

### Compatibility Check

Run the test suite across all supported C++ standards:

```bash
make compat
```

This executes:
1. `make clean && make check CXX_VERSION=c++11`
2. `make clean && make check CXX_VERSION=c++14`
3. `make clean && make check CXX_VERSION=c++17`
4. `make clean && make check CXX_VERSION=c++23`

### Sanitizer Build

Run with AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
make asan
```

### Clean

Remove generated test binaries:

```bash
make clean
```

### Custom Compiler

Override the compiler with `CXX`:

```bash
make check CXX=clang++
```

## Compatibility

| Standard | Status | Notes |
|----------|--------|-------|
| C++11 | Supported | Uses `std::make_unique` polyfill, polymorphic fixture wrappers |
| C++14 | Supported | Same as C++11 with improved SFINAE |
| C++17 | Supported | Uses `std::any`, `if constexpr`, `std::optional` |
| C++23 | Supported | Full modern C++ support |

### Tested Compilers

- GCC (g++) — Primary development compiler
- Clang (clang++) — Verified via `CXX=clang++ make check`

### Required Headers

| Header | Standard | Purpose |
|--------|----------|---------|
| `<cstdint>` | C++11 | Fixed-width integers |
| `<cstdarg>` | C++11 | Varargs support |
| `<utility>` | C++11 | `std::move`, `std::forward` |
| `<vector>` | C++11 | Container storage |
| `<functional>` | C++11 | `std::function` |
| `<algorithm>` | C++11 | `std::remove` |
| `<memory>` | C++11 | Smart pointers |
| `<chrono>` | C++11 | Timing utilities |
| `<optional>` | C++17 | Optional return values |
| `<any>` | C++17 | Type-erased storage |

## Generating Documentation

The project includes a `Doxyfile` for generating API documentation:

```bash
# Generate HTML documentation
doxygen Doxyfile

# Open in browser (Linux)
xdg-open docs/html/index.html
```

Documentation is output to `docs/html/`.

## Contributing

### Code Style

- Two-space indentation in class bodies and control blocks
- Opening braces on the same line for functions and conditionals
- `lower_snake_case` for identifiers
- Template parameter names should be descriptive (e.g., `return_type`, `arg_type`)

### Testing Requirements

- All new functionality must include test cases in `tests/testftw_tests.cc`
- Tests must pass across all supported C++ standards (`make compat`)
- New code should pass sanitizer checks (`make asan`)

### Commit Guidelines

- One concise sentence per commit
- Follow up in paragraphs with details if you may
- Capitalize first word

### Pull Requests

- Describe the API or behavior change
- List C++ standards tested
- Mention compatibility tradeoffs
- Include command output summaries

## License

MIT License — see [LICENSE](LICENSE) for details.

Copyright (C) 2026 notweerdmonk

## Acnkowledgements

Assisted-by: Codex:gpt-5.5-medium

Assisted-by: OpenCode:mimo-v2.5-free
