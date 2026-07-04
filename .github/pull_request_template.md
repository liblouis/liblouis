## Description

Describe what this pull request changes and why. If it fixes a bug,
describe the observed behavior and the expected behavior. Include any
information that helps reviewing this change: links to specs, issues,
or prior discussions.

## Checklist

### Tests

If this pull request modifies a table in a way that it changes the
behavior, these changes must be reflected in the tests. Liblouis
relies on tests as the contract for behavior. They enable test-driven
development, make it possible to review changes reliably, guard
against regressions, allow for refactoring and rewrites while ensuring
stability, and preserve knowledge over time.

Read more about why tests are important for the Liblouis project, and
how to write tests:
https://github.com/liblouis/liblouis/wiki/Contribution-guidelines#tests

Please check one of the following:

- [ ] I added or updated tests covering all intended functional
      changes (including bug fixes)
- [ ] Some functional changes do not require tests (please explain
      below)

Strongly recommended:

- [ ] Tests pay special attention to edge cases, interactions between
      rules, and potential side effects
- [ ] Where intended behavior is known but not yet fully implemented,
      tests are marked as such ("expected failures")
- [ ] I have tested my changes locally
- [ ] The test suite passes for every commit

**If this pull request contains changes that are not covered by tests,
explain why**: ___

If you are unsure how or where to add tests, that's okay — **please
ask**, and we'll gladly help.

### Table metadata and documentation

If this pull request adds a new table or updates a table to a new
revision of the braille code, metadata and documentation must be
included or updated accordingly. Table metadata and documentation are
required for identification and discoverability of tables.

Read more about Liblouis' metadata syntax, standard metadata tags and
minimal required metadata:
https://github.com/liblouis/liblouis/wiki/Contribution-guidelines#table-metadata

Required checks:

- [ ] Table metadata and documentation is complete and up to date

Strongly recommended:

- [ ] The table has links to the official specification of the
      implemented braille code, if available

### Legal and project hygiene

- [ ] License headers are included where required
- [ ] Copyright notices are up-to-date (including correct years)
- [ ] Whenever table content was copy-pasted from other tables, this
      was done to fulfil a specific functional requirement
- [ ] No table content is commented out without a clear explanation
