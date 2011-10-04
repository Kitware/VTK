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
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <vector>
#include <map>
#include <list>

//-----------------------------------------------------------------------------
namespace
  {
  static const char* TypeNames[] = {
    "PARTICLE",
    "BEAM",
    "SHELL",
    "THICK_SHELL",
    "SOLID",
    "RIGID_BODY",
    "ROAD_SURFACE",
    NULL};

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

  typedef std::vector<bool> BitVector;
  typedef std::vector<vtkIdType> IdTypeVector;
  typedef std::vector<vtkLSDynaPartCollection::LSDynaPart*> PartVector;
  typedef std::vector<vtkDataArray*> DataArrayVector;

  }

//-----------------------------------------------------------------------------
class vtkLSDynaPartCollection::LSDynaPart
  {
  public:
  LSDynaPart(LSDynaMetaData::LSDYNA_TYPES t, std::string n):Type(t),Name(n)
    {
    this->Grid = NULL;
    this->InitGrid();
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

  void InitGrid()
    {
    if(this->Grid != NULL)
      {
      this->Grid->Delete();
      }
    //currently construcutGridwithoutdeadcells calls insertnextcell
    this->Grid = vtkUnstructuredGrid::New();


    //now add in the field data to the grid. Data is the name and type
    vtkFieldData *fd = this->Grid->GetFieldData();

    vtkStringArray *partName = vtkStringArray::New();
    partName->SetName("Name");
    partName->SetNumberOfValues(1);
    partName->SetValue(0,this->Name);
    fd->AddArray(partName);
    partName->FastDelete();

    vtkStringArray *partType = vtkStringArray::New();
    partType->SetName("Type");
    partType->SetNumberOfValues(1);
    partType->SetValue(0,TypeNames[this->Type]);
    fd->AddArray(partType);
    partType->Delete();
    }

  void ResetTimeStepInfo()
    {
    CellPropertyInfo.clear();
    }
  
  //Storage of information to build the grid before we call finalize
  //these are constant across all timesteps
  IdTypeMap PointIds; //maps local point id to global point id
  vtkIdType NextPointId;

  //These need to be cleared every time step
  CPIVector CellPropertyInfo;
  //Used to hold the grid representation of this part.
  //Only is valid afer finalize has been called on a timestep
  vtkUnstructuredGrid* Grid;
  

  //Information of the part type
  const LSDynaMetaData::LSDYNA_TYPES Type;
  const std::string Name;
  };

//-----------------------------------------------------------------------------
class vtkLSDynaPartCollection::LSDynaPartStorage
{
public:
  LSDynaPartStorage(const int& size )
    {
    this->CellIndexToPart = new CTPCVector[size];
    this->DeadCells = new BitVector[size];
    }
  ~LSDynaPartStorage()
    {
    delete[] this->CellIndexToPart;
    delete[] this->DeadCells;
    }

  //Stores the information needed to construct an unstructured grid of the part
  PartVector Parts;

  //maps cell indexes which are tracked by output type to the part
  //Since cells are ordered the same between the cell connectivity data block
  //and the state block in the d3plot format we only need to know which part
  //the cell is part of.
  //This info is constant for each time step so it can't be cleared.
  CTPCVector *CellIndexToPart; 

  BitVector *DeadCells;

  //Stores all the point properties for all the parts.
  //When we finalize each part we will split these property arrays up
  DataArrayVector PointProperties;
};

vtkStandardNewMacro(vtkLSDynaPartCollection);
//-----------------------------------------------------------------------------
vtkLSDynaPartCollection::vtkLSDynaPartCollection()
{
  this->MetaData = NULL;
  this->Storage = NULL;
  this->MinIds = NULL;
  this->MaxIds = NULL;
}

//-----------------------------------------------------------------------------
vtkLSDynaPartCollection::~vtkLSDynaPartCollection()
{
  PartVector::iterator it;
  for(it=this->Storage->Parts.begin();
      it!=this->Storage->Parts.end();
      ++it)
    {
    if(*it)
      {
      (*it)->ResetTimeStepInfo();
      delete (*it);
      (*it)=NULL;
      }
    }
  if(this->Storage)
    {
    this->Storage->Parts.clear();
    delete this->Storage;
    }

  if(this->MinIds)
    {
    delete[] this->MinIds;
    }
  if(this->MaxIds)
    {
    delete[] this->MaxIds;
    }
  this->MetaData = NULL;
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::PrintSelf(ostream &os, vtkIndent indent)
{

}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::InitCollection(LSDynaMetaData *metaData,
  vtkIdType* mins, vtkIdType* maxs)
{
  if(this->Storage)
    {
    this->Storage->Parts.clear();
    delete this->Storage;
    }

  if(this->MinIds)
    {
    delete[] this->MinIds;
    }
  if(this->MaxIds)
    {
    delete[] this->MaxIds;
    }

  this->Storage = new LSDynaPartStorage(LSDynaMetaData::NUM_CELL_TYPES);
  this->MinIds = new vtkIdType[LSDynaMetaData::NUM_CELL_TYPES];
  this->MaxIds = new vtkIdType[LSDynaMetaData::NUM_CELL_TYPES];

  //reserve enough space for cell index to part.  We only
  //have to map the cell ids between min and max, so we
  //skip into the proper place
  cellToPartCell t(-1,-1);
  for(int i=0; i < LSDynaMetaData::NUM_CELL_TYPES;++i)
    {
    this->MinIds[i]= (mins!=NULL) ? mins[i] : 0;
    this->MaxIds[i]= (maxs!=NULL) ? maxs[i] : metaData->NumberOfCells[i];
    const vtkIdType reservedSpaceSize(this->MaxIds[i]-this->MinIds[i]);
    this->Storage->CellIndexToPart[i].resize(reservedSpaceSize,t);
    this->Storage->DeadCells[i].resize(reservedSpaceSize,false);
    }

  if(metaData)
    {
    this->MetaData = metaData;
    this->BuildPartInfo(mins,maxs);
    }
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::BuildPartInfo(vtkIdType* mins, vtkIdType* maxs)
{
  //reserve enough space for the grids. Each node
  //will have a part allocated, since we don't know yet
  //how the cells map to parts.
  size_t size = this->MetaData->PartIds.size();
  this->Storage->Parts.resize(size,NULL);

  //we iterate on part materials as those are those are from 1 to num Parts.
  //the part ids are the user part numbers
  std::vector<int>::const_iterator partMIt;
  std::vector<int>::const_iterator statusIt = this->MetaData->PartStatus.begin();
  std::vector<LSDynaMetaData::LSDYNA_TYPES>::const_iterator typeIt = this->MetaData->PartTypes.begin();
  std::vector<std::string>::const_iterator nameIt = this->MetaData->PartNames.begin();

  for (partMIt = this->MetaData->PartMaterials.begin();
       partMIt != this->MetaData->PartMaterials.end();
       ++partMIt,++statusIt,++typeIt,++nameIt)
    {
    if (*statusIt)
      {
      //make the index contain a part
      this->Storage->Parts[*partMIt-1] =
      new vtkLSDynaPartCollection::LSDynaPart(*typeIt,*nameIt);
      }
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

  if(!this->Storage->DeadCells[partType][cellIndex])
    {
    vtkIdType pos = part->Grid->InsertNextCell(cellType,npts,conn);
    this->Storage->CellIndexToPart[partType][cellIndex] =
      cellToPartCell(matId-1,pos);
    }
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

  //The array that passed in from the reader only contains the subset
  //of the full data that we are interested in so we don't have to adjust
  //any indices
  vtkIdType size = death->GetNumberOfTuples();
  bool deleted = false;
  for(vtkIdType i=0;i<size;++i)
    {
    deleted = (death->GetValue(i)==1);
    this->Storage->DeadCells[partType][i]=deleted;
    }
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::AddPointArray(vtkDataArray* data)
{
  this->Storage->PointProperties.push_back(data);
  data->Register(NULL);
}

//-----------------------------------------------------------------------------
int vtkLSDynaPartCollection::GetNumberOfPointArrays() const
{
  return static_cast<int>(this->Storage->PointProperties.size());
}

//-----------------------------------------------------------------------------
vtkDataArray* vtkLSDynaPartCollection::GetPointArray(const int& index) const
{
  if ( index < 0 || index >= this->GetNumberOfPointArrays())
    {
    return NULL;
    }
  return this->Storage->PointProperties[index];
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
      numTuples = part->Grid->GetNumberOfCells();
      cellPropertyInfo* t = new cellPropertyInfo(name,offset,numTuples,numComps,
        this->MetaData->Fam.GetWordSize());
      part->CellPropertyInfo.push_back(t);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::FillCellProperties(float *buffer,
  const LSDynaMetaData::LSDYNA_TYPES& type, const vtkIdType& startId,
  const vtkIdType& numCells, const int& numTuples)
{
  this->FillCellArray(buffer,type,startId,numCells,numTuples);
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::FillCellProperties(double *buffer,
  const LSDynaMetaData::LSDYNA_TYPES& type, const vtkIdType& startId,
  const vtkIdType& numCells, const int& numTuples)
{
  this->FillCellArray(buffer,type,startId,numCells,numTuples);
}

//-----------------------------------------------------------------------------
template<typename T>
void vtkLSDynaPartCollection::FillCellArray(T *buffer,
  const LSDynaMetaData::LSDYNA_TYPES& type, const vtkIdType& startId,
  const vtkIdType& numCells, const int& numTuples)
{
  //we only need to iterate the array for the subsection we need
  if(this->Storage->CellIndexToPart[type].size() == 0)
    {
    return;
    }

  for(vtkIdType i=0;i<numCells;++i)
    {
    cellToPartCell pc = this->Storage->CellIndexToPart[type][startId+i];
    if(pc.part > -1)
      {
      //read the next chunk from the buffer
      T* tuple = &buffer[i*numTuples];      

      //take that chunk and move it to the properties that are active
      vtkLSDynaPartCollection::LSDynaPart* part = this->Storage->Parts[pc.part];
      if(!part)
        {
        continue;
        }
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
  return this->Storage->Parts[index]->Grid;
}

//-----------------------------------------------------------------------------
int vtkLSDynaPartCollection::GetNumberOfParts() const
{
  return static_cast<int>(this->Storage->Parts.size());
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::GetPartReadInfo(const int& partType,
  vtkIdType& numberOfCells, vtkIdType& numCellsToSkipStart,
  vtkIdType& numCellsToSkipEnd) const
{
  if(this->Storage->CellIndexToPart[partType].size() == 0)
    {
    numberOfCells = 0;
    //skip everything
    numCellsToSkipStart = this->MetaData->NumberOfCells[partType];
    numCellsToSkipEnd = 0; //no reason to skip anything else
    }
  else
    {
    numberOfCells = this->Storage->CellIndexToPart[partType].size();
    numCellsToSkipStart = this->MinIds[partType];
    numCellsToSkipEnd = this->MetaData->NumberOfCells[partType] -
                                        (numberOfCells+numCellsToSkipStart);
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

  //Note the cell ids are fortran ordered so we are going to change
  //them to C ordered at the same time

  std::list< std::pair<vtkIdType,vtkIdType> > inputToMap;
  IdTypeVector lookup;
  lookup.resize(this->MetaData->NumberOfNodes,-1);

  PartVector::iterator partIt;

  //make sure to only build topology info the parts we are loading
  vtkIdType index = 0;
  for (partIt = this->Storage->Parts.begin();
       partIt != this->Storage->Parts.end();
       ++partIt,++index)
    {
    if((*partIt) && (*partIt)->Grid->GetNumberOfCells() == 0)
      {
      //this part wasn't valid given the cells we read in so we have to remove it
      //this is really only happens when running in parallel and if a node
      //is reading all the cells for a part, all other nodes will than delete that part
      delete (*partIt);
      (*partIt)=NULL;
      }
    else if (*partIt)
      {
      inputToMap.clear();

      //take the cell array and find all the unique points
      //once that is done convert them into a map
      
      vtkIdType nextPointId=0;
      vtkIdType npts,*pts;

      vtkCellArray *cells = (*partIt)->Grid->GetCells();
      cells->InitTraversal();
      while(cells->GetNextCell(npts,pts))
        {
        for(vtkIdType i=0;i<npts;++i)
          {
          const vtkIdType Cid(pts[i]-1);
          if(lookup[Cid] == -1)
            {
            //update the lookup table
            std::pair<vtkIdType,vtkIdType> pair(nextPointId,Cid);
            lookup[Cid] = nextPointId;
            ++nextPointId;
            inputToMap.push_back(pair);
            }
          pts[i] = lookup[Cid];
          }
        }
      //create the mapping from new ids to old ids for the points
      //this constructor will be linear time since the list is already sorted
      (*partIt)->PointIds = IdTypeMap(inputToMap.begin(),inputToMap.end());

      //reset the lookup table
      std::fill(lookup.begin(),lookup.end(),-1);

      //remove any unused allocations
      (*partIt)->Grid->Squeeze();
      }
    }
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::Finalize(vtkPoints *commonPoints, vtkPoints *roadPoints)
{
  PartVector::iterator partIt;
  for (partIt = this->Storage->Parts.begin();
       partIt != this->Storage->Parts.end();
       ++partIt)
    {
    if ( (*partIt))
      {
      this->ConstructGridCells(*partIt);
      //now construct the points for the grid
      if ((*partIt)->Type != LSDynaMetaData::ROAD_SURFACE)
        {
        this->ConstructGridPoints(*partIt,commonPoints);
        }
      else
        {
        this->ConstructGridPoints(*partIt,roadPoints);
        }
      }
    }

  this->ResetTimeStepInfo();
}


//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::ConstructGridCells(LSDynaPart *part)
{
  //now copy the cell data into the part and delete the gird data
  //the delete is here instead of reset time step so that we can
  //call fast delete instead of delete
  vtkCellData *gridData = part->Grid->GetCellData();
  CPIVector::iterator it;
  for(it=part->CellPropertyInfo.begin();
      it!=part->CellPropertyInfo.end();
      ++it)
      {
      gridData->AddArray((*it)->Data);
      (*it)->Data->FastDelete();
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

    part->Grid->GetPointData()->AddArray((*newArrayIt));
    (*newArrayIt)->FastDelete();
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
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::ResetTimeStepInfo()
{
  PartVector::iterator it;
  for(it=this->Storage->Parts.begin();
      it!=this->Storage->Parts.end();
      ++it)
    {
    if(*it)
      {
      (*it)->ResetTimeStepInfo();
      }
    }

  //delete all the point properties in the global form
  DataArrayVector::iterator doIt;
  for(doIt=this->Storage->PointProperties.begin();
    doIt!=this->Storage->PointProperties.end();
    ++doIt)
    {
    vtkDataArray* da = vtkDataArray::SafeDownCast(*doIt);
    da->Delete();
    }
  this->Storage->PointProperties.clear();
}
