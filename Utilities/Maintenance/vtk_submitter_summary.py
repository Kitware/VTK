#!/usr/bin/python
"""
This module scrapes test results from a day's dashboard that explain the
configuration of the submitting machine. It is useful to find under tested spaces
in the option and platform set.

To use it get to a command line and run:
python vtk_submitter_summary.py
That will load the day's results, save them locally and
print and save two reports, which can then be imported into a spreadsheet.

You can also import the module in python and query the results manually.
"""

import sys
import urllib
import datetime
import re

configs = {}
submitters = configs
categories = {}
summary = {}

def scrape_cdash(date):

  #test_sysinfo_url = 'http://open.cdash.org/testSummary.php?project=11&name=vtkCommonCore-TestSystemInformation&date='+date
  test_sysinfo_url = 'http://open.cdash.org/testSummary.php?project=11&name=vtkCommonCoreCxx-TestSystemInformation&date='+date
  test_fbo_url = 'http://open.cdash.org/testSummary.php?project=11&name=vtkRenderingOpenGLCxx-TestFBO&date='+date

  testspage = urllib.urlopen(test_sysinfo_url)
  response = "".join(testspage.readlines())
  #print response

  print "scraping config info"

  #scan page for all machines that submitted that test
  testdetailspage_re = 'testDetails[^"]+'
  tester = re.compile(testdetailspage_re, re.DOTALL)
  matches = tester.finditer(response)

  #to get buildid so we can ask cdash for supplemental info
  buildid_re = 'build=(.+)'
  buildidtester = re.compile(buildid_re)

  #to get number of test failures for each submitter
  testfails_re = '<h3>(.+) tests failed.'
  testfails_tester = re.compile(testfails_re)

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
      #print configuration

      #get number of failures for this submission
      buildid = buildidtester.search(x.group(0)).group(0)[6:]
      #print buildid
      surl = 'http://open.cdash.org/viewTest.php?onlyfailed&buildid='+str(buildid)
      testspage = urllib.urlopen(surl)
      testsresponse = "".join(testspage.readlines())
      tfails = testfails_tester.search(testsresponse).group(1)
      #print tfails
      configuration['FAILS'] = tfails
      modules = configuration['MODULES']
      del configuration['MODULES']
      for x in modules:
         configuration[x] = 'ON'
    except (AttributeError,SyntaxError):
      configuration = {'NA' : "summary not found on this dashboard"}
      print configuration
    configs[key] = configuration

  print
  print "scraping GPU info"

  #TODO: pull out common parts into a scraper function
  #Now grab GL info from TestFBO

  testspage = urllib.urlopen(test_fbo_url)
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
      #print configs[key]['GL_VENDOR']
      #print configs[key]['GL_RENDERER']
      #print configs[key]['GL_VERSION']
    except (AttributeError,SyntaxError):
      print "GPU info not found on this dashboard"
      #configs[key]['GL_RENDERER'] = "unknown"
      #configs[key]['GL_VENDOR'] = "unknown"
      #configs[key]['GL_VERSION'] = "unknown"

  #print configs
  print "done scraping"

def save_database(filename = "pickled_cdash.txt"):
  #save that off for later reuse
  import pickle
  pfile = open(filename, "w")
  pickle.dump(configs, pfile)

def restore_database(filename = "pickled_cdash.txt"):
  global configs
  #save that off for later reuse
  import pickle
  pfile = open(filename, "r")
  configs = pickle.load(pfile)

def interpret_database():
  global submitters
  global categories
  global summary

  #make up list of all the submitters that day
  submitters = configs

  #make up a list of categories recorded that day
  categories = {}
  for x in submitters.keys():
    for y in submitters[x].keys():
      categories[y] = "1"

  categories = categories.keys()

  #groups submitters by commonality in each category
  summary = {}
  for x in categories:
    summary[x] = {}

  for y in submitters.keys():
    for x in categories:
      if x in submitters[y]:
        z = submitters[y][x]
        if z == "ON":
          z = 1
          submitters[y][x] = z
        if z == "OFF":
          z = 0
          submitters[y][x] = z
        if not str(z) in summary[x]:
          summary[x][str(z)] = []
        summary[x][str(z)].append(y)


def make_feature_view(filename="features.txt"):
  result = ""
  result = result + "FEATURE; VALUE; AVGFAILS; SUBMITTERS..."
  scategories = sorted(categories)
  for x in scategories:
    result = result + "\n"
    for y in sorted(summary[x].keys()):
      if y: #ignore submitters who said nothing about this category
        result = result + x + ";"
        result = result + y + ";"

        #augment with the number of failures for submitters with this
        #configuration
        failcnt = 0
        hascnt = 0
        for z in summary[x][y]:
          if 'FAILS' in submitters[z]:
            if  int(submitters[z]['FAILS'])<100:
              #ignore balky machines
              failcnt = failcnt + float(submitters[z]['FAILS'])
              hascnt = hascnt + 1
        avgfails = "NA"
        if hascnt:
          avgfails = failcnt/hascnt
        result = result + str(avgfails) + ";"

        result = result + str(summary[x][y])
        result = result + "\n"
  return result


def print_feature_view():
  print make_feature_view()

def save_feature_view(filename="features.txt"):
  ofile = open(filename, "w")
  ofile.write(make_feature_view())

def make_submitter_view(filename="submitters.txt"):
  result = ""
  ssubmitters = sorted(submitters.keys())
  scategories = sorted(categories)
  result = result + " ;"
  for y in scategories:
      result = result + y + ";"
  result = result + "\n"

  for x in ssubmitters:
      result = result + "\n"
      result = result + x + ";"
      for y in scategories:
          z = "''"
          if y in submitters[x]:
             z = str(submitters[x][y])
          result = result + z + ";"
      result = result + "\n"

  return result

def print_submitter_view():
  print make_submitter_view()

def save_submitter_view(filename="submitters.txt"):
  ofile = open(filename, "w")
  ofile.write(make_submitter_view())


if __name__ == '__main__':
  if len(sys.argv) == 2:
    #assumes YYYY-MM-DD" format
    date = sys.argv[1]
  else:
    now = datetime.datetime.now()
    date = now.strftime("%Y-%m-%d")
  scrape_cdash(date)
  save_database()
  restore_database()
  interpret_database()
  print_feature_view()
  save_feature_view()
  print
  print_submitter_view()
  save_submitter_view()
