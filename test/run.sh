#!/bin/bash

PACKFAILS="\
	undef_fds
"

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

for TST in $PACKFAILS; do
	echo "Running $TST"
	make clean || exit 1
	if [ -ne `make -s OBJ=$TST | grep -q "Unknown symbol"` ]; then
		RES="$RES\n$TST: PASS"
	else
		RES="$RES\n$TST: FAIL"
	fi
done

./run_cli.sh && RES="$RES\nCLI: PASS" || RES="$RES\nCLI: FAIL"

echo -e "$RES"
