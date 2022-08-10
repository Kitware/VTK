# Python Wrapper Core Classes

This directory provides the core support classes that are used by VTK's
Python wrappers.  The classes can be split into two broad categories:
the *PyVTK* classes provide C APIs for Python types, while the *vtkPython*
classes are C++ utility classes.

## The Python Classes

### PyVTKObject

This defines APIs for creating and managing *PyVTKClass* objects, which
are Python extension types that wrap vtkObjectBase-derived classes, and
*PyVTKObject* objects, which are instances of the these extension types.

### PyVTKSpecialObject

Similarly, *PyVTKSpecialType* objects are Python extension types that
wrap C++ classes that are *not* derived from vtkObjectBase, and
*PyVTKSpecialObject* wraps the instances.  These object are reference
counted on the Python side, but not on the C++ side.  In general they
are lightweight objects that are cheap to copy.

### PyVTKTemplate

These objects represent C++ class templates.  The wrappers instantiate the
templates over a limited set of template parameters, and *PyVTKTemplate*
is a container for the template instantiations.  It is implemented as a
dictionary that maps template parameters to template instantiations.

### PyVTKEnum

This provides an API for managing subtypes of the Python *int* type that
represent named C++ enum types.

### PyVTKNamespace

This provides an API for managing subtypes of the Python *module* type that
represent C++ namespaces.

### PyVTKReference

Python does not support C++-style pass-by-reference, but pass-by-reference
can be simulated by passing a typed container whose contents can be modified.
The *PyVTKReference* type defines such containers.  Within Python, this
type can be accessed as *vtkmodules.vtkCommonCore.reference*.

### PyVTKMethodDescriptor

In Python, a method descriptor is an object that customizes method lookup,
specifically it customizes *object.method* and *class.method* method access.
The *PyVTKMethodDescriptor* customizes the access of *PyVTKClass* methods.
It handles bound method calls, unbound method calls, static method calls,
and calls to overloaded methods.

### PyVTKExtras

This one is not actually a class, it is a helper function that adds utility
methods and types like the previously-mentioned *reference* type to the
vtkCommonCore module.  Everything in this file becomes part of vtkCommonCore.


## The C++ Classes

### vtkPythonUtil

This is a singleton that keeps track of all the vtk-python extension modules
that have been loaded, and all of the vtk-python objects that have been
instantiated.  It contains all of the machinery that is needed for moving
VTK objects from C++ to Python and back again.

### vtkPythonCommand

This is a subclass of vtkCommand that allows Python methods to be used as
VTK observer callbacks.

### vtkPythonArgs

When a method call is performed in the wrappers, vtkPythonArgs does the
conversion of the arguments from Python to C++, and it also converts the
return value from C++ to Python.

### vtkPythonOverload

When an overloaded method is called from Python, this class uses the method
arguments to decide which overload to use.

### vtkPythonCompatibility

This is actually just a header, not a class.  It contains macros that make
it easier to write code that is compatible with different versions of the
Python C API.

### vtkSmartPyObject

Whereas the other classes in this directory are for using VTK C++ objects
through Python, this class is for using Python objects through C++.  This
class is a C++ smart pointer that handles Python reference counting.
