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

#include "vtkCellArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <vector>
#include <map>

//-----------------------------------------------------------------------------
namespace
  {
  //stores the mapping from cell to part index and cell index
  struct cellToPartCell
    {
    cellToPartCell(vtkIdType p, vtkIdType c):part(p),cell(c){}
    vtkIdType part;
    vtkIdType cell;
    };

  typedef std::map<vtkIdType,vtkIdType> IdTypeMap;

  typedef std::vector<cellToPartCell> CTPCVector;

  typedef std::vector<int> IntVector;
  typedef std::vector<unsigned char> UCharVector;
  typedef std::vector<vtkIdType> IdTypeVector;
  typedef std::vector<vtkLSDynaPartCollection::LSDynaPart*> PartVector;
  }

//-----------------------------------------------------------------------------
struct vtkLSDynaPartCollection::LSDynaPart
  {
  LSDynaPart()
    {
    Grid = NULL;
    NextPointId = 0;
    }
  ~LSDynaPart()
    {
    if(Grid)
      {
      Grid->Delete();
      Grid=NULL;
      }
    }
  
  //Used to hold the grid representation of this part.
  //Only is valid afer finalize has been called
  vtkUnstructuredGrid* Grid;

  //temporary storage of information to build the grid before we call finalize
  UCharVector CellTypes;
  IdTypeVector CellLocation;
  IdTypeVector CellStructure;
  IntVector DeadCells;

  vtkIdType NextPointId;
  };

//-----------------------------------------------------------------------------
class vtkLSDynaPartCollection::LSDynaPartStorage
{
public:
  LSDynaPartStorage()
    {
    this->CellIndexToPart = new CTPCVector[LSDynaMetaData::NUM_CELL_TYPES];
    }
  ~LSDynaPartStorage()
    {
    delete[] this->CellIndexToPart;
    }

  PartVector Parts;
  //maps cell indexes which are tracked by output type to the part
  //Since cells are ordered the same between the cell connectivity data block
  //and the state block in the d3plot format we only need to know which part
  //the cell is part of.
  CTPCVector *CellIndexToPart; 
};

vtkStandardNewMacro(vtkLSDynaPartCollection);
//-----------------------------------------------------------------------------
vtkLSDynaPartCollection::vtkLSDynaPartCollection():
  Finalized(false)
{
  this->MetaData = NULL;
  this->Storage = new LSDynaPartStorage();
}

//-----------------------------------------------------------------------------
vtkLSDynaPartCollection::~vtkLSDynaPartCollection()
{
  this->Reset();
  delete this->Storage;
  this->MetaData = NULL;
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::PrintSelf(ostream &os, vtkIndent indent)
{

}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::SetMetaData(LSDynaMetaData *metaData)
{
  if(metaData && !this->Finalized)
    {
    this->MetaData = metaData;
    this->BuildPartInfo();
    }
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::InsertCell(const int& partType,
                                         const vtkIdType& cellIndex,
                                         const vtkIdType& matId,
                                         const int& cellType,
                                         const vtkIdType& npts,
                                         vtkIdType conn[8])
{
  if (this->Finalized)
    {
    //you cant add cells after calling finalize
    return;
    }

  vtkLSDynaPartCollection::LSDynaPart *part = this->Storage->Parts[matId-1];
  if (!part)
    {
    return;
    }

  //push back the cell into the proper part grid for storage
  part->CellTypes.push_back(static_cast<unsigned char>(cellType));  
  
  part->CellStructure.push_back(npts);
  //compute the direct postion this is needed when we finalize the data into
  //a unstructured grid. We need to find the size after we push back the npts
  part->CellLocation.push_back(
    static_cast<vtkIdType>(part->CellStructure.size()-1));
  
  //now push back the rest of the cell structure
  for(int i=0; i<npts; ++i)
    {
    part->CellStructure.push_back(conn[i]);
    }

   //setup the cell index to part lookup table
  this->Storage->CellIndexToPart[partType][cellIndex] =
    cellToPartCell(matId-1,part->CellTypes.size()-1); 

}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::SetCellDeadFlags(
                                      const int& partType, vtkIntArray *death)
{
  if (this->Finalized)
    {
    //you cant add cell info after calling finalize
    return;
    }

  //go through and flag each part cell as deleted or not.
  //this means breaking up this array into an array for each part
  if (!death)
    {
    return;
    }
  vtkIdType size = death->GetNumberOfTuples();
  int partIndex = 0, deleted = 0;
  for(vtkIdType i=0;i<size;++i)
    {
    cellToPartCell pc =  this->Storage->CellIndexToPart[partType][i];
    deleted = death->GetValue(i);
    if(deleted && pc.part > -1)
      {
      //only store the deleted cells.
      this->Storage->Parts[pc.part]->DeadCells.push_back(pc.cell);
      }
    }
}

//-----------------------------------------------------------------------------
bool vtkLSDynaPartCollection::IsActivePart(const int& id) const
{
  if (id < 0 || id >= this->Storage->Parts.size())
    {
    return false;
    }

  return this->Storage->Parts[id] != NULL;
}

//-----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkLSDynaPartCollection::GetGridForPart(
  const int& index) const
{

  if (!this->Finalized)
    {
    //you have to call finalize first
    return NULL;
    }

  return this->Storage->Parts[index]->Grid;
}

//-----------------------------------------------------------------------------
int vtkLSDynaPartCollection::GetNumberOfParts() const
{
  return static_cast<int>(this->Storage->Parts.size());
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::Reset()
{
  for(int i=0; i < LSDynaMetaData::NUM_CELL_TYPES;++i)
    {
    this->Storage->CellIndexToPart[i].clear();
    }

  PartVector::iterator it;
  for(it=this->Storage->Parts.begin();
      it!=this->Storage->Parts.end();
      ++it)
    {
    delete (*it);
    (*it)=NULL;
    }
  this->Storage->Parts.clear();
  this->Finalized = false;
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::BuildPartInfo()
{
  //fill the vector of parts up, if the part is active
  //construct a Part at that index otherwise leave it empty

  //reserve enough space for cell index to part
  for(int i=0; i < LSDynaMetaData::NUM_CELL_TYPES;++i)
    {
    cellToPartCell t(-1,-1);
    this->Storage->CellIndexToPart[i].resize(this->MetaData->NumberOfCells[i],t);
    }  
  //reserve enough space for the grids
  size_t size = this->MetaData->PartIds.size();
  this->Storage->Parts.resize(size,NULL);

  std::vector<int>::const_iterator partIt;
  std::vector<int>::const_iterator start = this->MetaData->PartIds.begin();
  for (partIt = this->MetaData->PartIds.begin();
       partIt != this->MetaData->PartIds.end();
       ++partIt)
    {
    if (this->MetaData->PartStatus[partIt - start])
      {
      //make the index contain a part
      this->Storage->Parts[partIt - start] =
        new vtkLSDynaPartCollection::LSDynaPart();
      }
    }
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::Finalize(vtkPoints *commonPoints,
  const int& removeDeletedCells)
{
  PartVector::iterator partIt;
  for (partIt = this->Storage->Parts.begin();
       partIt != this->Storage->Parts.end();
       ++partIt)
    {
    if ( (*partIt))
      {
      (*partIt)->Grid = vtkUnstructuredGrid::New();   
      if(removeDeletedCells && (*partIt)->DeadCells.size() > 0)
        {
        this->ConstructGridCellsWithoutDeadCells(*partIt);
        }
      else
        {
        this->ConstructGridCells(*partIt);
        }
      //now construct the points for the grid
      this->ConstructGridPoints(*partIt,commonPoints);
      }
    }
  this->Finalized = true;
}


//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::ConstructGridCells(LSDynaPart *part)
{  
  if(part->CellTypes.size() == 0 )
    {
    //the part is empty
    return;
    }  
  
  //needed info
  vtkIdType numCells = static_cast<vtkIdType>(part->CellTypes.size());
  vtkIdType sizeOfCellStruct = static_cast<vtkIdType>(part->CellStructure.size());

  //copy the contents from the part into a cell array.  
  vtkIdTypeArray *cellArray = vtkIdTypeArray::New();
  cellArray->SetNumberOfValues(sizeOfCellStruct);
  std::copy(part->CellStructure.begin(),part->CellStructure.end(),
    reinterpret_cast<vtkIdType*>(cellArray->GetVoidPointer(0)));

  //set the idtype aray as the cellarray
  vtkCellArray *cells = vtkCellArray::New();
  cells->SetCells(numCells,cellArray);
  cellArray->FastDelete();

  //now copy the cell types from the vector to 
  vtkUnsignedCharArray* cellTypes = vtkUnsignedCharArray::New();
  cellTypes->SetNumberOfValues(numCells);
  std::copy(part->CellTypes.begin(),part->CellTypes.end(),
     reinterpret_cast<unsigned char*>(cellTypes->GetVoidPointer(0)));

  //last is the cell locations
  vtkIdTypeArray *cellLocation = vtkIdTypeArray::New();
  cellLocation->SetNumberOfValues(numCells);
  std::copy(part->CellLocation.begin(),part->CellLocation.end(),
     reinterpret_cast<vtkIdType*>(cellLocation->GetVoidPointer(0)));

  //actually set up the grid
  part->Grid->SetCells(cellTypes,cellLocation,cells);

  //remove references
  cellTypes->FastDelete();
  cellLocation->FastDelete();
  cells->FastDelete();
  
  //clear the part storage
  part->CellLocation.clear();
  part->CellStructure.clear();
  part->CellTypes.clear();
  part->DeadCells.clear();
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::ConstructGridCellsWithoutDeadCells(LSDynaPart *part)
{
  if(part->CellTypes.size() == 0 )
    {
    //the part is empty
    return;
    }
  vtkUnstructuredGrid *grid = part->Grid;

  //this has a totally different method since we can't use the clean implementation
  //of copying the memory right from the vectors. Instead we have to skip
  //the chunks that have been deleted. For te cell location and cell types this
  //is fairly easy for the cell structure it is a bit more complicated

  //needed info
  vtkIdType numCells = static_cast<vtkIdType>(part->CellTypes.size());
  size_t numDeadCells = part->DeadCells.size();
  size_t currentDeadCellPos=0;
  UCharVector::iterator typeIt = part->CellTypes.begin();
  IdTypeVector::iterator locIt = part->CellLocation.begin();

  vtkIdType i=0;
  for(; i<numCells && currentDeadCellPos<numDeadCells ;++i,++typeIt,++locIt)
    {
    if(part->DeadCells[currentDeadCellPos] != i)
      {
      grid->InsertNextCell(*typeIt,part->CellStructure[*locIt],&part->CellStructure[*locIt+1]);
      }
    else
      {
      ++currentDeadCellPos;
      }
    }

  //we have all the dead cells tight loop the rest
  for(; i<numCells;++i,++typeIt,++locIt)
    {
    grid->InsertNextCell(*typeIt,part->CellStructure[*locIt],&part->CellStructure[*locIt+1]);
    }
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::ConstructGridPoints(LSDynaPart *part, vtkPoints *commonPoints)
{
  //now compute the points for the grid
  vtkPoints *points = vtkPoints::New();

  //take the cell array and find all the unique points
  //once that is done convert them
  IdTypeVector lookup;
  lookup.resize(this->MetaData->NumberOfNodes,-1);
  
  //Reset the NextPointId ivar
  part->NextPointId = 0;
  double pos[3];
 
  vtkIdType npts,*pts;
  vtkCellArray *cells = part->Grid->GetCells();
  cells->InitTraversal();
  while(cells->GetNextCell(npts,pts))
    {
    for(vtkIdType i=0;i<npts;++i)
      {
      const vtkIdType idx(pts[i]);
      if(lookup[idx] != -1)
        {
        pts[i] = lookup[idx];
        }
      else
        {
        //add the point to points array
        commonPoints->GetPoint(idx,pos);
        points->InsertNextPoint(pos);

        //update the cell array and lookup table
        lookup[idx] = part->NextPointId;
        pts[i] = part->NextPointId;
        ++part->NextPointId;
        }
      }
    }

  points->Squeeze();
  part->Grid->SetPoints(points);
  points->FastDelete();
}
