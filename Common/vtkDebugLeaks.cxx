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
#include "vtkObjectFactory.h"
#include "vtkCriticalSection.h"

static const char *vtkDebugLeaksIgnoreClasses[] = {
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

vtkCxxRevisionMacro(vtkDebugLeaks, "1.21");
vtkStandardNewMacro(vtkDebugLeaks);

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
  void Print(ostream& os)
    {
      if(this->Count)
        {
        os << "Class " << this->Key << " has " 
           << this->Count << ((this->Count==1)? " instance" : " instances")
           << " still around.\n";
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
  void PrintTable(ostream& os);
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

void vtkDebugLeaksHashTable::PrintTable(ostream& os)
{
  for(int i =0; i < 64; i++)
    {
    vtkDebugLeaksHashNode *pos = this->Nodes[i];
    if(pos)
      { 
      if(!vtkDebugLeaksIgnoreClassesCheck(pos->Key))
        {
        pos->Print(os);
        }
      while(pos->Next)
        {
        pos = pos->Next;
        if(!vtkDebugLeaksIgnoreClassesCheck(pos->Key))
          {
          pos->Print(os);
          }
        }
      }
    }
}



#ifdef VTK_DEBUG_LEAKS
void vtkDebugLeaks::ConstructClass(const char* name)
{
  vtkDebugLeaks::CriticalSection->Lock();
  vtkDebugLeaks::MemoryTable->IncrementCount(name);
  vtkDebugLeaks::CriticalSection->Unlock();
}
#else
void vtkDebugLeaks::ConstructClass(const char*)
{
}
#endif

#ifdef VTK_DEBUG_LEAKS
void vtkDebugLeaks::DestructClass(const char* p)
{
  vtkDebugLeaks::CriticalSection->Lock();
  // Due to globals being deleted, this table may already have
  // been deleted.
  if(vtkDebugLeaks::MemoryTable &&
     !vtkDebugLeaks::MemoryTable->DecrementCount(p))
    {
    vtkDebugLeaks::CriticalSection->Unlock();
    vtkGenericWarningMacro("Deleting unknown object: " << p);
    }
  else
    {
    vtkDebugLeaks::CriticalSection->Unlock();
    }
}
#else
void vtkDebugLeaks::DestructClass(const char*)
{
}
#endif

void vtkDebugLeaks::PrintCurrentLeaks()
{
#ifdef VTK_DEBUG_LEAKS
  if(vtkDebugLeaks::MemoryTable->IsEmpty())
    {
    return;
    }
  // print the table
  strstream leaks;
  vtkDebugLeaks::MemoryTable->PrintTable(leaks);
  leaks << ends;
  
#ifdef _WIN32
  int cancel=0;
  while(!cancel && !!leaks)
    {
    char line[1000];
    strstream msg;
    msg << "vtkDebugLeaks has detected LEAKS!\n";
    int i=0;
    while((++i <= 10) && !!leaks.getline(line, 1000))
      {
      msg << line << "\n";
      }
    msg << ends;
    cancel = vtkDebugLeaks::DisplayMessageBox(msg.str());
    msg.rdbuf()->freeze(0);
    }
#else
  cout << "vtkDebugLeaks has detected LEAKS!\n";
  cout << leaks.rdbuf() << "\n";
#endif
#endif
}

#ifdef _WIN32
int vtkDebugLeaks::DisplayMessageBox(const char* msg)
{
#ifdef UNICODE
  wchar_t *wmsg = new wchar_t [mbstowcs(NULL, msg, 32000)];
  mbstowcs(wmsg, msg, 32000);
  int result = (MessageBox(NULL, wmsg, L"Error",
                           MB_ICONERROR | MB_OKCANCEL) == IDCANCEL);
  delete [] wmsg;
#else
  int result = (MessageBox(NULL, msg, "Error",
                           MB_ICONERROR | MB_OKCANCEL) == IDCANCEL);
#endif
  return result;
}
#else
int vtkDebugLeaks::DisplayMessageBox(const char*)
{
  return 0;
}
#endif

//----------------------------------------------------------------------------
void vtkDebugLeaks::ClassInitialize()
{
#ifdef VTK_DEBUG_LEAKS
  // Create the hash table.
  vtkDebugLeaks::MemoryTable = new vtkDebugLeaksHashTable;
  
  // Create the lock for the critical sections.
  vtkDebugLeaks::CriticalSection = new vtkSimpleCriticalSection;
#else
  vtkDebugLeaks::MemoryTable = 0;
  vtkDebugLeaks::CriticalSection = 0;
#endif
}

//----------------------------------------------------------------------------
void vtkDebugLeaks::ClassFinalize()
{
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::PrintCurrentLeaks();
  
  // Destroy the hash table.
  delete vtkDebugLeaks::MemoryTable;
  vtkDebugLeaks::MemoryTable = 0;
  
  // Destroy the lock for the critical sections.
  delete vtkDebugLeaks::CriticalSection;
  vtkDebugLeaks::CriticalSection = 0;
#endif
}

//----------------------------------------------------------------------------

// Purposely not initialized.  ClassInitialize will handle it.
vtkDebugLeaksHashTable* vtkDebugLeaks::MemoryTable;

// Purposely not initialized.  ClassInitialize will handle it.
vtkSimpleCriticalSection* vtkDebugLeaks::CriticalSection;
