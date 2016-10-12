/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHeap.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHeap
 * @brief   replacement for malloc/free and new/delete
 *
 * This class is a replacement for malloc/free and new/delete for software
 * that has inherent memory leak or performance problems. For example,
 * external software such as the PLY library (vtkPLY) and VRML importer
 * (vtkVRMLImporter) are often written with lots of malloc() calls but
 * without the corresponding free() invocations. The class
 * vtkOrderedTriangulator may create and delete millions of new/delete calls.
 * This class allows the overloading of the C++ new operator (or other memory
 * allocation requests) by using the method AllocateMemory(). Memory is
 * deleted with an invocation of CleanAll() (which deletes ALL memory; any
 * given memory allocation cannot be deleted). Note: a block size can be used
 * to control the size of each memory allocation. Requests for memory are
 * fulfilled from the block until the block runs out, then a new block is
 * created.
 *
 * @warning
 * Do not use this class as a general replacement for system memory
 * allocation.  This class should be used only as a last resort if memory
 * leaks cannot be tracked down and eliminated by conventional means. Also,
 * deleting memory from vtkHeap is not supported. Only the deletion of
 * the entire heap is. (A Reset() method allows you to reuse previously
 * allocated memory.)
 *
 * @sa
 * vtkVRMLImporter vtkPLY vtkOrderedTriangulator
*/

#ifndef vtkHeap_h
#define vtkHeap_h

#include "vtkCommonMiscModule.h" // For export macro
#include "vtkObject.h"

class vtkHeapBlock; //forward declaration

class VTKCOMMONMISC_EXPORT vtkHeap : public vtkObject
{
public:
  static vtkHeap *New();
  vtkTypeMacro(vtkHeap,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Allocate the memory requested.
   */
  void* AllocateMemory(size_t n);

  //@{
  /**
   * Set/Get the size at which blocks are allocated. If a memory
   * request is bigger than the block size, then that size
   * will be allocated.
   */
  virtual void SetBlockSize(size_t);
  virtual size_t GetBlockSize() { return this->BlockSize;};
  //@}

  //@{
  /**
   * Get the number of allocations thus far.
   */
  vtkGetMacro(NumberOfBlocks,int);
  vtkGetMacro(NumberOfAllocations,int);
  //@}

  /**
   * This methods resets the current allocation location
   * back to the beginning of the heap. This allows
   * reuse of previously allocated memory which may be
   * beneficial to performance in many cases.
   */
  void Reset();

  /**
   * Convenience method performs string duplication.
   */
  char* StringDup(const char* str);

protected:
  vtkHeap();
  ~vtkHeap() VTK_OVERRIDE;

  void Add(size_t blockSize);
  void CleanAll();
  vtkHeapBlock* DeleteAndNext();

  size_t BlockSize;
  int    NumberOfAllocations;
  int    NumberOfBlocks;
  size_t Alignment;

  // Manage the blocks
  vtkHeapBlock* First;
  vtkHeapBlock* Last;
  vtkHeapBlock* Current;
  // Manage the memory in the block
  size_t Position; //the position in the Current block

private:
  vtkHeap(const vtkHeap&) VTK_DELETE_FUNCTION;
  void operator=(const vtkHeap&) VTK_DELETE_FUNCTION;
};

#endif
