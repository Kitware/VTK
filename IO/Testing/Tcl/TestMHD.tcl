package require vtk
package require vtkinteraction

vtkRenderer Ren1
	Ren1 SetBackground 0.33 0.35 0.43
vtkRenderWindow renWin
	renWin AddRenderer Ren1
	renWin SetSize 300 300
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

vtkImageReader reader
  reader SetDataByteOrderToLittleEndian
  reader SetDataExtent 0 63 0 63 1 93
  reader SetDataSpacing 3.2 3.2 1.5
  reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
  reader SetDataMask 0x7fff
  reader Update

vtkMetaImageWriter pvTemp200
  pvTemp200 SetFileName "mhdWriter.mhd"
  pvTemp200 SetInputData [ reader GetOutput]
  pvTemp200 Write
vtkMetaImageReader pvTemp90
	pvTemp90 SetFileName "mhdWriter.mhd"
	pvTemp90 Update

vtkLookupTable pvTemp109
	pvTemp109 SetNumberOfTableValues 256
	pvTemp109 SetHueRange 0.6667 0
	pvTemp109 SetSaturationRange 1 1
	pvTemp109 SetValueRange 1 1
	pvTemp109 SetTableRange 37.3531 260
	pvTemp109 SetVectorComponent 0
	pvTemp109 Build

vtkContourFilter pvTemp110
	pvTemp110 SetInputData [pvTemp90 GetOutput 0]
	pvTemp110 SetValue 0 1150
	pvTemp110 SetComputeNormals 1
	pvTemp110 SetComputeGradients 0
	pvTemp110 SetComputeScalars 0
vtkPolyDataMapper pvTemp114
	pvTemp114 SetInputConnection [pvTemp110 GetOutputPort]
	pvTemp114 SetImmediateModeRendering 1
	pvTemp114 SetScalarRange 0 1
	pvTemp114 UseLookupTableScalarRangeOn
	pvTemp114 SetScalarVisibility 1
	pvTemp114 SetScalarModeToUsePointFieldData
	pvTemp114 SelectColorArray "ImageFile"
pvTemp114 SetLookupTable pvTemp109
vtkActor pvTemp115
	pvTemp115 SetMapper pvTemp114
	[ pvTemp115 GetProperty] SetRepresentationToSurface
	[pvTemp115 GetProperty] SetInterpolationToGouraud
	[pvTemp115 GetProperty] SetAmbient 0
	[pvTemp115 GetProperty] SetDiffuse 1
	[pvTemp115 GetProperty] SetSpecular 0
	[pvTemp115 GetProperty] SetSpecularPower 1
	[pvTemp115 GetProperty] SetSpecularColor 1 1 1
Ren1 AddActor pvTemp115


iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

