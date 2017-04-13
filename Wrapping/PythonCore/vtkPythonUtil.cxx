/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonUtil.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPythonUtil.h"
#include "vtkPythonOverload.h"

#include "vtkSystemIncludes.h"

#include "vtkObject.h"
#include "vtkPythonCommand.h"
#include "vtkSmartPointerBase.h"
#include "vtkStdString.h"
#include "vtkToolkits.h"
#include "vtkUnicodeString.h"
#include "vtkVariant.h"
#include "vtkWeakPointer.h"
#include "vtkWindows.h"

#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <utility>
#include <algorithm>

#ifdef VTK_WRAP_PYTHON_SIP
#include "sip.h"
#endif

// for uintptr_t
#ifdef _MSC_VER
#include <stddef.h>
#else
#include <stdint.h>
#endif

//--------------------------------------------------------------------
// A ghost object, can be used to recreate a deleted PyVTKObject
class PyVTKObjectGhost
{
public:
  PyVTKObjectGhost() : vtk_ptr(), vtk_class(0), vtk_dict(0) {};

  vtkWeakPointerBase vtk_ptr;
  PyTypeObject *vtk_class;
  PyObject *vtk_dict;
};

//--------------------------------------------------------------------
// There are six maps associated with the Python wrappers

// Map VTK objects to python objects (this is also the cornerstone
// of the vtk/python garbage collection system, because it contains
// exactly one pointer reference for each VTK object known to python)
class vtkPythonObjectMap
  : public std::map<vtkObjectBase*, std::pair<PyObject *, vtkAtomicInt32> >
{
public:
  ~vtkPythonObjectMap();

  void add(vtkObjectBase* key, PyObject* value);
  void remove(vtkObjectBase* key);
};

// Call Delete instead of relying on vtkSmartPointer, so that crashes
// caused by deletion are easier to follow in the debug stack trace
vtkPythonObjectMap::~vtkPythonObjectMap()
{
  iterator i;
  for (i = this->begin(); i != this->end(); ++i)
  {
    for (int j = 0; j < i->second.second; ++j)
    {
      i->first->Delete();
    }
  }
}

void
vtkPythonObjectMap::add(vtkObjectBase* key, PyObject* value)
{
  key->Register(0);
  iterator i = this->find(key);
  if (i == this->end())
  {
    (*this)[key] = std::make_pair(value, 1);
  }
  else
  {
    i->second.first = value;
    ++i->second.second;
  }
}

void
vtkPythonObjectMap::remove(vtkObjectBase* key)
{
  iterator i = this->find(key);
  if (i != this->end())
  {
    // Save the object. The iterator will become invalid if the iterator is
    // erased.
    vtkObjectBase* obj = i->first;
    // Remove it from the map if necessary.
    if (!--i->second.second)
    {
      this->erase(i);
    }
    // Remove a reference to the object. This must be done *after* removing it
    // from the map (if needed) because if there's a callback which reacts when
    // the reference is dropped, it might call RemoveObjectFromMap as well. If
    // it still exists in the map at that point, this becomes an infinite loop.
    obj->Delete();
  }
}

// Keep weak pointers to VTK objects that python no longer has
// references to.  Python keeps the python 'dict' for VTK objects
// even when they pass leave the python realm, so that if those
// VTK objects come back, their 'dict' can be restored to them.
// Periodically the weak pointers are checked and the dicts of
// VTK objects that have been deleted are tossed away.
class vtkPythonGhostMap
  : public std::map<vtkObjectBase*, PyVTKObjectGhost>
{
};

// Keep track of all the VTK classes that python knows about.
class vtkPythonClassMap
  : public std::map<std::string, PyVTKClass>
{
};

// Like the ClassMap, for types not derived from vtkObjectBase.
class vtkPythonSpecialTypeMap
  : public std::map<std::string, PyVTKSpecialType>
{
};

// Keep track of all the C++ namespaces that have been wrapped.
class vtkPythonNamespaceMap
  : public std::map<std::string, PyObject*>
{
};

// Keep track of all the C++ enums that have been wrapped.
class vtkPythonEnumMap
  : public std::map<std::string, PyTypeObject*>
{
};

// Keep track of all vtkPythonCommand instances.
class vtkPythonCommandList
  : public std::vector<vtkWeakPointer<vtkPythonCommand> >
{
public:
  ~vtkPythonCommandList()
  {
    iterator iter;
    for (iter = this->begin(); iter != this->end(); ++iter)
    {
      if (iter->GetPointer())
      {
        iter->GetPointer()->obj = NULL;
        iter->GetPointer()->ThreadState = NULL;
      }
    }
  }
  void findAndErase(vtkPythonCommand* ptr)
  {
    this->erase(std::remove(this->begin(), this->end(), ptr), this->end());
  }
};

//--------------------------------------------------------------------
// The singleton for vtkPythonUtil

static vtkPythonUtil *vtkPythonMap = NULL;

// destructs the singleton when python exits
void vtkPythonUtilDelete()
{
  delete vtkPythonMap;
  vtkPythonMap = NULL;
}

// constructs the singleton
void vtkPythonUtilCreateIfNeeded()
{
  if (vtkPythonMap == NULL)
  {
    vtkPythonMap = new vtkPythonUtil();
    Py_AtExit(vtkPythonUtilDelete);
  }
}

//--------------------------------------------------------------------
vtkPythonUtil::vtkPythonUtil()
{
  this->ObjectMap = new vtkPythonObjectMap;
  this->GhostMap = new vtkPythonGhostMap;
  this->ClassMap = new vtkPythonClassMap;
  this->SpecialTypeMap = new vtkPythonSpecialTypeMap;
  this->NamespaceMap = new vtkPythonNamespaceMap;
  this->EnumMap = new vtkPythonEnumMap;
  this->PythonCommandList = new vtkPythonCommandList;
}

//--------------------------------------------------------------------
vtkPythonUtil::~vtkPythonUtil()
{
  delete this->ObjectMap;
  delete this->GhostMap;
  delete this->ClassMap;
  delete this->SpecialTypeMap;
  delete this->NamespaceMap;
  delete this->EnumMap;
  delete this->PythonCommandList;
}

//--------------------------------------------------------------------
void vtkPythonUtil::RegisterPythonCommand(vtkPythonCommand* cmd)
{
  if (cmd)
  {
    vtkPythonUtilCreateIfNeeded();
    vtkPythonMap->PythonCommandList->push_back(cmd);
  }
}

//--------------------------------------------------------------------
void vtkPythonUtil::UnRegisterPythonCommand(vtkPythonCommand* cmd)
{
  if (cmd && vtkPythonMap)
  {
    vtkPythonMap->PythonCommandList->findAndErase(cmd);
  }
}


//--------------------------------------------------------------------
// Concatenate an array of strings into a single python string object.
// The array of strings must be null-terminated,
// e.g. static char *strings[] = {"string1", "string2", NULL};
PyObject *vtkPythonUtil::BuildDocString(const char *docstring[])
{
  PyObject *result;

  // count the number of segments for the docstring
  int n;
  for (n = 0; docstring[n] != NULL; n++)
  {
    ;
  }

  if (n == 0)
  {
    result = PyString_FromString("");
  }
  else if (n == 1)
  {
    result = PyString_FromString(docstring[0]);
  }
  else
  {
    Py_ssize_t *m = new Py_ssize_t[n];

    Py_ssize_t l = 0;
    for (int i = 0; i < n; i++)
    {
      m[i] = (Py_ssize_t)strlen(docstring[i]);
      l += m[i];
    }

    char *data = new char[l + 1];

    size_t j = 0;
    for (int i = 0; i < n; i++)
    {
      strcpy(&data[j], docstring[i]);
      j += m[i];
    }

    result = PyString_FromStringAndSize(data, l);

    delete [] data;
    delete [] m;
  }

  return result;
}

//--------------------------------------------------------------------
PyVTKSpecialType *vtkPythonUtil::AddSpecialTypeToMap(
  PyTypeObject *pytype, PyMethodDef *methods, PyMethodDef *constructors,
  vtkcopyfunc copyfunc)
{
  const char *classname = vtkPythonUtil::StripModule(pytype->tp_name);
  vtkPythonUtilCreateIfNeeded();

#ifdef VTKPYTHONDEBUG
  //  vtkGenericWarningMacro("Adding an type " << type << " to map ptr");
#endif

  // lets make sure it isn't already there
  vtkPythonSpecialTypeMap::iterator i =
    vtkPythonMap->SpecialTypeMap->find(classname);
  if (i != vtkPythonMap->SpecialTypeMap->end())
  {
    return 0;
  }

  i = vtkPythonMap->SpecialTypeMap->insert(i,
    vtkPythonSpecialTypeMap::value_type(
      classname,
      PyVTKSpecialType(pytype, methods, constructors, copyfunc)));

#ifdef VTKPYTHONDEBUG
  //  vtkGenericWarningMacro("Added type to map type = " << typeObject);
#endif

  return &i->second;
}

//--------------------------------------------------------------------
PyVTKSpecialType *vtkPythonUtil::FindSpecialType(const char *classname)
{
  if (vtkPythonMap)
  {
    vtkPythonSpecialTypeMap::iterator it =
      vtkPythonMap->SpecialTypeMap->find(classname);

    if (it != vtkPythonMap->SpecialTypeMap->end())
    {
      return &it->second;
    }
  }

  return NULL;
}

//--------------------------------------------------------------------
void vtkPythonUtil::AddObjectToMap(PyObject *obj, vtkObjectBase *ptr)
{
  vtkPythonUtilCreateIfNeeded();

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Adding an object to map ptr = " << ptr);
#endif

  ((PyVTKObject *)obj)->vtk_ptr = ptr;
  vtkPythonMap->ObjectMap->add(ptr, obj);

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Added object to map obj= " << obj << " "
                         << ptr);
#endif
}

//--------------------------------------------------------------------
void vtkPythonUtil::RemoveObjectFromMap(PyObject *obj)
{
  PyVTKObject *pobj = (PyVTKObject *)obj;

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Deleting an object from map obj = "
                         << pobj << " " << pobj->vtk_ptr);
#endif

  if (vtkPythonMap && vtkPythonMap->ObjectMap->count(pobj->vtk_ptr))
  {
    vtkWeakPointerBase wptr;

    // check for customized class or dict
    if (pobj->vtk_class->py_type != Py_TYPE(pobj) ||
        PyDict_Size(pobj->vtk_dict))
    {
      wptr = pobj->vtk_ptr;
    }

    vtkPythonMap->ObjectMap->remove(pobj->vtk_ptr);

    // if the VTK object still exists, then make a ghost
    if (wptr.GetPointer())
    {
      // List of attrs to be deleted
      std::vector<PyObject*> delList;

      // Erase ghosts of VTK objects that have been deleted
      vtkPythonGhostMap::iterator i =
        vtkPythonMap->GhostMap->begin();
      while (i != vtkPythonMap->GhostMap->end())
      {
        if (!i->second.vtk_ptr.GetPointer())
        {
          delList.push_back((PyObject *)i->second.vtk_class);
          delList.push_back(i->second.vtk_dict);
          vtkPythonMap->GhostMap->erase(i++);
        }
        else
        {
          ++i;
        }
      }

      // Add this new ghost to the map
      PyVTKObjectGhost &g = (*vtkPythonMap->GhostMap)[pobj->vtk_ptr];
      g.vtk_ptr = wptr;
      g.vtk_class = Py_TYPE(pobj);
      g.vtk_dict = pobj->vtk_dict;
      Py_INCREF(g.vtk_class);
      Py_INCREF(g.vtk_dict);

      // Delete attrs of erased objects.  Must be done at the end.
      for (size_t j = 0; j < delList.size(); j++)
      {
        Py_DECREF(delList[j]);
      }
    }
  }
}

//--------------------------------------------------------------------
PyObject *vtkPythonUtil::GetObjectFromPointer(vtkObjectBase *ptr)
{
  PyObject *obj = NULL;

  if (ptr && vtkPythonMap)
  {
    vtkPythonObjectMap::iterator i =
      vtkPythonMap->ObjectMap->find(ptr);
    if (i != vtkPythonMap->ObjectMap->end())
    {
      obj = i->second.first;
    }
    if (obj)
    {
      Py_INCREF(obj);
      return obj;
    }
  }
  else
  {
    Py_INCREF(Py_None);
    return Py_None;
  }

  // search weak list for object, resurrect if it is there
  vtkPythonGhostMap::iterator j =
    vtkPythonMap->GhostMap->find(ptr);
  if (j != vtkPythonMap->GhostMap->end())
  {
    if (j->second.vtk_ptr.GetPointer())
    {
      obj = PyVTKObject_FromPointer(
        j->second.vtk_class, j->second.vtk_dict, ptr);
    }
    Py_DECREF(j->second.vtk_class);
    Py_DECREF(j->second.vtk_dict);
    vtkPythonMap->GhostMap->erase(j);
  }

  if (obj == NULL)
  {
    // create a new object
    PyVTKClass *vtkclass = NULL;
    vtkPythonClassMap::iterator k =
      vtkPythonMap->ClassMap->find(ptr->GetClassName());
    if (k != vtkPythonMap->ClassMap->end())
    {
      vtkclass = &k->second;
    }

    // if the class was not in the map, then find the nearest base class
    // that is, and associate ptr->GetClassName() with that base class
    if (vtkclass == NULL)
    {
      const char *classname = ptr->GetClassName();
      vtkclass = vtkPythonUtil::FindNearestBaseClass(ptr);
      vtkPythonClassMap::iterator i =
        vtkPythonMap->ClassMap->find(classname);
      if (i == vtkPythonMap->ClassMap->end())
      {
        vtkPythonMap->ClassMap->insert(
          i, vtkPythonClassMap::value_type(classname, *vtkclass));
      }
    }

    obj = PyVTKObject_FromPointer(vtkclass->py_type, NULL, ptr);
  }

  return obj;
}

//--------------------------------------------------------------------
const char *vtkPythonUtil::PythonicClassName(const char *classname)
{
  const char *cp = classname;

  /* check for non-alphanumeric chars */
  if (isalpha(*cp) || *cp == '_')
  {
    do { cp++; } while (isalnum(*cp) || *cp == '_');
  }

  if (*cp != '\0')
  {
    /* look up class and get its pythonic name */
    PyVTKClass *o = vtkPythonUtil::FindClass(classname);
    if (o)
    {
      classname = vtkPythonUtil::StripModule(o->py_type->tp_name);
    }
  }

  return classname;
}

//--------------------------------------------------------------------
const char *vtkPythonUtil::StripModule(const char *tpname)
{
  const char *cp = tpname;
  while (*cp != '.' && *cp != '\0')
  {
    cp++;
  }
  if (*cp == '.')
  {
    return ++cp;
  }
  return tpname;
}

//--------------------------------------------------------------------
PyVTKClass *vtkPythonUtil::AddClassToMap(
  PyTypeObject *pytype, PyMethodDef *methods,
  const char *classname, vtknewfunc constructor)
{
  vtkPythonUtilCreateIfNeeded();

  // lets make sure it isn't already there
  vtkPythonClassMap::iterator i =
    vtkPythonMap->ClassMap->find(classname);
  if (i != vtkPythonMap->ClassMap->end())
  {
    return 0;
  }

  i = vtkPythonMap->ClassMap->insert(i,
    vtkPythonClassMap::value_type(
      classname,
      PyVTKClass(pytype, methods, classname, constructor)));

  return &i->second;
}

//--------------------------------------------------------------------
PyVTKClass *vtkPythonUtil::FindClass(const char *classname)
{
  if (vtkPythonMap)
  {
    vtkPythonClassMap::iterator it =
      vtkPythonMap->ClassMap->find(classname);
    if (it != vtkPythonMap->ClassMap->end())
    {
      return &it->second;
    }
  }

  return NULL;
}

//--------------------------------------------------------------------
// this is a helper function to find the nearest base class for an
// object whose class is not in the ClassDict
PyVTKClass *vtkPythonUtil::FindNearestBaseClass(vtkObjectBase *ptr)
{
  PyVTKClass *nearestbase = NULL;
  int maxdepth = 0;
  int depth;

  for (vtkPythonClassMap::iterator classes =
        vtkPythonMap->ClassMap->begin();
       classes != vtkPythonMap->ClassMap->end(); ++classes)
  {
    PyVTKClass *pyclass = &classes->second;

    if (ptr->IsA(pyclass->vtk_name))
    {
      PyTypeObject *base = pyclass->py_type->tp_base;
      // count the hierarchy depth for this class
      for (depth = 0; base != 0; depth++)
      {
        base = base->tp_base;
      }
      // we want the class that is furthest from vtkObjectBase
      if (depth > maxdepth)
      {
        maxdepth = depth;
        nearestbase = pyclass;
      }
    }
  }

  return nearestbase;
}

//--------------------------------------------------------------------
vtkObjectBase *vtkPythonUtil::GetPointerFromObject(
  PyObject *obj, const char *result_type)
{
  vtkObjectBase *ptr;

  // convert Py_None to NULL every time
  if (obj == Py_None)
  {
    return NULL;
  }

  // check to ensure it is a vtk object
  if (!PyVTKObject_Check(obj))
  {
    obj = PyObject_GetAttrString(obj, "__vtk__");
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
      if (!PyVTKObject_Check(result))
      {
        PyErr_SetString(PyExc_TypeError, "__vtk__() doesn't return a VTK object");
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
      PyErr_SetString(PyExc_TypeError, "method requires a VTK object");
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
    char error_string[2048];
#ifdef VTKPYTHONDEBUG
    vtkGenericWarningMacro("vtk bad argument, type conversion failed.");
#endif
    snprintf(error_string, sizeof(error_string), "method requires a %.500s, a %.500s was provided.",
             vtkPythonUtil::PythonicClassName(result_type),
             vtkPythonUtil::PythonicClassName(
               ((vtkObjectBase *)ptr)->GetClassName()));
    PyErr_SetString(PyExc_TypeError, error_string);
    return NULL;
  }
}

//----------------
// union of long int and pointer
union vtkPythonUtilPointerUnion
{
  void *p;
  uintptr_t l;
};

//----------------
// union of long int and pointer
union vtkPythonUtilConstPointerUnion
{
  const void *p;
  uintptr_t l;
};

//--------------------------------------------------------------------
PyObject *vtkPythonUtil::GetObjectFromObject(
  PyObject *arg, const char *type)
{
  union vtkPythonUtilPointerUnion u;
  PyObject *tmp = 0;

#ifdef Py_USING_UNICODE
  if (PyUnicode_Check(arg))
  {
    tmp = PyUnicode_AsUTF8String(arg);
    arg = tmp;
  }
#endif

  if (PyBytes_Check(arg))
  {
    vtkObjectBase *ptr;
    char *ptrText = PyBytes_AsString(arg);

    char typeCheck[1024];  // typeCheck is currently not used
    unsigned long long l;
    int i = sscanf(ptrText,"_%llx_%s", &l, typeCheck);
    u.l = static_cast<uintptr_t>(l);

    if (i <= 0)
    {
      i = sscanf(ptrText,"Addr=0x%llx", &l);
      u.l = static_cast<uintptr_t>(l);
    }
    if (i <= 0)
    {
      i = sscanf(ptrText, "%p", &u.p);
    }
    if (i <= 0)
    {
      Py_XDECREF(tmp);
      PyErr_SetString(PyExc_ValueError, "could not extract hexadecimal address from argument string");
      return NULL;
    }

    ptr = static_cast<vtkObjectBase *>(u.p);

    if (!ptr->IsA(type))
    {
      char error_string[2048];
      snprintf(error_string,sizeof(error_string),"method requires a %.500s address, a %.500s address was provided.",
              type, ptr->GetClassName());
      Py_XDECREF(tmp);
      PyErr_SetString(PyExc_TypeError, error_string);
      return NULL;
    }

    Py_XDECREF(tmp);
    return vtkPythonUtil::GetObjectFromPointer(ptr);
  }

  Py_XDECREF(tmp);
  PyErr_SetString(PyExc_TypeError, "method requires a string argument");
  return NULL;
}

//--------------------------------------------------------------------
void *vtkPythonUtil::GetPointerFromSpecialObject(
  PyObject *obj, const char *result_type, PyObject **newobj)
{
  if (vtkPythonMap == NULL)
  {
    PyErr_SetString(PyExc_TypeError, "method requires a vtkPythonMap");
    return NULL;
  }

  const char *object_type =
    vtkPythonUtil::StripModule(Py_TYPE(obj)->tp_name);

  // do a lookup on the desired type
  vtkPythonSpecialTypeMap::iterator it =
    vtkPythonMap->SpecialTypeMap->find(result_type);
  if (it != vtkPythonMap->SpecialTypeMap->end())
  {
    PyVTKSpecialType *info = &it->second;

    // first, check if object is the desired type
    if (PyObject_TypeCheck(obj, info->py_type))
    {
      return ((PyVTKSpecialObject *)obj)->vtk_ptr;
    }

    // try to construct the special object from the supplied object
    PyObject *sobj = 0;

    PyMethodDef *meth =
      vtkPythonOverload::FindConversionMethod(info->vtk_constructors, obj);

    // If a constructor signature exists for "obj", call it
    if (meth && meth->ml_meth)
    {
      PyObject *args = PyTuple_New(1);
      PyTuple_SET_ITEM(args, 0, obj);
      Py_INCREF(obj);

      sobj = meth->ml_meth(0, args);

      Py_DECREF(args);
    }

    if (sobj && newobj)
    {
      *newobj = sobj;
      return ((PyVTKSpecialObject *)sobj)->vtk_ptr;
    }
    else if (sobj)
    {
      char error_text[2048];
      Py_DECREF(sobj);
      snprintf(error_text, sizeof(error_text), "cannot pass %.500s as a non-const %.500s reference",
              object_type, result_type);
      PyErr_SetString(PyExc_TypeError, error_text);
      return NULL;
    }

    // If a TypeError occurred, clear it and set our own error
    PyObject *ex = PyErr_Occurred();
    if (ex != NULL)
    {
      if (PyErr_GivenExceptionMatches(ex, PyExc_TypeError))
      {
        PyErr_Clear();
      }
      else
      {
        return NULL;
      }
    }
  }

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("vtk bad argument, type conversion failed.");
#endif

  char error_string[2048];
  snprintf(error_string, sizeof(error_string), "method requires a %.500s, a %.500s was provided.",
          result_type, object_type);
  PyErr_SetString(PyExc_TypeError, error_string);

  return NULL;
}

//--------------------------------------------------------------------
void vtkPythonUtil::AddNamespaceToMap(PyObject *module)
{
  if (!PyVTKNamespace_Check(module))
  {
    return;
  }

  vtkPythonUtilCreateIfNeeded();

  const char *name = PyVTKNamespace_GetName(module);
  // let's make sure it isn't already there
  vtkPythonNamespaceMap::iterator i =
    vtkPythonMap->NamespaceMap->find(name);
  if (i != vtkPythonMap->NamespaceMap->end())
  {
    return;
  }

  (*vtkPythonMap->NamespaceMap)[name] = module;
}

//--------------------------------------------------------------------
// This method is called from PyVTKNamespace_Delete
void vtkPythonUtil::RemoveNamespaceFromMap(PyObject *obj)
{
  if (vtkPythonMap && PyVTKNamespace_Check(obj))
  {
    const char *name = PyVTKNamespace_GetName(obj);
    vtkPythonNamespaceMap::iterator it =
      vtkPythonMap->NamespaceMap->find(name);
    if (it != vtkPythonMap->NamespaceMap->end() &&
        it->second == obj)
    {
      // The map has a pointer to the object, but does not hold a
      // reference, therefore there is no decref.
      vtkPythonMap->NamespaceMap->erase(it);
    }
  }
}

//--------------------------------------------------------------------
PyObject *vtkPythonUtil::FindNamespace(const char *name)
{
  if (vtkPythonMap)
  {
    vtkPythonNamespaceMap::iterator it =
      vtkPythonMap->NamespaceMap->find(name);
    if (it != vtkPythonMap->NamespaceMap->end())
    {
      return it->second;
    }
  }

  return NULL;
}

//--------------------------------------------------------------------
void vtkPythonUtil::AddEnumToMap(PyTypeObject *enumtype)
{
  vtkPythonUtilCreateIfNeeded();

  // Only add to map if it isn't already there
  const char *enumname = vtkPythonUtil::StripModule(enumtype->tp_name);
  vtkPythonEnumMap::iterator i =
    vtkPythonMap->EnumMap->find(StripModule(enumname));
  if (i == vtkPythonMap->EnumMap->end())
  {
    (*vtkPythonMap->EnumMap)[enumname] = enumtype;
  }
}

//--------------------------------------------------------------------
PyTypeObject *vtkPythonUtil::FindEnum(const char *name)
{
  PyTypeObject *pytype = NULL;

  if (vtkPythonMap)
  {
    vtkPythonEnumMap::iterator it =
      vtkPythonMap->EnumMap->find(name);
    if (it != vtkPythonMap->EnumMap->end())
    {
      pytype = it->second;
    }
  }

  return pytype;
}

//--------------------------------------------------------------------
// mangle a void pointer into a SWIG-style string
char *vtkPythonUtil::ManglePointer(const void *ptr, const char *type)
{
  static char ptrText[128];
  int ndigits = 2*(int)sizeof(void *);
  union vtkPythonUtilConstPointerUnion u;
  u.p = ptr;
  snprintf(ptrText, sizeof(ptrText), "_%*.*llx_%s", ndigits, ndigits,
          static_cast<unsigned long long>(u.l), type);

  return ptrText;
}

//--------------------------------------------------------------------
// unmangle a void pointer from a SWIG-style string
void *vtkPythonUtil::UnmanglePointer(char *ptrText, int *len, const char *type)
{
  int i;
  union vtkPythonUtilPointerUnion u;
  char text[1024];
  char typeCheck[1024];
  typeCheck[0] = '\0';

  // Do some minimal checks that it might be a swig pointer.
  if (*len < 256 && *len > 4 && ptrText[0] == '_')
  {
    strncpy(text, ptrText, *len);
    text[*len] = '\0';
    i = *len;
    // Allow one null byte, in case trailing null is part of *len
    if (i > 0 && text[i-1] == '\0')
    {
      i--;
    }
    // Verify that there are no other null bytes
    while (i > 0 && text[i-1] != '\0')
    {
      i--;
    }

    // If no null bytes, then do a full check for a swig pointer
    if (i == 0)
    {
      unsigned long long l;
      i = sscanf(text, "_%llx_%s", &l ,typeCheck);
      u.l = static_cast<uintptr_t>(l);

      if (strcmp(type,typeCheck) == 0)
      { // successfully unmangle
        *len = 0;
        return u.p;
      }
      else if (i == 2)
      { // mangled pointer of wrong type
        *len = -1;
        return NULL;
      }
    }
  }

  // couldn't unmangle: return string as void pointer if it didn't look
  // like a SWIG mangled pointer
  return (void *)ptrText;
}

//--------------------------------------------------------------------
Py_hash_t vtkPythonUtil::VariantHash(const vtkVariant *v)
{
  Py_hash_t h = -1;

  // This uses the same rules as the vtkVariant "==" operator.
  // All types except for vtkObject are converted to strings.
  // Quite inefficient, but it gets the job done.  Fortunately,
  // the python vtkVariant is immutable, so its hash can be cached.

  switch (v->GetType())
  {
    case VTK_OBJECT:
    {
      h = _Py_HashPointer(v->ToVTKObject());
      break;
    }

#ifdef Py_USING_UNICODE
    case VTK_UNICODE_STRING:
    {
      vtkUnicodeString u = v->ToUnicodeString();
      const char *s = u.utf8_str();
      PyObject *tmp = PyUnicode_DecodeUTF8(s, strlen(s), "strict");
      if (tmp == 0)
      {
        PyErr_Clear();
        return 0;
      }
      h = PyObject_Hash(tmp);
      Py_DECREF(tmp);
      break;
    }
#endif

    default:
    {
      vtkStdString s = v->ToString();
      PyObject *tmp = PyString_FromString(s.c_str());
      h = PyObject_Hash(tmp);
      Py_DECREF(tmp);
      break;
    }
  }

  return h;
}

//--------------------------------------------------------------------
void vtkPythonVoidFunc(void *arg)
{
  PyObject *arglist, *result;
  PyObject *func = (PyObject *)arg;

  // Sometimes it is possible for the function to be invoked after
  // Py_Finalize is called, this will cause nasty errors so we return if
  // the interpreter is not initialized.
  if (Py_IsInitialized() == 0)
  {
    return;
  }

#ifndef VTK_NO_PYTHON_THREADS
  vtkPythonScopeGilEnsurer gilEnsurer(true);
#endif

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

  // Sometimes it is possible for the function to be invoked after
  // Py_Finalize is called, this will cause nasty errors so we return if
  // the interpreter is not initialized.
  if (Py_IsInitialized() == 0)
  {
    return;
  }

#ifndef VTK_NO_PYTHON_THREADS
  vtkPythonScopeGilEnsurer gilEnsurer(true);
#endif

  if (func)
  {
    Py_DECREF(func);
  }
}




#ifdef VTK_WRAP_PYTHON_SIP
// utilities to provide access to Python objects wrapped with SIP
static const sipAPIDef *get_sip_api()
{
  static sipAPIDef *sip_api = NULL;

  if(!sip_api)
  {
    PyObject *c_api = NULL;
    PyObject *sip_module;
    PyObject *sip_module_dict;

    /* Import the SIP module. */
    sip_module = PyImport_ImportModule("sip");

    if (sip_module == NULL)
      return NULL;

    /* Get the module's dictionary. */
    sip_module_dict = PyModule_GetDict(sip_module);

    /* Get the "_C_API" attribute. */
    c_api = PyDict_GetItemString(sip_module_dict, "_C_API");

    if (c_api == NULL)
      return NULL;

    /* Sanity check that it is the right type. */
    if (PyCObject_Check(c_api))
      sip_api = (sipAPIDef *)PyCObject_AsVoidPtr(c_api);

    /* some versions of SIP use PyCapsule instead of PyCObject */
#if PY_VERSION_HEX >= 0x02070000
    if (PyCapsule_CheckExact(c_api))
      sip_api = (sipAPIDef *)PyCapsule_GetPointer(c_api, "sip._C_API");
#endif
  }

  /* Get the actual pointer from the object. */
  return sip_api;
}
#endif

void* vtkPythonUtil::SIPGetPointerFromObject(PyObject *obj, const char *classname)
{
#ifdef VTK_WRAP_PYTHON_SIP
  char etext[512];
  const sipAPIDef * api = get_sip_api();
  if(!api)
  {
    snprintf(etext, sizeof(etext), "unable to convert to %.200s without SIP api", classname);
    PyErr_SetString(PyExc_TypeError, etext);
    return NULL;
  }

  const sipTypeDef * td = api->api_find_type(classname);
  if(!td)
  {
    snprintf(etext, sizeof(etext), "unable to convert to %.200s without a typedef", classname);
    PyErr_SetString(PyExc_TypeError, etext);
    return NULL;
  }

  if(sipTypeIsEnum(td))
  {
    if (!api->api_can_convert_to_enum(obj, td))
    {
      snprintf(etext, sizeof(etext), "unable to convert to %.200s enum", classname);
      PyErr_SetString(PyExc_TypeError, etext);
      return NULL;
    }
    // Call PyInt_AsLong() to retrieve the value
    return obj;
  }

  if(!api->api_can_convert_to_type(obj, td, 0))
  {
    snprintf(etext, sizeof(etext), "unable to convert to %.200s", classname);
    PyErr_SetString(PyExc_TypeError, etext);
    return NULL;
  }

  int iserr = 0;
  void* ptr = api->api_convert_to_type(obj, td, NULL, 0, NULL, &iserr);
  if(iserr)
  {
    snprintf(etext, sizeof(etext), "error while converting to %.200s", classname);
    PyErr_SetString(PyExc_TypeError, etext);
    return NULL;
  }
  return ptr;
#else
  (void)obj;
  (void)classname;
  PyErr_SetString(PyExc_TypeError, "method requires VTK built with SIP support");
  return NULL;
#endif
}


PyObject* vtkPythonUtil::SIPGetObjectFromPointer(const void *ptr, const char* classname, bool is_new)
{
#ifdef VTK_WRAP_PYTHON_SIP
  const sipAPIDef * api = get_sip_api();
  if(!api)
  {
    PyErr_SetString(PyExc_TypeError, "Unable to convert to SIP type without api");
    return NULL;
  }

  const sipTypeDef * td = api->api_find_type(classname);
  if(!td)
  {
    PyErr_SetString(PyExc_TypeError, "Unable to convert to SIP type without typedef");
    return NULL;
  }

  if(sipTypeIsEnum(td))
  {
    size_t v = reinterpret_cast<size_t>(ptr);
    return api->api_convert_from_enum(v, td);
  }

  if(is_new)
  {
    return api->api_convert_from_new_type(const_cast<void*>(ptr), td, NULL);
  }

  return api->api_convert_from_type(const_cast<void*>(ptr), td, NULL);

#else
  (void)ptr;
  (void)classname;
  (void)is_new;
  PyErr_SetString(PyExc_TypeError, "method requires VTK built with SIP support");
  return NULL;
#endif
}
