# Python repr() prints a prettier result

The new style prints the address of both the C++ vtkObjectBase object,
and the address of the PyObject that wraps it:

    <vtkmodules.vtkCommonCore.vtkFloatArray(0xbd306710) at 0x69252b820>

Wrapped objects like vtkVariant or vtkVector3f that are not derived from
vtkObjectBase are printed in a manner that suggests their construction:

    vtkmodules.vtkCommonCore.vtkVariant("hello")
    vtkmodules.vtkCommonDataModel.vtkVector3f([1.0, 2.0, 3.0])
