#!/usr/bin/env python
import os
import sys
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot, vtkGetTempDir

gotWarning = False
gotError = False

def WarningCallback(obj, evt):
    global gotWarning
    gotWarning = True


VTK_DATA_ROOT = vtkGetDataRoot()
VTK_TEMP_DIR = vtkGetTempDir()

# Image pipeline
image1 = vtk.vtkTIFFReader()
image1.SetFileName(VTK_DATA_ROOT + "/Data/beach.tif")
# "beach.tif" image contains ORIENTATION tag which is
# ORIENTATION_TOPLEFT (row 0 top, col 0 lhs) type. The TIFF
# reader parses this tag and sets the internal TIFF image
# orientation accordingly.  To overwrite this orientation with a vtk
# convention of ORIENTATION_BOTLEFT (row 0 bottom, col 0 lhs ), invoke
# SetOrientationType method with parameter value of 4.
image1.SetOrientationType(4)
image1.Update()

filename = VTK_TEMP_DIR + "/" + "pngw1.png"
testKey = "test key"
testValue = "test value"
longKey = "0123456789012345678901234567890123456789"\
          "0123456789012345678901234567890123456789"
longKeyValue = "this also prints a warning"

try:
    # Can we write to the directory?
    channel = open(filename, "wb")
    channel.close()

    writer = vtk.vtkPNGWriter()
    writer.SetInputConnection(image1.GetOutputPort())
    writer.SetFileName(filename)
    writer.AddText(testKey, testValue);
    # this is fine
    writer.AddText(testKey, testValue);
    observerId = writer.AddObserver(vtk.vtkCommand.WarningEvent, WarningCallback)
    # this prints a warning and does not add the text chunk
    writer.AddText("", "this prints a warning")
    if (not gotWarning):
        print("Error: expect warning when adding a text chunk with empty key")
        gotError = True
    gotWarning = False
    # this prints a warning and add a text chunk with a truncated key
    writer.AddText(longKey, longKeyValue)
    if (not gotWarning):
        print("Error: expect warning when adding a text chunk "\
              "with key length bigger than 79 characters")
        gotError = True
    writer.RemoveObserver(observerId)
    writer.Write()

    reader = vtk.vtkPNGReader()
    reader.SetFileName(filename);
    reader.Update();
    if (reader.GetNumberOfTextChunks() != 3):
        print("Error: Expecting three text chunks in the PNG file but got",\
              reader.GetNumberOfTextChunks())
        gotError = True
    beginEnd = [0, 0]
    reader.GetTextChunks(testKey,beginEnd)
    # the key starting with 0 comes in first.
    if (beginEnd[0] != 1 and beginEnd[1] != 3):
        print("Error: expect \"%s\" at index 1 and 2 but got "\
              "them at positions %d and %d" % (testKey, beginEnd[0], beginEnd[1]))
        gotError = True
    if (reader.GetTextKey(1) != testKey or reader.GetTextKey(2) != testKey):
        print("Error: expecting key \"%s\" at index 1 and 2 but got \"%s\"" % \
              (testKey, reader.GetTextKey(1)))
        gotError = True
    if (reader.GetTextValue(1) != testValue or reader.GetTextValue(2) != testValue):
        print("Error: expecting value \"%s\" at index 1 and 2 but got \"%s\"" % \
              (testValue, reader.GetTextValue(1)))
        gotError = True
    if (reader.GetTextKey(0) != longKey[:-1]):
        print("Error: expecting value \"%s\" at index but got \"%s\"" % \
              (longKey[:-1], reader.GetTextKey(0)))
        gotError = True
    if (gotError):
        sys.exit(1)
    else:
        sys.exit(0)

except IOError:
    print("Error: Unable to test PNG write/read of text chunks.")
    sys.exit(1)
