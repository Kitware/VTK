// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAMRDataInternals
 * @brief   container of vtkUniformGrid for an AMR data set
 *
 *
 * vtkAMRDataInternals stores a list of non-empty blocks of an AMR data set
 *
 * @sa
 * vtkOverlappingAMR, vtkAMRBox
 */

#ifndef vtkAMRDataInternals_h
#define vtkAMRDataInternals_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"
#include "vtkSmartPointer.h" //for storing smart pointers to blocks
#include <vector>            //for storing blocks

VTK_ABI_NAMESPACE_BEGIN
class vtkUniformGrid;
class VTKCOMMONDATAMODEL_EXPORT vtkAMRDataInternals : public vtkObject
{
public:
  struct Block
  {
    vtkSmartPointer<vtkUniformGrid> Grid;
    unsigned int Index;
    Block(unsigned int i, vtkUniformGrid* g);
  };
  typedef std::vector<vtkAMRDataInternals::Block> BlockList;

  static vtkAMRDataInternals* New();
  vtkTypeMacro(vtkAMRDataInternals, vtkObject);

  void Initialize();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Insert(unsigned int index, vtkUniformGrid* grid);
  vtkUniformGrid* GetDataSet(unsigned int compositeIndex);

  void CompositeShallowCopy(vtkObject* src);
  void ShallowCopy(vtkObject* src);
  void DeepCopy(vtkObject* src);

  bool Empty() const { return this->GetNumberOfBlocks() == 0; }

public:
  unsigned int GetNumberOfBlocks() const { return static_cast<unsigned int>(this->Blocks.size()); }
  const Block& GetBlock(unsigned int i) { return this->Blocks[i]; }
  const BlockList& GetAllBlocks() const { return this->Blocks; }

protected:
  vtkAMRDataInternals();
  ~vtkAMRDataInternals() override;

  void GenerateIndex(bool force = false);

  std::vector<Block> Blocks;
  std::vector<int>* InternalIndex; // map from the composite index to internal index
  bool GetInternalIndex(unsigned int compositeIndex, unsigned int& internalIndex);

private:
  vtkAMRDataInternals(const vtkAMRDataInternals&) = delete;
  void operator=(const vtkAMRDataInternals&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
