/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonUtil.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkPythonUtil
*/

#ifndef vtkPythonUtil_h
#define vtkPythonUtil_h

#include "vtkPython.h"
#include "vtkPythonCompatibility.h"
#include "PyVTKMutableObject.h"
#include "PyVTKNamespace.h"
#include "PyVTKObject.h"
#include "PyVTKSpecialObject.h"

class vtkPythonClassMap;
class vtkPythonCommand;
class vtkPythonCommandList;
class vtkPythonGhostMap;
class vtkPythonObjectMap;
class vtkPythonSpecialTypeMap;
class vtkPythonNamespaceMap;
class vtkPythonEnumMap;
class vtkStdString;
class vtkUnicodeString;
class vtkVariant;

extern "C" void vtkPythonUtilDelete();

class VTKWRAPPINGPYTHONCORE_EXPORT vtkPythonUtil
{
public:

  /**
   * If the name is templated or mangled, converts it into
   * a python-printable name.
   */
  static const char *PythonicClassName(const char *classname);

  /**
   * Given a qualified python name "module.name", remove "module.".
   */
  static const char *StripModule(const char *tpname);

  /**
   * Add a PyVTKClass to the type lookup table, this allows us to later
   * create object given only the class name.
   */
  static PyVTKClass *AddClassToMap(
    PyTypeObject *pytype, PyMethodDef *methods,
    const char *classname, vtknewfunc constructor);

  /**
   * Get information about a special VTK type, given the type name.
   */
  static PyVTKClass *FindClass(const char *classname);

  /**
   * For an VTK object whose class is not in the ClassMap, search
   * the whole ClassMap to find out which class is the closest base
   * class of the object.  Returns a PyVTKClass.
   */
  static PyVTKClass *FindNearestBaseClass(vtkObjectBase *ptr);

  /**
   * Extract the vtkObjectBase from a PyVTKObject.  If the PyObject is
   * not a PyVTKObject, or is not a PyVTKObject of the specified type,
   * the python error indicator will be set.
   * Special behavior: Py_None is converted to NULL without no error.
   */
  static vtkObjectBase *GetPointerFromObject(PyObject *obj,
                                             const char *classname);

  /**
   * Convert a vtkObjectBase to a PyVTKObject.  This will first check to
   * see if the PyVTKObject already exists, and create a new PyVTKObject
   * if necessary.  This function also passes ownership of the reference
   * to the PyObject.
   * Special behaviour: NULL is converted to Py_None.
   */
  static PyObject *GetObjectFromPointer(vtkObjectBase *ptr);

  /**
   * Extract the SIP wrapped object from a PyObject.  If the conversion cannot
   * be done, an error indicator is set.
   * Special behavior: Py_None is converted to NULL without no error.
   */
  static void *SIPGetPointerFromObject(PyObject *obj, const char *classname);

  /**
   * Convert a SIP wrapped object to a PyObject.
   * Special behaviour: NULL is converted to Py_None.
   */
  static PyObject *SIPGetObjectFromPointer(
    const void *ptr, const char* classname, bool is_new);

  /**
   * Try to convert some PyObject into a PyVTKObject, currently conversion
   * is supported for SWIG-style mangled pointer strings.
   */
  static PyObject *GetObjectFromObject(PyObject *arg, const char *type);

  /**
   * Add PyVTKObject/vtkObjectBase pairs to the internal mapping.
   * This methods do not change the reference counts of either the
   * vtkObjectBase or the PyVTKObject.
   */
  static void AddObjectToMap(PyObject *obj, vtkObjectBase *anInstance);

  /**
   * Remove a PyVTKObject from the internal mapping.  No reference
   * counts are changed.
   */
  static void RemoveObjectFromMap(PyObject *obj);

  /**
   * Add a special VTK type to the type lookup table, this allows us to
   * later create object given only the class name.
   */
  static PyVTKSpecialType *AddSpecialTypeToMap(
    PyTypeObject *pytype, PyMethodDef *methods, PyMethodDef *constructors,
    vtkcopyfunc copyfunc);

  /**
   * Get information about a special VTK type, given the type name.
   */
  static PyVTKSpecialType *FindSpecialType(const char *classname);

  /**
   * Given a PyObject, convert it into a "result_type" object, where
   * "result_type" must be a wrapped type.  The C object is returned
   * as a void *, which must be cast to a pointer of the desired type.
   * If conversion was necessary, then the created python object is
   * returned in "newobj", but if the original python object was
   * already of the correct type, then "newobj" will be set to NULL.
   * If a python exception was raised, NULL will be returned.
   */
  static void *GetPointerFromSpecialObject(
    PyObject *obj, const char *result_type, PyObject **newobj);

  /**
   * Add a wrapped C++ namespace as a python module object.  This allows
   * the namespace to be retrieved and added to as necessary.
   */
  static void AddNamespaceToMap(PyObject *o);

  /**
   * Remove a wrapped C++ namespace from consideration.  This is called
   * from the namespace destructor.
   */
  static void RemoveNamespaceFromMap(PyObject *o);

  /**
   * Return an existing namespace, or NULL if it doesn't exist.
   */
  static PyObject *FindNamespace(const char *name);

  /**
   * Add a wrapped C++ enum as a python type object.
   */
  static void AddEnumToMap(PyTypeObject *o);

  /**
   * Return an enum type object, or NULL if it doesn't exist.
   */
  static PyTypeObject *FindEnum(const char *name);

  /**
   * Utility function to build a docstring by concatenating a series
   * of strings until a null string is found.
   */
  static PyObject *BuildDocString(const char *docstring[]);

  /**
   * Utility function for creating SWIG-style mangled pointer string.
   */
  static char *ManglePointer(const void *ptr, const char *type);

  /**
   * Utility function decoding a SWIG-style mangled pointer string.
   */
  static void *UnmanglePointer(char *ptrText, int *len, const char *type);

  /**
   * Compute a hash for a vtkVariant.
   */
  static Py_hash_t VariantHash(const vtkVariant *variant);

  //@{
  /**
   * Register a vtkPythonCommand. Registering vtkPythonCommand instances ensures
   * that when the interpreter is destroyed (and Py_AtExit() gets called), the
   * vtkPythonCommand state is updated to avoid referring to dangling Python
   * objects pointers. Note, this will not work with Py_NewInterpreter.
   */
  static void RegisterPythonCommand(vtkPythonCommand*);
  static void UnRegisterPythonCommand(vtkPythonCommand*);
  //@}

private:
  vtkPythonUtil();
  ~vtkPythonUtil();
  vtkPythonUtil(const vtkPythonUtil&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPythonUtil&) VTK_DELETE_FUNCTION;

  vtkPythonObjectMap *ObjectMap;
  vtkPythonGhostMap *GhostMap;
  vtkPythonClassMap *ClassMap;
  vtkPythonSpecialTypeMap *SpecialTypeMap;
  vtkPythonNamespaceMap *NamespaceMap;
  vtkPythonEnumMap *EnumMap;
  vtkPythonCommandList *PythonCommandList;

  friend void vtkPythonUtilDelete();
  friend void vtkPythonUtilCreateIfNeeded();
};

// For use by SetXXMethod() , SetXXMethodArgDelete()
extern VTKWRAPPINGPYTHONCORE_EXPORT void vtkPythonVoidFunc(void *);
extern VTKWRAPPINGPYTHONCORE_EXPORT void vtkPythonVoidFuncArgDelete(void *);

#endif
