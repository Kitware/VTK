/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonUtil.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
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
class vtkPythonUtil
{
public:
  vtkPythonUtil();
  ~vtkPythonUtil();

  PyObject *PointerDict;
  PyObject *TypeDict;
};

//--------------------------------------------------------------------
vtkPythonUtil *vtkPythonHash = NULL;

//--------------------------------------------------------------------
vtkPythonUtil::vtkPythonUtil()
{
  this->PointerDict = PyDict_New();
  this->TypeDict = PyDict_New();
}

//--------------------------------------------------------------------
vtkPythonUtil::~vtkPythonUtil()
{
  Py_DECREF(this->PointerDict);
  Py_DECREF(this->TypeDict);
}

//--------------------------------------------------------------------
void vtkPythonAddTypeToHash(PyTypeObject *typeObject, char *type)
{
  if (vtkPythonHash == NULL)
    {
    vtkPythonHash = new vtkPythonUtil();
    }

#ifdef VTKPYTHONDEBUG
  //  vtkGenericWarningMacro("Adding an type " << type << " to hash ptr");
#endif  

  // lets make sure it isn't already there
  if (PyDict_GetItemString(vtkPythonHash->TypeDict,type))
    {
#ifdef VTKPYTHONDEBUG
    vtkGenericWarningMacro("Attempt to add type to the hash when already there!!!");
#endif
    return;
    }

  PyObject *pyType = PyInt_FromLong((long)typeObject);
  PyDict_SetItemString(vtkPythonHash->TypeDict,type,pyType);
  Py_DECREF(pyType);

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

  ((PyVTKObject *)obj)->ptr = (vtkObject *)ptr;
  PyObject *pyPtr1 = PyInt_FromLong((long)ptr);
  PyObject *pyPtr2 = PyInt_FromLong((long)obj);
  PyDict_SetItem(vtkPythonHash->PointerDict,pyPtr1,pyPtr2);
  Py_DECREF(pyPtr1);
  Py_DECREF(pyPtr2);

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Added object to hash obj= " << obj << " " 
			 << ((PyVTKObject *)obj)->ptr);
#endif  
}  

//--------------------------------------------------------------------
void vtkPythonDeleteObjectFromHash(PyObject *obj)
{
#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Deleting an object from hash obj = " << obj << " "
			 << ((PyVTKObject *)obj)->ptr);
#endif  

  void *ptr = (void *)((PyVTKObject *)obj)->ptr;
  PyObject *pyPtr = PyInt_FromLong((long)ptr);
  PyDict_DelItem(vtkPythonHash->PointerDict,pyPtr);
  Py_DECREF(pyPtr);  
}

//--------------------------------------------------------------------
PyObject *vtkPythonGetObjectFromPointer(vtkObject *ptr)
{
#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Checking into pointer " << ptr);
#endif
  
  PyObject *pyPtr1 = PyInt_FromLong((long)ptr);
  PyObject *pyPtr2 = PyDict_GetItem(vtkPythonHash->PointerDict,pyPtr1);
  Py_DECREF(pyPtr1);

  PyObject *obj = 0;
  if (pyPtr2)
    {
    obj = (PyObject *)PyInt_AsLong(pyPtr2);
    }

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Checking into pointer " << ptr << " obj = " << obj);
#endif

  if (obj == 0)
    {
    PyObject *pyType = PyDict_GetItemString(vtkPythonHash->TypeDict,
			     (char *)((vtkObject *)ptr)->GetClassName());
    PyTypeObject *typeObject = (PyTypeObject *)PyInt_AsLong(pyType);
    obj = PyObject_NEW(PyObject, typeObject);
    if (obj == NULL)
      {
      return NULL;
      }
    vtkPythonAddObjectToHash(obj,ptr);
    ((vtkObject *)ptr)->Register(NULL);
    }
  else
    {
    Py_INCREF(obj);
    }

  return obj;
}

//--------------------------------------------------------------------
vtkObject *vtkPythonGetPointerFromObject(PyObject *obj, char *result_type)
{ 
  // if the type name is in the hash, it is a VTK object
  if (!PyDict_GetItemString(vtkPythonHash->TypeDict,obj->ob_type->tp_name))
    {
#ifdef VTKPYTHONDEBUG
    vtkGenericWarningMacro("Object " << obj << " is not a VTK object!!");
#endif  
    PyErr_SetString(PyExc_ValueError,"method requires a VTK object");
    return NULL;
    }   

  vtkObject *ptr = ((PyVTKObject *)obj)->ptr;

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

//--------------------------------------------------------------------
// create a python object from a python string object
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
      PyErr_SetString(PyExc_ValueError,error_string);
      return NULL;
      }

    return vtkPythonGetObjectFromPointer(ptr);
    }

  PyErr_SetString(PyExc_ValueError,"method requires a string argument");
  return NULL;
}

//--------------------------------------------------------------------
// mangle a void pointer into a SWIG-style string
char *vtkPythonManglePointer(void *ptr, const char *type)
{
  static char ptrText[128];
  sprintf(ptrText,"_%*.*lx_%s",(int)sizeof(void *),(int)sizeof(void *),
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
  
