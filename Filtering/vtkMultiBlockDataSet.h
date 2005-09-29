/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiBlockDataSet - abstract superclass for hierarchical datasets
// .SECTION Description
// vtkMultiBlockDataSet is a vtkCompositeDataSet that stores
// a hierarchy of datasets. The dataset collection consists of
// multiple blocks. Each block can have zero or more sub-blocks.
// Sub-blocks are usually used to distribute blocks across processors.
// For example, a 1 block dataset can be distributed as following:
// @verbatim
// proc 0:
// Block 0:
//   * ds 0
//   * (null)
//
// proc 1:
// Block 0:
//   * (null)
//   * ds 1
// @endverbatim
//
// .SECTION See Also
// vtkMultiGroupDataSet

#ifndef __vtkMultiBlockDataSet_h
#define __vtkMultiBlockDataSet_h

#include "vtkMultiGroupDataSet.h"

class vtkDataObject;

class VTK_FILTERING_EXPORT vtkMultiBlockDataSet : public vtkMultiGroupDataSet
{
public:
  static vtkMultiBlockDataSet *New();

  vtkTypeRevisionMacro(vtkMultiBlockDataSet,vtkMultiGroupDataSet);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return class name of data type (see vtkType.h for
  // definitions).
  virtual int GetDataObjectType() {return VTK_MULTIBLOCK_DATA_SET;}

  // Description:
  // Set the number of blocks. This call might cause
  // allocation if the new number of blocks is larger than the
  // current one.
  void SetNumberOfBlocks(unsigned int numBlocks)
    {
      this->SetNumberOfGroups(numBlocks);
    }

  // Description:
  // Returns the number of blocks.
  unsigned int GetNumberOfBlocks()
    {
      return this->GetNumberOfGroups();
    }

  // Description:
  // Uses keys BLOCK() and INDEX() to call SetDataSet(BLOCK, INDEX, dobj)
  virtual void AddDataSet(vtkInformation* index, vtkDataObject* dobj);

  // Description:
  // Uses keys BLOCK() and INDEX() to call GetDataSet(BLOCK, INDEX)
  virtual vtkDataObject* GetDataSet(vtkInformation* index);

  vtkDataObject* GetDataSet(unsigned int block, unsigned int id)
    { return this->Superclass::GetDataSet(block, id); }

  static vtkInformationIntegerKey* BLOCK();

protected:
  vtkMultiBlockDataSet();
  ~vtkMultiBlockDataSet();

private:
  vtkMultiBlockDataSet(const vtkMultiBlockDataSet&);  // Not implemented.
  void operator=(const vtkMultiBlockDataSet&);  // Not implemented.
};

#endif

