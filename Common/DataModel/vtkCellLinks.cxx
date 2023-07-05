// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellLinks.h"

#include "vtkCellArray.h"
#include "vtkDataSet.h"
#include "vtkGenericCell.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkCellLinks);

//------------------------------------------------------------------------------
vtkCellLinks::vtkCellLinks()
  : Array(nullptr)
  , Size(0)
  , MaxId(-1)
  , Extend(1000)
  , NumberOfPoints(0)
  , NumberOfCells(0)
{
  this->Type = vtkAbstractCellLinks::CELL_LINKS;
}

//------------------------------------------------------------------------------
vtkCellLinks::~vtkCellLinks()
{
  this->Initialize();
}

//------------------------------------------------------------------------------
void vtkCellLinks::Initialize()
{
  if (this->Array != nullptr)
  {
    for (vtkIdType i = 0; i <= this->MaxId; i++)
    {
      delete[] this->Array[i].cells;
    }

    delete[] this->Array;
    this->Array = nullptr;
  }
  this->Size = 0;
  this->NumberOfPoints = 0;
  this->NumberOfCells = 0;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkCellLinks::Allocate(vtkIdType sz, vtkIdType ext)
{
  static vtkCellLinks::Link linkInit = { 0, nullptr };

  this->Initialize();
  this->Size = sz;
  this->Array = new vtkCellLinks::Link[sz];
  this->Extend = ext;
  this->MaxId = -1;

  for (vtkIdType i = 0; i < sz; i++)
  {
    this->Array[i] = linkInit;
  }
  this->Modified();
}

//------------------------------------------------------------------------------
namespace
{
struct LinkAllocator
{
private:
  vtkCellLinks::Link* LinkArray;

public:
  LinkAllocator(vtkCellLinks::Link* linkArray)
    : LinkArray(linkArray)
  {
  }
  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    for (; cellId < endCellId; ++cellId)
    {
      this->LinkArray[cellId].cells = new vtkIdType[this->LinkArray[cellId].ncells];
    }
  }
};
} // namespace

//----------------------------------------------------------------------------
// Allocate memory for the list of lists of cell ids.
void vtkCellLinks::AllocateLinks(vtkIdType n)
{
  LinkAllocator allocator(this->Array);
  vtkSMPTools::For(0, n, allocator);
}

//------------------------------------------------------------------------------
// Reclaim any unused memory.
void vtkCellLinks::Squeeze()
{
  this->Resize(this->MaxId + 1);
}

//------------------------------------------------------------------------------
void vtkCellLinks::Reset()
{
  this->MaxId = -1;
  this->Modified();
}

//------------------------------------------------------------------------------
//
// Private function does "reallocate"
//
vtkCellLinks::Link* vtkCellLinks::Resize(vtkIdType sz)
{
  vtkIdType i;
  vtkCellLinks::Link* newArray;
  vtkIdType newSize;
  vtkCellLinks::Link linkInit = { 0, nullptr };

  if (sz >= this->Size)
  {
    newSize = this->Size + sz;
  }
  else
  {
    newSize = sz;
  }

  newArray = new vtkCellLinks::Link[newSize];

  for (i = 0; i < sz && i < this->Size; i++)
  {
    newArray[i] = this->Array[i];
  }

  for (i = this->Size; i < newSize; i++)
  {
    newArray[i] = linkInit;
  }

  this->Size = newSize;
  delete[] this->Array;
  this->Array = newArray;

  return this->Array;
}

//------------------------------------------------------------------------------
// Build the link list array.
void vtkCellLinks::BuildLinks()
{
  // don't rebuild if build time is newer than modified and dataset modified time
  if (this->Array && this->BuildTime > this->MTime && this->BuildTime > this->DataSet->GetMTime())
  {
    return;
  }
  vtkIdType numPts = this->NumberOfPoints = this->DataSet->GetNumberOfPoints();
  vtkIdType numCells = this->NumberOfCells = this->DataSet->GetNumberOfCells();
  int j;
  vtkIdType cellId;

  if (this->Array == nullptr)
  {
    this->Allocate(numPts);
  }
  // This is checked to capture changes in the size of the allocation
  else if (this->DataSet->GetMTime() > this->BuildTime && this->DataSet->GetMTime() > this->MTime)
  {
    this->Allocate(numPts);
  }

  // fill out lists with number of references to cells
  std::vector<vtkIdType> linkLoc(numPts, 0);

  // Use fast path if polydata
  if (this->DataSet->GetDataObjectType() == VTK_POLY_DATA)
  {
    vtkIdType npts;
    const vtkIdType* pts;

    vtkPolyData* pdata = static_cast<vtkPolyData*>(this->DataSet);
    // traverse data to determine number of uses of each point
    for (cellId = 0; cellId < numCells; cellId++)
    {
      pdata->GetCellPoints(cellId, npts, pts);
      for (j = 0; j < npts; j++)
      {
        this->IncrementLinkCount(pts[j]);
      }
    }

    // now allocate storage for the links
    this->AllocateLinks(numPts);
    this->MaxId = numPts - 1;

    for (cellId = 0; cellId < numCells; cellId++)
    {
      pdata->GetCellPoints(cellId, npts, pts);
      for (j = 0; j < npts; j++)
      {
        this->InsertCellReference(pts[j], (linkLoc[pts[j]])++, cellId);
      }
    }
  }

  else // any other type of dataset
  {
    vtkIdType numberOfPoints, ptId;
    vtkGenericCell* cell = vtkGenericCell::New();

    // traverse data to determine number of uses of each point
    for (cellId = 0; cellId < numCells; cellId++)
    {
      this->DataSet->GetCell(cellId, cell);
      numberOfPoints = cell->GetNumberOfPoints();
      for (j = 0; j < numberOfPoints; j++)
      {
        this->IncrementLinkCount(cell->PointIds->GetId(j));
      }
    }

    // now allocate storage for the links
    this->AllocateLinks(numPts);
    this->MaxId = numPts - 1;

    for (cellId = 0; cellId < numCells; cellId++)
    {
      this->DataSet->GetCell(cellId, cell);
      numberOfPoints = cell->GetNumberOfPoints();
      for (j = 0; j < numberOfPoints; j++)
      {
        ptId = cell->PointIds->GetId(j);
        this->InsertCellReference(ptId, (linkLoc[ptId])++, cellId);
      }
    }
    cell->Delete();
  } // end else
  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
// Insert a new point into the cell-links data structure. The size parameter
// is the initial size of the list.
vtkIdType vtkCellLinks::InsertNextPoint(int numLinks)
{
  if (++this->MaxId >= this->Size)
  {
    this->Resize(this->MaxId + 1);
  }
  this->Array[this->MaxId].cells = new vtkIdType[numLinks];
  return this->MaxId;
}

//------------------------------------------------------------------------------
// Mark cells with one or more points whose degree lies in the range indicated.
void vtkCellLinks::SelectCells(vtkIdType minMaxDegree[2], unsigned char* cellSelection)
{
  std::fill_n(cellSelection, this->NumberOfCells, 0);
  vtkSMPTools::For(0, this->NumberOfPoints,
    [this, minMaxDegree, cellSelection](vtkIdType ptId, vtkIdType endPtId) {
      for (; ptId < endPtId; ++ptId)
      {
        vtkIdType degree = this->GetNcells(0);
        if (degree >= minMaxDegree[0] && degree < minMaxDegree[1])
        {
          vtkIdType* cells = this->GetCells(ptId);
          for (auto i = 0; i < degree; ++i)
          {
            cellSelection[cells[i]] = 1;
          }
        }
      } // for all points in this batch
    }); // end lambda
}

//------------------------------------------------------------------------------
unsigned long vtkCellLinks::GetActualMemorySize()
{
  vtkIdType size = 0;
  vtkIdType ptId;

  for (ptId = 0; ptId < (this->MaxId + 1); ptId++)
  {
    size += this->GetNcells(ptId);
  }

  size *= sizeof(int*);                                   // references to cells
  size += (this->MaxId + 1) * sizeof(vtkCellLinks::Link); // list of cell lists

  return static_cast<unsigned long>(ceil(size / 1024.0)); // kibibytes
}

//------------------------------------------------------------------------------
void vtkCellLinks::DeepCopy(vtkAbstractCellLinks* src)
{
  this->SetDataSet(src->GetDataSet());
  this->SetSequentialProcessing(src->GetSequentialProcessing());
  vtkCellLinks* clinks = static_cast<vtkCellLinks*>(src);
  this->Allocate(clinks->Size, clinks->Extend);
  vtkSMPTools::For(0, clinks->MaxId + 1, [this, clinks](vtkIdType ptId, vtkIdType endPtId) {
    for (; ptId < endPtId; ++ptId)
    {
      vtkIdType ncells = clinks->GetNcells(ptId);
      this->Array[ptId].cells = new vtkIdType[ncells];
      this->Array[ptId].ncells = ncells;
      std::copy_n(clinks->Array[ptId].cells, ncells, this->Array[ptId].cells);
    }
  });
  this->MaxId = clinks->MaxId;
  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
void vtkCellLinks::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Size: " << this->Size << "\n";
  os << indent << "MaxId: " << this->MaxId << "\n";
  os << indent << "Extend: " << this->Extend << "\n";
}
VTK_ABI_NAMESPACE_END
