#!/usr/bin/env python

import sys
import re
import os
import stat

class TestVTKFiles:
    def __init__(self):
        self.FileName = ""
        self.ErrorValue = 0;
        self.Errors = {}
        self.FileLines = []        
        pass
    def Print(self, text=""):
        rtext = text
        rtext = rtext.replace("<", "&lt;")
        rtext = rtext.replace(">", "&gt;")
        print rtext
    def Error(self, error):
        self.ErrorValue = 1
        self.Errors[error] = 1
        pass
    def PrintErrors(self):
        if self.ErrorValue:
            self.Print( )
            self.Print( "There were errors:" )
        for a in self.Errors.keys():
            self.Print( "* %s" % a )
        
    def TestFile(self, filename):
        self.FileName = filename
        self.FileLines = []
        self.ClassName = ""
        self.ParentName = ""
        try:
            file = open(filename)
            self.FileLines = file.readlines()
            file.close()
        except:
            self.Print( "Problem reading file: %s" % filename )
            sys.exit(1)
        pass
            
    def CheckIncludes(self):
        count = 0
        lines = []
        nplines = []
        includere = "^\s*#\s*include\s*[\"<]([^>\"]+)"
        regx = re.compile(includere)
        cc = 0
        includeparent = 0
        for a in self.FileLines:
            line = a.strip()
            rm = regx.match(line)
            if rm:
                lines.append(" %4d: %s" % (cc, line))
                file = rm.group(1)
                if file == (self.ParentName + ".h"):
                    includeparent = 1
                if not file.endswith(".h"):
                    nplines.append(" %4d: %s" % (cc, line))
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
            self.Error("Non-portabile includes")
        if not includeparent and self.ParentName:
            self.Print()
            self.Print( "File: %s does not include parent \"%s.h\"" %
                        ( self.FileName, self.ParentName ) )
            self.Error("Does not include parent")
        pass
    
    def CheckParent(self):
        classre = "^class\s*VTK_.*EXPORT (vtk[A-Z0-9][^ :\n]*)\s*:\s*public\s*(vtk[^ \n\{]*)"
        cname = ""
        pname = ""
        classlines = []
        regx = re.compile(classre)
        cc = 0
        for a in self.FileLines:
            line = a.strip()
            rm = regx.match(line)
            if rm:
                cname = rm.group(1)
                pname = rm.group(2)
                classlines.append(" %4d: %s" % (cc, line))
            cc = cc + 1
        if len(classlines) > 1:
            self.Print()
            self.Print( "File: %s defines %d classes: " %
                        (self.FileName, len(classlines)) )
            for a in lines:
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
        typere = "^\s*vtkType(Revision)*Macro\((vtk[^ ,]+)\s*,\s*(vtk[^ \)]+)\s*\)\s*;"
        regx = re.compile(typere)
        cc = 0
        for a in self.FileLines:
            line = a.strip()
            rm = regx.match(line)
            if rm:
                if rm.group(1) != "Revision":
                    oldlines.append(" %4d: %s" % (cc, line))
                cname = rm.group(2)
                pname = rm.group(3)
                if cname != self.ClassName or pname != self.ParentName:
                    lines.append(" %4d: %s" % (cc, line))
            cc = cc + 1
        if len(lines) > 0:
            self.Print( "File: %s has broken type macro(s):" % self.FileName )
            for a in lines:
                self.Print( a )
            self.Print( "Should be: vtkTypeRevisionMacro(%s, %s)" %
                        (self.ClassName, self.ParentName) )
            self.Error("Broken type macro")
        if len(oldlines) > 0:
            self.Print( "File: %s has old type macro(s):" % self.FileName )
            for a in lines:
                self.Print( a )
                self.Print( "Should be:\n vtkTypeRevisionMacro(%s, %s);" %
                            (self.ClassName, self.ParentName))
            self.Error("Old style type macro")
        pass
    def CheckForCopyAndAssignment(self):
        if not self.ClassName:
            return
        count = 0
        lines = []
        oldlines = []
        copyoperator = "^\s*%s\s*\(\s*const\s*%s\s*&\s*\)\s*;\s*\/\/\s*Not\s*implemented(\.)*" % ( self.ClassName, self.ClassName)
        asgnoperator = "^\s*void\s*operator=\s*\(\s*const\s*%s\s*&\s*\)\s*;\s*\/\/\s*Not\s*implemented(\.)*" % self.ClassName
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
            
        if foundcopy < 1:
            self.Print( "File: %s does not define copy constructor" %
                        self.FileName )
            self.Print( "Should be:\n%s(const %s&); // Not implemented" %
                        (self.ClassName, self.ClassName) )
            self.Error("No copy constructor")
        if foundcopy > 1:
            self.Print( "File: %s defines multiple copy constructors" %
                        self.FileName )
            self.Error("Multiple copy constructor")
        if foundasgn < 1:
            self.Print( "File: %s does not define assignment operator" %
                        self.FileName )
            self.Print( "Should be:\nvoid operator=(const %s&); // Not implemented"
                        % self.ClassName )
            self.Error("No assignment operator")
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
            self.Error("Wierd constructor")
        pass
    
    def CheckPrintSelf(self):
        if not self.ClassName:
            return
        typere = "^\s*void\s*PrintSelf\s*\(\s*ostream\s*&\s*os*\s*,\s*vtkIndent\s*indent\s*\)"
        regx = re.compile(typere)
        found = 0;
        for a in self.FileLines:
            line = a.strip()
            if regx.match(line):
                found = 1;
        if not found:
            self.Print( "File: %s does not define PrintSelf method:" %
                        self.FileName )
            self.Error("No PrintSelf method")
        pass
        


test = TestVTKFiles()

dirname = sys.argv[1]
for a in os.listdir(dirname):
    if not a.endswith(".h"):
        continue
    pathname = '%s/%s' % (dirname, a)
    mode = os.stat(pathname)[stat.ST_MODE]
    if stat.S_ISDIR(mode):
        pass
    elif stat.S_ISREG(mode):
        test.TestFile(pathname)
        test.CheckParent()
        test.CheckIncludes()
        test.CheckTypeMacro()
        test.CheckForCopyAndAssignment()
        test.CheckWeirdConstructors()
        test.CheckPrintSelf()

test.PrintErrors()
sys.exit(test.ErrorValue)
    
