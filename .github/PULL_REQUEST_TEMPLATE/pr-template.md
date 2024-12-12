# Description

(Write your PR description here)

# Checks

## Human checks (please check those!):

- [ ] 01. Could parts of this PR be split into another PR (while keeping the PRs "atomic")?
- [ ] 02. Does this PR depend on another PR or does it block another PR?
- [ ] 03. Does this PR solve a known issue?
- [ ] 04. Is there sufficient user documentation in the `doc/` folder?
- [ ] 05. Is the code realtime safe? (e.g. no `new`/`delete`, no other syscalls like `sleep`/file handling/mutexes, no prints in non-error code pathways, ...)
- [ ] 06. Is aliasing avoided?
- [ ] 07. Are clicks/pops/discontinuities avoided?
- [ ] 08. Are any new parameters well mapped over their provided range?
- [ ] 09. Does the code define all magic numbers at a common place (e.g. on top of the file, or for constants which are used in several places, inside `src/globals.h`)?
- [ ] 10. Are nontrivial formulae or constants explained and the source is referenced?
- [ ] 11. Do nontrivial class variables have a doxygen comment?
- [ ] 12. Code: Are grammar and spelling correct? (Note: spell checks are only done for `doc/`)
- [ ] 13. Documentation: Is the grammar correct? (Note: spell checks are done by CI)
- [ ] 14. Are all function parameters `const` - if possible?
- [ ] 15. Are all class members `const` - if possible?
- [ ] 16. Is `memmove` avoided where a ringbuffer would be faster?
- [ ] 17. Is a unit test needed for this functionality?

## CI checks:

- No trailing whitespace?
- Are all included headers necessary?
- No compiler warnings?
- No spell errors in documentation? (new words must be appended to `doc/wordlist.txt`)
