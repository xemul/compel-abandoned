#!/bin/bash

TESTS="\
       sync_simple\
       async_simple\
       sync_args\
       async_fds\
       async_shmem\
"

RES=""

for TST in $TESTS; do
	echo "Running $TST"
	make clean || exit 1
	make OBJ=$TST test || exit 1
	tail -n1 piggie.out | fgrep -q PASS && RES="$RES\n$TST: PASS" || RES="$RES\n$TST: FAIL"
done

./run_cli.sh && RES="$RES\nCLI: PASS" || RES="$RES\nCLI: FAIL"

echo -e "$RES"
