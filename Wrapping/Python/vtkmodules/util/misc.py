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
    """vtkGetDataRoot() -- return vtk example data directory"""
    dataRoot = None
    for i, argv in enumerate(sys.argv):
        if argv == '-D' and i+1 < len(sys.argv):
            dataRoot = sys.argv[i+1]

    if dataRoot is None:
        dataRoot = os.environ.get('VTK_DATA_ROOT', '../../../../VTKData')

    return dataRoot

def vtkGetTempDir():
    """vtkGetTempDir() -- return vtk testing temp dir"""
    tempDir = None
    for i, argv in enumerate(sys.argv):
        if argv == '-T' and i+1 < len(sys.argv):
            tempDir = sys.argv[i+1]

    if tempDir is None:
        tempDir = '.'

    return tempDir

def vtkRegressionTestImage(renWin):
    """vtkRegressionTestImage(renWin) -- produce regression image for window

    This function writes out a regression .png file for a vtkWindow.
    Does anyone involved in testing care to elaborate?
    """
    from vtkmodules.vtkRenderingCore import vtkWindowToImageFilter
    from vtkmodules.vtkIOImage import vtkPNGReader
    from vtkmodules.vtkImagingCore import vtkImageDifference

    fname = None
    for i, argv in enumerate(sys.argv):
        if argv == '-V' and i+1 < len(sys.argv):
            fname = os.path.join(vtkGetDataRoot(), sys.argv[i+1])

    if fname is None:
        return 2

    else:
        rt_w2if = vtkWindowToImageFilter()
        rt_w2if.SetInput(renWin)

        if not os.path.isfile(fname):
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
