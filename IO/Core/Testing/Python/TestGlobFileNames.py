#!/usr/bin/env python
# -*- coding: utf-8 -*-



# Run this test like so:
# vtkpython TestGlobFileNames.py  -D $VTK_DATA_ROOT

import re
from vtkmodules.vtkCommonSystem import vtkDirectory
from vtkmodules.vtkIOCore import vtkGlobFileNames
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class TestGlobFileNames(vtkmodules.test.Testing.vtkTest):

    def testGlobFileNames(self):

        globFileNames = vtkGlobFileNames()
        globFileNames.SetDirectory(VTK_DATA_ROOT + "/Data/")
        # globs do not include Kleene star support for pattern repetitions thus
        # we insert a pattern for both single and double digit file extensions.
        globFileNames.AddFileNames("headsq/quarter.[1-9]")
        globFileNames.AddFileNames("headsq/quarter.[1-9][0-9]")

        fileNames = globFileNames.GetFileNames()
        n = globFileNames.GetNumberOfFileNames()

        if n != 93:
            for i in range(0, n):
                print("File:", i, " ", fileNames.GetValue(i))
            print("GetNumberOfValues should return 93, returned", n)

            print("Listing of ", VTK_DATA_ROOT, "/Data/headsq")
            directory = vtkDirectory()
            directory.Open(VTK_DATA_ROOT + "/Data/headsq")
            m = directory.GetNumberOfFiles()
            for j in range(0, m):
                print(directory.GetFile(j))
            exit(1)

        for i in range(0, n):
            filename = fileNames.GetValue(i)
            if filename != globFileNames.GetNthFileName(i):
                print("mismatched filename for pattern quarter.*:", filename)
                exit(1)
            m = re.search("[\w|\W]*quarter.*", filename)
            if m == None:
                print("string does not match pattern quarter.*:", filename)


        # check that we can re-use the Glob object
        globFileNames.Reset()
        globFileNames.SetDirectory(VTK_DATA_ROOT + "/Data/")
        globFileNames.AddFileNames(VTK_DATA_ROOT + "/Data/financial.*")
        fileNames = globFileNames.GetFileNames()

        n = fileNames.GetNumberOfValues()
        for i in range(0, n):
            filename = fileNames.GetValue(i)

            if filename != globFileNames.GetNthFileName(i):
                print("mismatched filename for pattern financial.*: ", filename)
                exit(1)

            m = re.search("[\w|\W]*financial.*", filename)
            if m == None:
                print("string does not match pattern financial.*:", filename)
                exit(1)

        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(TestGlobFileNames, 'test')])
