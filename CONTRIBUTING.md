# Contributing to liblouis

## liblouis is moving to louis-rs

[louis-rs](https://github.com/liblouis/louis-rs) is a reimplementation of
liblouis in Rust. It uses the same braille tables and runs the liblouis
test suite. We are moving development from liblouis to louis-rs.

liblouis is now in maintenance mode. It will get four more regular
releases, the last one planned for November 2027. After that, only
louis-rs will be maintained.

### What we accept in liblouis

- **Braille tables and their tests.** Table work goes on here as before.
  louis-rs uses the same tables, so every table improvement benefits both.
- **Fixes for serious bugs**, such as crashes and security issues.
- **Features and behavior changes that are also accepted in louis-rs**
  (see below).

### Features and behavior changes go into both

During the transition, liblouis and louis-rs must stay in step. A new
feature or behavior change (a new opcode, a new API function, a change
to how existing rules translate) goes into liblouis only once it has
also been accepted in louis-rs, with the same tests. You can start in
either project; please open an issue in the other one early, so the
change can be discussed for both.

A bug fix that changes how existing tables translate is a behavior
change too. It is welcome when liblouis and louis-rs then agree on the
correct result. If louis-rs gets the case wrong as well, please report it
there too.

### The tables work in both

The braille tables are shared, so until liblouis is retired every table
must work in both liblouis and louis-rs: a table can use a feature only
when both support it. The tests that come with the tables must pass in
liblouis. louis-rs does not pass all of them yet; closing that gap is
part of the transition.

### If you develop an application that uses liblouis

Please try louis-rs now. If something you need is missing, open an issue
on louis-rs.

## Security issues

- Report security issues as normal issues in the public tracker. Pull
  requests with a fix are very welcome.
- A report must include a reproducer: an input that crashes or misbehaves
  with the latest liblouis release (e.g. a fuzzer test case), and the
  command to run it. Reports without one will be closed.
- Braille tables are trusted input, like source code for a compiler. A
  crash caused by a malformed table is a bug, not a security issue. Text
  to be translated is untrusted input.

## Contribution guidelines

Contributions that meet a certain standard are key for an efficient
review process and long-term maintainability. By providing qualitative
contributions, you show respect for reviewers' and maintainers' time
and effort, and you greatly increase the chance that your contribution
will be accepted in a timely manner.

A good contribution contains tests for all intended functional
changes, clear documentation, and accurate metadata. The combination
of these things is in fact more important than the implementation
itself. In addition, the communication around a contribution is
expected to be to the point and without fluff. We prefer to talk to
humans, not LLMs.

This guide explains why these things are so important, and how they can
be achieved practically.

## Tests: show us what your change does

### Why tests matter in Liblouis

In Liblouis, we consider tests to be the authoritative specification
of behavior. Tables and C code may change. Tests describe what must
remain true.

Tests enable test-driven development, make it possible to review
changes reliably, guard against regressions, allow for refactoring and
rewrites while ensuring stability, and preserve knowledge over
time. They have both a verification and a documentation role.

#### Tests are useful for contributors

Tests ensure that things keep working in the future, and allow future
contributors to change the implementation without accidentally
changing behavior.

Liblouis tables are not "owned" by a single person or instance. This
would not be manageable, as table authors inevitably come and
go. Instead, everyone is allowed to contribute. This is the strength
of Liblouis, but also requires great care. Without good tests, your
hard work may be lost when the next contributor that comes along is
not careful enough.

On the other hand, tests are not holy either. If they are not backed
by an official specification, your tests may be invalidated by future
contributors who provide more substantiated test data.

Another benefit of tests is that they allow for test-driven
development. This means that you start with writing test cases,
confirm that they initially fail, observe how the results change as
you make modifications to the table, and work on the table until all
test cases pass. This offers contributors a great development workflow
and can make them more confident. This is not just about expertise,
but about combinatorial complexity, interactions between rules, and
unintended side effects. The test-driven development approach also
helps to avoid unnecessary or redundant rules.

Finally, tests encourage collaboration from non-experts. They make it
possible for anyone, even someone who is not familiar with the braille
code in question, to improve a table, based on the expertise of
others.

#### Tests are useful for reviewers

When a pull request changes behavior, reviewers, who are often not
familiar with the braille code, need to know:

- what behavior is intended
- whether existing behavior is preserved

Tests answer these questions more reliably than prose or code
inspection.

A lack of tests make a PR much harder to review, which is critical
when reviewers have a lot on their plate. Tests are not just a
convenience, they are the only scalable review mechanism.

Tests also make it more apparent when there are conflicting needs for
different user groups. With good tests, a contribution that fixes a
defect for one group may not break a feature of another group
unnoticed. Conflicts can be immediately detected this way, and the
decision makers can set up a conversation between users to resolve the
issue.

#### Tests are useful for maintainers

Cleaning up code is often necessary to keep the project maintainable
and future-proof. This matters for both table maintainers and higher
level maintainers. Tests allow:

- safely reorganizing tables,
- safely refactoring C code,
- or even rewriting (parts of) the software from scratch.

Without tests, maintainers will more easily avoid refactoring, and
become stuck with messy and outdated code.

Last but not least, tests help to transfer and preserve knowledge
about expected behavior over time. The documentation function that
tests have for reviewers, is all the more important for maintainers
(who are typically dealing with the long term) and future
contributors. Over time, table contributors come and go ("drive-by"
contributions are a common thing), and braille code specifications
evolve, so without tests, knowledge about why a table behaves the way
it does is easily lost. Externalizing knowledge and being able to
understand the intent of code that was written in the past, are
key. Having a clean git history and being able to easily match code
with corresponding test cases greatly helps with this.

### The Liblouis test suite

The Liblouis test suite consists of a collection of test files, in the
folder `tests/braille-specs`, that describe the behavior of
translation tables through a set of test cases - strings with their
corresponding expected translations - in a special YAML-based
syntax. The tests are automatically executed as part of the build, and
in pull requests checks on Github. Details about the YAML format can
be found in [the user
manual](https://liblouis.io/documentation/liblouis/Automated-Testing-of-Translation-Tables).

Ultimately, the goal of the test suite is to strive for perfect
coverage, while also watching over the size (minimizing redundancy).

### How to add tests for Liblouis

Below is a step-by-step guide for adding or updating YAML tests. If
you need help with writing tests, please don't hesitate to ask. Also
note that using the YAML format is not mandatory. As long as the test
data is there, in whatever form, and it is correct, we are happy.

- Find the test file that contains existing test cases for your
  braille code, or create a new file if needed. Test files are located
  in the `tests/braille-specs` folder, and are sorted by language tag.

  Below is a simple template for a test file. The metadata section
  under `table:` is used to select the table and should match your
  table metadata.

  ```yaml
  # Tests for Quenya braille
  #
  # Copyright © 2026 by J. R. R. Tolkien <glossopoeia@tolkien.info>
  #
  # Copying and distribution of this file, with or without modification,
  # are permitted in any medium without royalty provided the copyright
  # notice and this notice are preserved. This file is offered as-is,
  # without any warranty.
  #
  # [optionally insert a short section with documentation and a link to the spec]
  # ---------------------------------------------------------------------
  
  display: unicode.dis
  table:
    language: qya
    type: literary
  flags:
    testmode: bothDirections
  tests:
    - - "a test"
      - "⠁⠀⠞⠑⠎⠞"
    - - "another test"
      - "⠁⠝⠕⠞⠓⠑⠗⠀⠞⠑⠎⠞"
  ```

  More information about YAML files can be found in [the user
  manual](https://liblouis.io/documentation/liblouis/Automated-Testing-of-Translation-Tables).

- Make sure you have test cases that clearly express the expected
  behavior. Prefer small, focused tests over large combined ones.
  
  If your fix already has a corresponding test case, it should
  normally have been marked with a `xfail` flag, meaning it is a test
  that is expected to fail, i.e. upon running it will report a
  (special kind of) success when the test actually fails.
  
  If your fix does not already have a corresponding test case, you
  need to create a new one. Use the `xfail` flag to describe intended
  behavior that is not yet implemented. A minimal test case looks like
  this (more info in [the user
  manual](https://liblouis.io/documentation/liblouis/Automated-Testing-of-Translation-Tables)):
  
  ```yaml
    - - "a test"
      - "⠁⠀⠞⠑⠎⠞"
      - xfail: |
          Does not work because of the bug described in
          https://github.com/liblouis/liblouis/pull/666
  ```

- Optionally: run the test locally, using the `lou_checkyaml`
  tool. For more information, please refer to [the user
  manual](https://liblouis.io/documentation/liblouis/lou_005fcheckyaml.html)
  and the `HACKING` file.

  Note that the test suite is also run as a pull request check on
  Github.

- Recommended: if you are able to run the test locally, run it both
  before and after you change the table, and confirm it changes from
  failing to passing.

### Checklist for pull requests

To summarize: before you send us your contribution, to save reviewers
time, make sure these boxes are ticked:

- [ ] I added or updated tests covering all intended functional
  changes (including bug fixes). If the contribution contains changes
  that are not covered by tests, at least mention this and explain
  why.

Strongly recommended:

- [ ] Tests pay special attention to edge cases, interactions between
  rules, and potential side effects
- [ ] Where intended behavior is known but not yet fully implemented,
  tests are marked as such ("expected failures")
- [ ] I have tested my changes locally
- [ ] The test suite passes for every commit

## Table metadata: say what your table is

### Why metadata is important

Liblouis tables are not a loose collection of files, but form a
catalog. Each table has its own unique scope. For identification,
categorization and discoverability of tables, accurate and complete
metadata is essential.

Any documentation that is included in a table is valuable, but
metadata is especially important because of its machine-readable
nature.

Like tests, metadata and documentation outlive individual
contributors, reduce reliance on personal expertise, and make
maintenance and review possible.

### Liblouis' metadata syntax and standard metadata tags

The metadata section of a table must be able to answer, among other
things:

- Which braille code does this table implement?
- Which revision of the specification?
- What is the intended scope of this table?

Liblouis metadata are special machine-readable comments. The syntax is
described in [the user
manual](https://liblouis.io/documentation/liblouis/Table-Metadata).

A list of standard metadata tags is documented [in the
wiki](https://github.com/liblouis/liblouis/wiki/Table-discovery-based-on-table-metadata).

The minimal required metadata for a table consists of a `language`
field and a `type` field, and depending on which other tables exist
with the same `language` and `type` metadata, any other fields that
are needed to distinguish the table from those other tables.

## Use of AI in communication: three rules

We value human connection, clarity, and our maintainers' limited
time. While AI tools can help brainstorm or refine ideas, their
unedited output often leads to unnecessary verbosity, generic
phrasing, and a high review burden. To maintain a high signal-to-noise
ratio in our community, we try to follow these rules:

1. **The "Value vs. review time" standard**

   Every comment, issue, and pull request description must provide
   clear value. AI tools are naturally verbose. Before submitting text
   coming from an AI, ask yourself: "Can I say this in half the
   words?". Strip out automated apologies, introductory pleasantries,
   and concluding summaries. Avoid text that simply states the obvious
   or repeats what the code already shows. If your submission consists
   of unedited text generated in seconds by an AI, requiring our
   maintainers to spend minutes reading through filler and fluff, it
   will be closed.

2. **Mandatory transparency**

   If any part of your text or code was generated or heavily rephrased
   by an AI tool, you are expected to communicate this. For code, add
   an `Assisted-by:` trailer to the commit message that names the tool
   and the model, as the Linux kernel asks
   ([coding assistants](https://docs.kernel.org/process/coding-assistants.html)):

   ```
   Assisted-by: Claude:claude-opus-5-5
   ```

3. **Full human responsibility**

   You are 100% accountable for what you post. If an AI hallucinates a
   fact, references a non-existent feature, or injects incorrect
   technical reasoning into a discussion, the blame lies with the
   human contributor who posted it, not the tool. Review and
   understand every single word before sharing it with the community.

 
