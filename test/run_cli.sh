#!/bin/bash

set -x

OUTF=cli.out
KWD="KwD"

make clean
make OBJ=cli/
./piggie > $OUTF &
PID=$!
../compel run --pid $PID --cfile ./parasite.co -- $KWD
kill -9 $PID

cat $OUTF | fgrep -q $KWD && exit 0 || exit 1
