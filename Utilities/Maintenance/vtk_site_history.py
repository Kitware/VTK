#!/usr/bin/python
"""
This script scrapes submitter test failures for the last N days in order to
diagnose general dashboard health and especially intermittently problematic
machines.

The script saves out a summary that shows, for each submitter and for each of
the last N days, the number of failures, the names of each failing test and
the url to the cdash page for each test.

To use it, get to a command line and run:
python vtk_site_history.py
Doing so will results in one cvs file for each submitter containing detailed
results. While it works it prints out a summary to show progress.
If you give the script a numeric argument it will instead append just the last
N days worth of results to the files.

NOTE: this script assumes a unix shell so it exits immediately on windows.
"""

import subprocess as sub
import time
from datetime import date, timedelta
import sys

import platform
print platform.system()
if platform.system() == 'Windows':
    print "Sorry this script assumes a posix shell."
    sys.exit()

oneday = timedelta(1)
today = date.today()
tomorrow = today + oneday
numdays = 4*30 #cdash saves 4 months back
start = today-(oneday*numdays)

url1 = ''
url2 = ''
url3 = ''
url4 = ''

#build up a string to ask CDASH for just what we want.
url1 = url1 +  'curl "https://open.cdash.org/queryTests.php?project=VTK'
url1 = url1 + '&date='
url2 = url2 + '&limit=200'
url2 = url2 + '&showfilters=1'
url2 = url2 + '&filtercombine=and'
url2 = url2 + '&filtercount=3'
url2 = url2 + '&field1=status/string&compare1=61&value1=Failed'
url2 = url2 + '&field2=site/string&compare2=61&value2='
url3 = url3 + '&field3=buildname/string&compare3=61&value3='
url4 = url4 + '" 2>&1 | egrep "testDetails.*Failed" -B 2 | egrep "/td|/a"'

#submissions in VTK's nightly expected section
submissions = [
['amber10.kitware','Win64-VS10'],
['amber12.kitware','Win32-mingw-gcc-4.5'],
['Blight.kitware','blight'],
['bubbles.hooperlab','Fedora-17_OSMesa-9.1.3-x86_64'],
['DASH11.kitware','Win32-vs71-static'],
['DASH3.kitware','Win32-vs9-Static'],
['DASH3.kitware','Win32-vs9-Shared'],
['firefly.kitware','Ubuntu-GCC-4.7-release'],
['hythloth.kitware','Linux-gcc'],
['hythloth.kitware','TestExternal-Linux-gcc'],
['kamino.kitware','Mac10.7.5-clang-release-x86_64-nightly_master'],
['kamino.kitware','Mac10.7.5-gcc-release-x86_64-nightly_master'],
['karego-at.kitware','Ubuntu-Valgrind'],
['karego-at.kitware','Ubuntu-Coverage'],
['londinium.kitware','Arch-Clang-3.2-x86_64-debug'],
['londinium.kitware','Arch-GCC-4.8-x86_64-debug'],
['londinium.kitware','Arch-GCC-4.8-x86_64-release'],
['mirkwood.dlrsoftware','Win32-ninja-Debug'],
['mirkwood.dlrsoftware','Win32-ninja-Release'],
['p90n03.pbm.ihost.com','AIX00F614-xlC'],
['RogueResearch11','Mac10.7-clang-dbg-x86_64'],
['RogueResearch11','Mac10.7-clang-rel-x86_64'],
['RogueResearch3','Mac10.5-gcc-dbg-ppc64-static'],
['RogueResearch3','Mac10.5-gcc-dbg-ppc-shared'],
['RogueResearch7','Mac10.8-clang-dbg-x86_64'],
['RogueResearch7','Mac10.8-clang-rel-x86_64'],
['RogueResearch9','Mac10.6-gcc-dbg-i386'],
['RogueResearch9','Mac10.6-gcc-rel-x86_64'],
]

#open the pages for one particular submitter and compile output into a dictionary
def getResults(site, submission):
    results = []
    aday = start
    while aday < tomorrow:
        datestr = "%4d-%02d-%02d"%(aday.year, aday.month, aday.day)
        cmd = url1+datestr+url2+site+url3+submission+url4
        aday = aday + oneday
        #print cmd
        p = sub.Popen(['/bin/bash', '-c', cmd], stdout=sub.PIPE, stderr=sub.PIPE)
        output = p.stdout.read()[:-1]
        error = p.stderr.read()[:-1]
        #print site, submission, ":", datestr
        #print output
        results.append({'date':datestr,'fails':output})
    return results

#run through gathered results and format as a long string
def formatResults(results):
    formatted = ""
    cnt = -1
    for r in results:
        cnt = cnt +1
        d = r['date']
        f = r['fails']
        lines = f.split('\n')
        if len(f) == 0: #no failures!
            formatted = formatted  + d + ", " + "0" + "\n"
            continue
        if len(lines)>20: #specifics are capped at 10 failures
            formatted = formatted +  d + ", " + "11" + "\n"
            continue
        #a small number of failures, keep details
        tres = [d,",",len(lines)/2]
        for c in range(0, len(lines), 2):
            tname = lines[c+0].strip()[4:-5]
            turl = "https://open.cdash.org/" + lines[c+1].strip()[9:-12].replace('amp;','')
            tres.append(",")
            tres.append(tname)
            tres.append(",")
            tres.append(turl)
        for x in tres:
            formatted = formatted + str(x) + " "
        formatted = formatted + "\n"
    return formatted

if __name__ == '__main__':
    printheader = True
    if len(sys.argv) == 2: #just append last few days
        numdays = int(sys.argv[1])
        printheader = False

    for x,y in submissions:
        fname = x+'_'+y+'.csv'
        print x,y
        fd = open(fname, 'a')
        if printheader:
            fd.write("#"+fname+" date, numfails, failed test name1, failed test url1, ...\n")
        start = today-(oneday*numdays)
        res = getResults(x,y)
        resStrings = formatResults(res)
        for x in resStrings.split("\n")[0:-1]:
           print x.split(",")[0], x.split(",")[1]
        fd.write(resStrings)
        fd.close()
