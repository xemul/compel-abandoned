#!/bin/sh

echo "Run sleeping in background"
test/sleeping &
sleep 1

echo "Inject blob"
pid=`pidof test/sleeping`
./compel run test/simple.blob.o -t $pid
