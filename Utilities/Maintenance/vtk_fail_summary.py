#!/usr/bin/python
"""
This script asks cdash to give it a summary of all of the failing tests
on the nightly expected section. It presents the tests ranked by the number
of failing machines. From this view you can more easily see what is in
the greatest need of fixing.
"""

import sys
import time
import datetime
import urllib

# Process args
csvOutput = True
wikiOutput = False
dashDate = str(datetime.date.today())

argc = len(sys.argv)
while argc > 1:
  argc = argc - 1
  if sys.argv[argc] == "--csv":
    csvOutput = True
    wikiOutput = False
  elif sys.argv[argc] == "--wiki":
    wikiOutput = True
    csvOutput = False

if wikiOutput:
  print "==Dashboard for " + dashDate + "=="

url = 'https://open.cdash.org/api/?method=build&task=sitetestfailures&project=VTK&group=Nightly%20Expected'
page = urllib.urlopen(url)
data = page.readlines()
if len(data[0]) == 2: #"[]"
  print "Cdash returned nothing useful, please try again later."
  raise SystemExit

submissions = eval(data[0])
tfails = dict()
if csvOutput:
  print "-"*20, "ANALYZING", "-"*20
elif wikiOutput:
  print "===Builds for " + dashDate + "==="
  print r'{| class="wikitable sortable" border="1" cellpadding="5" cellspacing="0"'
  print r'|-'
  print r'| Build Name'
  print r'| Failing'

for skey in submissions.keys():
  submission = submissions[skey]
  bname = submission['buildname']
  bfails = submission['tests']
  if len(bfails) > 100:
    continue
  if csvOutput:
    print bname
    print len(bfails)
  elif wikiOutput:
    print r'|-'
    print r'| ',
    print r'[https://open.cdash.org/index.php?project=VTK' + '&date=' + dashDate + r'&filtercount=1' + r'&field1=buildname/string&compare1=61&value1=' + bname + " " + bname + "]"
    print r'|'
    print len(bfails)
  for tnum in range(0, len(bfails)):
    test = bfails[tnum]
    tname = test['name']
    if not tname in tfails:
       tfails[tname] = list()
    tfails[tname].append(bname)
if wikiOutput:
  print r'|}'

if csvOutput:
  print "-"*20, "REPORT", "-"*20
  print len(tfails)," FAILURES"
elif wikiOutput:
  print "===Tests for " + dashDate + "==="
  print r'{| class="wikitable sortable" border="1" cellpadding="5" cellspacing="0"'
  print r'|-'
  print r'| Test'
  print r'| Failing'
  print r'| Platforms'

failcounts = map(lambda x: (x,len(tfails[x])), tfails.keys())
sortedfails = sorted(failcounts, key=lambda fail: fail[1])
for test in sortedfails:
  tname = test[0]
  if csvOutput:
    print tname, ",", len(tfails[tname]), ",", tfails[tname]
  elif wikiOutput:
    print r'|-'
    print r'| '
    print r'[https://open.cdash.org/testSummary.php?' + r'project=11' + r'&date=' + dashDate + r'&name=' + tname + ' ' + tname + ']'
    print r'|',
    print len(tfails[tname])
    print r'|',
    print tfails[tname]
