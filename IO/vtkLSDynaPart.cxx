/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLSDynaPart.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "vtkLSDynaPart.h"

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

typedef std::vector<bool> BitVector;

class CellProperty
  {
  public:
    template<typename T>
    CellProperty(T, const int& sp,
      const vtkIdType &numTuples, const vtkIdType& nc):
    startPos(sp),
    numComps(nc)
      {
      Data =new unsigned char[numTuples * nc * sizeof(T)];
      loc = Data;
      len = numComps * sizeof(T);
      }
    ~CellProperty()
      {
      delete[] Data;
      }
    template<typename T>
    void insertNextTuple(T* values)
      {
      memcpy(loc,values+startPos,len);
      loc = ((T*)loc) + numComps;
      }
    void resetForNextTimeStep()
      {
      loc = Data;
      }

  unsigned char *Data;
protected:
  int startPos;
  size_t len;
  vtkIdType numComps;
  void *loc;
  };
}

//-----------------------------------------------------------------------------
class vtkLSDynaPart::InternalCellProperties
{
//lightweight class that holds the cell properties

public:
  InternalCellProperties():
    GhostCells(NULL),GhostIndex(0),UserIds(NULL),UserIdIndex(0){}

  ~InternalCellProperties()
  {
  std::vector<CellProperty*>::iterator it;
  for(it=Properties.begin();it!=Properties.end();++it)
    {
    if(*it)
      {
      delete (*it);
      (*it)=NULL;
      }
    }
  this->Properties.clear();

  if(this->GhostCells)
    {
    delete[] this->GhostCells;
    }
  if(this->UserIds)
    {
    delete[] this->UserIds;
    }
  }

  bool NoGhostCells() const { return GhostCells == NULL; }
  bool NoUserIds() const { return UserIds == NULL; }

  template<typename T>
  void* AddProperty(const int& offset, const int& numTuples, const int& numComps)
  {
    CellProperty *prop = new CellProperty(T(),offset,numTuples,numComps);
    this->Properties.push_back(prop);

    //return the location to set the void pointer too
    return prop->Data;
  }

  template<typename T>
  void AddCellInfo(T* cellproperty)
  {
    std::vector<CellProperty*>::iterator it;
    for(it=Properties.begin();it!=Properties.end();++it)
      {
      (*it)->insertNextTuple(cellproperty);
      }
  }

  void SetGhostCells(unsigned char* dead, const vtkIdType& size)
  {
    memcpy(this->GhostCells+this->GhostIndex,dead,sizeof(unsigned char)*size);
    this->GhostIndex += size;
  }

  void SetNextUserId(const vtkIdType &id)
  {
    this->UserIds[this->UserIdIndex++]=id;
  }


  void SetGhostCellArray(unsigned char* gc)
  {
    this->GhostCells = gc;
    this->GhostIndex = 0;
  }

  void SetMaterialIdArray(vtkIdType* ids)
  {
    this->UserIds = ids;
    this->UserIdIndex = 0;
  }

  void ResetForNextTimeStep()
  {
    this->GhostIndex = 0;
    this->UserIdIndex = 0;
    std::vector<CellProperty*>::iterator it;
    for(it=Properties.begin();it!=Properties.end();++it)
      {
      (*it)->resetForNextTimeStep();
      }
  }

  void* GetGhostVoidPtr()
    {
    return static_cast<void*>(this->GhostCells);
    }

protected:
  std::vector<CellProperty*> Properties;

  //the two cell data arrays that aren't packed with cell state info
  unsigned char* GhostCells;
  vtkIdType* UserIds;

  vtkIdType GhostIndex;
  vtkIdType UserIdIndex;


};

//-----------------------------------------------------------------------------
class vtkLSDynaPart::InternalCells
{
//lightweight class that holds the cell topology.In buildTopology
//we will set the unstructured grid pointers to look at these vectors
public:

  size_t size() const {return types.size();}
  size_t dataSize() const {return data.size();}

  void add(const int& cellType, const vtkIdType& npts, vtkIdType conn[8])
  {
    types.push_back(static_cast<unsigned char>(cellType));

    data.push_back(npts); //add in the num of points
    locations.push_back(static_cast<vtkIdType>(data.size()-1));
    data.insert(data.end(),conn,conn+npts);
  }

  void reserve(const vtkIdType& numCells, const vtkIdType& dataLen)
  {
    types.reserve(numCells);
    locations.reserve(numCells);
    //data len only holds total number of points across the cells
    data.reserve(numCells+dataLen);
  }

  std::vector<unsigned char> types;
  std::vector<vtkIdType> locations;
  std::vector<vtkIdType> data;
};

//-----------------------------------------------------------------------------
class vtkLSDynaPart::InternalPointsUsed
{
//Base class that tracks which points this part uses
public:
  //uses the relative index based on the minId
  InternalPointsUsed(const vtkIdType& minId,
                     const vtkIdType& maxId):
    MinId(minId),MaxId(maxId+1){} //maxId is meant to be exclusive

  virtual bool isUsed(const vtkIdType &index) const = 0;

  //the min and max id allow the parts to be sorted in the collection
  //based on the points they need to allow for subsections of the global point
  //array to be sent to only  parts that use it
  vtkIdType minId() const { return MinId; }
  vtkIdType maxId() const { return MaxId; }
protected:
  vtkIdType MinId;
  vtkIdType MaxId;
};

//-----------------------------------------------------------------------------
class vtkLSDynaPart::DensePointsUsed : public vtkLSDynaPart::InternalPointsUsed
{
  //uses a min and max id to bound the bit vector of points that this part
  //uses. If the points for the part are all bunched up in the global point
  //space this is used as it saves tons of space.
public:
  DensePointsUsed(BitVector *pointsUsed, const vtkIdType& minId,
                  const vtkIdType& maxId):
    InternalPointsUsed(minId,maxId)
  {
    //remember that max is a point we need to include
    this->UsedPoints.resize(this->MaxId-this->MinId,false);
    vtkIdType idx=0;
    for(vtkIdType i=this->MinId; i<this->MaxId; ++i)
      {
      //we need relative ids
      this->UsedPoints[idx++]=(*pointsUsed)[i];
      }
  }


  bool isUsed(const vtkIdType &index) const {return UsedPoints[index];}

protected:
  BitVector UsedPoints;
};

//-----------------------------------------------------------------------------
class vtkLSDynaPart::SparsePointsUsed : public vtkLSDynaPart::InternalPointsUsed
{
  //uses a set to store highly unrelated points. I doubt this is used by
  //many parts as the part would need to use a few points whose indicies was
  //at the extremes of the global point set
public:
  SparsePointsUsed(BitVector *pointsUsed, const vtkIdType& minId,
                   const vtkIdType& maxId):
    InternalPointsUsed(minId,maxId)
    {
    for(vtkIdType i=this->MinId; i<this->MaxId; ++i)
      {
      //we need relative ids
      if((*pointsUsed)[i])
        {
        this->UsedPoints.insert(i-this->MinId);
        }
      }
    }
  bool isUsed(const vtkIdType &index) const
    {
    return this->UsedPoints.find(index) != this->UsedPoints.end();
    }
protected:
  std::set<vtkIdType> UsedPoints;
};

//-----------------------------------------------------------------------------
class vtkLSDynaPart::InternalCurrentPointInfo
{
  public:
  InternalCurrentPointInfo():ptr(NULL),index(0){}
  void *ptr;
  vtkIdType index;
};

vtkStandardNewMacro(vtkLSDynaPart);

//-----------------------------------------------------------------------------
vtkLSDynaPart::vtkLSDynaPart()
{
  this->Cells = new vtkLSDynaPart::InternalCells();
  this->CellProperties = new vtkLSDynaPart::InternalCellProperties();
  this->CurrentPointPropInfo = new vtkLSDynaPart::InternalCurrentPointInfo();
  this->GlobalPointsUsed = NULL;

  this->Type = LSDynaMetaData::NUM_CELL_TYPES;
  this->Name = vtkStdString();
  this->UserMaterialId = -1;
  this->PartId = -1;

  this->NumberOfCells = -1;
  this->NumberOfPoints = -1;


  this->TopologyBuilt = false;
  this->DoubleBased = true;

  this->Grid = NULL;
  this->Points = NULL;
}

//-----------------------------------------------------------------------------
vtkLSDynaPart::~vtkLSDynaPart()
{
  delete this->Cells;
  delete this->CellProperties;
  delete this->CurrentPointPropInfo;

  if(Grid)
    {
    Grid->Delete();
    Grid=NULL;
    }
  if(Points)
    {
    Points->Delete();
    Points=NULL;
    }
  if(this->GlobalPointsUsed)
    {
    delete this->GlobalPointsUsed;
    }
}

//-----------------------------------------------------------------------------
void vtkLSDynaPart::PrintSelf(ostream &os, vtkIndent indent)
{
  os << indent << "Type " << this->Type << endl;
  os << indent << "Name " << this->Name << endl;
  os << indent << "UserMaterialId " << this->UserMaterialId << endl;
  os << indent << "Number of Cells " << this->NumberOfCells << endl;
  os << indent << "Number of Points " << this->NumberOfPoints << endl;
  os << indent << "TopologyBuilt"  << this->TopologyBuilt << endl;
}

//-----------------------------------------------------------------------------
bool vtkLSDynaPart::HasCells() const
{
  return this->Cells->size() > 0;
}


//-----------------------------------------------------------------------------
void vtkLSDynaPart::InitPart(LSDynaMetaData::LSDYNA_TYPES t, vtkStdString name,
                             const vtkIdType& partId,
                             const vtkIdType& userMatId,
                             const vtkIdType& numGlobalPoints,
                             const int& sizeOfWord)
{
  this->Type = t;
  this->Name = name;
  this->PartId = partId;
  this->UserMaterialId = userMatId;
  this->DoubleBased = (sizeOfWord == 8);
  this->NumberOfGlobalPoints = numGlobalPoints;

  this->GlobalPointsUsed = NULL;

  this->Grid = vtkUnstructuredGrid::New();
  this->Points = vtkPoints::New();

  this->Grid->SetPoints(this->Points);

  //now add in the field data to the grid.
  //Data is the name, type, and material id
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
  partType->FastDelete();

  vtkIntArray *materialId = vtkIntArray::New();
  materialId->SetName("Material Id");
  materialId->SetNumberOfValues(1);
  materialId->SetValue(0,this->UserMaterialId);
  fd->AddArray(materialId);
  materialId->FastDelete();
}

//-----------------------------------------------------------------------------
void vtkLSDynaPart::AllocateCellMemory(const vtkIdType& numCells,
                                       const vtkIdType& cellLen)
{
  this->Cells->reserve(numCells,cellLen);
}

//-----------------------------------------------------------------------------
void vtkLSDynaPart::AddCell(const int& cellType, const vtkIdType& npts, vtkIdType conn[8])
{
  this->Cells->add(cellType,npts,conn);
}

//-----------------------------------------------------------------------------
void vtkLSDynaPart::BuildToplogy()
{
  //make the unstrucuted grid data point to the Cells memory
  this->BuildCells();

  //determine the number of points that this part has
  //and what points those are in the global point map
  //fixup the cell topology to use the local parts point ids
  this->BuildUniquePoints();

  this->TopologyBuilt = true;
}

//-----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkLSDynaPart::GenerateGrid()
{
  this->CellProperties->ResetForNextTimeStep();

  //we have to mark all the properties as modified so the information
  //tab will be at the correct values
  vtkCellData* cd = this->Grid->GetCellData();
  int numArrays = cd->GetNumberOfArrays();
  for(int i=0; i<numArrays; ++i)
    {
    cd->GetArray(i)->Modified();
    }

  this->Points->Modified();

  vtkPointData *pd = this->Grid->GetPointData();
  numArrays = pd->GetNumberOfArrays();
  for(int i=0; i<numArrays; ++i)
    {
    pd->GetArray(i)->Modified();
    }

  return this->Grid;
}

//-----------------------------------------------------------------------------
void vtkLSDynaPart::EnableDeadCells()
{
  if(this->CellProperties->NoGhostCells())
    {
    //we are using the ghost levels to hide cells that have been
    //classified as dead, rather than the intended purpose
    unsigned char* ghost = new unsigned char[this->NumberOfCells];

    //the cell properties will delete the ghost array when needed
    this->CellProperties->SetGhostCellArray(ghost);
    }

  if(!this->Grid->GetCellData()->HasArray("vtkGhostLevels"))
    {
    vtkUnsignedCharArray *ghostCells = vtkUnsignedCharArray::New();
    ghostCells->SetName("vtkGhostLevels");
    ghostCells->SetVoidArray(this->CellProperties->GetGhostVoidPtr(),
                             this->NumberOfCells,1);

    this->Grid->GetCellData()->AddArray(ghostCells);
    ghostCells->FastDelete();  
    }
}

//-----------------------------------------------------------------------------
void vtkLSDynaPart::DisableDeadCells()
{
  if(this->Grid->GetCellData()->HasArray("vtkGhostLevels"))
    {
    this->Grid->GetCellData()->RemoveArray("vtkGhostLevels");
    }
}

//-----------------------------------------------------------------------------
void vtkLSDynaPart::SetCellsDeadState(unsigned char *dead,const vtkIdType &size)
{
  //presumes the HideDeletedCells is true, doesn't check for speed
  this->CellProperties->SetGhostCells(dead,size);
}

//-----------------------------------------------------------------------------
void vtkLSDynaPart::EnableCellUserIds()
{
  if(this->CellProperties->NoUserIds())
    {
    vtkIdType *ids = new vtkIdType[this->NumberOfCells];

    //the cell properties will delete the ghost array when needed
    this->CellProperties->SetMaterialIdArray(ids);

    vtkIdTypeArray *userIds = vtkIdTypeArray::New();
    userIds->SetName("UserIds");
    userIds->SetVoidArray(ids,this->NumberOfCells,1);
    this->Grid->GetCellData()->AddArray(userIds);
    userIds->FastDelete();
    }
}

//-----------------------------------------------------------------------------
void vtkLSDynaPart::SetNextCellUserIds(const vtkIdType& value)
{
  this->CellProperties->SetNextUserId(value);
}

//-----------------------------------------------------------------------------
void vtkLSDynaPart::AddPointProperty(const char* name, 
        const vtkIdType& numComps, const bool &isIdTypeProperty,
        const bool &isProperty, const bool& isGeometryPoints)
{
  //adding a point property means that this is the next property
  //we are going to be reading from file

  //first step is getting the ptr to the start of the right property
  this->GetPropertyData(name,numComps,isIdTypeProperty,isProperty,
                        isGeometryPoints);
  this->CurrentPointPropInfo->index = 0;
}

//-----------------------------------------------------------------------------
void vtkLSDynaPart::ReadPointBasedProperty(float *data, const vtkIdType& numTuples,
                                      const vtkIdType& numComps,
                                      const vtkIdType& currentGlobalPointIndex)
{
  float *ptr = static_cast<float*>(this->CurrentPointPropInfo->ptr);
  this->AddPointInformation(data,ptr,numTuples,
                            numComps,currentGlobalPointIndex);
}

//-----------------------------------------------------------------------------
void vtkLSDynaPart::ReadPointBasedProperty(double *data, const vtkIdType& numTuples,
                            const vtkIdType& numComps,
                            const vtkIdType& currentGlobalPointIndex)
{
  double *ptr = static_cast<double*>(this->CurrentPointPropInfo->ptr);
  this->AddPointInformation(data,ptr,numTuples,
                            numComps,currentGlobalPointIndex);
}

//-----------------------------------------------------------------------------
template<typename T>
void vtkLSDynaPart::AddPointInformation(T *buffer, T* pointData,
                                       const vtkIdType& numTuples,
                                       const vtkIdType& numComps,
                                       const vtkIdType& currentGlobalIndex)
{
  //only read the subset of points of this part that fall
  //inside the src buffer
  vtkIdType start(std::max(this->GlobalPointsUsed->minId(),
                                 currentGlobalIndex));
  vtkIdType end(std::min(this->GlobalPointsUsed->maxId(),
                               currentGlobalIndex+numTuples));

  //if the part has no place in this section of the points buffer
  //end will be larger than start
  if(start>=end)
    {
    return;
    }

  //offset all the pointers to the correct place
  T *src = buffer + ((start-currentGlobalIndex) * numComps);
  T *dest = pointData + (this->CurrentPointPropInfo->index * numComps);
  const size_t msize = sizeof(T) * numComps;

  //fix the start and end to be relative to the min id
  //this is because the global point used class is relative index based
  start -= this->GlobalPointsUsed->minId();
  end -= this->GlobalPointsUsed->minId();
  vtkIdType numPointsRead = 0;
  for(;start<end;++start,src+=numComps)
    {

    if(this->GlobalPointsUsed->isUsed(start))
      {
      memcpy(dest,src,msize);
      dest+=numComps;
      ++numPointsRead;
      }
    }

  this->CurrentPointPropInfo->index += numPointsRead;
}

//-----------------------------------------------------------------------------
void vtkLSDynaPart::GetPropertyData(const char* name,const vtkIdType &numComps,
    const bool &isIdTypeProperty, const bool& isProperty,
    const bool& isGeometry)
{
  this->CurrentPointPropInfo->ptr = NULL;
  vtkDataArray *data = NULL;
  if(isProperty)
    {
    data = this->Grid->GetPointData()->GetArray(name);
    if(!data)
      {
      //we have to construct the data array first
      if(!isIdTypeProperty)
        {
        data = (this->DoubleBased) ?
             (vtkDataArray*) vtkDoubleArray::New() :
             (vtkDataArray*) vtkFloatArray::New();
        }
      else
        {
        //the exception of the point arrays is the idType array which is
        data = vtkIdTypeArray::New();
        }
      data->SetName(name);
      data->SetNumberOfComponents(numComps);
      data->SetNumberOfTuples(this->NumberOfPoints);
      this->Grid->GetPointData()->AddArray(data);
      data->FastDelete();
      }
    }
  if(isGeometry)
    {
    if(this->DoubleBased)
      {
      this->Points->SetDataTypeToDouble();
      }
    else
      {
      this->Points->SetDataTypeToFloat();
      }

    if(data)
      {
      //this is the deflection array and needs to be set as the points
      //array
      this->Points->SetData(data);
      }
    else
      {
      //this is a pure geometry array and nothing else
      this->Points->SetNumberOfPoints(this->NumberOfPoints);
      data = this->Points->GetData();
      }
    }

  this->CurrentPointPropInfo->ptr = data->GetVoidPointer(0);
}


//-----------------------------------------------------------------------------
void vtkLSDynaPart::AddCellProperty(const char* name, const int& offset,
                                    const int& numComps)
{
  if(this->Grid->GetCellData()->HasArray(name))
    {
    //we only have to fill the cell properties class the first
    //time step after creating the part, the reset of the time
    //we are just changing the value in the data arrays
    return;
    }

  vtkDataArray *data=NULL;
  void *ptr = NULL;
  if(this->DoubleBased)
    {
    ptr = this->CellProperties->AddProperty<double>(offset,this->NumberOfCells,
                                                    numComps);
    }
  else
    {
    ptr = this->CellProperties->AddProperty<float>(offset,this->NumberOfCells,
                                                   numComps);
    }

  if(ptr)
    {
    data = (this->DoubleBased) ?
             (vtkDataArray*) vtkDoubleArray::New():
             (vtkDataArray*) vtkFloatArray::New();

    //we will manage the memory that the cell property points too
    data->SetNumberOfComponents(numComps);
    data->SetVoidArray(ptr,this->NumberOfCells*numComps,1);
    data->SetName(name);
    this->Grid->GetCellData()->AddArray(data);
    data->FastDelete();
    }

}

//-----------------------------------------------------------------------------
void vtkLSDynaPart::ReadCellProperties(float *cellProperties,
                                       const vtkIdType& numCells,
                                       const vtkIdType& numPropertiesInCell)
{
  float *cell = cellProperties;
  for(vtkIdType i=0;i<numCells;++i)
    {
    this->CellProperties->AddCellInfo(cell);
    cell += numPropertiesInCell;
    }

}
//-----------------------------------------------------------------------------
void vtkLSDynaPart::ReadCellProperties(double *cellProperties,
                                       const vtkIdType& numCells,
                                       const vtkIdType& numPropertiesInCell)
{
  double *cell = cellProperties;
  for(vtkIdType i=0;i<numCells;++i)
    {
    this->CellProperties->AddCellInfo(cell);
    cell += numPropertiesInCell;
    }
}

//-----------------------------------------------------------------------------
vtkIdType vtkLSDynaPart::GetMinGlobalPointId() const
{
  //presumes topology has been built already
  return this->GlobalPointsUsed->minId();
}

//-----------------------------------------------------------------------------
vtkIdType vtkLSDynaPart::GetMaxGlobalPointId() const
{
  //presumes topology has been built already
  return this->GlobalPointsUsed->maxId();
}

//-----------------------------------------------------------------------------
void vtkLSDynaPart::BuildCells()
{
  this->NumberOfCells = this->Cells->size();

  //make the unstrucuted grid data structures point to the
  //Cells vectors underlying memory
  vtkIdType cellDataSize = this->Cells->dataSize();

  //copy the contents from the part into a cell array.
  vtkIdTypeArray *cellArray = vtkIdTypeArray::New();
  cellArray->SetVoidArray(&this->Cells->data[0],cellDataSize,1);

  //set the idtype aray as the cellarray
  vtkCellArray *cells = vtkCellArray::New();
  cells->SetCells(this->NumberOfCells,cellArray);
  cellArray->FastDelete();

  //now copy the cell types from the vector to
  vtkUnsignedCharArray* cellTypes = vtkUnsignedCharArray::New();
  cellTypes->SetVoidArray(&this->Cells->types[0],this->NumberOfCells,1);

  //last is the cell locations
  vtkIdTypeArray *cellLocations = vtkIdTypeArray::New();
  cellLocations->SetVoidArray(&this->Cells->locations[0],this->NumberOfCells,1);

  //actually set up the grid
  this->Grid->SetCells(cellTypes,cellLocations,cells,NULL,NULL);

  //remove references
  cellTypes->FastDelete();
  cellLocations->FastDelete();
  cells->FastDelete();
}

//-----------------------------------------------------------------------------
void vtkLSDynaPart::BuildUniquePoints()
{

  //we need to determine the number of unique points in this part
  //walk the cell structure to find all the unique points

  std::vector<vtkIdType>::const_iterator cellIt;
  std::vector<vtkIdType>::iterator cIt;

  BitVector pointUsage(this->NumberOfGlobalPoints,false);
  this->NumberOfPoints = 0;
  for(cellIt=this->Cells->data.begin();cellIt!=this->Cells->data.end();)
    {
    const vtkIdType npts(*cellIt);
    ++cellIt;
    for(vtkIdType i=0;i<npts;++i,++cellIt)
      {
      const vtkIdType id((*cellIt)-1);
      if(!pointUsage[id])
        {
        pointUsage[id] = true;
        ++this->NumberOfPoints; //get the number of unique points
        }
      }
    }

  //find the min and max points used
  vtkIdType min = this->NumberOfGlobalPoints+1;
  vtkIdType max = -1;
  vtkIdType pos=0, numPointsFound=0;
  for(BitVector::const_iterator constIt=pointUsage.begin();
      constIt!=pointUsage.end();
      ++constIt,++pos)
    {
    if(*constIt)
      {
      ++numPointsFound;
      }
    if(numPointsFound==1 && min > pos)
      {
      min = pos;
      }
    if(numPointsFound==this->NumberOfPoints)
      {
      max = pos;
      break; //we iterated long enough
      }
    }

  //we do a two phase because we can minimize memory usage
  //we should make this a class like DensePointsUsed since
  //we can use the ratio to determine if a vector or a map is more
  //space efficent
  std::vector<vtkIdType> uniquePoints;
  const vtkIdType size( 1 + max-min );
  uniquePoints.resize(size,-1);

  vtkIdType idx=0;
  pos=0;
  for(vtkIdType i=min;i<=max;++i,++idx)
    {
    if(pointUsage[i])
      {
      uniquePoints[idx]=pos++;
      }
    }

  //now fixup the cellIds
  for(cIt=this->Cells->data.begin();cIt!=this->Cells->data.end();)
    {
    const vtkIdType npts(*cIt);
    ++cIt;
    for(vtkIdType i=0;i<npts;++i,++cIt)
      {
      const vtkIdType oId((*cIt)-min-1);
      *cIt = uniquePoints[oId];
      }
    }

  //determine the type of global point id storage is best
  vtkIdType ratio = (this->NumberOfPoints * sizeof(vtkIdType) ) / (max-min);
  if(ratio>0)
    {
    //the size of the bit array is less than the size of each number in memory
    //by it self
    this->GlobalPointsUsed = new vtkLSDynaPart::DensePointsUsed(&pointUsage,min,max);
    }
  else
    {
    this->GlobalPointsUsed = new vtkLSDynaPart::SparsePointsUsed(&pointUsage,min,max);
    }
}
