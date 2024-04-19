from vtkmodules.vtkCommonCore import (
    vtkDataArray,
    vtkFloatArray,
)

from vtkmodules.vtkCommonDataModel import (
    vtkImageData,
    vtkPlane,
    vtkPointData,
)

from vtkmodules.vtkCommonExecutionModel import (
    vtkTrivialProducer,
)

from vtkmodules.vtkFiltersCore import vtkFlyingEdgesPlaneCutter

X_AXES = 0
Y_AXES = 1
Z_AXES = 2
MIN_BOUNDARY = 0
MAX_BOUNDARY = 1

def Compare(nb1, nb2, message):
    if nb1 != nb2:
        print(str("ERROR: {} has {} instead of {}").format(message, nb1, nb2))
        return False

    return True

def CheckSlice(generatedSlice, expectedData):
    """ Compare generatedSlice polydata content with expectedData data array.
    Return false on error. """

    if not Compare(generatedSlice.GetNumberOfPoints(), expectedData.GetNumberOfTuples(), "wrong number of points in output"):
        return False

    data = generatedSlice.GetPointData().GetScalars()
    if data is None:
        print("ERROR: no scalar on output")
        return False

    if not Compare(data.GetNumberOfTuples(), expectedData.GetNumberOfTuples(), "wrong number of scalars in output"):
        return False

    for idx in range(data.GetNumberOfTuples()):
        if not Compare(data.GetValue(idx), expectedData.GetValue(idx), str("wrong value for point {}").format(idx)):
            return False

    return True

def SliceImage(mainAxes, imageOrigin, boundary):
    """ Slice an image data and return output polydata.
        Plane normal is aligned with mainAxes, and positioned at boundary.
        (0 is min boundary, 1 is max). """
    # create a source data
    image = vtkImageData()
    image.SetOrigin(imageOrigin, imageOrigin, imageOrigin)
    image.SetSpacing(8, 9, 10)
    image.SetExtent(0, 1, 0, 2, 0, 3)

    # Flying edges requires a scalars...
    data = vtkFloatArray()
    data.SetName("data")
    for i in range(image.GetNumberOfPoints()):
        data.InsertNextValue(42 + i)
    image.GetPointData().SetScalars(data)

    source = vtkTrivialProducer()
    source.SetOutput(image)

    # create a plane with Z normal
    plane = vtkPlane()
    axes = [0, 0, 0]
    axes[mainAxes] = 1
    plane.SetNormal(axes)

    sliceOrigin = [0, 0, 0]
    sliceOrigin[mainAxes] = image.GetBounds()[2 * mainAxes + boundary]
    plane.SetOrigin(sliceOrigin)

    # create plane cutter
    cutter = vtkFlyingEdgesPlaneCutter()
    cutter.SetInputConnection(source.GetOutputPort())
    cutter.SetPlane(plane)
    cutter.ComputeNormalsOff()
    cutter.Update()

    return cutter.GetOutput()

def TestImageOrigin():
    print("TestImageOrigin")

    expectedArray = vtkFloatArray()
    expectedArray.SetName("expected")

    # Test Z axes at max boundary
    outputPlane = SliceImage(Z_AXES, 0, MIN_BOUNDARY)
    for i in range(6):
        expectedArray.InsertNextValue(42 + i)
    if not CheckSlice(outputPlane, expectedArray):
        print("Cutting fails on image centered at 0")

    outputPlane = SliceImage(Z_AXES, -1, MIN_BOUNDARY)
    if not CheckSlice(outputPlane, expectedArray):
        print("Cutting fails on image centered at -1")

    outputPlane = SliceImage(Z_AXES, 13., MIN_BOUNDARY)
    if not CheckSlice(outputPlane, expectedArray):
        print("Cutting fails on image centered at 13.")

def TestSliceAxes():
    print("TestSliceAxes")

    expectedArray = vtkFloatArray()
    expectedArray.SetName("expected")

    # X normal plane
    outputPlane = SliceImage(X_AXES, 0, MIN_BOUNDARY)
    for i in range(12):
        expectedArray.InsertNextValue(42 + 2 * i)
    if not CheckSlice(outputPlane, expectedArray):
        print("Cutting on X at min boundary fails.")

    # Y normal plane
    outputPlane = SliceImage(Y_AXES, 0, MIN_BOUNDARY)
    expectedArray.Initialize()
    expectedArray.InsertNextValue(42)
    expectedArray.InsertNextValue(43)
    expectedArray.InsertNextValue(48)
    expectedArray.InsertNextValue(49)
    expectedArray.InsertNextValue(54)
    expectedArray.InsertNextValue(55)
    expectedArray.InsertNextValue(60)
    expectedArray.InsertNextValue(61)

    if not CheckSlice(outputPlane, expectedArray):
        print("Cutting on Y at min boundary fails.")

    # Z normal plane
    outputPlane = SliceImage(Z_AXES, 0, MIN_BOUNDARY)
    expectedArray.Initialize()
    for i in range(6):
        expectedArray.InsertNextValue(42 + i)
    if not CheckSlice(outputPlane, expectedArray):
        print("Cutting on Z at min boundary fails.")

def TestMaxBoundary():
    print("TestSliceAxes")

    expectedArray = vtkFloatArray()
    expectedArray.SetName("expected")

    outputPlane = SliceImage(Z_AXES, 0, MAX_BOUNDARY)
    for i in range(6):
        expectedArray.InsertNextValue(60 + i)
    if not CheckSlice(outputPlane, expectedArray):
        print("Cutting on max boundary fails.")


TestImageOrigin()
TestSliceAxes()
TestMaxBoundary()
