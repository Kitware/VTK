
catch {load vtktcl}

# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93 
reader SetFilePrefix "../../../vtkdata/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn
reader UpdateWholeExtent

vtkImageMagnify magnify
magnify SetInput [reader GetOutput]
magnify SetMagnificationFactors 2 2 1
#magnify InterpolateOn

vtkImageMapper mapper1
  mapper1 SetInput [magnify GetOutput]
  mapper1 SetColorWindow 2000
  mapper1 SetColorLevel 1000
  mapper1 SetZSlice 20
#  mapper1 DebugOn

vtkImageMapper mapper2
  mapper2 SetInput [reader GetOutput]
  mapper2 SetColorWindow 2000
  mapper2 SetColorLevel 1000
  mapper2 SetZSlice 50 
#  mapper2 DebugOn

vtkImageMapper mapper3
  mapper3 SetInput [reader GetOutput]
  mapper3 SetColorWindow 2000
  mapper3 SetColorLevel 1000
  mapper3 SetZSlice 70 
#  mapper3 DebugOn

vtkImageMapper mapper4
  mapper4 SetInput [reader GetOutput]
  mapper4 SetColorWindow 2000
  mapper4 SetColorLevel 1000
  mapper4 SetZSlice 90 
#  mapper4 DebugOn

vtkImageMapper mapper5
  mapper5 SetInput [reader GetOutput]
  mapper5 SetColorWindow 2000
  mapper5 SetColorLevel 1000
  mapper5 SetZSlice 90 
#  mapper5 DebugOn

vtkImageMapper mapper6
  mapper6 SetInput [reader GetOutput]
  mapper6 SetColorWindow 2000
  mapper6 SetColorLevel 1000
  mapper6 SetZSlice 90 
#  mapper6 DebugOn

vtkActor2D actor1
  actor1 SetMapper mapper1
#  actor1 DebugOn

vtkActor2D actor2
  actor2 SetMapper mapper2
#  actor2 DebugOn

vtkActor2D actor3
  actor3 SetMapper mapper3
#  actor3 DebugOn

vtkActor2D actor4
  actor4 SetMapper mapper4
#  actor4 DebugOn

vtkActor2D actor5
  actor5 SetMapper mapper5
#  actor5 DebugOn

vtkActor2D actor6
  actor6 SetMapper mapper6
#  actor6 DebugOn

vtkImager imager1
  imager1 AddActor2D actor1
#  imager1 SetViewport 0.0 0.66 0.33 1.0 
  imager1 SetViewport 0.0 0.33 0.66 1.0
#  imager1 DebugOn

vtkImager imager2
  imager2 AddActor2D actor2
#  imager2 SetViewport 0.0 0.33 0.0 0.33 
  imager2 SetViewport 0.0 0.0 0.33 0.33
#  imager2 DebugOn

vtkImager imager3
  imager3 AddActor2D actor3
#  imager3 SetViewport 0.33 0.66 0.0 0.33 
  imager3 SetViewport 0.33 0.0 0.66 0.33
#  imager3 DebugOn

vtkImager imager4
  imager4 AddActor2D actor4
#  imager4 SetViewport 0.66 1.0 0.0 0.33 
  imager4 SetViewport 0.66 0.0 1.0 0.33
#  imager4 DebugOn

vtkImager imager5
  imager5 AddActor2D actor5
#  imager5 SetViewport 0.66 1.0 0.33 0.66 
  imager5 SetViewport 0.66 0.33 1.0 0.66
#  imager5 DebugOn

vtkImager imager6
  imager6 AddActor2D actor6
#  imager6 SetViewport 0.66 1.0 0.66 1.0
  imager6 SetViewport 0.66 0.66 1.0 1.0
#  imager6 DebugOn



vtkImageWindow imgWin
  imgWin AddImager imager1
  imgWin AddImager imager2
  imgWin AddImager imager3
  imgWin AddImager imager4
  imgWin AddImager imager5
  imgWin AddImager imager6
  imgWin SetSize 512 512
#  imgWin DebugOn

imgWin Render

wm withdraw .


# time the window level operation
set i 0;
proc timeit {} {
  global i
  puts [expr 1000000.0/[lindex [time {mapper1 SetColorLevel $i; 
                                      imgWin Render; 
                                      incr i} 100] 0]]
}

proc cine {} {
  for {set i 0} {$i < 89} {incr i 1} {
    mapper1 SetZSlice $i
    mapper2 SetZSlice [expr $i + 1]
    mapper3 SetZSlice [expr $i + 2]
    mapper4 SetZSlice [expr $i + 3]
    mapper5 SetZSlice [expr $i + 4]
    mapper6 SetZSlice [expr $i + 5]
    imgWin Render
  }
}

proc cine_up {} {
  for {set i 30} {$i < 90} {incr i 1} {
    mapper1 SetZSlice $i
    imgWin Render
  }
}

vtkWindowToImageFilter w2i
w2i SetInput imgWin

vtkBMPWriter bmp
bmp SetFileName viewport.bmp
bmp SetInput [w2i GetOutput]
#bmp Write