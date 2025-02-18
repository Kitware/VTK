// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHyperTreeGridCellSizeStrategy.h"

#include "vtkDoubleArray.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridCellSizeStrategy);

namespace
{
/**
 * Return the size of the cell pointed by the cursor.
 * In practice, we multiply every non-null size value for the current cell.
 */
double GetCellSize(vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  double cellSize = 1.0;
  bool nullSize = true;
  double* size = cursor->GetSize();
  std::vector<double> dimensions(size, size + 3);
  for (auto& edgeSize : dimensions)
  {
    if (edgeSize != 0.0)
    {
      nullSize = false;
      cellSize *= edgeSize;
    }
  }
  if (nullSize)
  {
    // Every size coordinate is null, so the cell size is also null
    cellSize = 0.0;
  }
  return cellSize;
}
}

//------------------------------------------------------------------------------
vtkHyperTreeGridCellSizeStrategy::vtkHyperTreeGridCellSizeStrategy() = default;

//------------------------------------------------------------------------------
vtkHyperTreeGridCellSizeStrategy::~vtkHyperTreeGridCellSizeStrategy() = default;

//------------------------------------------------------------------------------
void vtkHyperTreeGridCellSizeStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseIndexedVolume: " << this->UseIndexedVolume << "\n";
  os << indent << "VolumeLookup size: " << this->VolumeLookup.size() << "\n";
  os << indent << "SizeIndirectionTable size: "
     << (this->SizeIndirectionTable ? this->SizeIndirectionTable->GetNumberOfTuples() : 0) << "\n";
  os << indent << "SizeDiscreteValues size: "
     << (this->SizeDiscreteValues ? this->SizeDiscreteValues->GetNumberOfTuples() : 0) << "\n";
  os << indent << "SizeFullValues size: "
     << (this->SizeFullValues ? this->SizeFullValues->GetNumberOfTuples() : 0) << "\n";
  os << indent << "OutputSizeArray size: "
     << (this->OutputSizeArray ? this->OutputSizeArray->GetNumberOfTuples() : 0) << "\n";
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridCellSizeStrategy::InsertSize(double cellSize, vtkIdType currentIndex)
{
  // Use a hash table for O(1) insertion and search time instead of searching the VTK array
  const auto& inserted = this->VolumeLookup.insert(
    std::make_pair(cellSize, static_cast<unsigned char>(this->VolumeLookup.size())));
  if (inserted.second)
  {
    // Insertion succeeded : new element in the lookup table
    this->SizeDiscreteValues->InsertTuple1(this->SizeDiscreteValues->GetNumberOfTuples(), cellSize);
  }
  // New element or not, fill the indirection table
  this->SizeIndirectionTable->SetTuple1(currentIndex, inserted.first->second);
  return inserted.first->second < std::numeric_limits<unsigned char>::max();
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridCellSizeStrategy::ConvertSizes()
{
  // Dump the volume values from the map keys
  std::vector<double> temp_volume;
  temp_volume.resize(this->VolumeLookup.size(), 0.0);
  for (const auto& vol_id : this->VolumeLookup)
  {
    temp_volume[vol_id.second] = vol_id.first;
  }
  this->VolumeLookup.clear();

  // Construct the volume array
  this->SizeFullValues->SetNumberOfComponents(1);
  this->SizeFullValues->SetNumberOfTuples(SizeIndirectionTable->GetNumberOfTuples());
  for (int i = 0; i < SizeIndirectionTable->GetNumberOfTuples(); i++)
  {
    this->SizeFullValues->SetTuple1(i, temp_volume[SizeIndirectionTable->GetTuple1(i)]);
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridCellSizeStrategy::Initialize(vtkHyperTreeGrid* inputHTG)
{
  this->UseIndexedVolume = true;
  this->SizeDiscreteValues->SetNumberOfComponents(
    1); // We don't know yet how many different values we can have

  this->OutputSizeArray->SetNumberOfComponents(1);
  this->OutputSizeArray->SetNumberOfTuples(inputHTG->GetNumberOfCells());

  this->SizeIndirectionTable->SetNumberOfComponents(1);
  this->SizeIndirectionTable->SetNumberOfTuples(inputHTG->GetNumberOfCells());

  // Initialize the whole indirection array with 0 values
  this->InsertSize(0.0, 0); // Make sure size 0 is in the size lookup map.
  for (vtkIdType i = 0; i < this->SizeIndirectionTable->GetNumberOfValues(); i++)
  {
    this->SizeIndirectionTable->SetTuple1(i, 0.0);
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridCellSizeStrategy::Compute(vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  double cellSize = ::GetCellSize(cursor);
  vtkIdType currentIndex = cursor->GetGlobalNodeIndex();
  if (!this->UseIndexedVolume)
  {
    // We don't use the implicit array anymore but a full Size array
    this->SizeFullValues->SetTuple1(currentIndex, cellSize);
    return;
  }

  // Try to insert size in the indexed array
  if (this->InsertSize(cellSize, currentIndex))
  {
    return;
  }

  // We have too many different size values to store them in an unsigned char,
  // so at this point, we give up on implicit indexed array and use a classic VTK double array to
  // store values. This requires that values are copied from the indirect storage array to the
  // final double array.
  this->UseIndexedVolume = false;
  this->ConvertSizes();
}

//------------------------------------------------------------------------------
vtkDataArray* vtkHyperTreeGridCellSizeStrategy::GetAndFinalizeArray()
{
  if (this->UseIndexedVolume)
  {
    // The size values take a discrete number of different values : one value for each level
    // Thus, we can use an indexed (implicit) array as an indirection table to store the size as a
    // uchar (256 possible values/levels) instead of a double for each cell to save memory (1 byte
    // stored instead of 8)
    this->OutputSizeArray->SetName(this->ArrayName.c_str());
    this->OutputSizeArray->SetNumberOfComponents(1);
    this->OutputSizeArray->SetNumberOfTuples(this->SizeIndirectionTable->GetNumberOfValues());
    this->OutputSizeArray->SetBackend(std::make_shared<vtkIndexedImplicitBackend<double>>(
      SizeIndirectionTable, SizeDiscreteValues));
    return this->OutputSizeArray;
  }
  else
  {
    this->SizeFullValues->SetName(this->ArrayName.c_str());
    return this->SizeFullValues;
  }
}

VTK_ABI_NAMESPACE_END
