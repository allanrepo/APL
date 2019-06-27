#!/bin/bash
PROCESS="$1"

#################################################################################
#
#################################################################################

#echo "Executing App Killer  by Allan Asis"

#################################################################################
# finds and list all $PROCESS that are running. kills them all if it can
# note that if caller is not admin, it can't kill $PROCESS owned by someone
# else
#################################################################################
#echo "Searching for processes with $PROCESS name on it..."
i=0
for entry in `ps -ce | grep $PROCESS | grep -v grep | awk '{print $1}'`
do
	echo "$entry"
	echo "Found $PROCESS process with PID=$entry. Killing it..."
	i=$i+1
	`kill -9 $entry`
done

# tell me if there's no process found
if [[  $i -eq "0" ]]; then
	echo "Didn't find any $PROCESS running."
fi

sleep 1

#################################################################################
# let's try again. this time if we find another, it means we didn't get 
# to kill it probably because it's owned by someone else. not our problem. 
# let user deal with it.
#################################################################################
for entry in `ps -ce | grep $PROCESS | grep -v grep | awk '{print $1}'`
do
	echo "Bad news... found $PROCESS with pid=$entry."
	echo "Try logging with UID that owns $PROCESS."
done

exit 0
