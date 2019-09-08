#!/bin/bash


echo "-------Start Server-------";
x-terminal-emulator -e ./osserver
#aspetto che il server sia avviato
while ![ps -A | grep -q osserver]; do 
	echo -n ...
	sleep 1
done
echo "-------Start Test 1-------" >> testout.log;
	for i in {1..50}; do ./osclient clientn$i 1 2>> testout.log &
	done;
	echo "-------End Test 1-------" >> testout.log;

echo -n "waiting test 1 to finish...";
#aspetto fine test1
while ps -A | grep -q osclient; do 
	echo -n ...
	sleep 1
done
echo
echo "-------Start Test 2-------" >> testout.log;	
	for i in {1..30}; do ./osclient clientn$i 2 2>> testout.log &
	done;
	echo "-------End test 2-------" >> testout.log;

echo -n "waiting test 2 to finish...";
#aspetto fine test2
while ps -A | grep -q osclient; do 
	echo -n ...
	sleep 1
done
echo
echo "-------Start Test 3-------" >> testout.log;
	for j in {31..50}; do ./osclient clientn$j 3 2>> testout.log &
	done;
	echo "-------End test 3-------">> testout.log;

echo -n "waiting test 3 to finish...";
#aspetto fine test3
while ps -A | grep -q osclient; do 
	echo -n ...
	sleep 1
done
echo
