# These are always built with VTK. If they are not found, throw
# an exception
try:
    from vtkCommonPython import *
except ImportError:
    from libvtkCommonPython import *
try:
    from vtkFilteringPython import *
except ImportError:
    from libvtkFilteringPython import *
try:
    from vtkIOPython import *
except ImportError:
    from libvtkIOPython import *
try:
    from vtkImagingPython import *
except ImportError:
    from libvtkImagingPython import *
try:
    from vtkGraphicsPython import *
except ImportError:
    from libvtkGraphicsPython import *

# These are optional. If they are not found, continue.
try:
    from vtkRenderingPython import *
except ImportError:
    try:
        from libvtkRenderingPython import *
    except:
        pass
try:
    from vtkHybridPython import *
except ImportError:
    try:
        from libvtkHybridPython import *
    except:
        pass
try:
    from vtkParallelPython import *
except ImportError:
    try:
        from libvtkParallelPython import *
    except:
        pass
try:
    from vtkPatentedPython import *
except ImportError:
    try:
        from libvtkPatentedPython import *
    except:
        pass
    

import sys, os.path

    
def vtkRegressionTestImage( renWin ):
    
    imageIndex=-1;
    for i in range(0, len(sys.argv)):
        if sys.argv[i] == '-V' and i < len(sys.argv)-1:
            imageIndex = i+1

    if imageIndex != -1:
        fname = vtkGetDataRoot() + '/' + sys.argv[imageIndex]

        rt_w2if = vtkWindowToImageFilter()
        rt_w2if.SetInput(renWin)

        if os.path.isfile(fname):
            pass
        else:
            rt_pngw = vtkPNGWriter()
            rt_pngw.SetFileName(fname)
            rt_pngw.SetInput(rt_w2if.GetOutput())
            rt_pngw.Write()
            rt_pngw = None

        rt_png = vtkPNGReader()
        rt_png.SetFileName(fname)

        rt_id = vtkImageDifference()
        rt_id.SetInput(rt_w2if.GetOutput())
        rt_id.SetImage(rt_png.GetOutput())
        rt_id.Update()

        if rt_id.GetThresholdedError() <= 10:
            return 1
        else:
            sys.stderr.write('Failed image test: %f\n'
                             % rt_id.GetThresholdedError())
            return 0
    return 2
            
