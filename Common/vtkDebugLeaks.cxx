/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDebugLeaks.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDebugLeaks.h"
#include "vtkOutputWindow.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkCriticalSection.h"

static const char *vtkDebugLeaksIgnoreClasses[] = {
  "vtkInstantiatorHashTable",
  "vtkWin32OutputWindow",
  "vtkOutputWindow",
  0
};

// return 1 if the class should be ignored
int vtkDebugLeaksIgnoreClassesCheck(const char* s)
{
  int i =0;
  while(vtkDebugLeaksIgnoreClasses[i])
    {
    if(strcmp(s, vtkDebugLeaksIgnoreClasses[i]) == 0)
      {
      return 1;
      }
    i++;
    }
  return 0;
}

vtkCxxRevisionMacro(vtkDebugLeaks, "1.20");
vtkStandardNewMacro(vtkDebugLeaks);

int vtkDebugLeaks::PromptUser = 1;

void vtkDebugLeaks::PromptUserOn()
{
  PromptUser = 1;
}
void vtkDebugLeaks::PromptUserOff()
{
  PromptUser = 0;
}
static vtkSimpleCriticalSection* DebugLeaksCritSec = 0;
void vtkDebugLeaksLock()
{
  if(!DebugLeaksCritSec)
    {
    DebugLeaksCritSec = new vtkSimpleCriticalSection;
    }
  DebugLeaksCritSec->Lock();
}
void vtkDebugLeaksUnlock()
{
  DebugLeaksCritSec->Unlock();
}

// A singleton that prints out the table, and deletes the table.
class vtkPrintLeaksAtExit
{
public:
  inline void Use() 
    {
    }
  ~vtkPrintLeaksAtExit()
    {
#ifdef VTK_DEBUG_LEAKS
      vtkObjectFactory::UnRegisterAllFactories();
      vtkOutputWindow::SetInstance(0);
      vtkDebugLeaks::PrintCurrentLeaks();
      vtkDebugLeaks::DeleteTable();
#endif
      // clean up memory
      delete DebugLeaksCritSec;
      DebugLeaksCritSec = 0;
    }  
};

// the global varible that should be destroyed at exit
static vtkPrintLeaksAtExit vtkPrintLeaksAtExitGlobal;


// A hash function for converting a string to a long
inline size_t vtkHashString(const char* s)
{
  unsigned long h = 0; 
  for ( ; *s; ++s)
    {
    h = 5*h + *s;
    }
  return size_t(h);
}

class vtkDebugLeaksHashNode 
{
public:
  vtkDebugLeaksHashNode() 
    {
      this->Count =1; // if it goes in, then there is one of them
      this->Key = 0;
      this->Next =0;
    }
  void Print()
    {
      if(this->Count)
        {
        vtkGenericWarningMacro("Class " << this->Key << " has " 
                               << this->Count << " instances still around" );
        }
    }
  ~vtkDebugLeaksHashNode()
    {
      delete [] this->Key;
    }
public:
  vtkDebugLeaksHashNode *Next;
  char *Key;
  int Count;
};

class vtkDebugLeaksHashTable
{
public:
  vtkDebugLeaksHashTable();
  vtkDebugLeaksHashNode* GetNode(const char* name);
  void IncrementCount(const char *name);
  unsigned int GetCount(const char *name);
  int DecrementCount(const char* name);
  void PrintTable();
  int IsEmpty();
  ~vtkDebugLeaksHashTable()
    {
      for (int i = 0; i < 64; i++)
        {
        delete this->Nodes[i];
        }
    }

private:
  vtkDebugLeaksHashNode* Nodes[64];
};

vtkDebugLeaksHashTable::vtkDebugLeaksHashTable()
{
  int i;
  for (i = 0; i < 64; i++)
    {
    this->Nodes[i] = NULL;
    }
}

void vtkDebugLeaksHashTable::IncrementCount(const char * name)
{
  vtkDebugLeaksHashNode *pos;
  vtkDebugLeaksHashNode *newpos;
  int loc;
  pos = this->GetNode(name);
  if(pos)
    {
    pos->Count++;
    return;
    }
  
  newpos = new vtkDebugLeaksHashNode;
  newpos->Key = strcpy(new char[strlen(name)+1], name);

  loc = (((unsigned long)vtkHashString(name)) & 0x03f0) / 16;
  
  pos = this->Nodes[loc];
  if (!pos)
    {
    this->Nodes[loc] = newpos;
    return;
    }
  while (pos->Next)
    {
    pos = pos->Next;
    }
  pos->Next = newpos;
}

vtkDebugLeaksHashNode* vtkDebugLeaksHashTable::GetNode(const char* key)
{
  vtkDebugLeaksHashNode *pos;
  int loc = (((unsigned long)vtkHashString(key)) & 0x03f0) / 16;
  
  pos = this->Nodes[loc];

  if (!pos)
    {
    return NULL;
    }
  while ((pos) && (strcmp(pos->Key, key) != 0) )
    {
    pos = pos->Next;
    }
  return pos;
}

unsigned int vtkDebugLeaksHashTable::GetCount(const char* key)
{
  vtkDebugLeaksHashNode *pos;
  int loc = (((unsigned long)vtkHashString(key)) & 0x03f0) / 16;
  
  pos = this->Nodes[loc];

  if (!pos)
    {
    return 0;
    }
  while ((pos)&&(pos->Key != key))
    {
    pos = pos->Next;
    }
  if (pos)
    {
    return pos->Count;
    }
  return 0;
}

int vtkDebugLeaksHashTable::IsEmpty()
{
  int count = 0;
  for(int i =0; i < 64; i++)
    {
    vtkDebugLeaksHashNode *pos = this->Nodes[i];
    if(pos)
      { 
      if(!vtkDebugLeaksIgnoreClassesCheck(pos->Key))
        {
        count += pos->Count;
        }
      while(pos->Next)
        {
        pos = pos->Next;
        if(!vtkDebugLeaksIgnoreClassesCheck(pos->Key))
          {
          count += pos->Count;
          }
        }
      }
    }
  return !count;
}

int vtkDebugLeaksHashTable::DecrementCount(const char *key)
{
  
  vtkDebugLeaksHashNode *pos = this->GetNode(key);
  if(pos)
    {
    pos->Count--;
    return 1;
    }
  else
    {
    return 0;
    }
}

void vtkDebugLeaksHashTable::PrintTable()
{
  for(int i =0; i < 64; i++)
    {
    vtkDebugLeaksHashNode *pos = this->Nodes[i];
    if(pos)
      { 
      if(!vtkDebugLeaksIgnoreClassesCheck(pos->Key))
        {
        pos->Print();
        }
      while(pos->Next)
        {
        pos = pos->Next;
        if(!vtkDebugLeaksIgnoreClassesCheck(pos->Key))
          {
          pos->Print();
          }
        }
      }
    }
}

vtkDebugLeaksHashTable* vtkDebugLeaks::MemoryTable = 0;


void vtkDebugLeaks::ConstructClass(const char* name)
{
  // force the use of the global varible so it gets constructed
  // but only do this if VTK_DEBUG_LEAKS is on
  vtkPrintLeaksAtExitGlobal.Use();
  vtkDebugLeaksLock();
  if(!vtkDebugLeaks::MemoryTable)
    {
    vtkDebugLeaks::MemoryTable = new vtkDebugLeaksHashTable;
    }
  vtkDebugLeaks::MemoryTable->IncrementCount(name);
  vtkDebugLeaksUnlock();
}

void vtkDebugLeaks::DestructClass(const char* p)
{
  vtkDebugLeaksLock();
  // Due to globals being deleted, this table may already have
  // been deleted.
  if(vtkDebugLeaks::MemoryTable && !vtkDebugLeaks::MemoryTable->DecrementCount(p))
    {
    vtkDebugLeaksUnlock();
    vtkGenericWarningMacro("Deleting unknown object: " << p);
    }
  else
    {
    vtkDebugLeaksUnlock();
    }
}
void vtkDebugLeaks::PrintCurrentLeaks()
{
  if(!vtkDebugLeaks::MemoryTable)
    {
    return;
    }
  if(vtkDebugLeaks::MemoryTable->IsEmpty())
    {
    return;
    }
  if ( vtkDebugLeaks::PromptUser)
    {
    vtkOutputWindow::GetInstance()->PromptUserOn();
    }
  else
    {
    vtkOutputWindow::GetInstance()->PromptUserOff();
    }
  vtkGenericWarningMacro("vtkDebugLeaks has detected LEAKS!\n ");
  // force some other singletons to delete themselves now
  vtkObjectFactory::UnRegisterAllFactories();
  // print the table
  vtkDebugLeaks::MemoryTable->PrintTable();
}

void vtkDebugLeaks::DeleteTable()
{
  delete vtkDebugLeaks::MemoryTable;
  vtkDebugLeaks::MemoryTable = 0;
}
