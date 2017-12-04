# apply clang-format and check for uncommitted changes
make format-sources && \
git diff-index --quiet HEAD
