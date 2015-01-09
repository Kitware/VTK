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
#include "vtkLSDynaPart.h"
#include "LSDynaMetaData.h"


#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <vector>
#include <list>

//-----------------------------------------------------------------------------
class vtkLSDynaPartCollection::LSDynaPartStorage
{
protected:
  //---------------------------------------------------------------------------
  //stores the number of cells for a given part
  //this struct is meant to resemble a run length encoding style of storage
  //of mapping cell ids to the part that holds those cells
  struct PartInfo
    {
    PartInfo(vtkLSDynaPart *p, const int& type, const vtkIdType& pId,
             const vtkIdType& start, const vtkIdType& npts):
      numCells(1), //we are inserting the first cell when we create this so start with 1
      startId(start),
      cellStructureSize(npts), //start with number of points in first cell
      partId(pId)
    {
      //we store the part id our selves because we can have null parts
      //if the user has disabled reading that part
      this->part = p;
      if(this->part)
        {
        this->part->SetPartType(type);
        }
    }

    vtkIdType numCells; //number of cells in this continuous block
    vtkIdType startId; //the global index to start of this block
    vtkIdType cellStructureSize; //stores the size of the cell array for this section
    vtkIdType partId; //id of the part this block represents, because the part can be NULL
    vtkLSDynaPart *part;
    };

  //---------------------------------------------------------------------------
  struct PartInsertion
    {
    PartInsertion():numCellsInserted(0){}

    PartInsertion(std::vector<PartInfo> *pInfo):numCellsInserted(0)
    {
      this->pIt = pInfo->begin();
    }

    //increments the numCells, and when needed increments to the next part
    void inc()
    {
      ++numCellsInserted;
      if ( (*pIt).numCells == numCellsInserted)
        {
        ++pIt;
        numCellsInserted=0;
        }
    }

    std::vector<PartInfo>::iterator pIt;
    vtkIdType numCellsInserted;
    };
  //---------------------------------------------------------------------------

public:
  LSDynaPartStorage(const vtkIdType& numMaterials):
    NumParts(numMaterials),PartIteratorLoc(0)
    {
    //a part represents a single material. A part type is
    this->Info = new std::vector<PartInfo>[LSDynaMetaData::NUM_CELL_TYPES];
    this->CellInsertionIterators = new PartInsertion[LSDynaMetaData::NUM_CELL_TYPES];
    this->Parts = new vtkLSDynaPart*[numMaterials];
    for(vtkIdType i=0; i<numMaterials; ++i)
      {
      this->Parts[i]=NULL;
      }
    }
  ~LSDynaPartStorage()
    {
    for(vtkIdType i=0; i < this->NumParts; ++i)
      {
      if(this->Parts[i])
        {
        this->Parts[i]->Delete();
        this->Parts[i]=NULL;
        }
      }
    delete[] this->Parts;
    delete[] this->CellInsertionIterators;
    delete[] this->Info;
    }

  //---------------------------------------------------------------------------
  vtkIdType GetNumParts() const { return NumParts; }

  //---------------------------------------------------------------------------
  void RegisterCell(const int& partType,const vtkIdType &matId,
                    const vtkIdType &npts)
  {
    if(this->Info[partType].size() != 0)
      {
      PartInfo *info = &this->Info[partType].back();
      if(info->partId == matId)
        {
        //append to this item
        ++info->numCells;
        info->cellStructureSize += npts;
        }
      else
        {
        //add a new item
        //PartInfo sets the part type!
        PartInfo newInfo(this->Parts[matId],partType,matId,
          (info->startId + info->numCells), npts);
        this->Info[partType].push_back(newInfo);
        }
      }
    else
      {
      //PartInfo sets the part type!
      PartInfo newInfo(this->Parts[matId],partType,matId,0,npts);
      this->Info[partType].push_back(newInfo);
      }
  }

  //---------------------------------------------------------------------------
  void ConstructPart(const vtkIdType &index,
                     const std::string &name,
                     const int &materialId,
                     const int &numGlobalNodes,
                     const int &wordSize
                     )
  {
    vtkLSDynaPart *p = vtkLSDynaPart::New();
    p->InitPart(name,index,materialId,
                numGlobalNodes,wordSize);
    this->Parts[index] = p;
  }

  //---------------------------------------------------------------------------
  void InitCellInsertion()
  {
    //we build up an array of cell insertion iterators
    //that point to the first element of each part type info
    for(int i=0; i < LSDynaMetaData::NUM_CELL_TYPES; ++i)
      {
      if(this->Info[i].size()>0)
        {
        PartInsertion partIt(&this->Info[i]);
        this->CellInsertionIterators[i] = partIt;
        }
      }
  }

  //---------------------------------------------------------------------------
  void InsertCell(const int& partType, const int& cellType,
                  const vtkIdType& npts, vtkIdType conn[8])
  {
    //get the correct iterator from the array of iterations
    if(this->CellInsertionIterators[partType].pIt->part)
      {
      //only insert the cell if the part is turned on
      this->CellInsertionIterators[partType].pIt->part->AddCell(
            cellType,npts,conn);
      }
    this->CellInsertionIterators[partType].inc();
  }

  //---------------------------------------------------------------------------
  bool PartExists(const vtkIdType &index) const
  {
    if(index<0||index>this->NumParts)
      {
      return false;
      }
    return (this->Parts[index]!=NULL && this->Parts[index]->HasCells());
  }

  //---------------------------------------------------------------------------
  vtkLSDynaPart* GetPart(const vtkIdType &index)
  {
    return this->Parts[index];
  }

  //---------------------------------------------------------------------------
  vtkUnstructuredGrid* GetPartGrid(const vtkIdType &index)
  {
    return this->Parts[index]->GenerateGrid();
  }

  //---------------------------------------------------------------------------
  void InitPartIteration(const int &partType)
  {
    for(vtkIdType i=0; i < this->NumParts; ++i)
      {
      if(this->Parts[i] && this->Parts[i]->PartType() == partType)
        {
        PartIteratorLoc = i;
        this->PartIterator = this->Parts[i];
        return;
        }
      }
    //failed to find a part that matches the type
    PartIteratorLoc = -1;
    this->PartIterator = NULL;
  }

  //---------------------------------------------------------------------------
  bool GetNextPart(vtkLSDynaPart *&part)
  {
    if(!this->PartIterator)
      {
      part = NULL;
      return false;
      }
    part=this->PartIterator;

    //clear iterator before we search for the next part
    vtkIdType pos = this->PartIteratorLoc + 1;
    this->PartIterator = NULL;
    this->PartIteratorLoc = -1;

    //find the next part
    for(vtkIdType i=pos; i<this->NumParts;i++)
      {
      if(this->Parts[i] && this->Parts[i]->PartType() == part->PartType())
        {
        this->PartIteratorLoc = i;
        this->PartIterator = this->Parts[i];
        break;
        }
      }
    return true;
  }

  //---------------------------------------------------------------------------
  void AllocateParts()
  {
    vtkIdType numCells=0,cellLength=0;
    for (vtkIdType i=0; i < this->NumParts; ++i)
      {
      vtkLSDynaPart* part = this->Parts[i];

      if(part)
        {
        bool canBeAllocated = this->GetInfoForPart(part, numCells,cellLength);
        if(canBeAllocated)
          {
          part->AllocateCellMemory(numCells,cellLength);
          }
        else
          {
          //this part has no cells allocated to it, so remove it now.
          part->Delete();
          this->Parts[i] = NULL;
          }
        }
      }
    //Only needed when debugging
    //this->DumpPartInfo();
  }

  //---------------------------------------------------------------------------
  bool GetInfoForPart(vtkLSDynaPart *part, vtkIdType &numCells,
                      vtkIdType &cellArrayLength) const

  {
    //verify that the part is valid
    numCells = 0;
    cellArrayLength = 0;
    bool validPart = part->hasValidType();
    if(!validPart)
      {
      //we return early because an invalid type would
      //cause the Info array to be accessed out of bounds
      return validPart;
      }

    //give a part type and a material id
    //walk the run length encoding to determe the total size
    std::vector<PartInfo>::const_iterator it;
    for(it = this->Info[part->PartType()].begin();
        it != this->Info[part->PartType()].end(); ++it)
      {
      const PartInfo *info = &(*it);
      if(info->partId== part->GetPartId())
        {
        validPart = true;
        numCells += info->numCells;
        cellArrayLength += info->cellStructureSize;
        }
      }

    return validPart;
  }

  //---------------------------------------------------------------------------
  void DumpPartInfo()
  {
  for(int i=0; i < LSDynaMetaData::NUM_CELL_TYPES;++i)
    {
    //now lets dump all the part info

    std::cout << "For Info index: " << i << std::endl;
    std::cout << "We have " << this->Info[i].size() << " info entries" <<std::endl;

    std::vector<PartInfo>::const_iterator it;
    for(it = this->Info[i].begin();
      it != this->Info[i].end(); ++it)
      {
      const PartInfo *info = &(*it);
      if(info->part != NULL)
        {
        std::cout << "The material id is: " << info->partId << std::endl;
        std::cout << "The numCells is:    " << info->numCells << std::endl;
        std::cout << std::endl;
        std::cout << "The Part is :" << std::endl;
        info->part->PrintSelf(cout,vtkIndent().GetNextIndent());
        std::cout << std::endl;
        std::cout << std::endl;
        }
      }
    }
  }


  //---------------------------------------------------------------------------
  void InitCellIteration(const int &partType, int pos=0)
  {
    this->CellIteratorEnd = this->Info[partType].end();
    if(this->Info[partType].size()>0)
      {
      this->CellIterator = this->Info[partType].begin();
      }
    else
      {
      this->CellIterator = this->Info[partType].end();
      }

    while(pos>0 && this->CellIterator != this->CellIteratorEnd)
      {
      pos -= (*this->CellIterator).numCells;
      if(pos>0)
        {
        ++this->CellIterator;
        }
      }
  }

  //---------------------------------------------------------------------------
  bool GetNextCellPart(vtkIdType& startId, vtkIdType &numCells,
                       vtkLSDynaPart *&part)
  {
    if(this->CellIterator == this->CellIteratorEnd)
      {
      return false;
      }

    startId = (*this->CellIterator).startId;
    numCells = (*this->CellIterator).numCells;
    part = (*this->CellIterator).part;
    ++this->CellIterator;
    return true;
  }

  //---------------------------------------------------------------------------
  void FinalizeTopology()
  {
    for (vtkIdType i=0; i < this->NumParts; ++i)
      {
      vtkLSDynaPart* part = this->Parts[i];
      if (part && part->HasCells())
        {
        part->BuildToplogy();
        }
      else if(part)
        {
        part->Delete();
        this->Parts[i]=NULL;
        }
      }
  }

  //---------------------------------------------------------------------------
  void DisableDeadCells()
  {
    for (vtkIdType i=0; i < this->NumParts; ++i)
      {
      vtkLSDynaPart* part = this->Parts[i];
      if (part && part->HasCells())
        {
        part->DisableDeadCells();
        }
      }
  }

  //---------------------------------------------------------------------------
  void PrintSelf(ostream &os, vtkIndent indent)
  {
    for (vtkIdType i=0; i < this->NumParts; ++i)
      {
      os << indent << "Part Number " << i << std::endl;
      if(this->PartExists(i))
        {
        vtkLSDynaPart* part = this->Parts[i];
        part->PrintSelf(os,indent.GetNextIndent());
        }
      else
        {
        os << indent.GetNextIndent() << "Does not exist." << std::endl;
        }
      }
    }

protected:
  vtkIdType NumParts;

  //stores all the parts for this collection.
  vtkLSDynaPart **Parts;

  //maps cell indexes which are tracked by output type to the part
  //Since cells are ordered the same between the cell connectivity data block
  //and the state block in the d3plot format we only need to know which part
  //the cell is part of.
  //This info is constant for each time step
  std::vector<PartInfo> *Info;
  PartInsertion *CellInsertionIterators;

  std::vector<PartInfo>::const_iterator CellIterator,CellIteratorEnd;
  vtkLSDynaPart *PartIterator;
  vtkIdType PartIteratorLoc;
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
  delete this->Storage;
  delete[] this->MinIds;
  delete[] this->MaxIds;
  this->MetaData = NULL;
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::PrintSelf(ostream &os, vtkIndent indent)
{
  //just needs to print all public accessible ivars
  this->Superclass::PrintSelf(os, indent);

  //number of parts
  os << indent << "Number of Parts: " << this->GetNumberOfParts() << std::endl;

  //print self for each part
  this->Storage->PrintSelf(os,indent.GetNextIndent());
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::InitCollection(LSDynaMetaData *metaData,
                                             vtkIdType* mins, vtkIdType* maxs)
{
  delete this->Storage;
  delete[] this->MinIds;
  delete[] this->MaxIds;

  //reserve enough space for the grids. Each node
  //will have a part allocated, since we don't know yet
  //how the cells map to parts.
  this->Storage = new LSDynaPartStorage(metaData->PartIds.size());

  this->MinIds = new vtkIdType[LSDynaMetaData::NUM_CELL_TYPES];
  this->MaxIds = new vtkIdType[LSDynaMetaData::NUM_CELL_TYPES];

  //We only have to map the cell ids between min and max, so we
  //skip into the proper place
  for(int i=0; i < LSDynaMetaData::NUM_CELL_TYPES;++i)
    {
    this->MinIds[i]= (mins!=NULL) ? mins[i] : 0;
    this->MaxIds[i]= (maxs!=NULL) ? maxs[i] : metaData->NumberOfCells[i];
    }

  if(metaData)
    {
    this->MetaData = metaData;
    this->BuildPartInfo();
    }
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::BuildPartInfo()
{
  //we iterate on part materials as those are those are from 1 to num Parts.
  //the part ids are the user material ids

  std::vector<int>::const_iterator partMIt;
  std::vector<int>::const_iterator materialIdIt = this->MetaData->PartIds.begin();
  std::vector<int>::const_iterator statusIt = this->MetaData->PartStatus.begin();
  std::vector<std::string>::const_iterator nameIt = this->MetaData->PartNames.begin();

  for (partMIt = this->MetaData->PartMaterials.begin();
       partMIt != this->MetaData->PartMaterials.end();
       ++partMIt,++statusIt,++nameIt,++materialIdIt)
    {
    if (*statusIt)
      {
      //make the index contain a part
      this->Storage->ConstructPart((*partMIt)-1,*nameIt,*materialIdIt,
                                   this->MetaData->NumberOfNodes,
                                   this->MetaData->Fam.GetWordSize());
      }
    }
}


//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::RegisterCellIndexToPart(const int& partType,
                                            const vtkIdType& matId,
                                            const vtkIdType&,
                                            const vtkIdType& npts)

{
  this->Storage->RegisterCell(partType,matId-1,npts);
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::AllocateParts( )
{
  this->Storage->AllocateParts();
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::InitCellInsertion()
{
  this->Storage->InitCellInsertion();
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::InsertCell(const int& partType,
                                         const vtkIdType&,
                                         const int& cellType,
                                         const vtkIdType& npts,
                                         vtkIdType conn[8])
{

  this->Storage->InsertCell(partType,cellType,npts,conn);
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::SetCellDeadFlags(const int& partType,
                                          vtkUnsignedCharArray *death,
                                          const int& deadCellsAsGhostArray)
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
  this->Storage->InitCellIteration(partType);
  vtkIdType numCells, startId;
  vtkLSDynaPart *part;
  unsigned char* dead = static_cast<unsigned char*>(death->GetVoidPointer(0));
  while(this->Storage->GetNextCellPart(startId,numCells,part))
    {
    //perfectly valid to have a NULL part being returned
    //just skip it as the user doesn't want it loaded.
    if(part)
      {
      part->EnableDeadCells(deadCellsAsGhostArray);
      part->SetCellsDeadState(dead,numCells);
      }
    dead += numCells;
    }
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::AddProperty(
                    const LSDynaMetaData::LSDYNA_TYPES& type, const char* name,
                    const int& offset, const int& numComps)
{

  vtkLSDynaPart* part = NULL;
  this->Storage->InitPartIteration(type);
  while(this->Storage->GetNextPart(part))
    {
    if(part)
      {
      part->AddCellProperty(name,offset,numComps);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::FillCellProperties(float *buffer,
  const LSDynaMetaData::LSDYNA_TYPES& type, const vtkIdType& startId,
  const vtkIdType& numCells, const int& numPropertiesInCell)
{
  this->FillCellArray(buffer,type,startId,numCells,numPropertiesInCell);
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::FillCellProperties(double *buffer,
  const LSDynaMetaData::LSDYNA_TYPES& type, const vtkIdType& startId,
  const vtkIdType& numCells, const int& numPropertiesInCell)
{
  this->FillCellArray(buffer,type,startId,numCells,numPropertiesInCell);
}

//-----------------------------------------------------------------------------
template<typename T>
void vtkLSDynaPartCollection::FillCellArray(T *buffer,
  const LSDynaMetaData::LSDYNA_TYPES& type, const vtkIdType& startId,
  vtkIdType numCells, const int& numPropertiesInCell)
{
  //we only need to iterate the array for the subsection we need
  T* loc = buffer;
  vtkIdType size, globalStartId;
  vtkLSDynaPart *part;
  this->Storage->InitCellIteration(type,startId);
  while(this->Storage->GetNextCellPart(globalStartId,size,part))
    {
    vtkIdType start = std::max(globalStartId,startId);
    vtkIdType end = std::min(globalStartId+size,startId+numCells);
    if(end<start)
      {
      break;
      }
    vtkIdType is = end - start;
    if(part)
      {
      part->ReadCellProperties(loc,is,numPropertiesInCell);
      }
    loc += is * numPropertiesInCell;
    }
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::ReadCellUserIds(
    const LSDynaMetaData::LSDYNA_TYPES& type, const int& status)
{

  vtkIdType numCells,numSkipStart,numSkipEnd;
  this->GetPartReadInfo(type,numCells,numSkipStart,numSkipEnd);

  if(!status)
    {
    //skip this part type
    this->MetaData->Fam.SkipWords(numSkipStart + numCells + numSkipEnd);
    return;
    }

  this->MetaData->Fam.SkipWords(numSkipStart);
  vtkIdType numChunks = this->MetaData->Fam.InitPartialChunkBuffering(numCells,1);
  vtkIdType startId = 0;
  if(this->MetaData->Fam.GetWordSize() == 8 && numCells > 0)
    {
    for(vtkIdType i=0; i < numChunks; ++i)
      {
      vtkIdType chunkSize = this->MetaData->Fam.GetNextChunk( LSDynaFamily::Float);
      vtkIdType numCellsInChunk = chunkSize;
      vtkIdType *buf = this->MetaData->Fam.GetBufferAs<vtkIdType>();
      this->FillCellUserId(buf,type,startId,numCellsInChunk);
      startId += numCellsInChunk;
      }
    }
  else if (numCells > 0)
    {
    for(vtkIdType i=0; i < numChunks; ++i)
      {
      vtkIdType chunkSize = this->MetaData->Fam.GetNextChunk( LSDynaFamily::Float);
      vtkIdType numCellsInChunk = chunkSize;
      int *buf = this->MetaData->Fam.GetBufferAs<int>();
      this->FillCellUserId(buf,type,startId,numCellsInChunk);
      startId += numCellsInChunk;
      }
    }
  this->MetaData->Fam.SkipWords(numSkipEnd);

  //clear the buffer as it will be very large and not needed
  this->MetaData->Fam.ClearBuffer();
}

//-----------------------------------------------------------------------------
template<typename T>
void vtkLSDynaPartCollection::FillCellUserIdArray(T *buffer,
  const LSDynaMetaData::LSDYNA_TYPES& type, const vtkIdType& startId,
  vtkIdType numCells)
{
  const int numWordsPerIdType(this->MetaData->Fam.GetWordSize() / sizeof(T));
  //we only need to iterate the array for the subsection we need
  T* loc = buffer;
  vtkIdType size,globalStartId;
  vtkLSDynaPart *part;
  this->Storage->InitCellIteration(type,startId);
  while(this->Storage->GetNextCellPart(globalStartId,size,part))
    {
    vtkIdType start = std::max(globalStartId,startId);
    vtkIdType end = std::min(globalStartId+size,startId+numCells);
    if(end<start)
      {
      break;
      }
    vtkIdType is = (end - start)*numWordsPerIdType;
    if(part)
      {
      part->EnableCellUserIds();
      for(vtkIdType i=0; i<is; i+=numWordsPerIdType)
        {
        part->SetNextCellUserIds((vtkIdType)loc[i]);
        }
      }
    //perfectly valid to have a NULL part being returned
    //just skip it as the user doesn't want it loaded.
    loc+=is;
    }
}

//-----------------------------------------------------------------------------
bool vtkLSDynaPartCollection::IsActivePart(const int& id) const
{
  return this->Storage->PartExists(id);
}

//-----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkLSDynaPartCollection::GetGridForPart(
  const int& index) const
{
  return this->Storage->GetPartGrid(index);
}

//-----------------------------------------------------------------------------
int vtkLSDynaPartCollection::GetNumberOfParts() const
{
  return static_cast<int>(this->Storage->GetNumParts());
}
//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::DisbleDeadCells()
{
  this->Storage->DisableDeadCells();
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::GetPartReadInfo(const int& partType,
  vtkIdType& numberOfCells, vtkIdType& numCellsToSkipStart,
  vtkIdType& numCellsToSkipEnd) const
{
  vtkIdType size = this->MaxIds[partType]-this->MinIds[partType];
  if(size<=0)
    {
    numberOfCells = 0;
    //skip everything
    numCellsToSkipStart = this->MetaData->NumberOfCells[partType];
    numCellsToSkipEnd = 0; //no reason to skip anything else
    }
  else
    {
    numberOfCells = size;
    numCellsToSkipStart = this->MinIds[partType];
    numCellsToSkipEnd = this->MetaData->NumberOfCells[partType] -
                                        (numberOfCells+numCellsToSkipStart);
    }
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::FinalizeTopology()
{
  this->Storage->FinalizeTopology();
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::ReadPointUserIds(const vtkIdType& numTuples,
                                        const char* name)
{
  this->SetupPointPropertyForReading(numTuples,1,name,true,true,false,false);
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::ReadPointProperty(
                                        const vtkIdType& numTuples,
                                        const vtkIdType& numComps,
                                        const char* name,
                                        const bool &isProperty,
                                        const bool& isGeometryPoints,
                                        const bool& isRoadPoints)
{
  this->SetupPointPropertyForReading(numTuples,numComps,name,false,isProperty,
                                     isGeometryPoints,isRoadPoints);
}

//-----------------------------------------------------------------------------
void vtkLSDynaPartCollection::SetupPointPropertyForReading(
                                        const vtkIdType& numTuples,
                                        const vtkIdType& numComps,
                                        const char* name,
                                        const bool &isIdType,
                                        const bool &isProperty,
                                        const bool& isGeometryPoints,
                                        const bool& isRoadPoints)
{
  if ( !isProperty && !isGeometryPoints && !isRoadPoints)
    {
    // don't read arrays the user didn't request, just skip them
    this->MetaData->Fam.SkipWords(numTuples * numComps);
    return;
    }

  //If this is a geometeric point property it needs to apply
  //to the following
  //BEAM,SHELL,THICK_SHELL,SOLID,Particles
  //if it is road surface it only applies to RigidSurfaceData part types
  vtkLSDynaPart* part=NULL;
  vtkLSDynaPart **validParts = new vtkLSDynaPart*[this->Storage->GetNumParts()];
  vtkIdType idx=0;
  if(!isRoadPoints)
    {
    enum LSDynaMetaData::LSDYNA_TYPES validCellTypes[5] = {
          LSDynaMetaData::PARTICLE,
          LSDynaMetaData::BEAM,
          LSDynaMetaData::SHELL,
          LSDynaMetaData::THICK_SHELL,
          LSDynaMetaData::SOLID
          };
    for(int i=0; i<5;++i)
      {
      this->Storage->InitPartIteration(validCellTypes[i]);
      while(this->Storage->GetNextPart(part))
        {
        part->AddPointProperty(name,numComps,isIdType,isProperty,
                              isGeometryPoints);
        validParts[idx++]=part;
        }
      }
    }
  else
    {
    //is a road point
    this->Storage->InitPartIteration(LSDynaMetaData::ROAD_SURFACE);
    while(this->Storage->GetNextPart(part))
      {
      part->AddPointProperty(name,numComps,isIdType,isProperty,
                            isGeometryPoints);
      validParts[idx++]=part;
      }
    }

  if(idx<=0)
    {
    //don't do anything as we have no valid parts
    }
  else if(this->MetaData->Fam.GetWordSize() == 8)
    {
    this->FillPointProperty<double>(numTuples,numComps,validParts, idx);
    }
  else
    {
    this->FillPointProperty<float>(numTuples,numComps,validParts, idx);
    }

  delete[] validParts;
}

namespace
{
  //this function is used to sort a collection of parts
  //based on the max and min global point ids that the part
  //we use both to enforce better weak ordering
  bool sortPartsOnGlobalIds(const vtkLSDynaPart *p1, const vtkLSDynaPart *p2)
    {
    if(p1->GetMaxGlobalPointId() < p2->GetMaxGlobalPointId())
      {
      return true;
      }
      return false;
    }
}

//-----------------------------------------------------------------------------
template<typename T>
void vtkLSDynaPartCollection::FillPointProperty(const vtkIdType& numTuples,
                                                const vtkIdType& numComps,
                                                vtkLSDynaPart** parts,
                                                const vtkIdType numParts)
{
  LSDynaMetaData* p = this->MetaData;

  //construct the sorted array of parts so we only
  //have to iterate a subset that are interested in the points we have
  //are reading in.
  std::list<vtkLSDynaPart*> sortedParts(parts,parts+numParts);
  std::list<vtkLSDynaPart*>::iterator partIt;

  sortedParts.sort(sortPartsOnGlobalIds);

  //find the max as the subset of points
  const vtkIdType maxGlobalPoint(sortedParts.back()->GetMaxGlobalPointId());
  vtkIdType minGlobalPoint = maxGlobalPoint;
  for(partIt = sortedParts.begin(); partIt != sortedParts.end(); ++partIt)
    {
    minGlobalPoint = std::min((*partIt)->GetMinGlobalPointId(),minGlobalPoint);
    }

  const vtkIdType realNumberOfTuples(maxGlobalPoint-minGlobalPoint);
  const vtkIdType numPointsToSkipStart(minGlobalPoint);
  const vtkIdType numPointsToSkipEnd(numTuples - (realNumberOfTuples + minGlobalPoint));

  vtkIdType offset = numPointsToSkipStart;
  const vtkIdType numPointsToRead(1048576);
  const vtkIdType loopTimes(realNumberOfTuples/numPointsToRead);
  const vtkIdType leftOver(realNumberOfTuples%numPointsToRead);
  const vtkIdType bufferChunkSize(numPointsToRead*numComps);

  T* buf = NULL;
  p->Fam.SkipWords(numPointsToSkipStart * numComps);
  for(vtkIdType j=0;j<loopTimes;++j,offset+=numPointsToRead)
    {
    p->Fam.BufferChunk(LSDynaFamily::Float,bufferChunkSize);
    buf = p->Fam.GetBufferAs<T>();

    partIt = sortedParts.begin();
    while(partIt!=sortedParts.end() &&
          (*partIt)->GetMaxGlobalPointId() < offset)
      {
      //remove all parts from the list that have already been
      //filled by previous loops
      sortedParts.pop_front();
      partIt = sortedParts.begin();
      }

    while(partIt!=sortedParts.end())
      {
      //only read the points which have a point that lies within this section
      //so we stop once the min is larger than our max id
      (*partIt)->ReadPointBasedProperty(buf,numPointsToRead,numComps,offset);
      ++partIt;
      }
    }
  if(leftOver>0 && sortedParts.size() > 0)
    {
    p->Fam.BufferChunk(LSDynaFamily::Float, leftOver*numComps);
    buf = p->Fam.GetBufferAs<T>();
    for (partIt = sortedParts.begin(); partIt!=sortedParts.end();++partIt)
      {
      (*partIt)->ReadPointBasedProperty(buf,leftOver,numComps,offset);
      }
    }
  p->Fam.SkipWords(numPointsToSkipEnd * numComps);
}
