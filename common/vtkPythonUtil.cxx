/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonUtil.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkObject.h"
#include "vtkPythonUtil.h"

//#define VTKPYTHONDEBUG

//--------------------------------------------------------------------
// There are two hash tables associated with the Python wrappers
class vtkPythonUtil
{
public:
  vtkPythonUtil();
  ~vtkPythonUtil();

  PyObject *PointerDict;
  PyObject *ClassDict;
};

//--------------------------------------------------------------------
vtkPythonUtil *vtkPythonHash = NULL;

//--------------------------------------------------------------------
vtkPythonUtil::vtkPythonUtil()
{
  this->PointerDict = PyDict_New();
  this->ClassDict = PyDict_New();
}

//--------------------------------------------------------------------
vtkPythonUtil::~vtkPythonUtil()
{
  Py_DECREF(this->PointerDict);
  Py_DECREF(this->ClassDict);
}

//--------------------------------------------------------------------
static int PyVTKObject_PyPrint(PyObject *self, FILE *fp, int)
{
  vtkObject *op;
  ostrstream buf;

  op = (vtkObject *)((PyVTKObject *)self)->vtk_ptr;
  op->Print(buf);
  buf.put('\0');
  fprintf(fp,"%s",buf.str());
  delete buf.str();
  return 0;
}

//--------------------------------------------------------------------
static PyObject *PyVTKObject_PyString(PyObject *self)
{
  vtkObject *op;
  PyObject *tempH;
  ostrstream buf;

  op = (vtkObject *)((PyVTKObject *)self)->vtk_ptr;
  op->Print(buf);
  buf.put('\0');
  tempH = PyString_FromString(buf.str());
  delete buf.str();
  return tempH;
}

//--------------------------------------------------------------------
static PyObject *PyVTKObject_PyRepr(PyObject *self)
{
  char buf[255];
  sprintf(buf,"<%s.%s %s at %p>",
	  ((PyVTKObject *)self)->vtk_class->vtk_module,
	  ((PyVTKObject *)self)->vtk_class->vtk_name,
	  self->ob_type->tp_name,self);
  
  return PyString_FromString(buf);
}

//--------------------------------------------------------------------
static PyObject *PyVTKObject_PyGetAttr(PyObject *self, char *name)
{
  PyVTKClass *pyclass = ((PyVTKObject *)self)->vtk_class;
  PyObject *bases;

  if (name[0] == '_')
    {
      if (strcmp(name,"__class__") == 0)
	{
	  Py_INCREF(((PyVTKObject *)self)->vtk_class);
	  return (PyObject *)((PyVTKObject *)self)->vtk_class;
	}
      
      if (strcmp(name,"__this__") == 0)
	{
	  char buf[256];
	  sprintf(buf,"%s_p",((PyVTKObject *)self)->vtk_ptr->GetClassName());
	  return PyString_FromString(
	         vtkPythonManglePointer(((PyVTKObject *)self)->vtk_ptr,buf));
	}
      
      if (strcmp(name,"__doc__") == 0)
	{
	  return PyString_FromString(pyclass->vtk_doc);
	}
      
      if (strcmp(name,"__methods__") == 0)
	{
	  PyMethodDef *meth = pyclass->vtk_methods;
	  PyObject *lst;
	  int i, n, m;
	  
	  n = 0;
	  if ((lst = PyList_New(0)) == NULL)
	    {
	      return NULL;
	    }
	  
	  bases = NULL;
	  while (pyclass != NULL)
	    {
	      m = 0;
	      for (meth = pyclass->vtk_methods; meth->ml_name; meth++)
		{
		  for (i = 0; i < n; i++)
		    {
		      if (strcmp(PyString_AsString(PyList_GetItem(lst,i)),
				 meth->ml_name) == 0)
			{
			  break;
			}
		    }
		  if (i == n)
		    {
		      if (PyList_Append(lst,PyString_FromString(meth->ml_name)) == -1)
			{
			  Py_DECREF(lst);
			  return NULL;
			}
		      m++;
		    }
		}
	      n += m;
	      bases = ((PyVTKClass *)pyclass)->vtk_bases;
	      pyclass = NULL;
	      if (PyTuple_Size(bases))
		{
		  pyclass = (PyVTKClass *)PyTuple_GetItem(bases,0);
		}
	    }
	  PyList_Sort(lst);
	  return lst;
	}
      
      if (strcmp(name,"__members__") == 0)
	{
	  PyObject *lst;
	  if ((lst = PyList_New(5)) != NULL)
	    {
	      PyList_SetItem(lst,0,PyString_FromString("__class__"));
	      PyList_SetItem(lst,1,PyString_FromString("__doc__"));
	      PyList_SetItem(lst,2,PyString_FromString("__members__"));
	      PyList_SetItem(lst,3,PyString_FromString("__methods__"));
	      PyList_SetItem(lst,4,PyString_FromString("__this__"));
	    }
	  return lst;
	}
    }
  
  while (pyclass != NULL)
    {
      PyMethodDef *m;
      for (m = pyclass->vtk_methods; m->ml_name; m++)
	{
	  if (name[0] == m->ml_name[0] && strcmp(name+1, m->ml_name+1) == 0)
	    {
	      return PyCFunction_New(m, self);
	    }
	} 
      bases = ((PyVTKClass *)pyclass)->vtk_bases;
      pyclass = NULL;
      if (PyTuple_Size(bases))
	{
	  pyclass = (PyVTKClass *)PyTuple_GetItem(bases,0);
	}
    }
  
  PyErr_SetString(PyExc_AttributeError, name);
  return NULL;
}

//--------------------------------------------------------------------
static void PyVTKObject_PyDelete(PyObject *self)
{
  vtkObject *ptr = (vtkObject *)((PyVTKObject *)self)->vtk_ptr;

  vtkPythonDeleteObjectFromHash(self);
  ptr->Delete();
  Py_DECREF((PyObject *)((PyVTKObject *)self)->vtk_class);
  PyMem_DEL(self);
}

//--------------------------------------------------------------------
static PyTypeObject PyVTKObjectType = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  "vtkobject",                           // tp_name
  sizeof(PyVTKObject),                   // tp_basicsize
  0,                                     // tp_itemsize
  (destructor)PyVTKObject_PyDelete,      // tp_dealloc
  (printfunc)PyVTKObject_PyPrint,        // tp_print
  (getattrfunc)PyVTKObject_PyGetAttr,    // tp_getattr
  (setattrfunc)0,                        // tp_setattr
  (cmpfunc)0,                            // tp_compare
  (reprfunc)PyVTKObject_PyRepr,          // tp_repr
  0,                                     // tp_as_number 
  0,                                     // tp_as_sequence
  0,                                     // tp_as_mapping
  (hashfunc)0,                           // tp_hash
  (ternaryfunc)0,                        // tp_call
  (reprfunc)PyVTKObject_PyString,        // tp_string
  (getattrofunc)0,                       // tp_getattro
  (setattrofunc)0,                       // tp_setattro
  0,                                     // tp_as_buffer
  0,                                     // tp_flags
  "A VTK object.  Special attributes are:  __class__ (the class that this object belongs to), __doc__ (the docstring for the class), __methods__ (a list of all methods for this object), and __this__ (a string that contains the hexidecimal address of the underlying VTK object)"  // tp_doc
};

int PyVTKObject_Check(PyObject *obj)
{
  return (obj->ob_type == &PyVTKObjectType);
}

PyObject *PyVTKObject_New(PyObject *vtkclass, vtkObject *ptr)
{
  if (ptr)
    {
    ptr->Register(NULL);
    }
  else if (((PyVTKClass *)vtkclass)->vtk_new != NULL)
    {
    ptr = ((PyVTKClass *)vtkclass)->vtk_new();
    }
  else
    {
    PyErr_SetString(PyExc_TypeError,
		    "this is an abstract class and cannot be instantiated");
    return 0;
    }
  PyVTKObject *self = PyObject_NEW(PyVTKObject, &PyVTKObjectType);
  self->vtk_ptr = ptr;
  self->vtk_class = (PyVTKClass *)
    PyDict_GetItemString(vtkPythonHash->ClassDict,(char *)ptr->GetClassName());
  
  // if specific type is not a python vtkclass (i.e. if it was loaded by
  // the Factories or some such thing) just use the originally specified class
  if (self->vtk_class == NULL)
    {
      self->vtk_class = (PyVTKClass *)vtkclass;
    }
  
  Py_INCREF(self->vtk_class);
  
  vtkPythonAddObjectToHash((PyObject *)self,ptr);
  
  return (PyObject *)self;
}

//--------------------------------------------------------------------
static int PyVTKClass_PyPrint(PyObject *self, FILE *fp, int)
{
  fprintf(fp,"%s.%s",((PyVTKClass *)self)->vtk_module,
	             ((PyVTKClass *)self)->vtk_name);
  return 0;
}

//--------------------------------------------------------------------
static PyObject *PyVTKClass_PyString(PyObject *self)
{
  char buf[255];
  sprintf(buf,"%s.%s",((PyVTKClass *)self)->vtk_module,
	              ((PyVTKClass *)self)->vtk_name);

  return PyString_FromString(buf);
}

//--------------------------------------------------------------------
static PyObject *PyVTKClass_PyRepr(PyObject *self)
{
  char buf[255];
  sprintf(buf,"<%s %s.%s at %p>",self->ob_type->tp_name,
 	                        ((PyVTKClass *)self)->vtk_module,
 	                        ((PyVTKClass *)self)->vtk_name,self);
  
  return PyString_FromString(buf);
}

//--------------------------------------------------------------------
static PyObject *PyVTKClass_PyCall(PyObject *self, PyObject *arg, PyObject *kw)
{
  if (kw != NULL)
    {
      PyErr_SetString(PyExc_TypeError,
		      "this function takes no keyword arguments");
      return NULL;
    }
  if (PyArg_ParseTuple(arg,""))
    {
      return PyVTKObject_New(self,NULL);
    }
  PyErr_Clear();
  if (PyArg_ParseTuple(arg,"O",&arg))
    {
      return vtkPythonGetObjectFromObject(arg, ((PyVTKClass *)self)->vtk_name);
    }
  PyErr_Clear();
  PyErr_SetString(PyExc_TypeError,
		  "function requires 0 or 1 arguments");

  return NULL;
}

//--------------------------------------------------------------------
static PyObject *PyVTKClass_PyGetAttr(PyObject *self, char *name)
{
  PyVTKClass *pyclass = (PyVTKClass *)self;
  PyObject *bases;

  if (name[0] == '_')
    {
      if (strcmp(name,"__bases__") == 0)
	{
	  Py_INCREF(((PyVTKClass *)self)->vtk_bases);
	  return ((PyVTKClass *)self)->vtk_bases;
	}
      
      if (strcmp(name,"__name__") == 0)
	{
	  return PyString_FromString(pyclass->vtk_name);
	}
      
      if (strcmp(name,"__module__") == 0)
	{
	  return PyString_FromString(pyclass->vtk_module);
	}
      
      if (strcmp(name,"__doc__") == 0)
	{
	  return PyString_FromString(pyclass->vtk_doc);
	}
      
      if (strcmp(name,"__methods__") == 0)
	{
	  PyMethodDef *meth = pyclass->vtk_methods;
	  PyObject *lst;
	  int i, n;
	  
	  for (n = 0; meth[n].ml_name; n++);
	  
	  if ((lst = PyList_New(n)) != NULL)
	    {
	      meth = pyclass->vtk_methods;
	      for (i = 0; i < n; i++)
		{
		  PyList_SetItem(lst, i, PyString_FromString(meth[i].ml_name));
		}
	      PyList_Sort(lst);
	    }
	  return lst;
	}
      
      if (strcmp(name,"__members__") == 0)
	{
	  PyObject *lst;
	  if ((lst = PyList_New(6)) != NULL)
	    {
	      PyList_SetItem(lst,0,PyString_FromString("__bases__"));
	      PyList_SetItem(lst,1,PyString_FromString("__doc__"));
	      PyList_SetItem(lst,2,PyString_FromString("__members__"));
	      PyList_SetItem(lst,3,PyString_FromString("__methods__"));
	      PyList_SetItem(lst,4,PyString_FromString("__module__"));
	      PyList_SetItem(lst,5,PyString_FromString("__name__"));
	    }
	  return lst;
	}
    }
  
  while (pyclass != NULL)
    {
      PyMethodDef *meth;
      for (meth = pyclass->vtk_methods; meth->ml_name; meth++)
	{
	  if (name[0] == meth->ml_name[0] && strcmp(name+1, meth->ml_name+1) == 0)
	    {
	      return PyCFunction_New(meth, self);
	    }
	} 
      bases = ((PyVTKClass *)pyclass)->vtk_bases;
      pyclass = NULL;
      if (PyTuple_Size(bases))
	{
	  pyclass = (PyVTKClass *)PyTuple_GetItem(bases,0);
	}
    }
  
  PyErr_SetString(PyExc_AttributeError, name);
  return NULL;
}

//--------------------------------------------------------------------
static void PyVTKClass_PyDelete(PyObject *self)
{
  PyMem_DEL(self);
}

//--------------------------------------------------------------------
static PyTypeObject PyVTKClassType = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  "vtkclass",                            // tp_name
  sizeof(PyVTKClass),                    // tp_basicsize
  0,                                     // tp_itemsize
  (destructor)PyVTKClass_PyDelete,       // tp_dealloc
  (printfunc)PyVTKClass_PyPrint,         // tp_print
  (getattrfunc)PyVTKClass_PyGetAttr,     // tp_getattr
  (setattrfunc)0,                        // tp_setattr
  (cmpfunc)0,                            // tp_compare
  (reprfunc)PyVTKClass_PyRepr,           // tp_repr
  0,                                     // tp_as_number 
  0,                                     // tp_as_sequence
  0,                                     // tp_as_mapping
  (hashfunc)0,                           // tp_hash
  (ternaryfunc)PyVTKClass_PyCall,        // tp_call
  (reprfunc)PyVTKClass_PyString,         // tp_string
  (getattrofunc)0,                       // tp_getattro
  (setattrofunc)0,                       // tp_setattro
  0,                                     // tp_as_buffer
  0,                                     // tp_flags
  "A generator for VTK objects.  Special attributes are: __bases__ (a tuple of base classes), __doc__ (the docstring for the class), __name__ (the name of class), __methods__ (methods for this class, not including inherited methods), and __module__ (module that the class is defined in)." // tp_doc
};

int PyVTKClass_Check(PyObject *obj)
{
  return (obj->ob_type == &PyVTKClassType);
}

PyObject *PyVTKClass_New(vtknewfunc constructor,
			 PyMethodDef *methods,
			 char *classname, char *modulename, char *docstring,
			 PyObject *base)
{
  PyVTKClass *self = NULL;
  if (vtkPythonHash)
    {
      (PyVTKClass *)PyDict_GetItemString(vtkPythonHash->ClassDict,classname);
    }
  if (self)
    {
      Py_INCREF((PyObject *)self);
    }
  else
    {
      self = PyObject_NEW(PyVTKClass, &PyVTKClassType);
      self->vtk_methods = methods;
      self->vtk_new = constructor;
      self->vtk_name = classname;
      self->vtk_module = modulename;
      self->vtk_doc = docstring;
      if (base)
	{
	  self->vtk_bases = PyTuple_New(1);
	  PyTuple_SET_ITEM(((PyVTKClass *)self)->vtk_bases, 0, base);
	}
      else
	{
	  self->vtk_bases = PyTuple_New(0);
	}
      vtkPythonAddClassToHash((PyObject *)self,classname);
    }
  
  return (PyObject *)self;
}
  
//--------------------------------------------------------------------
static int PyVTKSpecialObject_PyPrint(PyObject *self, FILE *fp, int)
{
  fprintf(fp,"%s",((PyVTKSpecialObject *)self)->vtk_name);
  return 0;
}

//--------------------------------------------------------------------
static PyObject *PyVTKSpecialObject_PyString(PyObject *self)
{
  char buf[255];
  sprintf(buf,"%s",((PyVTKSpecialObject *)self)->vtk_name);

  return PyString_FromString(buf);
}

//--------------------------------------------------------------------
static PyObject *PyVTKSpecialObject_PyRepr(PyObject *self)
{
  char buf[255];
  sprintf(buf,"<%s %s at %p>", self->ob_type->tp_name, 
	                       ((PyVTKSpecialObject *)self)->vtk_name,
	                       self);
  return PyString_FromString(buf);
}

//--------------------------------------------------------------------
static PyObject *PyVTKSpecialObject_PyGetAttr(PyObject *self, char *name)
{
  PyVTKSpecialObject *pyobject = (PyVTKSpecialObject *)self;
  PyMethodDef *meth;

  if (name[0] == '_')
    {
      if (strcmp(name,"__name__") == 0)
	{
	  return PyString_FromString(pyobject->vtk_name);
	}
      if (strcmp(name,"__doc__") == 0)
	{
	  return PyString_FromString(pyobject->vtk_doc);
	}
      if (strcmp(name,"__methods__") == 0)
	{
	  PyMethodDef *meth = pyobject->vtk_methods;
	  PyObject *lst;
	  int i, n;
	  
	  for (n = 0; meth[n].ml_name; n++);
	  
	  if ((lst = PyList_New(n)) != NULL)
	    {
	      meth = pyobject->vtk_methods;
	      for (i = 0; i < n; i++)
		{
		  PyList_SetItem(lst, i, PyString_FromString(meth[i].ml_name));
		}
	      PyList_Sort(lst);
	    }
	  return lst;
	}
      
      if (strcmp(name,"__members__") == 0)
	{
	  PyObject *lst;
	  if ((lst = PyList_New(4)) != NULL)
	    {
	      PyList_SetItem(lst,0,PyString_FromString("__doc__"));
	      PyList_SetItem(lst,1,PyString_FromString("__members__"));
	      PyList_SetItem(lst,2,PyString_FromString("__methods__"));
	      PyList_SetItem(lst,3,PyString_FromString("__name__"));
	    }
	  return lst;
	}
    }  

  for (meth = pyobject->vtk_methods; meth->ml_name; meth++)
    {
      if (name[0] == meth->ml_name[0] && strcmp(name+1, meth->ml_name+1) == 0)
	{
	  return PyCFunction_New(meth, self);
	}
    } 
  
  PyErr_SetString(PyExc_AttributeError, name);
  return NULL;
}

//--------------------------------------------------------------------
static void PyVTKSpecialObject_PyDelete(PyObject *self)
{
  // commented out the following line because it is not allowed in C++
  // even though some compilers seem to do something with with it.
  // basically the gist is that you cannto delete a void * because the
  // compiler doesn't have any idea how big the void is.
  //
  //delete ((PyVTKSpecialObject *)self)->vtk_ptr;
  ((PyVTKSpecialObject *)self)->vtk_ptr = NULL;

  PyMem_DEL(self);
}

//--------------------------------------------------------------------
static PyTypeObject PyVTKSpecialObjectType = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  "vtkspecialobject",                    // tp_name
  sizeof(PyVTKSpecialObject),            // tp_basicsize
  0,                                     // tp_itemsize
  (destructor)PyVTKSpecialObject_PyDelete, // tp_dealloc
  (printfunc)PyVTKSpecialObject_PyPrint, // tp_print
  (getattrfunc)PyVTKSpecialObject_PyGetAttr, // tp_getattr
  (setattrfunc)0,                        // tp_setattr
  (cmpfunc)0,                            // tp_compare
  (reprfunc)PyVTKSpecialObject_PyRepr,   // tp_repr
  0,                                     // tp_as_number 
  0,                                     // tp_as_sequence
  0,                                     // tp_as_mapping
  (hashfunc)0,                           // tp_hash
  (ternaryfunc)0,                        // tp_call
  (reprfunc)PyVTKSpecialObject_PyString, // tp_string
  (getattrofunc)0,                       // tp_getattro
  (setattrofunc)0,                       // tp_setattro
  0,                                     // tp_as_buffer
  0,                                     // tp_flags
  "vtkspecialobject - a vtk object not derived from vtkObject." // tp_doc
};

int PyVTKSpecialObject_Check(PyObject *obj)
{
  return (obj->ob_type == &PyVTKSpecialObjectType);
}

PyObject *PyVTKSpecialObject_New(void *ptr, PyMethodDef *methods,
				 char *classname, char *docstring)
{
  PyVTKSpecialObject *self = PyObject_NEW(PyVTKSpecialObject, 
					  &PyVTKSpecialObjectType);
  self->vtk_ptr = ptr;
  self->vtk_methods = methods;
  self->vtk_name = classname;
  self->vtk_doc = docstring;
  
  return (PyObject *)self;
}

//--------------------------------------------------------------------
vtkObject *PyArg_VTKParseTuple(PyObject *self, PyObject *args, 
			       char *format, ...)
{
  vtkObject *obj = NULL;
  va_list va;
  va_start(va, format);

  /* check if this was called as an unbound method */
  if (self->ob_type == &PyVTKClassType)
    {
      int n = PyTuple_Size(args);
      PyVTKClass *vtkclass = (PyVTKClass *)self;
      
      if (n == 0 || (self = PyTuple_GetItem(args,0)) == NULL ||
	  self->ob_type != &PyVTKObjectType ||
	  !((PyVTKObject *)self)->vtk_ptr->IsA(vtkclass->vtk_name))
	{
	  char buf[256];
	  sprintf(buf,"unbound method requires a %s as the first argument",
		  vtkclass->vtk_name);
	  PyErr_SetString(PyExc_ValueError,buf);
	  return NULL;
	}
      // re-slice the args to remove 'self'
      args = PyTuple_GetSlice(args,1,n);
      if (PyArg_VaParse(args,format,va))
	{
	  obj = ((PyVTKObject *)self)->vtk_ptr;
	}
      Py_DECREF(args);
    }
  /* it was called as a bound method */
  else
    {
      if (PyArg_VaParse(args,format,va))
	{
	  obj = ((PyVTKObject *)self)->vtk_ptr;
	}
    }    
  return obj;
}

//--------------------------------------------------------------------
void vtkPythonAddClassToHash(PyObject *vtkclass, char *classname)
{
  if (vtkPythonHash == NULL)
    {
      vtkPythonHash = new vtkPythonUtil();
    }

#ifdef VTKPYTHONDEBUG
  //  vtkGenericWarningMacro("Adding an type " << type << " to hash ptr");
#endif  

  // lets make sure it isn't already there
  if (PyDict_GetItemString(vtkPythonHash->ClassDict,classname))
    {
#ifdef VTKPYTHONDEBUG
      vtkGenericWarningMacro("Attempt to add type to the hash when already there!!!");
#endif
      return;
    }

  PyDict_SetItemString(vtkPythonHash->ClassDict,classname,vtkclass);

#ifdef VTKPYTHONDEBUG
  //  vtkGenericWarningMacro("Added type to hash type = " << typeObject);
#endif  
}  

//--------------------------------------------------------------------
void vtkPythonAddObjectToHash(PyObject *obj, vtkObject *ptr)
{
  if (vtkPythonHash == NULL)
    {
      vtkPythonHash = new vtkPythonUtil();
    }

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Adding an object to hash ptr = " << ptr);
#endif  

  ((PyVTKObject *)obj)->vtk_ptr = (vtkObject *)ptr;
  PyObject *pyPtr1 = PyInt_FromLong((long)ptr);
  PyObject *pyPtr2 = PyInt_FromLong((long)obj);
  PyDict_SetItem(vtkPythonHash->PointerDict,pyPtr1,pyPtr2);
  Py_DECREF(pyPtr1);
  Py_DECREF(pyPtr2);

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Added object to hash obj= " << obj << " " 
			 << ((PyVTKObject *)obj)->vtk_ptr);
#endif  
}  

//--------------------------------------------------------------------
void vtkPythonDeleteObjectFromHash(PyObject *obj)
{
#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Deleting an object from hash obj = " << obj << " "
			 << ((PyVTKObject *)obj)->vtk_ptr);
#endif  

  void *ptr = (void *)((PyVTKObject *)obj)->vtk_ptr;
  PyObject *pyPtr = PyInt_FromLong((long)ptr);
  PyDict_DelItem(vtkPythonHash->PointerDict,pyPtr);
  Py_DECREF(pyPtr);  
}

//--------------------------------------------------------------------
static PyObject *vtkFindNearestBase(vtkObject *ptr);

PyObject *vtkPythonGetObjectFromPointer(vtkObject *ptr)
{
  PyObject *obj = 0;

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Checking into pointer " << ptr);
#endif
  
  if (ptr)
    {
      PyObject *pyPtr1 = PyInt_FromLong((long)ptr);
      PyObject *pyPtr2 = PyDict_GetItem(vtkPythonHash->PointerDict,pyPtr1);
      Py_DECREF(pyPtr1);
      
      if (pyPtr2)
	{
	  obj = (PyObject *)PyInt_AsLong(pyPtr2);
	}
    }
  else
    {
      obj = Py_None;
    }
  
#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Checking into pointer " << ptr << " obj = " << obj);
#endif
  
  if (obj == NULL)
    {
      PyObject *vtkclass = PyDict_GetItemString(vtkPythonHash->ClassDict,
						(char *)((vtkObject *)ptr)->GetClassName());
      
      // if the class was not in the hash, then find the nearest base class
      // that is and associate ptr->GetClassName() with that base class
      if (vtkclass == NULL)
	{
	  vtkclass = vtkFindNearestBase(ptr);
	  vtkPythonAddClassToHash(vtkclass, 
				  (char *)((vtkObject *)ptr)->GetClassName());
	}      
      
      obj = PyVTKObject_New(vtkclass,ptr);
    }
  else
    {
      Py_INCREF(obj);
    }
  
  return obj;
}

// this is a helper function to find the nearest base class for an
// object whose class is not in the ClassDict
static PyObject *vtkFindNearestBase(vtkObject *ptr)
{
  PyObject *classes = PyDict_Values(vtkPythonHash->ClassDict);
  PyObject *nearestbase = NULL;
  int maxdepth = 0;
  int n = PyList_Size(classes);
  int i, depth;

  for (i = 0; i < n; i++)
    {
      PyObject *pyclass = PyList_GetItem(classes,i);
      // check to see if ptr is derived from this class
      if (ptr->IsA(((PyVTKClass *)pyclass)->vtk_name))
	{ 
	  PyObject *cls = pyclass;
	  PyObject *bases = ((PyVTKClass *)pyclass)->vtk_bases;
	  // count the heirarchy depth for this class
	  for (depth = 0; PyTuple_Size(bases) != 0; depth++)
	    {
	      cls = PyTuple_GetItem(bases,0);
	      bases = ((PyVTKClass *)cls)->vtk_bases;
	    }
	  // we want the class that is furthest from vtkObject
	  if (depth > maxdepth)
	    {
	      maxdepth = depth;
	      nearestbase = pyclass;
	    }
	}
    }
  
  Py_DECREF(classes);

  return nearestbase;
}

//--------------------------------------------------------------------
vtkObject *vtkPythonGetPointerFromObject(PyObject *obj, char *result_type)
{ 
  vtkObject *ptr;

  // convert Py_None to NULL every time
  if (obj == Py_None)
    {
      return NULL;
    }

  // check to ensure it is a vtk object
  if (obj->ob_type != &PyVTKObjectType)
    {
    obj = PyObject_GetAttrString(obj,"__vtk__");
    if (obj)
      {
      PyObject *arglist = Py_BuildValue("()");
      PyObject *result = PyEval_CallObject(obj, arglist);
      Py_DECREF(arglist);
      Py_DECREF(obj);
      if (result == NULL)
	{
	return NULL;
	}
      if (result->ob_type != &PyVTKObjectType)
	{
	PyErr_SetString(PyExc_ValueError,"__vtk__() doesn't return a VTK object");
	Py_DECREF(result);
	return NULL;
	}
      else
	{
	ptr = ((PyVTKObject *)result)->vtk_ptr;
	Py_DECREF(result);
	}
      }
    else
      {
#ifdef VTKPYTHONDEBUG
	vtkGenericWarningMacro("Object " << obj << " is not a VTK object!!");
#endif  
	PyErr_SetString(PyExc_ValueError,"method requires a VTK object");
	return NULL;
      }
    }
  else
    {
    ptr = ((PyVTKObject *)obj)->vtk_ptr;
    }
  
#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Checking into obj " << obj << " ptr = " << ptr);
#endif  

  if (ptr->IsA(result_type))
    {
#ifdef VTKPYTHONDEBUG
      vtkGenericWarningMacro("Got obj= " << obj << " ptr= " << ptr << " " << result_type);
#endif  
      return ptr;
    }
  else
    {
      char error_string[256];
#ifdef VTKPYTHONDEBUG
      vtkGenericWarningMacro("vtk bad argument, type conversion failed.");
#endif
      sprintf(error_string,"method requires a %s, a %s was provided.",
	      result_type,((vtkObject *)ptr)->GetClassName());
      PyErr_SetString(PyExc_ValueError,error_string);
      return NULL;
    }
}

PyObject *vtkPythonGetObjectFromObject(PyObject *arg, const char *type)
{
  if (PyString_Check(arg))
    {
      char *ptrText = PyString_AsString(arg);
      
      vtkObject *ptr;
      char typeCheck[256];  // typeCheck is currently not used
      int i = sscanf(ptrText,"_%lx_%s",(long *)&ptr,typeCheck);
      
      if (i <= 0)
	{
	  i = sscanf(ptrText,"Addr=0x%lx",(long *)&ptr);
	}      
      if (i <= 0)
	{
	  i = sscanf(ptrText,"%lx",(long *)&ptr);
	}
      if (i <= 0)
	{
	  PyErr_SetString(PyExc_ValueError,"could not extract hexidecimal address from argument string");
	  return NULL;
	}
      
      if (!ptr->IsA(type))
	{
	  char error_string[256];
	  sprintf(error_string,"method requires a %s address, a %s address was provided.",
		  type,((vtkObject *)ptr)->GetClassName());
	  PyErr_SetString(PyExc_TypeError,error_string);
	  return NULL;
	}
      
      return vtkPythonGetObjectFromPointer(ptr);
    }
  
  PyErr_SetString(PyExc_TypeError,"method requires a string argument");
  return NULL;
}

//--------------------------------------------------------------------
// mangle a void pointer into a SWIG-style string
char *vtkPythonManglePointer(void *ptr, const char *type)
{
  static char ptrText[128];
  sprintf(ptrText,"_%*.*lx_%s",2*(int)sizeof(void *),2*(int)sizeof(void *),
	  (long)ptr,type);
  return ptrText;
}

//--------------------------------------------------------------------
// unmangle a void pointer from a SWIG-style string
void *vtkPythonUnmanglePointer(char *ptrText, int *len, const char *type)
{
  int i; 
  void *ptr;
  char typeCheck[128];
  if (*len < 128)
    {
    i = sscanf(ptrText,"_%lx_%s",(long *)&ptr,typeCheck);
    if (strcmp(type,typeCheck) == 0)
      { // sucessfully unmangle
	*len = 0;
	return ptr;
      }
    else if (i == 2)
      { // mangled pointer of wrong type
	*len = -1;
	return NULL;
      }
    }
  // couldn't unmangle: return string as void pointer if it didn't look
  // like a SWIG mangled pointer
  return (void *)ptrText;
}

//--------------------------------------------------------------------
void vtkPythonVoidFunc(void *arg)
{
  PyObject *arglist, *result;
  PyObject *func = (PyObject *)arg;

  arglist = Py_BuildValue("()");

  result = PyEval_CallObject(func, arglist);
  Py_DECREF(arglist);

  if (result)
    {
      Py_XDECREF(result);
    }
  else
    {
      if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt))
	{
	  cerr << "Caught a Ctrl-C within python, exiting program.\n";
	  Py_Exit(1);
	}
      PyErr_Print();
    }
}

//--------------------------------------------------------------------
void vtkPythonVoidFuncArgDelete(void *arg)
{
  PyObject *func = (PyObject *)arg;
  if (func)
    {
      Py_DECREF(func);
    }
}
  
//--------------------------------------------------------------------
vtkPythonCommand::vtkPythonCommand()
{ 
  this->obj = NULL;
}

vtkPythonCommand::~vtkPythonCommand()
{ 
  if (this->obj)
    {
      Py_DECREF(this->obj);
    }
  this->obj = NULL;
}

void vtkPythonCommand::SetObject(PyObject *o)
{ 
  this->obj = o; 
}

void vtkPythonCommand::Execute(vtkObject *ptr, unsigned long eventtype, 
			       void *)
{
  PyObject *arglist, *result, *obj2;
  const char *eventname;

  if (ptr && ptr->GetReferenceCount() > 0)
    {
      obj2 = vtkPythonGetObjectFromPointer(ptr);
    }
  else
    {
      obj2 = Py_None;
      Py_XINCREF(Py_None);
    }

  eventname = this->GetStringFromEventId(eventtype);
  
  arglist = Py_BuildValue("(Ns)",obj2,eventname);
  
  result = PyEval_CallObject(this->obj, arglist);
  Py_DECREF(arglist);
  
  if (result)
    {
      Py_XDECREF(result);
    }
  else
    {
      if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt))
	{
	  cerr << "Caught a Ctrl-C within python, exiting program.\n";
	  Py_Exit(1);
	}
      PyErr_Print();
    }
}
//--------------------------------------------------------------------



