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
not really considered a security problem, as typically tables are not
loaded from untrusted sources.

On the other hand, issues found in the translation code, which deals
with untrusted input, are indeed critical and need to be reported.

Hence, issues reported by `fuzz_translate` and
`fuzz_translate_generic` are most likely critical and we'd appreciate
reports. Issues reported by `table_fuzzer` are not critical and have
lower priority.
