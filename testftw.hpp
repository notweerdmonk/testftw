/*
 * testftw - testing framework for C++ for the win
 * Copyright (C) 2026 notweerdmonk
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

/*
 *
 *    ░██                             ░██        ░████    ░██
 *    ░██                             ░██       ░██       ░██
 * ░████████  ░███████   ░███████  ░████████ ░████████ ░████████ ░██    ░██    ░██
 *    ░██    ░██    ░██ ░██           ░██       ░██       ░██    ░██    ░██    ░██
 *    ░██    ░█████████  ░███████     ░██       ░██       ░██     ░██  ░████  ░██
 *    ░██    ░██               ░██    ░██       ░██       ░██      ░██░██ ░██░██
 *     ░████  ░███████   ░███████      ░████    ░██        ░████    ░███   ░███
 *
 */

/**
 * @file testftw.hpp
 * @brief Header-only test and fixture framework for C++.
 *
 * testftw provides composable fixtures, testcase wrappers, testsuites, and
 * small benchmarking helpers. The header supports modern C++ through
 * std::any/std::optional when compiling as C++17 or newer, and provides a
 * typed wrapper fallback for older language modes.
 */

#ifndef _TESTFTW_HPP_
#define _TESTFTW_HPP_

#include <cstdint>
#include <cstdarg>
#include <utility>
#include <vector>
#include <functional>
#include <algorithm>
#include <optional>
#include <memory>
#include <chrono>

#if __cplusplus >= 201703L
#include <any>
#endif

#if __cplusplus < 201402L

namespace std {
  namespace detail {

    template<typename>
      struct is_unbounded_array {
        static constexpr bool value = false;
      };

    template<typename T>
      struct is_unbounded_array<T[]> {
        static constexpr bool value = true;
      };

    template<typename>
      struct is_bounded_array {
        static constexpr bool value = false;
      };

    template<typename T, std::size_t N>
      struct is_bounded_array<T[N]> {
        static constexpr bool value = true;
      };

  }; /* namespace detail */

  template<
    typename T,
    class... Args
  >
  typename std::enable_if<!std::is_array<T>::value, std::unique_ptr<T>>::type
  make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
  }

  template<
    typename T
  >
  typename std::enable_if<
    detail::is_unbounded_array<T>::value,
    std::unique_ptr<T>
  >::type
  make_unique(std::size_t n) {
    return std::unique_ptr<T>(new T(std::remove_extent<T>::type[n]()));
  }

  template<
    typename T,
    class... Args
  >
  typename std::enable_if<
    detail::is_bounded_array<T>::value,
    std::unique_ptr<T>
  >::type
  make_unique(Args&&... args) = delete;

}; /* namespace std */

#endif /* __cplusplus < 201402L */

namespace testftw {

/**
 * @brief Dynamically cast ownership from a base unique_ptr to a derived
 * unique_ptr.
 *
 * On success, ownership is released from @p p and returned as a
 * std::unique_ptr<derived>. On failure, @p p retains ownership and nullptr is
 * returned.
 *
 * @tparam derived Target pointee type.
 * @tparam base Source pointee type.
 * @param p Source unique_ptr rvalue.
 * @return A derived unique_ptr on success, or nullptr on failed cast.
 */
template<class derived, class base>
std::unique_ptr<derived>
unique_ptr_dynamic_cast(std::unique_ptr<base>&& p) {
  if (!p) {
    return nullptr;
  }
  if (auto raw = dynamic_cast<derived*>(p.get())) {
    p.release();
    return std::unique_ptr<derived>(raw);
  }
  return nullptr; // p still owns (since we didn't release)
}

/**
 * @brief Dynamically cast ownership from a base unique_ptr lvalue reference.
 *
 * On success, ownership is released from @p p and returned as a
 * std::unique_ptr<derived>. On failure, @p p retains ownership and nullptr is
 * returned.
 *
 * @tparam derived Target pointee type.
 * @tparam base Source pointee type.
 * @param p Source unique_ptr lvalue reference.
 * @return A derived unique_ptr on success, or nullptr on failed cast.
 */
template<class derived, class base>
std::unique_ptr<derived>
unique_ptr_dynamic_cast(std::unique_ptr<base>& p) {
  if (!p) {
    return nullptr;
  }
  if (auto raw = dynamic_cast<derived*>(p.get())) {
    p.release();
    return std::unique_ptr<derived>(raw);
  }
  return nullptr; // p still owns (since we didn't release)
}

/**
 * @def BENCHMARK_START(runs)
 * @brief Start a scoped benchmark loop.
 *
 * Opens a repeated block that executes @p runs times. Must be paired with
 * BENCHMARK_END(totalns).
 *
 * @param runs Number of iterations to execute.
 */
#define BENCHMARK_START(runs) \
  do { \
    auto __start = std::chrono::high_resolution_clock::now(); \
    for (auto i = 0; i < (runs); ++i) \
      do {

/**
 * @def BENCHMARK_END(totalns)
 * @brief End a benchmark loop and store elapsed time.
 *
 * Closes a block opened by BENCHMARK_START and assigns the total elapsed
 * duration to @p totalns.
 *
 * @param totalns std::chrono::nanoseconds variable receiving elapsed time.
 */
#define BENCHMARK_END(totalns) \
      } while (0); \
    auto __stop = std::chrono::high_resolution_clock::now(); \
    (totalns) =\
      std::chrono::duration_cast<std::chrono::nanoseconds>(__stop - __start); \
  } while (0);

#if __cplusplus >= 201703L

/**
 * @brief Type-erased fixture interface for C++17 and newer.
 *
 * The interface exposes setup, test-call, and teardown operations. Each
 * operation can be invoked with an explicit std::any argument or with a
 * previously bound argument supplied by setarg().
 */
class fixture_interface {
  public:
  /**
   * @brief Invoke setup with an explicit argument.
   * @param arg Type-erased argument passed to the fixture.
   * @return Type-erased setup return value.
   */
  virtual
  std::any call_setup(const std::any&) = 0;

  /**
   * @brief Invoke setup with the bound fixture argument.
   * @return Type-erased setup return value.
   */
  virtual
  std::any bindcall_setup() = 0;

  /**
   * @brief Invoke the fixture body with an explicit argument.
   * @param arg Type-erased argument passed to the fixture.
   * @return Type-erased fixture body return value.
   */
  virtual
  std::any call(const std::any&) = 0;

  /**
   * @brief Invoke the fixture body with the bound fixture argument.
   * @return Type-erased fixture body return value.
   */
  virtual
  std::any bindcall() = 0;

  /**
   * @brief Invoke teardown with an explicit argument.
   * @param arg Type-erased argument passed to the fixture.
   * @return Type-erased teardown return value.
   */
  virtual
  std::any call_teardown(const std::any&) = 0;

  /**
   * @brief Invoke teardown with the bound fixture argument.
   * @return Type-erased teardown return value.
   */
  virtual
  std::any bindcall_teardown() = 0;

  /**
   * @brief Shared pointer to a bound std::any fixture argument.
   */
  using fixture_interface_argtype = std::shared_ptr<std::any>;

  /**
   * @brief Bind an argument for later bindcall_* invocations.
   * @param arg Shared type-erased argument storage.
   */
  virtual
  void setarg(const fixture_interface_argtype&) = 0;

  /**
   * @brief Destroy the fixture interface.
   */
  virtual ~fixture_interface() = default;

  protected:
  fixture_interface_argtype arg;
};

/**
 * @brief Typed fixture base implementation for C++17 and newer.
 *
 * Derive from this class and implement setup(), operator()(), and teardown().
 * The base class handles conversion between std::any and the fixture's typed
 * argument and return values.
 *
 * @tparam returntype Return type for setup, body, and teardown operations.
 * @tparam argtype Argument type accepted by the fixture.
 */
template<typename returntype, typename argtype>
class fixture_base : public fixture_interface {
  /**
   * @brief Invoke setup with an explicit std::any argument.
   * @param arg Type-erased argument; null pointer converted to nullptr_t.
   * @return Type-erased setup return value.
   */
  std::any call_setup(const std::any &arg) override {
    argtype local_arg;
    if constexpr (std::is_pointer<argtype>::value) {
      if (std::any_cast<std::nullptr_t>(&arg)) {
        local_arg = argtype(nullptr);
      } else {
        local_arg = std::any_cast<argtype>(arg);
      }
    } else {
      local_arg = std::any_cast<argtype>(arg);
    }
    return this->setup(local_arg);
  }

  /**
   * @brief Invoke setup with the bound std::any argument.
   * @return Type-erased setup return value.
   */
  std::any bindcall_setup() override {
    argtype local_arg;
    if constexpr (std::is_pointer<argtype>::value) {
      if (!arg.get() || std::any_cast<std::nullptr_t>(arg.get())) {
        local_arg = argtype(nullptr);
      } else {
        local_arg = std::any_cast<argtype>(*arg);
      }
    } else {
      local_arg = std::any_cast<argtype>(*arg);
    }
    return this->setup(local_arg);
  }

  /**
   * @brief Invoke the fixture body with an explicit std::any argument.
   * @param arg Type-erased argument; null pointer converted to nullptr_t.
   * @return Type-erased fixture body return value.
   */
  std::any call(const std::any &arg) override {
    argtype local_arg;
    if constexpr (std::is_pointer<argtype>::value) {
      if (std::any_cast<std::nullptr_t>(&arg)) {
        local_arg = argtype(nullptr);
      } else {
        local_arg = std::any_cast<argtype>(arg);
      }
    } else {
      local_arg = std::any_cast<argtype>(arg);
    }
    return this->operator()(local_arg);
  }

  /**
   * @brief Invoke the fixture body with the bound std::any argument.
   * @return Type-erased fixture body return value.
   */
  std::any bindcall() override {
    argtype local_arg;
    if constexpr (std::is_pointer<argtype>::value) {
      if (!arg.get() || std::any_cast<std::nullptr_t>(arg.get())) {
        local_arg = argtype(nullptr);
      } else {
        local_arg = std::any_cast<argtype>(*arg);
      }
    } else {
      local_arg = std::any_cast<argtype>(*arg);
    }
    return this->operator()(local_arg);
  }

  /**
   * @brief Invoke teardown with an explicit std::any argument.
   * @param arg Type-erased argument; null pointer converted to nullptr_t.
   * @return Type-erased teardown return value.
   */
  std::any call_teardown(const std::any &arg) override {
    argtype local_arg;
    if constexpr (std::is_pointer<argtype>::value) {
      if (std::any_cast<std::nullptr_t>(&arg)) {
        local_arg = argtype(nullptr);
      } else {
        local_arg = std::any_cast<argtype>(arg);
      }
    } else {
      local_arg = std::any_cast<argtype>(arg);
    }
    return this->teardown(local_arg);
  }

  /**
   * @brief Invoke teardown with the bound std::any argument.
   * @return Type-erased teardown return value.
   */
  std::any bindcall_teardown() override {
    argtype local_arg;
    if constexpr (std::is_pointer<argtype>::value) {
      if (!arg.get() || std::any_cast<std::nullptr_t>(arg.get())) {
        local_arg = argtype(nullptr);
      } else {
        local_arg = std::any_cast<argtype>(*arg);
      }
    } else {
      local_arg = std::any_cast<argtype>(*arg);
    }
    return this->teardown(local_arg);
  }

  /**
   * @brief Fixture setup hook.
   * @param a Typed fixture argument.
   * @return Setup result.
   */
  virtual returntype setup(argtype a) = 0;

  /**
   * @brief Fixture body hook.
   * @param a Typed fixture argument.
   * @return Fixture body result.
   */
  virtual returntype operator()(argtype a) = 0;

  /**
   * @brief Fixture teardown hook.
   * @param a Typed fixture argument.
   * @return Teardown result.
   */
  virtual returntype teardown(argtype a) = 0;

  public:
  /**
   * @brief Bind an argument for bindcall_* operations.
   * @param a Shared std::any argument storage.
   */
  void
  setarg(const fixture_interface_argtype &a) override {
    arg = std::move(const_cast<fixture_interface_argtype&>(a));
  }

  /**
   * @brief Create a bound argument object for this fixture type.
   * @param a Typed argument value.
   * @return Shared type-erased argument storage.
   */
  static
  fixture_interface_argtype
  makearg(argtype a) {
    return std::make_shared<std::any>(a);
  }

  /**
   * @brief Destroy the typed fixture base.
   */
  virtual ~fixture_base() = default;
};

#else /* ! __cplusplus >= 201703L */

/**
 * @brief Type-erased fixture interface for pre-C++17 builds.
 *
 * This interface uses polymorphic wrapper objects instead of std::any so the
 * same fixture lifecycle can be used in C++11 and C++14.
 */
class fixture_interface {
  public:

  /**
   * @brief Base type for wrapped fixture return values.
   */
  struct fixture_interface_ret {
    virtual ~fixture_interface_ret() = default;
  };

  /**
   * @brief Base type for wrapped fixture arguments.
   */
  struct fixture_interface_arg {
    virtual ~fixture_interface_arg() = default;
  };

  /**
   * @brief Shared pointer to a type-erased fixture return wrapper.
   */
  using fixture_interface_returntype = std::shared_ptr<fixture_interface_ret>;

  /**
   * @brief Shared pointer to a type-erased fixture argument wrapper.
   */
  using fixture_interface_argtype = std::shared_ptr<fixture_interface_arg>;

  /**
   * @brief Invoke setup with an explicit wrapped argument.
   * @param arg Wrapped argument.
   * @return Wrapped setup return value, or nullptr for null input.
   */
  virtual
  fixture_interface_returntype call_setup(
    const fixture_interface_argtype&
  ) = 0;

  /**
   * @brief Invoke the fixture body with an explicit wrapped argument.
   * @param arg Wrapped argument.
   * @return Wrapped fixture body return value, or nullptr for null input.
   */
  virtual
  fixture_interface_returntype call(
    const fixture_interface_argtype&
  ) = 0;

  /**
   * @brief Invoke teardown with an explicit wrapped argument.
   * @param arg Wrapped argument.
   * @return Wrapped teardown return value, or nullptr for null input.
   */
  virtual
  fixture_interface_returntype call_teardown(
    const fixture_interface_argtype&
  ) = 0;

  /**
   * @brief Bind an argument for later bindcall_* invocations.
   * @param arg Wrapped bound argument.
   */
  virtual
  void setarg(const fixture_interface_argtype&) = 0;

  /**
   * @brief Invoke setup with the bound argument.
   * @return Wrapped setup return value.
   */
  virtual
  fixture_interface_returntype bindcall_setup() = 0;

  /**
   * @brief Invoke the fixture body with the bound argument.
   * @return Wrapped fixture body return value.
   */
  virtual
  fixture_interface_returntype bindcall() = 0;

  /**
   * @brief Invoke teardown with the bound argument.
   * @return Wrapped teardown return value.
   */
  virtual
  fixture_interface_returntype bindcall_teardown() = 0;

  /**
   * @brief Destroy the fixture interface.
   */
  virtual ~fixture_interface() = default;

  protected:
  fixture_interface_argtype arg;
};

/**
 * @brief Typed fixture base implementation for pre-C++17 builds.
 *
 * Derive from this class and implement setup(), operator()(), and teardown().
 * The base class wraps and unwraps typed arguments and return values through
 * polymorphic wrapper objects.
 *
 * @tparam returntype Return type for setup, body, and teardown operations.
 * @tparam argtype Argument type accepted by the fixture.
 */
template<typename returntype, typename argtype>
class fixture_base : public fixture_interface {
  public:
  /**
   * @brief Typed return wrapper used by pre-C++17 fixture calls.
   */
  struct fixture_base_ret : fixture_interface_ret {
    /**
     * @brief Stored typed return value.
     */
    returntype value;

    /**
     * @brief Construct a return wrapper.
     * @param value_ Value to store.
     */
    fixture_base_ret(const returntype &value_) : value(value_) { }
  };

  /**
   * @brief Typed argument wrapper used by pre-C++17 fixture calls.
   */
  struct fixture_base_arg : fixture_interface_arg {
    /**
     * @brief Stored typed argument value.
     */
    argtype value;

    /**
     * @brief Construct an argument wrapper.
     * @param value_ Value to store.
     */
    fixture_base_arg(const argtype &value_) : value(value_) { }
  };

  /**
   * @brief Shared pointer to this fixture's typed return wrapper.
   */
  using fixture_base_returntype = std::shared_ptr<fixture_base_ret>;

  /**
   * @brief Shared pointer to this fixture's typed argument wrapper.
   */
  using fixture_base_argtype = std::shared_ptr<fixture_base_arg>;

  /**
   * @brief Destroy the typed fixture base.
   */
  virtual ~fixture_base() = default;

  private:
  /**
   * @brief Invoke setup with an explicit wrapped argument.
   * @param a Wrapped argument; nullptr returns nullptr.
   * @return Wrapped setup return value.
   */
  fixture_interface_returntype
  call_setup(const fixture_interface_argtype &a) override {
    if (!a) {
      return nullptr;
    }
    fixture_base_argtype argptr =
      std::dynamic_pointer_cast<fixture_base_arg>(a);
    auto retval = this->setup(argptr->value);
    return std::shared_ptr<fixture_interface_ret>(
        new fixture_base_ret(retval)
    );
  }

  /**
   * @brief Invoke setup with the bound wrapped argument.
   * @return Wrapped setup return value.
   */
  fixture_interface_returntype
  bindcall_setup() override {
    fixture_base_argtype argptr =
      std::dynamic_pointer_cast<fixture_base_arg>(arg);
    auto retval = this->setup(argptr->value);
    return std::shared_ptr<fixture_interface_ret>(
        new fixture_base_ret(retval)
    );
  }

  /**
   * @brief Invoke the fixture body with an explicit wrapped argument.
   * @param a Wrapped argument; nullptr returns nullptr.
   * @return Wrapped fixture body return value.
   */
  fixture_interface_returntype
  call(const fixture_interface_argtype &a) override {
    if (!a) {
      return nullptr;
    }
    fixture_base_argtype argptr =
      std::dynamic_pointer_cast<fixture_base_arg>(a);
    auto retval = this->operator()(argptr->value);
    return std::shared_ptr<fixture_interface_ret>(
        new fixture_base_ret(retval)
    );
  }

  /**
   * @brief Invoke the fixture body with the bound wrapped argument.
   * @return Wrapped fixture body return value.
   */
  fixture_interface_returntype
  bindcall() override {
    fixture_base_argtype argptr =
      std::dynamic_pointer_cast<fixture_base_arg>(arg);
    auto retval = this->operator()(argptr->value);
    return std::shared_ptr<fixture_interface_ret>(
        new fixture_base_ret(retval)
    );
  }

  /**
   * @brief Invoke teardown with an explicit wrapped argument.
   * @param a Wrapped argument; nullptr returns nullptr.
   * @return Wrapped teardown return value.
   */
  fixture_interface_returntype
  call_teardown(const fixture_interface_argtype &a) override {
    if (!a) {
      return nullptr;
    }
    fixture_base_argtype argptr =
      std::dynamic_pointer_cast<fixture_base_arg>(a);
    auto retval = this->teardown(argptr->value);
    return std::shared_ptr<fixture_interface_ret>(
        new fixture_base_ret(retval)
    );
  }

  /**
   * @brief Invoke teardown with the bound wrapped argument.
   * @return Wrapped teardown return value.
   */
  fixture_interface_returntype
  bindcall_teardown() override {
    fixture_base_argtype argptr =
      std::dynamic_pointer_cast<fixture_base_arg>(arg);
    auto retval = this->teardown(argptr->value);
    return std::shared_ptr<fixture_interface_ret>(
        new fixture_base_ret(retval)
    );
  }

  public:
  /**
   * @brief Bind an argument for bindcall_* operations.
   * @param a Shared wrapped argument.
   */
  void
  setarg(const fixture_interface_argtype &a) override {
    arg = std::move(const_cast<fixture_interface_argtype&>(a));
  }

  /**
   * @brief Create a wrapped argument object for this fixture type.
   * @param a Typed argument value.
   * @return Shared typed argument wrapper.
   */
  static
  fixture_base_argtype
  makearg(argtype a) {
    return std::make_shared<fixture_base_arg>(a);
  }

  /**
   * @brief Cast a type-erased return wrapper to this fixture's return wrapper.
   * @param r Type-erased return wrapper.
   * @return Shared typed return wrapper, or nullptr on type mismatch.
   */
  static
  fixture_base_returntype
  retval(const fixture_interface_returntype &r) {
    return std::dynamic_pointer_cast<fixture_base_ret>(r);
  }

  /**
   * @brief Fixture setup hook.
   * @param a Typed fixture argument.
   * @return Setup result.
   */
  virtual returntype setup(argtype a) = 0;

  /**
   * @brief Fixture body hook.
   * @param a Typed fixture argument.
   * @return Fixture body result.
   */
  virtual returntype operator()(argtype a) = 0;

  /**
   * @brief Fixture teardown hook.
   * @param a Typed fixture argument.
   * @return Teardown result.
   */
  virtual returntype teardown(argtype a) = 0;
};

#endif /* __cplusplus >= 201703L */

/**
 * @brief Abstract testcase interface.
 *
 * testsuite stores testcase objects through this base class and invokes them
 * with testcase-specific data and optional timing output pointers.
 *
 * @tparam return_type Return type produced by the testcase.
 * @tparam arg_type Argument type accepted by the testcase.
 */
template <
  typename return_type = int,
  typename arg_type = void*
>
class testcase_base {
  public:
#if __cplusplus >= 201703L

  /**
   * @brief Execute the testcase.
   * @param data Testcase argument.
   * @param ptotal Optional total duration output.
   * @param pns Optional average duration output.
   * @return Optional testcase return value.
   */
  virtual std::optional<return_type> operator()(
    arg_type data,
    std::chrono::nanoseconds *ptotal = nullptr,
    std::chrono::nanoseconds *pns = nullptr
  ) = 0;

#else /* ! __cplusplus >= 201703L */

  /**
   * @brief Execute the testcase.
   * @param data Testcase argument.
   * @param ptotal Optional total duration output.
   * @param pns Optional average duration output.
   * @return Testcase return value.
   */
  virtual return_type operator()(
    arg_type data,
    std::chrono::nanoseconds *ptotal = nullptr,
    std::chrono::nanoseconds *pns = nullptr
  ) = 0;

#endif /* __cplusplus >= 201703L */

  /**
   * @brief Destroy the testcase interface.
   */
  virtual ~testcase_base() = default;
};

/**
 * @brief Concrete testcase wrapper.
 *
 * A testcase owns no fixtures; it stores fixture pointers and invokes their
 * bound setup/teardown hooks around the wrapped function. When @p timed is
 * true, the wrapped function is executed @p runs times and timing outputs are
 * populated when provided.
 *
 * @tparam timed Whether to collect timing and repeat the body.
 * @tparam runs Number of timed iterations.
 * @tparam return_type Return type produced by the testcase.
 * @tparam arg_type Argument type accepted by the testcase.
 * @tparam fn_type Callable type with signature
 *         return_type(arg_type, std::vector<fixture_interface*>).
 */
template <
  bool timed = false,
  std::size_t runs = 1,
  typename return_type = int,
  typename arg_type = void*,
  typename fn_type =
    std::function<
      return_type (
          arg_type,
          std::vector<fixture_interface*>
      )
    >
>
class testcase final : public testcase_base<return_type, arg_type> {

  using fixture_interface_argtype
    = fixture_interface::fixture_interface_argtype;

  fn_type fn = nullptr;
  std::vector<fixture_interface*> fixtures;
#if __cplusplus < 201703L

  template <
    std::size_t runs_ = runs,
    typename std::enable_if<(runs_ > 0), int>::type = 0
  >
  std::chrono::nanoseconds avgduration(std::chrono::nanoseconds total) {
    return total / runs_;
  }

  template <
    std::size_t runs_ = runs,
    typename std::enable_if<(runs_ == 0), int>::type = 0
  >
  std::chrono::nanoseconds avgduration(std::chrono::nanoseconds total) {
    return total;
  }

#endif /* __cplusplus < 201703L */

  public:
  /**
   * @brief Construct a testcase from a callable and varargs fixture packs.
   *
   * The varargs sequence must contain @p nfixtures_ pairs of
   * fixture_interface* and fixture_interface::fixture_interface_argtype.
   * Null fixture pointers are ignored.
   *
   * @param fn_ Callable testcase body.
   * @param nfixtures_ Number of fixture pointer/argument pairs.
   */
  testcase(fn_type fn_, std::size_t nfixtures_, ...) : fn(fn_) {
    va_list fixtures_;
    va_start(fixtures_, nfixtures_);

    while (nfixtures_--) {
      fixture_interface *fixtureptr = va_arg(fixtures_, fixture_interface*);
      fixture_interface::fixture_interface_argtype fixturearg =
          va_arg(fixtures_, fixture_interface::fixture_interface_argtype);
      if (fixtureptr) {
        fixtures.push_back(fixtureptr);
        fixtureptr->setarg(fixturearg);
      }
    }

    va_end(fixtures_);
  }

  /**
   * @brief Fixture pointer plus bound argument for testcase construction.
   */
  struct fixture_pack {
    /**
     * @brief Fixture pointer to include; nullptr is ignored.
     */
    fixture_interface *fixture;

    /**
     * @brief Bound argument assigned to the fixture.
     */
    const fixture_interface::fixture_interface_argtype &arg;
  };

  /**
   * @brief Construct a testcase from a callable and fixture pack list.
   * @param fn_ Callable testcase body.
   * @param fixtures_ Fixture packs to bind and pass to the testcase body.
   */
  testcase(
      fn_type fn_,
      std::initializer_list<fixture_pack> fixtures_ = {}
  ) : fn(fn_) {

    for (auto &fixture : fixtures_) {
      if (fixture.fixture) {
        fixtures.push_back(fixture.fixture);
        fixture.fixture->setarg(fixture.arg);
      }
    }
  }

  /**
   * @brief Construct a testcase from an existing fixture pointer vector.
   * @param fn_ Callable testcase body.
   * @param fixtures_ Fixture pointers passed to the testcase body.
   */
  testcase(
      fn_type fn_,
      const std::vector<fixture_interface*> &fixtures_
  ) : fn(fn_) {

    fixtures = std::move(fixtures_);
  }

  /**
   * @brief Construct a testcase from an rvalue fixture pointer vector.
   * @param fn_ Callable testcase body.
   * @param fixtures_ Fixture pointers passed to the testcase body.
   */
  testcase(
      fn_type fn_,
      std::vector<fixture_interface*> &&fixtures_
  ) : fn(fn_) {

    fixtures = fixtures_;
  }

#if __cplusplus >= 201703L

  /**
   * @brief Run the testcase.
   *
   * Timed testcases execute the body @p runs times and optionally populate
   * total and average durations. Untimed testcases execute the body once.
   *
   * @param data Testcase argument.
   * @param ptotal Optional total duration output.
   * @param pavg Optional average duration output.
   * @return Testcase return value.
   */
  return_type run(
      arg_type data,
      std::chrono::nanoseconds *ptotal = nullptr,
      std::chrono::nanoseconds *pavg = nullptr
  ) {
    if (!fn) {
      return return_type();
    }

    for (auto &f : fixtures) {
      f->bindcall_setup();
    }

    return_type ret = return_type();

    if constexpr (timed) {
      auto start = std::chrono::high_resolution_clock::now();

      for (decltype(runs) n = 0; n < runs; ++n) {
        ret = fn(data, fixtures);
      }

      auto stop = std::chrono::high_resolution_clock::now();
      std::chrono::nanoseconds total_duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);

      if (ptotal) {
        *ptotal = total_duration;
      }

      if constexpr (runs > 0) {
        total_duration = total_duration / runs;
      }

      if (pavg) {
        *pavg = total_duration;
      }

    } else {
      ret = fn(data, fixtures);
    }

    for (auto &f : fixtures) {
      f->bindcall_teardown();
    }

    return ret;
  }

  /**
   * @brief Execute the testcase through the base-class interface.
   * @param data Testcase argument.
   * @param ptotal Optional total duration output.
   * @param pns Optional average duration output.
   * @return Optional testcase return value.
   */
  std::optional<return_type> operator()(
      arg_type data,
      std::chrono::nanoseconds *ptotal = nullptr,
      std::chrono::nanoseconds *pns = nullptr
  ) {
    return run(data, ptotal, pns);
  }

#else /* ! __cplusplus >= 201703L */

  template <
    bool timed_ = timed,
    std::size_t runs_ = runs,
    typename std::enable_if<(timed_ && runs_ > 0), int>::type = 0
  >
  /**
   * @brief Run a timed pre-C++17 testcase.
   * @param data Testcase argument.
   * @param ptotal Optional total duration output.
   * @param pavg Optional average duration output.
   * @return Last testcase return value.
   */
  return_type run(
      arg_type data,
      std::chrono::nanoseconds *ptotal = nullptr,
      std::chrono::nanoseconds *pavg = nullptr
  ) {
    if (!fn) {
      return return_type();
    }

    for (auto &f : fixtures) {
      f->bindcall_setup();
    }

    return_type ret = return_type();

    auto start = std::chrono::high_resolution_clock::now();

    for (decltype(runs) n = 0; n < runs; ++n) {
      ret = fn(data, fixtures);
    }

    auto stop = std::chrono::high_resolution_clock::now();
    std::chrono::nanoseconds total_duration =
      std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);

    if (ptotal) {
      *ptotal = total_duration;
    }

    if (pavg) {
      *pavg = avgduration<runs>(total_duration);
    }

    for (auto &f : fixtures) {
      f->bindcall_teardown();
    }

    return ret;
  }

  template <
    bool timed_ = timed,
    std::size_t runs_ = runs,
    typename std::enable_if<!timed_, int>::type = 0
  >
  /**
   * @brief Run an untimed pre-C++17 testcase.
   * @param data Testcase argument.
   * @param ptotal Ignored timing output pointer.
   * @param pavg Ignored timing output pointer.
   * @return Testcase return value.
   */
  return_type run(
      arg_type data,
      std::chrono::nanoseconds *ptotal = nullptr,
      std::chrono::nanoseconds *pavg = nullptr
  ) {
    (void)ptotal;
    (void)pavg;

    if (!fn) {
      return return_type();
    }

    for (auto &f : fixtures) {
      f->bindcall_setup();
    }

    return_type ret = fn(data, fixtures);

    for (auto &f : fixtures) {
      f->bindcall_teardown();
    }

    return ret;
  }

  /**
   * @brief Execute the testcase through the base-class interface.
   * @param data Testcase argument.
   * @param ptotal Optional total duration output.
   * @param pns Optional average duration output.
   * @return Testcase return value.
   */
  return_type operator()(
      arg_type data,
      std::chrono::nanoseconds *ptotal = nullptr,
      std::chrono::nanoseconds *pns = nullptr
  ) override {
    return run(data, ptotal, pns);
  }

#endif /* __cplusplus >= 201703L */

};

/**
 * @brief Collection of testcases and suite-level fixtures.
 *
 * A testsuite stores testcase pointers with per-testcase arguments and
 * optional suite-level fixtures. Suite fixtures run their bound setup hooks
 * before all testcases and their bound teardown hooks afterward.
 *
 * @tparam return_type Return type produced by stored testcases.
 * @tparam arg_type Argument type accepted by stored testcases.
 */
template <
  typename return_type = int,
  typename arg_type = void*
>
class testsuite {

  using testcase_type = testcase_base<return_type, arg_type>;

  std::vector<std::pair<testcase_type*, arg_type>> testcases;
  std::vector<fixture_interface*> fixtures;

  public:
  /**
   * @brief Pair of total and average testcase durations.
   */
  using times_type =
    std::pair<std::chrono::nanoseconds, std::chrono::nanoseconds>;

  /**
   * @brief Construct a testsuite from varargs testcase and fixture packs.
   *
   * The varargs sequence first contains @p ntestcases_ pairs of testcase
   * pointer and testcase argument, followed by @p nfixtures_ pairs of fixture
   * pointer and fixture argument. Null pointers are ignored.
   *
   * @param ntestcases_ Number of testcase pointer/argument pairs.
   * @param nfixtures_ Number of fixture pointer/argument pairs.
   */
  testsuite(
      std::size_t ntestcases_,
      std::size_t nfixtures_,
      ...
  ) {
    va_list args_;
    va_start(args_, nfixtures_);

    while (ntestcases_--) {
      testcase_type *tcptr = va_arg(args_, testcase_type*);
      arg_type arg = va_arg(args_, arg_type);
      if (tcptr) {
        testcases.push_back({tcptr, arg});
      }
    }

    while (nfixtures_--) {
      fixture_interface *fixtureptr = va_arg(args_, fixture_interface*);
      fixture_interface::fixture_interface_argtype fixturearg =
          va_arg(args_, fixture_interface::fixture_interface_argtype);
      if (fixtureptr) {
        fixtures.push_back(fixtureptr);
        fixtureptr->setarg(fixturearg);
      }
    }

    va_end(args_);
  }

  /**
   * @brief Construct a testsuite from varargs fixture packs only.
   *
   * The varargs sequence must contain @p nfixtures_ pairs of fixture pointer
   * and fixture argument. Null fixture pointers are ignored.
   *
   * @param nfixtures_ Number of fixture pointer/argument pairs.
   */
  testsuite(std::size_t nfixtures_, ...) {
    va_list fixtures_;
    va_start(fixtures_, nfixtures_);

    while (nfixtures_--) {
      fixture_interface *fixtureptr = va_arg(fixtures_, fixture_interface*);
      fixture_interface::fixture_interface_argtype fixturearg =
          va_arg(fixtures_, fixture_interface::fixture_interface_argtype);
      if (fixtureptr) {
        fixtures.push_back(fixtureptr);
        fixtureptr->setarg(fixturearg);
      }
    }

    va_end(fixtures_);
  }

  /**
   * @brief Fixture pointer plus bound argument for suite construction.
   */
  struct fixture_pack {
    /**
     * @brief Fixture pointer to include; nullptr is ignored.
     */
    fixture_interface *fixture;

    /**
     * @brief Bound argument assigned to the fixture.
     */
    const fixture_interface::fixture_interface_argtype &arg;
  };

  /**
   * @brief Testcase pointer plus argument for suite construction.
   */
  struct testcase_pack {
    /**
     * @brief Testcase pointer to include; nullptr is ignored.
     */
    testcase_type *testcase;

    /**
     * @brief Argument passed when this testcase is executed.
     */
    arg_type arg;
  };

  /**
   * @brief Construct a testsuite from testcase and fixture pack lists.
   * @param testcases_ Testcase packs to add.
   * @param fixtures_ Suite fixture packs to bind and add.
   */
  testsuite(
      std::initializer_list<testcase_pack> testcases_,
      std::initializer_list<fixture_pack> fixtures_
  ) {
    for (auto &testcase : testcases_) {
      if (testcase.testcase) {
        testcases.push_back({testcase.testcase, testcase.arg});
      }
    }
    for (auto &fixture : fixtures_) {
      if (fixture.fixture) {
        fixtures.push_back(fixture.fixture);
        fixture.fixture->setarg(fixture.arg);
      }
    }
  }

  /**
   * @brief Construct a testsuite from testcase packs.
   * @param testcases_ Testcase packs to add.
   */
  testsuite(std::initializer_list<testcase_pack> testcases_) {
    for (auto &testcase : testcases_) {
      if (testcase.testcase) {
        testcases.push_back({testcase.testcase, testcase.arg});
      }
    }
  }

  /**
   * @brief Construct a testsuite from fixture packs.
   * @param fixtures_ Suite fixture packs to bind and add.
   */
  testsuite(std::initializer_list<fixture_pack> fixtures_ = {}) {
    for (auto &fixture : fixtures_) {
      if (fixture.fixture) {
        fixtures.push_back(fixture.fixture);
        fixture.fixture->setarg(fixture.arg);
      }
    }
  }

  /**
   * @brief Construct a testsuite from testcase packs and fixture pointers.
   * @param testcases_ Testcase packs to add.
   * @param fixtures_ Suite fixture pointers to add.
   */
  testsuite(
      const std::vector<testcase_pack> &testcases_,
      const std::vector<fixture_interface*> &fixtures_ = {}
  ) {
    for (auto &testcase : testcases_) {
      if (testcase.testcase) {
        testcases.push_back({testcase.testcase, testcase.arg});
      }
    }
    fixtures = std::move(fixtures_);
  }

  /**
   * @brief Construct a testsuite from rvalue testcase packs and fixture
   * pointers.
   * @param testcases_ Testcase packs to add.
   * @param fixtures_ Suite fixture pointers to add.
   */
  testsuite(
      std::vector<testcase_pack> &&testcases_,
      std::vector<fixture_interface*> &&fixtures_ = {}
  ) {
    for (auto &testcase : testcases_) {
      if (testcase.testcase) {
        testcases.push_back({testcase.testcase, testcase.arg});
      }
    }
    fixtures = fixtures_;
  }

  /**
   * @brief Construct a testsuite from fixture pointers.
   * @param fixtures_ Suite fixture pointers to add.
   */
  testsuite(const std::vector<fixture_interface*> &fixtures_) {
    fixtures = std::move(fixtures_);
  }

  /**
   * @brief Construct a testsuite from rvalue fixture pointers.
   * @param fixtures_ Suite fixture pointers to add.
   */
  testsuite(std::vector<fixture_interface*> &&fixtures_) {
    fixtures = fixtures_;
  }

  /**
   * @brief Add a testcase to the suite.
   * @param tcptr Testcase pointer to add; nullptr is ignored.
   * @param arg Argument passed to the testcase when the suite runs.
   */
  void add(testcase_type *tcptr, arg_type arg) {
    if (tcptr) {
      testcases.push_back({tcptr, arg});
    }
  }

  /**
   * @brief Remove matching testcase pointer/argument pairs from the suite.
   * @param tcptr Testcase pointer to remove; nullptr is ignored.
   * @param arg Argument value paired with the testcase.
   */
  void remove(testcase_type *tcptr, arg_type arg) {
    if (tcptr) {
      testcases.erase(
          std::remove(
            testcases.begin(),
            testcases.end(),
            std::pair<testcase_type*, arg_type>(tcptr, arg)
          ),
          testcases.end()
      );
    }
  }

  /**
   * @brief Run all stored testcases.
   *
   * Suite fixtures run setup before any testcase and teardown after all
   * testcases. When @p ptimes is provided, it must contain at least one entry
   * per stored testcase.
   *
   * @param data Reserved suite-level data argument.
   * @param ptimes Optional timing output vector indexed by testcase position.
   */
  void run(arg_type data, std::vector<times_type> *ptimes = nullptr) {
    (void)data;

    for (auto &f : fixtures) {
      f->bindcall_setup();
    }

    std::vector<times_type>::size_type pos = 0;

    for (auto &testcase : testcases) {
      if (ptimes) {
        testcase.first->operator()(
            testcase.second,
            &(*ptimes)[pos].first,
            &(*ptimes)[pos].second
          );
      } else {
        testcase.first->operator()(testcase.second);
      }

      ++pos;
    }

    for (auto &f : fixtures) {
      f->bindcall_teardown();
    }
  }
};

} /* namespace testftw */

#endif /* _TESTFTW_HPP_ */
