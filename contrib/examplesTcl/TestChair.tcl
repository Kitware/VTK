catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetDataVOI 50 199 50 199 10 90
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageShiftScale ss
ss SetOutputScalarTypeToUnsignedChar
ss SetInput [reader GetOutput]
ss SetScale 0.05
ss SetShift 1000

# Create outline
vtkChairDisplay chair
chair SetInput [ss GetOutput]
chair SetXNotchSize 40
chair SetYNotchSize 60
chair SetZNotchSize 20

vtkPolyDataMapper chairMapper
    chairMapper SetInput [chair GetOutput]

vtkActor chairActor
    chairActor SetMapper chairMapper

vtkTexture atext
atext SetInput [chair GetTextureOutput]
atext InterpolateOn

chairActor SetTexture atext
[chairActor GetProperty] SetAmbient 0.2

# Okay now the graphics stuff
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor chairActor
ren1 SetBackground 0.1 0.2 0.4
renWin Render

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

#vtkImageViewer viewer
#viewer SetInput [chair GetTextureOutput]
#viewer SetColorWindow 255
#viewer SetColorLevel 127.5
#viewer Render

wm withdraw .

proc loop {} {
   for {set i 1} {$i < 40} {incr i} {
      chair SetXNotchSize [expr $i*3]
      chair SetYNotchSize [expr $i*2]
      chair SetZNotchSize $i
      renWin Render
#      viewer Render
   }
}

proc loop2 {} {
   ss UpdateWholeExtent
   for {set i 1} {$i < 40} {incr i} {
      chair SetXNotchSize [expr $i*3]
      chair SetYNotchSize [expr $i*2]
      chair SetZNotchSize $i
      renWin Render
#      viewer Render
   }
}

proc timeit {} {
   expr [lindex [time loop 1] 0]/1000000.0
   set a "   Normal = [expr [lindex [time loop 1] 0]/1000000.0]
   Loaded into memory = [expr [lindex [time loop2 1] 0]/1000000.0]"
}


