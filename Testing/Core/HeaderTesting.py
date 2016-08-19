#!/usr/bin/env python

## /*=========================================================================

##   Program:   Visualization Toolkit
##   Module:    HeaderTesting.py

##   Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
##   All rights reserved.
##   See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

##      This software is distributed WITHOUT ANY WARRANTY; without even
##      the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##      PURPOSE.  See the above copyright notice for more information.

## =========================================================================*/
## .NAME HeaderTesting - a VTK style and validity checking utility
## .SECTION Description
## HeaderTesting is a script which checks the list of header files for
## validity based on VTK coding standard. It checks for proper super
## classes, number and style of include files, type macro, private
## copy constructor and assignment operator, broken constructors, and
## exsistence of PrintSelf method. This script should be run as a part
## of the dashboard checking of the Visualization Toolkit and related
## projects.

## .SECTION See Also
## http://www.vtk.org http://public.kitware.com/Dart/HTML/Index.shtml
## http://www.vtk.org/contribute.php#coding-standards

import sys
import re
import os
import stat

# Get the path to the directory containing this script.
if __name__ == '__main__':
    selfpath = os.path.abspath(sys.path[0] or os.curdir)
else:
    selfpath = os.path.abspath(os.path.dirname(__file__))

# Load the list of names mangled by windows.h.
exec(compile(open(os.path.join(selfpath, 'WindowsMangleList.py')).read(),
     os.path.join(selfpath, 'WindowsMangleList.py'), 'exec'))

## If tested from dart, make sure to fix all the output strings
test_from_dart = 0
if "DART_TEST_FROM_DART" in os.environ:
    test_from_dart = 1

## For backward compatibility
def StringEndsWith(str1, str2):
    l1 = len(str1)
    l2 = len(str2)
    if l1 < l2:
        return 0
    return (str1[(l1-l2):] == str2)

##
class TestVTKFiles:
    def __init__(self):
        self.FileName = ""
        self.ErrorValue = 0;
        self.Errors = {}
        self.WarningValue = 0;
        self.Warnings = {}
        self.FileLines = []
        self.Export = ""
        self.UnnecessaryIncludes = [
            "stdio.h",
            "stdlib.h",
            "string.h",
            "iostream",
            "iostream.h",
            "strstream",
            "strstream.h",
            "fstream",
            "fstream.h",
            "windows.h"
            ]
        pass
    def SetExport(self, export):
        self.Export = export
    def Print(self, text=""):
        rtext = text
        if test_from_dart:
            rtext = rtext.replace("<", "&lt;")
            rtext = rtext.replace(">", "&gt;")
        print(rtext)
    def Error(self, error):
        self.ErrorValue = 1
        self.Errors[error] = 1
        pass
    def Warning(self, warning):
        self.WarningValue = 1
        self.Warnings[warning] = 1
        pass
    def PrintErrors(self):
        if self.ErrorValue:
            self.Print( )
            self.Print( "There were errors:" )
        for a in self.Errors:
            self.Print( "* %s" % a )
    def PrintWarnings(self):
        if self.WarningValue:
            self.Print( )
            self.Print( "There were warnings:" )
        for a in self.Warnings:
            self.Print( "* %s" % a )

    def TestFile(self, filename):
        self.FileName = filename
        self.FileLines = []
        self.ClassName = ""
        self.ParentName = ""
        try:
            if sys.hexversion >= 0x03000000:
                file = open(filename, encoding='ascii', errors='ignore')
            else:
                file = open(filename)
            self.FileLines = file.readlines()
            file.close()
        except:
            self.Print("Problem reading file %s:\n%s" %
                       (filename, str(sys.exc_info()[1])))
            sys.exit(1)
        return not self.CheckExclude()

    def CheckExclude(self):
        prefix = '// VTK-HeaderTest-Exclude:'
        exclude = 0
        for l in self.FileLines:
            if l.startswith(prefix):
                e = l[len(prefix):].strip()
                if e == os.path.basename(self.FileName):
                    exclude += 1
                else:
                    self.Error("Wrong exclusion: "+l.rstrip())
        if exclude > 1:
            self.Error("Multiple VTK-HeaderTest-Exclude lines")
        return exclude > 0

    def CheckIncludes(self):
        count = 0
        lines = []
        nplines = []
        unlines = []
        includere = "^\s*#\s*include\s*[\"<]([^>\"]+)"
        ignincludere = ".*\/\/.*"
        regx = re.compile(includere)
        regx1 = re.compile(ignincludere)
        cc = 0
        includeparent = 0
        for a in self.FileLines:
            line = a.strip()
            rm = regx.match(line)
            if rm and not regx1.match(line):
                lines.append(" %4d: %s" % (cc, line))
                file = rm.group(1)
                if file == (self.ParentName + ".h"):
                    includeparent = 1
                if not StringEndsWith(file, ".h"):
                    nplines.append(" %4d: %s" % (cc, line))
                if file in self.UnnecessaryIncludes:
                    unlines.append(" %4d: %s" % (cc, line))
            cc = cc + 1
        if len(lines) > 1:
            self.Print()
            self.Print( "File: %s has %d includes: " %
                        ( self.FileName, len(lines)) )
            for a in lines:
                self.Print( a )
            self.Error("Multiple includes")
        if len(nplines) > 0:
            self.Print( )
            self.Print( "File: %s has non-portable include(s): " % self.FileName )
            for a in nplines:
                self.Print( a )
            self.Error("Non-portable includes")
        if len(unlines) > 0:
            self.Print( )
            self.Print( "File: %s has unnecessary include(s): " % self.FileName )
            for a in unlines:
                self.Print( a )
            self.Error("Unnecessary includes")
        if not includeparent and self.ParentName:
            self.Print()
            self.Print( "File: %s does not include parent \"%s.h\"" %
                        ( self.FileName, self.ParentName ) )
            self.Error("Does not include parent")
        pass

    def CheckGuard(self):
        guardre = r"^#ifndef\s+([^ ]*)_h$"
        guardsetre = r"^#define\s+([^ ]*)_h$"
        guardrex = re.compile(guardre)
        guardsetrex = re.compile(guardsetre)

        guard = None
        guard_set = None
        expect_trigger = False
        for line in self.FileLines:
            line = line.strip()
            if expect_trigger:
                gs = guardsetrex.match(line)
                if gs:
                    guard_set = gs.group(1)
                break
            g = guardrex.match(line)
            if g:
                guard = g.group(1)
                expect_trigger = True

        if not guard or not guard_set:
            self.Print("File: %s is missing a header guard." % self.FileName)
            self.Error("Missing header guard")
        elif not guard == guard_set:
            self.Print("File: %s is not guarded properly." % self.FileName)
            self.Error("Guard does is not set properly")
        elif not ('%s.h' % guard) == os.path.basename(self.FileName):
            self.Print("File: %s has a guard (%s) which does not match its filename." % (self.FileName, guard))
            self.Error("Guard does not match the filename")

    def CheckParent(self):
        classre = "^class(\s+[^\s]*_EXPORT)?\s+(vtk[A-Z0-9_][^ :\n]*)\s*:\s*public\s+(vtk[^ \n\{]*)"
        cname = ""
        pname = ""
        classlines = []
        regx = re.compile(classre)
        cc = 0
        lastline = ""
        for a in self.FileLines:
            line = a.strip()
            rm = regx.match(line)
            if not rm and not cname:
                rm = regx.match(lastline + line)
            if rm:
                export = rm.group(1)
                if export:
                    export = export.strip()
                cname = rm.group(2)
                pname = rm.group(3)
                classlines.append(" %4d: %s" % (cc, line))
                if not export:
                    self.Print("File: %s defines 1 class with no export macro:" % self.FileName)
                    self.Print(" %4d: %s" % (cc, line))
                    self.Error("No export macro")
                elif self.Export and self.Export != export:
                    self.Print("File: %s defines 1 class with wrong export macro:" % self.FileName)
                    self.Print(" %4d: %s" % (cc, line))
                    self.Print("      The export macro should be: %s" % (self.Export))
                    self.Error("Wrong export macro")
            cc = cc + 1
            lastline = a
        if len(classlines) > 1:
            self.Print()
            self.Print( "File: %s defines %d classes: " %
                        (self.FileName, len(classlines)) )
            for a in classlines:
                self.Print( a )
            self.Error("Multiple classes defined")
        if len(classlines) < 1:
            self.Print()
            self.Print( "File: %s does not define any classes" % self.FileName )
            self.Error("No class defined")
            return
        #self.Print( "Classname: %s ParentName: %s" % (cname, pname)
        self.ClassName = cname
        self.ParentName = pname
        pass
    def CheckTypeMacro(self):
        count = 0
        lines = []
        oldlines = []
        typere = "^\s*vtk(Abstract|Base)?Type(Revision)*Macro\s*\(\s*(vtk[^ ,]+)\s*,\s*(vtk[^ \)]+)\s*\)\s*"
        typesplitre = "^\s*vtk(Abstract|Base)?Type(Revision)*Macro\s*\("

        regx = re.compile(typere)
        regxs = re.compile(typesplitre)
        cc = 0
        found = 0
        for a in range(len(self.FileLines)):
            line = self.FileLines[a].strip()
            rm = regx.match(line)
            if rm:
                found = 1
                if rm.group(2) == "Revision":
                    oldlines.append(" %4d: %s" % (cc, line))
                cname = rm.group(3)
                pname = rm.group(4)
                if cname != self.ClassName or pname != self.ParentName:
                    lines.append(" %4d: %s" % (cc, line))
            else:
                # Maybe it is in two lines
                rm = regxs.match(line)
                if rm:
                    nline = nline = line + " " + self.FileLines[a+1].strip()
                    line = nline.strip()
                    rm = regx.match(line)
                    if rm:
                        found = 1
                        if rm.group(2) == "Revision":
                            oldlines.append(" %4d: %s" % (cc, line))
                        cname = rm.group(3)
                        pname = rm.group(4)
                        if cname != self.ClassName or pname != self.ParentName:
                            lines.append(" %4d: %s" % (cc, line))
            cc = cc + 1
        if len(lines) > 0:
            self.Print( "File: %s has broken type macro(s):" % self.FileName )
            for a in lines:
                self.Print( a )
            self.Print( "Should be:\n vtkTypeMacro(%s, %s)" %
                        (self.ClassName, self.ParentName) )
            self.Error("Broken type macro")
        if len(oldlines) > 0:
            self.Print( "File: %s has legacy type-revision macro(s):" % self.FileName )
            for a in oldlines:
                self.Print( a )
                self.Print( "Should be:\n vtkTypeMacro(%s, %s)" %
                            (self.ClassName, self.ParentName))
            self.Error("Legacy style type-revision macro")
        if not found:
            self.Print( "File: %s does not have type macro" % self.FileName )
            self.Print( "Should be:\n vtkTypeMacro(%s, %s)" %
                            (self.ClassName, self.ParentName))
            self.Error("No type macro")
        pass
    def CheckForCopyAndAssignment(self):
        if not self.ClassName:
            return
        count = 0
        lines = []
        oldlines = []
        copyoperator = "^\s*%s\s*\(\s*const\s*%s\s*&\s*\) VTK_DELETE_FUNCTION;" % ( self.ClassName, self.ClassName)
        asgnoperator = "^\s*void\s*operator\s*=\s*\(\s*const\s*%s\s*&\s*\) VTK_DELETE_FUNCTION;" % self.ClassName
        #self.Print( copyoperator
        regx1 = re.compile(copyoperator)
        regx2 = re.compile(asgnoperator)
        foundcopy = 0
        foundasgn = 0
        for a in self.FileLines:
            line = a.strip()
            if regx1.match(line):
                foundcopy = foundcopy + 1
            if regx2.match(line):
                foundasgn = foundasgn + 1
        lastline = ""
        if foundcopy < 1:
          for a in self.FileLines:
            line = a.strip()
            if regx1.match(lastline + line):
                foundcopy = foundcopy + 1
            lastline = a
        lastline = ""
        if foundasgn < 1:
          for a in self.FileLines:
            line = a.strip()
            if regx2.match(lastline + line):
                foundasgn = foundasgn + 1
            lastline = a

        if foundcopy < 1:
            self.Print( "File: %s does not define copy constructor" %
                        self.FileName )
            self.Print( "Should be:\n%s(const %s&) VTK_DELETE_FUNCTION;" %
                        (self.ClassName, self.ClassName) )
            self.Error("No private copy constructor")
        if foundcopy > 1:
            self.Print( "File: %s defines multiple copy constructors" %
                        self.FileName )
            self.Error("Multiple copy constructor")
        if foundasgn < 1:
            self.Print( "File: %s does not define assignment operator" %
                        self.FileName )
            self.Print( "Should be:\nvoid operator=(const %s&) VTK_DELETE_FUNCTION;"
                        % self.ClassName )
            self.Error("No private assignment operator")
        if foundcopy > 1:
            self.Print( "File: %s defines multiple assignment operators" %
                        self.FileName )
            self.Error("Multiple assignment operators")
        pass
    def CheckWeirdConstructors(self):
        count = 0
        lines = []
        oldlines = []
        constructor = "^\s*%s\s*\(([^ )]*)\)" % self.ClassName
        copyoperator = "^\s*%s\s*\(\s*const\s*%s\s*&\s*\)\s*;\s*\/\/\s*Not\s*implemented(\.)*" % ( self.ClassName, self.ClassName)
        regx1 = re.compile(constructor)
        regx2 = re.compile(copyoperator)
        cc = 0
        for a in self.FileLines:
            line = a.strip()
            rm = regx1.match(line)
            if rm:
                arg = rm.group(1).strip()
                if arg and not regx2.match(line):
                    lines.append(" %4d: %s" % (cc, line))
            cc = cc + 1
        if len(lines) > 0:
            self.Print( "File: %s has weird constructor(s):" % self.FileName )
            for a in lines:
                self.Print( a )
            self.Print( "There should be only:\n %s();" % self.ClassName )
            self.Error("Weird constructor")
        pass

    def CheckPrintSelf(self):
        if not self.ClassName:
            return
        typere = "^\s*void\s*PrintSelf\s*\(\s*ostream\s*&\s*os*\s*,\s*vtkIndent\s*indent\s*\)"
        newtypere = "^\s*virtual\s*void\s*PrintSelf\s*\(\s*ostream\s*&\s*os*\s*,\s*vtkIndent\s*indent\s*\)"
        regx1 = re.compile(typere)
        regx2 = re.compile(newtypere)
        found = 0
        oldstyle = 0
        for a in self.FileLines:
            line = a.strip()
            rm1 = regx1.match(line)
            rm2 = regx2.match(line)
            if rm1 or rm2:
                found = 1
                if rm1:
                    oldstyle = 1
        if not found:
            self.Print( "File: %s does not define PrintSelf method:" %
                        self.FileName )
            self.Warning("No PrintSelf method")
        pass

    def CheckWindowsMangling(self):
        lines = []
        regx1 = WindowsMangleRegEx
        regx2 = re.compile("^.*VTK_LEGACY.*$")
        # This version will leave out comment lines but we probably do
        # not want to refer to mangled (hopefully deprecated) methods
        # in comments.
        # regx2 = re.compile("^(\s*//|\s*\*|.*VTK_LEGACY).*$")
        cc = 1
        for a in self.FileLines:
            line = a.strip()
            rm = regx1.match(line)
            if rm:
                arg =  rm.group(1).strip()
                if arg and not regx2.match(line):
                    lines.append(" %4d: %s" % (cc, line))
            cc = cc + 1
        if len(lines) > 0:
            self.Print( "File: %s has windows.h mangling violations:" % self.FileName )
            for a in lines:
                self.Print(a)
            self.Error("Windows Mangling Violation - choose another name that does not conflict.")
        pass

##
test = TestVTKFiles()

## Check command line arguments
if len(sys.argv) < 2:
    print("Testing directory not specified...")
    print("Usage: %s <directory> [ exception(s) ]" % sys.argv[0])
    sys.exit(1)

dirname = sys.argv[1]
exceptions = sys.argv[2:]
if len(sys.argv) > 2:
  export = sys.argv[2]
  if export[:3] == "VTK" and export[len(export)-len("EXPORT"):] == "EXPORT":
    print("Use export macro: %s" % export)
    exceptions = sys.argv[3:]
    test.SetExport(export)

## Traverse through the list of files
for a in os.listdir(dirname):
    ## Skip non-header files
    if not StringEndsWith(a, ".h"):
        continue
    ## Skip non-vtk files
    if not a.startswith('vtk'):
        continue
    ## Skip exceptions
    if a in exceptions:
        continue
    pathname = '%s/%s' % (dirname, a)
    if pathname in exceptions:
        continue
    mode = os.stat(pathname)[stat.ST_MODE]
    ## Skip directories
    if stat.S_ISDIR(mode):
        continue
    elif stat.S_ISREG(mode) and test.TestFile(pathname):
        ## Do all the tests
        test.CheckGuard()
        test.CheckParent()
        test.CheckIncludes()
        test.CheckTypeMacro()
        test.CheckForCopyAndAssignment()
        test.CheckWeirdConstructors()
        test.CheckPrintSelf()
        test.CheckWindowsMangling()

## Summarize errors
test.PrintWarnings()
test.PrintErrors()
sys.exit(test.ErrorValue)
