# ParaView Version -1.6

package require vtk
package require vtkinteraction

vtkRenderer Ren1
	Ren1 SetBackground 0.33 0.35 0.43
vtkRenderWindow renWin
	renWin AddRenderer Ren1
	renWin SetSize 300 300
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin
# camera parameters
set camera [Ren1 GetActiveCamera]
	$camera SetPosition 13.36 12.7128 29.5781
	$camera SetFocalPoint 0 0 0
	$camera SetViewUp -0.0792596 0.928333 -0.363202
	$camera SetViewAngle 30
	$camera SetClippingRange 18.6219 55.3764

vtkRTAnalyticSource pvTemp69
	pvTemp69 SetWholeExtent -10 10 -10 10 -10 10
	pvTemp69 SetCenter 0 0 0
	pvTemp69 SetMaximum 255
	pvTemp69 SetXFreq 60
	pvTemp69 SetYFreq 30
	pvTemp69 SetZFreq 40
	pvTemp69 SetXMag 10
	pvTemp69 SetYMag 18
	pvTemp69 SetZMag 5
	pvTemp69 SetStandardDeviation 0.5
  pvTemp69 Update

vtkMetaImageWriter pvTemp200
  pvTemp200 SetFileName {mhdWriter.mhd}
  pvTemp200 SetInput [ pvTemp69 GetOutput ]
  pvTemp200 Write
vtkMetaImageReader pvTemp90
	pvTemp90 SetFileName {mhdWriter.mhd}
vtkLookupTable pvTemp109
	pvTemp109 SetNumberOfTableValues 256
	pvTemp109 SetHueRange 0.6667 0
	pvTemp109 SetSaturationRange 1 1
	pvTemp109 SetValueRange 1 1
	pvTemp109 SetTableRange 37.3531 260
	pvTemp109 SetVectorComponent 0
	pvTemp109 Build

vtkContourFilter pvTemp110
	pvTemp110 SetInput [pvTemp90 GetOutput 0]
	pvTemp110 SelectInputScalars {ImageFile}
	pvTemp110 SetValue 0 148.677
	pvTemp110 SetComputeNormals 1
	pvTemp110 SetComputeGradients 0
	pvTemp110 SetComputeScalars 0
vtkPolyDataMapper pvTemp114
	pvTemp114 SetInput [pvTemp110 GetOutput]
	pvTemp114 SetImmediateModeRendering 1
	pvTemp114 SetScalarRange 0 1
	pvTemp114 UseLookupTableScalarRangeOn
	pvTemp114 SetScalarVisibility 1
	pvTemp114 SetScalarModeToUsePointFieldData
	pvTemp114 SelectColorArray {ImageFile}
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

