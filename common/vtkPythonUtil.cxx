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

// mangle a void pointer into a SWIG-style string
char *vtkPythonManglePointer(void *ptr, const char *type)
{
  static char ptrText[128];
  sprintf(ptrText,"_%*.*lx_%s",(int)sizeof(void *),(int)sizeof(void *),
	  ptr,type);
  return ptrText;
}

// unmangle a void pointer from a SWIG-style string
void *vtkPythonUnmanglePointer(char *ptrText, int *len, const char *type)
{
  int i; 
  void *ptr;
  char typeCheck[64];
  if (*len < 64)
    {
    i = sscanf(ptrText,"_%lx_%s",&ptr,typeCheck);
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

// add an object to the hash table
void vtkPythonAddObjectToHash(PyObject *obj, void *ptr, void *tcFunc)
{ 
  if (!vtkInstanceLookup)
    {
    vtkInstanceLookup = new vtkHashTable;
    vtkTypecastLookup = new vtkHashTable;
    vtkPointerLookup = new vtkHashTable;
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

#ifdef VTKPYTHONDEBUG
  vtkGenericWarningMacro("Added object to hash obj= " << obj << " " << ptr);
#endif  
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
  if (func)
    {
    Py_DECREF(func);
    }
}
