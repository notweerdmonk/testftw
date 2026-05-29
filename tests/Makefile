CXX ?= g++
CXX_VERSION ?= c++23
CXXFLAGS ?= -g
WARNINGS ?= -Wall -Wextra -Werror -pedantic
CPPFLAGS ?= -I..

TEST_BIN = testftw_tests

.PHONY: all check asan compat clean

all: $(TEST_BIN)

$(TEST_BIN): testftw_tests.cc ../testftw.hpp
	$(CXX) -std=$(CXX_VERSION) $(CPPFLAGS) $(CXXFLAGS) $(WARNINGS) -o $@ testftw_tests.cc

check: $(TEST_BIN)
	./$(TEST_BIN)

asan:
	$(MAKE) clean check CXXFLAGS='-g -fsanitize=address,undefined' CXX_VERSION=$(CXX_VERSION)

compat:
	$(MAKE) clean check CXX_VERSION=c++11
	$(MAKE) clean check CXX_VERSION=c++14
	$(MAKE) clean check CXX_VERSION=c++17
	$(MAKE) clean check CXX_VERSION=c++23

clean:
	rm -f $(TEST_BIN)
