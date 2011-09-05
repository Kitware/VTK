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
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"

#include <vector>
#include <map>

//-----------------------------------------------------------------------------
namespace
  {
  typedef std::map<vtkIdType,vtkIdType> IdTypeMap;
  typedef std::vector<vtkIdType> IdTypeVector;
  typedef std::vector<vtkLSDynaPartCollection::LSDynaPart*> PartVector;
  }

//-----------------------------------------------------------------------------
struct vtkLSDynaPartCollection::LSDynaPart
  {
  LSDynaPart()
    {
    Grid = vtkUnstructuredGrid::New();
    NextPointId = 0;
    }
  vtkUnstructuredGrid *Grid;
  vtkIdType NextPointId;
  };

//-----------------------------------------------------------------------------
class vtkLSDynaPartCollection::LSDynaPartStorage
{
public:
  LSDynaPartStorage()
    {
    this->CellIndexToPart = new IdTypeVector[LSDynaMetaData::NUM_CELL_TYPES];
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
  IdTypeVector *CellIndexToPart; 
};

vtkStandardNewMacro(vtkLSDynaPartCollection);
//-----------------------------------------------------------------------------
vtkLSDynaPartCollection::vtkLSDynaPartCollection()
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
  if(metaData)
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
  vtkLSDynaPartCollection::LSDynaPart *part = this->Storage->Parts[matId-1];
  if (!part)
    {
    return;
    }

  //setup the cell index to part lookup table
  this->Storage->CellIndexToPart[partType][cellIndex] = matId;

  //push back the cell into the proper part grid
  part->Grid->InsertNextCell(cellType,npts,conn);
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
int vtkLSDynaPartCollection::PartIdForCellIndex(const int& partType,
                                                const int& index) const
{
  return this->Storage->CellIndexToPart[partType][index];
}

//-----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkLSDynaPartCollection::GetGridForPart(
  const int& index) const
{
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
    vtkUnstructuredGrid *grid = (*it)->Grid;
    if(grid)
      {
      grid->Delete();
      grid = NULL;
      }
    delete (*it);
    (*it)=NULL;
    }
  this->Storage->Parts.clear();
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::BuildPartInfo()
{
  //fill the vector of parts up, if the part is active
  //construct a Part at that index otherwise leave it empty

  //reserve enough space for cell index to part
  for(int i=0; i < LSDynaMetaData::NUM_CELL_TYPES;++i)
    {
    this->Storage->CellIndexToPart[i].resize(this->MetaData->NumberOfCells[i],-1);
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
void vtkLSDynaPartCollection::Finalize(vtkPoints *commonPoints)
{
  PartVector::iterator partIt;
  for (partIt = this->Storage->Parts.begin();
       partIt != this->Storage->Parts.end();
       ++partIt)
    {
    if ( (*partIt))
      {
      //create the point set for this grid
      this->ConstructPointSet(*partIt,commonPoints);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::ConstructPointSet(LSDynaPart *part,
                                vtkPoints *commonPoints)
{
  vtkPoints *points = vtkPoints::New();

  //take the cell array and find all the unique points
  //once that is done convert them
  IdTypeVector lookup;
  lookup.resize(this->MetaData->NumberOfNodes,-1);
  
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

/*
//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::ConstructPointSet(LSDynaPart *part,
                                vtkPoints *commonPoints)
{
  vtkPoints *points = vtkPoints::New();

  //take the cell array and find all the unique points
  //once that is done convert them
  IdTypeVector lookup;
  lookup.resize(this->MetaData->NumberOfNodes,0);


  //if this becomes a bottleneck we need to use a vector lookup
  std::pair<IdTypeMap::iterator,bool> ret;
  for(vtkIdType i=0; i < npts; ++i)
    {
    ret = part->PointIdLookup.insert(
      std::pair<vtkIdType,vtkIdType>(conn[i],part->NextPointId));
    //replace the cell id with the updated id.
    conn[i] = ret.first->second;
    if (ret.second)
      {
      //if we actually added this point to the map update the nextpointid
      ++part->NextPointId;
      }
    }



  points->SetNumberOfPoints(part->PointIdLookup.size());

  //take the point id mapping from this part and construct a pointset
  //with it
  double pos[3];
  IdTypeMap::const_iterator it;
  for(it=part->PointIdLookup.begin();it!=part->PointIdLookup.end();++it)
    {
    commonPoints->GetPoint(it->first,pos);
    points->SetPoint(it->second,pos);
    }
    
  part->Grid->SetPoints(points);
  points->FastDelete();
}
*/
