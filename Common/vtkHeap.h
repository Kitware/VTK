/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHeap.h
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
// .NAME vtkHeap - replacement for malloc/free and new/delete
// .SECTION Description
// This class is a replacement for malloc/free and new/delete for software
// that has inherent memory leak problems. For example, external software
// such as the PLY library (vtkPLY) and VRML importer (vtkVRMLImporter) are
// often written with lots of malloc() calls but without the corresponding
// free() invocations. This class allows the replacement of malloc() with
// the method AllocateMemory(). Memory is then deleted with an invocation
// of CleanUp().
//
// .SECTION Caveats
// Do not use this method as a replacement for system memory allocation.
// This class should be used only as a last resort if memory leaks cannot
// be tracked down and eliminated by conventional means.

// .SECTION See Also
// vtkVRMLImporter vtkPLY

#ifndef __vtkHeap_h
#define __vtkHeap_h

#include "vtkObject.h"

//BTX
class VTK_COMMON_EXPORT vtkHeapNode
{
public:
  void* Ptr;
  vtkHeapNode* Next;
  vtkHeapNode():Ptr(0),Next(0) {};
};
//ETX

class VTK_COMMON_EXPORT vtkHeap : public vtkObject
{
public:
  static vtkHeap *New();
  vtkTypeRevisionMacro(vtkHeap,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Allocate the memory requested.
  void* AllocateMemory(size_t n);
  
  // Description:
  // Convenience method performs string duplication.
  char* vtkStrDup(const char* str);

  // Description:
  // Get the number of allocations thus far.
  vtkGetMacro(NumberOfAllocations,int);

protected:  
  vtkHeap();
  ~vtkHeap();

  void Add(vtkHeapNode* node);
  void CleanAll();
  vtkHeapNode* DeleteAndNext();

  vtkHeapNode* First;
  vtkHeapNode* Last;
  vtkHeapNode* Current;

  int NumberOfAllocations;
private:
  vtkHeap(const vtkHeap&); // Not implemented.
  void operator=(const vtkHeap&);  // Not implemented.
};

#endif
