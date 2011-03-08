/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExodusReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkExodusReader.h"
#include "vtkExodusModel.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLParser.h"
#include "vtkWarpVector.h"
#include "vtkDSPFilterDefinition.h"

#include <sys/stat.h>
#include <ctype.h>
#include "vtk_exodusII.h"

#include <vtkstd/algorithm>
#include <vtkstd/string>
#include <vtkstd/vector>
#include <vtkstd/map>
#include <vtkstd/list>
#include "vtkStdString.h"

#include <math.h>

#define DEBUG 0

vtkStdString _blocks;

// vtkExodusMetadata is an internal helper class that
// manages the metadata associated with the point and cell
// arrays. This class uses stl
class vtkExodusMetadata
{
private:
  int ArrayStatusInitValue[vtkExodusReader::NUM_ARRAY_TYPES];

  // Field array stuff
  vtkstd::vector<vtkStdString> pointArrayNames;
  vtkstd::vector<int> pointArrayComponents;
  vtkstd::vector<int> pointArrayStatus;
  vtkstd::map<vtkStdString,int> pointArrayInitStatus;

  vtkstd::vector<vtkStdString> cellArrayNames;
  vtkstd::vector<int> cellArrayComponents;
  vtkstd::vector<int> cellArrayStatus;
  vtkstd::map<vtkStdString,int> cellArrayInitStatus;

  int originalNumberOfPointArrays;
  int originalNumberOfCellArrays;

  // This is a cruddy hack... because we need to pass a
  // char ** pointer to the exodus function
  char **pointArrayRawBuffer;
  char **cellArrayRawBuffer;

  // Block stuff
  vtkstd::vector<vtkStdString> blockNames;
  vtkstd::vector<int> blockIds;
  vtkstd::vector<int> blockStatus;
  vtkstd::vector<int> numElementsInBlock;
  vtkstd::map<vtkStdString,int> blockInitStatus;

  // Node/Side set id stuff
  //  Store *Ids* for each node/side set
  //  Store *Size* for each node/side set
  //  Store number of *Distribution Factors** for each node/side set
  //  FIXME!!! since node/side set ids should be unique, should this
  //           be a 'set' instead of a 'vector'?
  //  FIXME!!! should we store side/edge type along with side set info?
  vtkstd::vector<int> nodeSetId;
  vtkstd::vector<int> nodeSetSize;
  vtkstd::vector<int> nodeSetDistFact;
  vtkstd::vector<int> nodeSetStatus;
  vtkstd::vector<vtkStdString> nodeSetName;
  vtkstd::map<vtkStdString,int> nodeSetInitStatus;

  vtkstd::vector<int> sideSetId;
  vtkstd::vector<int> sideSetSize;
  vtkstd::vector<int> sideSetDistFact;
  vtkstd::vector<int> sideSetStatus;
  vtkstd::vector<vtkStdString> sideSetName;
  vtkstd::map<vtkStdString,int> sideSetInitStatus;

  //part stuff. There is no PartStatus array
  //a part is active only if all its blocks are active
  vtkstd::vector<vtkStdString> partNames;
  vtkstd::map<int,int> blockIDToPartID;
  vtkstd::map<int,vtkstd::vector<int> > partIDToBlockIDs;
  
  //material stuff. Again, no material array
  //a material is active only is all its blocks are active
  vtkstd::vector<vtkStdString> materialNames;
  vtkstd::map<int,int> blockIDToMaterialID;
  vtkstd::map<int,vtkstd::vector<int> > materialIDToBlockIDs;

  //assembly stuff
  vtkstd::vector<vtkStdString> assemblyNames;
  vtkstd::map<int,vtkstd::vector<int> > blockIDToAssemblyIDs;
  vtkstd::map<int,vtkstd::vector<int> > assemblyIDToBlockIDs;
  

  //sortedorder[sortedID]=unsortedID
  //can do name[sortedOrder[idx]]
  vtkstd::map<int,int> sortedOrder;

  int DisplayType;

public:

  // Constructor
  vtkExodusMetadata()
    {
    pointArrayRawBuffer = NULL;
    cellArrayRawBuffer = NULL;
    originalNumberOfPointArrays = 0;
    originalNumberOfCellArrays = 0;
    DisplayType=1;
    for ( int i=0;i<vtkExodusReader::NUM_ARRAY_TYPES;i++ )
      {
      this->ArrayStatusInitValue[i] = 0;
      }
    }

  // Destructor
  ~vtkExodusMetadata()
    {
    int i;
    for (i=0; i< this->originalNumberOfPointArrays; i++)
      {
      delete [] this->pointArrayRawBuffer[i];
      }
    if (this->pointArrayRawBuffer)
      {
      delete [] this->pointArrayRawBuffer;
      }
    for (i=0; i< this->originalNumberOfCellArrays; i++)
      {
      delete [] this->cellArrayRawBuffer[i];
      }
    if (this->cellArrayRawBuffer)
      {
      delete [] this->cellArrayRawBuffer;
      }
    }

  // Point array attributes
  char** AllocatePointArrayNameBuffer(int num_strings);
  char** GetPointArrayNameBuffer();
  char *GetPointArrayOriginalName(int i)
    {
      return pointArrayRawBuffer[i];
    }
  inline int GetNumberOfPointArrays() 
    { 
      return static_cast<int>( pointArrayNames.size() ); 
    }
  inline const char* GetPointArrayName(int idx) 
    { 
      return pointArrayNames[idx].c_str(); 
    }
  inline int GetPointArrayComponents(int idx) 
    { 
      return pointArrayComponents[idx]; 
    }
  inline int GetPointArrayStatus(int idx) 
    { 
      return pointArrayStatus[idx]; 
    }
  inline void SetPointArrayInitStatus(vtkStdString name, int on) 
    { 
      pointArrayInitStatus[name] = on; 
    }
  inline void SetPointArrayStatus(int idx, int on) 
    { 
      pointArrayStatus[idx] = on; 
    }
  inline void SetPointArrayStatus(vtkStdString name, int flag) 
    {
      for(int idx=0; idx<this->GetNumberOfPointArrays(); ++idx)
        {
        if ( name == this->pointArrayNames[idx] ) 
          {
          this->pointArrayStatus[idx] = flag;
          return;
          }
        }
    }
  inline int GetPointArrayStatus(vtkStdString name) 
    {
      for(int idx=0; idx<this->GetNumberOfPointArrays(); ++idx)
        {
        if ( name == this->pointArrayNames[idx] )
          return this->pointArrayStatus[idx];
        }
      return -1;
    }

  // Cell array attributes
  char** AllocateCellArrayNameBuffer(int num_strings);
  char** GetCellArrayNameBuffer();
  char *GetCellArrayOriginalName(int i)
    {
      return cellArrayRawBuffer[i];
    }
  inline int GetNumberOfCellArrays() 
    { 
      return static_cast<int>( cellArrayNames.size() ); 
    }
  inline const char* GetCellArrayName(int idx) 
    { 
      return cellArrayNames[idx].c_str(); 
    }
  inline int GetCellArrayComponents(int idx) 
    { 
      return cellArrayComponents[idx]; 
    }
  inline void SetCellArrayInitStatus(vtkStdString name, int on) 
    { 
      cellArrayInitStatus[name] = on; 
    }
  inline void SetCellArrayStatus(int idx, int flag) 
    { 
      cellArrayStatus[idx] = flag; 
    }
  inline void SetCellArrayStatus(vtkStdString name, int flag) 
    {
      for(int idx=0; idx<this->GetNumberOfCellArrays(); ++idx)
        {
        if ( name == this->cellArrayNames[idx] ) 
          {
          this->cellArrayStatus[idx] = flag;
          return;
          }
        }
    }
  inline int GetCellArrayStatus(int idx) 
    { 
      return cellArrayStatus[idx]; 
    }
  inline int GetCellArrayStatus(vtkStdString name) {
    for(int idx=0; idx<this->GetNumberOfCellArrays(); ++idx)
      {
      if ( name == this->cellArrayNames[idx] ) 
        return this->cellArrayStatus[idx];
      }
    return -1;
  }

  // These are used when accessing things like truth tables indices
  inline int GetOriginalNumberOfPointArrays() 
    { 
      return originalNumberOfPointArrays; 
    }
  inline int GetOriginalNumberOfCellArrays() 
    { 
      return originalNumberOfCellArrays; 
    }

//-----------------------------------------------------------------------
  // Node/Side set functions
  //-----------------------------------------------------------------------
  // id - node/side set exodus id
  // size - node/side set size
  // dist - number of distribution factors
  // default node/side set status to 'of' or '0'
  inline void AddNodeSet( vtkStdString name, const int id, const int size, const int dist, int status )
    {
      this->nodeSetId.push_back( id );
      this->nodeSetName.push_back( name );
      this->nodeSetSize.push_back( size );
      this->nodeSetDistFact.push_back( dist );
      this->nodeSetStatus.push_back( status );
    }
  inline void AddSideSet( vtkStdString name, const int id, const int size, const int dist, int status )
    {
      this->sideSetId.push_back( id );
      this->sideSetName.push_back( name );
      this->sideSetSize.push_back( size );
      this->sideSetDistFact.push_back( dist );
      this->sideSetStatus.push_back( status );
    }

  // Do a bound check and return a non-sensical -1 for a count if the user
  // asks for a non-existent id. The return value is not an error value 
  // per se, but we have to return something instead of overrunning the
  // array bounds
  //
  // FIXME!!! this is simply the number of node/side sets set by
  //          vtkExodusReader, it *could* be different than the number
  //          reported by the exodus file. since vtkExodusReader loads
  //          metadata for all node/side sets this *should* agree with
  //          the number reported by exodus. Status flags specify which
  //          node/side sets get loaded and visualized.
  inline int GetNumberOfNodeSets() { return static_cast<int>( this->nodeSetId.size() ); }
  inline int GetNumberOfSideSets() { return static_cast<int>( this->sideSetId.size() ); }

  inline int GetNodeSetId( const int id )
    {
      return (id>=0 && id<(int)nodeSetId.size()) ? nodeSetId[id] : -1;
    }
  inline int GetSideSetId( const int id )
    {
      return (id>=0 && id<(int)sideSetId.size()) ? sideSetId[id] : -1;
    }
  inline const char* GetNodeSetName( const int id )
    {
      return nodeSetName[id].c_str();
    }
  inline const char* GetSideSetName( const int id )
    {
      return sideSetName[id].c_str();
    }

  inline int GetNodeSetSize( const int id )
    {
      return (id>=0 && id<(int)nodeSetSize.size()) ? nodeSetSize[id] : -1;
    }
  inline int GetSideSetSize( const int id )
    {
      return (id>=0 && id<(int)sideSetSize.size()) ? sideSetSize[id] : -1;
    }

  inline int GetNodeSetDistFact( const int id )
    {
      return (id>=0 && id<(int)nodeSetDistFact.size()) ? nodeSetDistFact[id] : -1;
    }
  inline int GetSideSetDistFact( const int id )
    {
      return (id>=0 && id<(int)sideSetDistFact.size()) ? sideSetDistFact[id] : -1;
    }

  inline int GetNodeSetInitStatus( vtkStdString name )
    {
    vtkstd::map<vtkStdString,int>::iterator i = nodeSetInitStatus.find(name);
    if( i!= nodeSetInitStatus.end() )
      {
      return (*i).second;
      }
    else
      {
      return 0;
      }
    }
  inline void SetNodeSetInitStatus( vtkStdString name, const int status )
    {
      this->nodeSetInitStatus[name] = status;
    }
  inline void SetNodeSetStatus( const int id, const int status )
    {
      if( id>=0 && id<(int)this->nodeSetStatus.size() )
        {
        this->nodeSetStatus[id] = status;
        }
      return;
    }
  inline void SetNodeSetStatus(vtkStdString name, int status) 
    {
      for(vtkstd::vector<int>::size_type id=0; id < this->nodeSetStatus.size(); ++id) 
        {
        if ( name == this->nodeSetName[id] ) 
          {
          this->SetNodeSetStatus(static_cast<int>(id),status);
          return;
          }
        }
    }
  inline int GetSideSetInitStatus( vtkStdString name )
    {
    vtkstd::map<vtkStdString,int>::iterator i = sideSetInitStatus.find(name);
    if( i!= sideSetInitStatus.end() )
      {
      return (*i).second;
      }
    else
      {
      return 0;
      }
    }
  inline void SetSideSetInitStatus( vtkStdString name, const int status )
    {
      this->sideSetInitStatus[name] = status;
    }
  inline void SetSideSetStatus( const int id, const int status )
    {
      if( id>=0 && id<(int)this->sideSetStatus.size() )
        {
        this->sideSetStatus[id] = status;
        }
      return;
    }
  inline void SetSideSetStatus(vtkStdString name, int status) 
    {
      for(vtkstd::vector<int>::size_type id=0; id < this->sideSetStatus.size(); ++id) 
        {
        if ( name == this->sideSetName[id] ) 
          {
          this->SetSideSetStatus(static_cast<int>(id),status);
          return;
          }
        }
    }

  // Return node/side set status if the id corresponds to a real node/side set.
  // I the user asks for the status of a node/side set that doesn't exist, return
  // zero and let the philosophers debate whether a node/side set that doesn't exist
  // is on or off; that's at least 3 Phd thesies right there!
  inline int GetNodeSetStatus(const int id )
    {
      return (id>=0 && id<(int)nodeSetStatus.size()) ? nodeSetStatus[id] : 0;
    }
  inline int GetNodeSetStatus(vtkStdString name) 
    {
      for(vtkstd::vector<int>::size_type id=0; id < this->nodeSetStatus.size(); ++id) 
        {
        if ( name == this->nodeSetName[id] ) 
          {
          return this->GetNodeSetStatus(static_cast<int>(id));
          }
        }
        return 0;
    }

  inline int GetSideSetStatus( const int id )
    {
      return (id>=0 && id<(int)sideSetStatus.size()) ? sideSetStatus[id] : 0;
    }
  inline int GetSideSetStatus(vtkStdString name) 
    {
      for(vtkstd::vector<int>::size_type id=0; id < this->sideSetStatus.size(); ++id) 
        {
        if ( name == this->sideSetName[id] ) 
          {
          return this->GetSideSetStatus(static_cast<int>(id));
          }
        }
        return 0;
    }
  //-----------------------------------------------------------------------
  // End Node/Side set functions
  //-----------------------------------------------------------------------
  
  
  // Block attributes
  inline void AddBlock(vtkStdString blockName, vtkStdString partName, 
                       vtkStdString materialName, 
                       vtkstd::vector<vtkStdString> localAssemblyNames,
                       int id, int num_elem, int status) 
  {
    blockNames.push_back(blockName);
    blockIds.push_back(id);
    blockStatus.push_back(status);
    numElementsInBlock.push_back(num_elem);
    unsigned int i;
    int found=0;
    int blockID=static_cast<int>(blockNames.size())-1;
    sortedOrder[blockID]=blockID;
    //set up the Part Arrays
    for (i=0;i<partNames.size();i++){
    if (partNames[i]==partName)
      {
      blockIDToPartID[blockID]=i;
      partIDToBlockIDs[i].push_back(blockID);
      found=1;
      }
    }
    if (!found)
      {
      int partID=static_cast<int>(partNames.size());
      partNames.push_back(partName);
      blockIDToPartID[blockID]=partID;
      partIDToBlockIDs[partID]=vtkstd::vector<int>();
      partIDToBlockIDs[partID].push_back(blockID);
      }
    
    //set up the material arrays
    found=0;
    for (i=0;i<materialNames.size();i++)
      {
      if (materialNames[i]==materialName)
        {
        blockIDToMaterialID[blockID]=i;
        materialIDToBlockIDs[i].push_back(blockID);
        found=1;
        }
      }
    if (!found)
      {
      int materialID=static_cast<int>(materialNames.size());
      materialNames.push_back(materialName);
      blockIDToMaterialID[blockID]=materialID;
      materialIDToBlockIDs[materialID]=vtkstd::vector<int>();
      materialIDToBlockIDs[materialID].push_back(blockID);
      }
    
    //handle assembly stuff
    for (i=0;i<localAssemblyNames.size();i++)
      {
      vtkStdString assemblyName=localAssemblyNames[i];
      found=0;
      for (vtkStdString::size_type j=0;j<assemblyNames.size();j++)
        {
        if (assemblyNames[j]==assemblyName){
        blockIDToAssemblyIDs[blockID].push_back(static_cast<int>(j));
        assemblyIDToBlockIDs[static_cast<int>(j)].push_back(blockID);
        found=1;
        }
        }
      if (!found)
        {
        int assemblyID=static_cast<int>(assemblyNames.size());
        assemblyNames.push_back(assemblyName);
        blockIDToAssemblyIDs[blockID]=vtkstd::vector<int>();
        blockIDToAssemblyIDs[blockID].push_back(assemblyID);
        assemblyIDToBlockIDs[assemblyID]=vtkstd::vector<int>();
        assemblyIDToBlockIDs[assemblyID].push_back(blockID);
        }
      }
    }
  
  inline void ResetBlocks() 
  {
    blockNames.erase(blockNames.begin(), blockNames.end());
    blockIds.erase(blockIds.begin(), blockIds.end());
    blockStatus.erase(blockStatus.begin(), blockStatus.end());
    numElementsInBlock.erase(numElementsInBlock.begin(), numElementsInBlock.end());
    partNames.erase(partNames.begin(),partNames.end());
    materialNames.erase(materialNames.begin(),materialNames.end());
    unsigned int i;
    for (i=0;i<materialIDToBlockIDs.size();i++)
      {
      materialIDToBlockIDs[i].erase(materialIDToBlockIDs[i].begin(),
                                    materialIDToBlockIDs[i].end());
      }
    materialIDToBlockIDs.erase(materialIDToBlockIDs.begin(),
                               materialIDToBlockIDs.end());
    for (i=0;i<partIDToBlockIDs.size();i++)
      {
      partIDToBlockIDs[i].erase(partIDToBlockIDs[i].begin(),
                                partIDToBlockIDs[i].end());
      }
    for (i=0;i<assemblyIDToBlockIDs.size();i++)
      {
      assemblyIDToBlockIDs[i].erase(assemblyIDToBlockIDs[i].begin(),
                                    assemblyIDToBlockIDs[i].end());
      }
    for (i=0;i<blockIDToAssemblyIDs.size();i++)
      {
      blockIDToAssemblyIDs[i].erase(blockIDToAssemblyIDs[i].begin(),
                                    blockIDToAssemblyIDs[i].end());
      }
    partIDToBlockIDs.erase(partIDToBlockIDs.begin(),
                           partIDToBlockIDs.end());
    blockIDToPartID.erase(blockIDToPartID.begin(),blockIDToPartID.end());
    blockIDToMaterialID.erase(blockIDToMaterialID.begin(),
                              blockIDToMaterialID.end());
    assemblyNames.erase(assemblyNames.begin(),assemblyNames.end());
    blockIDToAssemblyIDs.erase(
      blockIDToAssemblyIDs.begin(),blockIDToAssemblyIDs.end());
    assemblyIDToBlockIDs.erase(
      assemblyIDToBlockIDs.begin(),assemblyIDToBlockIDs.end());
    sortedOrder.erase(sortedOrder.begin(),sortedOrder.end());
  }
  
  inline void SetDisplayType(int type)
  {
    DisplayType=type;    
  }
  
  inline int GetNumberOfBlocks() 
  { 
    return static_cast<int>(blockNames.size()); 
  }
  inline int GetSortedOrder(int idx) 
  { 
    return sortedOrder[idx];
  }
  inline const char* GetBlockName(int idx) 
  { 
    return blockNames[sortedOrder[idx]].c_str(); 
  }
  inline int GetBlockId(int idx) 
  { 
    return blockIds[sortedOrder[idx]]; 
  }
  inline int GetNumElementsInBlock(int idx) 
  { 
    return numElementsInBlock[sortedOrder[idx]]; 
  }
  inline void SetBlockInitStatus( vtkStdString name, const int status )
    {
      this->blockInitStatus[name] = status;
    }
  inline int GetBlockInitStatus( vtkStdString name )
    {
    vtkstd::map<vtkStdString,int>::iterator i = blockInitStatus.find(name);
    if( i!= blockInitStatus.end() )
      {
      return (*i).second;
      }
    else
      {
      return 1;
      }
    }
  inline void SetBlockStatus(int idx, int flag) 
  { 
    blockStatus[sortedOrder[idx]] = flag; 
  }
  inline void SetBlockStatus(vtkStdString name, int flag) 
  {
    for(int idx=0; idx<this->GetNumberOfBlocks(); ++idx) 
      {
      if ( name == this->blockNames[idx] ) 
        {
        this->blockStatus[idx] = flag;
        return;
        }
      }
  }
  inline void SetUnsortedBlockStatus(int idx, int flag) 
  { 
    if (idx >= 0 && static_cast<int>(blockStatus.size()) > idx)
      {
      blockStatus[idx] = flag; 
      }
  }
  
  inline int GetUnsortedBlockStatus(int idx) 
  { 
    if (idx >= 0 && static_cast<int>(blockStatus.size()) > idx)
      {
      return blockStatus[idx]; 
      }
    return 0;
  }
  inline int GetBlockStatus(int idx) 
  { 
    return blockStatus[sortedOrder[idx]]; 
  }
  inline int GetBlockStatus(vtkStdString name) 
  {
    for(int idx=0; idx<this->GetNumberOfBlocks(); ++idx)
      {
      if ( name == this->blockNames[idx] ) 
        return this->blockStatus[idx];
      }
    return -1;
  }
  
  //Parts and Materials need to act directly on the 
  //blockStatus array. Otherwise the index is put through
  //the sortedOrder array, which gives us garbage since we have a 
  //valid index already
  inline int GetNumberOfParts()
  {
    return static_cast<int>(partNames.size());
  }
  inline const char* GetPartName(int idx)
  {
    return partNames[idx].c_str();
  }
  inline const char* GetPartBlockInfo(int idx)
  {
    _blocks.erase();
    char buffer[80];
    for (unsigned int i=0;i<partIDToBlockIDs[idx].size();i++) 
      {
      sprintf(buffer,"%d, ",blockIds[partIDToBlockIDs[idx][i]]);
      _blocks += buffer;
      }
    
    _blocks.erase(_blocks.size()-2,_blocks.size()-1);
    return _blocks.c_str();
  }
  
  inline int GetPartStatus(int idx)
  {
    //a part is only active if all its blocks are active
    for (unsigned int i=0;i<partIDToBlockIDs[idx].size();i++)
      {
      if (!blockStatus[partIDToBlockIDs[idx][i]])
        {
        return 0;
        }
      }
    return 1;
  }  
  
  inline int GetPartStatus(vtkStdString name)
  {
    for (unsigned int i=0;i<partNames.size();i++)
      {
      if (partNames[i]==name)
        {
        return GetPartStatus(i);
        }
      }
    return -1;
  }
  
  inline void SetPartStatus(int idx, int on) 
  { 
    //update the block status for all the blocks in this part
    for (unsigned int i=0;i<partIDToBlockIDs[idx].size();i++)
      {
      this->blockStatus[partIDToBlockIDs[idx][i]]=on;
      }
  }
  
  inline void SetPartStatus(vtkStdString name, int flag) 
  {
    for(int idx=0; idx<this->GetNumberOfParts(); ++idx) 
      {
      if ( name == this->partNames[idx] ) 
        {
        this->SetPartStatus(idx,flag);
        return;
        }
      }
  }
  
  inline int GetNumberOfMaterials()
  {
    return static_cast<int>(materialNames.size());
  }
  
  inline const char* GetMaterialName(int idx)
  {
    return materialNames[idx].c_str();
  }
  
  inline int GetMaterialStatus(int idx)
  {
    for (unsigned int i=0;i<materialIDToBlockIDs[idx].size();i++)
      {
      if (!blockStatus[materialIDToBlockIDs[idx][i]])
        {
        return 0;
        }
      }
    return 1;
  }  
  
  inline int GetMaterialStatus(vtkStdString name)
  {
    for (unsigned int i=0;i<materialNames.size();i++)
      {
      if (materialNames[i]==name)
        {
        return GetMaterialStatus(i);
        }
      }
    return -1;
  }
  
  inline void SetMaterialStatus(int idx, int on) 
  { 
    //update the block status for all the blocks in this material
    for (unsigned int i=0;i<materialIDToBlockIDs[idx].size();i++)
      {
      this->blockStatus[materialIDToBlockIDs[idx][i]]=on;
      }
  }
  
  inline void SetMaterialStatus(vtkStdString name, int flag) 
  {
    for(int idx=0; idx<this->GetNumberOfMaterials(); ++idx) 
      {
      if ( name == this->materialNames[idx] ) 
          {
          this->SetMaterialStatus(idx,flag);
          return;
          }
      }
  }
  
  //Assembly stuff
  inline int GetNumberOfAssemblies()
  {
    return static_cast<int>(assemblyNames.size());
  }
  
  inline const char* GetAssemblyName(int idx)
  {
    return assemblyNames[idx].c_str();
  }
  
  inline int GetAssemblyStatus(int idx)
  {
    for (unsigned int i=0;i<assemblyIDToBlockIDs[idx].size();i++)
      {
      if (!blockStatus[assemblyIDToBlockIDs[idx][i]])
        {
        return 0;
        }
      }
    return 1;
  }  
  
  inline int GetAssemblyStatus(vtkStdString name)
  {
    for (unsigned int i=0;i<assemblyNames.size();i++)
      {
      if (assemblyNames[i]==name)
        {
        return GetAssemblyStatus(i);
        }
      }
    return -1;
  }
  
  inline void SetAssemblyStatus(int idx, int on) 
  { 
    //update the block status for all the blocks in this material
    for (unsigned int i=0;i<assemblyIDToBlockIDs[idx].size();i++)
      {
      this->blockStatus[assemblyIDToBlockIDs[idx][i]]=on;
      }
  }
  
  inline void SetAssemblyStatus(vtkStdString name, int flag) 
  {
    for(int idx=0; idx<this->GetNumberOfAssemblies(); ++idx) 
      {
      if ( name == this->assemblyNames[idx] ) 
        {
        this->SetAssemblyStatus(idx,flag);
        return;
        }
      }
  }
  
  //performs a mapping from the exodus block ID to the block ID used by 
  //Set/GetUnsortedBlockStatus
  inline int GetBlockIndex(int exodusID)
  {
    for (vtkstd::vector<int>::size_type i=0;i<blockIds.size();i++)
      {
      if (exodusID==blockIds[i])
        {
        return static_cast<int>(i);
        }
      }
    return -1;
  }
  
  
  // This method is important to call when your done setting metadata
  void Finalize();

//BTX
  static int VectorizeArrays(
    int numOriginalNames, char **originalNames, 
    vtkstd::vector<vtkStdString> *newNames, vtkstd::vector<int> *newSize);
//ETX
  
  void SortBlocks()
    {
      int i;
      for (i=static_cast<int>(blockIds.size())-1;i>=0;i--)
        {
        for (int j=1;j<=i;j++)
          {
          if (blockIds[sortedOrder[j-1]]>blockIds[sortedOrder[j]])
            {
            int t=sortedOrder[j-1];
            sortedOrder[j-1]=sortedOrder[j];
            sortedOrder[j]=t;
            }
          }
        }
    }

  void SetArrayStatusInitValue( vtkExodusReader::ArrayType type, int value )
    {
      this->ArrayStatusInitValue[type] = value;
    }

  int GetArrayStatusInitValue( vtkExodusReader::ArrayType type )
    {
      return this->ArrayStatusInitValue[type];
    }

};


class vtkExodusXMLParser: public vtkXMLParser
{
private:
  vtkstd::map<vtkStdString,vtkStdString> MaterialSpecifications;
  vtkstd::map<vtkStdString,vtkStdString> MaterialDescriptions; 

  vtkstd::map<vtkStdString,vtkStdString> PartDescriptions;
  vtkstd::vector<vtkStdString> MaterialNames;
  vtkstd::vector<vtkStdString> BlockNames;
  vtkStdString PartNumber;
  vtkStdString InstanceNumber;
  int ParseMaterials;
  vtkstd::map<int,vtkStdString> BlockIDToPartNumber;
  vtkstd::map<vtkStdString,vtkstd::vector<vtkStdString> > PartNumberToAssemblyNumbers;
  vtkstd::map<vtkStdString,vtkstd::vector<vtkStdString> > PartNumberToAssemblyDescriptions;
  vtkstd::map<vtkStdString,vtkStdString> AssemblyDescriptions;
  vtkstd::vector<vtkStdString> CurrentAssemblyNumbers;
  vtkstd::vector<vtkStdString> CurrentAssemblyDescriptions;

  //mappings for as-tested materials
  vtkstd::map<vtkStdString,vtkStdString> MaterialSpecificationsBlocks; //maps material name to spec
  vtkstd::map<vtkStdString,vtkStdString> MaterialDescriptionsBlocks; //maps material name to desc
  vtkstd::map<int,vtkStdString> BlockIDToMaterial; //maps block id to material

  //hierarchical list mappings
  vtkstd::list<vtkStdString> apbList;
  vtkstd::map<vtkStdString,vtkstd::vector<int> > apbToBlocks;
  vtkstd::map<vtkStdString,int> apbIndents;

public:
  vtkTypeMacro(vtkExodusXMLParser,vtkXMLParser);

  static vtkExodusXMLParser* New();

  // Description:
  // Parse the XML input.
  virtual int Parse(const char* inputString)
    {
      return this->Superclass::Parse(inputString);
    }
  virtual int Parse(const char* inputString, unsigned int length)
    {
      return this->Superclass::Parse(inputString, length);
    }
  virtual int Parse()
    {
      int retVal = this->Superclass::Parse();
      this->PartNumber="";
      this->InstanceNumber="";
      this->ParseMaterials=0;
      return retVal;
    }
  
  virtual vtkStdString GetPartNumber(int block)
  {
    return this->BlockIDToPartNumber[block];
  }
  virtual vtkStdString GetPartDescription(int block)
  {
    return this->PartDescriptions[this->BlockIDToPartNumber[block]];
  }
  virtual vtkStdString GetMaterialDescription(int block)
  {
    return this->MaterialDescriptions[this->BlockIDToPartNumber[block]];
  }
  virtual vtkStdString GetMaterialSpecification(int block)
  {
    return this->MaterialSpecifications[this->BlockIDToPartNumber[block]];
  }
  virtual vtkstd::vector<vtkStdString> GetAssemblyNumbers(int block)
  {  
    return this->PartNumberToAssemblyNumbers[this->BlockIDToPartNumber[block]];
  }
  virtual vtkstd::vector<vtkStdString> GetAssemblyDescriptions(int block)
  {
    return this->PartNumberToAssemblyDescriptions[this->BlockIDToPartNumber[block]];
  }
  
  virtual int GetNumberOfHierarchyEntries()
  {
    return static_cast<int>(this->apbList.size());
  }
  
  virtual vtkStdString GetHierarchyEntry(int num)
  {
    //since it's an STL list, we need to get the correct entry
    vtkstd::list<vtkStdString>::iterator iter=this->apbList.begin();
    for(int i=0;i<num;i++)
      {
      iter++;
      }
    return (*iter);
  }
  
  virtual vtkstd::vector<int> GetBlocksForEntry(int num)
  {
    return this->apbToBlocks[this->GetHierarchyEntry(num)];
  }
  
  virtual vtkstd::vector<int> GetBlocksForEntry(vtkStdString entry)
  {
    return this->apbToBlocks[entry];
  }
  
protected: 
  vtkExodusXMLParser()
  {
    this->FileName = 0;
    this->PartNumber="";
    this->InstanceNumber="";
    this->ParseMaterials=0;
  }
  
  virtual ~vtkExodusXMLParser() 
    {
      this->SetFileName(0);
    }
  
  virtual void StartElement(const char* tname, const char** attrs)
  { 
    const char* name=strrchr(tname,':');
    if (!name)
      {
      name=tname;
      }
    else
      {
      name++;
      }
    // ***********Assembly
    if (strcmp(name,"assembly")==0)
      {
      const char* assemblyNumber=this->GetValue("number",attrs);
      if (assemblyNumber)
        {
        this->CurrentAssemblyNumbers.push_back(vtkStdString(assemblyNumber));
        }
      
      const char* assemblyDescription=this->GetValue("description",attrs);
      if (assemblyDescription)
        {
        this->CurrentAssemblyDescriptions.push_back(vtkStdString(assemblyDescription));
        }
      
      //make the entry for the hierarchical list
      vtkStdString result=vtkStdString("");
      for (vtkstd::vector<int>::size_type i=0;
           i<this->CurrentAssemblyNumbers.size()-1;
           i++)
        {
        result+=vtkStdString("       ");
        }
      
      result+=vtkStdString("Assembly: ")+
        assemblyDescription+vtkStdString(" (")+
        assemblyNumber+vtkStdString(")");
      apbList.push_back(result);
      //record the indent level, used when we add blocks
      apbIndents[result]=static_cast<int>(this->CurrentAssemblyNumbers.size())-1;
      //make the blocks array
      apbToBlocks[result]=vtkstd::vector<int>();
      }
    // ***********Part
    if (strcmp(name,"part")==0)
      {
      const char* instance=this->GetValue("instance",attrs);
      vtkStdString instanceString=vtkStdString("");
      if (instance)
        {
        instanceString=vtkStdString(instance);
        }
      
      const char* partString=this->GetValue("number",attrs);
      if (partString)
        {
        this->PartNumber=vtkStdString(partString)+
          vtkStdString(" Instance: ")+
          instanceString;
        }
      
      const char* partDescString=this->GetValue("description",attrs);
      if (partDescString && this->PartNumber!="")
        {
        this->PartDescriptions[this->PartNumber]=
          partDescString;
        }
      
      //copy the current assemblies to the assemblies list for this part.
      this->PartNumberToAssemblyNumbers[this->PartNumber]=
        vtkstd::vector<vtkStdString>(this->CurrentAssemblyNumbers);
      this->PartNumberToAssemblyDescriptions[this->PartNumber]=
        vtkstd::vector<vtkStdString>(this->CurrentAssemblyDescriptions);
      
      //make the hierarchical display entry
      vtkStdString result=vtkStdString("");
      for (vtkstd::vector<int>::size_type i=0;
           i<this->CurrentAssemblyNumbers.size();
           i++)
        {
        result+=vtkStdString("       ");
        }
      result+=vtkStdString("Part: ")+
        partDescString+vtkStdString(" (")+
        partString+vtkStdString(")")+vtkStdString(" Instance: ")+
        instanceString;
      apbList.push_back(result);
      //record the indent level
      apbIndents[result]=static_cast<int>(this->CurrentAssemblyNumbers.size());
      apbToBlocks[result]=vtkstd::vector<int>();
      }
    // ***********Material-specification
    else if (strcmp(name,"material-specification")==0)
      {
      if (this->PartNumber!="")
        {
        const char * materialDescriptionString=
          GetValue("description",attrs);
        if (materialDescriptionString)
          {
          this->MaterialDescriptions[this->PartNumber]=
            vtkStdString(materialDescriptionString);
          }
        
        const char * materialSpecificationString=
          GetValue("specification",attrs);
        if (materialSpecificationString)
          {
          this->MaterialSpecifications[this->PartNumber]=
            vtkStdString(materialSpecificationString);
          }
        }
      }
    // *********** blocks
    else if (strcmp(name,"blocks")==0)
      {
      const char* instance=this->GetValue("part-instance",attrs);
      vtkStdString instanceString=vtkStdString("");
      if (instance)
        {
        this->InstanceNumber=vtkStdString(instance);
        }
      const char* partString=this->GetValue("part-number",attrs);
      if (partString)
        {
        this->PartNumber=vtkStdString(partString);
        } 
      }
    // *********** block
    else if (strcmp(name,"block")==0)
      {      
      const char* blockString=this->GetValue("id",attrs);
      int id=-1;
      if (blockString)
        {
        id=atoi(blockString);
        }
      if (this->PartNumber!="" && id>=0)
        {
        this->BlockIDToPartNumber[id]=this->PartNumber+
          vtkStdString(" Instance: ")+this->InstanceNumber;
        
        //first insert block entry into apblist
        vtkStdString apbIndexString=this->PartNumber+
          vtkStdString(") Instance: ")+this->InstanceNumber;
        vtkStdString partEntry=findEntry(this->apbList,apbIndexString);
        vtkStdString blockEntry=vtkStdString("");
        if (partEntry!=vtkStdString(""))
          {
          //insert into apbList
          vtkstd::list<vtkStdString>::iterator pos=
            vtkstd::find(this->apbList.begin(),this->apbList.end(),partEntry);
          pos++;
          
          vtkStdString result=vtkStdString("");
          for (int i=0;i<apbIndents[partEntry]+1;i++)
            {
            result+=vtkStdString("       ");
            }
          result+=vtkStdString("Block: ")+vtkStdString(blockString);
          blockEntry=result;
          this->apbList.insert(pos,result);
          apbToBlocks[result]=vtkstd::vector<int>();
          }
        if (partEntry!=vtkStdString("") && blockEntry!=vtkStdString(""))
          {
          //update mapping
          //we know block number, so can get part number to update that.
          //using part number, we can update assembly mappings
          vtkStdString partIndexString=this->PartNumber+
            vtkStdString(" Instance: ")+this->InstanceNumber;
          //we know the part entry
          //add block ID to block entry
          apbToBlocks[blockEntry].push_back(id);
          //add block ID to part
          apbToBlocks[partEntry].push_back(id);
          
          //get the assemblies
          vtkstd::vector<vtkStdString> assemblies=
            this->PartNumberToAssemblyNumbers[partIndexString];
          //add block ID to assemblies
          for (vtkstd::vector<vtkStdString>::size_type j=0;j<assemblies.size();j++)
            {
            vtkStdString assemblyEntry=findEntry(this->apbList,assemblies[j]);
            apbToBlocks[assemblyEntry].push_back(id);
            }
          }
        }
      
      //parse material information if this block tag is part of a
      //material-assignments tag
      if (this->ParseMaterials==1 && id>=0)
        {
        const char* tmaterialName=this->GetValue("material-name",attrs);
        if (tmaterialName)
          {
          this->BlockIDToMaterial[id]=vtkStdString(tmaterialName);
          }
        }
      }
    // ***********Material-assignments
    else if  (strcmp(name,"material-assignments")==0 )
      {
      this->ParseMaterials=1;
      }
    // ***********Material
    else if  (strcmp(name,"material")==0 )
      {
      const char* material=this->GetValue("name",attrs);
      const char* spec=this->GetValue("specification",attrs);
      const char* desc=this->GetValue("description",attrs);
      if (material && spec)
        {
        this->MaterialSpecificationsBlocks[vtkStdString(material)]=vtkStdString(spec);
        }
      if (material && desc)
        {
        this->MaterialDescriptionsBlocks[vtkStdString(material)]=vtkStdString(desc);
        }
      }
  }
  
  //returns the first string that contains sstring
  virtual vtkStdString findEntry(vtkstd::list<vtkStdString> slist, 
                                 vtkStdString sstring){
    for (vtkstd::list<vtkStdString>::iterator i=slist.begin();
         i!=slist.end();
         i++)
      {
      if ((*i).find(sstring)!=vtkStdString::npos)
        {
        return (*i);
        }
      }
    return vtkStdString("");
  }
  
  virtual void EndElement(const char* tname)
  {
    const char* name=strrchr(tname,':');
    if (!name)
      {
      name=tname;
      }
    else
      {
      name++;
      }
    
    if (strcmp(name,"assembly")==0)
      {
      this->CurrentAssemblyNumbers.pop_back();
      this->CurrentAssemblyDescriptions.pop_back();
      }
    else if (strcmp(name,"blocks")==0)
      {
      this->PartNumber="";
      }
    else if (strcmp(name,"material-assignments")==0)
      {
      this->ParseMaterials=0;
      }
  }
  
  virtual int ParsingComplete()
  {
    //if we have as-tested materials, overwrite MaterialDescriptions
    //and MaterialSpecifications
    if (this->BlockIDToMaterial.size()>0)
      {
      this->MaterialSpecifications.clear();
      this->MaterialDescriptions.clear();
      
      for (vtkstd::map<int,vtkStdString>::iterator i=this->BlockIDToPartNumber.begin();i!=this->BlockIDToPartNumber.end();i++)
        {
        int blockID=(*i).first;
        this->MaterialSpecifications[this->BlockIDToPartNumber[blockID]]=
          this->MaterialSpecificationsBlocks[this->BlockIDToMaterial[blockID]];
        this->MaterialDescriptions[this->BlockIDToPartNumber[blockID]]=
          this->MaterialDescriptionsBlocks[this->BlockIDToMaterial[blockID]];
        }
      }

    //if we have no assembly information, we need to generate a bunch
    //of items from the BlockIDToPartNumber array
    if (this->apbList.size()==0)
      {
      for (vtkstd::map<int,vtkStdString>::iterator i=this->BlockIDToPartNumber.begin();i!=this->BlockIDToPartNumber.end();i++)
        {
        int id=(*i).first;
        vtkStdString part=(*i).second;
        vtkStdString partSpec=vtkStdString("");
        vtkStdString instance=vtkStdString("");
        //get part spec and instance from part
        int pos=static_cast<int>(part.find(" Instance: "));
        if (pos!=(int)vtkStdString::npos)
          {
          partSpec.assign(part,0,pos);
          instance.assign(part,pos+11,part.size()-(pos+11));
        }
        
        this->PartDescriptions[part]=vtkStdString("None");
        
        //convert id to a string
        char buffer[20];
        sprintf(buffer,"%d",id);

        //find the Part entry in the apbList
        vtkStdString apbPartEntry=vtkStdString("Part: None (")+partSpec+vtkStdString(") Instance: ")+instance;
        vtkStdString apbBlockEntry=vtkStdString("       ")+vtkStdString("Block: ")+vtkStdString(buffer);
        vtkStdString foundEntry=this->findEntry(this->apbList,apbPartEntry);
        if (foundEntry==vtkStdString(""))
          {
          this->apbList.push_back(apbPartEntry);
          
          this->apbToBlocks[apbPartEntry]=vtkstd::vector<int>();
          
          this->apbToBlocks[apbPartEntry].push_back(id);

          this->AssemblyDescriptions[apbPartEntry]=vtkStdString("None");
          }
        //insert into apbList
        vtkstd::list<vtkStdString>::iterator positer=
          vtkstd::find(this->apbList.begin(),this->apbList.end(),apbPartEntry);
        positer++;
        this->apbList.insert(positer,apbBlockEntry);
        this->apbToBlocks[apbBlockEntry]=vtkstd::vector<int>();
        this->apbToBlocks[apbBlockEntry].push_back(id);        
        
        
        }
      }

    return vtkXMLParser::ParsingComplete();
  }
  
  virtual const char* GetValue(const char* attr,const char** attrs)
  {
    int i;
    for (i=0;attrs[i];i+=2)
      {
      const char* name=strrchr(attrs[i],':');
      if (!name)
        {
        name=attrs[i];
        }
      else
        {
        name++;
        }
      if (strcmp(attr,name)==0)
        {
        return attrs[i+1];
        }
      }
    return NULL;
  }
  
private:
  vtkExodusXMLParser(const vtkExodusXMLParser&); // Not implemented
  void operator=(const vtkExodusXMLParser&); // Not implemented
};


vtkStandardNewMacro(vtkExodusXMLParser);

// This is a cruddy hack... because we need to pass a
// char ** pointer to the exodus function
char** vtkExodusMetadata::GetPointArrayNameBuffer()
{
  return this->pointArrayRawBuffer;
}

char** vtkExodusMetadata::AllocatePointArrayNameBuffer(int num_strings)
{
  int idx;

  if (this->pointArrayRawBuffer)
    {
    for (idx = 0; idx < this->originalNumberOfPointArrays; ++idx)
      {
      delete [] this->pointArrayRawBuffer[idx];
      }
    delete [] this->pointArrayRawBuffer;
    this->pointArrayRawBuffer = NULL;
    this->originalNumberOfPointArrays = 0;
    }

  if (num_strings > 0)
    {
    this->originalNumberOfPointArrays = num_strings;
    this->pointArrayRawBuffer = new char*[num_strings];
  
    for (idx = 0; idx < num_strings; ++idx)
      {
      this->pointArrayRawBuffer[idx] = new char[MAX_STR_LENGTH+1];
      }
    }

  return this->pointArrayRawBuffer;
}

char** vtkExodusMetadata::GetCellArrayNameBuffer()
{
  return this->cellArrayRawBuffer;
}

char** vtkExodusMetadata::AllocateCellArrayNameBuffer(int num_strings)
{
  int idx;

  if (this->cellArrayRawBuffer)
    {
    for (idx = 0; idx < this->originalNumberOfCellArrays; ++idx)
      {
      delete [] this->cellArrayRawBuffer[idx];
      }
    delete [] this->cellArrayRawBuffer;
    this->cellArrayRawBuffer = NULL;
    this->originalNumberOfCellArrays = 0;
    }

  if (num_strings > 0)
    {
    this->originalNumberOfCellArrays = num_strings;
    this->cellArrayRawBuffer = new char*[num_strings];
  
    for (idx = 0; idx < num_strings; ++idx)
      {
      this->cellArrayRawBuffer[idx] = new char[MAX_STR_LENGTH+1];
      }
    }

  return this->cellArrayRawBuffer;
}

int vtkExodusMetadata::VectorizeArrays(
  int numOriginalNames, char **originalNames, 
  vtkstd::vector<vtkStdString> *newNames, vtkstd::vector<int> *newSize)
{
  newNames->erase(newNames->begin(),newNames->end());
  newSize->erase(newSize->begin(),newSize->end());

  // Go through names and combine any vectors
  //
  //   namex )
  //   namey )-> name
  //   namez )

  int idx = 0;
  while (idx < numOriginalNames)
    {
    int len = static_cast<int>(strlen(originalNames[idx]));
    char last_char = toupper(originalNames[idx][len-1]);

    char *newName = vtkExodusReader::StrDupWithNew(originalNames[idx]);
    int nextIdx = idx + 1;

    if (last_char=='X')
      {
      if ((nextIdx < numOriginalNames) &&
          (toupper(originalNames[nextIdx][len-1])=='Y') &&
          !(strncmp(newName, originalNames[nextIdx], len-1)))
        {
        nextIdx++;
        newName[len-1] = '\0';

        if ((nextIdx < numOriginalNames) &&
            (toupper(originalNames[nextIdx][len-1])=='Z') &&
            !(strncmp(newName, originalNames[nextIdx], len-1)))
          {
          nextIdx++;
          }
        }
      }

    int numComponents = nextIdx - idx;

    newNames->push_back(newName);
    delete []newName;
    newSize->push_back(numComponents);

    idx = nextIdx;
    }

  return static_cast<int>(newNames->size());
}

void vtkExodusMetadata::Finalize()
{
  int i;
  vtkstd::map<vtkStdString,int>::iterator iter;

  ////////////////////////////////
  // Point Arrays
  ////////////////////////////////

  int numNewArrays = vtkExodusMetadata::VectorizeArrays(
    originalNumberOfPointArrays, pointArrayRawBuffer,
    &pointArrayNames, &pointArrayComponents);

  pointArrayStatus.erase(pointArrayStatus.begin(), pointArrayStatus.end());

  for (i=0; i<numNewArrays; i++)
    {
    pointArrayStatus.push_back( 
      this->GetArrayStatusInitValue( vtkExodusReader::POINT ) );
    }

  // Check to see if any initial values have been set for this array
  for (iter=pointArrayInitStatus.begin();iter!=pointArrayInitStatus.end();iter++)
    {
    this->SetPointArrayStatus((*iter).first,(*iter).second);
    }
  // Delete the values when we're done
  pointArrayInitStatus.erase(pointArrayInitStatus.begin(),pointArrayInitStatus.end());

  ////////////////////////////////
  // Cell Arrays
  ////////////////////////////////

  numNewArrays = vtkExodusMetadata::VectorizeArrays(
    originalNumberOfCellArrays, cellArrayRawBuffer,
    &cellArrayNames, &cellArrayComponents);

  cellArrayStatus.erase(cellArrayStatus.begin(), cellArrayStatus.end());

  for (i=0; i<numNewArrays; i++)
    {
    cellArrayStatus.push_back( 
      this->GetArrayStatusInitValue( vtkExodusReader::CELL ) );
    }

  // Check to see if any initial values have been set for this array
  for (iter=cellArrayInitStatus.begin();iter!=cellArrayInitStatus.end();iter++)
    {
    this->SetCellArrayStatus((*iter).first,(*iter).second);
    }
  // Delete the values when we're done:
  cellArrayInitStatus.erase(cellArrayInitStatus.begin(),cellArrayInitStatus.end());

  this->SortBlocks();
}


vtkStandardNewMacro(vtkExodusReader);

#ifdef ARRAY_TYPE_NAMES_IN_CXX_FILE
const char *ArrayTypeNames[vtkExodusReader::NUM_ARRAY_TYPES] = { 
  "CELL",
  "POINT",
  "BLOCK",
  "PART",
  "MATERIAL",
  "ASSEMBLY",
  "HIERARCHY"
};

const char *
vtkExodusReader::GetArrayTypeName( vtkExodusReader::ArrayType type )
{
  return ArrayTypeNames[ type ];
}

#else // method in .h file
const char *vtkExodusReader::ArrayTypeNames[NUM_ARRAY_TYPES] = {
  "CELL",
  "POINT",
  "BLOCK",
  "PART",
  "MATERIAL",
  "ASSEMBLY",
  "HIERARCHY"
};
#endif

// Helper function
int vtkExodusReaderFileExist(const char *file_name) 
{
  struct stat fs;
  if (file_name)
    {
    return (stat(file_name, &fs) != -1);
    }
  else
    {
    return 0;
    }
}

// Description:
// Determine if the file can be readed with this reader.
int vtkExodusReader::CanReadFile(const char* fname)
{ 
  // First see if the file exists at all
  if (vtkExodusReaderFileExist(fname) == 0)
    {
    return 0;
    }
    
  // Okay now see if it's really an exodus file
  int returnVal = ex_open( fname, EX_READ, 
                           &(this->ExodusCPUWordSize),
                           &(this->ExodusIOWordSize), 
                           &(this->ExodusVersion));
  // Failure
  if (returnVal < 0)
    {
    return 0;
    }

  // close the file opened for testing.
  ex_close(returnVal);
  // Success
  return 1;
}
//----------------------------------------------------------------------------
// Description:
// Instantiate object with NULL filename.
vtkExodusReader::vtkExodusReader()
{
  this->FileName = NULL;
  this->XMLFileName=NULL;

  this->TimeStep = 0;
  this->ActualTimeStep = 0;
  this->TimeSteps = 0;
  this->GenerateBlockIdCellArray = 1;
  this->GenerateGlobalElementIdArray = 1;
  this->GenerateGlobalNodeIdArray = 1;
  this->ApplyDisplacements = 1;
  this->DisplacementMagnitude = 1;
  
  this->Title = new char[MAX_LINE_LENGTH+1];
  this->Title[0] = (char)'\0';
  this->Dimensionality = 0;
  this->NumberOfUsedElements = 0;
  this->NumberOfElementsInFile = 0;
  this->NumberOfBlocks = 0;
  this->NumberOfUsedNodes = 0;
  this->NumberOfNodesInFile = 0;
  this->NumberOfNodeSets = 0;
  this->NumberOfSideSets = 0;
  this->NumberOfTimeSteps = 0;
  this->ExodusCPUWordSize = 0;
  this->ExodusIOWordSize = 0;
  this->ExodusVersion = 0.0;
  this->CurrentHandle   = -1;
  this->CurrentFileName = NULL;
  this->CurrentXMLFileName = NULL;
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;

  this->DataCache = vtkUnstructuredGrid::New();
  this->MetaData = new vtkExodusMetadata;
  this->RemakeDataCacheFlag = 1;
  this->NewGeometryCount = 0;

  this->CellVarTruthTable = vtkIntArray::New();
  this->PointMap = vtkIntArray::New();
  this->ReversePointMap = vtkIntArray::New();

  this->HasModeShapes = 0;

  this->ExodusModel = NULL;
  this->ExodusModelMetadata = 0;
  this->PackExodusModelOntoOutput = 1;
  this->DisplayType=1;
  this->Parser=NULL;

  this->GlobalElementIdCache=NULL;

  //begin USE_EXO_DSP_FILTERS
  this->DSPFilteringIsEnabled=0;
  this->DSPFilters=NULL;
  this->AddingFilter = vtkDSPFilterDefinition::New();
  //end USE_EXO_DSP_FILTERS

  this->ProgressOffset = 0.0;
  this->ProgressScale = 1.0;
  this->SetNumberOfInputPorts(0);
}


//----------------------------------------------------------------------------
vtkExodusReader::~vtkExodusReader()
{
  this->SetFileName(NULL);
  this->SetXMLFileName(NULL);
  this->SetCurrentXMLFileName(NULL);

  this->SetTitle(NULL);
  this->SetCurrentFileName(NULL);

  this->DataCache->Delete();
  this->DataCache = NULL;

  this->CellVarTruthTable->Delete();
  this->CellVarTruthTable = NULL;

  this->PointMap->Delete();
  this->PointMap = NULL;
  this->ReversePointMap->Delete();
  this->ReversePointMap = NULL;

  if (this->ExodusModel)
    {
    this->ExodusModel->Delete();
    this->ExodusModel = NULL;
    }

  this->SetGlobalElementIdCache(NULL);

  //begin USE_EXO_DSP_FILTERS
  if(this->DSPFilters)
    {
    for(int i=0;i<this->GetNumberOfBlockArrays();i++)
      {
      this->DSPFilters[i]->Delete();
      }
    delete[] this->DSPFilters ;
    this->DSPFilters=NULL;
    }
  //end USE_EXO_DSP_FILTERS


  if (this->Parser)
    {
    this->Parser->Delete();
    this->Parser=NULL;
    }

  delete this->MetaData;

  //begin USE_EXO_DSP_FILTERS
  if (this->AddingFilter)
    {
    this->AddingFilter->Delete();
    }
  //end USE_EXO_DSP_FILTERS

  if (this->TimeSteps)
    {
    delete [] this->TimeSteps;
    this->TimeSteps = 0;
    this->NumberOfTimeSteps = 0;
    }
}

void vtkExodusReader::NewExodusModel()
{
  // These arrays are required by vtkExodusIIWriter

  this->GenerateBlockIdCellArrayOn();
  this->GenerateGlobalElementIdArrayOn();
  this->GenerateGlobalNodeIdArrayOn();

  if (this->ExodusModel)
    {
    this->ExodusModel->Reset();
    return;
    }

  this->ExodusModel = vtkExodusModel::New();
}

void vtkExodusReader::SetGlobalElementIdCache(int *list)
{
  if (this->GlobalElementIdCache)
    {
    delete [] this->GlobalElementIdCache;
    this->GlobalElementIdCache = NULL;
    }

  if (list)
    {
    this->GlobalElementIdCache = list;
    }
}

// Point array info accessors
int vtkExodusReader::GetNumberOfPointArrays() 
{
  return this->MetaData->GetNumberOfPointArrays();
}

const char* vtkExodusReader::GetPointArrayName(int arrayIdx) 
{
  return this->MetaData->GetPointArrayName(arrayIdx);
}

int vtkExodusReader::GetPointArrayNumberOfComponents(int arrayIdx)
{
  return this->MetaData->GetPointArrayComponents(arrayIdx);
}

void vtkExodusReader::SetPointArrayStatus(int index, int flag)
{
  int n = this->MetaData->GetOriginalNumberOfPointArrays();

  if ((index >= 0) && (index < n))
    {
    this->MetaData->SetPointArrayStatus(index, flag);
    this->Modified();
    }
}

void vtkExodusReader::SetPointArrayStatus(const char* name, int flag)
{
  if(this->MetaData->GetNumberOfPointArrays()==0)
    {
    // The point array status is being set before the meta data has been finalized
    // so cache this value for later and use as the initial value
    // If the number of arrays really is zero then this doesn't do any harm.
    this->MetaData->SetPointArrayInitStatus(name, flag);
    }

  this->MetaData->SetPointArrayStatus(name, flag);
  this->Modified();
}

int vtkExodusReader::GetPointArrayStatus(int index)
{
  int n = this->MetaData->GetOriginalNumberOfPointArrays();

  if ((index >= 0) && (index < n))
    {
    return this->MetaData->GetPointArrayStatus(index);
    }

  return 0;
}

int vtkExodusReader::GetPointArrayStatus(const char* name)
{
  return this->MetaData->GetPointArrayStatus(name);
}

// Cell array info accessors
int vtkExodusReader::GetNumberOfCellArrays()
{
  return this->MetaData->GetNumberOfCellArrays();
}

const char* vtkExodusReader::GetCellArrayName(int arrayIdx)
{
  return this->MetaData->GetCellArrayName(arrayIdx);
}

int vtkExodusReader::GetCellArrayNumberOfComponents(int arrayIdx)
{
  return this->MetaData->GetCellArrayComponents(arrayIdx);
}

void vtkExodusReader::SetCellArrayStatus(int index, int flag)
{
  int n = this->MetaData->GetOriginalNumberOfCellArrays();

  if ((index >= 0) && (index < n))
    {
    this->MetaData->SetCellArrayStatus(index, flag);
    this->Modified();
    }
}

void vtkExodusReader::SetCellArrayStatus(const char* name, int flag)
{
  if(this->MetaData->GetNumberOfCellArrays()==0)
    {
    // The cell array status is being set before the meta data has been finalized
    // so cache this value for later and use as the initial value
    // If the number of arrays really is zero then this doesn't do any harm.
    this->MetaData->SetCellArrayInitStatus(name, flag);
    }

  this->MetaData->SetCellArrayStatus(name, flag);
  this->Modified();
}

int vtkExodusReader::GetCellArrayStatus(int index)
{
  int n = this->MetaData->GetOriginalNumberOfCellArrays();

  if ((index >= 0) && (index < n))
    {
    return this->MetaData->GetCellArrayStatus(index);
    }

  return 0;
}

int vtkExodusReader::GetCellArrayStatus(const char* name)
{
  return this->MetaData->GetCellArrayStatus(name);
}

// Block info accessors
int vtkExodusReader::GetNumberOfBlockArrays()
{
  return this->MetaData->GetNumberOfBlocks();
}

const char* vtkExodusReader::GetBlockArrayName(int arrayIdx)
{
  return this->MetaData->GetBlockName(arrayIdx);
}

void vtkExodusReader::SetBlockArrayStatus(int index, int flag)
{
  // Only modify if we are 'out of sync'
  //cout << "called SetBlockArrayStatus " << index << endl;
  if (this->MetaData->GetBlockStatus(index) != flag)
    {
    this->MetaData->SetBlockStatus(index, flag);
    
    // Because which blocks are on/off affects the
    // geometry we need to remake the mesh cache
    this->RemakeDataCacheFlag = 1;
    this->Modified();
    }
}

void vtkExodusReader::SetBlockArrayStatus(const char* name, int flag)
{
  if(this->MetaData->GetNumberOfBlocks()==0)
    {
    // The value is being set before the metadata has been finalized
    // So store for later and use as the initial value if needed
    this->MetaData->SetBlockInitStatus(name,flag);
    }
  // Only modify if we are 'out of sync'
  if (this->MetaData->GetBlockStatus(name) != flag)
    {
    this->MetaData->SetBlockStatus(name, flag);

    // Because which blocks are on/off affects the
    // geometry we need to remake the mesh cache
    this->RemakeDataCacheFlag = 1;
    this->Modified();
    }
}

int vtkExodusReader::GetBlockArrayStatus(int index)
{
  return this->MetaData->GetBlockStatus(index);
}

int vtkExodusReader::GetBlockArrayStatus(const char* name)
{
  return this->MetaData->GetBlockStatus(name);
}

int vtkExodusReader::GetNumberOfElementsInBlock(int block_idx)
{
  return this->MetaData->GetNumElementsInBlock(block_idx);
}

//----------------------------
// Node/Side set accessors
//----------------------------
void vtkExodusReader::SetNodeSetArrayStatus(int index, int flag)
{
  // Only modify if we are 'out of sync'
  if (this->MetaData->GetNodeSetStatus(index) != flag)
    {
    this->MetaData->SetNodeSetStatus(index, flag);

    // Because which node sets are on/off affects the
    // geometry we need to remake the mesh cache
    this->RemakeDataCacheFlag = 1;
    this->Modified();
    }
}
void vtkExodusReader::SetNodeSetArrayStatus(const char* name, int flag)
{
  if(this->MetaData->GetNumberOfNodeSets()==0)
    {
    // The value is being set before the metadata has been finalized
    // So store for later and use as the initial value if needed
    this->MetaData->SetNodeSetInitStatus(name,flag);
    }
  // Only modify if we are 'out of sync'
  if (this->MetaData->GetNodeSetStatus(name) != flag)
    {
    this->MetaData->SetNodeSetStatus(name, flag);

    // Because which node sets are on/off affects the
    // geometry we need to remake the mesh cache
    this->RemakeDataCacheFlag = 1;
    this->Modified();
    }
}
int vtkExodusReader::GetNodeSetArrayStatus(int index )
{
  return this->MetaData->GetNodeSetStatus( index );
}
int vtkExodusReader::GetNodeSetArrayStatus(const char* name )
{
  return this->MetaData->GetNodeSetStatus( name );
}
const char* vtkExodusReader::GetNodeSetArrayName(int index)
{
  return this->MetaData->GetNodeSetName(index);
}

void vtkExodusReader::SetSideSetArrayStatus(int index, int flag)
{
  // Only modify if we are 'out of sync'
  if (this->MetaData->GetSideSetStatus(index) != flag)
    {
    this->MetaData->SetSideSetStatus(index, flag);

    // Because which side sets are on/off affects the
    // geometry we need to remake the mesh cache
    this->RemakeDataCacheFlag = 1;
    this->Modified();
    }
}
void vtkExodusReader::SetSideSetArrayStatus(const char* name, int flag)
{
  if(this->MetaData->GetNumberOfSideSets()==0)
    {
    // The value is being set before the metadata has been finalized
    // So store for later and use as the initial value if needed
    this->MetaData->SetSideSetInitStatus(name,flag);
    }
  // Only modify if we are 'out of sync'
  if (this->MetaData->GetSideSetStatus(name) != flag)
    {
    this->MetaData->SetSideSetStatus(name, flag);

    // Because which side sets are on/off affects the
    // geometry we need to remake the mesh cache
    this->RemakeDataCacheFlag = 1;
    this->Modified();
    }
}
int vtkExodusReader::GetSideSetArrayStatus(int index)
{
  return this->MetaData->GetSideSetStatus( index );
}
int vtkExodusReader::GetSideSetArrayStatus(const char* name)
{
  return this->MetaData->GetSideSetStatus( name );
}
const char* vtkExodusReader::GetSideSetArrayName(int index)
{
  return this->MetaData->GetSideSetName(index);
}

//----------------------------
// End Node/Side set accessors
//----------------------------

// Part info accessors
int vtkExodusReader::GetNumberOfPartArrays()
{
  return this->MetaData->GetNumberOfParts();
}

const char* vtkExodusReader::GetPartArrayName(int arrayIdx)
{
  return this->MetaData->GetPartName(arrayIdx);
}

const char* vtkExodusReader::GetPartBlockInfo(int arrayIdx)
{
  return this->MetaData->GetPartBlockInfo(arrayIdx);
}

void vtkExodusReader::SetPartArrayStatus(int index, int flag)
{
  // Only modify if we are 'out of sync'
  if (this->MetaData->GetPartStatus(index) != flag)
    {
    this->MetaData->SetPartStatus(index, flag);
    
    // Because which parts are on/off affects the
    // geometry we need to remake the mesh cache
    this->RemakeDataCacheFlag = 1;
    this->Modified();
    }
}

void vtkExodusReader::SetPartArrayStatus(const char* name, int flag)
{
  // Only modify if we are 'out of sync'
  if (this->MetaData->GetPartStatus(name) != flag)
    {
    this->MetaData->SetPartStatus(name, flag);

    // Because which parts are on/off affects the
    // geometry we need to remake the mesh cache
    this->RemakeDataCacheFlag = 1;
    this->Modified();
    }
}

int vtkExodusReader::GetPartArrayStatus(int index)
{
  return this->MetaData->GetPartStatus(index);
}

int vtkExodusReader::GetPartArrayStatus(const char* name)
{
  return this->MetaData->GetPartStatus(name);
}

// Material info accessors
int vtkExodusReader::GetNumberOfMaterialArrays()
{
  //cout << "reader asked for nummat " << this->MetaData->GetNumberOfMaterials() << endl;
  return this->MetaData->GetNumberOfMaterials();
}

const char* vtkExodusReader::GetMaterialArrayName(int arrayIdx)
{
  return this->MetaData->GetMaterialName(arrayIdx);
}

void vtkExodusReader::SetMaterialArrayStatus(int index, int flag)
{
  // Only modify if we are 'out of sync'
  if (this->MetaData->GetMaterialStatus(index) != flag)
    {
    this->MetaData->SetMaterialStatus(index, flag);
    
    // Because which materials are on/off affects the
    // geometry we need to remake the mesh cache
    this->RemakeDataCacheFlag = 1;
    this->Modified();
    }
}

void vtkExodusReader::SetMaterialArrayStatus(const char* name, int flag)
{
  // Only modify if we are 'out of sync'
  if (this->MetaData->GetMaterialStatus(name) != flag)
    {
    this->MetaData->SetMaterialStatus(name, flag);

    // Because which materials are on/off affects the
    // geometry we need to remake the mesh cache
    this->RemakeDataCacheFlag = 1;
    this->Modified();
    }
}

int vtkExodusReader::GetMaterialArrayStatus(int index)
{
  return this->MetaData->GetMaterialStatus(index);
}

int vtkExodusReader::GetMaterialArrayStatus(const char* name)
{
  return this->MetaData->GetMaterialStatus(name);
}
//end of Material Accessors

// Assembly info accessors
int vtkExodusReader::GetNumberOfAssemblyArrays()
{
  return this->MetaData->GetNumberOfAssemblies();
}

const char* vtkExodusReader::GetAssemblyArrayName(int arrayIdx)
{
  return this->MetaData->GetAssemblyName(arrayIdx);
}

int vtkExodusReader::GetAssemblyArrayID( char const *name )
{
  int numArrays = this->GetNumberOfAssemblyArrays();
  for ( int i=0;i<numArrays;i++ )
    {
    if ( strcmp( name, this->GetAssemblyArrayName( i ) ) == 0 )
      {
      return i;
      }
    }
  return -1;
}

void vtkExodusReader::SetAssemblyArrayStatus(int index, int flag)
{
  // Only modify if we are 'out of sync'
  if (this->MetaData->GetAssemblyStatus(index) != flag)
    {
    this->MetaData->SetAssemblyStatus(index, flag);
    
    // Because which materials are on/off affects the
    // geometry we need to remake the mesh cache
    this->RemakeDataCacheFlag = 1;
    this->Modified();
    }
}

void vtkExodusReader::SetAssemblyArrayStatus(const char* name, int flag)
{
  // Only modify if we are 'out of sync'
  if (this->MetaData->GetAssemblyStatus(name) != flag)
    {
    this->MetaData->SetAssemblyStatus(name, flag);

    // Because which materials are on/off affects the
    // geometry we need to remake the mesh cache
    this->RemakeDataCacheFlag = 1;
    this->Modified();
    }
}

int vtkExodusReader::GetAssemblyArrayStatus(int index)
{
  return this->MetaData->GetAssemblyStatus(index);
}

int vtkExodusReader::GetAssemblyArrayStatus(const char* name)
{
  return this->MetaData->GetAssemblyStatus(name);
}

//end of Assembly Accessor

// Hierarchy Entry info accessors
int vtkExodusReader::GetNumberOfHierarchyArrays()
{
  if (this->Parser)
    {
    return this->Parser->GetNumberOfHierarchyEntries();
    }
  return 0;
}

const char* vtkExodusReader::GetHierarchyArrayName(int arrayIdx)
{
  if (this->Parser)
    {
    //MEMORY LEAK - without copying the result, the list does not appear on SGI's
    char* result=new char[512];
    sprintf(result,"%s",this->Parser->GetHierarchyEntry(arrayIdx).c_str());
    return result;
    //return this->Parser->GetHierarchyEntry(arrayIdx).c_str();
    }
  return "Should not see this";
}

void vtkExodusReader::SetHierarchyArrayStatus(int index, int flag)
{
  // Only modify if we are 'out of sync'
  //if (this->GetHierarchyArrayStatus(index) != flag)
  // {
  if (this->Parser)
    {
    vtkstd::vector<int> blocks=this->Parser->GetBlocksForEntry(index);
    for (vtkstd::vector<int>::size_type i=0;i<blocks.size();i++)
      {
      //cout << "turning block " << blocks[i] << " " << flag << endl;
      this->MetaData->SetUnsortedBlockStatus
        (this->MetaData->GetBlockIndex(blocks[i]),flag);
      }
    
    // Because which materials are on/off affects the
    // geometry we need to remake the mesh cache
    this->RemakeDataCacheFlag = 1;
    this->Modified();
    }
}

void vtkExodusReader::SetHierarchyArrayStatus(const char* name, int flag)
{
  // Only modify if we are 'out of sync'
  //if (this->GetHierarchyArrayStatus(name) != flag)
  //{
  if (this->Parser)
    {
    vtkstd::vector<int> blocks=this->Parser->GetBlocksForEntry
      (vtkStdString(name));
    for (vtkstd::vector<int>::size_type i=0;i<blocks.size();i++)
      {
      //cout << "turning block " << blocks[i] << " " << flag << endl;
      this->MetaData->SetUnsortedBlockStatus
        (this->MetaData->GetBlockIndex(blocks[i]),flag);
      }
    
    // Because which materials are on/off affects the
    // geometry we need to remake the mesh cache
    this->RemakeDataCacheFlag = 1;
    this->Modified();
    }
}

int vtkExodusReader::GetHierarchyArrayStatus(int index)
{
  if (this->Parser)
    {
    vtkstd::vector<int> blocks=this->Parser->GetBlocksForEntry(index);
    for (vtkstd::vector<int>::size_type i=0;i<blocks.size();i++)
      {
      if (this->MetaData->GetUnsortedBlockStatus
          (this->MetaData->GetBlockIndex(blocks[i]))==0)
        return 0;
      }
    }
  return 1;
}

int vtkExodusReader::GetHierarchyArrayStatus(const char* name)
{
  if (this->Parser)
    {
    vtkstd::vector<int> blocks=this->Parser->GetBlocksForEntry(name);
    for (vtkstd::vector<int>::size_type i=0;i<blocks.size();i++)
      {
      if (this->MetaData->GetUnsortedBlockStatus
          (this->MetaData->GetBlockIndex(blocks[i]))==0)
        return 0;
      }
    }
  return 1;
}

//end of Hierarchy Entry Accessors

int vtkExodusReader::GetBlockId(int block_idx)
{
  return this->MetaData->GetBlockId(block_idx);
}

void vtkExodusReader::SetDisplayType(int type){
  if (this->MetaData)
    {
    this->MetaData->SetDisplayType(type);
    }
  this->DisplayType=type;
}

//----------------------------------------------------------------------------
int vtkExodusReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  int error;
  float fdum;
  int num_node_vars, num_ele_vars;
  int i;
  int num_elem_in_block;
  int num_nodes_per_elem;
  int num_attr;
  char *cdum = NULL;
  char elem_type[MAX_STR_LENGTH+1];

  // We can skip ExecuteInformation if all of these are true:
  //    The filename has not changed
  //    The XML filename has not changed
  //    The request for metadata has not changed

  int newXMLFile = 1;
  int newMetaData = 1;

  int newFile = 
    (vtkExodusReader::StringsEqual(this->FileName, this->CurrentFileName) == 0);

  if (!newFile)
    {
    newXMLFile = 
      (vtkExodusReader::StringsEqual(this->XMLFileName, this->CurrentXMLFileName) == 0);

    newMetaData = ((this->ExodusModelMetadata && (this->ExodusModel == NULL)) ||
                   (!this->ExodusModelMetadata && this->ExodusModel));
    }

  if ( !newFile && !newXMLFile && !newMetaData)
    {
    // always set the time step values even if we short circuit
    if (!this->HasModeShapes)
      {
      if (this->NumberOfTimeSteps)
        {
        vtkInformation* outInfo = outputVector->GetInformationObject(0);
        outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), 
                     this->TimeSteps, 
                     this->NumberOfTimeSteps);
        double timeRange[2];
        timeRange[0] = this->TimeSteps[0];
        timeRange[1] = this->TimeSteps[this->NumberOfTimeSteps-1];
        outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), 
                     timeRange, 2);
        }
      }
    else
      {
      vtkInformation* outInfo = outputVector->GetInformationObject(0);
      outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      double timeRange[2];
      timeRange[0] = 0;
      timeRange[1] = 1;
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
                   timeRange, 2);
      }
    return 1;
    }
  
  if (newXMLFile)
    {
    // Clean up any old xml parsers
    if (this->Parser)
      {
      this->Parser->Delete();
      this->Parser=NULL;
      }
  
     // If the xml file does not exist, try try again
    if (!vtkExodusReaderFileExist(this->XMLFileName)) 
      {
      // Okay try to create an xml file using exodus file as basename
      char tempName[512];
      strcpy(tempName,this->FileName);
      char* fpt=strrchr(tempName,'.');
      if (fpt) strncpy(fpt,".xml\0",5);
      // Does the xml file exist?
      int XMLfound=0;
      if (vtkExodusReaderFileExist(tempName)) 
        {
        SetXMLFileName(tempName);
        XMLfound=1;
        } 
      if (!XMLfound) 
        {
        //try .dart
        fpt=strrchr(tempName,'.');
        if (fpt) strncpy(fpt,".dart\0",6);
        if (vtkExodusReaderFileExist(tempName)) 
          {
          SetXMLFileName(tempName);
          XMLfound=1;
          }
        }
      if (!XMLfound)
        {
        //try artifact.dta
#ifdef _WIN32
        fpt=strrchr(tempName,'\\');
        if (fpt) strncpy(fpt,"\\artifact.dta\0",14);
#else
        fpt=strrchr(tempName,'/');
        if (fpt) strncpy(fpt,"/artifact.dta\0",14);
#endif
        if (vtkExodusReaderFileExist(tempName)) 
          {
          SetXMLFileName(tempName);
          XMLfound=1;
          }
        }
      if (!XMLfound)
        {
        SetXMLFileName(NULL);
        }
      }
    
    // Okay if we have a valid file create a parser
    if (this->XMLFileName)
      {
      //cout << "parsing: " << this->XMLFileName << endl;
      this->Parser = vtkExodusXMLParser::New();
      this->Parser->SetFileName(this->XMLFileName);
      this->Parser->Parse();
      }
  
    // The filename is different so we need to open the file
    // and remake the data cache and get all the metadata
    this->RemakeDataCacheFlag = 1;
    newFile = 1;
    this->SetCurrentXMLFileName(this->XMLFileName);
    }

  if ( this->OpenCurrentFile() != 1 )
    {
    return 0;
    }

  if (newMetaData)
    {
    if (this->ExodusModelMetadata)
      {
      // Write the global metadata now.  This is all the information 
      // which does not depend on which cells, timestep, or field data
      // we are going to read in.
  
      this->NewExodusModel();
  
      this->ExodusModel->SetGlobalInformation( this->CurrentHandle, 
                                               this->ExodusCPUWordSize );
      }
    else
      {
      if (this->ExodusModel)
        {
        this->ExodusModel->Delete();
        this->ExodusModel = NULL;
        }
      }
    }

  // **KEN** Why not have vtkExodusMetadata do all of this instead.
  // Encapsulate the functionality of reading it along with the data.
  // That's the whole idea behind object oriented programming, right?
  
  if (newFile)
    {
    this->SetGlobalElementIdCache(NULL);

    // Get metadata
    error = ex_get_init(this->CurrentHandle, this->Title, &this->Dimensionality, 
                        &this->NumberOfNodesInFile, &this->NumberOfElementsInFile, 
                        &this->NumberOfBlocks, &this->NumberOfNodeSets, &this->NumberOfSideSets);
    if (error < 0)
      {
      vtkErrorMacro("Error: " << error << " calling ex_get_init " << this->FileName);
      this->CloseCurrentFile();
      return 0;
      }
  
    // Read the number of time steps available.
    error = ex_inquire(this->CurrentHandle, 
                       EX_INQ_TIME, &this->NumberOfTimeSteps, &fdum,cdum);
    if (error < 0)
      {
      vtkErrorMacro("Error: " << error << " calling ex_inquire " << this->FileName);
      this->CloseCurrentFile();
      return 0;
      }

    this->TimeStepRange[0] = 0;
    this->TimeStepRange[1] = this->NumberOfTimeSteps-1;

    this->GetAllTimes(outputVector);
     
    // Read element block paramemters.
    this->MetaData->ResetBlocks();
    int *ids = new int[this->NumberOfBlocks];
    char block_name_buffer[80];
    int status = 1;
    ex_get_elem_blk_ids (this->CurrentHandle, ids);
  
    for (i = 0; i < this->NumberOfBlocks; ++i)
      {
      error = ex_get_elem_block (this->CurrentHandle, ids[i], elem_type, 
                                 &(num_elem_in_block),
                                 &(num_nodes_per_elem), &(num_attr));
      if (error < 0)
        {
        vtkErrorMacro(
          "Error: " << error << " calling ex_get_elem_blk_ids " << this->FileName);
        this->CloseCurrentFile();
        delete [] ids;
        return 0;
        }
      
      // Check for empty block
      if (!strcmp(elem_type,"NULL"))
        {
        strcpy(elem_type,"empty");
        }
  
      sprintf(block_name_buffer,"Block: %d (%s)",ids[i],elem_type);
  
      // Get whether an initial state for this block has been specified
      // If none is found, the default value is 'on'
      status = this->MetaData->GetBlockInitStatus(block_name_buffer);

      if (this->Parser && this->Parser->GetPartDescription(ids[i])!="")
        {
        //construct assembly names from number and description arrays
        vtkstd::vector<vtkStdString> assemblyNumbers=
          this->Parser->GetAssemblyNumbers(ids[i]);
        vtkstd::vector<vtkStdString> assemblyDescriptions=
          this->Parser->GetAssemblyDescriptions(ids[i]);
        
        vtkstd::vector<vtkStdString> assemblyNames;
        for (vtkstd::vector<int>::size_type j=0;j<assemblyNumbers.size();j++)
          {
          assemblyNames.push_back(assemblyDescriptions[j]+vtkStdString(" (")+
                                  assemblyNumbers[j]+vtkStdString(")"));
          }
        sprintf(block_name_buffer,"Block: %d (%s) %s",ids[i],
                this->Parser->GetPartDescription(ids[i]).c_str(),
                this->Parser->GetPartNumber(ids[i]).c_str());
        this->MetaData->AddBlock(block_name_buffer,
                                 this->Parser->GetPartDescription(ids[i])+" ("+
                                 this->Parser->GetMaterialDescription(ids[i])+")"+" : "+
                                 this->Parser->GetPartNumber(ids[i]), 
                                 this->Parser->GetMaterialDescription(ids[i])+" : "+
                                 this->Parser->GetMaterialSpecification(ids[i]), 
                                 assemblyNames,
                                 ids[i], num_elem_in_block, status);
        }
      else
        {
        vtkstd::vector<vtkStdString> assemblyNames;
        assemblyNames.push_back(vtkStdString("Default Assembly"));
        this->MetaData->AddBlock(block_name_buffer,"Default Part", 
                                 "Default Material", assemblyNames,ids[i], 
                                 num_elem_in_block, status); 
        }
  
      }
    delete [] ids;
  
    // Read the number of node arrays
    error = ex_get_var_param(this->CurrentHandle, "n", &(num_node_vars));
    if (error < 0)
      {
      vtkErrorMacro(
        "Error: " << error << " calling ex_get_var_param " << this->FileName);
      this->CloseCurrentFile();
      return 0;
      }
      
    // Get node array information
    if (num_node_vars > 0)
      {
      error = ex_get_var_names(this->CurrentHandle, "n", num_node_vars,
                               MetaData->AllocatePointArrayNameBuffer(num_node_vars));
      if (error < 0)
        {
        vtkErrorMacro(
          "Error: " << error << " reading point array names " << this->FileName);
        this->CloseCurrentFile();
        return 0;
        }
  
      this->RemoveBeginningAndTrailingSpaces(
        MetaData->GetPointArrayNameBuffer(),num_node_vars);
      }
    
    // Read the number of cell arrays 
    error = ex_get_var_param(this->CurrentHandle, "e", &(num_ele_vars));
    if (error < 0)
      {
      vtkErrorMacro(
        "Error: " << error << " calling ex_get_var_param " << this->FileName);
      this->CloseCurrentFile();
      return 0;
      }
  
    // Get cell array information
    if (num_ele_vars > 0)
      {
      // Not all cell variables exist over all element blocks.  A "truth table"
      // will say whether a variable is defined for a certain block.

      this->CellVarTruthTable->Resize(num_ele_vars*this->NumberOfBlocks);
      int *ptr = CellVarTruthTable->GetPointer(0);
      ex_get_elem_var_tab(this->CurrentHandle, this->NumberOfBlocks, 
                          num_ele_vars, ptr);
      error = ex_get_var_names (this->CurrentHandle, "e", num_ele_vars,
                                MetaData->AllocateCellArrayNameBuffer(num_ele_vars));
      if (error < 0)
        {
        vtkErrorMacro("Error: " << error << " calling ex_get_var_names " << this->FileName);
        this->CloseCurrentFile();
        return 0;
        }
  
      this->RemoveBeginningAndTrailingSpaces(
        MetaData->GetCellArrayNameBuffer(),num_ele_vars);
      }
  
    // Read Node Set and Side Set metadata
    this->ReadNodeSetMetadata();
    this->ReadSideSetMetadata();
    }

  // Close the exodus file
  this->CloseCurrentFile();

  if (newFile)
    {
    // Call finalize on the array metadata. This is important
    // otherwise does not work at all

    MetaData->Finalize();
    }

  return 1;
}


//------------------------------------------------------------------------
// Read Node Set and Side Set MetaData for future use
//  Set ids
//  Set size
//  number of distribution factors in each set
//------------------------------------------------------------------------
void vtkExodusReader::ReadNodeSetMetadata()
{
  // Stop if there are no Node Sets
  if ( this->NumberOfNodeSets <= 0 )
    {
    return;
    }
  // allocate memory for node set ids
  vtkstd::vector<int> nodeSetId( this->NumberOfNodeSets, -1 );
  int size = 0;
  int dist = 0;
  // read node set ids
  int error = ex_get_node_set_ids( this->CurrentHandle, &nodeSetId[0] );
  if (error < 0)
    {
    vtkErrorMacro("Error: " << error << " calling ex_get_node_set_ids " << this->FileName);
    }
  // read meta data for each node set
  int i = 0;
  char bufferName[80];
  int status = 0;
  for( i = 0; i < this->NumberOfNodeSets; i++ )
    {
    error = ex_get_node_set_param( this->CurrentHandle, nodeSetId[i], &size, &dist );
    if (error < 0)
      {
      vtkErrorMacro("Error: " << error << " calling ex_get_node_set_param " << this->FileName);
      }
    // Add a node set's metadata to vtkExodusMetaData
    // Store it's Exodus Id, size, and number of distribution factors
    sprintf(bufferName, "NodeSet %d",nodeSetId[i]);
    // Get whether an initial state has been given for this node set
    // If none is found, the default is "off"
    status = this->MetaData->GetNodeSetInitStatus(bufferName);
    this->MetaData->AddNodeSet( bufferName, nodeSetId[i], size, dist, status );
    }
}
void vtkExodusReader::ReadSideSetMetadata()
{
  // Stop if there are no Side Sets
  if ( this->NumberOfSideSets <= 0 )
    {
    return;
    }
  // allocate memory for side set ids
  vtkstd::vector<int> sideSetId( this->NumberOfSideSets, -1 );
  int size = 0;
  int dist = 0;
  // read side set ids
  int error = ex_get_side_set_ids( this->CurrentHandle, &sideSetId[0] );
  if (error < 0)
    {
    vtkErrorMacro(<< "Error: " << error 
                  << " calling ex_get_side_set_ids " << this->FileName);
    }
  // read meta data for each side set
  int i = 0;
  char bufferName[80];
  int status = 0;
  for( i = 0; i < this->NumberOfSideSets; i++ )
    {
    error = ex_get_side_set_param( this->CurrentHandle, sideSetId[i], &size, &dist );
    if (error < 0)
      {
      vtkErrorMacro(<< "Error: " << error 
                    << " calling ex_get_side_set_param " << this->FileName);
      }
    // Add a side set's metadate to vtkExodusMetaData
    // Store it's Exodus Id, size, and number of distribution factors
    sprintf(bufferName, "SideSet %d",sideSetId[i]);
    status = this->MetaData->GetSideSetInitStatus(bufferName);
    // Get whether an initial state has been given for this side set
    // If none is found, the default is "off"
    this->MetaData->AddSideSet( bufferName, sideSetId[i], size, dist, status );
    }
}

//----------------------------------------------------------------------------
int vtkExodusReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  if ( this->OpenCurrentFile() != 1 )
    {
    vtkWarningMacro(<< "Can't open file");
    return 0;
    }
  this->ActualTimeStep = this->TimeStep;

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  // get the ouptut
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int tsLength =
    outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  double* steps =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    
  // Check if a particular time was requested.
  if(outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    // Get the requested time step. We only supprt requests of a single time
    // step in this reader right now
    double *requestedTimeSteps = 
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    this->TimeValue = requestedTimeSteps[0];

    if (!this->HasModeShapes)
      {
      //find the timestep with the closest value to the requested time
      //value
      int cnt = 0;
      int closestStep=0;
      double minDist=-1;
      for (cnt=0;cnt<tsLength-1;cnt++)
        {
        double tdist=(steps[cnt]-this->TimeValue>this->TimeValue-steps[cnt])?steps[cnt]-this->TimeValue:this->TimeValue-steps[cnt];
        if (minDist<0 || tdist<minDist)
          {
          minDist=tdist;
          closestStep=cnt;
          }
        }
      this->ActualTimeStep=closestStep;
       // find the first time value larger than requested time value
      // this logic could be improved
      //while (cnt < tsLength-1 && steps[cnt] < this->TimeValue)
      //  {
      //  
      //  cnt++;
      //  }
      //this->ActualTimeStep = cnt;
      }
    }

  // Force TimeStep into the "known good" range. Although this
  // could be accomplished inside SetTimeStep(),
  // - we'd rather not override a VTK macro-defined method
  //   because the macro might change
  // - it might be good to allow out-of-range values in
  //   places
  if ( this->ActualTimeStep < this->TimeStepRange[0] )
    {
    this->ActualTimeStep = this->TimeStepRange[0];
    }
  else if ( this->ActualTimeStep > this->TimeStepRange[1] )
    {
    this->ActualTimeStep = this->TimeStepRange[1];
    }

  // Okay because they may have changed which blocks are on/off
  // we need to recompute the actual number of elements
  int actualElements=0;
  for (int i=0; i < this->MetaData->GetNumberOfBlocks(); ++i)
    {
    // Do we read this block?
    if (this->MetaData->GetBlockStatus(i)==1)
      actualElements += this->MetaData->GetNumElementsInBlock(i);
    }
#if DEBUG
  vtkWarningMacro(<< "NumElements: " << NumberOfElements 
                  << " actualElements: " << actualElements); 
#endif
  this->NumberOfUsedElements = actualElements;
  
  
  // In general the geometry of an exodus file remains the same
  // So often you can just read in new field array
  // The 'RemakeDataCacheFlag' is set if you need to read
  // in the geometry again for some reason (eg new file or a 
  // change which blocks get read in etc)
  if ( RemakeDataCacheFlag ) 
    { 
    this->ReadGeometry(this->CurrentHandle, output);

    this->NewGeometryCount++;

    // **KEN** Would it make sense to relase the DataCache data BEFORE
    // reading the new geometry?  That might reduce the amount of memory
    // needed at one time.

    // Make a shallowcopy to the my DataCache
    this->DataCache->ReleaseData(); // FIXME Do I need this?
    this->DataCache->ShallowCopy(output);

    // Okay I don't need to do this again until someone
    // says 'Hey you need to remake this mesh data!'
    this->RemakeDataCacheFlag = 0;

#if DEBUG
    vtkWarningMacro("Remade data cache"); 
#endif
    } 
  else 
    {
    // Just copy (shallow) the points and cells from the DataCache. 
    // The points and cell information is always the same for exodus
    // Also copy the generated arrays which also stay the same
    output->ShallowCopy(this->DataCache);
    }

  // Save the time value in the output data information.
  if (steps)
    {
    if (!this->HasModeShapes)
      {
      output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEPS(),
                                    steps+this->ActualTimeStep, 1);
      }
    else
      {
      output->GetInformation()->Remove(vtkDataObject::DATA_TIME_STEPS());
      }
    }
  
  // Read in the arrays.
  // If we are in the execution method then either the file, the time 
  // step or array selections changed. In all cases we have to reload arrays.
  this->ReadArrays(this->CurrentHandle, output);

  //begin USE_EXO_DSP_FILTERS
  this->GetDSPOutputArrays(this->CurrentHandle, output);
  //end USE_EXO_DSP_FILTERS

  // If the user wants the apply displacements then add here
  // TODO: This is recalled everytime a user changes arrays
  // look into avoiding this if possible
  if (ApplyDisplacements)
    {
    AddDisplacements(output);
    }

  // Generated arrays include block id, global node id, global element id
  this->GenerateExtraArrays(output);

  // This just makes sure the arrays are the same size as the number
  // of nodes or cell
  output->CheckAttributes();

  // We may have some mem that can be condensed
  output->Squeeze();
 
  if (this->ExodusModel)
    {
    int rc = this->ExodusModel->SetLocalInformation(
      output, 
      this->CurrentHandle, this->ActualTimeStep, 
      this->NewGeometryCount, this->ExodusIOWordSize);

    if (rc)
      {
      vtkErrorMacro(<< "Can't create the local model information");
      }

    if (this->PackExodusModelOntoOutput)
      {
      // The metadata will be written to field arrays and added
      // to the output.

      this->ExodusModel->GetModelMetadata()->Pack(output);
      }
    }

  // Close the exodus file
  this->CloseCurrentFile();

  return 1;
}


//----------------------------------------------------------------------------
void vtkExodusReader::ReadGeometry(int handle, vtkUnstructuredGrid* output)
{
  // Reset the entire unstructured grid
  output->Reset();

  this->ProgressOffset = 0;
  this->ProgressScale = 0.4;

  // Read in cell topology
  this->ReadCells(handle, output);

  this->UpdateProgress(0.4);
  this->ProgressOffset = 0.4;
  this->ProgressScale = 0.2;

  // Read in node and side sets
  this->ReadNodeAndSideSets(handle, output);

  this->UpdateProgress(0.6);

  this->ProgressOffset = 0.6;
  this->ProgressScale = 0.4;
  
  // Now read in the points
  // Note: This should come after reading in
  // the cells and node/side set so that
  // we know which points should actually be
  // stored and put in the output
  this->ReadPoints(handle, output);

}

// Read in node and side set data
void vtkExodusReader::ReadNodeAndSideSets(int handle, vtkUnstructuredGrid* output)
{
  int i, j, k;
  int *indexPtr;
  vtkIdList *cellIds=vtkIdList::New();
  vtkIntArray *counts=vtkIntArray::New();
  vtkIntArray *nodeIndexes=vtkIntArray::New();
  int cellType;
  int cellNumPoints;

 
  // Read in all the node sets that are 'on'
  for (i=0; i < this->GetNumberOfNodeSets(); ++i)
    {
    // Do we read this node set?
    if (this->MetaData->GetNodeSetStatus(i)) 
      { 
      
      // Allocate storage for node indexes
      nodeIndexes->Reset();
      nodeIndexes->SetNumberOfValues(this->MetaData->GetNodeSetSize(i)); 
      
      // Get the node indexes
      ex_get_node_set(handle, this->MetaData->GetNodeSetId(i), 
          nodeIndexes->GetPointer(0));
      
      // Okay now loop though the nodes indexes and insert into the output
      for(j=0; j < this->MetaData->GetNodeSetSize(i); ++j) 
        {
        cellIds->Reset();
        cellIds->InsertNextId(GetPointMapIndex(nodeIndexes->GetValue(j)-1));
        output->InsertNextCell(VTK_VERTEX, cellIds);
        }
      }
    }

  this->UpdateProgress(this->ProgressOffset + this->ProgressScale*0.5);
    
  // Read in all the side sets that are 'on'
  for (i=0; i < this->GetNumberOfSideSets(); ++i)
    {
    // Do we read this side set?
    if (this->MetaData->GetSideSetStatus(i)) 
      {
   
      // Allocate storage for 'counts' (element size) array and 
      // node indexes (connectivity) 
      // Assumption: Side set elements will not be more than 4 nodes :)
      counts->Reset();
      counts->SetNumberOfValues(this->MetaData->GetSideSetSize(i));
      nodeIndexes->Reset();
      nodeIndexes->SetNumberOfValues(this->MetaData->GetSideSetSize(i)*4); 
        
      // Get the counts (element size) array and the node indexes
      ex_get_side_set_node_list(handle, this->MetaData->GetSideSetId(i), 
          counts->GetPointer(0), nodeIndexes->GetPointer(0));
      
      indexPtr = nodeIndexes->GetPointer(0);
      
      // Okay now loop though for each element and set up the cells
      for(j=0; j < this->MetaData->GetSideSetSize(i); ++j) 
        {
        
        // What kind of cell do we have
        cellNumPoints = counts->GetValue(j);
        switch (cellNumPoints)
          {
          case 1:
            cellType=VTK_VERTEX;
            break;
          case 2:
            cellType=VTK_LINE;
            break;
          case 3:
            cellType=VTK_TRIANGLE;
            break;
          case 4:
            cellType=VTK_QUAD;
            break;
          default:
            vtkErrorMacro("Unknown side side element with: " << 
                counts->GetValue(j) << " nodes");
            return;
          }
          
        // Now set up connectivity for cell
        cellIds->Reset();
        for (k=0; k<cellNumPoints; ++k)
          {
          cellIds->InsertNextId(GetPointMapIndex(indexPtr[k]-1));
          }
        indexPtr+=cellNumPoints;
        
        // Now insert the cell
        output->InsertNextCell(cellType, cellIds);
        } 
      }
    }  
       
  // Delete any allocated stuff   
  cellIds->Delete();
  counts->Delete();
  nodeIndexes->Delete();   
}

// Read in connectivity information
void vtkExodusReader::ReadCells(int handle, vtkUnstructuredGrid* output)
{
  int i, j, k;
  int num_elem_in_block;
  int num_nodes_per_elem;
  int num_attr;
  char sm_elem_type[MAX_STR_LENGTH+1];
  int *connect, *pConnect;
  vtkIdList *cellIds=vtkIdList::New();
  int cellType;
  int cellNumPoints;

  // Allocate memory in the output
  output->Allocate(this->NumberOfUsedElements);

  // **KEN** The point maps can be really big.  Therefore, they should be
  // deleted as soon as possible.

  // Set up point map
  this->SetUpPointMap(this->NumberOfNodesInFile);
   
  // Initialize using the type of cells.  
  // A block contains only one type of cell.
  int num_of_blocks = this->MetaData->GetNumberOfBlocks();
  for (i=0; i < num_of_blocks; ++i)
    {
    // Do we read this block?
    if (this->MetaData->GetBlockStatus(i)==0) 
      { 
      continue; 
      }

    // Read in the metadata about this block
    ex_get_elem_block(handle, this->MetaData->GetBlockId(i), sm_elem_type,
                      &num_elem_in_block, &num_nodes_per_elem, &num_attr);

    // If for some weird reason the block has no elements go to next block
    if (num_elem_in_block == 0) 
      {
      continue;
      }
      
    // Okay allocate memory for connectivity data
    connect = new int [num_nodes_per_elem*num_elem_in_block];
    ex_get_elem_conn (handle, this->MetaData->GetBlockId(i), connect);

    // Grab the element type
    char *elem_type = new char[MAX_STR_LENGTH];
    this->StringUppercase(sm_elem_type, elem_type);

    // Check for quadratic elements
    if (!strncmp(elem_type,"TRI",3) && (num_nodes_per_elem==6))
      {cellType=VTK_QUADRATIC_TRIANGLE; cellNumPoints=6;}
    else if (!strncmp(elem_type,"SHE",3) && (num_nodes_per_elem==8)) 
      {cellType=VTK_QUADRATIC_QUAD; cellNumPoints=8;}
    else if (!strncmp(elem_type,"SHE",3) && (num_nodes_per_elem==9)) 
      {cellType=VTK_QUADRATIC_QUAD; cellNumPoints=8;}
    else if (!strncmp(elem_type,"TET",3) && (num_nodes_per_elem==10))
      {cellType=VTK_QUADRATIC_TETRA; cellNumPoints=10;}
    else if (!strncmp(elem_type,"TET",3) && (num_nodes_per_elem==11))
      {cellType=VTK_QUADRATIC_TETRA; cellNumPoints=10;}
    else if (!strncmp(elem_type,"HEX",3) && (num_nodes_per_elem==20)) 
      {cellType=VTK_QUADRATIC_HEXAHEDRON; cellNumPoints=20;}
    else if (!strncmp(elem_type,"HEX",3) && (num_nodes_per_elem==21)) 
      {cellType=VTK_QUADRATIC_HEXAHEDRON; cellNumPoints=20;} 
    else if (!strncmp(elem_type,"HEX",3) && (num_nodes_per_elem==27)) 
      {cellType=VTK_TRIQUADRATIC_HEXAHEDRON; cellNumPoints=27;} 
    else if (!strncmp(elem_type,"QUA",3) && (num_nodes_per_elem==8)) 
      {cellType=VTK_QUADRATIC_QUAD; cellNumPoints=8;}   
    else if (!strncmp(elem_type,"QUA",3) && (num_nodes_per_elem==9)) 
      {cellType=VTK_QUADRATIC_QUAD; cellNumPoints=8;}     
    else if (!strncmp(elem_type,"TRU",3) && (num_nodes_per_elem==3)) 
      {cellType=VTK_QUADRATIC_EDGE; cellNumPoints=3;}    
    else if (!strncmp(elem_type,"BEA",3) && (num_nodes_per_elem==3)) 
      {cellType=VTK_QUADRATIC_EDGE; cellNumPoints=3;}    
    else if (!strncmp(elem_type,"BAR",3) && (num_nodes_per_elem==3)) 
      {cellType=VTK_QUADRATIC_EDGE; cellNumPoints=3;}    
    else if (!strncmp(elem_type,"EDG",3) && (num_nodes_per_elem==3)) 
      {cellType=VTK_QUADRATIC_EDGE; cellNumPoints=3;}    

    // Check for regular elements
    else if (!strncmp(elem_type,"CIR",3)) {cellType=VTK_VERTEX;     cellNumPoints=1;}
    else if (!strncmp(elem_type,"SPH",3)) {cellType=VTK_VERTEX;     cellNumPoints=1;}
    else if (!strncmp(elem_type,"BAR",3)) {cellType=VTK_LINE;       cellNumPoints=2;}
    else if (!strncmp(elem_type,"TRU",3)) {cellType=VTK_LINE;       cellNumPoints=2;}
    else if (!strncmp(elem_type,"BEA",3)) {cellType=VTK_LINE;       cellNumPoints=2;}
    else if (!strncmp(elem_type,"EDG",3)) {cellType=VTK_LINE;       cellNumPoints=2;}
    else if (!strncmp(elem_type,"TRI",3)) {cellType=VTK_TRIANGLE;   cellNumPoints=3;}
    else if (!strncmp(elem_type,"QUA",3)) {cellType=VTK_QUAD;       cellNumPoints=4;}
    else if (!strncmp(elem_type,"TET",3)) {cellType=VTK_TETRA;      cellNumPoints=4;}
    else if (!strncmp(elem_type,"PYR",3)) {cellType=VTK_PYRAMID;    cellNumPoints=5;}
    else if (!strncmp(elem_type,"WED",3)) {cellType=VTK_WEDGE;      cellNumPoints=6;}
    else if (!strncmp(elem_type,"HEX",3)) {cellType=VTK_HEXAHEDRON; cellNumPoints=8;}
    else if (!strncmp(elem_type,"SHE",3) && (num_nodes_per_elem==3)) {cellType=VTK_TRIANGLE;    cellNumPoints=3;}
    else if (!strncmp(elem_type,"SHE",3) && (num_nodes_per_elem==4)) {cellType=VTK_QUAD;        cellNumPoints=4;}
    else if (!strncmp(elem_type,"SUPER",5)) {cellType=VTK_POLY_VERTEX; cellNumPoints=num_nodes_per_elem;}
    else 
      {
      vtkErrorMacro("Unsupported element type: " << elem_type);
      delete [] connect;
      continue;  
      }
    delete []elem_type;
      
    //cell types not currently handled
    //quadratic wedge - 15,16 nodes
    //quadratic pyramid - 13 nodes

    // Now save the cells in a cell array.
    pConnect = connect;
    for (j = 0; j < num_elem_in_block; ++j)
      {
      cellIds->Reset();

      // Build up a list of cell pt ids to insert in to the output data set.
      // Note: The '-1' is because Exodus stores ids starting from 1 not 0.

      // Special setup for quadratic hex because exodus node numbering
      // for this cell type is different than vtk node numbering
      if (cellType==VTK_QUADRATIC_HEXAHEDRON)
        {
        for (k=0; k<12; ++k)
          cellIds->InsertNextId(GetPointMapIndex(pConnect[k]-1));  
        for (k=12; k<16; ++k)
          cellIds->InsertNextId(GetPointMapIndex(pConnect[k+4]-1));
        for (k=16; k<20; ++k)
          cellIds->InsertNextId(GetPointMapIndex(pConnect[k-4]-1));
        }
      else if (cellType==VTK_TRIQUADRATIC_HEXAHEDRON)
        {
        for (k=0; k<12; ++k)
          cellIds->InsertNextId(GetPointMapIndex(pConnect[k]-1));  
        for (k=12; k<16; ++k)
          cellIds->InsertNextId(GetPointMapIndex(pConnect[k+4]-1));
        for (k=16; k<20; ++k)
          cellIds->InsertNextId(GetPointMapIndex(pConnect[k-4]-1));
        cellIds->InsertNextId(GetPointMapIndex(pConnect[23]-1));
        cellIds->InsertNextId(GetPointMapIndex(pConnect[24]-1));
        cellIds->InsertNextId(GetPointMapIndex(pConnect[25]-1));
        cellIds->InsertNextId(GetPointMapIndex(pConnect[26]-1));
        cellIds->InsertNextId(GetPointMapIndex(pConnect[21]-1));
        cellIds->InsertNextId(GetPointMapIndex(pConnect[22]-1));
        cellIds->InsertNextId(GetPointMapIndex(pConnect[20]-1));
        }

      // All the rest of the cells have the same node numbering 
      else
        {
        for (k=0; k<cellNumPoints; ++k)
          cellIds->InsertNextId(GetPointMapIndex(pConnect[k]-1));
        }

      // Skip to next element.
      pConnect += num_nodes_per_elem;

      // Insert cell into output.
      output->InsertNextCell(cellType, cellIds);
      if (j%1000==0)
        {
        double prog = 
          static_cast<double>(i*j)/(num_elem_in_block * num_of_blocks);
        this->UpdateProgress(this->ProgressOffset + this->ProgressScale*prog);
        }
      }
    
    delete [] connect;
    connect = NULL;
    }
  
  cellIds->Delete();
  cellIds = NULL;
}



void vtkExodusReader::ReadPoints(int handle, vtkUnstructuredGrid* output)
{
  vtkPoints *newPoints;
  int pointId, point_index;
  
  // Sanity Check
  if (this->NumberOfUsedNodes == 0)
    {
#if DEBUG
    vtkWarningMacro("The number of used nodes is zero\n");
#endif
    return;
    }

  // Allocate point arrays
  float *x = new float[this->NumberOfNodesInFile];
  float *y = new float[this->NumberOfNodesInFile];
  float *z = new float[this->NumberOfNodesInFile];

  // Clear z values in case you don't really use them
  memset(z,0,this->NumberOfNodesInFile*sizeof(float));

  
  // Get node coordinates
  ex_get_coord(handle, x, y, z);
 
  // Create new points
  newPoints = vtkPoints::New();
  newPoints->SetNumberOfPoints(this->NumberOfUsedNodes);

  // Set up points
  for (pointId=0; pointId<this->NumberOfUsedNodes; ++pointId)
    {
    point_index = this->ReversePointMap->GetValue(pointId);
    newPoints->InsertPoint(pointId, x[point_index], y[point_index], z[point_index]);
    if (pointId%1000 == 0)
      {
      this->UpdateProgress(this->ProgressOffset + 
        static_cast<double>(pointId)/this->NumberOfUsedNodes * this->ProgressScale);
      }
    }
  output->SetPoints(newPoints);
  delete [] x;
  delete [] y;
  delete [] z;
  newPoints->Delete();
  newPoints = NULL;

  // **KEN** I think this would be a good place to clear out the point
  // map and squeeze the reverse map.
}


//----------------------------------------------------------------------------
void vtkExodusReader::ReadArrays(int handle, vtkUnstructuredGrid* output)
{
  vtkDataArray *array;
  int dim, arrayIdx, idx;
  int haveArray, getArray;
  char arrayName[MAX_STR_LENGTH];
  char arrayNameUpper[MAX_STR_LENGTH];

  // Read point arrays.
  // The first vector array encounters is set to vectors,
  // and the first array encountered is set to scalars.
  arrayIdx = 0;
  for (idx = 0; idx < this->GetNumberOfPointArrays(); ++idx) 
    {
    // Get the name of the array
    strcpy(arrayName,this->GetPointArrayName(idx));

    // Do I already have this array?
    if (output->GetPointData()->GetArray(arrayName) != NULL)
      {
      haveArray=1;
      }
    else
      {
      haveArray=0;
      }

    // Does user want displacements
    this->StringUppercase(arrayName,arrayNameUpper);
    if ((this->ApplyDisplacements ||  /* user wants displacement field */
         this->ExodusModelMetadata)   /* user may plan to write out file */
         &&
         !strncmp(arrayNameUpper,"DIS",3)) 
      {
      // Add it to the arrays I want
      this->MetaData->SetPointArrayStatus(idx, 1);
      }

    // Does the user want this array?
    getArray = this->GetPointArrayStatus(idx);

    // If I have the array and the user DOESN'T want it
    // then remove the array from the output
    if (haveArray && !getArray)
      {
      output->GetPointData()->RemoveArray(arrayName);

      if (this->ExodusModel)
        {
        this->ExodusModel->RemoveUGridNodeVariable(arrayName);
        }
      }

    // If the user wants this array AND I don't already have it
    // then read in the data
    else if (getArray && !haveArray)
      { 
      // How many dimensions is this array
      dim = this->GetPointArrayNumberOfComponents(idx);

      if (dim == 1)
        {
        array = this->ReadPointArray(handle, arrayIdx);
        }
      else
        {
        array = this->ReadPointVector(handle, arrayIdx, dim);
        }

      // Opps some sort of problem
      if (array == NULL) 
        {
        vtkErrorMacro("Problem reading node array " << this->GetPointArrayName(idx));
        // Do not try loading this again
        this->MetaData->SetPointArrayStatus(idx,0); 
        } 
      else 
        {
        array->SetName(this->GetPointArrayName(idx));
        output->GetPointData()->AddArray(array);

        // Delete array memory (FIXME this seems inefficient)
        array->Delete();
        array = NULL;

        if (this->ExodusModel)
          {
          // So ExodusIIWriter can map names in UGrid back to names in
          // Exodus II file.

          char *origName = 
            vtkExodusReader::StrDupWithNew(
              this->MetaData->GetPointArrayOriginalName(arrayIdx));
          char *newName = 
            vtkExodusReader::StrDupWithNew(this->GetPointArrayName(idx));

          this->ExodusModel->AddUGridNodeVariable(newName, origName, dim);
          }
        }  
      }

    // The array index needs to be incremented by the dimension
    dim = this->GetPointArrayNumberOfComponents(idx);
    arrayIdx += dim;  
    }

  // Read cell arrays.
  arrayIdx = 0;
  for (idx = 0; idx < this->GetNumberOfCellArrays(); ++idx)
    {

    // Get the name of the array
    strcpy(arrayName,this->GetCellArrayName(idx));

    // Do I already have this array?
    if (output->GetCellData()->GetArray(arrayName) != NULL)
      haveArray=1;
    else
      haveArray=0;

    // Does the user want this array?
    getArray = this->GetCellArrayStatus(idx);

    // If I have the array and the user DOESN'T want it
    // then remove the array from the output
    if (haveArray && !getArray)
      {
      output->GetCellData()->RemoveArray(arrayName);

      if (this->ExodusModel)
        {
        this->ExodusModel->RemoveUGridElementVariable(arrayName);
        }
      }
    // If the user wants this array AND I don't already have it
    // then read in the data
    else if (getArray && !haveArray)
      {
      // How many dimensions is this array
      dim = this->GetCellArrayNumberOfComponents(idx);
      if (dim == 1)
        {
        array = this->ReadCellArray(handle, arrayIdx);
        }
      else
        {
        array = this->ReadCellVector(handle, arrayIdx, dim);
        }

      // Opps some sort of problem
      if (array == NULL)
        {
        vtkErrorMacro("Problem reading cell array " << this->GetCellArrayName(idx));
        // Do not try loading this again
        this->MetaData->SetCellArrayStatus(idx, 0);

        }
      else
        {
        array->SetName(this->GetCellArrayName(idx));
        output->GetCellData()->AddArray(array);

        array->Delete();
        array = NULL;

        if (this->ExodusModel)
          {
          // So ExodusIIWriter can map names in UGrid back to names in
          // Exodus II file.

          char *origName = 
            vtkExodusReader::StrDupWithNew(
              this->MetaData->GetCellArrayOriginalName(arrayIdx));
          char *newName = 
            vtkExodusReader::StrDupWithNew(this->GetCellArrayName(idx));

          this->ExodusModel->AddUGridElementVariable(newName, origName, dim);
          }
        }  
      }
    // The array index needs to be incremented by the dimension
    dim = this->GetCellArrayNumberOfComponents(idx);
    arrayIdx += dim;
    }

}

void vtkExodusReader::AddDisplacements(vtkUnstructuredGrid* output)
{
  char arrayName[MAX_STR_LENGTH];
  char arrayNameUpper[MAX_STR_LENGTH];
  int FOUND = 0;

  // Now find the displacement array
  strcpy(arrayName, "None");
  for (int idx = 0; idx < this->GetNumberOfPointArrays(); ++idx) 
    {
    // Is this array displacement?
    strcpy(arrayName, this->GetPointArrayName(idx));
    this->StringUppercase(arrayName,arrayNameUpper);
    if (!strncmp(arrayNameUpper,"DIS",3)) 
      {
      FOUND = 1;
      break;
      }
    }

  // If I did not find displacements then just return
  if (!FOUND) return;

  // Create warp vector filter
  vtkWarpVector *warp = vtkWarpVector::New();

  vtkUnstructuredGrid *geom = vtkUnstructuredGrid::New();
  geom->ShallowCopy(output);

  warp->SetInput(geom);
  warp->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, arrayName);
  if (this->HasModeShapes)
    {
    warp->SetScaleFactor(
              this->DisplacementMagnitude*cos(2*vtkMath::Pi()*this->TimeValue));
    }
  else
    {
    warp->SetScaleFactor(this->DisplacementMagnitude);
    }
  warp->Update();

  geom->Delete();

  // Copy warped mesh and cell/point data to my output

  output->CopyStructure(warp->GetUnstructuredGridOutput());
  output->GetCellData()->PassData(warp->GetUnstructuredGridOutput()->GetCellData());
  output->GetPointData()->PassData(warp->GetUnstructuredGridOutput()->GetPointData());

  // Delete warp filter
  warp->Delete();

}


vtkDataArray *vtkExodusReader::ReadPointVector(int handle, int varIndex, int dim)
{
  // Sanity check
  if (dim !=2 && dim!=3)
    {
    vtkErrorMacro("Error: Only support 2 or 3 dim vectors var_index:" \
                  << varIndex << " dim:" << dim<< " file: " << this->FileName);
    return NULL; 
    }

  // Create vector array
  vtkFloatArray *vectors = vtkFloatArray::New();
  vectors->SetNumberOfComponents(3);
  vectors->SetNumberOfTuples(this->NumberOfUsedNodes);

  // Grab first two arrays
  vtkFloatArray *dim1 = (vtkFloatArray*)ReadPointArray(handle,varIndex);
  vtkFloatArray *dim2 = (vtkFloatArray*)ReadPointArray(handle,varIndex+1);

  // Do they need a third
  vtkFloatArray *dim3 = NULL;
  if (dim == 3)
    dim3 = (vtkFloatArray*)ReadPointArray(handle,varIndex+2);

  // Okay now set up memory pointers
  float *vector_ptr = vectors->GetPointer(0);
  float *dim1_ptr = dim1->GetPointer(0);
  float *dim2_ptr = dim2->GetPointer(0);
  float *dim3_ptr = NULL; if(dim==3) dim3_ptr = dim3->GetPointer(0);

  // Okay now swizzle into vector array
  if (dim==2)
    {
    for(int i=0; i<this->NumberOfUsedNodes; ++i)
      {
      *vector_ptr++ = *dim1_ptr++;
      *vector_ptr++ = *dim2_ptr++;
      *vector_ptr++ = 0;
      }
    }
  else
    {
    for(int i=0; i<this->NumberOfUsedNodes; ++i)
      {
      *vector_ptr++ = *dim1_ptr++;
      *vector_ptr++ = *dim2_ptr++;
      *vector_ptr++ = *dim3_ptr++;
      }
    }

  dim1->Delete();
  dim2->Delete();
  if ( dim3 )
    {
    dim3->Delete();
    }

  // Okay all done
  return vectors;
}

vtkDataArray *vtkExodusReader::ReadPointArray(int handle, int varIndex)
{
  // Temp float array
  float *exo_array_data;
  exo_array_data = new float[this->NumberOfNodesInFile];

  // Create data array
  vtkFloatArray *array = vtkFloatArray::New();
  array->SetNumberOfValues(this->NumberOfUsedNodes);

  // Read in data into temp array (note that Exodus is 1 based)
  int error = ex_get_nodal_var (handle, 
                                this->ActualTimeStep+1, 
                                varIndex+1, 
                                this->NumberOfNodesInFile,
                                exo_array_data);
  
  // Opps something bad happened
  if (error < 0)
    {
    vtkErrorMacro("Error: " << error << " ex_get_nodal_var timestep:" 
                  << this->ActualTimeStep << " var_index: " << varIndex 
                  << " file: " << this->FileName);
    return NULL;  
    }

  // Okay copy the points that are actually used into the vtk array
  int point_index;
  for(int i=0; i<this->NumberOfUsedNodes; i++)
    {
    point_index = this->ReversePointMap->GetValue(i);
    array->SetValue(i, exo_array_data[point_index]);
    }

  // Clean up temp float array
  delete [] exo_array_data;

  
  return array;
}

vtkDataArray *vtkExodusReader::ReadCellVector(int handle, int varIndex, int dim)
{
  // Sanity check
  if (dim !=2 && dim!=3)
    {
    vtkErrorMacro("Error: Only support 2 or 3 dim vectors var_index:" \
                  << varIndex << " dim:" << dim<< " file: " << this->FileName);
    return NULL; 
    }

  // Create vector array
  vtkFloatArray *vectors = vtkFloatArray::New();
  vectors->SetNumberOfComponents(3);
  vectors->SetNumberOfTuples(this->NumberOfUsedElements);

  // Grab first two arrays
  vtkFloatArray *dim1 = (vtkFloatArray*)ReadCellArray(handle,varIndex);
  vtkFloatArray *dim2 = (vtkFloatArray*)ReadCellArray(handle,varIndex+1);

  // Do they need a third
  vtkFloatArray *dim3 = NULL;
  if (dim == 3)
    {
    dim3 = (vtkFloatArray*)ReadCellArray(handle,varIndex+2);
    }

  // Okay now set up memory pointers
  float *vector_ptr = vectors->GetPointer(0);
  float *dim1_ptr = dim1->GetPointer(0);
  float *dim2_ptr = dim2->GetPointer(0);
  float *dim3_ptr = NULL; if(dim==3) dim3_ptr = dim3->GetPointer(0);

  // Okay now swizzle into vector array
  if (dim==2)
    {
    for(int i=0; i<this->NumberOfUsedElements; ++i)
      {
      *vector_ptr++ = *dim1_ptr++;
      *vector_ptr++ = *dim2_ptr++;
      *vector_ptr++ = 0;
      }
    }
  else
    {
    for(int i=0; i<this->NumberOfUsedElements; ++i)
      {
      *vector_ptr++ = *dim1_ptr++;
      *vector_ptr++ = *dim2_ptr++;
      *vector_ptr++ = *dim3_ptr++;
      }
    }

  dim1->Delete();
  dim2->Delete();
  if ( dim3 )
    {
    dim3->Delete();
    }

  // Okay all done
  return vectors;
}

void vtkExodusReader::FixMetadataTruthTable(int *table, int len)
{
  if (this->ExodusModelMetadata && this->ExodusModel)
    {
    // vtkModelMetadata will free table when it is destroyed

    int *newtable = new int [len];
    memcpy(newtable, table, len * sizeof(int));

    this->ExodusModel->GetModelMetadata()->SetElementVariableTruthTable(newtable);
    }
}

// Read in the cell array specified by index 'varIndex'
vtkDataArray *vtkExodusReader::ReadCellArray(int handle, int varIndex)
{
  int i;
  int error;
  int blockIdx;
 
  // Create the data array 
  vtkFloatArray *array = vtkFloatArray::New();
  array->SetNumberOfValues(this->NumberOfUsedElements);
  float *arrayPtr = array->GetPointer(0);
  int nblocks = this->MetaData->GetNumberOfBlocks();
 
  // Loop through the blocks.
  for (blockIdx=0; blockIdx < nblocks; ++blockIdx)
    {
    // Do we read this block?
    if (this->MetaData->GetBlockStatus(blockIdx)==0) 
      {
      continue;
      }

    // Get number of elements in block
    int numBlockElements = this->MetaData->GetNumElementsInBlock(blockIdx);

    // Truth Table may say no variables on this block for this variable
    int cell_arrays = this->MetaData->GetOriginalNumberOfCellArrays();
    int truthTableIdx = 
      this->MetaData->GetSortedOrder(blockIdx)*cell_arrays + varIndex;

    if (this->CellVarTruthTable->GetValue(truthTableIdx) == 1)
      {
      error = 
        ex_get_elem_var (handle, this->ActualTimeStep+1, varIndex+1, 
                         this->MetaData->GetBlockId(blockIdx), numBlockElements, 
                         arrayPtr);
  
      if (error < 0)
        {
        vtkWarningMacro("Warning: Truth Table indicated that cell variable " <<
                        this->GetCellArrayName(varIndex) <<
                        " appears in block " << this->MetaData->GetBlockId(blockIdx) <<
                        ",\nhowever it is not there.  "
                        "Truth table has been modified (in VTK memory only).");
  
          // Assume the truth table is wrong.  (Because we have seen this occur.)
          // Change our table and change the one saved in the metadata.
          this->CellVarTruthTable->SetValue(truthTableIdx, 0);
          this->FixMetadataTruthTable(this->CellVarTruthTable->GetPointer(0),
                                      nblocks * cell_arrays);
        }
      }

    if (this->CellVarTruthTable->GetValue(truthTableIdx) == 0)
      {
      for(i=0; i<numBlockElements; ++i)
        {
        arrayPtr[i] = 0;
        }
      }

    arrayPtr += numBlockElements;
    }
    
  // Padding cell arrays to have 'some' value for the addition 
  // of nodesets and sidesets
  for(i=0;i<GetExtraCellCountForNodeSideSets();++i)
    {
    array->InsertNextValue(0);
    }

   
  // AOK so return array
  return array;
}


int vtkExodusReader::GetExtraCellCountForNodeSideSets()
{
  int i;
  int count=0;
  
  // Count number of additional 'cells' for nodesets
  for (i=0; i < this->GetNumberOfNodeSets(); ++i)
    {
    // Do we read this node set?
    if (this->MetaData->GetNodeSetStatus(i)) 
      { 
      count += this->MetaData->GetNodeSetSize(i);
      }
    }
    
  // Count number of additional 'cells' for sidesets
  for (i=0; i < this->GetNumberOfSideSets(); ++i)
    {
    // Do we read this node set?
    if (this->MetaData->GetSideSetStatus(i)) 
      { 
      count += this->MetaData->GetSideSetSize(i);
      }
    }
    
    
  // Return number of additional cell values needed
  // to accommodate scalar fields count  
  return count;
 }

//----------------------------------------------------------------------------
void vtkExodusReader::GenerateExtraArrays(vtkUnstructuredGrid* output)
{
  vtkIntArray *array;
  int count;
  int blockId;
  int i, j, numBlockElem;
  int haveArray, getArray;

  ///////////////////////////////////////
  // Get block array
  ///////////////////////////////////////

  // Do I already have this array?
  if (output->GetPointData()->GetArray( this->GetBlockIdArrayName() ) != NULL)
    haveArray=1;
  else
    haveArray=0;

  // Does the user want this array?
  getArray = this->GenerateBlockIdCellArray;

  // If I have the array and the user DOESN'T want it
  // then remove the array from the output
  if (haveArray && !getArray)
    {
    output->GetPointData()->RemoveArray( this->GetBlockIdArrayName() );
    }
  // If the user wants this array AND I don't already have it
  // then read in the data
  else if (getArray && !haveArray)
    {
    array = vtkIntArray::New();
    array->SetNumberOfValues(this->NumberOfUsedElements);
    count = 0;
    for (i=0; i < this->MetaData->GetNumberOfBlocks(); ++i)
      {
      // Do we read this block?
      if (this->MetaData->GetBlockStatus(i)==0)
        continue;

      numBlockElem = this->MetaData->GetNumElementsInBlock(i);
      blockId = this->MetaData->GetBlockId(i);
      for (j = 0; j < numBlockElem; ++j)
        {
        array->SetValue(count++,blockId);
        }
      }
     // Padding cell arrays to have 'some' value for the addition 
     // of nodesets and sidesets
     for(i=0;i<GetExtraCellCountForNodeSideSets();++i)
       {
       array->InsertNextValue(0);
       }
    array->SetName( this->GetBlockIdArrayName() );
    output->GetCellData()->AddArray(array);

    // Block IDs will be the default cell data
    output->GetCellData()->SetScalars(array);
    array->Delete();
    array = NULL;
    }

  ///////////////////////////////////////
  // Get node id array
  ///////////////////////////////////////

  // Do I already have this array?
  if ( output->GetPointData()->GetArray( 
         this->GetGlobalNodeIdArrayName() ) != NULL)
    {
    haveArray = 1;
    }
  else
    {
    haveArray = 0;
    }

  // Does the user want this array?
  getArray = this->GenerateGlobalNodeIdArray;

  // If I have the array and the user DOESN'T want it
  // then remove the array from the output
  if (haveArray && !getArray)
    {
    output->GetPointData()->RemoveArray( this->GetGlobalNodeIdArrayName() );
    }
  // If the user wants this array AND I don't already have it
  // then read in the data
  else if (getArray && !haveArray)
    {
    // Temp int array
    int *exo_array_data;
    exo_array_data = new int[this->NumberOfNodesInFile];
  
    // Temp vtk array
    vtkIdTypeArray* idarray = vtkIdTypeArray::New();
    idarray->SetNumberOfValues(this->NumberOfUsedNodes);

    // Get the data into the temp int array
    ex_get_node_num_map(this->CurrentHandle, exo_array_data);
cerr << "node num map : ";
for (i = 0; i < this->NumberOfNodesInFile; i ++)
{
  cerr << exo_array_data[i] << " ";
}
cerr << endl;


    // Okay copy the points that are actually used into the vtk array
    int point_index;
    for (i=0; i<this->NumberOfUsedNodes; i++)
      {
      point_index = this->ReversePointMap->GetValue(i);
      idarray->SetValue(i, exo_array_data[point_index]);
      }

    // Clean up temp int array
    delete [] exo_array_data;

    // Set up array and 'give' to output
    idarray->SetName( this->GetGlobalNodeIdArrayName() );

    vtkIdTypeArray* pedigree = vtkIdTypeArray::New();
    pedigree->DeepCopy(idarray);
    pedigree->SetName(this->GetPedigreeNodeIdArrayName());
    
    output->GetPointData()->AddArray(pedigree);
    output->GetPointData()->SetGlobalIds(idarray);

    // Delete my copy of the array
    pedigree->Delete();
    pedigree = NULL;
    idarray->Delete();
    idarray = NULL;
    }

  ///////////////////////////////////////
  // Get element id array
  ///////////////////////////////////////

  // Do I already have this array?
  if (output->GetPointData()->GetArray( 
        this->GetGlobalElementIdArrayName() ) != NULL)
    {
    haveArray=1;
    }
  else
    {
    haveArray=0;
    }

  // Does the user want this array?
  getArray = this->GenerateGlobalElementIdArray;

  // If I have the array and the user DOESN'T want it
  // then remove the array from the output
  if (haveArray && !getArray)
    {
    output->GetPointData()->RemoveArray( this->GetGlobalElementIdArrayName() );
    }
  // If the user wants this array AND I don't already have it
  // then read in the data
  else if (getArray && !haveArray)
    {
    int nblocks = this->GetNumberOfBlockArrays();
    int nblocksUsed = 0;

    for (i=0; i<nblocks; i++)
      {
      if (this->GetBlockArrayStatus(i) == 1)
        {
        nblocksUsed++;
        }
      }
    if (!this->GlobalElementIdCache)
      {
      int *tmp = new int [this->NumberOfElementsInFile];
      ex_get_elem_num_map(this->CurrentHandle, tmp);
      this->GlobalElementIdCache = tmp;
      }

    int *idList = (int*)malloc(this->NumberOfUsedElements*sizeof(int));

    if (nblocksUsed < nblocks)
      {
      int *from = this->GlobalElementIdCache;
      int *to = idList;

      for (i=0; i<nblocks; i++)
        {
        int used = this->GetBlockArrayStatus(i);
        int bsize =  this->GetNumberOfElementsInBlock(i);

        if (used)
          {
          memcpy(to, from, bsize * sizeof(int));
          to += bsize;
          }
        from += bsize;
        }
      }
    else
      {
      memcpy(idList, this->GlobalElementIdCache, 
             this->NumberOfUsedElements * sizeof(int));
      }

    vtkIdTypeArray* idarray = vtkIdTypeArray::New();
#ifdef VTK_USE_64BIT_IDS
    idarray->SetNumberOfValues(this->NumberOfUsedElements);
    for (int idIdx=0; idIdx < this->NumberOfUsedElements; idIdx++)
      {
      idarray->SetValue(idIdx, idList[idIdx]);
      }
#else
    idarray->SetArray(idList, this->NumberOfUsedElements, 0);
#endif
    idarray->SetName( this->GetGlobalElementIdArrayName() );
    
    // Padding cell arrays to have 'some' value for the addition 
    // of nodesets and sidesets
    for(i=0;i<GetExtraCellCountForNodeSideSets();++i)
      {
      idarray->InsertNextValue(0);
      }

    vtkIdTypeArray* pedigree = vtkIdTypeArray::New();
    pedigree->DeepCopy(idarray);
    pedigree->SetName(this->GetPedigreeElementIdArrayName());

    output->GetCellData()->AddArray(pedigree);
    output->GetCellData()->SetGlobalIds(idarray);
    pedigree->Delete();
    pedigree = NULL;
    idarray->Delete();
    idarray = NULL;
    }
  // **KEN** I think we should be done with the ReversePointMap.  Should we
  // delete it here?

  // Used by the ExodusIIWriter when writing back to the original file 
  // as a map between the used node ids and the actual ids in the file
  
  //this->ReversePointMap->SetNumberOfValues(this->NumberOfUsedNodes);
  //this->ReversePointMap->SetName( "InternalNodeId" );
  //output->GetPointData()->AddArray(this->ReversePointMap);

}


  
//----------------------------------------------------------------------------
void vtkExodusReader::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  this->Superclass::PrintSelf(os,indent);

  if (this->GenerateBlockIdCellArray)
    {
    os << indent << "GenerateBlockIdCellArray: On\n";
    }
  else
    {
    os << indent << "GenerateBlockIdCellArray: Off\n";
    }

  if (this->GenerateGlobalElementIdArray)
    {
    os << indent << "GenerateGlobalElementIdArray: On\n";
    }
  else
    {
    os << indent << "GenerateGlobalElementIdArray: Off\n";
    }

  if (this->GenerateGlobalNodeIdArray)
    {
    os << indent << "GenerateGlobalNodeIdArray: On\n";
    }
  else
    {
    os << indent << "GenerateGlobalNodeIdArray: Off\n";
    }  
  
  if (this->PackExodusModelOntoOutput )
    {
    os << indent << "PackExodusModelOntoOutput: On\n";
    }
  else
    {
    os << indent << "PackExodusModelOntoOutput: Off\n";
    }  
  
  if (this->ApplyDisplacements)
    {
    os << indent << "ApplyDisplacements: On\n";
    }
  else
    {
    os << indent << "ApplyDisplacements: Off\n";
    }  
  
  if (this->ExodusModelMetadata)
    {
    os << indent << "ExodusModelMetadata: On\n";
    }
  else
    {
    os << indent << "ExodusModelMetadata: Off\n";
    }  
  
  os << indent << "File Name: " 
     << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "XML File Name: " 
     << (this->XMLFileName ? this->XMLFileName : "(none)") << "\n";
  os << indent << "Title: " 
     << (this->Title ? this->Title : "(none)") << "\n";
  os << indent << "Dimensionality: " << this->Dimensionality << "\n";
  os << indent << "NumberOfUsedNodes: " << this->NumberOfUsedNodes << "\n";
  os << indent << "NumberOfNodesInFile: " << this->NumberOfNodesInFile << "\n";
  os << indent << "NumberOfUsedElements: " << this->NumberOfUsedElements << "\n";
  os << indent << "NumberOfElementsInFile: " << this->NumberOfElementsInFile << "\n";
  os << indent << "NumberOfBlocks: " << this->NumberOfBlocks << "\n";
  for (idx = 0; idx < this->NumberOfBlocks; ++idx)
    {
    os << indent << "  " << this->MetaData->GetNumElementsInBlock(idx)
       << " elements in block " << this->MetaData->GetBlockId(idx) << "\n";
    }
  os << indent << "NumberOfTimeSteps: " << this->NumberOfTimeSteps << "\n";
  os << indent << "NumberOfPointArrays: " << this->GetNumberOfPointArrays() << "\n";
  for (idx = 0; idx < this->GetNumberOfPointArrays(); ++idx)
    {
    os << indent << "  " << this->GetPointArrayName(idx);
    if (this->GetPointArrayNumberOfComponents(idx) != 1)
      {
      os << " " << this->GetPointArrayNumberOfComponents(idx) << " components";
      }
    if (this->GetPointArrayStatus(idx) == 0)
      {
      os << " do not load";
      }
    os << endl;
    }
  os << indent << "NumberOfCellArrays: " << this->GetNumberOfCellArrays() << "\n";
  for (idx = 0; idx < this->GetNumberOfCellArrays(); ++idx)
    {
    os << indent << "  " << this->GetCellArrayName(idx);
    if (this->GetCellArrayNumberOfComponents(idx) != 1)
      {
      os << " " << this->GetCellArrayNumberOfComponents(idx) << " components";
      }
    if (this->GetCellArrayStatus(idx) == 0)
      {
      os << " do not load";
      }
    os << endl;
    }
  os << indent << "NumberOfSideSets: " << this->NumberOfSideSets << "\n";
  os << indent << "NumberOfNodeSets: " << this->NumberOfNodeSets << "\n";
  os << indent << "TimeStep: " << this->TimeStep << endl;
  os << indent << "TimeStepRange: " << this->TimeStepRange[0] << " " << this->TimeStepRange[1] << endl;
  os << indent << "DisplacementMagnitude: " << this->DisplacementMagnitude << "\n";
  os << indent << "DisplayType: " << this->DisplayType << "\n";
  os << indent << "HasModeShapes: " << this->HasModeShapes << endl;
}


void vtkExodusReader::CloseCurrentFile()
{
  int error;
  error = ex_close(this->CurrentHandle);
  if (error < 0)
    {
    vtkErrorMacro("Error: " << error << " calling ex_close " << this->FileName);
    }    
  this->CurrentHandle = -1; 
}

int vtkExodusReader::StringsEqual(const char* s1, char* s2)
{
  int same = 0;

  if ((s1 == NULL) && (s2 == NULL))
    {
    same = 1;
    }
  else if (s1 && s2 && (strcmp(s1, s2) == 0))
    {
    same = 1;
    }

  return same;
}

void vtkExodusReader::StringUppercase(const char* str, char* upperstr)
{ 
  int len = 0;
  if ( str )
    {
    len = static_cast<int>( strlen(str) );
    for (int i=0; i<len; i++)
      upperstr[i] = toupper(str[i]);
    }
  else
    { 
    upperstr = NULL;
    }

  // Add string terminator
  upperstr[len] = '\0';
}

void vtkExodusReader::SetUpPointMap(int num_points) 
{
  // Allocate and set to -1
  this->PointMap->SetNumberOfValues(num_points);
  for(int i=0; i<num_points; i++)
    {
    this->PointMap->SetValue(i, -1);
    }

  // I have used zero nodes at this time
  this->NumberOfUsedNodes = 0;

  // Also allocate the reverse point map
  this->ReversePointMap->SetNumberOfValues(num_points);
}

int vtkExodusReader::GetPointMapIndex(int point_id)
{
  // We may not have this point
  if (this->PointMap->GetValue(point_id) == -1)
    {
    this->PointMap->SetValue(point_id, this->NumberOfUsedNodes);

    // Store reverse lookup
    this->ReversePointMap->SetValue(this->NumberOfUsedNodes, point_id);

    // Increment the number of used nodes
    this->NumberOfUsedNodes++;

    // Return the value of the stored index
    return this->NumberOfUsedNodes-1;
    }

  // We have the point so return it's mapped value
  return this->PointMap->GetValue(point_id);
}

void vtkExodusReader::SetAllAssemblyArrayStatus( int status )
{
  this->MetaData->SetArrayStatusInitValue( vtkExodusReader::ASSEMBLY, status );
  int numVars = this->GetNumberOfAssemblyArrays();
  for ( int id=0;id<numVars;id++ )
    {
    SetAssemblyArrayStatus( id, status );
    }
}

void vtkExodusReader::SetAllBlockArrayStatus( int status )
{
  this->MetaData->SetArrayStatusInitValue( vtkExodusReader::BLOCK, status );
  int numVars = this->GetNumberOfBlockArrays();
  for ( int id=0;id<numVars;id++ )
    {
    SetBlockArrayStatus( id, status );
    }
}

void vtkExodusReader::SetAllCellArrayStatus( int status )
{
  this->MetaData->SetArrayStatusInitValue( vtkExodusReader::CELL, status );
  int numVars = this->GetNumberOfCellArrays();
  for ( int id=0;id<numVars;id++ )
    {
    SetCellArrayStatus( id, status );
    }
}

void vtkExodusReader::SetAllHierarchyArrayStatus( int status )
{
  this->MetaData->SetArrayStatusInitValue( vtkExodusReader::HIERARCHY, status );
  int numVars = this->GetNumberOfHierarchyArrays();
  for ( int id=0;id<numVars;id++ )
    {
    SetHierarchyArrayStatus( id, status );
    }
}

void vtkExodusReader::SetAllMaterialArrayStatus( int status )
{
  this->MetaData->SetArrayStatusInitValue( vtkExodusReader::MATERIAL, status );
  int numVars = this->GetNumberOfMaterialArrays();
  for ( int id=0;id<numVars;id++ )
    {
    this->SetMaterialArrayStatus( id, status );
    }
}

void vtkExodusReader::SetAllPartArrayStatus( int status )
{
  this->MetaData->SetArrayStatusInitValue( vtkExodusReader::PART, status );
  int numVars = this->GetNumberOfPartArrays();
  for ( int id=0;id<numVars;id++ )
    {
    this->SetPartArrayStatus( id, status );
    }
}

void vtkExodusReader::SetAllPointArrayStatus( int status )
{
  this->MetaData->SetArrayStatusInitValue( vtkExodusReader::POINT, status );
  int numVars = this->GetNumberOfPointArrays();
  for ( int id=0;id<numVars;id++ )
    {
    this->SetPointArrayStatus( id, status );
    }
}

char *vtkExodusReader::StrDupWithNew(const char *s)
{
  char *newstr = NULL;

  if (s)
    {
    int len = static_cast<int>( strlen(s) );
    if (len == 0)
      {
      newstr = new char [1];
      newstr[0] = '\0';
      }
    else
      {
      newstr = new char [len + 1];
      strcpy(newstr, s);
      }
    } 
  
  return newstr;
}

void vtkExodusReader::SetArrayStatus ( vtkExodusReader::ArrayType type, 
                                       const char *name, int flag )
{
  switch (type)
    {
    case vtkExodusReader::CELL:
      this->SetCellArrayStatus( name, flag );
      break;
    case vtkExodusReader::POINT:
      this->SetPointArrayStatus( name, flag );
      break;
    case vtkExodusReader::BLOCK:
      this->SetBlockArrayStatus( name, flag );
      break;
    case vtkExodusReader::PART:
      this->SetPartArrayStatus( name, flag );
      break;
    case vtkExodusReader::MATERIAL:
      this->SetMaterialArrayStatus( name, flag );
      break;
    case vtkExodusReader::ASSEMBLY:
      this->SetAssemblyArrayStatus( name, flag );
      break;
    case vtkExodusReader::HIERARCHY:
      this->SetHierarchyArrayStatus( name, flag );
      break;
    case vtkExodusReader::NUM_ARRAY_TYPES:
    case vtkExodusReader::UNKNOWN_TYPE:
    default:
      break;
    }
}

void vtkExodusReader::SetAllArrayStatus ( vtkExodusReader::ArrayType type, 
                                          int flag )
{
  switch (type)
    {
    case vtkExodusReader::CELL:
      this->SetAllCellArrayStatus( flag );
      break;
    case vtkExodusReader::POINT:
      this->SetAllPointArrayStatus( flag );
      break;
    case vtkExodusReader::BLOCK:
      this->SetAllBlockArrayStatus( flag );
      break;
    case vtkExodusReader::PART:
      this->SetAllPartArrayStatus( flag );
      break;
    case vtkExodusReader::MATERIAL:
      this->SetAllMaterialArrayStatus( flag );
      break;
    case vtkExodusReader::ASSEMBLY:
      this->SetAllAssemblyArrayStatus( flag );
      break;
    case vtkExodusReader::HIERARCHY:
      this->SetAllHierarchyArrayStatus( flag );
      break;
    case vtkExodusReader::NUM_ARRAY_TYPES:
    case vtkExodusReader::UNKNOWN_TYPE:
    default:
      break;
    }
}


int vtkExodusReader::GetArrayStatus ( vtkExodusReader::ArrayType type, 
                                      const char *name )
{
  switch (type)
    {
    case vtkExodusReader::CELL:
      return this->GetCellArrayStatus( name );
      break;
    case vtkExodusReader::POINT:
      return this->GetPointArrayStatus( name );
      break;
    case vtkExodusReader::BLOCK:
      return this->GetBlockArrayStatus( name );
      break;
    case vtkExodusReader::PART:
      return this->GetPartArrayStatus( name );
      break;
    case vtkExodusReader::MATERIAL:
      return this->GetMaterialArrayStatus( name );
      break;
    case vtkExodusReader::ASSEMBLY:
      return this->GetAssemblyArrayStatus( name );
      break;
    case vtkExodusReader::HIERARCHY:
      return this->GetHierarchyArrayStatus( name );
      break;
    case vtkExodusReader::NUM_ARRAY_TYPES:
    case vtkExodusReader::UNKNOWN_TYPE:
    default:
      break;
    }

  return 0;
}

int vtkExodusReader::GetNumberOfArrays ( vtkExodusReader::ArrayType type )
{
  switch (type)
    {
    case vtkExodusReader::CELL:
      return this->GetNumberOfCellArrays();
      break;
    case vtkExodusReader::POINT:
      return this->GetNumberOfPointArrays();
      break;
    case vtkExodusReader::BLOCK:
      return this->GetNumberOfBlockArrays();
      break;
    case vtkExodusReader::PART:
      return this->GetNumberOfPartArrays();
      break;
    case vtkExodusReader::MATERIAL:
      return this->GetNumberOfMaterialArrays();
      break;
    case vtkExodusReader::ASSEMBLY:
      return this->GetNumberOfAssemblyArrays();
      break;
    case vtkExodusReader::HIERARCHY:
      return this->GetNumberOfHierarchyArrays();
      break;
    case vtkExodusReader::NUM_ARRAY_TYPES:
    case vtkExodusReader::UNKNOWN_TYPE:
    default:
      break;
    }

  return 0;
}

const char*vtkExodusReader::GetArrayName ( vtkExodusReader::ArrayType type, 
    int id )
{
  switch (type)
    {
    case vtkExodusReader::CELL:
      return this->GetCellArrayName( id );
      break;
    case vtkExodusReader::POINT:
      return this->GetPointArrayName( id );
      break;
    case vtkExodusReader::BLOCK:
      return this->GetBlockArrayName( id );
      break;
    case vtkExodusReader::PART:
      return this->GetPartArrayName( id );
      break;
    case vtkExodusReader::MATERIAL:
      return this->GetMaterialArrayName( id );
      break;
    case vtkExodusReader::ASSEMBLY:
      return this->GetAssemblyArrayName( id );
      break;
    case vtkExodusReader::HIERARCHY:
      return this->GetHierarchyArrayName( id );
      break;
    case vtkExodusReader::NUM_ARRAY_TYPES:
    case vtkExodusReader::UNKNOWN_TYPE:
    default:
      break;
    }

  return NULL;
}

// Had to add this because displacement vector names had spaces
// following them (dispx{space}, dispy{space}, dispz{space}).  They
// didn't get made into a vector, so displacement was garbage.

void vtkExodusReader::RemoveBeginningAndTrailingSpaces(char **names, int len)
{
  int i, j;

  for (i=0; i<len; i++)
    {
    char *c = names[i];
    int nmlen = static_cast<int>( strlen(c) );

    char *cbegin = c;
    char *cend = c + nmlen - 1;

    // remove spaces or non-printing character from start and end

    for (j=0; j<nmlen; j++)
      {
      if (!isgraph(*cbegin)) cbegin++;
      else break;
      }

    for (j=0; j<nmlen; j++)
      {
      if (!isgraph(*cend)) cend--;
      else break;
      }

    if (cend < cbegin)
      {
      sprintf(names[i], "null_%d", i);
      continue;
      }

    int newlen = cend - cbegin + 1;

    if (newlen < nmlen)
      {
      for (j=0; j<newlen; j++)
        {
        *c++ = *cbegin++;
        }
      *c = '\0';
      }
    }
}

int vtkExodusReader::GetTimeSeriesData( int itemID, const char *vName, 
                                         const char *vType, vtkFloatArray *result ) 
{
  int retVal = 0;

  if ( this->OpenCurrentFile() )
    {
    int numTimesteps = this->GetNumberOfTimeSteps();
    result->SetNumberOfComponents( 1 );
    result->SetNumberOfTuples( numTimesteps );
    // result->Initialize();
    result->SetName( vName );
    float *memory = result->GetPointer( 0 ); 

    if ((strcmp(vType, "CELL") == 0) || (strcmp(vType, "cell") == 0) ) 
      {
      int varid = GetCellArrayID( vName );
      // ex_get_elem_var_time assumes zero-based index for varid
      ex_get_elem_var_time( this->CurrentHandle, varid, itemID, 1, 
                                  numTimesteps, memory );
      retVal = 1;
      }
    else if ((strcmp(vType, "POINT") == 0) || (strcmp(vType, "point") == 0) ) 
      {
      int varid = GetPointArrayID( vName );
      // ex_get_nodal_var_time assumes one-based index for varid
      ex_get_nodal_var_time( this->CurrentHandle, varid + 1, itemID, 1, 
                                   numTimesteps, memory );
      retVal = 1;
      }
    else
      {
      }
    this->CloseCurrentFile();
    }

  if ( retVal == 0 )
    {
    // in case there was a problem, we initialize the return data, because
    // the result is expected to reflect the data we got
    result->Initialize();
    result->SetName( vName ); 
    }

  return retVal;
}

void vtkExodusReader::GetAllTimes(vtkInformationVector *outputVector) 
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  this->NumberOfTimeSteps = this->GetNumberOfTimeSteps();
  if (this->NumberOfTimeSteps == 0)
    {
    return;
    }
  float* ftimeSteps = new float[this->NumberOfTimeSteps];
  ex_get_all_times( this->CurrentHandle, ftimeSteps );
  if (this->TimeSteps)
    {
    delete [] this->TimeSteps;
    }
  this->TimeSteps = new double[this->NumberOfTimeSteps];
  for (int i=0; i<this->NumberOfTimeSteps; i++)
    {
    this->TimeSteps[i] = ftimeSteps[i];
    }
  if (!this->HasModeShapes)
    {
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), 
                 this->TimeSteps, 
                 this->NumberOfTimeSteps);
    double timeRange[2];
    timeRange[0] = this->TimeSteps[0];
    timeRange[1] = this->TimeSteps[this->NumberOfTimeSteps-1];
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), 
                 timeRange, 2);
    }
  else
    {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    double timeRange[2];
    timeRange[0] = 0;
    timeRange[1] = 1;
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
    }
  delete[] ftimeSteps;
}

int vtkExodusReader::GetPointArrayID( const char *name )
{
  int numArrays = this->GetNumberOfPointArrays();
  for ( int i=0;i<numArrays;i++ )
    {
    if ( strcmp( name, this->GetPointArrayName( i ) ) == 0 )
      {
      return i;
      }
    }
  return -1;
}

int vtkExodusReader::GetCellArrayID( const char *name )
{
  int numArrays = this->GetNumberOfCellArrays();
  for ( int i=0;i<numArrays;i++ )
    {
    if ( strcmp( name, this->GetCellArrayName( i ) ) == 0 )
      {
      return i;
      }
    }
  return -1;
}

int vtkExodusReader::GetBlockArrayID( const char *name )
{
  int numArrays = this->GetNumberOfBlockArrays();
  for ( int i=0;i<numArrays;i++ )
    {
    if ( strcmp( name, this->GetBlockArrayName( i ) ) == 0 )
      {
      return i;
      }
    }
  return -1;
}

int vtkExodusReader::GetPartArrayID( const char *name )
{
  int numArrays = this->GetNumberOfPartArrays();
  for ( int i=0;i<numArrays;i++ )
    {
    if ( strcmp( name, this->GetPartArrayName( i ) ) == 0 )
      {
      return i;
      }
    }
  return -1;
}

int vtkExodusReader::GetMaterialArrayID( const char *name )
{
  int numArrays = this->GetNumberOfMaterialArrays();
  for ( int i=0;i<numArrays;i++ )
    {
    if ( strcmp( name, this->GetMaterialArrayName( i ) ) == 0 )
      {
      return i;
      }
    }
  return -1;
}

int vtkExodusReader::OpenCurrentFile()
{
  int result = 0;

  // is there a file open now?
  if ( this->CurrentHandle == -1 && this->FileName ) 
    {
    this->CurrentHandle = ex_open( this->FileName, EX_READ, 
                                   &(this->ExodusCPUWordSize),
                                   &(this->ExodusIOWordSize), 
                                   &(this->ExodusVersion)
      );

    // Problem opening the file
    if (this->CurrentHandle < 0)
      {
      vtkErrorMacro("Problem with the ex_open function for file " << 
                    this->FileName);
      this->SetTitle( 0 );
      this->SetCurrentFileName( NULL );
      }
    else 
      {
      this->SetCurrentFileName( this->GetFileName() );
      result = 1;
      }
    }

  return result;
}

//----------------------------------------------------------------------------
//begin USE_EXO_DSP_FILTERS

int vtkExodusReader::GetNumberOfVariableArrays() 
{ 
  return( GetNumberOfPointArrays()+GetNumberOfCellArrays() ); 
};

const char *vtkExodusReader::GetVariableArrayName(int a_which)
{
  if(a_which<GetNumberOfCellArrays())
    {
    return(GetCellArrayName(a_which));
    }
  else
    {
    return(GetPointArrayName(a_which-GetNumberOfCellArrays()));
    }
}

void vtkExodusReader::EnableDSPFiltering() 
{ 
  this->DSPFilteringIsEnabled=1;
  if(!this->DSPFilters && this->GetNumberOfBlockArrays() )
    {
    this->DSPFilters = new vtkDSPFilterGroup*[this->GetNumberOfBlockArrays()];
    for(int i=0;i<this->GetNumberOfBlockArrays();i++)
      {
      this->DSPFilters[i] = vtkDSPFilterGroup::New();
      }
    }
};

void vtkExodusReader::AddFilter(vtkDSPFilterDefinition *a_filter) 
{
  //printf("vtkExodusReader(%p)::addFilter %s -> %s   numDSPFilters=%d  GetNumberOfBlockArrays()=%d\n",this,
  // a_filter->GetInputVariableName(),a_filter->GetOutputVariableName(), 
  // this->DSPFilters[0]->GetNumFilters(), this->GetNumberOfBlockArrays() );

  this->DSPFilteringIsEnabled = 1;//Is this var necessary any more?

  //allocate if necessary.....is this necessary?
  if(!this->DSPFilters && this->GetNumberOfBlockArrays())
    {
    this->DSPFilters = new vtkDSPFilterGroup*[this->GetNumberOfBlockArrays()];
    for(int i=0;i<this->GetNumberOfBlockArrays();i++)
      {
      this->DSPFilters[i] = vtkDSPFilterGroup::New();
      }
    }
  if(!this->DSPFilters) return;

  for(int i=0;i<this->GetNumberOfBlockArrays();i++)
    {
    this->DSPFilters[i]->AddFilter(a_filter);
    }

  //printf("after vtkExodusReader(%p)::addFilter %s -> %s   numDSPFilters=%d  GetNumberOfBlockArrays()=%d\n",this,
  //a_filter->GetInputVariableName(),a_filter->GetOutputVariableName(), 
  //this->DSPFilters[0]->GetNumFilters(), this->GetNumberOfBlockArrays() );

  this->Modified();//need to do this to cause the new filter to be calculated
};

//----------------------------------------------------------------------------
void vtkExodusReader::RemoveFilter(char *a_outputVariableName)
{
  //allocate if necessary.....is this necessary?
  if(!this->DSPFilters && this->GetNumberOfBlockArrays())
    {
    this->DSPFilters = new vtkDSPFilterGroup*[this->GetNumberOfBlockArrays()];
    for(int i=0;i<this->GetNumberOfBlockArrays();i++)
      {
      this->DSPFilters[i] = vtkDSPFilterGroup::New();
      }
    }
  if(!this->DSPFilters) return;

  for(int i=0;i<this->GetNumberOfBlockArrays();i++)
    {
    this->DSPFilters[i]->RemoveFilter(a_outputVariableName);
    }

  this->Modified();//need to do this to cause the old filter to be removed
}

void vtkExodusReader::StartAddingFilter()
{
  this->AddingFilter->Clear();
}
void vtkExodusReader::AddFilterInputVar(char *name)
{
  this->AddingFilter->SetInputVariableName( name );
}
void vtkExodusReader::AddFilterOutputVar(char *name)
{
  this->AddingFilter->SetOutputVariableName( name );
}
void vtkExodusReader::AddFilterNumeratorWeight(double weight)
{
  this->AddingFilter->PushBackNumeratorWeight(weight);
}
void vtkExodusReader::AddFilterForwardNumeratorWeight(double weight)
{
  this->AddingFilter->PushBackForwardNumeratorWeight(weight);
}
void vtkExodusReader::AddFilterDenominatorWeight(double weight)
{
  this->AddingFilter->PushBackDenominatorWeight(weight);
}
void vtkExodusReader::FinishAddingFilter()
{
  this->AddFilter(AddingFilter);
}

vtkExodusReader::ArrayType vtkExodusReader::GetArrayTypeID( const char *type )
{
  if ( strcmp( type, "CELL" ) == 0 )
    {
    return vtkExodusReader::CELL;
    }
  else if ( strcmp( type, "POINT" ) == 0 )
    {
    return vtkExodusReader::POINT;
    }
  else if ( strcmp( type, "BLOCK" ) == 0 )
    {
    return vtkExodusReader::BLOCK;
    }
  else if ( strcmp( type, "PART" ) == 0 )
    {
    return vtkExodusReader::PART;
    }
  else if ( strcmp( type, "MATERIAL" ) == 0 )
    {
    return vtkExodusReader::MATERIAL;
    }
  else if ( strcmp( type, "ASSEMBLY" ) == 0 )
    {
    return vtkExodusReader::ASSEMBLY;
    }
  else if ( strcmp( type, "HIERARCHY" ) == 0 )
    {
    return vtkExodusReader::HIERARCHY;
    }
  else 
    {
    return vtkExodusReader::UNKNOWN_TYPE;
    }
}

void vtkExodusReader::GetDSPOutputArrays(int handle, vtkUnstructuredGrid* output)
{

  //printf("\nvtkExodusReader(%p)::GetDSPOutputArrays numDSPFilters=%d numblocks=%d isenabled=%d  numvars=%d  num pt arrays=%d  num cell arrays=%d\n\n",
  // this,this->DSPFilters[0]->GetNumFilters(),this->GetNumberOfBlockArrays(),
  // this->DSPFilteringIsEnabled,this->GetNumberOfVariableArrays(),
  // this->GetOutput()->GetPointData()->GetNumberOfArrays(), this->GetOutput()->GetCellData()->GetNumberOfArrays() );

  //allocate if necessary.....is this necessary?
  if(!this->DSPFilters && this->GetNumberOfBlockArrays())
    {
    this->DSPFilters = new vtkDSPFilterGroup*[this->GetNumberOfBlockArrays()];
    for(int i=0;i<this->GetNumberOfBlockArrays();i++)
      {
      this->DSPFilters[i] = vtkDSPFilterGroup::New();
      }
    }

  if(this->DSPFilteringIsEnabled && this->DSPFilters)
    {
    //printf("in vtkExodusReader::GetDSPOutputArrays DSPFilters IS allocated\n");

    int l_numPointVarInstancesLoaded=0;
    int l_numCellVarInstancesLoaded=0;
    int i,j;

    //GET ALL THE INPUTS
    //This is a brute force approach, but will never be problem-sized
    for(int l_whichVar=0;
        l_whichVar<this->GetNumberOfVariableArrays();
        l_whichVar++)
      {
      const char *l_name = this->GetVariableArrayName(l_whichVar);
      for(int l_whichTime=0;
          l_whichTime<this->GetNumberOfTimeSteps();
          l_whichTime++)
        {
        //assuming all blocks' filters have same needs
        int l_needed = 
          this->DSPFilters[0]->IsThisInputVariableInstanceNeeded(
            l_name,l_whichTime,this->ActualTimeStep);
        if(l_needed) 
          {
          //cannot assume all blocks' filters have the same cache,
          //because a block may have been turned off before
          for(i=0;i<this->GetNumberOfBlockArrays();i++)
            {
            //XXX STILL NEED TO HANDLE TRUTH TABLES FOR SHIP.EXO
            if( this->GetBlockArrayStatus(i) )
              {
              int l_cached = 
                this->DSPFilters[i]->IsThisInputVariableInstanceCached(
                  l_name,l_whichTime);
              if(!l_cached)
                {
                //Get the type of var, and the index of var
                int l_varIndex=-1;
                int l_pointArrayIndex=0, l_cellArrayIndex=0;//this is confusing
                int l_isPointArray=0;
                for(j=0;j<this->GetNumberOfPointArrays();j++)
                  {
                  if(!strcmp(this->GetPointArrayName(j),l_name))
                    {
                    l_varIndex=j;
                    l_isPointArray=1;
                    break;
                    }
                  l_pointArrayIndex += this->GetPointArrayNumberOfComponents(j);
                  }
                for(j=0;j<this->GetNumberOfCellArrays();j++)
                  {
                  if(!strcmp(this->GetCellArrayName(j),l_name))
                    {
                    if(l_varIndex>=0)
                      {           
                      vtkErrorMacro(
                        "Apparently there are cell and point vars with same name: " 
                        << l_name);
                      break;
                      }
                    l_varIndex=j;
                    l_isPointArray=0;
                    break;
                    }
                  l_cellArrayIndex += this->GetCellArrayNumberOfComponents(j);
                  }
                if(l_varIndex<0)
                  {           
                  vtkErrorMacro(
                    "Cant find cell or point vars with name: " << l_name);
                  break;
                  }



                if(l_isPointArray)
                  {
                  //Cant use l_varIndex here, because the output's 
                  // 'Point Data' may
                  //not have all the vars that the actual input Point Data has
                  vtkDataArray *l_array=output->GetPointData()->GetArray(l_name);
        
                  if(!l_array)
                    {
                    int l_dim = 
                      this->GetPointArrayNumberOfComponents(l_varIndex);

                    if (l_dim == 1)
                      l_array = this->ReadPointArray(handle, l_pointArrayIndex);
                    else
                      l_array = this->ReadPointVector(
                        handle, l_pointArrayIndex, l_dim);
                    }
                  if(!l_array)
                    {           
                    vtkErrorMacro("Cant get point array: " << l_name);
                    break;
                    }

                  if(!l_array->GetNumberOfComponents() || 
                     !l_array->GetNumberOfTuples())
                    {           
                    vtkErrorMacro("Zero sized point array: " << l_name);
                    break;
                    }

      
                  int l_type = l_array->GetDataType();
                  if( l_type!=VTK_FLOAT) 
                    {
                    printf("vtkExodusReader::GetDSPOutputArrays can only do "
                           "floats for now (type=%d)\n",l_type);
                    }
                  else
                    {
                    vtkFloatArray *l_floatArray = 
                      static_cast<vtkFloatArray *> (l_array);
                    this->DSPFilters[i]->AddInputVariableInstance(
                      l_name,l_whichTime,l_floatArray);

                    l_numPointVarInstancesLoaded++;
                    }

          
                  //l_array->Delete(); //DONT DELETE HERE 26aug
          
                  /*END OF POINT ARRAY PART*/ 
                  }
                else
                  {
                  //BEGIN CELL ARRAY PART
                  //Cant use l_varIndex here, because the output's 'Point Data' may
                  //not have all the vars that the actual input Point Data has
                  vtkDataArray *l_array = 
                    output->GetCellData()->GetArray(l_name);
                  if(!l_array)
                    {
                    int l_dim = this->GetCellArrayNumberOfComponents(l_varIndex);

                    if (l_dim == 1)
                      l_array = this->ReadCellArray(handle, l_cellArrayIndex);
                    else
                      l_array = 
                        this->ReadCellVector(handle, l_cellArrayIndex, l_dim);
                    }
                  if(!l_array)
                    {           
                    vtkErrorMacro("Cant get cell array: " << l_name);
                    break;
                    }

                  if(!l_array->GetNumberOfComponents() || 
                     !l_array->GetNumberOfTuples())
                    {           
                    vtkErrorMacro("Zero sized cell array: " << l_name);
                    break;
                    }         



                  int l_type = l_array->GetDataType();
                  if( l_type!=VTK_FLOAT) 
                    {
                    printf("vtkExodusReader::GetDSPOutputArrays can only "
                           "do floats for now (type=%d)\n",l_type);
                    }
                  else
                    {
                    vtkFloatArray *l_floatArray = 
                      static_cast<vtkFloatArray *> (l_array);
                    this->DSPFilters[i]->AddInputVariableInstance(
                      l_name,l_whichTime,l_floatArray);

                    l_numCellVarInstancesLoaded++;
                    }

          
                  //l_array->Delete(); //DONT DELETE HERE 26aug

                  /*END OF CELL ARRAY PART*/

                  }         
                }
              //else printf("...vtkExodusReader DSP FILTERING not loading time %d %s for block %d of %d ALREADY CACHED\n",l_whichTime,l_name,i,this->GetNumberOfBlockArrays());

              }
            //else  printf("...vtkExodusReader DSP FILTERING time %d %s for block %d of %d HAS 0 STATUS\n",l_whichTime,l_name,i,this->GetNumberOfBlockArrays());
            }
          }
        //else  printf("...vtkExodusReader DSP FILTERING time %d %s NOT NEEDED\n",l_whichTime,l_name);
        }
      }
    printf("vtkExodusReader::GetDSPOutputArrays() read %d dsp POINT "
           "input variable instances\n",l_numPointVarInstancesLoaded);
    printf("vtkExodusReader::GetDSPOutputArrays() read %d dsp CELL "
           "input variable instances\n",l_numCellVarInstancesLoaded);
    
    //CALCULATE THE OUTPUTS
    int l_numCalculated=0;
    int l_numFilters = (int) this->DSPFilters[0]->GetNumFilters();
    for(i=0;i<this->GetNumberOfBlockArrays();i++)
      {
      for(j=0;j<l_numFilters;j++)
        {
        //Figure out whether the input (and therefore output) var
        //is cell or point
        int l_isCellVar=1;
        int l_var=-1;
        for(l_var=0;l_var<this->GetNumberOfPointArrays();l_var++)
          {
          if(!strcmp(this->GetPointArrayName(l_var),
                     this->DSPFilters[i]->GetInputVariableName(j) ))
            {
            l_isCellVar=0;
            break;
            } 
          }

        vtkFloatArray *l_array=NULL;
        if( this->GetBlockArrayStatus(i) )
          {
          l_array = this->DSPFilters[i]->GetOutput( 
            j, this->ActualTimeStep, l_numCalculated );
          }



        if(l_array)
          {


#if 0 //get the min/max and print it out
          int l_datatype = l_array->GetDataType();
          if( l_datatype==VTK_FLOAT) 
            {         
            for(int l_comp=0;l_comp<l_array->GetNumberOfComponents();l_comp++)
              {
              float l_min,l_max;
              float *l_data = (float *)l_array->GetVoidPointer(0);
        
              for(int l_tup=0;l_tup<l_array->GetNumberOfTuples();l_tup++)
                {
                float l_val = 
                  l_data[l_tup*l_array->GetNumberOfComponents()+l_comp];
                if(!l_tup)
                  {
                  l_min=l_max=l_val;
                  }
                else if(l_min>l_val)
                  {
                  l_min=l_val;
                  }
                else if(l_max<l_val)
                  {
                  l_max=l_val;
                  }
                }
              printf("*****************comp=%d  min=%f max=%f\n",l_comp,l_min,l_max);
              }
            }

#endif




          if(!l_isCellVar)
            {
            output->GetPointData()->AddArray(l_array);
            //printf("added dsp point var block %d\n",i);
            }
          else
            {
            output->GetCellData()->AddArray(l_array);
            //printf("added dsp cell var block %d\n",i);
            }
          }

        //printf("block %d AFTER DSP  num pt arrays = %d, num cell arrays = %d    num pts=%d  num cells=%d\n", i,
        // output->GetPointData()->GetNumberOfArrays(), output->GetCellData()->GetNumberOfArrays(),
        // output->GetPoints()->GetNumberOfPoints(), output->GetNumberOfCells() );




        }
      }

    printf("---vtkExodusReader::GetDSPOutputArrays() calculated %d dsp output "
           "variable instances---\n",l_numCalculated);

    }


}
//end USE_EXO_DSP_FILTERS


int vtkExodusReader::IsValidVariable( const char *type, const char *name )
{
  if ( this->GetVariableID( type, name ) != -1 )
    {
    return 1;
    }

  return 0;
}

int vtkExodusReader::GetVariableID ( const char *type, const char *name )
{
  ArrayType typeID = this->GetArrayTypeID( type );

  int res = -1;
  switch ( typeID )
    {
    case vtkExodusReader::CELL:
      res = this->GetCellArrayID( name );
      break;
    case vtkExodusReader::POINT:
      res = this->GetPointArrayID( name );
      break;
    case vtkExodusReader::BLOCK:
      res = this->GetBlockArrayID( name );
      break;
    case vtkExodusReader::PART:
      res = this->GetPartArrayID( name );
      break;
    case vtkExodusReader::MATERIAL:
      res = this->GetMaterialArrayID( name );
      break;
    case vtkExodusReader::ASSEMBLY:
      res = this->GetAssemblyArrayID( name );
      break;
    case vtkExodusReader::HIERARCHY:
    case vtkExodusReader::NUM_ARRAY_TYPES:
    case vtkExodusReader::UNKNOWN_TYPE:
    default:
      ;
    }
  return res;
}

int vtkExodusReader::GetGlobalElementID ( vtkDataSet *data, int localID )
{
  return vtkExodusReader::GetIDHelper( 
    vtkExodusReader::GetGlobalElementIdArrayName(), 
    data, localID, vtkExodusReader::SEARCH_TYPE_ELEMENT );
}

int vtkExodusReader::GetGlobalElementID ( vtkDataSet *data, int localID,
                                          int searchType )
{
  return vtkExodusReader::GetGlobalID( 
    vtkExodusReader::GetGlobalElementIdArrayName(), data, 
    localID, searchType );
}

int vtkExodusReader::GetGlobalNodeID ( vtkDataSet *data, int localID )
{
  return vtkExodusReader::GetIDHelper( 
    vtkExodusReader::GetGlobalNodeIdArrayName(),
    data, localID, vtkExodusReader::SEARCH_TYPE_NODE );
}

int vtkExodusReader::GetGlobalNodeID ( vtkDataSet *data, int localID,
                                       int searchType )
{
  return vtkExodusReader::GetGlobalID( 
    vtkExodusReader::GetGlobalNodeIdArrayName(), data, 
    localID, searchType );
}

int vtkExodusReader::GetGlobalID ( const char *arrayName, vtkDataSet *data, 
                                   int localID, int searchType )
{
  int newID = vtkExodusReader::ID_NOT_FOUND;  
  switch ( searchType )
    {
    case vtkExodusReader::SEARCH_TYPE_ELEMENT:
    case vtkExodusReader::SEARCH_TYPE_NODE:
      newID = vtkExodusReader::GetIDHelper( arrayName, data, localID, searchType );
      break;
    case vtkExodusReader::SEARCH_TYPE_ELEMENT_THEN_NODE:
      // if this search fails ... 
      newID = vtkExodusReader::GetIDHelper( arrayName,  data, localID, 
                                            vtkExodusReader::SEARCH_TYPE_ELEMENT ); 
      if ( newID == vtkExodusReader::ID_NOT_FOUND )
        {
        // try this search 
        newID = vtkExodusReader::GetIDHelper( arrayName, data, localID, 
                                              vtkExodusReader::SEARCH_TYPE_NODE ); 
        }
      break;
    case vtkExodusReader::SEARCH_TYPE_NODE_THEN_ELEMENT:
      // if this search fails ... 
      newID = vtkExodusReader::GetIDHelper( arrayName, data, localID, 
                                            vtkExodusReader::SEARCH_TYPE_NODE ); 
      if ( newID == vtkExodusReader::ID_NOT_FOUND )
        {
        // try this search 
        newID = vtkExodusReader::GetIDHelper( arrayName, data, localID, 
                                              vtkExodusReader::SEARCH_TYPE_ELEMENT ); 
        }
      break;
    }
  return newID;
}

int vtkExodusReader::GetIDHelper ( const char *arrayName, vtkDataSet *data, 
                                   int localID, int searchType )
{
  int newID = vtkExodusReader::ID_NOT_FOUND;  
  if ( data )
    {
    vtkDataArray *IDs = NULL;
    vtkCellData *cData = data->GetCellData();
    vtkPointData *pData = data->GetPointData();
    if ( searchType == vtkExodusReader::SEARCH_TYPE_ELEMENT )
      {
      IDs = cData->GetScalars( arrayName ); 
      }
    else if ( searchType == vtkExodusReader::SEARCH_TYPE_NODE )
      {
      IDs = pData->GetScalars( arrayName ); 
      }
    if ( IDs )
      {
      if ( ( localID >=0 ) && ( localID < IDs->GetNumberOfTuples() ) )
        {
        newID = (int)( IDs->GetTuple1( localID ) );
        }
      }
    }
  return newID;
}
