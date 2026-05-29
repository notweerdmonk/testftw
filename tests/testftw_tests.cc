#include <testftw.hpp>

#include <cassert>
#include <memory>
#include <vector>

struct test_context {
  int direct_count;
  int timed_count;
  int suite_first_count;
  int suite_second_count;
  std::vector<int> events;

  test_context() :
    direct_count(0),
    timed_count(0),
    suite_first_count(0),
    suite_second_count(0),
    events() { }

  void reset_events() {
    events.clear();
  }
};

class int_fixture : public fixture_base<int, int> {
  std::vector<int> *events;

  public:
  explicit int_fixture(std::vector<int> *events_) : events(events_) { }

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

class ptr_fixture : public fixture_base<int, void*> {
  std::vector<int> *events;

  public:
  explicit ptr_fixture(std::vector<int> *events_) : events(events_) { }

  int setup(void *value) override {
    events->push_back(value ? 101 : 100);
    return value ? 1 : 0;
  }

  int operator()(void *value) override {
    events->push_back(value ? 201 : 200);
    return value ? 1 : 0;
  }

  int teardown(void *value) override {
    events->push_back(value ? 301 : 300);
    return value ? 1 : 0;
  }
};

class base_type {
  public:
  virtual ~base_type() { }
};

class derived_type : public base_type { };
class other_type : public base_type { };

static void expect_events(
    const std::vector<int> &actual,
    const std::vector<int> &expected
) {
  assert(actual == expected);
}

static int test_unique_ptr_dynamic_cast(
    test_context*,
    std::vector<fixture_interface*>
) {
  std::unique_ptr<base_type> success(new derived_type());
  std::unique_ptr<derived_type> casted =
    unique_ptr_dynamic_cast<derived_type>(success);
  assert(casted.get() != nullptr);
  assert(success.get() == nullptr);

  std::unique_ptr<base_type> failure(new other_type());
  std::unique_ptr<derived_type> not_casted =
    unique_ptr_dynamic_cast<derived_type>(failure);
  assert(not_casted.get() == nullptr);
  assert(failure.get() != nullptr);

  return 0;
}

static int test_benchmark_macro(test_context*, std::vector<fixture_interface*>) {
  int counter = 0;
  std::chrono::nanoseconds total;

  BENCHMARK_START(5)
    ++counter;
  BENCHMARK_END(total)

  assert(counter == 5);
  assert(total.count() >= 0);
  return 0;
}

static int test_fixture_direct_calls(
    test_context *ctx,
    std::vector<fixture_interface*>
) {
  ctx->reset_events();
  int_fixture fixture(&ctx->events);
  fixture_interface &iface = fixture;

#if __cplusplus >= 201703L
  assert(std::any_cast<int>(iface.call_setup(std::any(7))) == 7);
  assert(std::any_cast<int>(iface.call(std::any(8))) == 8);
  assert(std::any_cast<int>(iface.call_teardown(std::any(9))) == 9);
#else
  int_fixture::fixture_base_returntype setup_ret =
    int_fixture::retval(iface.call_setup(int_fixture::makearg(7)));
  int_fixture::fixture_base_returntype call_ret =
    int_fixture::retval(iface.call(int_fixture::makearg(8)));
  int_fixture::fixture_base_returntype teardown_ret =
    int_fixture::retval(iface.call_teardown(int_fixture::makearg(9)));
  assert(setup_ret->value == 7);
  assert(call_ret->value == 8);
  assert(teardown_ret->value == 9);
#endif

  expect_events(ctx->events, std::vector<int>{1007, 2008, 3009});
  return 0;
}

static int test_fixture_bound_calls(
    test_context *ctx,
    std::vector<fixture_interface*>
) {
  ctx->reset_events();
  int_fixture fixture(&ctx->events);
  fixture_interface &iface = fixture;
  fixture.setarg(int_fixture::makearg(4));

#if __cplusplus >= 201703L
  assert(std::any_cast<int>(iface.bindcall_setup()) == 4);
  assert(std::any_cast<int>(iface.bindcall()) == 4);
  assert(std::any_cast<int>(iface.bindcall_teardown()) == 4);
#else
  int_fixture::fixture_base_returntype setup_ret =
    int_fixture::retval(iface.bindcall_setup());
  int_fixture::fixture_base_returntype call_ret =
    int_fixture::retval(iface.bindcall());
  int_fixture::fixture_base_returntype teardown_ret =
    int_fixture::retval(iface.bindcall_teardown());
  assert(setup_ret->value == 4);
  assert(call_ret->value == 4);
  assert(teardown_ret->value == 4);
#endif

  expect_events(ctx->events, std::vector<int>{1004, 2004, 3004});
  return 0;
}

static int testcase_body_returns_77(
    int data,
    std::vector<fixture_interface*> fixtures
) {
  assert(data == 9);
  assert(fixtures.size() == 1);
  return 77;
}

static int test_testcase_bound_lifecycle(
    test_context *ctx,
    std::vector<fixture_interface*>
) {
  ctx->reset_events();
  int_fixture fixture(&ctx->events);
  fixture_interface::fixture_interface_argtype arg = int_fixture::makearg(4);
  testcase<false, 1, int, int> tc(testcase_body_returns_77, {{&fixture, arg}});

  int ret = tc.run(9);
  assert(ret == 77);
  expect_events(ctx->events, std::vector<int>{1004, 3004});
  return 0;
}

static int null_fixture_body(int, std::vector<fixture_interface*> fixtures) {
  assert(fixtures.empty());
  return 0;
}

static int test_testcase_null_fixtures(
    test_context*,
    std::vector<fixture_interface*>
) {
  fixture_interface::fixture_interface_argtype arg;

  testcase<false, 1, int, int> from_pack(null_fixture_body, {{nullptr, arg}});
  assert(from_pack.run(1) == 0);

  testcase<false, 1, int, int> from_varargs(
      null_fixture_body,
      1,
      static_cast<fixture_interface*>(nullptr),
      arg
  );
  assert(from_varargs.run(1) == 0);

  return 0;
}

static int timed_body(test_context *ctx, std::vector<fixture_interface*>) {
  ++ctx->timed_count;
  return ctx->timed_count;
}

static int test_timed_testcase(test_context *ctx, std::vector<fixture_interface*>) {
  std::chrono::nanoseconds total;
  std::chrono::nanoseconds average;
  testcase<true, 3, int, test_context*> tc(timed_body);

  ctx->timed_count = 0;
  int ret = tc.run(ctx, &total, &average);

  assert(ctx->timed_count == 3);
  assert(ret == 3);
  assert(total.count() >= 0);
  assert(average.count() >= 0);
  assert(average <= total);
  return 0;
}

static int ptr_fixture_body(void*, std::vector<fixture_interface*> fixtures) {
  assert(fixtures.size() == 1);
  return 0;
}

static int test_pointer_fixture_null_arg(
    test_context *ctx,
    std::vector<fixture_interface*>
) {
  ctx->reset_events();
  ptr_fixture fixture(&ctx->events);
  fixture_interface::fixture_interface_argtype arg = ptr_fixture::makearg(nullptr);
  testcase<false, 1, int, void*> tc(ptr_fixture_body, {{&fixture, arg}});

  assert(tc.run(reinterpret_cast<void*>(0x1)) == 0);
  expect_events(ctx->events, std::vector<int>{100, 300});
  return 0;
}

static int suite_first_body(
    test_context *ctx,
    std::vector<fixture_interface*>
) {
  ++ctx->suite_first_count;
  return 0;
}

static int suite_second_body(
    test_context *ctx,
    std::vector<fixture_interface*>
) {
  ++ctx->suite_second_count;
  return 0;
}

static int test_testsuite_add_remove(
    test_context *ctx,
    std::vector<fixture_interface*>
) {
  testcase<false, 1, int, test_context*> first(suite_first_body);
  testcase<false, 1, int, test_context*> second(suite_second_body);
  testsuite<int, test_context*> suite;

  ctx->suite_first_count = 0;
  ctx->suite_second_count = 0;

  suite.add(&first, ctx);
  suite.add(&second, ctx);
  suite.remove(&first, ctx);
  suite.run(ctx);

  assert(ctx->suite_first_count == 0);
  assert(ctx->suite_second_count == 1);
  return 0;
}

static int suite_lifecycle_body(
    test_context *ctx,
    std::vector<fixture_interface*> fixtures
) {
  assert(fixtures.empty());
  ctx->events.push_back(4000);
  return 0;
}

static int test_testsuite_bound_lifecycle(
    test_context *ctx,
    std::vector<fixture_interface*>
) {
  ctx->reset_events();
  int_fixture fixture(&ctx->events);
  fixture_interface::fixture_interface_argtype fixture_arg =
    int_fixture::makearg(6);
  testcase<false, 1, int, test_context*> tc(suite_lifecycle_body);
  testsuite<int, test_context*> suite(
      {{&tc, ctx}},
      {{&fixture, fixture_arg}}
  );

  suite.run(ctx);
  expect_events(ctx->events, std::vector<int>{1006, 4000, 3006});
  return 0;
}

int main() {
  test_context ctx;

  testcase<false, 1, int, test_context*> t1(test_unique_ptr_dynamic_cast);
  testcase<false, 1, int, test_context*> t2(test_benchmark_macro);
  testcase<false, 1, int, test_context*> t3(test_fixture_direct_calls);
  testcase<false, 1, int, test_context*> t4(test_fixture_bound_calls);
  testcase<false, 1, int, test_context*> t5(test_testcase_bound_lifecycle);
  testcase<false, 1, int, test_context*> t6(test_testcase_null_fixtures);
  testcase<false, 1, int, test_context*> t7(test_timed_testcase);
  testcase<false, 1, int, test_context*> t8(test_pointer_fixture_null_arg);
  testcase<false, 1, int, test_context*> t9(test_testsuite_add_remove);
  testcase<false, 1, int, test_context*> t10(test_testsuite_bound_lifecycle);

  testsuite<int, test_context*> suite;
  suite.add(&t1, &ctx);
  suite.add(&t2, &ctx);
  suite.add(&t3, &ctx);
  suite.add(&t4, &ctx);
  suite.add(&t5, &ctx);
  suite.add(&t6, &ctx);
  suite.add(&t7, &ctx);
  suite.add(&t8, &ctx);
  suite.add(&t9, &ctx);
  suite.add(&t10, &ctx);
  suite.run(&ctx);

  return 0;
}
