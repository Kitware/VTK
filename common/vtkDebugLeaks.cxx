/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDebugLeaks.cxx
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
#include "vtkDebugLeaks.h"
#include "vtkOutputWindow.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkCriticalSection.h"

// A singleton that prints out the table, and deletes the table.
class vtkPrintLeaksAtExit
{
public:
  inline void Use() 
    {
    }
  ~vtkPrintLeaksAtExit()
    {
      vtkObjectFactory::UnRegisterAllFactories();
      vtkOutputWindow::SetInstance(0);
      vtkDebugLeaks::PrintCurrentLeaks();
      vtkDebugLeaks::DeleteTable();
    }  
};

#ifdef VTK_DEBUG_LEAKS
// the global varible that should be destroyed at exit
static vtkPrintLeaksAtExit vtkPrintLeaksAtExitGlobal;
#endif

// A hash function for converting a string to a long
inline size_t vtkHashString(const char* s)
{
  unsigned long h = 0; 
  for ( ; *s; ++s)
    h = 5*h + *s;
  
  return size_t(h);
}

class vtkDebugLeaksHashNode 
{
public:
  vtkDebugLeaksHashNode() 
    {
      count =1; // if it goes in, then there is one of them
      key = 0;
      next =0;
    }
  void Print()
    {
      if(this->count)
	{
	vtkGenericWarningMacro("Class " << this->key << " has " 
			       << this->count << " instances still around" );
	}
    }
  ~vtkDebugLeaksHashNode()
    {
      delete [] key;
    }
public:
  vtkDebugLeaksHashNode *next;
  char *key;
  int count;
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
private:
  vtkDebugLeaksHashNode* nodes[64];
};

vtkDebugLeaksHashTable::vtkDebugLeaksHashTable()
{
  int i;
  for (i = 0; i < 64; i++)
    {
    this->nodes[i] = NULL;
    }
}



vtkDebugLeaks* vtkDebugLeaks::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkDebugLeaks");
  if(ret)
    {
    return (vtkDebugLeaks*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkDebugLeaks;
}


void vtkDebugLeaksHashTable::IncrementCount(const char * name)
{
  vtkDebugLeaksHashNode *pos;
  vtkDebugLeaksHashNode *newpos;
  int loc;
  pos = this->GetNode(name);
  if(pos)
    {
    pos->count++;
    return;
    }
  
  newpos = new vtkDebugLeaksHashNode;
  newpos->key = strcpy(new char[strlen(name)+1], name);

  loc = (((unsigned long)vtkHashString(name)) & 0x03f0) / 16;
  
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

vtkDebugLeaksHashNode* vtkDebugLeaksHashTable::GetNode(const char* key)
{
  vtkDebugLeaksHashNode *pos;
  int loc = (((unsigned long)vtkHashString(key)) & 0x03f0) / 16;
  
  pos = this->nodes[loc];

  if (!pos)
    {
    return NULL;
    }
  while ((pos) && (strcmp(pos->key, key) != 0) )
    {
    pos = pos->next;
    }
  return pos;
}

unsigned int vtkDebugLeaksHashTable::GetCount(const char* key)
{
  vtkDebugLeaksHashNode *pos;
  int loc = (((unsigned long)vtkHashString(key)) & 0x03f0) / 16;
  
  pos = this->nodes[loc];

  if (!pos)
    {
    return 0;
    }
  while ((pos)&&(pos->key != key))
    {
    pos = pos->next;
    }
  if (pos)
    {
    return pos->count;
    }
  return 0;
}

int vtkDebugLeaksHashTable::IsEmpty()
{
  int count = 0;
  for(int i =0; i < 64; i++)
    {
    vtkDebugLeaksHashNode *pos = this->nodes[i];
    if(pos)
      { 
      count += pos->count;
      while(pos->next)
	{
	pos = pos->next;
	count += pos->count;
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
    pos->count--;
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
    vtkDebugLeaksHashNode *pos = this->nodes[i];
    if(pos)
      { 
      pos->Print();
      while(pos->next)
	{
	pos = pos->next;
	pos->Print();
	}
      }
    }
}

vtkDebugLeaksHashTable* vtkDebugLeaks::MemoryTable = 0;
static vtkSimpleCriticalSection DebugLeaksCritSec;

void vtkDebugLeaks::ConstructClass(const char* name)
{
#ifdef VTK_DEBUG_LEAKS
  // force the use of the global varible so it gets constructed
  // but only do this if VTK_DEBUG_LEAKS is on
  vtkPrintLeaksAtExitGlobal.Use();
#endif  
  DebugLeaksCritSec.Lock();

  if(!vtkDebugLeaks::MemoryTable)
    vtkDebugLeaks::MemoryTable = new vtkDebugLeaksHashTable;
  
  vtkDebugLeaks::MemoryTable->IncrementCount(name);
  DebugLeaksCritSec.Unlock();
}

void vtkDebugLeaks::DestructClass(const char* p)
{
  DebugLeaksCritSec.Lock();
  // Due to globals being deleted, this table may already have
  // been deleted.
  if(vtkDebugLeaks::MemoryTable)
    {
    if(!vtkDebugLeaks::MemoryTable->DecrementCount(p))
      {
      vtkGenericWarningMacro("Deleting unknown object: " << p);
      }
    }
  DebugLeaksCritSec.Unlock();
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
  vtkOutputWindow::GetInstance()->PromptUserOn();
  vtkGenericWarningMacro("vtkDebugLeaks has detected LEAKS!\n ");
  vtkObjectFactory::UnRegisterAllFactories();
  vtkDebugLeaks::MemoryTable->PrintTable();
}

void vtkDebugLeaks::DeleteTable()
{
  delete vtkDebugLeaks::MemoryTable;
  vtkDebugLeaks::MemoryTable = 0;
}
