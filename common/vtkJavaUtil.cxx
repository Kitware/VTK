/*=========================================================================

  Program:   Java Wrapper for VTK
  Module:    vtkJavaUtil.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file's contents may be copied, reproduced or altered in any way 
without the express written consent of the author.

Copyright (c) Ken Martin 1995

=========================================================================*/

#include <iostream.h>
#include <stdlib.h>
#include <stdio.h>
#include "vtkJavaUtil.h"


// #define VTKJAVADEBUG

// semaphore for the sun
#include <synch.h>
sema_t VTKsema;
#define SEMAINIT sema_init(&VTKsema, 1, USYNC_THREAD,NULL)
#define SEMAWAIT sema_wait(&VTKsema);
#define SEMAPOST sema_post(&VTKsema);
//#else
// semaphore for SGI
//#include <ulocks.h>
//usema_t *VTKsema;
//usptr_t *VTKlock;
//#define SEMAINIT VTKlock = usinit(VTKsema = usnewsema( ,1)
//#define SEMAINIT if (1)
//#define SEMAWAIT if (1)
//#define SEMAPOST if (1)
//#endif

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
void vtkJavaAddObjectToHash(void *hnd,void *obj,void *tcFunc,int deleteMe)
{
  if (!vtkInstanceLookup)
    {
    SEMAINIT;
    }
  SEMAWAIT;
  
  if (!vtkInstanceLookup)
    {
    vtkInstanceLookup = new vtkHashTable;
    vtkTypecastLookup = new vtkHashTable;
    vtkPointerLookup = new vtkHashTable;
    vtkDeleteLookup = new vtkHashTable;
    }
#ifdef VTKJAVADEBUG
  cerr << "Adding an object to hash " << hnd << " " << obj << "\n";
#endif  
  // lets make sure it isn't already there
  if (vtkInstanceLookup->GetHashTableValue(hnd) != NULL)
    {
#ifdef VTKJAVADEBUG
    cerr << "Attempt to add an object to the hash when one already exists!!!\n";
#endif
    SEMAPOST;
    return;
    }

  vtkInstanceLookup->AddHashEntry(hnd,obj);
  vtkTypecastLookup->AddHashEntry(hnd,tcFunc);
  vtkPointerLookup->AddHashEntry(obj,hnd);
  vtkDeleteLookup->AddHashEntry(hnd,(void *)deleteMe);

#ifdef VTKJAVADEBUG
  cerr << "Added object to hash " << hnd << " " << obj << "\n";
#endif  
  SEMAPOST;
}

// should we delete this object
int vtkJavaShouldIDeleteObject(void *hnd)
{
  SEMAWAIT;
  if ((int)(vtkDeleteLookup->GetHashTableValue(hnd)))
    {
#ifdef VTKJAVADEBUG
    cerr << "Decided to delete " << hnd << "\n";
#endif
    vtkJavaDeleteObjectFromHash(hnd);
    SEMAPOST;
    return 1;
    }

#ifdef VTKJAVADEBUG
  cerr << "Decided to NOT delete " << hnd << "\n";
#endif
  vtkJavaDeleteObjectFromHash(hnd);
  SEMAPOST;
  return 0;
}


// delete an object from the hash
void vtkJavaDeleteObjectFromHash(void *hnd)
{
  void *obj;

  obj = vtkInstanceLookup->GetHashTableValue(hnd);
  if (!obj) 
    {
#ifdef VTKJAVADEBUG
    cerr << "Attempt to delete an object that doesnt exist!!!";
#endif  
    return;
    }
  vtkInstanceLookup->DeleteHashEntry(hnd);
  vtkTypecastLookup->DeleteHashEntry(hnd);
  vtkPointerLookup->DeleteHashEntry(obj);
  vtkDeleteLookup->DeleteHashEntry(hnd);
}

void *vtkJavaGetObjectFromPointer(void *obj)
{
  void *hnd;

#ifdef VTKJAVADEBUG
  cerr << "Checking into object " << obj << "\n";
#endif  
  hnd = vtkPointerLookup->GetHashTableValue(obj);
#ifdef VTKJAVADEBUG
  cerr << "Checking into object " << obj << " hnd = " << hnd << "\n";
#endif  
  return hnd;
}

void *vtkJavaGetPointerFromObject(void *hnd,char *result_type)
{
  void *obj;
  void *(*command)(void *,char *);

  obj = vtkInstanceLookup->GetHashTableValue(hnd);
  command = (void *(*)(void *,char *))vtkTypecastLookup->GetHashTableValue(hnd);

#ifdef VTKJAVADEBUG
  cerr << "Checking into hnd " << hnd << " obj = " << obj << "\n";
#endif  

  if (!obj)
    {
    return NULL;
    }
  
  if (command(obj,result_type))
    {
#ifdef VTKJAVADEBUG
    cerr << "Got hnd " << hnd << " obj = " << obj << " " << result_type << "\n";
#endif  
    return command(obj,result_type);
    }
  else
    {
#ifdef VTKJAVADEBUG
    fprintf(stderr,"vtk bad argument, type conversion failed.\n");
#endif  
    return NULL;
    }
}

HArrayOfDouble *vtkJavaMakeHArrayOfDoubleFromDouble(double *ptr, int size)
{
  HArrayOfDouble *ret;
  double *vals;
  int i;

  ret = (HArrayOfDouble *)ArrayAlloc(T_DOUBLE,size);
  if (ret == 0)
    {
    SignalError(0,"OutOfMemoryError", 0);
    return 0;
    }
  vals = (double *)unhand(ret)->body;

  for (i = 0; i < size; i++)
    {
    vals[i] = ptr[i];
    }
  return ret;
}

HArrayOfDouble *vtkJavaMakeHArrayOfDoubleFromFloat(float *ptr, int size)
{
  HArrayOfDouble *ret;
  double *vals;
  int i;

  ret = (HArrayOfDouble *)ArrayAlloc(T_DOUBLE,size);
  if (ret == 0)
    {
    SignalError(0,"OutOfMemoryError", 0);
    return 0;
    }
  vals = (double *)unhand(ret)->body;

  for (i = 0; i < size; i++)
    {
    vals[i] = ptr[i];
    }
  return ret;
}

HArrayOfInt *vtkJavaMakeHArrayOfIntFromInt(int *ptr, int size)
{
  HArrayOfInt *ret;
  long *vals;
  int i;

  ret = (HArrayOfInt *)ArrayAlloc(T_INT,size);
  if (ret == 0)
    {
    SignalError(0,"OutOfMemoryError", 0);
    return 0;
    }
  vals = (long *)unhand(ret)->body;

  for (i = 0; i < size; i++)
    {
    vals[i] = ptr[i];
    }
  return ret;
}
