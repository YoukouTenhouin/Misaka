# GitHub Issues

Issues and specs live in `YoukouTenhouin/Misaka`. Run `gh` from the clone.

| Task | Command |
| --- | --- |
| Publish a ticket or spec | `gh issue create --title "<title>" --body-file <path>` |
| Read a ticket and its discussion | `gh issue view <number> --json title,body,labels,comments` |
| Find tickets by triage label | `gh issue list --state open --label "<label>"` |
| Add a comment | `gh issue comment <number> --body-file <path>` |
| Change triage state | `gh issue edit <number> --remove-label "<old>" --add-label "<new>"` |
| Close a ticket with a reason | `gh issue close <number> --comment "<reason>"` |

**PRs as a request surface: no.**

## Wayfinder

Keep each map in an issue labelled `wayfinder:map`. Link child tickets as
GitHub sub-issues and label each by type (`wayfinder:research`,
`wayfinder:prototype`, `wayfinder:grilling`, or `wayfinder:task`).
If sub-issues are unavailable, use a task list in the map and put
`Part of #<map>` at the top of each child.

Record blockers with GitHub issue dependencies. If dependencies are
unavailable, put `Blocked by: #<number>` at the top of the child ticket.
In map order, claim the first open, unassigned child whose blockers are
all closed. Assign it to yourself with `gh issue edit <number> --add-assignee @me`.
When finished, comment with the result, close the child, and add a
summary and link to the map’s decisions.
