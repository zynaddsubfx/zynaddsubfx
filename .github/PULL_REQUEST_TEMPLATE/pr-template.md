# Description

(Write your PR description here)

# Checks

## Human checks (please check those!):

- [ ] 01. Is the PR atomic? (or could parts of this PR be split into another PR)
- [ ] 02. Are PRs relying on or blocking this PR marked and linked in both PRs descriptions
- [ ] 03. Are known issues that this PR solves linked in both descriptions (PR and issue)?
- [ ] 04. Is there sufficient user documentation in the `doc/` folder?
- [ ] 05. Is the code realtime safe? (e.g. no `new`/`delete`, no other syscalls like `sleep`/file handling/mutexes, no prints in non-error code pathways, ...)
- [ ] 06. Is aliasing properly avoided?
- [ ] 07. Are clicks/pops/discontinuities successfully avoided?
- [ ] 08. Are all new parameters well-mapped and behave as expected across their provided range?
- [ ] 09. Is divide by zero completely impossible with these changes?
- [ ] 10. Are all magic numbers clearly defined and centralized (local ones at the top of the file, global ones in src/globals.h)?
- [ ] 11. Are all nontrivial formulae or constants explained, with appropriate references provided?
- [ ] 12. Do nontrivial class variables have appropriate Doxygen comments?
- [ ] 13. Code: Are grammar and spelling correct? (Note: spell checks are only done for `doc/`)
- [ ] 14. Documentation: Is the grammar correct? (Note: spell checks are done by CI)
- [ ] 15. Are all function parameters `const` where possible?
- [ ] 16. Are all class members `const` where possible?
- [ ] 17. Is the use of `memmove` avoided in favor of more efficient alternatives like a ringbuffer when suitable?
- [ ] 18. Is there sufficient coverage with unit tests for this functionality?

## CI checks:

- No trailing whitespace?
- Are all included headers necessary?
- No compiler warnings?
- No spell errors in documentation? (new words must be appended to `doc/wordlist.txt`)
