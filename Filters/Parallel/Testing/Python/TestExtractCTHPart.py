#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkLookupTable
from vtkmodules.vtkCommonExecutionModel import (
    vtkAlgorithm,
    vtkCompositeDataPipeline,
)
from vtkmodules.vtkFiltersParallel import vtkExtractCTHPart
from vtkmodules.vtkIOXML import vtkXMLRectilinearGridReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCompositePolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot

VTK_DATA_ROOT = vtkGetDataRoot()

# we need to use composite data pipeline with multiblock datasets
alg = vtkAlgorithm()
pip = vtkCompositeDataPipeline()
alg.SetDefaultExecutivePrototype(pip)
del pip
# Create the RenderWindow, Renderer and both Actors
#
Ren1 = vtkRenderer()
Ren1.SetBackground(0.33,0.35,0.43)
renWin = vtkRenderWindow()
renWin.AddRenderer(Ren1)
renWin.SetSize(300,300)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
pvTemp59 = vtkXMLRectilinearGridReader()
pvTemp59.SetFileName(VTK_DATA_ROOT + "/Data/cth.vtr")
pvTemp59.UpdateInformation()
pvTemp59.SetCellArrayStatus("X Velocity",0)
pvTemp59.SetCellArrayStatus("Y Velocity",0)
pvTemp59.SetCellArrayStatus("Z Velocity",0)
pvTemp59.SetCellArrayStatus("Mass for Armor Plate",0)
pvTemp59.SetCellArrayStatus("Mass for Body, Nose",0)
pvTemp79 = vtkExtractCTHPart()
pvTemp79.SetInputConnection(pvTemp59.GetOutputPort())
pvTemp79.AddVolumeArrayName("Volume Fraction for Armor Plate")
pvTemp79.AddVolumeArrayName("Volume Fraction for Body, Nose")
pvTemp79.SetClipPlane(None)
pvTemp104 = vtkLookupTable()
pvTemp104.SetNumberOfTableValues(256)
pvTemp104.SetHueRange(0.6667,0)
pvTemp104.SetSaturationRange(1,1)
pvTemp104.SetValueRange(1,1)
pvTemp104.SetTableRange(0,1)
pvTemp104.SetVectorComponent(0)
pvTemp104.Build()
pvTemp87 = vtkCompositePolyDataMapper()
pvTemp87.SetInputConnection(pvTemp79.GetOutputPort())
pvTemp87.SetScalarRange(0,1)
pvTemp87.UseLookupTableScalarRangeOn()
pvTemp87.SetScalarVisibility(1)
pvTemp87.SetScalarModeToUsePointFieldData()
pvTemp87.SelectColorArray("Part Index")
pvTemp87.SetLookupTable(pvTemp104)
pvTemp88 = vtkActor()
pvTemp88.SetMapper(pvTemp87)
pvTemp88.GetProperty().SetRepresentationToSurface()
pvTemp88.GetProperty().SetInterpolationToGouraud()
pvTemp88.GetProperty().SetAmbient(0)
pvTemp88.GetProperty().SetDiffuse(1)
pvTemp88.GetProperty().SetSpecular(0)
pvTemp88.GetProperty().SetSpecularPower(1)
pvTemp88.GetProperty().SetSpecularColor(1,1,1)
Ren1.AddActor(pvTemp88)
renWin.Render()
alg.SetDefaultExecutivePrototype(None)
# --- end of script --
