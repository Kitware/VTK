package require vtk



set RANGE            150
set MAX_ITERATIONS_1 $RANGE
set MAX_ITERATIONS_2 $RANGE
set XRAD             200
set YRAD             200

vtkImageMandelbrotSource mandelbrot1
  mandelbrot1 SetMaximumNumberOfIterations 150
  mandelbrot1 SetWholeExtent 0 [expr $XRAD-1] \
                            0 [expr $YRAD-1] 0 0
  mandelbrot1 SetSampleCX [expr 1.3 / $XRAD] [expr 1.3 / $XRAD] \
        [expr 1.3 / $XRAD] [expr 1.3 / $XRAD]
  mandelbrot1 SetOriginCX -0.72 0.22  0.0 0.0
  mandelbrot1 SetProjectionAxes 0 1 2

vtkImageMapToWindowLevelColors mapToWL
mapToWL SetInputConnection [mandelbrot1 GetOutputPort]
mapToWL SetWindow $RANGE
mapToWL SetLevel [expr $RANGE/3.0]

# set the window/level to 255.0/127.5 to view full range
vtkImageViewer viewer
viewer SetInputConnection [mapToWL GetOutputPort]
viewer SetColorWindow 255.0
viewer SetColorLevel 127.5

viewer Render

#make interface
viewer Render

vtkWindowToImageFilter windowToimage
  windowToimage SetInput [viewer GetRenderWindow]


