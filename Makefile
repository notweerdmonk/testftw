TEST_DIR = tests

.PHONY: all check asan compat clean $(TEST_DIR)

all: $(TEST_DIR)

$(TEST_DIR):
	$(MAKE) -C $(TEST_DIR)

check:
	$(MAKE) -C $(TEST_DIR) $@

asan:
	$(MAKE) -C $(TEST_DIR) $@

compat:
	$(MAKE) -C $(TEST_DIR) $@

clean:
	$(MAKE) -C $(TEST_DIR) $@
