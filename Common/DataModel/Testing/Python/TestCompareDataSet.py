from vtkmodules.vtkImagingCore import vtkRTAnalyticSource

w1 = vtkRTAnalyticSource()()
w2 = vtkRTAnalyticSource()()

try:
    import numpy

    # In case numpy is available we're able to compare data arrays as well
    # along with just identity. If data arrays have the same contents, i.e.
    # if the two objects are essentially clones, this equality test should
    # pass
    assert w1 == w2
except:
    # When numpy is not available fall back to the default VTK behavior which
    # only tests for identity. In that case w1 and w2 are not considered equal
    assert w1 != w2
