# Description

(Write your PR description here)

# Checks

## Human checks (please check those!):

- [ ] Could parts of this PR be split into another PR (while keeping the PRs "atomic")?
- [ ] Does this PR depend on another PR or does it block another PR?
- [ ] Does this PR solve a known issue?
- [ ] Is there sufficient user documentation in the `doc/` folder?
- [ ] Is the code realtime safe? (e.g. no `new`/`delete`, no other syscalls like `sleep`/file handling/mutexes, no prints in non-error code pathways, ...)
- [ ] Is aliasing avoided?
- [ ] Are clicks/pops/discontinuities avoided?
- [ ] Are any new parameters well mapped over their provided range?
- [ ] Does the code define all magic numbers at a common place (e.g. on top of the file, or for constants which are used in several places, inside `src/globals.h`)?
- [ ] Are nontrivial formulae or constants explained and the source is referenced?
- [ ] Do nontrivial class variables have a doxygen comment?
- [ ] Code: Are grammar and spelling correct? (Note: spell checks are only done for `doc/`)
- [ ] Documentation: Is the grammar correct? (Note: spell checks are done by CI)
- [ ] Are all function parameters `const` - if possible?
- [ ] Are all class members `const` - if possible?
- [ ] Is `memmove` avoided where a ringbuffer would be faster?
- [ ] Is a unit test needed for this functionality?

## CI checks:

- No trailing whitespace?
- Are all included headers necessary?
- No compiler warnings?
- No spell errors in documentation? (new words must be appended to `doc/wordlist.txt`)
