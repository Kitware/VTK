/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRDataInternals2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAMRDataInternals2 - container of vtkUniformGrid for an AMR data set
//
// .SECTION Description
// vtkAMRDataInternals2 stores a list of non-empty blocks of an AMR data set
//
// .SECTION See Also
// vtkOverlappingAMR, vtkAMRBox

#ifndef vtkAMRDataInternals2_h
#define vtkAMRDataInternals2_h

#include "vtkObject.h"
#include "vtkSmartPointer.h" //for storing smart pointers to blocks
#include <vector> //for storing blocks

class vtkUniformGrid;
class VTK_EXPORT vtkAMRDataInternals2 : public vtkObject
{
public:
  struct Block
  {
    vtkSmartPointer<vtkUniformGrid> Grid;
    unsigned int Index;
    Block(unsigned int i, vtkUniformGrid* g);
  };
  typedef std::vector<vtkAMRDataInternals2::Block> BlockList;

  static vtkAMRDataInternals2* New();
  vtkTypeMacro(vtkAMRDataInternals2, vtkObject);

  void Initialize(unsigned int size=0);
  void PrintSelf(ostream& os, vtkIndent indent);

  void Insert(unsigned int index, vtkUniformGrid* grid);
  vtkUniformGrid* GetDataSet(unsigned int compositeIndex);

  virtual void ShallowCopy(vtkObject *src);

  bool Empty()const{ return this->GetNumberOfBlocks()==0;}

public:
  unsigned int GetNumberOfBlocks() const{ if(this->HasBlocksInserted) this->CompactBlocks(); return static_cast<unsigned int>(this->Blocks.size());}
  const Block& GetBlock(unsigned int i) { if(this->HasBlocksInserted) this->CompactBlocks(); return this->Blocks[i];}
  const BlockList& GetAllBlocks() const{ if(this->HasBlocksInserted) this->CompactBlocks(); return this->Blocks;}

protected:

  vtkAMRDataInternals2();
  ~vtkAMRDataInternals2();

  mutable std::vector<Block> Blocks;
  std::vector<Block*> SparseBlocks;
  mutable bool HasBlocksInserted;
  void CompactBlocks() const;
  void ClearSparseBlocks();

private:
  vtkAMRDataInternals2(const vtkAMRDataInternals2&); // Not implemented.
  void operator=(const vtkAMRDataInternals2&); // Not implemented
};

#endif
