# A set of recipes to interact with Github pull requests

milestone := "3.38"
maintainers := "egli bertfrees"

# Show available recipes
[private]
help:
    @just --list

# all open PRs for this release
all:
    gh pr list --search "milestone:{{milestone}}"

# PRs ready to merge: both maintainers approved (or one approved when the other is author)
ready-to-merge:
    gh pr list --search "milestone:{{milestone}} review:approved -label:\"needs fixup\"" \
      --json number,title,author,reviews \
      | jq -r '.[] | . as $pr | ($pr.reviews | map(select(.state == "APPROVED")) | map(.author.login)) as $approvers | select(($approvers | (contains(["egli"]) and contains(["bertfrees"]))) or ($pr.author.login == "bertfrees" and ($approvers | contains(["egli"]))) or ($pr.author.login == "egli" and ($approvers | contains(["bertfrees"])))) | "#\(.number)\t\(.title)"'

# View a PR
view pr:
    gh pr view {{pr}} --web

# Auto-merge a PR using rebase strategy when CI passes
merge pr:
    gh pr merge {{pr}} --auto --rebase

# Rebase a PR
rebase pr:
    gh pr update-branch {{pr}} --rebase

# PRs that are assigned to me
assigned-to-me:
    gh pr list --search "milestone:{{milestone}} assignee:@me"

# PRs that need review (no approval yet, excluding own PRs)
review-required:
    gh pr list --search "milestone:{{milestone}} review:required -author:@me"

# PRs waiting on contributor to address feedback
waiting-on-contributor:
    gh pr list --search "milestone:{{milestone}} review:changes_requested"

# Approved PRs that need branch cleanup before merging
needs-fixup:
    gh pr list --label "needs fixup" --search "milestone:{{milestone}} review:approved"

# PRs approved by egli but not yet by bertfrees
waiting-for-bert:
    gh pr list --search "milestone:{{milestone}} review:approved reviewed-by:egli -reviewed-by:bertfrees"

# Merged PRs that still need a NEWS entry
needs-news:
    gh pr list --state merged --label "needs news" --search "milestone:{{milestone}}"
