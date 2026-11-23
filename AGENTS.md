## Documentation changes

Whenever you change repository documentation, including this file, commit the documentation changes and push the commit directly to the current branch on `origin`.

## Feature implementation workflow

Use this workflow for each feature. Documentation-only changes and small fixes are exempt.

1. Create `<feature>-dev` from `master`. Commit in the smallest increments that make logical sense. Individual development commits may be unverified or incomplete.
2. When the feature is ready, ask the user to review it and wait for explicit approval. If issues surface, fix them on the same `<feature>-dev` branch in small, logical commits. Request review again after each round of fixes and repeat until the user approves.
3. After approval, create `<feature>-reflow` and reorganize the feature commits on that branch. Each commit should represent one logical change, and the sequence should be easy to review.
4. Verify that the two branch tips have identical Git tree IDs. Then open a pull request from `<feature>-reflow` to `master`.

## Agent skills

### Issue tracker

Track issues and specs in GitHub Issues. See `docs/agents/issue-tracker.md`.

### Triage labels

Use the five default triage labels defined in `docs/agents/triage-labels.md`.

### Domain docs

Misaka uses a single domain context. Before exploring the code, follow `docs/agents/domain.md`.
