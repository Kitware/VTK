/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHeap.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Tom Citriniti who implemented and contributed this class


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
  vtkTypeMacro(vtkHeap,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Allocate the memory requested.
  void* AllocateMemory(size_t n);
  
  // Description:
  // Convenience method performs string duplication.
  char* StrDup(const char* str);

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
