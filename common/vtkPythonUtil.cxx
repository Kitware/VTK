/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonUtil.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/

#include <iostream.h>
#include <stdlib.h>
#include <stdio.h>

#include "vtkObject.h"
#include "vtkPythonUtil.h"

//#define VTKPYTHONDEBUG

class vtkHashNode 
{
public:
  vtkHashNode *next;
  void *key;
  void *value;
};


class vtkHashTable
{
public:
  vtkHashTable();
  vtkHashNode *(nodes[64]);
  void AddHashEntry(void *key,void *value);
  void *GetHashTableValue(void *key);
  void DeleteHashEntry(void *key);
};

vtkHashTable::vtkHashTable()
{
  int i;
  for (i = 0; i < 64; i++)
    {
    this->nodes[i] = NULL;
    }
}

vtkHashTable *vtkInstanceLookup = NULL;
vtkHashTable *vtkPointerLookup = NULL;
vtkHashTable *vtkTypecastLookup = NULL;
vtkHashTable *vtkDeleteLookup = NULL;

void vtkHashTable::AddHashEntry(void *key,void *value)
{
  vtkHashNode *pos;
  vtkHashNode *newpos;
  int loc;
  
  newpos = new vtkHashNode;
  newpos->key = key;
  newpos->value = value;
  newpos->next = NULL;
  
  loc = (((unsigned long)key) & 0x03f0) / 16;
  
  pos = this->nodes[loc];
  if (!pos)
    {
    this->nodes[loc] = newpos;
    return;
    }
  while (pos->next)
    {
    pos = pos->next;
    }
  pos->next = newpos;
}

void *vtkHashTable::GetHashTableValue(void *key)
{
  vtkHashNode *pos;
  int loc = (((unsigned long)key) & 0x03f0) / 16;
  
  pos = this->nodes[loc];

  if (!pos)
    {
    return NULL;
    }
  while ((pos)&&(pos->key != key))
    {
    pos = pos->next;
    }
  if (pos)
    {
    return pos->value;
    }
  return NULL;
}

void vtkHashTable::DeleteHashEntry(void *key)
{
  vtkHashNode *pos;
  vtkHashNode *prev = NULL;
  int loc = (((unsigned long)key) & 0x03f0) / 16;
  
  pos = this->nodes[loc];

  while ((pos)&&(pos->key != key))
    {
    prev = pos;
    pos = pos->next;
    }
  if (pos)
    {
    // we found this object
    if (prev)
      {
      prev->next = pos->next;
      }
    else
      {
      this->nodes[loc] = pos->next;
      }
    delete pos;
    }
}

// add an object to the hash table
void vtkPythonAddObjectToHash(PyObject *obj, void *ptr,
			      void *tcFunc,int deleteMe)
{ 
  if (!vtkInstanceLookup)
    {
    vtkInstanceLookup = new vtkHashTable;
    vtkTypecastLookup = new vtkHashTable;
    vtkPointerLookup = new vtkHashTable;
    vtkDeleteLookup = new vtkHashTable;
    }

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Adding an object to hash ptr = " << ptr);
#endif  
  // lets make sure it isn't already there
  if (vtkInstanceLookup->GetHashTableValue((void *)obj))
    {
#ifdef VTKPYTHONDEBUG
    vtkGenericWarningMacro("Attempt to add an object to the hash when one already exists!!!");
#endif
    return;
    }

  vtkInstanceLookup->AddHashEntry((void *)obj,ptr);
  vtkTypecastLookup->AddHashEntry((void *)obj,tcFunc);
  vtkPointerLookup->AddHashEntry(ptr,(void *)obj);
  vtkDeleteLookup->AddHashEntry((void *)obj,(void *)deleteMe);

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Added object to hash obj= " << obj << " " << ptr);
#endif  
}

// should we delete this object
int vtkPythonShouldIDeleteObject(PyObject *obj)
{
  if (vtkDeleteLookup->GetHashTableValue((void *)obj))
    {
#ifdef VTKPYTHONDEBUG
    vtkGenericWarningMacro("Decided to delete obj = " << obj);
#endif
    vtkPythonDeleteObjectFromHash(obj);
    return 1;
    }

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Decided to NOT delete obj = " << obj);
#endif
  vtkPythonDeleteObjectFromHash(obj);
  return 0;
}


// delete an object from the hash
// doesn't need a mutex because it is only called from within
// the above func which does have a mutex
void vtkPythonDeleteObjectFromHash(PyObject *obj)
{
  void *ptr;
  
  ptr = vtkInstanceLookup->GetHashTableValue((void *)obj);
  if (!ptr) 
    {
#ifdef VTKPYTHONDEBUG
    vtkGenericWarningMacro("Attempt to delete an object that doesnt exist!!!");
#endif  
    return;
    }
  vtkInstanceLookup->DeleteHashEntry((void *)obj);
  vtkTypecastLookup->DeleteHashEntry((void *)obj);
  vtkPointerLookup->DeleteHashEntry(ptr);
  vtkDeleteLookup->DeleteHashEntry((void *)obj);
}

PyObject *vtkPythonGetObjectFromPointer(void *ptr)
{
  PyObject *obj;

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Checking into pointer " << ptr);
#endif  
  obj = (PyObject *)vtkPointerLookup->GetHashTableValue((void *)ptr);
#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Checking into pointer " << ptr << " obj = " << obj);
#endif  
  return obj;
}

void *vtkPythonGetPointerFromObject(PyObject *obj, char *result_type)
{
  void *ptr;
  void *result;
  void *(*command)(void *,char *);
  
  ptr = vtkInstanceLookup->GetHashTableValue((void *)obj);
  command = 
    (void *(*)(void *,char *))vtkTypecastLookup->GetHashTableValue((void *)obj);
  
#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Checking into obj " << obj << " ptr = " << ptr);
#endif  

  if (!ptr)
    {
    return NULL;
    }
  
  result = command(ptr,result_type);
  if (result)
    {
#ifdef VTKPYTHONDEBUG
    vtkGenericWarningMacro("Got obj= " << obj << " ptr= " << ptr << " " << result_type);
#endif  
    return result;
    }
  else
    {
#ifdef VTKPYTHONDEBUG
    vtkGenericWarningMacro("vtk bad argument, type conversion failed.");
#endif  
    return NULL;
    }
}

void vtkPythonVoidFunc(void *arg)
{
  PyObject *arglist, *result;
  PyObject *func = (PyObject *)arg;

  arglist = Py_BuildValue("()");

  result = PyEval_CallObject(func, arglist);
  Py_XDECREF(result);
  Py_DECREF(arglist);
}

void vtkPythonVoidFuncArgDelete(void *arg)
{
  PyObject *func = (PyObject *)arg;
  Py_DECREF(func);
}
