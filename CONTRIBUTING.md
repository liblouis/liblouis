# Contributing to liblouis

## liblouis is moving to louis-rs

[louis-rs](https://github.com/liblouis/louis-rs) is a reimplementation of
liblouis in Rust. It uses the same braille tables and passes the liblouis
test suite. We are moving development from liblouis to louis-rs.

liblouis is now in maintenance mode. It will get four more regular
releases, the last one planned for November 2027. After that, only
louis-rs will be maintained.

### What we accept in liblouis

- **Braille tables and their tests.** Table work goes on here as before.
  louis-rs uses the same tables, so every table improvement benefits both.
- **Fixes for serious bugs**, such as crashes and security issues.

### What goes to louis-rs

- **New features**: new opcodes, new API functions, changes to translation
  behavior. Please open these against louis-rs. We will not add new
  features to liblouis.

### If you develop an application that uses liblouis

Please try louis-rs now. If something you need is missing, open an issue
on louis-rs while there is still time to add it.

## Security issues

- Report security issues as normal issues in the public tracker. Pull
  requests with a fix are very welcome.
- A report must include a reproducer: an input that crashes or misbehaves
  with the latest liblouis release (e.g. a fuzzer test case), and the
  command to run it. Reports without one will be closed.
- Braille tables are trusted input, like source code for a compiler. A
  crash caused by a malformed table is a bug, not a security issue. Text
  to be translated is untrusted input.

## Tests: show us what your change does

Tests are the specification of a table's behavior. Reviewers rarely know
the braille code, so they review your tests, not your rules.

- Add or update tests for every change in behavior, including bug fixes.
  If something isn't covered, say why.
- Tests are in `tests/braille-specs`, one file per braille code, sorted by
  language tag. The format is described in [the
  manual](https://liblouis.io/documentation/liblouis/Automated-Testing-of-Translation-Tables).
- Prefer small, focused tests. Cover edge cases and interactions between
  rules.
- Mark known but unimplemented behavior with `xfail`.
- If you can, run the test with `lou_checkyaml` before and after your
  change, and check that it goes from failing to passing.

A minimal test file:

```yaml
display: unicode.dis
table:
  language: qya
  type: literary
flags:
  testmode: bothDirections
tests:
  - - "a test"
    - "⠁⠀⠞⠑⠎⠞"
  - - "not yet"
    - "⠝⠕⠞⠀⠽⠑⠞"
    - xfail: Does not work because of #666
```

Not sure how to write a test? Ask, and we'll help.

## Table metadata: say what your table is

Every table needs [metadata](https://liblouis.io/documentation/liblouis/Table-Metadata)
that says which braille code it implements, which revision, and its
scope. At least `language` and `type`, plus whatever else distinguishes
it from other tables with the same `language` and `type`. Use the
[standard tags](https://github.com/liblouis/liblouis/wiki/Table-discovery-based-on-table-metadata),
and link to the official specification if there is one.

## Using AI: fine, with three rules

- **Be brief.** Don't post unedited AI text. If a maintainer needs longer
  to read it than you needed to generate it, it will be closed.
- **Say so.** If AI wrote or rephrased any part of your text or code, say
  so. For code, add an `Assisted-by:` trailer to the commit message, as
  the [Linux kernel](https://docs.kernel.org/process/coding-assistants.html)
  asks: `Assisted-by: Claude:claude-opus-5-5`.
- **You are responsible.** You answer for every word and line you post,
  not the tool.
