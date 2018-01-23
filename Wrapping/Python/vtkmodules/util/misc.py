"""Miscellaneous functions and classes that don't fit into specific
categories."""

import sys, os

def calldata_type(type):
    """set_call_data_type(type) -- convenience decorator to easily set the CallDataType attribute
    for python function used as observer callback.
    For example:

    import vtkmodules.util.calldata_type
    import vtkmodules.util.vtkConstants
    import vtkmodules.vtkCommonCore import vtkCommand, vtkLookupTable

    @calldata_type(vtkConstants.VTK_STRING)
    def onError(caller, event, calldata):
        print("caller: %s - event: %s - msg: %s" % (caller.GetClassName(), event, calldata))

    lt = vtkLookupTable()
    lt.AddObserver(vtkCommand.ErrorEvent, onError)
    lt.SetTableRange(2,1)
    """
    from vtkmodules import vtkCommonCore
    supported_call_data_types = ['string0', vtkCommonCore.VTK_STRING,
            vtkCommonCore.VTK_OBJECT, vtkCommonCore.VTK_INT,
            vtkCommonCore.VTK_LONG, vtkCommonCore.VTK_DOUBLE, vtkCommonCore.VTK_FLOAT]
    if type not in supported_call_data_types:
        raise TypeError("'%s' is not a supported VTK call data type. Supported types are: %s" % (type, supported_call_data_types))
    def wrap(f):
        f.CallDataType = type
        return f
    return wrap

#----------------------------------------------------------------------
# the following functions are for the vtk regression testing and examples

def vtkGetDataRoot():
    """vtkGetDataRoot() -- return vtk example data directory
    """
    dataIndex=-1;
    for i in range(0, len(sys.argv)):
        if sys.argv[i] == '-D' and i < len(sys.argv)-1:
            dataIndex = i+1

    if dataIndex != -1:
        dataRoot = sys.argv[dataIndex]
    else:
        try:
            dataRoot = os.environ['VTK_DATA_ROOT']
        except KeyError:
            dataRoot = '../../../../VTKData'

    return dataRoot

def vtkGetTempDir():
    """vtkGetTempDir() -- return vtk testing temp dir
    """
    tempIndex=-1;
    for i in range(0, len(sys.argv)):
        if sys.argv[i] == '-T' and i < len(sys.argv)-1:
            tempIndex = i+1

    if tempIndex != -1:
        tempDir = sys.argv[tempIndex]
    else:
        tempDir = '.'

    return tempDir


def vtkRegressionTestImage( renWin ):
    """vtkRegressionTestImage(renWin) -- produce regression image for window

    This function writes out a regression .png file for a vtkWindow.
    Does anyone involved in testing care to elaborate?
    """
    from vtkmodules.vtkRenderingCore import vtkWindowToImageFilter
    from vtkmodules.vtkIOImage import vtkPNGReader
    from vtkmodules.vtkImagingCore import vtkImageDifference

    imageIndex=-1;
    for i in range(0, len(sys.argv)):
        if sys.argv[i] == '-V' and i < len(sys.argv)-1:
            imageIndex = i+1

    if imageIndex != -1:
        fname = os.path.join(vtkGetDataRoot(), sys.argv[imageIndex])

        rt_w2if = vtkWindowToImageFilter()
        rt_w2if.SetInput(renWin)

        if os.path.isfile(fname):
            pass
        else:
            rt_pngw = vtkPNGWriter()
            rt_pngw.SetFileName(fname)
            rt_pngw.SetInputConnection(rt_w2if.GetOutputPort())
            rt_pngw.Write()
            rt_pngw = None

        rt_png = vtkPNGReader()
        rt_png.SetFileName(fname)

        rt_id = vtkImageDifference()
        rt_id.SetInputConnection(rt_w2if.GetOutputPort())
        rt_id.SetImageConnection(rt_png.GetOutputPort())
        rt_id.Update()

        if rt_id.GetThresholdedError() <= 10:
            return 1
        else:
            sys.stderr.write('Failed image test: %f\n'
                             % rt_id.GetThresholdedError())
            return 0
    return 2
