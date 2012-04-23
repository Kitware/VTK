package require vtk

# This script is for testing the 3D threshold filter.

# Image pipeline

vtkRenderWindow imgWin
  imgWin SetSize 192 256

vtkImageReader reader
  reader ReleaseDataFlagOff
  reader SetDataByteOrderToLittleEndian
  reader SetDataExtent 0 63 0 63 1 93
  reader SetDataSpacing 3.2 3.2 1.5
  reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
  reader SetDataMask 0x7fff

set outputtype(1) SetOutputScalarTypeToSignedChar
set outputtype(2) SetOutputScalarTypeToUnsignedChar
set outputtype(3) SetOutputScalarTypeToLong
set outputtype(4) SetOutputScalarTypeToUnsignedLong
set outputtype(5) SetOutputScalarTypeToInt
set outputtype(6) SetOutputScalarTypeToUnsignedInt
set outputtype(7) SetOutputScalarTypeToShort
set outputtype(8) SetOutputScalarTypeToUnsignedShort
set outputtype(9) SetOutputScalarTypeToDouble
set outputtype(10) SetOutputScalarTypeToFloat
set outputtype(11) SetOutputScalarTypeToDouble
set outputtype(12) SetOutputScalarTypeToFloat

set replacein "ReplaceInOn ReplaceInOff"
set replaceout "ReplaceOutOn ReplaceOutOff"
set thresholds [list "ThresholdByLower 800" "ThresholdByUpper 1200" "ThresholdBetween 800 1200"]
set k 1
foreach rin $replacein {
  foreach rout $replaceout {
    foreach t $thresholds {
      vtkImageThreshold thresh$k
        thresh$k SetInValue 2000
        thresh$k SetOutValue 0
        thresh$k $rin
        thresh$k $rout
        thresh$k SetInputConnection [reader GetOutputPort]
        eval thresh$k $t
        thresh$k $outputtype($k)
      vtkImageMapper map$k
        map$k SetInputConnection [thresh$k GetOutputPort]
        if {$k < 3} {
          map$k SetColorWindow 255
          map$k SetColorLevel 127.5
         } else {
           map$k SetColorWindow 2000
           map$k SetColorLevel 1000
         }
      vtkActor2D act$k
        act$k SetMapper map$k
      vtkRenderer ren$k
        ren$k AddActor2D act$k
      imgWin AddRenderer ren$k
      incr k
      }
  }
}
ren1 SetViewport   0       0   .33333  .25
ren2 SetViewport  .33333   0   .66667  .25
ren3 SetViewport  .66667   0   1       .25
ren4 SetViewport   0       .25   .33333  .5
ren5 SetViewport  .33333   .25   .66667  .5
ren6 SetViewport  .66667   .25    1      .5
ren7 SetViewport   0       .5   .33333  .75
ren8 SetViewport  .33333   .5   .66667  .75
ren9 SetViewport  .66667   .5    1      .75
ren10 SetViewport   0       .75   .33333  1
ren11 SetViewport  .33333   .75   .66667  1
ren12 SetViewport  .66667   .75    1      1

imgWin Render
