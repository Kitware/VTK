package require vtk
package require vtkinteraction

# we need to use composite data pipeline with multiblock datasets
vtkAlgorithm alg
vtkCompositeDataPipeline pip
alg SetDefaultExecutivePrototype pip
pip Delete

vtkRenderer Ren1
Ren1 SetBackground 0.33 0.35 0.43

vtkRenderWindow renWin
renWin AddRenderer Ren1

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin

vtkPLOT3DReader Plot3D0
Plot3D0 SetFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
Plot3D0 SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
Plot3D0 SetBinaryFile 1
Plot3D0 SetMultiGrid 0
Plot3D0 SetHasByteCount 0
Plot3D0 SetIBlanking 0
Plot3D0 SetTwoDimensionalGeometry 0
Plot3D0 SetForceRead 0
Plot3D0 SetByteOrder 0

vtkStructuredGridOutlineFilter Geometry5
Geometry5 SetInputConnection [Plot3D0 GetOutputPort]

vtkPolyDataMapper Mapper5
Mapper5 SetInputConnection [Geometry5 GetOutputPort]
Mapper5 SetImmediateModeRendering 1
Mapper5 UseLookupTableScalarRangeOn
Mapper5 SetScalarVisibility 0
Mapper5 SetScalarModeToDefault

vtkActor Actor5
Actor5 SetMapper Mapper5
[Actor5 GetProperty] SetRepresentationToSurface
[Actor5 GetProperty] SetInterpolationToGouraud
[Actor5 GetProperty] SetAmbient 0.15
[Actor5 GetProperty] SetDiffuse 0.85
[Actor5 GetProperty] SetSpecular 0.1
[Actor5 GetProperty] SetSpecularPower 100
[Actor5 GetProperty] SetSpecularColor 1 1 1

[Actor5 GetProperty] SetColor 1 1 1
Ren1 AddActor Actor5

vtkExtractGrid ExtractGrid0
ExtractGrid0 SetInputConnection [Plot3D0 GetOutputPort]
ExtractGrid0 SetVOI 0 14 0 32 0 24 
ExtractGrid0 SetSampleRate 1 1 1 
ExtractGrid0 SetIncludeBoundary 0

vtkExtractGrid ExtractGrid1
ExtractGrid1 SetInputConnection [Plot3D0 GetOutputPort]
ExtractGrid1 SetVOI 14 29 0 32 0 24 
ExtractGrid1 SetSampleRate 1 1 1 
ExtractGrid1 SetIncludeBoundary 0

vtkExtractGrid ExtractGrid2
ExtractGrid2 SetInputConnection [Plot3D0 GetOutputPort]
ExtractGrid2 SetVOI 29 56 0 32 0 24 
ExtractGrid2 SetSampleRate 1 1 1 
ExtractGrid2 SetIncludeBoundary 0

vtkLineSource LineSourceWidget0
LineSourceWidget0 SetPoint1 3.05638 -3.00497 28.2211
LineSourceWidget0 SetPoint2 3.05638 3.95916 28.2211
LineSourceWidget0 SetResolution 20

vtkMultiBlockDataSet mbds
mbds SetNumberOfBlocks 3

for {set i 0} {$i<3} {incr i 1} {
    ExtractGrid$i Update
    vtkStructuredGrid sg$i
    sg$i ShallowCopy [ExtractGrid$i GetOutput]
    mbds SetDataSet $i 0 sg$i
    sg$i Delete
}

vtkStreamTracer Stream0
Stream0 SetInput mbds
Stream0 SetSource [LineSourceWidget0 GetOutput]
Stream0 SetMaximumPropagationUnit 1
Stream0 SetMaximumPropagation 20
Stream0 SetInitialIntegrationStepUnit 2
Stream0 SetInitialIntegrationStep 0.5
Stream0 SetIntegrationDirection 0
Stream0 SetIntegratorType 0
Stream0 SetMaximumNumberOfSteps 2000
Stream0 SetTerminalSpeed 1e-12

mbds Delete

vtkAssignAttribute aa
aa SetInputConnection [Stream0 GetOutputPort]
aa Assign "Normals" "NORMALS" "POINT_DATA"

vtkRibbonFilter Ribbon0
Ribbon0 SetInputConnection [aa GetOutputPort]
Ribbon0 SetWidth 0.1
Ribbon0 SetAngle 0
Ribbon0 SetDefaultNormal 0 0 1 
Ribbon0 SetVaryWidth 0

vtkLookupTable LookupTable1
LookupTable1 SetNumberOfTableValues 256
LookupTable1 SetHueRange 0 0.66667
LookupTable1 SetSaturationRange 1 1
LookupTable1 SetValueRange 1 1
LookupTable1 SetTableRange 0.197813 0.710419
LookupTable1 SetVectorComponent 0
LookupTable1 Build

vtkPolyDataMapper Mapper10
Mapper10 SetInputConnection [Ribbon0 GetOutputPort]
Mapper10 SetImmediateModeRendering 1
Mapper10 UseLookupTableScalarRangeOn
Mapper10 SetScalarVisibility 1
Mapper10 SetScalarModeToUsePointFieldData
Mapper10 SelectColorArray "Density"
Mapper10 SetLookupTable LookupTable1

vtkActor Actor10
Actor10 SetMapper Mapper10
[Actor10 GetProperty] SetRepresentationToSurface
[Actor10 GetProperty] SetInterpolationToGouraud
[Actor10 GetProperty] SetAmbient 0.15
[Actor10 GetProperty] SetDiffuse 0.85
[Actor10 GetProperty] SetSpecular 0
[Actor10 GetProperty] SetSpecularPower 1
[Actor10 GetProperty] SetSpecularColor 1 1 1
Ren1 AddActor Actor10

# enable user interface interactor
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

alg SetDefaultExecutivePrototype {}
alg Delete