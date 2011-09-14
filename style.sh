#!/bin/sh
uncrustify -c style.cfg --no-backup -l CPP `find . | grep -e "\.h$"`
uncrustify -c style.cfg --no-backup -l CPP `find . | grep -e "\.cpp$"`
