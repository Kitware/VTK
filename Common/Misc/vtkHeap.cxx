/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHeap.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCommonMiscModule.h" // For export macro
#include "vtkHeap.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkHeap);

struct vtkTestAlignLong
{
  char    pad;
  long    x;
};

static int vtkGetLongAlignment()
{
  struct vtkTestAlignLong    s1;
  char *               p1;
  char *               p2;

  p1 = reinterpret_cast<char *>(&s1);   // Get address of struct
  p2 = reinterpret_cast<char *>(&s1.x); // Get address of long within struct

  return (p2 - p1);    // Get member offset/alignment
}

class VTKCOMMONMISC_EXPORT vtkHeapBlock
{
public:
  char*         Data;
  vtkHeapBlock* Next;
  size_t        Size; //Variable size guards against block size changing from SetBlockSize()
                      //or large requests greater than the standard block size.

  vtkHeapBlock(size_t size):Next(0),Size(size)
    {this->Data = new char [size];}
  ~vtkHeapBlock()
    {delete [] this->Data;}
};

vtkHeap::vtkHeap()
{
  this->BlockSize = 256000;
  this->NumberOfBlocks = 0;
  this->NumberOfAllocations = 0;
  this->Alignment = vtkGetLongAlignment();
  this->First = 0;
  this->Last = 0;
  this->Current = 0;
  this->Position = 0;
}

vtkHeap::~vtkHeap()
{
  this->CleanAll();
}

void vtkHeap::SetBlockSize(size_t _arg)
{
  vtkDebugMacro(
    << this->GetClassName() << " (" << this << "): setting BlockSize to "
    << static_cast<int>(_arg));
  if (this->BlockSize != _arg)
    {
    this->BlockSize = _arg;
    this->Modified();
    }
}

void* vtkHeap::AllocateMemory(size_t n)
{
  if ( n%this->Alignment ) //4-byte word alignement
    {
    n += this->Alignment - (n%this->Alignment);
    }

  size_t blockSize = (n > this->BlockSize ? n : this->BlockSize );
  this->NumberOfAllocations++;

  if ( ! this->Current ||
       (this->Position + n) >= this->Current->Size )
    {
    this->Add(blockSize);
    }

  char *ptr = this->Current->Data + this->Position;
  this->Position += n;

  return ptr;
}

// If a Reset() was invoked, then we reuse memory (i.e., the list of blocks)
// or allocate it as necessary. Otherwise a block is allocated and placed into
// the list of blocks.
void vtkHeap::Add(size_t blockSize)
{
  this->Position = 0; //reset to the beginning of the block

  if ( this->Current && this->Current != this->Last &&
       this->Current->Next->Size >= blockSize ) //reuse
    {
    this->Current = this->Current->Next;
    }

  else //allocate a new block
    {
    this->NumberOfBlocks++;
    vtkHeapBlock* block = new vtkHeapBlock(blockSize);

    if (!this->Last)
      {
      this->First = block;
      this->Current = block;
      this->Last = block;
      return;
      }

    this->Last->Next = block;
    this->Last = block;
    this->Current = block;
    }
}

void vtkHeap::CleanAll()
{
  this->Current = this->First;
  if (!this->Current) { return; }
  while (this->DeleteAndNext())
    {
    ;
    }
  this->First = this->Current = this->Last = 0;
  this->Position = 0;
}

vtkHeapBlock* vtkHeap::DeleteAndNext()
{
  if (this->Current)
    {
    vtkHeapBlock* tmp = this->Current;
    this->Current = this->Current->Next;
    delete tmp;
    return this->Current;
    }
  else
    {
    return 0;
    }
}

void vtkHeap::Reset()
{
  this->Current = this->First;
  this->Position = 0;
}

char* vtkHeap::StringDup(const char* str)
{
  char *newStr = static_cast<char*>(this->AllocateMemory(strlen(str)+1));
  strcpy(newStr,str);
  return newStr;
}

void vtkHeap::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Block Size: " << static_cast<int>(this->BlockSize) << "\n";
  os << indent << "Number of Blocks: " << this->NumberOfBlocks << "\n";
  os << indent << "Number of Allocations: " << this->NumberOfAllocations << "\n";
  os << indent << "Current bytes allocated: "
     << ((this->NumberOfBlocks-1)*static_cast<int>(this->BlockSize) +
         static_cast<int>(this->Position)) << "\n";
}

