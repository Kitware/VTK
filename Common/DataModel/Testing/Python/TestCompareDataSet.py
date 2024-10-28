from vtkmodules.vtkImagingCore import vtkRTAnalyticSource

w1 = vtkRTAnalyticSource()()
w2 = vtkRTAnalyticSource()()

assert(w1 == w2)
