#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkTIFFReader
from vtkmodules.vtkInteractionStyle import vtkInteractorStyleRubberBandZoom
from vtkmodules.vtkRenderingCore import (
    vtkImageActor,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Set up the pipeline
reader = vtkTIFFReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/beach.tif")
# "beach.tif" image contains ORIENTATION tag which is
# ORIENTATION_TOPLEFT (row 0 top, col 0 lhs) type. The TIFF
# reader parses this tag and sets the internal TIFF image
# orientation accordingly.  To overwrite this orientation with a vtk
# convention of ORIENTATION_BOTLEFT (row 0 bottom, col 0 lhs ), invoke
# SetOrientationType method with parameter value of 4.
reader.SetOrientationType(4)
ia = vtkImageActor()
ia.GetMapper().SetInputConnection(reader.GetOutputPort())
ren = vtkRenderer()
ren.AddActor(ia)
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(400,400)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
rbz = vtkInteractorStyleRubberBandZoom()
rbz.SetInteractor(iren)
iren.SetInteractorStyle(rbz)
renWin.Render()
ren.GetActiveCamera().SetClippingRange(538.2413295991446, 551.8332823667997)
# Test style
iren.SetEventInformationFlipY(250,250,0,0,chr(0),0,"")
iren.InvokeEvent("LeftButtonPressEvent")
iren.SetEventInformationFlipY(100,100,0,0,chr(0),0,"")
iren.InvokeEvent("MouseMoveEvent")
iren.InvokeEvent("LeftButtonReleaseEvent")
# --- end of script --
