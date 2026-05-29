# Contributing to testftw

Thank you for your interest in contributing to testftw!

## Development Setup

1. Clone the repository:
   ```bash
   git clone https://github.com/notweerdmonk/testftw.git
   cd testftw
   ```

2. Run the test suite:
   ```bash
   cd tests
   make check
   ```

3. Run compatibility checks:
   ```bash
   make compat
   ```

## Code Style

- Two-space indentation in class bodies and control blocks
- Opening braces on the same line for functions and conditionals
- `lower_snake_case` for identifiers
- Template parameter names should be descriptive (e.g., `return_type`, `arg_type`)
- Keep includes minimal and sorted

## Testing Requirements

- All new functionality must include test cases in `tests/testftw_tests.cc`
- Tests must pass across all supported C++ standards (`make compat`)
- New code should pass sanitizer checks (`make asan`)
- Use C++ `assert` checks inside test bodies

## Commit Guidelines

- Use imperative mood: "Add feature" not "Added feature"
- One concise sentence per commit
- Capitalize first word
- Examples:
  - `Add fixture lifecycle test.`
  - `Fix pre-C++17 teardown dispatch.`
  - `Update documentation for testcase constructor.`

## Pull Requests

1. Create a feature branch from `main`
2. Make your changes following the guidelines above
3. Ensure all tests pass (`make compat` and `make asan`)
4. Submit a pull request with:
   - Description of the API or behavior change
   - List of C++ standards tested
   - Any compatibility tradeoffs
   - Command output summaries if relevant

## Reporting Issues

- Use GitHub Issues for bug reports and feature requests
- Include minimal reproduction steps
- Mention your compiler version and C++ standard

## License

By contributing, you agree that your contributions will be licensed under the MIT License.
