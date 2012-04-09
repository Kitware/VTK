/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDebugLeaks.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDebugLeaks.h"

#include "vtkCriticalSection.h"
#include "vtkObjectFactory.h"
#include "vtkWindows.h"

#include <string>

static const char *vtkDebugLeaksIgnoreClasses[] = {
  0
};

//----------------------------------------------------------------------------
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

vtkStandardNewMacro(vtkDebugLeaks);

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
class vtkDebugLeaksHashNode
{
public:
  vtkDebugLeaksHashNode()
    {
      this->Count =1; // if it goes in, then there is one of them
      this->Key = 0;
      this->Next =0;
    }
  void Print(std::string& os)
    {
      if(this->Count)
        {
        char tmp[256];
        sprintf(tmp,"\" has %i %s still around.\n",this->Count,
                (this->Count == 1) ? "instance" : "instances");
        os += "Class \"";
        os += this->Key;
        os += tmp;
        }
    }
  ~vtkDebugLeaksHashNode()
    {
      delete [] this->Key;
      if(this->Next)
        {
        delete this->Next;
        }
    }
public:
  vtkDebugLeaksHashNode *Next;
  char *Key;
  int Count;
};

//----------------------------------------------------------------------------
class vtkDebugLeaksHashTable
{
public:
  vtkDebugLeaksHashTable();
  vtkDebugLeaksHashNode* GetNode(const char* name);
  void IncrementCount(const char *name);
  unsigned int GetCount(const char *name);
  int DecrementCount(const char* name);
  void PrintTable(std::string &os);
  int IsEmpty();
  ~vtkDebugLeaksHashTable()
    {
      for (int i = 0; i < 64; i++)
        {
        vtkDebugLeaksHashNode *pos = this->Nodes[i];
        if(pos)
          {
          delete pos;
          }
        }
    }

private:
  vtkDebugLeaksHashNode* Nodes[64];
};

//----------------------------------------------------------------------------
vtkDebugLeaksHashTable::vtkDebugLeaksHashTable()
{
  int i;
  for (i = 0; i < 64; i++)
    {
    this->Nodes[i] = NULL;
    }
}

//----------------------------------------------------------------------------
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

  loc = (static_cast<unsigned long>(vtkHashString(name)) & 0x03f0) / 16;

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

//----------------------------------------------------------------------------
vtkDebugLeaksHashNode* vtkDebugLeaksHashTable::GetNode(const char* key)
{
  vtkDebugLeaksHashNode *pos;
  int loc = (static_cast<unsigned long>(vtkHashString(key)) & 0x03f0) / 16;

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

//----------------------------------------------------------------------------
unsigned int vtkDebugLeaksHashTable::GetCount(const char* key)
{
  vtkDebugLeaksHashNode *pos;
  int loc = (static_cast<unsigned long>(vtkHashString(key)) & 0x03f0) / 16;

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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkDebugLeaksHashTable::PrintTable(std::string &os)
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkDebugLeaks::SetDebugLeaksObserver(vtkDebugLeaksObserver* observer)
{
  vtkDebugLeaks::Observer = observer;
}

//----------------------------------------------------------------------------
vtkDebugLeaksObserver* vtkDebugLeaks::GetDebugLeaksObserver()
{
  return vtkDebugLeaks::Observer;
}

//----------------------------------------------------------------------------
void vtkDebugLeaks::ConstructingObject(vtkObjectBase* object)
{
  if (vtkDebugLeaks::Observer)
    {
    vtkDebugLeaks::Observer->ConstructingObject(object);
    }
}

//----------------------------------------------------------------------------
void vtkDebugLeaks::DestructingObject(vtkObjectBase* object)
{
  if (vtkDebugLeaks::Observer)
    {
    vtkDebugLeaks::Observer->DestructingObject(object);
    }
}

//----------------------------------------------------------------------------
int vtkDebugLeaks::PrintCurrentLeaks()
{
#ifdef VTK_DEBUG_LEAKS
  if(vtkDebugLeaks::MemoryTable->IsEmpty())
    {
    return 0;
    }

  // we must not use stringstream ior strstream due to problems with
  // finalizers in MSVC
  std::string leaks;
  vtkDebugLeaks::MemoryTable->PrintTable(leaks);
#ifdef _WIN32
  fprintf(stderr,"%s",leaks.c_str());
  fflush(stderr);
  int cancel=0;
  if(getenv("DASHBOARD_TEST_FROM_CTEST") ||
     getenv("DART_TEST_FROM_DART"))
    {
    // Skip dialogs when running on dashboard.
    return 1;
    }
  std::string::size_type myPos = 0;
  int count = 0;
  std::string msg;
  msg = "vtkDebugLeaks has detected LEAKS!\n";
  while(!cancel && myPos != leaks.npos)
    {
    std::string::size_type newPos = leaks.find('\n',myPos);
    if (newPos != leaks.npos)
      {
      msg += leaks.substr(myPos,newPos-myPos);
      msg += "\n";
      myPos = newPos;
      myPos++;
      }
    else
      {
      myPos = newPos;
      }
    count++;
    if (count == 10)
      {
      count = 0;
      cancel = vtkDebugLeaks::DisplayMessageBox(msg.c_str());
      msg = "";
      }
    }
  if (!cancel && count > 0)
    {
    vtkDebugLeaks::DisplayMessageBox(msg.c_str());
    }
#else
  cout << "vtkDebugLeaks has detected LEAKS!\n";
  cout << leaks.c_str() << endl;
#endif
#endif
  return 1;
}

//----------------------------------------------------------------------------
#ifdef _WIN32
int vtkDebugLeaks::DisplayMessageBox(const char* msg)
{
#ifdef UNICODE
  wchar_t *wmsg = new wchar_t [mbstowcs(NULL, msg, 32000)+1];
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
int vtkDebugLeaks::GetExitError()
{
  return vtkDebugLeaks::ExitError;
}

//----------------------------------------------------------------------------
void vtkDebugLeaks::SetExitError(int flag)
{
  vtkDebugLeaks::ExitError = flag;
}

//----------------------------------------------------------------------------
void vtkDebugLeaks::ClassInitialize()
{
#ifdef VTK_DEBUG_LEAKS
  // Create the hash table.
  vtkDebugLeaks::MemoryTable = new vtkDebugLeaksHashTable;

  // Create the lock for the critical sections.
  vtkDebugLeaks::CriticalSection = new vtkSimpleCriticalSection;

  // Default to error when leaks occur while running tests.
  vtkDebugLeaks::ExitError = 1;
  vtkDebugLeaks::Observer = 0;
#else
  vtkDebugLeaks::MemoryTable = 0;
  vtkDebugLeaks::CriticalSection = 0;
  vtkDebugLeaks::ExitError = 0;
  vtkDebugLeaks::Observer = 0;
#endif
}

//----------------------------------------------------------------------------
void vtkDebugLeaks::ClassFinalize()
{
#ifdef VTK_DEBUG_LEAKS
  // Report leaks.
  int leaked = vtkDebugLeaks::PrintCurrentLeaks();

  // Destroy the hash table.
  delete vtkDebugLeaks::MemoryTable;
  vtkDebugLeaks::MemoryTable = 0;

  // Destroy the lock for the critical sections.
  delete vtkDebugLeaks::CriticalSection;
  vtkDebugLeaks::CriticalSection = 0;

  // Exit with error if leaks occurred and error mode is on.
  if(leaked && vtkDebugLeaks::ExitError)
    {
    exit(1);
    }
#endif
}

//----------------------------------------------------------------------------

// Purposely not initialized.  ClassInitialize will handle it.
vtkDebugLeaksHashTable* vtkDebugLeaks::MemoryTable;

// Purposely not initialized.  ClassInitialize will handle it.
vtkSimpleCriticalSection* vtkDebugLeaks::CriticalSection;

// Purposely not initialized.  ClassInitialize will handle it.
int vtkDebugLeaks::ExitError;

// Purposely not initialized.  ClassInitialize will handle it.
vtkDebugLeaksObserver* vtkDebugLeaks::Observer;
