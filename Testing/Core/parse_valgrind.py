#!/usr/bin/python
"""
This script helps you create a valgrind suppression file that hides
bogus leaks so that only fixable leaks inside your project are reported.
For instance, leaks within the drivers may not be fixable or worth fixing,
but leaks within VTK itself should be fixed.

To use this script:
1) setup a vtk memory check dashboard and give valgrind the
-v and --gen-suppressions arguments that tell it to dump a verbose report
about all of the leaks it finds.
set(dashboard_do_memcheck ON)
set(CTEST_TEST_TIMEOUT 500) # long for valgrind.
set(CTEST_MEMORYCHECK_COMMAND "/usr/bin/valgrind")
set(CTEST_MEMORYCHECK_COMMAND_OPTIONS "--gen-suppressions=all
-v --trace-children=yes --leak-check=yes --show-reachable=yes
--num-callers=50")
2) run that dashboard locally, (no need to send it to cdash yet), and
direct the report to a log file. Consider using -I ,,N to run only every
N'th test to speed things up while you finalize the suppression file.
3) run this script on the log file:
python parse_valgrind logfilename.txt

The output will contain a list of proposed suppressions for all leaks that
valgrind found along with the list of tests that exhibited each particular
leak. For each bogus leak add the suppression to a file.

4) Let ctest know about the supressions you want to use like so:
set(CTEST_MEMORYCHECK_SUPPRESSIONS_FILE
"${CTEST_SCRIPT_DIRECTORY}/karego-at_vtk_valgrind_supression.txt")

The output of the script also contains a list of the suppresions in use
and the tests that use them. Use that to trim down the suppression
file to a minimal set, removing all unused suppressions etc.
"""

import re
import sys

#cmd line argument is name of valgrind output file to parse
file = open(sys.argv[1], "r")

#when valgrind run with -v this will show which suppressions
#were actually used
supp_report = re.compile(".*used_suppression:\s+(\d+) (.*)$")

sups = dict()
supps_used = dict()

def parse():
    global sups, supps_used
    insup = False
    currentSup = ""
    currentTest = ""
    for line in file.readlines():
        newtest = line.find("Testing: ")
        if newtest > 0:
            currentTest = line[newtest+8:].strip()
            #print "NEXT TEST: ", currentTest
        suppression_report = supp_report.match(line)
        if suppression_report:
            sname = suppression_report.group(2)
            #print "USED SUP: ", sname
            tests = (currentTest,)
            if sname in supps_used:
                tests = supps_used[sname]
                tests = tests + (currentTest,)
            supps_used[sname] = tests
        if line[0] == "{":
            insup = True
            currentSup = currentSup + line
        elif line[0] == "}":
            currentSup = currentSup + line
            #print "SUGGESTED SUP: ", currentSup
            insup = False
            tests = (currentTest,)
            if currentSup in sups:
                tests = sups[currentSup]
                tests = tests + (currentTest,)
            sups[currentSup] = tests
            currentSup = ""
        else:
            if insup:
                currentSup = currentSup + line

parse()

print "__suppressions needed____________________________"
for key in sups.keys():
    print len(sups[key]), ":", sups[key]
    print key
print "__suppressions used______________________________"
for key in supps_used.keys():
    print len(supps_used[key]), ":", supps_used[key]
    print key
print "_________________________________________________"
print "# Suppressions needed:", len(sups)
print "# Suppressions used:", len(supps_used)
