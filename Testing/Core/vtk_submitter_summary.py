#!/usr/bin/python
"""
This script scrapes two test results from a days dashboard that explain the
configuration of the submitting machine. Source this script and then you can
query 'configs' to see what each machine was testing.

To use it get to a command line and run:
python vtk_submitter_summary.py
Or better yet:
call it from a python interpretter and then work interactively with the configs map.
ex, to see what GPUs we are testing on:
import vtk_submitter_summary
for sub, info in sorted(vtk_submitter_summary.configs.iteritems()):
	if 'GL_RENDERER' in info:
		print sub, info['GL_RENDERER']
"""

import urllib
import datetime
import re

now = datetime.datetime.now()

url = 'http://open.cdash.org/testSummary.php?project=11&name=vtkCommonCore-TestSystemInformation&date='+now.strftime("%Y-%m-%d")
#url = 'http://open.cdash.org/testSummary.php?project=11&name=vtkCommonCore-TestSystemInformation&date=2013-02-13'
#print url

testspage = urllib.urlopen(url)
response = "".join(testspage.readlines())
#print response

#scan page for all machines that submitted that test
testdetailspage_re = 'testDetails[^"]+'
tester = re.compile(testdetailspage_re, re.DOTALL)
matches = tester.finditer(response)

configs = {}

#loop through and scan each machine's test page
for x in matches:
  url = 'http://open.cdash.org/' + str(x.group(0))
  url = url.replace('&amp;', '&')
  #print url

  #open that machine's test result we care about
  testpage = urllib.urlopen(url)
  testresponse = "".join(testpage.readlines())

  #grab build and site name
  buildname_re = 'buildSummary.php\?buildid=.+">(.+)</a>'
  buildname = re.search(buildname_re, testresponse).group(1)
  print buildname
  sitename_re = 'viewSite.php\?siteid=.+">(.+)</a>'
  sitename = re.search(sitename_re, testresponse).group(1)
  print sitename
  key = sitename + " " + buildname
  #grab test result
  testresult_re = 'CSMarker = \[(.+)\]#CSMarker'
  tester = re.compile(testresult_re, re.DOTALL)
  try:
    result = tester.search(testresponse).group(1)
    #todo: do this safely, don't use exec
    exec(result)
    configuration = ConfigSummary
    print configuration["MODULES"]
  except (AttributeError,SyntaxError):
    configuration = {'NA' : "summary not found on this dashboard"}
    print configuration
  configs[key] = configuration

print

#Now grab GL info from TestFBO
url = 'http://open.cdash.org/testSummary.php?project=11&name=vtkRenderingOpenGLCxx-TestFBO&date='+now.strftime("%Y-%m-%d")
#url = 'http://open.cdash.org/testSummary.php?project=11&name=vtkRenderingOpenGLCxx-TestFBO&date=2013-02-13'

testspage = urllib.urlopen(url)
response = "".join(testspage.readlines())
#print response

#scan page for all machines that submitted that test
testdetailspage_re = 'testDetails[^"]+'
tester = re.compile(testdetailspage_re, re.DOTALL)
matches = tester.finditer(response)

#loop through and scan each machine's test page
for x in matches:
  url = 'http://open.cdash.org/' + str(x.group(0))
  url = url.replace('&amp;', '&')
  #print url

  #open that machine's test result we care about
  testpage = urllib.urlopen(url)
  testresponse = "".join(testpage.readlines())

  #grab build and site name
  buildname_re = 'buildSummary.php\?buildid=.+">(.+)</a>'
  buildname = re.search(buildname_re, testresponse).group(1)
  print buildname
  sitename_re = 'viewSite.php\?siteid=.+">(.+)</a>'
  sitename = re.search(sitename_re, testresponse).group(1)
  print sitename
  key = sitename + " " + buildname
  if not(key in configs):
    configs[key] = {}
  #grab renderer string
  try:
    result = re.search('GL_RENDERER=(.+)', testresponse).group(1)
    configs[key]['GL_RENDERER'] = result
    result = re.search('GL_VENDOR=(.+)', testresponse).group(1)
    configs[key]['GL_VENDOR'] = result
    result = re.search('GL_VERSION=(.+)', testresponse).group(1)
    configs[key]['GL_VERSION'] = result
    print configs[key]['GL_VENDOR']
    print configs[key]['GL_RENDERER']
    print configs[key]['GL_VERSION']
  except (AttributeError,SyntaxError):
    print "GPU info not found on this dashboard"
    configs[key]['GL_RENDERER'] = "unknown"
    configs[key]['GL_VENDOR'] = "unknown"
    configs[key]['GL_VERSION'] = "unknown"

print configs
