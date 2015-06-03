#!/usr/bin/env python
'''
This program takes one mandatory parameter, the VTK base folder, and up to
 four optional parameters corresponding to the names of the testing language
 subfolders, e.g Cxx, Python, Tcl and, possibly, an optional print option along
 with some other optional options.

If no optional parameters are given, then some statistics about the number
 of tests are provided. These statistics are also displayed when optional
 parameters are provided.

Optional parameters are one or more of: Tcl, Cxx or Python.

If one optional parameter e.g Python, is provided, then a list of the tests
 by folder for that parameter are given if the print option is specified.

If two optional parameters, e.g Tcl Python, are provided, then a list
 of the tests by folder corresponding to those tests that are in Tcl but
 not Python is provided if the print option is specified.

If three optional parameters, e.g Tcl Cxx Python, are provided, then a list
 of the tests by folder corresponding to those tests that are in Tcl but
 not Cxx or Python is provided if the print option is specified.

These are the additional options:
  -e: include only those tests enabled in the CMakeLists.txt file.
  -d: include only those tests disabled in the CMakeLists.txt file.
  -n: include only those tests not in the CMakeLists.txt file.
  The above are mutually exclusive.
  -p Print out a list of files.

  Some examples of typical usage:
  <program file Name> <path to VTK> Tcl Python Cxx
  <program file Name> <path to VTK> Tcl Python Cxx -p
  <program file Name> <path to VTK> Tcl -n -p
  <program file Name> <path to VTK> -d -p

'''

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import sys
import os
import re
import errno
import shutil
from collections import defaultdict
import string
import copy
import argparse

class FindTests(object):
    '''
    Search through the VTK folder and create a dictionary of all the tests.
    Only the Testing folder's subdirectories: Cxx, Python and Tcl are searched.
    '''
    def __init__(self, inputPath):
        self.patterns = {'Cxx': re.compile(r'(^.*Testing)(\/Cxx)(.?|\/)$'),
                         'Python': re.compile(
                                        r'(^.*Testing)(\/Python)(.?|\/)$'),
                         'Tcl': re.compile(r'(^.*Testing)(\/Tcl)(.?|\/)$')}

        self.inputPath = inputPath

        # Dictionary of test names keyed by type e.g Python
        self.tests = dict()
        for key in self.patterns:
            self.tests[key] = defaultdict(set)
        self.fileExtensions = dict()
        for key in self.patterns:
            self.fileExtensions[key] = key.lower()
        self.fileExtensions['Python'] = 'py'
        # Used to validate input.
        self.keys = {'cpp': 'Cxx', 'cxx': 'Cxx', 'c++': 'Cxx',
                      'python': 'Python', 'tcl': 'Tcl'}

    def ParseCMakeFile(self, dir):
        '''
        Parse the CMakeLists.txt file as best we can by eliminating spurious
         lines and return two strings that can be searched for
         programs/scripts.

         The approach here is to just partition the file into two strings,
          one containing comments and the other the non-comment lines.

        :param: dir - the directory
        :return: a pair of strings containing the enabled programs/scripts and
                commented out programs/scripts.
        '''
        filePath = os.path.abspath(os.path.join(dir, 'CMakeLists.txt'))
        if not os.path.exists(filePath):
            return None, None
        fh = open(filePath, 'rb')
        nonComments = []
        comments = []
        for line in fh:
            # convert bytes to a string.
            line = line.strip().decode()
            if not line:
                continue
            if line[0] == '#':
                if len(line) > 1:
                    comments.append(line[1:])
                    continue
            if line:
                nonComments.append(line)
        # Remove duplicates.
        nonComments = set(nonComments)
        comments = set(comments)
        return '\n'.join(nonComments), '\n'.join(comments)
#        return '\n'.join(map(str, nonComments)), '\n'.join(map(str, comments))

    def AddValue(self, fn, key, v):
        '''
        :param: fn - the dictionary
        :param: key - the key
        :param: v - the value to add to the set of values in fn[key].
        '''
        try:
            values = fn[key]
            values.add(v)
            fn[key] = values
        except KeyError:
            # Key is not present
            fn[key] = set([v])

    def WalkPaths(self, option=0):
        """
        Walk the paths producing a dictionary keyed by the testing
         subdirectory type e.g Cxx, whose value is dictionary keyed by
         relative path
         whose value is a set of all the programs in that particular path.

        :param: option - This relates to the CMakeLists.txt file.
                         If 1: only enabled programs/scripts are selected,
                            2: only disabled programs/scripts are selected,
                            3: only programs/scripts not in CMakeLists.txt
                                 are selected.
                         Any other value - all programs/scripts are selected.
        """
        for root, dirs, files in os.walk(self.inputPath, topdown=True):
            for key in self.patterns:
                # Need to do this for Windows.
                r = root.replace('\\', '/')
                match = re.search(self.patterns[key], r)
                if match:
                    newKey = match.group(1)
                    if option > 0 and option < 4:
                        nonComments, comments = self.ParseCMakeFile(root)
                    for name in files:
                        fn = os.path.splitext(name)[0]
                        if option == 1:
                            if nonComments:
                                if fn in nonComments:
                                    if name.lower().endswith(
                                            self.fileExtensions[key]):
                                        self.AddValue(
                                            self.tests[key], newKey, fn)
                        elif option == 2:
                            if comments:
                                if fn in comments:
                                    if name.lower().endswith(
                                            self.fileExtensions[key]):
                                        self.AddValue(
                                            self.tests[key], newKey, fn)
                        elif option == 3:
                            s = None
                            if comments and nonComments:
                                s = comments + nonComments
                            elif comments:
                                s = comments
                            else:
                                s = nonComments
                            if s:
                                if not(fn in s):
                                    if name.lower().endswith(
                                            self.fileExtensions[key]):
                                        self.AddValue(self.tests[key],
                                                         newKey, fn)
                        else:
                            if name.lower().endswith(self.fileExtensions[key]):
                                self.AddValue(self.tests[key], newKey, fn)

    def Difference(self, a, b):
        '''
        :param: a - A dictionary of values.
        :param: b - A dictionary of values.
        :return: A dictionary where the values are the differences of the
                 values in a and b for each key.
        '''
        c = defaultdict(set)
        for key in a:
            if key in b:
                setDifference = a[key] - b[key]
                if setDifference:
                    for fn in setDifference:
                        try:
                            values = c[key]
                            values.add(fn)
                            c[key] = values
                        except KeyError:
                            # Key is not present
                            c[key] = set([fn])
            else:
                for fn in a[key]:
                    try:
                        values = c[key]
                        values.add(fn)
                        c[key] = values
                    except KeyError:
                        # Key is not present
                        c[key] = set([fn])
        return c

    def Union(self, a, b):
        '''
        :param: a - A dictionary of values.
        :param: b - A dictionary of values.
        :return: A dictionary of where the values are the union of the
                 values in a and b for each key.
        '''
        c = copy.deepcopy(a)
        for key in b:
            if key in a:
                setUnion = c[key] | b[key]
            else:
                setUnion = b[key]
            if setUnion:
                for fn in setUnion:
                    newKey = key
                    try:
                        values = c[newKey]
                        values.add(fn)
                        c[newKey] = values
                    except KeyError:
                        # Key is not present
                        c[newKey] = set([fn])
        return c

    def GetTotalNumberOfItems(self, a):
        '''
        :param: a - the dictionary.
        :return: The sum of all the elements in each value in the dictionary.
        '''
        numberOfItems = 0;
        for key in a:
            numberOfItems += len(a[key])
        return numberOfItems

    def NumberOfTestsByLanguage(self, tl):
        '''
        :param: tl - the language, e.g. Cxx, Python or Tcl.
        :return: The sum of all the elements in the dictionary for that language.
        '''
        return self.GetTotalNumberOfItems(self.tests[tl])

    def NumberOfUniqueTestsByLanguage(self, tl1, tl2, tl3):
        '''
        :param: tl - the language, e.g. Cxx, Python or Tcl.
        :param: t2 - the language, e.g. Cxx, Python or Tcl.
        :param: t3 - the language, e.g. Cxx, Python or Tcl.
        :return: The sum of all the elements in the dictionary for
                  the language tl1 that are unique.
        '''
        x = self.InANotBOrC(tl1, tl2, tl3)
        return self.GetTotalNumberOfItems(x)

    def InANotB(self, a, b):
        '''
        :param: a - The key for the first dictionary.
        :param: b - The key for the second dictionary.
        :return: d[a] - d[b], where d is a dictionary.
        '''
        try:
            v1 = self.tests[self.keys[a.lower()]]
            v2 = self.tests[self.keys[b.lower()]]
            return self.Difference(v1, v2)
        except:
            return None

    def InANotBOrC(self, a, b, c):
        '''
        :param: a - The key for the first dictionary.
        :param: b - The key for the second dictionary.
        :return: d[a] - (d[b] + d[c]), where d is a dictionary.
        '''
        try:
            v1 = self.tests[self.keys[a.lower()]]
            v2 = self.tests[self.keys[b.lower()]]
            v3 = self.tests[self.keys[c.lower()]]
            u = self.Union(v2, v3)
            return self.Difference(v1, u)
        except:
            return None

    def MakeSorted(self, d, s=None):
        '''
        :param: d - The dictionary.
        :param: s the string to append to the key.
        :return: A string containing a sorted list of the keys and values.
        '''
        l = list()
        for key, value in sorted(d.items()):
            if s:
                l.append(key + '/' + s)
            else:
                l.append(key)
            for v in sorted(value):
                if s:
                    l.append('  {:s}'.format(v))
                else:
                    l.append('  {:s}'.format(v))
        return '\n'.join(l)

def GetProgramParameters():
    description = 'Review the testing programs and scripts in VTK.'
    epilogue = '''
        This program produces statistics about the number of tests in VTK.
        It can also list programs/scripts that are in one language but not
         in other languages.
    '''
    parser = argparse.ArgumentParser(description=description, epilog=epilogue)
    parser.add_argument('inputPath',
        help='The path to the VTK source folder.', action='store')
    parser.add_argument('a',
        help='Tests in this folder e.g. one of Cxx, Python, Tcl.',
        nargs='?', default=None, action='store')
    parser.add_argument('b',
        help='Exclude tests in this folder e.g. one of Cxx, Python, Tcl.',
        nargs='?', default=None, action='store')
    parser.add_argument('c',
        help='Exclude tests in this folder e.g. one of Cxx, Python, Tcl.',
        nargs='?', default=None, action='store')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('-e', '--enabled',
        help='Tests enabled as determined by the CMakeLists.txt file.',
        action='store_true')
    group.add_argument('-d', '--disabled',
        help='Tests disabled as determined by the CMakeLists.txt file.',
        action='store_true')
    group.add_argument('-n', '--notIn',
        help='Tests not in the CMakeLists.txt file.',
        action='store_true')
    group1 = parser.add_argument_group()
    group1.add_argument('-p', '--printFileNames',
        help='Print the filenames sorted by folder.',
        action='store_true')
    args = parser.parse_args()
    return args.inputPath, \
            [args.a, args.b, args.c, args.enabled, args.disabled,
             args.notIn, args.printFileNames]

def CheckPythonVersion(ver):
    '''
    Check the Python version.

    :param: ver - the minimum required version number as hexadecimal.
    :return: True if if the Python version is greater than or equal to ver.
    '''
    if sys.hexversion < ver:
        return False
    return True

def main():
    if not CheckPythonVersion(0x02070000):
        print('This program requires Python 2.7 or greater.')
        return

    Ok = True

    inputPath, opts = GetProgramParameters()
    if not(os.path.exists(inputPath) and os.path.isdir(inputPath)):
        print(
    'Input path: {:s}\n should be the VTK top-level folder.'.format(inputPath))
        Ok = False

    tests = FindTests(inputPath)

    # An index for the number of languages selected.
    select = 0
    idx = 0
    for v in opts[:3]:
        if v:
            if not v.lower() in tests.keys:
                print('Invalid value: ', v)
                Ok = False
            else:
                opts[idx] = tests.keys[v.lower()]
                select += 1
        idx += 1
    if not Ok:
        return

    testOptions = 0
    testType = 'All tests'
    if opts[3]:
        testOptions = 1
        testType = 'Enabled tests'
    if opts[4]:
        testOptions = 2
        estType = 'Disabled tests'
    if opts[5]:
        testOptions = 3
        testType = 'Not in CmakeLists.txt'

    tests.WalkPaths(testOptions)

    c = tests.NumberOfTestsByLanguage('Cxx')
    ce = tests.NumberOfUniqueTestsByLanguage('Cxx', 'Python', 'Tcl')
    p = tests.NumberOfTestsByLanguage('Python')
    pe = tests.NumberOfUniqueTestsByLanguage('Python', 'Cxx', 'Tcl')
    t = tests.NumberOfTestsByLanguage('Tcl')
    te = tests.NumberOfUniqueTestsByLanguage('Tcl', 'Cxx', 'Python')

    print('-' * 40)
    print(testType + '.')
    print('-' * 40)
    print('{:32s}:{:6d}'.format('Unique Cxx tests', ce))
    print('{:32s}:{:6d}'.format('Total number of Cxx tests', c))
    print('-' * 40)
    print('{:32s}:{:6d}'.format('Unique Python tests', pe))
    print('{:32s}:{:6d}'.format('Total number of Python tests', p))
    print('-' * 40)
    print('{:32s}:{:6d}'.format('Unique Tcl tests', te))
    print('{:32s}:{:6d}'.format('Total number of Tcl tests', t))
    print('-' * 40)
    print('{:32s}:{:6d}'.format('Total number of unique tests', ce + pe + te))
    print('{:32s}:{:6d}'.format('Total number of tests', c + p + t))
    print('-' * 40)
    if opts[6]:  # Print the filenames.
        if testOptions >= 0 and testOptions < 4:
            if select > 0:
                if select == 1:
                    print('In {:s}.'.format(opts[0]))
                    print('-' * 40)
                    print(tests.MakeSorted(tests.tests[opts[0]], opts[0]))
                elif select == 2:
                    inANotB = tests.InANotB(opts[0], opts[1])
                    if inANotB:
                        print('In {:s} but not in {:s}.'.format(
                                opts[0], opts[1]))
                        print('-' * 40)
                        print(tests.MakeSorted(inANotB, opts[0]))
                elif select == 3:
                    inANotBOrCOrCxx = tests.InANotBOrC(opts[0],
                                                        opts[1], opts[2])
                    if inANotBOrCOrCxx:
                        print('In {:s} but not in {:s} or {:s}.'.format(
                                opts[0], opts[1], opts[2]))
                        print('-' * 40)
                        print(tests.MakeSorted(inANotBOrCOrCxx, opts[0]))
            else:
                for k in sorted(tests.tests.keys()):
                    print('-' * 40)
                    print(tests.MakeSorted(tests.tests[k], k))
            print('-' * 40)

    return Ok

if __name__ == '__main__':
    main()
