package require vtk
package require vtkinteraction
vtkRenderer ren1
  ren1 SetBackground 0 0 0
vtkRenderWindow renWin
  renWin AddRenderer ren1
  renWin SetSize 300 300
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

vtkPDBReader pdb0
  pdb0 SetFileName "$VTK_DATA_ROOT/Data/caffeine.pdb"

  pdb0 SetHBScale 1.0
  pdb0 SetBScale 1.0

vtkSphereSource Sphere0
  Sphere0 SetCenter 0 0 0 
  Sphere0 SetRadius 1
  Sphere0 SetThetaResolution 8
  Sphere0 SetStartTheta 0
  Sphere0 SetEndTheta 360
  Sphere0 SetPhiResolution 8
  Sphere0 SetStartPhi 0
  Sphere0 SetEndPhi 180

vtkGlyph3D Glyph0
  Glyph0 SetInputConnection [pdb0 GetOutputPort]
  Glyph0 SetOrient 1
  Glyph0 SetColorMode 1
  #Glyph0 ScalingOn
  Glyph0 SetScaleMode 2
  Glyph0 SetScaleFactor .25
  Glyph0 SetSource [Sphere0 GetOutput]

vtkPolyDataMapper Mapper5
  Mapper5 SetInputConnection [Glyph0 GetOutputPort]
  Mapper5 SetImmediateModeRendering 1
  Mapper5 UseLookupTableScalarRangeOff
  Mapper5 SetScalarVisibility 1
  Mapper5 SetScalarModeToDefault
vtkLODActor Actor5
  Actor5 SetMapper Mapper5
  [Actor5 GetProperty] SetRepresentationToSurface
  [Actor5 GetProperty] SetInterpolationToGouraud
  [Actor5 GetProperty] SetAmbient 0.15
  [Actor5 GetProperty] SetDiffuse 0.85
  [Actor5 GetProperty] SetSpecular 0.1
  [Actor5 GetProperty] SetSpecularPower 100
  [Actor5 GetProperty] SetSpecularColor 1 1 1
  [Actor5 GetProperty] SetColor 1 1 1
  Actor5 SetNumberOfCloudPoints 30000

ren1 AddActor Actor5

vtkTubeFilter Tuber0
  Tuber0 SetInputConnection [pdb0 GetOutputPort]
  Tuber0 SetNumberOfSides 8
  Tuber0 SetCapping 0
  Tuber0 SetRadius 0.2
  Tuber0 SetVaryRadius 0
  Tuber0 SetRadiusFactor 10
vtkPolyDataMapper Mapper7
  Mapper7 SetInputConnection [Tuber0 GetOutputPort]
  Mapper7 SetImmediateModeRendering 1
  Mapper7 UseLookupTableScalarRangeOff
  Mapper7 SetScalarVisibility 1
  Mapper7 SetScalarModeToDefault
vtkLODActor Actor7
  Actor7 SetMapper Mapper7
  [Actor7 GetProperty] SetRepresentationToSurface
  [Actor7 GetProperty] SetInterpolationToGouraud
  [Actor7 GetProperty] SetAmbient 0.15
  [Actor7 GetProperty] SetDiffuse 0.85
  [Actor7 GetProperty] SetSpecular 0.1
  [Actor7 GetProperty] SetSpecularPower 100
  [Actor7 GetProperty] SetSpecularColor 1 1 1
  [Actor7 GetProperty] SetColor 1 1 1

ren1 AddActor Actor7

# enable user interface interactor
#iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .
