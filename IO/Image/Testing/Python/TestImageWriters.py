#!/usr/bin/env python
import os
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

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

sp = vtk.vtkStructuredPoints()
sp.SetDimensions(image1.GetOutput().GetDimensions())
sp.SetExtent(image1.GetOutput().GetExtent())
sp.SetScalarType(
  image1.GetOutput().GetScalarType(), image1.GetOutputInformation(0))
sp.SetNumberOfScalarComponents(
  image1.GetOutput().GetNumberOfScalarComponents(),
  image1.GetOutputInformation(0))
sp.GetPointData().SetScalars(image1.GetOutput().GetPointData().GetScalars())

luminance = vtk.vtkImageLuminance()
luminance.SetInputData(sp)

# Let's create a dictionary to test the writers, the key will be the writer
# and the value the file name used by the writer.
filenames = ["tiff1.tif", "tiff2.tif", "bmp1.bmp", "bmp2.bmp",
  "pnm1.pnm", "pnm2.pnm", "psw1.ps", "psw2.ps",
  "pngw1.png", "pngw2.png", "jpgw1.jpg", "jpgw2.jpg"]
writerObjects = list()
writerObjectTypes = ["vtk.vtkTIFFWriter()", "vtk.vtkBMPWriter()",
  "vtk.vtkPNMWriter()", "vtk.vtkPostScriptWriter()",
  "vtk.vtkPNGWriter()", "vtk.vtkJPEGWriter()" ]
idx = 0
for fn in filenames:
    # Create the writer object
    exec(fn.split(".")[0] + " = " + writerObjectTypes[int(idx / 2)])
    # Append the writer object to the list called writerObjects
    writerObjects.append(eval(fn.split(".")[0]))
    idx += 1

# Now create the dictionary.
writers = dict()
for idx in range(len(writerObjects)):
    writers.update({writerObjects[idx]: filenames[idx]})

#
# If the current directory is writable, then test the writers
#
try:
    for writer in writers:
        # The file name
        fn = writers[writer]
        # Use the file name to determine whether we are working
        # with the image or luninance.
        il = int(fn.split(".")[0][-1:])

        # Can we write to the directory?
        channel = open(fn, "wb")
        channel.close()

        if il == 1:
            writer.SetInputConnection(image1.GetOutputPort())
        elif il == 2:
            writer.SetInputConnection(luminance.GetOutputPort())
        else:
            continue
        writer.SetFileName(fn)
        writer.Write()

        # cleanup
        #
        try:
            os.remove(fn)
        except OSError:
            pass

    viewer = vtk.vtkImageViewer()
    viewer.SetInputConnection(luminance.GetOutputPort())
    viewer.SetColorWindow(255)
    viewer.SetColorLevel(127.5)
    viewer.Render()

except IOError:
    print  "Unable to test the writers."
