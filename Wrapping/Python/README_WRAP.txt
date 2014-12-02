Notes on the Python Wrappers for VTK

First version by David Gobbi: Dec 19, 2002
Last update was on Dec 2, 2014

Abstract:
=========

This document provides a detailed description of how VTK objects are
accessed and modified through Python.  The installation of VTK-Python
is covered in README.txt.


Overview:
=========

Nearly all the power of VTK objects are available through Python
(with a few exceptions as noted below).  The C++ symantics are
translated as directly as possible to Python symantics.  Currently,
full support is provided for Python 2.3 through Python 2.6, though
the full Python 2 series should work (but not Python 3, yet).


The Basics:
===========

If VTK has been properly built and installed with the python wrappers,
then VTK can be accessed by importing the "vtk" module:
    import vtk
or
    from vtk import *

For the most part, using VTK from Python is very similar to using VTK
from C++ except for changes in syntax, e.g.
    vtkObject *o = vtkObject::New()
becomes
    o = vtkObject()

and
    o->Method()
becomes
    o.Method()

Some other important differences are that you can pass a Python tuple,
list or array to a method that requires a C++ array e.g.:

>>> a = vtkActor()
>>> p = (100.0, 200.0, 100.0)
>>> a.SetPosition(p)

If the C++ array parameter is used to return information, you must
pass a Python list or array that has the correct number of slots to
accept the returned values:

>>> z = [0.0, 0.0, 0.0]
>>> vtkMath.Cross((1,0,0),(0,1,0),z)
>>> print z
[0.0, 0.0, 1.0]

For multi-dimensional arrays, it is your choice whether to use
a nested list, or whether to use a numpy array with the correct
size and number of dimensions.

If the C++ method returns a pointer to an array, then that method is
only wrapped if there is a hint (usually in VTK/Wrapping/hints)
that gives the size of the array.  Hinted pointers are returned as
tuples:

>>> a = vtkActor()
>>> print a.GetPosition()
(0.0, 0.0, 0.0)

Finally, the python 'None' is treated the same as C++ NULL:

>>> a = vtkActor()
>>> a.SetMapper(None)
>>> print a.GetMapper()
None

And perhaps one of the most pleasant features of Python is that all
type-checking is performed at run time, so the type casts that are
often necessary in VTK-C++ are never needed in VTK-Python.


Constants
---------

Most VTK constants are available in Python, usually in the "vtk"
module but sometimes as class attributes:

>>> vtk.VTK_DOUBLE_MAX
1.0000000000000001e+299
>>> vtk.vtkCommand.ErrorEvent
39

Each named enum type is wrapped as a new Python type, and members
of the enum are instances of that type.  This allows type checking
for enum types:

>>> # works if given a constant of the correct enum type
>>> o.SetIntegrationMode(o.Continuous)
>>> # does not work, because "int" is not of the correct type
>>> o.SetIntegrationMode(10)
TypeError: SetIntegrationMode arg 1: expected enum EnumType, got int

Note that members of anonymous enums do not have a special type, and
are simply wrapped as python ints.


Namespaces
----------

Namespaces are currently wrapped in a very limited manner: the only
namespace members that are wrapped are constants and enum types.
There is no wrapping of namespaced classes or functions, or of nested
namespaces.  This is likely to be expanded upon when (or if) VTK begins
to make greater use of namespaces.


Unavailable methods
-------------------

A method is not wrapped if
1) its parameter list contains a pointer that isn't a vtkObject
   pointer, char pointer, or void pointer -- though the vtkDataArray
   "Tuple" methods are an exception, they are wrapped
2) it returns a pointer that is not a vtkObject pointer, char pointer,
   or void pointer, unless the method has an entry in the wrapping
   hints file -- again, vtkDataArray methods are an exception
3) it is an operator method (though many exceptions exist)


Unavailable classes
-------------------

Some classes are meant to be used only by other VTK classes and are
not wrapped.  These are labelled as WRAP_EXCLUDE in the CMakeLists.txt
files.


Printing VTK objects
====================

Printing a vtk object will provide the same information as provided
by the vtkObject::Print() method (i.e. the values of all instance variables)
The same information is provided by calling str(obj).  A more compact
representation of the object is available by calling repr(obj)

  repr(obj)  ->  '(vtkFloatArray)0x100c8d48'


Built-in documentation
======================

All of the documentation that is available in the VTK header files and
in the html man pages is also available through python.

If you want to see what methods are available for a class, use e.g.

>>> dir(vtk.vtkActor)

or, equivalently,

>>> a = vtk.vtkActor()
>>> dir(a)


You can also retrieve documentation about VTK classes and methods
from the built-in "docstrings":

>>> help(vtk.vtkActor)
>>> help(vtk.vtkActor.SetUserTransform)
[ lots of info printed, try it yourself ]

If you have an object, rather than a class, you must call help()
on the __class__ atribute of the object:

>>> help(a.__class__)

For the method documentation, all the different 'signatures' for the
method are given in Python format and the original C++ format:

>>> help(vtkActor.SetPosition)
SetPosition(...)
    V.SetPosition(float, float, float)
    C++: virtual void SetPosition(double _arg1, double _arg2, double _arg3)
    V.SetPosition([float, float, float])
    C++: virtual void SetPosition(double _arg[3])

    Set/Get/Add the position of the Prop3D in world coordinates.


Peculiarities and special features
==================================

Deleting a vtkObject
---------------------

There is no direct equivalent of o->Delete() since Python provides a
mechanism for automatic garbage collection.  The object will be
deleted once there are no remaining references to it either from
inside Python or from other VTK objects.  It is possible to get rid of
the local reference to the object by using the python 'del' command,
i.e. "del o", and this will result in a call to o->Delete() if the
local reference to the object was the last remaining reference to the
object from within Python.


Templated classes
-----------------

Templated classes are rare in VTK.  Where they occur, they can be
instantiated much like they can be in C++, except that [ ] brackets
are used for the template arguments instead of < > brackets:

>>> v = vtkVector['float64',3]([1.0, 2.0, 3.0])
>>> a = vtkDenseArray[str]()

Only a limited range of template args can be used, usually dictated by
by which args are used by typedefs and by other classes in the C++ code.
A list of the allowed argument combinations available for a particular
template can be found by calling help() on the template:

>>> help(vtkVector)

The types are usually given as strings, in the form 'int32', 'uint16',
'float32', 'float64', 'bool', 'char', 'str', 'vtkVariant'.
Python type objects are acceptable, too, if the name of the type is
the same as one of the accepted type strings:

>>> a = vtkDenseArray[int]()

Note that python 'int' is the same size as a C++ 'long', and python
'float' is the same size as C++ 'double'.  For compatibility with the
python array module, single-character typecodes are allowed, taken from
this list: '?', 'c', 'b', 'B', 'h', 'H', 'i', 'I', 'l', 'L', 'q', 'Q',
'f', 'd'.  The python array documentation explains what these mean.


Operator methods
----------------

Some useful operators are wrapped in python: the [ ] operator is
wrapped for indexing and item assignment, but because it relies on
hints to guess which indices are out-of-bounds, it is only wrapped
for vtkVector and a few other classes.

The comparison operators '<' '<=' '==' '>=' '>' are wrapped for all
classes that have these operators in C++.

The '<<' operator for printing is wrapped and is used by the python
'print()' and 'str()' commands.


Pass-by-reference
-----------------

Pass-by-reference of values that are mutable in C++ but not in
Python (such as string, int, and float) is only possible by
using vtk.mutable(), which is in the vtk module:

>>> plane = vtkPlane()
>>> t = mutable(0.0)
>>> x = [0.0, 0.0, 0.0]
>>> plane.InsersectWithLine([0, 0, -1], [0, 0, 1], t, x)
>>> print t
0.5


Observer, Event and CallData
----------------------------

* Simple callback

Similarly to what can be done in C++, a python function can be called
each time a VTK event is invoked on a given object:

>>> def onObjectModified(object, event):
>>>     print("object: %s - event: %s" % (object.GetClassName(), event))
>>>
>>> o = vtkObject()
>>> o.AddObserver(vtkCommand.ModifiedEvent, onObjectModified)
1
>>> o.Modified()
object: vtkObject - event: ModifiedEvent


* Callback with CallData

In case there is a "CallData" value associated with an event, in C++, you
have to cast it from void* to the expected type using reinterpret_cast.
For example, see http://www.vtk.org/Wiki/VTK/Examples/Cxx/Interaction/CallData

The equivalent in python is to set a CallDataType attribute on the
associated python callback. The supported CallDataType are vtk.VTK_STRING,
vtk.VTK_OBJECT, vtk.VTK_INT, vtk.VTK_LONG, vtk.VTK_DOUBLE, vtk.VTK_FLOAT

For example:

>>> def onError(object, event, calldata):
>>>     print("object: %s - event: %s - msg: %s" % (object.GetClassName(), event, calldata))
>>>
>>> onError.CallDataType = vtk.VTK_INT
>>>
>>> lt = vtkLookupTable()
>>> lt.AddObserver(vtkCommand.ErrorEvent, onError)
1
>>> lt.SetTableRange(2,1)
object: vtkLookupTable - event: ErrorEvent - msg: ERROR: In /home/jchris/Projects/VTK6/Common/Core/vtkLookupTable.cxx, line 122
vtkLookupTable (0x6b40b30): Bad table range: [2, 1]


For convenience, the CallDataType can also be specified where the function is first
declared with the help of the "calldata_type" decorator:

>>> @calldata_type(vtk.VTK_INT)
>>> def onError(object, event, calldata):
>>>     print("object: %s - event: %s - msg: %s" % (object.GetClassName(), event, calldata))


Subclassing a VTK class
-----------------------

It is possible to subclass a VTK class from within Python, but it
is NOT possible to override the virtual methods of the class.
The python-level code will be invisible to the VTK C++ code,
the methods that you define will only be visible when you access
the class from Python.

It is therefore not reasonable, for instance, to subclass the
vtkInteractorStyle to provide custom python interaction.  Instead,
you have to do this by adding Observers to the vtkInteractor object.


Class methods
-------------

In C++, if you want to call a method from a superclass you can do the
following:

vtkActor *a = vtkActor::New();
a->vtkProp3D::SetPosition(10,20,50);

The equivalent in python is

>>> a = vtkActor()
>>> vtkProp3D.SetPosition(a,10,20,50)


Void pointers
-------------

As a special feature, a C++ method that requires a void * can
be passed any python object that supports the "buffer" protocol,
which include string objects, numpy arrays and even VTK arrays.
Extreme caution should be applied when using this feature.

Methods that return a void * in C++ will, in Python, return a
string with a hexidecimal number that gives the memory address.


Transmitting data from Python to VTK
------------------------------------

If you have a large block of data in Python (for example a Numeric
array) that you want to access from VTK, then you can do so using
the vtkDataArray.SetVoidArray() method.


Creating a Python object from just the address of a VTK object
--------------------------------------------------------------

When you instantiate a class, you can provide a hexidecimal string
containing the address of an existing vtk object, e.g.
   t = vtkTransform('_1010e068_vtkTransform_p')
The string follows SWIG mangling conventions.  If a wrapper for the
specified object already exists, then that wrapper will be used rather
than a new wrapper being created.  If you want to use this feature
of vtkpython, please think twice.


VTK C++ methods with "pythonic" equivalents
-------------------------------------------

SafeDownCast(): Python knows the real type of any VTK object,
so casts don't do anything.

IsA(): Python provides a built-in isinstance() method.

IsTypeOf(): Python provides a built-in issubclass() method.

GetClassName(): This info is given by o.__class__.__name__


Special VTK types
===================

In addition to VTK objects that are derived from vtkObjectBase, there
are many lightweight types in VTK such as vtkTimeStamp or vtkVariant.
These can usually be distinguished from vtkObjects because they do
not have a C++ "::New()" method for construction.

These types are wrapped in different way from vtkObject-derived classes.
In Python they look like "types" instead of looking like python "classes".
They cannot be subclassed, and the details of memory management are
different because Python actually keeps a copy of the object within
its "wrapper", whereas for vtkObjects it just keeps a pointer.

An incomplete list of these types is as follows:
vtkVariant, vtkTimeStamp, vtkArrayCoordinates, vtkArrayExtents,
vtkArrayExtentsList, vtkArrayRange


Automatic conversion
--------------------

These special types can have several constructors, and the constructors
can be used for automatic type conversion for VTK methods.  For
example, vtkVariantArray has a method InsertNextItem(vtkVariant v), and
vtkVariant has a constructor vtkVariant(int x).  So, you can do this:

>>> variantArray.InsertNextItem(1)

The wrappers will automatically construct a vtkVariant from "1", and
will then pass it as a parameter to InsertNextItem.


Comparison and mapping
----------------------

Some special types can be sorted by value, and some can be used as
dict keys.  Sorting requires the existence of comparison operators
such as "< <= == != > >=" and these are not automatically wrapped.
The use of an object as a dict key requires the computation of a
hash.  Comparison and hashing are supported by vtkVariant and
vtkTimeStamp, and will be supported by other types on a case-by-case
basis.

The reason that all vtkObjects can be easily hashed, while vtk
special types are hard to hash, is that vtkObjects are hashed
by memory address.  This cannot be done for special types, since
they must be hashed by value, not by address.  I.e. vtkVariant(1)
must hash equal to every other vtkVariant(1), even though the
various instances will lie and different memory addresses.


Special attributes available from VTK-Python
============================================

Special vtkobject attributes:
-----------------------------

 o.__class__     the vtkclass that this object is an instance of
 o.__doc__       a description of the class (obtained from C++ header file)
 o.__this__      a string containing the address of the VTK object


Special method attributes:
--------------------------

 m.__doc__       a description of the method (obtained from C++ header file)


Special vtkclass attributes:
----------------------------

 c.__bases__     a tuple of base classes for this class (empty for vtkObject)
 c.__doc__       a description of the class (obtained from C++ header file)
 c.__name__      the name of the class, same as returned by GetClassName()


END OF FILE

