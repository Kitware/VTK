/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLSDynaReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "vtkLSDynaPartCollection.h"

#include "LSDynaMetaData.h"

#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"

#include <vector>

class vtkLSDynaPartCollection::LSDynaPartVector
{
public:
  LSDynaPartVector()
    {
    this->CellIndexToPart = new IdTypeVector[LSDynaMetaData::NUM_CELL_TYPES];
    }
  ~LSDynaPartVector()
    {
    delete[] this->CellIndexToPart;
    }

  typedef std::vector<vtkUnstructuredGrid*> GridVector;
  typedef std::vector<vtkIdType> IdTypeVector;
  GridVector Grids;
  IdTypeVector *CellIndexToPart;

};

vtkStandardNewMacro(vtkLSDynaPartCollection);
//------------------------------------------------------------------------------
vtkLSDynaPartCollection::vtkLSDynaPartCollection()
{
  this->MetaData = NULL;
  this->Parts = new LSDynaPartVector();
}

//------------------------------------------------------------------------------
vtkLSDynaPartCollection::~vtkLSDynaPartCollection()
{
  this->Reset();
  delete this->Parts;
  this->MetaData = NULL;
}

//------------------------------------------------------------------------------
void vtkLSDynaPartCollection::PrintSelf(ostream &os, vtkIndent indent)
{

}

//------------------------------------------------------------------------------
void vtkLSDynaPartCollection::SetMetaData(LSDynaMetaData *metaData)
{
  if(metaData)
    {
    this->MetaData = metaData;
    this->BuildPartInfo();
    }
}

//------------------------------------------------------------------------------
void vtkLSDynaPartCollection::InsertCell(const int& partType,
                                         const vtkIdType& cellIndex,
                                         const vtkIdType& matId,
                                         const int& cellType,
                                         const vtkIdType& npts,
                                         vtkIdType conn[8])
{
  if (!this->IsActivePart(matId))
    {
    return;
    }

  //setup the cell index to part lookup table
  this->Parts->CellIndexToPart[partType][cellIndex] = matId;

  //push back the cell into the proper part grid
  vtkUnstructuredGrid *part = this->Parts->Grids[matId];
  part->InsertNextCell(cellType,npts,conn);
}

//------------------------------------------------------------------------------
bool vtkLSDynaPartCollection::IsActivePart(const int& id) const
{
  if (id < 0 || id >= this->Parts->Grids.size())
    {
    return false;
    }

  //only considered a valid part if it has cells added to it.
  return this->Parts->Grids[id] != NULL && this->Parts->Grids[id]->GetNumberOfCells() > 0;
}

//------------------------------------------------------------------------------
int vtkLSDynaPartCollection::PartIdForCellIndex(const int& partType,
                                                const int& index) const
{
  return this->Parts->CellIndexToPart[partType][index];
}

//------------------------------------------------------------------------------
vtkUnstructuredGrid* vtkLSDynaPartCollection::GetGridForPart(
  const int& index) const
{
  return this->Parts->Grids[index];
}

//------------------------------------------------------------------------------
int vtkLSDynaPartCollection::GetNumberOfParts() const
{
  return this->Parts->Grids.size();
}

//------------------------------------------------------------------------------
void vtkLSDynaPartCollection::Reset()
{
  for(int i=0; i < LSDynaMetaData::NUM_CELL_TYPES;++i)
    {
    this->Parts->CellIndexToPart[i].clear();
    }

  std::vector<vtkUnstructuredGrid*>::iterator it;
  for(it=this->Parts->Grids.begin();
      it!=this->Parts->Grids.end();
      ++it)
    {
    vtkUnstructuredGrid *grid = (*it);
    if(grid)
      {
      grid->Delete();
      }
    (*it)=NULL;
    }
}

//------------------------------------------------------------------------------
void vtkLSDynaPartCollection::BuildPartInfo()
{
  //fill the vector of parts up, if the part is active
  //construct a Part at that index otherwise leave it empty

  //reserve enough space for cell index to part
  for(int i=0; i < LSDynaMetaData::NUM_CELL_TYPES;++i)
    {
    this->Parts->CellIndexToPart[i].reserve(this->MetaData->NumberOfCells[i]);
    }

  //reserve enough space for the grids
  size_t size = this->MetaData->PartIds.size();
  this->Parts->Grids.resize(size,NULL);

  std::vector<int>::const_iterator partIt;
  std::vector<int>::const_iterator start = this->MetaData->PartIds.begin();
  for (partIt = this->MetaData->PartIds.begin();
       partIt != this->MetaData->PartIds.end();
       ++partIt)
    {
    if (this->MetaData->PartStatus[partIt - start])
      {
      //make the index contain a part
      this->Parts->Grids[partIt - start] = vtkUnstructuredGrid::New();
      }
    }
}
