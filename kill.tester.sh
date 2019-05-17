#!/bin/bash
TESTER="$1"

#################################################################################
# as simple script to kill tester execs. we do this here rather than
# on evxa EVXA::endTester() because shell locks down thread until execution
# is complete. this ensures that before we try to launch another OICu or
# optool, existing ones are dead.
#################################################################################

echo "Executing Tester Killer  by Allan Asis"


restart_tester $TESTER -k

sleep 1

exit 0
