#!/usr/bin/env python
from vtkmodules.vtkIOCore import (
    vtkGlobFileNames,
    vtkSortFileNames,
)
from vtkmodules.vtkIOImage import vtkImageReader2
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

globFileNames = vtkGlobFileNames()
globFileNames.AddFileNames(VTK_DATA_ROOT + "/Data/headsq/quarter.*[0-9]")

sortFileNames = vtkSortFileNames()
sortFileNames.SetInputFileNames(globFileNames.GetFileNames())
sortFileNames.NumericSortOn()

reader = vtkImageReader2()
reader.SetFileNames(sortFileNames.GetFileNames())
reader.SetDataExtent(0, 63, 0, 63, 1, 1)
reader.SetDataByteOrderToLittleEndian()

# set Z slice to 2: if output is not numerically sorted, the wrong
# slice will be shown
viewer = vtkImageViewer()
viewer.SetInputConnection(reader.GetOutputPort())
viewer.SetZSlice(2)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.GetRenderer().SetBackground(0, 0, 0)
viewer.Render()
