package require vtk
package require vtkinteraction

# we need to use composite data pipeline with multiblock datasets
vtkAlgorithm alg
vtkCompositeDataPipeline pip
alg SetDefaultExecutivePrototype pip
pip Delete

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer Ren1
  Ren1 SetBackground 0.33 0.35 0.43
vtkRenderWindow renWin
  renWin AddRenderer Ren1
  renWin SetSize 300 300
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

vtkXMLRectilinearGridReader pvTemp59
  pvTemp59 SetFileName "$VTK_DATA_ROOT/Data/cth.vtr"
  pvTemp59 UpdateInformation
  pvTemp59 SetCellArrayStatus "X Velocity" 0
  pvTemp59 SetCellArrayStatus "Y Velocity" 0
  pvTemp59 SetCellArrayStatus "Z Velocity" 0
  pvTemp59 SetCellArrayStatus "Mass for Armor Plate" 0
  pvTemp59 SetCellArrayStatus "Mass for Body, Nose" 0

vtkExtractCTHPart pvTemp79
  pvTemp79 SetInputConnection [pvTemp59 GetOutputPort]
  pvTemp79 AddVolumeArrayName "Volume Fraction for Armor Plate"
  pvTemp79 AddVolumeArrayName "Volume Fraction for Body, Nose"
  pvTemp79 SetClipPlane {}
vtkLookupTable pvTemp104
  pvTemp104 SetNumberOfTableValues 256
  pvTemp104 SetHueRange 0.6667 0
  pvTemp104 SetSaturationRange 1 1
  pvTemp104 SetValueRange 1 1
  pvTemp104 SetTableRange 0 1
  pvTemp104 SetVectorComponent 0
  pvTemp104 Build
vtkCompositePolyDataMapper pvTemp87
  pvTemp87 SetInputConnection [pvTemp79 GetOutputPort]
  pvTemp87 SetImmediateModeRendering 1
  pvTemp87 SetScalarRange 0 1
  pvTemp87 UseLookupTableScalarRangeOn
  pvTemp87 SetScalarVisibility 1
  pvTemp87 SetScalarModeToUsePointFieldData
  pvTemp87 SelectColorArray "Part Index"
pvTemp87 SetLookupTable pvTemp104
vtkActor pvTemp88
  pvTemp88 SetMapper pvTemp87
  [ pvTemp88 GetProperty] SetRepresentationToSurface
  [pvTemp88 GetProperty] SetInterpolationToGouraud
  [pvTemp88 GetProperty] SetAmbient 0
  [pvTemp88 GetProperty] SetDiffuse 1
  [pvTemp88 GetProperty] SetSpecular 0
  [pvTemp88 GetProperty] SetSpecularPower 1
  [pvTemp88 GetProperty] SetSpecularColor 1 1 1
Ren1 AddActor pvTemp88

renWin Render

alg SetDefaultExecutivePrototype {}

