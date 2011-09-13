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
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <vector>
#include <map>
#include <list>

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

  class cellPropertyInfo
    {
    public:
      cellPropertyInfo(const char* n, const int& sp,
        const vtkIdType &numTuples, const vtkIdType& numComps,
        const int& dataSize):
      StartPos(sp),
      Id(0)
        {
        Data = (dataSize == 4) ? (vtkDataArray*) vtkFloatArray::New() : 
                               (vtkDataArray*) vtkDoubleArray::New();
        Data->SetNumberOfComponents(numComps);
        Data->SetNumberOfTuples(numTuples);
        Data->SetName(n);
        }
      ~cellPropertyInfo()
        {
        Data->Delete();
        }
    int StartPos;
    vtkIdType Id; //Id of the tuple to set next
    vtkDataArray* Data;
    };

  typedef std::map<vtkIdType,vtkIdType> IdTypeMap;

  typedef std::vector<cellToPartCell> CTPCVector;
  typedef std::vector<cellPropertyInfo*> CPIVector;

  typedef std::vector<int> IntVector;
  typedef std::vector<unsigned char> UCharVector;
  typedef std::vector<vtkIdType> IdTypeVector;
  typedef std::vector<vtkLSDynaPartCollection::LSDynaPart*> PartVector;
  typedef std::vector<vtkDataArray*> DataArrayVector;
  }

//-----------------------------------------------------------------------------
struct vtkLSDynaPartCollection::LSDynaPart
  {
  LSDynaPart(LSDynaMetaData::LSDYNA_TYPES t):Type(t)
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
  void ResetTimeStepInfo()
    {
    DeadCells.clear();
    CPIVector::iterator it;
    for(it=CellPropertyInfo.begin();
        it!=CellPropertyInfo.end();
        ++it)
        {
        (*it)->Data->Delete();
        }
    CellPropertyInfo.clear();
    }
  
  //temporary storage of information to build the grid before we call finalize
  //these are constant across all timesteps
  UCharVector CellTypes;
  IdTypeVector CellLocation;
  IdTypeVector CellStructure;
  IdTypeMap PointIds; //maps local point id to global point id
  vtkIdType NextPointId;

  //These need to be cleared every time step
  IntVector DeadCells;
  CPIVector CellPropertyInfo;

  //these are handled by finalize to determine the proper lifespan
  
  //Used to hold the grid representation of this part.
  //Only is valid afer finalize has been called on a timestep
  vtkUnstructuredGrid* Grid;
  

  //Information of the part type
  const LSDynaMetaData::LSDYNA_TYPES Type;
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

  //Stores the information needed to construct an unstructured grid of the part
  PartVector Parts;

  //maps cell indexes which are tracked by output type to the part
  //Since cells are ordered the same between the cell connectivity data block
  //and the state block in the d3plot format we only need to know which part
  //the cell is part of.
  //This info is constant for each time step so it can't be cleared.
  CTPCVector *CellIndexToPart; 


  //Stores all the point properties for all the parts.
  //When we finalize each part we will split these property arrays up
  DataArrayVector PointProperties;
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
  PartVector::iterator it;
  for(it=this->Storage->Parts.begin();
      it!=this->Storage->Parts.end();
      ++it)
    {
    (*it)->ResetTimeStepInfo();
    delete (*it);
    (*it)=NULL;
    }
  this->Storage->Parts.clear();

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
    //LSDyna usin Fortran indexes (starts at 1)
    part->CellStructure.push_back(conn[i]-1);
    }

   //setup the cell index to part lookup table
  this->Storage->CellIndexToPart[partType][cellIndex] =
    cellToPartCell(matId-1,part->CellTypes.size()-1); 

}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::SetCellDeadFlags(
                                      const int& partType, vtkIntArray *death)
{
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
void vtkLSDynaPartCollection::AddPointArray(vtkDataArray* data)
{
  this->Storage->PointProperties.push_back(data);
  data->Register(this); //we up the ref count
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::AddProperty(
                    const LSDynaMetaData::LSDYNA_TYPES& type, const char* name,
                    const int& offset, const int& numComps)
{
  vtkIdType numTuples=0;
  PartVector::iterator partIt;
  for (partIt = this->Storage->Parts.begin();
       partIt != this->Storage->Parts.end();
       ++partIt)
    {
    vtkLSDynaPartCollection::LSDynaPart* part = *partIt;    
    if (part && part->Type == type)
      {
      numTuples = part->CellTypes.size();
      cellPropertyInfo* t = new cellPropertyInfo(name,offset,numTuples,numComps,
        this->MetaData->Fam.GetWordSize());
      part->CellPropertyInfo.push_back(t);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::ReadProperties(
                const LSDynaMetaData::LSDYNA_TYPES& type, const int& numTuples)
{
  const vtkIdType numCells(this->MetaData->NumberOfCells[type]);
  this->MetaData->Fam.BufferChunk(LSDynaFamily::Float, numCells * numTuples);

  if(this->MetaData->Fam.GetWordSize() == 4)
    {
    float *fbuf = this->MetaData->Fam.GetBufferAsFloat();
    this->FillPropertyArray(fbuf,type,numCells,numTuples);
    }
  else
    {
    double *dbuf = this->MetaData->Fam.GetBufferAsDouble();
    this->FillPropertyArray(dbuf,type,numCells,numTuples);
    }
}

//-----------------------------------------------------------------------------
template<typename T>
void vtkLSDynaPartCollection::FillPropertyArray(T *buffer,const LSDynaMetaData::LSDYNA_TYPES& type, const vtkIdType& numCells, const int& numTuples)
{
  for(vtkIdType i=0;i<numCells;++i)
    {
    cellToPartCell pc = this->Storage->CellIndexToPart[type][i];
    if(pc.part > -1) 
      {
      //read the next chunk from the buffer
      T* tuple = &buffer[i*numTuples];      

      //take that chunk and move it to the properties that are active
      vtkLSDynaPartCollection::LSDynaPart* part = this->Storage->Parts[pc.part];
      CPIVector::iterator it;
      for(it=part->CellPropertyInfo.begin();
        it!=part->CellPropertyInfo.end();
        ++it)
        {
        //set the next tuple in this property.
        //start pos is the offset in the data
        (*it)->Data->SetTuple((*it)->Id++,tuple + (*it)->StartPos);
        }

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
  std::vector<int>::const_iterator statusIt = this->MetaData->PartStatus.begin();
  std::vector<LSDynaMetaData::LSDYNA_TYPES>::const_iterator typeIt = this->MetaData->PartTypes.begin();
  for (partIt = this->MetaData->PartIds.begin();
       partIt != this->MetaData->PartIds.end();
       ++partIt,++statusIt,++typeIt)
    {
    if (*statusIt)
      {
      //make the index contain a part
      this->Storage->Parts[*partIt-1] =
      new vtkLSDynaPartCollection::LSDynaPart(*typeIt);
      }
    }
}


//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::FinalizeTopology()
{
  //we are going to take all the old point ids and convert them to the new
  //ids based on the point subset required for this topology.

  //If you use a map while inserting cells you get really really bad performance
  //instead we will create a lookup table of old ids to new ids. From that
  //we will create a reduced set of pairs in sorted order. those sorted pairs
  //will be used to create the map which means it the map will be constructed
  //in linear time.

  //Note the trade off here is that removing dead points will be really hard,
  //so we wont!

  //we are making the PointId map be new maps to old.

  std::list< std::pair<vtkIdType,vtkIdType> > inputToMap;
  IdTypeVector lookup;
  lookup.resize(this->MetaData->NumberOfNodes,-1);

  PartVector::iterator partIt;

  for (partIt = this->Storage->Parts.begin();
       partIt != this->Storage->Parts.end();
       ++partIt)
    {
    if (*partIt)
      {
      inputToMap.clear();

      //take the cell array and find all the unique points
      //once that is done convert them into a map
      
      vtkIdType nextPointId=0, npts=0;
      IdTypeVector::iterator csIt;
      
      for(csIt=(*partIt)->CellStructure.begin();
          csIt!=(*partIt)->CellStructure.end();)
        {
        npts = (*csIt);
        ++csIt; //move to the first point for this cell
        for(vtkIdType i=0;i<npts;++i,++csIt)
          {
          if(lookup[*csIt] == -1)
            {
            //update the lookup table
            std::pair<vtkIdType,vtkIdType> pair(nextPointId,*csIt);
            lookup[*csIt] = nextPointId;            
            ++nextPointId;
            inputToMap.push_back(pair);
            }
          *csIt = lookup[*csIt];
          }
        }

      //create the mapping from new ids to old ids for the points
      //this constructor will be linear time since the list is already sorted
      (*partIt)->PointIds = IdTypeMap(inputToMap.begin(),inputToMap.end());

      //reset the lookup table
      std::fill(lookup.begin(),lookup.end(),-1);
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
      if((*partIt)->Grid != NULL)
        {
        (*partIt)->Grid->Delete();
        }
      //currently construcutGridwithoutdeadcells calls insertnextcell
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

  this->ResetTimeStepInfo();
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
  part->Grid->SetCells(cellTypes,cellLocation,cells,NULL,NULL);

  //remove references
  cellTypes->FastDelete();
  cellLocation->FastDelete();
  cells->FastDelete();

  //now copy the cell data into the part
  vtkCellData *gridData = part->Grid->GetCellData();
  CPIVector::iterator it;
  for(it=part->CellPropertyInfo.begin();
      it!=part->CellPropertyInfo.end();
      ++it)
      {
      gridData->AddArray((*it)->Data);
      }
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
  vtkIdType numCells = static_cast<vtkIdType>(part->CellTypes.size());
  vtkIdType numDeadCells = part->DeadCells.size();

  //setup the cell properties
  CPIVector::iterator oldArrayIt;
  DataArrayVector::iterator newArrayIt;
  
  DataArrayVector newArrays;
  newArrays.resize(part->CellPropertyInfo.size());

  oldArrayIt = part->CellPropertyInfo.begin();
  vtkCellData *cd = grid->GetCellData();
  for(newArrayIt=newArrays.begin();newArrayIt!=newArrays.end();
    ++newArrayIt,++oldArrayIt)
    {
    vtkDataArray *d = (*oldArrayIt)->Data;
    (*newArrayIt)=d->NewInstance();
    (*newArrayIt)->SetName(d->GetName());
    (*newArrayIt)->SetNumberOfComponents(d->GetNumberOfComponents());
    (*newArrayIt)->SetNumberOfTuples(numCells-numDeadCells);
    cd->AddArray(*newArrayIt);
    (*newArrayIt)->FastDelete();
    }
 
  //this has a totally different method since we can't use the clean implementation
  //of copying the memory right from the vectors. Instead we have to skip
  //the chunks that have been deleted. For te cell location and cell types this
  //is fairly easy for the cell structure it is a bit more complicated

  //needed infos  
  vtkIdType currentDeadCellPos=0;
  UCharVector::iterator typeIt = part->CellTypes.begin();
  IdTypeVector::iterator locIt = part->CellLocation.begin();
  vtkIdType i=0,idx=0;
  for(; i<numCells && currentDeadCellPos<numDeadCells ;++i,++typeIt,++locIt)
    {
    if(part->DeadCells[currentDeadCellPos] != i)
      {
      grid->InsertNextCell(*typeIt,part->CellStructure[*locIt],&part->CellStructure[*locIt+1]);

      oldArrayIt = part->CellPropertyInfo.begin();
      for(newArrayIt=newArrays.begin();
      newArrayIt!=newArrays.end();
      ++newArrayIt,++oldArrayIt)
        {
        (*newArrayIt)->SetTuple(idx, (*oldArrayIt)->Data->GetTuple(i));
        }
      ++idx;
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
    oldArrayIt = part->CellPropertyInfo.begin();
    for(newArrayIt=newArrays.begin();
      newArrayIt!=newArrays.end();
      ++newArrayIt,++oldArrayIt)
      {
      (*newArrayIt)->SetTuple(idx, (*oldArrayIt)->Data->GetTuple(i));
      }
    ++idx;        
    }

}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::ConstructGridPoints(LSDynaPart *part, vtkPoints *commonPoints)
{
  vtkIdType size = part->PointIds.size();

  //now compute the points for the grid
  vtkPoints *points = vtkPoints::New();
  points->SetNumberOfPoints(size);

  //create new property arrays
  DataArrayVector::iterator newArrayIt, ppArrayIt;
  DataArrayVector newArrays;
  newArrays.resize(this->Storage->PointProperties.size(),NULL);
  ppArrayIt = this->Storage->PointProperties.begin();
  for(newArrayIt=newArrays.begin();newArrayIt!=newArrays.end();
    ++newArrayIt,++ppArrayIt)
    {
    (*newArrayIt)=(*ppArrayIt)->NewInstance();
    (*newArrayIt)->SetName((*ppArrayIt)->GetName());
    (*newArrayIt)->SetNumberOfComponents((*ppArrayIt)->GetNumberOfComponents());
    (*newArrayIt)->SetNumberOfTuples(size);
    }

  //fill the points and point property classes
  IdTypeMap::const_iterator pIt;
  for(pIt=part->PointIds.begin();
      pIt!=part->PointIds.end();
      ++pIt)
    {
    //set the point
    points->SetPoint(pIt->first,commonPoints->GetPoint(pIt->second));

    //set the properties for the point
    for(newArrayIt=newArrays.begin(),ppArrayIt=this->Storage->PointProperties.begin();
            newArrayIt!=newArrays.end();
            ++newArrayIt,++ppArrayIt)
        {
        (*newArrayIt)->SetTuple(pIt->first, (*ppArrayIt)->GetTuple(pIt->second));
        }
    }

  part->Grid->SetPoints(points);
  points->FastDelete();

  for(newArrayIt=newArrays.begin();
      newArrayIt!=newArrays.end();
      ++newArrayIt)
    {
    part->Grid->GetPointData()->AddArray((*newArrayIt));
    (*newArrayIt)->FastDelete();
    }

}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::ResetTimeStepInfo()
{
  PartVector::iterator it;
  for(it=this->Storage->Parts.begin();
      it!=this->Storage->Parts.end();
      ++it)
    {
    (*it)->ResetTimeStepInfo();
    }

  //delete all the point properties in the global form
  DataArrayVector::iterator doIt;
  for(doIt=this->Storage->PointProperties.begin();
    doIt!=this->Storage->PointProperties.end();
    ++doIt)
    {
    (*doIt)->Delete();
    }
  this->Storage->PointProperties.clear();

  this->Finalized = false;
}
