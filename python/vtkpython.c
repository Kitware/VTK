#include <stdio.h>
#include <string.h>
#include "Python.h"

static PyMethodDef Pyvtkpython_ClassMethods[] = {
{NULL, NULL}};

/*
  This tiny little module will automatically load each of
  libVTKCommonPython, libVTKGraphicsPython and libVTKImagingPython
  and return an error in any of these are not present.
  It will load libVTKPatentedPython, libVTKContribPython, and
  libVTKLocalPython if these are also present.
*/
  

void initvtkpython()
{
  PyObject *m1, *m2, *d1, *d2, *k, *v;
  int i,j,n;

  m1 = Py_InitModule("vtkpython", Pyvtkpython_ClassMethods);
  d1 = PyModule_GetDict(m1);
  if (!d1) Py_FatalError("can't get dictionary for module vtkpython!");

  initlibVTKCommonPython();
  m2 = PyImport_AddModule("libVTKCommonPython");
  d2 = PyModule_GetDict(m2);
  if (!d2) Py_FatalError("can't get dictionary for module libVTKCommonPython!");
  
  n = PyDict_Size(d2);
  i = 0;
  for (j = 0; j < n; j++)
    {
    PyDict_Next(d2,&i,&k,&v);
    PyDict_SetItem(d1,k,v);
    }

  initlibVTKGraphicsPython();
  m2 = PyImport_AddModule("libVTKGraphicsPython");
  d2 = PyModule_GetDict(m2);
  if (!d2) Py_FatalError("can't get dictionary for module libVTKGraphicsPython!");
 
  n = PyDict_Size(d2);
  i = 0;
  for (j = 0; j < n; j++)
    {
    PyDict_Next(d2,&i,&k,&v);
    PyDict_SetItem(d1,k,v);
    }

  initlibVTKImagingPython();
  m2 = PyImport_AddModule("libVTKImagingPython");
  d2 = PyModule_GetDict(m2);
  if (!d2) Py_FatalError("can't get dictionary for module libVTKImagingPython!");
 
  n = PyDict_Size(d2);
  i = 0;
  for (j = 0; j < n; j++)
    {
    PyDict_Next(d2,&i,&k,&v);
    PyDict_SetItem(d1,k,v);
    }

  initlibVTKPatentedPython();
  m2 = PyImport_AddModule("libVTKPatentedPython");
  d2 = PyModule_GetDict(m2);
  if (d2)
    {
    n = PyDict_Size(d2);
    i = 0;
    for (j = 0; j < n; j++)
      {
      PyDict_Next(d2,&i,&k,&v);
      PyDict_SetItem(d1,k,v);
      }
    }

  initlibVTKContribPython();
  m2 = PyImport_AddModule("libVTKContribPython");
  d2 = PyModule_GetDict(m2);
  if (d2)
    { 
    n = PyDict_Size(d2);
    i = 0;
    for (j = 0; j < n; j++)
      {
      PyDict_Next(d2,&i,&k,&v);
      PyDict_SetItem(d1,k,v);
      }
    }

  initlibVTKLocalPython();
  m2 = PyImport_AddModule("libVTKLocalPython");
  d2 = PyModule_GetDict(m2);
  if (d2) 
    {
    n = PyDict_Size(d2);
    i = 0;
    for (j = 0; j < n; j++)
      {
      PyDict_Next(d2,&i,&k,&v);
      PyDict_SetItem(d1,k,v);
      }
    }
}
