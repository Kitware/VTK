#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkTIFFReader
from vtkmodules.vtkIOParallel import vtkPImageWriter
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
import os

VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
image1 = vtkTIFFReader()
image1.SetFileName(VTK_DATA_ROOT + "/Data/beach.tif")
# "beach.tif" image contains ORIENTATION tag which is
# ORIENTATION_TOPLEFT (row 0 top, col 0 lhs) type. The TIFF
# reader parses this tag and sets the internal TIFF image
# orientation accordingly.  To overwrite this orientation with a vtk
# convention of ORIENTATION_BOTLEFT (row 0 bottom, col 0 lhs ), invoke
# SetOrientationType method with parameter value of 4.
image1.SetOrientationType(4)
image1.Update()
#
# If the current directory is writable, then test the writers
#
if os.access('.', os.W_OK):
    piw = vtkPImageWriter()
    piw.SetInputConnection(image1.GetOutputPort())
    piw.SetFileName('piw.raw')
    piw.SetMemoryLimit(1)
    piw.Write()
    os.remove('piw.raw')
viewer = vtkImageViewer()
viewer.SetInputConnection(image1.GetOutputPort())
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
viewer.Render()
# --- end of script --
