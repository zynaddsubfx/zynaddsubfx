# Description

(Write your PR description here)

# Checks

## Human checks (please check those!):

- [ ] Could parts of this PR be split into another PR (while keeping the PRs "atomic")?
- [ ] Is there sufficient user documentation in the `doc/` folder?
- [ ] Is the code realtime safe?
- [ ] Is aliasing avoided?
- [ ] Are clicks/pops/discontinuities avoided?
- [ ] Does the code define all magic numbers at a common place (e.g. on top of the file, or in `src/globals.h`)?
- [ ] Are nontrivial formulae or constants explained and the source is referenced?
- [ ] Do nontrivial class variables have a doxygen comment?
- [ ] Documentation/Comments: Is the grammar correct? (Note: spell checks are done by CI)
- [ ] Are all function parameters `const` - if possible?
- [ ] Are all class members `const` - if possible?
- [ ] Is memmove used where a ringbuffer would be faster?

## CI checks:

- No trailing whitespace?
- Are all included headers necessary?
- No compiler warnings?
- No spell errors in code and documentation? (new words must be appended to `doc/wordlist.txt`)
