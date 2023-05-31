# Security Policy

## Reporting a Vulnerability

To report a security issue, please email the maintainers (see
[AUTHORS](https://github.com/liblouis/liblouis/blob/master/AUTHORS))
with a description of the issue, the steps you took to create the
issue, affected versions, and, if known, mitigations for the issue.

Our vulnerability management team will respond within 5 working days
of your email. If the issue is confirmed as a vulnerability, we will
open a Security Advisory. This project follows a 90 day disclosure
timeline.

## Supported Versions

Security updates are only provided for the latest version of liblouis.

## Critical issues

Security problems that originate from loading of liblouis tables are
not really considered a security problem. Typically tables are not
loaded from untrusted sources.

On the other hand, issues found in the translation code, which deals
with untrusted input, are indeed critical and need to be reported.

Issues reported by the [input fuzzer][1] are most likely critical and
we'd appreciate reports. However, only some of the issues reported by
the [table loading fuzzer][2] are critical, as this fuzzer tests both
table loading and translation. In particular only *fuzz_translate* and
*fuzz_translate_generic* issues are critical. Issues of the
*table_fuzzer* kind are not critical and have lower priority.

[1]: https://github.com/liblouis/liblouis/blob/master/.github/workflows/fuzzing.yml
[2]: https://github.com/liblouis/liblouis/blob/master/.github/workflows/cifuzz.yml
