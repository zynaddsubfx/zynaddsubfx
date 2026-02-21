# This is a list of things to do when releasing a new version.

## Preparation

- Update version number in CMakeLists.txt
- Update version number in src/Tests/guitar-adnote.xmz
- Update Copyright (mainly YEAR) info in src/UI/MasterUI.fl (note this is mentioned 2 times on line 5 and line 872)
- Update AUTHORS.txt to included new contributors since the last release
- Check and update (if needed) the manpage file : doc/zynaddsubfx.1.txt
- Check NEWS.txt contains all the changes since the previous version

## Build and test a release candidate

- Check the code builds on Linux: no gui, fltk, ntk, zest.
  Standalone and plugin versions.
- Check the code builds on Windows: zest. Standalone and plugin versions.
- Check the code builds on Mac: fltk, standalone only.
- Check the docker containers produce binaries
- Check the code runs on Linux, Mac and Windows (standalone and plugin versions)

Then tag the release git commit with the version number (zynaddsubfx,
mruby-zest-build).

## Publish the new version

- Build tarballs
- Upload source tarball to sourceforge website
- Update the "Linux/BSD/Source" link on http://zynaddsubfx.sourceforge.net/download.html (do that here : https://github.com/zynaddsubfx/zyn-website/blob/master/download.html)

- Update news on sourceforge website (https://zynaddsubfx.sourceforge.io/news.html)
- Issue release announcement to linux-audio-announce, zynaddsubfx-user, kvr,
  linux-musicians
