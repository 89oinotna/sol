#!/bin/bash


#echo "-------Start Server-------";

#x-terminal-emulator -e ./osserver
#aspetto che il server sia avviato
#var=$(ps -A | grep osserver)
#while [ -z "$var" ]; do 
	
#	echo -n ...
#	sleep 1
#	var=$(ps -A | grep osserver);
	#echo $var;
#done


echo "-------Start Test 1-------" >> testout.log;
	for i in {1..50}; do ./osclient client$i 1 2>> testout.log &
	pid+=" $!";
	done;
	

echo -n "waiting test 1 to finish...";
#aspetto fine test1
wait $pid
echo "-------End Test 1-------" >> testout.log;
unset pid
echo
echo "-------Start Test 2-------" >> testout.log;	
	for i in {1..30}; do ./osclient client$i 2 2>> testout.log &
	pid+=" $!";
	done;
	

echo -n "waiting test 2 to finish...";
#aspetto fine test2
wait $pid
echo "-------End test 2-------" >> testout.log;
unset pid
echo
echo "-------Start Test 3-------" >> testout.log;
	for j in {31..50}; do ./osclient client$j 3 2>> testout.log &
	pid+=" $!";
	done;
	

echo -n "waiting test 3 to finish...";
#aspetto fine test3
wait $pid
echo "-------End test 3-------">> testout.log;
echo
