import sys, os

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
