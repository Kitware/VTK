/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonUtil.h
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
#include "vtkTimeStamp.h"
#include "Python.h"
#include "vtkCommand.h"

#if defined(WIN32)
#if defined(vtkCommonPython_EXPORTS)
  #define VTK_PYTHON_EXPORT __declspec( dllexport )
#else
    #define VTK_PYTHON_EXPORT __declspec( dllimport )
#endif
#else
  #define VTK_PYTHON_EXPORT
#endif

// This is the VTK/Python 'class,' it contains the method list and a pointer
// to the superclass
typedef vtkObject *(*vtknewfunc)();

typedef struct {
  PyObject_HEAD
  PyMethodDef *vtk_methods;
  vtknewfunc vtk_new;
  char *vtk_name;
  char *vtk_module;
  char *vtk_doc;
  PyObject *vtk_bases;
} PyVTKClass;

// This is the VTK/Python 'object,' it contains the python object header
// plus a pointer to the associated vtkObject and PyVTKClass.
typedef struct {
  PyObject_HEAD
  vtkObject *vtk_ptr;
  PyVTKClass *vtk_class;
} PyVTKObject;

// This for objects not derived from vtkObject
typedef struct {
  PyObject_HEAD
  void *vtk_ptr;
  PyMethodDef *vtk_methods;
  char *vtk_name;
  char *vtk_doc;
} PyVTKSpecialObject;

// Standard methods for all vtk/python objects
extern "C" 
{
VTK_PYTHON_EXPORT int PyVTKObject_Check(PyObject *obj);
VTK_PYTHON_EXPORT int PyVTKClass_Check(PyObject *obj);
VTK_PYTHON_EXPORT int PyVTKSpecialObjectCheck(PyObject *obj);
VTK_PYTHON_EXPORT PyObject *PyVTKObject_New(PyObject *vtkclass, vtkObject *ptr);
VTK_PYTHON_EXPORT PyObject *PyVTKClass_New(vtknewfunc constructor, PyMethodDef *methods,
			 char *classname, char *modulename, char *docstring,
			 PyObject *base);
VTK_PYTHON_EXPORT PyObject *PyVTKSpecialObject_New(void *ptr, PyMethodDef *methods,
				 char *classname, char *docstring);

// this is a special version of ParseTuple that handles both bound
// and unbound method calls for VTK objects
VTK_PYTHON_EXPORT vtkObject *PyArg_VTKParseTuple(PyObject *self, PyObject *args, 
			       char *format, ...);
}

// Add a PyVTKClass to the type lookup table, this allows us to later
// create object given only the class name.
extern VTK_PYTHON_EXPORT void vtkPythonAddClassToHash(PyObject *obj, char *type); 

// Extract the vtkObject from a PyVTKObject.  If the PyObject is not a 
// PyVTKObject, or is not a PyVTKObject of the specified type, the python
// error indicator will be set.
// Special behaviour: Py_None is converted to NULL without no error.
extern VTK_PYTHON_EXPORT vtkObject *vtkPythonGetPointerFromObject(PyObject *obj, char *type);

// Convert a vtkObject to a PyVTKObject.  This will first check to see if
// the PyVTKObject already exists, and create a new PyVTKObject if necessary.
// This function also passes ownership of the reference to the PyObject.
// Special behaviour: NULL is converted to Py_None.
extern VTK_PYTHON_EXPORT PyObject *vtkPythonGetObjectFromPointer(vtkObject *ptr);

// Try to convert some PyObject into a PyVTKObject, currently conversion
// is supported for SWIG-style mangled pointer strings.
extern VTK_PYTHON_EXPORT PyObject *vtkPythonGetObjectFromObject(PyObject *arg, const char *type);

// Add and delete PyVTKObject/vtkObject pairs from the wrapper hash table,
// these methods do not change the reference counts of either the vtkObject
// or the PyVTKObject.
extern VTK_PYTHON_EXPORT void vtkPythonAddObjectToHash(PyObject *obj, vtkObject *anInstance);
extern VTK_PYTHON_EXPORT void vtkPythonDeleteObjectFromHash(PyObject *obj);

// Utility functions for creating/usinge SWIG-style mangled pointer strings.
extern VTK_PYTHON_EXPORT char *vtkPythonManglePointer(void *ptr, const char *type);
extern VTK_PYTHON_EXPORT void *vtkPythonUnmanglePointer(char *ptrText, int *len,
				      const char *type);

// For use by SetXXMethod() , SetXXMethodArgDelete()
extern VTK_PYTHON_EXPORT void vtkPythonVoidFunc(void *);
extern VTK_PYTHON_EXPORT void vtkPythonVoidFuncArgDelete(void *);

// To allow Python to use the vtkCommand features
class vtkPythonCommand : public vtkCommand
{
public:
  vtkPythonCommand();
  ~vtkPythonCommand(); 
  static vtkPythonCommand *New() { return new vtkPythonCommand; };

  void SetObject(PyObject *o);
  void Execute(vtkObject *ptr, unsigned long eventtype, void *);
 
  PyObject *obj;
};


