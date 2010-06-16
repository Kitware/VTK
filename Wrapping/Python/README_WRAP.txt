Notes on the Python Wrappers for VTK

First version by David Gobbi: Dec 19, 2002
Updated May 11, 2010 for VTK 6.0

Abstract:
=========

This document provides a detailed description of how VTK objects are
accessed and modified through Python.  The installation of VTK-Python
is covered in README.txt.


Overview:
=========

Nearly all the power of VTK objects are available through Python
(with a few exceptions as noted below).  The C++ symantics are
translated as directly as possible to Python symantics.


The Basics:
===========

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

or if the C++ array is used to return information, you must pass a
Python list or array that has the correct number of slots:

>>> z = [0.0, 0.0, 0.0]
>>> vtkMath.Cross((1,0,0),(0,1,0),z)
>>> print z
[0.0, 0.0, 1.0]

If the C++ method returns a pointer to an array, then that method is
only wrapped if there is a hint that gives the size of the array.
This hint is either given in the 'hints' file e.g. VTK/Wrapping/hints
or by the number specified in the macro used to declare the method in
the C++ header file e.g. vtkGetVectorMacro(Position,float,3):

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

Finally, a C++ method that requires a void * can be passed
any python object that supports the "buffer" protocol, which
include string objects, numpy arrays and even VTK arrays.
No check is done to ensure that the string is the correct
size, and the string's reference count is not incremented.  Extreme
caution should be applied when using this feature.


Unavailable methods
-------------------

A method is not wrapped if
1) its parameter list contains a pointer that isn't a vtkObject
   pointer, char pointer, or void pointer
2) its parameter list contains a reference that isn't a reference to
   a special vtk type like vtkVariant
3) its parameter list contains a multidimensional array
4) it returns a pointer that is not a vtkObject pointer, char pointer,
   or void pointer, unless the method has an entry in the wrapping
   hints file
5) it is an operator method


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

to get a list of all the methods defined in vtkActor.h or if you also
want a listing of all method defined for vtkActor and its superclasses

>>> a = vtk.vtkActor()
>>> dir(a)

will print a very long list of all the methods for object 'a'.


You can also retrieve documentation about VTK classes and methods
from the built-in "docstrings":

>>> help(vtk.vtkActor)
>>> help(vtk.vtkActor.SetUserTransform)
[ lots of info printed, try it yourself ]

For the method documentation, all the different 'signatures' for the
method are given in the original C++ format:

>>> help(vtkActor.SetPosition)
Help on built-in function SetPosition:

SetPosition(...)
    virtual void SetPosition(float _arg1, float _arg2, float _arg3)
    virtual void SetPosition(float _arg[3])

    Set/Get/Add the position of the Prop3D in world coordinates.


Peculiarities and special features
==================================

Deleting a vtkObject:
---------------------

There is no direct equivalent of o->Delete() since Python provides a
mechanism for automatic garbage collection.  The object will be
deleted once there are no remaining references to it either from
inside Python or from other VTK objects.  It is possible to get rid of
the local reference to the object by using the python 'del' command,
i.e. "del o", and this will result in a call to o->Delete() if the
local reference to the object was the last remaining reference to the
object from within Python.


Class methods
-------------

In C++, if you want to call a method from a superclass you can do the
following:

vtkActor *a = vtkActor::New();
a->vtkProp3D::SetPosition(10,20,50);

The equivalent in python is

>>> a = vtkActor()
>>> vtkProp3D.SetPosition(a,10,20,50)

The wrappers also provide an additional feature which isn't very
'pythonic' at all: support for static methods e.g. class methods
that don't require a 'self' (equivalent to the C++ 'this'):

vtkMath::Norm(x);

becomes

>>> vtkMath.Norm(x)

This sort of thing is not allowed by most Python classes!  In fact,
the wrapped VTK classes aren't real Python classes at all, they just
behave similarly.


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


Special VTK types
===================

In addition to VTK objects that are derived from vtkObjectBase, there
are many lightweight types in VTK such as vtkTimeStamp or vtkVariant.
These types are wrapped in different way from vtkObject-based types.
The main difference is that instead of python maintaining a "reference"
to these objects, python makes its own copy of the object.


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
must hash equal to every other vtkVariant(1).


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

