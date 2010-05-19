
/*=========================================================================

  Program:   ParaView
  Module:    vtkModelMetadata.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkModelMetadata.h"
#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkFieldData.h"
#include "vtkCharArray.h"
#include "vtkFloatArray.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkIdList.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include <time.h>

vtkStandardNewMacro(vtkModelMetadata);

#include <vtkstd/set>
#include <vtkstd/map>
#include <vtkstd/algorithm>

class vtkModelMetadataSTLCloak
{
public:
  vtkstd::set<int> IntSet;
  vtkstd::map<int, int> IntMap; 
};


#undef FREE
#undef FREELIST

#define FREE(x) \
  if (x)        \
    {           \
    delete [] x;\
    x = NULL;   \
    }        

#define FREELIST(x, len)       \
  if (x && len)                \
    {                          \
    for (i=0; i<len; i++)      \
      {                        \
      if (x[i]) delete [] x[i];\
      }                        \
    delete [] x;               \
    x = NULL;                  \
    }

void vtkModelMetadata::InitializeAllMetadata()
{
  this->Title = NULL;

  this->NumberOfQARecords = 0;
  this->QARecord = NULL;

  this->NumberOfInformationLines = 0;
  this->InformationLine = NULL;

  this->Dimension = 0;
  this->CoordinateNames = NULL;

  this->TimeStepIndex = -1;
  this->NumberOfTimeSteps = 0;
  this->TimeStepValues = NULL;

  this->NumberOfBlocks = 0;

  this->BlockIds = NULL;
  this->BlockElementType = NULL;
  this->BlockNumberOfElements = NULL;
  this->BlockNodesPerElement = NULL;
  this->BlockNumberOfAttributesPerElement = NULL;
  this->BlockAttributes = NULL;
  this->BlockElementIdList = NULL;
 
  this->NumberOfNodeSets = 0;

  this->NodeSetIds = NULL;
  this->NodeSetSize = NULL;
  this->NodeSetNumberOfDistributionFactors = NULL;
  this->NodeSetNodeIdList = NULL;
  this->NodeSetDistributionFactors = NULL;

  this->NodeSetNodeIdListIndex = NULL;
  this->NodeSetDistributionFactorIndex = NULL;

  this->NumberOfSideSets = 0;
  
  this->SideSetIds = NULL;
  this->SideSetSize = NULL;
  this->SideSetNumberOfDistributionFactors = NULL;
  this->SideSetElementList = NULL;
  this->SideSetSideList = NULL;
  this->SideSetNumDFPerSide = NULL;
  this->SideSetDistributionFactors = NULL;

  this->SideSetListIndex = NULL;
  this->SideSetDistributionFactorIndex = NULL; 

  this->NumberOfBlockProperties = 0;
  this->BlockPropertyNames = NULL;
  this->BlockPropertyValue = NULL;

  this->NumberOfNodeSetProperties = 0;
  this->NodeSetPropertyNames = NULL;
  this->NodeSetPropertyValue = NULL;

  this->NumberOfSideSetProperties = 0;
  this->SideSetPropertyNames = NULL;
  this->SideSetPropertyValue = NULL;

  this->NumberOfGlobalVariables = 0;
  this->GlobalVariableNames = NULL;
  this->GlobalVariableValue = NULL;

  this->OriginalNumberOfElementVariables = 0;
  this->OriginalElementVariableNames = NULL;
  this->NumberOfElementVariables = 0;
  this->MaxNumberOfElementVariables = 0;
  this->ElementVariableNames = NULL;
  this->ElementVariableNumberOfComponents = NULL;
  this->MapToOriginalElementVariableNames = NULL;
  
  this->ElementVariableTruthTable = NULL;

  this->OriginalNumberOfNodeVariables = 0;
  this->OriginalNodeVariableNames = NULL;
  this->NumberOfNodeVariables = 0;
  this->MaxNumberOfNodeVariables = 0;
  this->NodeVariableNames = NULL;
  this->NodeVariableNumberOfComponents = NULL;
  this->MapToOriginalNodeVariableNames = NULL;
}
void vtkModelMetadata::InitializeAllIvars()
{
  this->InitializeAllMetadata();

  this->SumElementsPerBlock = 0;
  this->BlockElementIdListIndex = NULL;

  this->SizeBlockAttributeArray = 0;
  this->BlockAttributesIndex = NULL;

  this->SumNodesPerNodeSet = 0;
  this->SumDistFactPerNodeSet = 0;

  this->NodeSetNodeIdListIndex = NULL;
  this->NodeSetDistributionFactorIndex = NULL;

  this->SumSidesPerSideSet = 0;
  this->SumDistFactPerSideSet = 0;

  this->SideSetListIndex = NULL;
  this->SideSetDistributionFactorIndex = NULL;

  this->AllVariablesDefinedInAllBlocks = 0;

  this->BlockIdIndex = NULL;
}
void vtkModelMetadata::FreeAllGlobalData()
{
  // All the fields which apply to the whole data set, and so
  // don't differ depending on which time step, which blocks,
  // or which variables are read in.

  this->SetTitle(NULL);
  this->FreeQARecords();
  this->SetInformationLines(0, NULL);

  this->SetCoordinateNames(0, NULL);
  this->SetTimeSteps(0, NULL);

  this->SetBlockIds(NULL);
  this->SetBlockElementType(NULL);
  this->SetBlockNodesPerElement(NULL);
  this->SetBlockNumberOfAttributesPerElement(NULL);

  if (this->BlockIdIndex)
    {
    delete this->BlockIdIndex;
    this->BlockIdIndex = NULL;
    }

  this->SetNodeSetIds(NULL);
  this->SetSideSetIds(NULL);

  this->SetBlockPropertyNames(0, NULL);
  this->SetBlockPropertyValue(NULL);
  this->SetNodeSetPropertyNames(0, NULL);
  this->SetNodeSetPropertyValue(NULL);
  this->SetSideSetPropertyNames(0, NULL);
  this->SetSideSetPropertyValue(NULL);
  this->SetGlobalVariableNames(0, NULL);

  this->SetElementVariableTruthTable(NULL);

  this->FreeOriginalElementVariableNames();
  this->FreeOriginalNodeVariableNames();
}

void vtkModelMetadata::FreeAllLocalData()
{
  // All the fields that depend on which blocks, which time step,
  // and which variables were read in.

  this->FreeBlockDependentData();          // depends on blocks
  this->FreeUsedElementVariables();        // depends on variables
  this->FreeUsedNodeVariables();
  this->SetGlobalVariableValue(NULL);      // depends on time step
}

void vtkModelMetadata::FreeBlockDependentData()
{
  // All the fields that depend on exactly which blocks
  // are in the vtkUnstructuredGrid

  this->SetBlockNumberOfElements(NULL);
  this->SetBlockElementIdList(NULL);
  this->SetBlockAttributes(NULL);

  this->SetNodeSetSize(NULL);
  this->SetNodeSetNumberOfDistributionFactors(NULL);
  this->SetNodeSetNodeIdList(NULL);
  this->SetNodeSetDistributionFactors(NULL);

  this->SetSideSetSize(NULL);
  this->SetSideSetNumberOfDistributionFactors(NULL);
  this->SetSideSetElementList(NULL);
  this->SetSideSetSideList(NULL);
  this->SetSideSetNumDFPerSide(NULL);
  this->SetSideSetDistributionFactors(NULL);
}

void vtkModelMetadata::FreeOriginalElementVariableNames()
{
  int i;
  FREELIST(this->OriginalElementVariableNames, 
           this->OriginalNumberOfElementVariables);
}
void vtkModelMetadata::FreeOriginalNodeVariableNames()
{
  int i;
  FREELIST(this->OriginalNodeVariableNames, 
           this->OriginalNumberOfNodeVariables);
}
void vtkModelMetadata::FreeUsedElementVariableNames()
{
  int i;
  FREELIST(this->ElementVariableNames,
           this->MaxNumberOfElementVariables); 
}

void vtkModelMetadata::FreeUsedNodeVariableNames()
{
  int i;
  FREELIST(this->NodeVariableNames,
           this->MaxNumberOfNodeVariables);
}
void vtkModelMetadata::FreeUsedElementVariables()
{
  this->FreeUsedElementVariableNames();
  FREE(this->ElementVariableNumberOfComponents);
  FREE(this->MapToOriginalElementVariableNames);
}
void vtkModelMetadata::FreeUsedNodeVariables()
{
  this->FreeUsedNodeVariableNames();
  FREE(this->NodeVariableNumberOfComponents);
  FREE(this->MapToOriginalNodeVariableNames);
}

void vtkModelMetadata::FreeAllMetadata()
{
  this->FreeAllLocalData();
  this->FreeAllGlobalData();
}
void vtkModelMetadata::FreeAllIvars()
{
  this->FreeAllMetadata();

  FREE(this->BlockElementIdListIndex);
  FREE(this->BlockAttributesIndex);
  FREE(this->NodeSetNodeIdListIndex);
  FREE(this->NodeSetDistributionFactorIndex);
  FREE(this->SideSetListIndex);
  FREE(this->SideSetDistributionFactorIndex);
}
//
// At last: The constructor, destructor and copy operator
//
vtkModelMetadata::vtkModelMetadata()
{
  this->InitializeAllIvars();
}
vtkModelMetadata::~vtkModelMetadata()
{
  this->FreeAllIvars();
}
void vtkModelMetadata::Reset()
{
  this->FreeAllIvars();
  this->InitializeAllIvars();
}

//-------------------------------------------------
// information  && QA fields
//-------------------------------------------------

void vtkModelMetadata::SetInformationLines(int nlines, char **lines)
{
  int i;

  FREELIST(this->InformationLine, this->NumberOfInformationLines);

  this->NumberOfInformationLines = nlines;
  this->InformationLine = lines;
}
void vtkModelMetadata::AddInformationLine(char *line)
{
  int newNum = this->NumberOfInformationLines + 1;
  char **lines = new char * [newNum];

  for (int i=0; i<this->NumberOfInformationLines; i++)
    {
    lines[i] = this->InformationLine[i];
    }

  lines[newNum-1] = line;

  delete [] this->InformationLine;

  this->InformationLine = lines;
}
int vtkModelMetadata::GetInformationLines(char ***lines) const
{
  *lines = this->InformationLine;

  return this->NumberOfInformationLines;
}

void vtkModelMetadata::FreeQARecords()
{
  if ((this->NumberOfQARecords > 0) && this->QARecord)
    {
    for (int i=0; i<this->NumberOfQARecords; i++)
      {
      if (this->QARecord[i])
        {
        delete [] this->QARecord[i][0];
        delete [] this->QARecord[i][1];
        delete [] this->QARecord[i][2];
        delete [] this->QARecord[i][3];
        }
      }
    delete [] this->QARecord;
    }
  this->QARecord = NULL;
  this->NumberOfQARecords = 0;
}
void vtkModelMetadata::SetQARecords(int nrecords, char *rec[][4])
{
  this->FreeQARecords();

  if (nrecords > 0)
    {
    this->QARecord = rec;
    this->NumberOfQARecords = nrecords;
    }
}
void vtkModelMetadata::AddQARecord(char *name, char *ver, char *recDate, char *recTime)
{
  typedef char *p4[4];

  int newNum = this->NumberOfQARecords + 1;

  p4 *qaRecs = new p4 [newNum];

  for (int i=0; i<this->NumberOfQARecords; i++)
    {
    qaRecs[i][0] = this->QARecord[i][0]; qaRecs[i][1] = this->QARecord[i][1];
    qaRecs[i][2] = this->QARecord[i][2]; qaRecs[i][3] = this->QARecord[i][3];
    }

  delete [] this->QARecord;

  if ((recDate == NULL) || (recTime == NULL))
    {
    time_t currentTime = time(NULL);
    struct tm *timeDate = localtime(&currentTime);

    if (timeDate->tm_year >= 100) timeDate->tm_year -= 100;  // Y2K 

    if (recDate == NULL)
      {
      char *dateStr = new char [10];
      sprintf(dateStr, "%02d/%02d/%02d",
        timeDate->tm_mon, timeDate->tm_mday, timeDate->tm_year);
      qaRecs[newNum-1][2] = dateStr;
      }

    if (recTime == NULL)
      {
      char *timeStr = new char [10];
      sprintf(timeStr, "%02d:%02d:%02d",
        timeDate->tm_hour, timeDate->tm_min, timeDate->tm_sec);
      qaRecs[newNum-1][3] = timeStr;
      }
    }

  qaRecs[newNum-1][0] = name;
  qaRecs[newNum-1][1] = ver;

  if (recDate) qaRecs[newNum-1][2] = recDate;
  if (recTime) qaRecs[newNum-1][3] = recTime;

  this->QARecord = qaRecs;

  this->NumberOfQARecords = newNum;
}
void vtkModelMetadata::GetQARecord(int which, 
               char **name, char **ver, char **date, char **time) const
{
  if ( which >= this->NumberOfQARecords)
    {
    return;
    }

  *name = this->QARecord[which][0];
  *ver  = this->QARecord[which][1];
  *date = this->QARecord[which][2];
  *time = this->QARecord[which][3];
}

void vtkModelMetadata::SetTimeSteps(int num, float *steps)
{
  FREE(this->TimeStepValues);

  this->NumberOfTimeSteps = num;
  this->TimeStepValues = steps;
}

void vtkModelMetadata::SetCoordinateNames(int dimension, char **n)
{
  int i;
  if (this->CoordinateNames)
    {
    FREELIST(this->CoordinateNames, this->Dimension);
    }

  this->CoordinateNames = n;
  this->Dimension = dimension;
}

//-------------------------------------------------
// Blocks
//-------------------------------------------------
int vtkModelMetadata::GetBlockLocalIndex(int id)
{
  if (this->BlockIdIndex == NULL)
     {
     this->BlockIdIndex = new vtkModelMetadataSTLCloak;
     }

  vtkstd::map<int, int> blockIdIndex = this->BlockIdIndex->IntMap;

  if (blockIdIndex.size() == 0)
    {
    for (int i=0; i<this->NumberOfBlocks; i++)
      {
      int gid = this->BlockIds[i];

      blockIdIndex.insert(vtkstd::map<int,int>::value_type(gid, i));
      }
    }

  vtkstd::map<int,int>::iterator mapit = blockIdIndex.find(id);

  int retVal = -1;

  if (mapit != blockIdIndex.end())
    {
    retVal = mapit->second;
    }

  return retVal;
}

void vtkModelMetadata::SetBlockIds(int *b)
{
  FREE(this->BlockIds);

  this->BlockIds = b;
}
void vtkModelMetadata::SetBlockElementType(char **t)
{
  int i;
    
  FREELIST(this->BlockElementType, this->NumberOfBlocks);

  this->BlockElementType = t;
}
void vtkModelMetadata::SetBlockNodesPerElement(int *e)
{
  FREE(this->BlockNodesPerElement);

  this->BlockNodesPerElement = e;
}
void vtkModelMetadata::SetBlockElementIdList(int *e)
{
  FREE(this->BlockElementIdList);

  this->BlockElementIdList = e;
}
void vtkModelMetadata::SetBlockAttributes(float *a)
{
  FREE(this->BlockAttributes);

  this->BlockAttributes = a;
}
int vtkModelMetadata::BuildBlockAttributesIndex()
{
  int nblocks = this->NumberOfBlocks;
  int *nelts = this->BlockNumberOfElements;
  int *natts = this->BlockNumberOfAttributesPerElement;

  if ((nblocks < 1) || !nelts || !natts)
    {
    return 1;
    }

  if (this->BlockAttributesIndex)
    {
    delete [] this->BlockAttributesIndex;
    }

  this->BlockAttributesIndex = new int [nblocks];

  int idx = 0;

  for (int i=0; i<nblocks; i++)
    {
    this->BlockAttributesIndex[i] = idx;
    idx += (int)(nelts[i] * natts[i]);
    }

  this->SizeBlockAttributeArray = idx;

  return 0;
}
int vtkModelMetadata::BuildBlockElementIdListIndex()
{
  int nblocks = this->NumberOfBlocks;
  int *size = this->BlockNumberOfElements;

  if ((nblocks < 1) || !size)
    {
    return 1;
    }

  if (this->BlockElementIdListIndex)
    {
    delete [] this->BlockElementIdListIndex;
    }
  this->BlockElementIdListIndex = new int [ nblocks ];

  int idx = 0;
  for (int i=0; i<nblocks; i++)
    {
    this->BlockElementIdListIndex[i] = idx;
    idx += size[i];      
    }

  this->SumElementsPerBlock = idx;
  return 0;
}
int vtkModelMetadata::SetBlockNumberOfElements(int *nelts)
{
  FREE(this->BlockNumberOfElements);

  if (nelts)
    {
    this->BlockNumberOfElements = nelts;

    this->BuildBlockAttributesIndex();
    this->BuildBlockElementIdListIndex();
    }

  return 0;
}
int vtkModelMetadata::SetBlockNumberOfAttributesPerElement(int *natts)
{
  FREE(this->BlockNumberOfAttributesPerElement);

  if (natts)
    {
    this->BlockNumberOfAttributesPerElement = natts;

    this->BuildBlockAttributesIndex();
    }

  return 0;
}

//-------------------------------------------------
// node set calculations
//-------------------------------------------------

void vtkModelMetadata::SetNodeSetIds(int *n)
{
  FREE(this->NodeSetIds);

  this->NodeSetIds = n;
}
void vtkModelMetadata::SetNodeSetNodeIdList(int *n)
{
  FREE(this->NodeSetNodeIdList);

  this->NodeSetNodeIdList = n;
}
void vtkModelMetadata::SetNodeSetDistributionFactors(float *d)
{
  FREE(this->NodeSetDistributionFactors);

  this->NodeSetDistributionFactors = d;
}
int vtkModelMetadata::BuildNodeSetNodeIdListIndex()
{
  int nsets = this->NumberOfNodeSets;
  int *size = this->NodeSetSize;

  if ((nsets < 1) || !size)
    {
    return 1;
    }

  if (this->NodeSetNodeIdListIndex)
    {
    delete [] this->NodeSetNodeIdListIndex;
    }
  this->NodeSetNodeIdListIndex = new int [ nsets ];

  int idx = 0;
  for (int i=0; i<nsets; i++)
    {
    this->NodeSetNodeIdListIndex[i] = idx;
    idx += size[i];      
    }

  this->SumNodesPerNodeSet = idx;
  return 0;
}
int vtkModelMetadata::BuildNodeSetDistributionFactorIndex()
{
  int nsets = this->NumberOfNodeSets;
  int *numFactors = this->NodeSetNumberOfDistributionFactors;

  if ((nsets < 1) || !numFactors)
    {
    return 1;
    }

  if (this->NodeSetDistributionFactorIndex)
    {
    delete [] this->NodeSetDistributionFactorIndex;
    }
  this->NodeSetDistributionFactorIndex = new int [ nsets ];

  int idx = 0;

  for (int i=0; i<nsets; i++)
    {
    this->NodeSetDistributionFactorIndex[i] = idx;

    idx += numFactors[i];
    }

  this->SumDistFactPerNodeSet = idx;

  return 0;
}
int vtkModelMetadata::SetNodeSetSize(int *size)
{
  FREE(this->NodeSetSize);

  if (size)
    {
    this->NodeSetSize = size;

    this->BuildNodeSetNodeIdListIndex();
    }

  return 0;
}
int vtkModelMetadata::SetNodeSetNumberOfDistributionFactors(int *df)
{
  FREE(this->NodeSetNumberOfDistributionFactors);

  if (df)
    {
    this->NodeSetNumberOfDistributionFactors = df;

    this->BuildNodeSetDistributionFactorIndex();
    }

  return 0;
}

//-------------------------------------------------
// side set calculations
//-------------------------------------------------

void vtkModelMetadata::SetSideSetIds(int *s)
{
  FREE(this->SideSetIds);

  this->SideSetIds = s;
}
void vtkModelMetadata::SetSideSetElementList(int *s)
{
  FREE(this->SideSetElementList);

  this->SideSetElementList= s;
}
void vtkModelMetadata::SetSideSetSideList(int *s)
{
  FREE(this->SideSetSideList);

  this->SideSetSideList= s;
}
void vtkModelMetadata::SetSideSetNumDFPerSide(int *s)
{
  FREE(this->SideSetNumDFPerSide);

  this->SideSetNumDFPerSide= s;
}
int vtkModelMetadata::SetSideSetSize(int *size)
{
  FREE(this->SideSetSize);

  if (size)
    {
    this->SideSetSize = size;

    this->BuildSideSetListIndex();
    }

  return 0;
}
int vtkModelMetadata::SetSideSetNumberOfDistributionFactors(int *df)
{
  FREE(this->SideSetNumberOfDistributionFactors)

  if (df)
    {
    this->SideSetNumberOfDistributionFactors = df;

    this->BuildSideSetDistributionFactorIndex();
    }

  return 0;
}
void vtkModelMetadata::SetSideSetDistributionFactors(float *d)
{
  FREE(this->SideSetDistributionFactors);

  this->SideSetDistributionFactors = d;
}

int vtkModelMetadata::BuildSideSetListIndex()
{
  int nsets = this->NumberOfSideSets;
  int *size = this->SideSetSize;

  if ((nsets < 1) || !size)
    {
    return 1;
    }

  if (this->SideSetListIndex)
    {
    delete [] this->SideSetListIndex;
    }
  this->SideSetListIndex = new int [ nsets ];

  int idx = 0;
  for (int i=0; i<nsets; i++)
    {
    this->SideSetListIndex[i] = idx;
    idx += size[i];      
    }

  this->SumSidesPerSideSet = idx;
  return 0;
}
int vtkModelMetadata::BuildSideSetDistributionFactorIndex()
{
  int nsets = this->NumberOfSideSets;
  int *numFactors = this->SideSetNumberOfDistributionFactors;

  if ((nsets < 1) || !numFactors)
    {
    return 1;
    }

  if (this->SideSetDistributionFactorIndex)
    {
    delete [] this->SideSetDistributionFactorIndex;
    }
  this->SideSetDistributionFactorIndex = new int [ nsets ];

  int idx = 0;

  for (int i=0; i<nsets; i++)
    {
    this->SideSetDistributionFactorIndex[i] = idx;
    idx += numFactors[i];
    }

  this->SumDistFactPerSideSet = idx;

  return 0;
}

//-------------------------------------------------
// Properties
//-------------------------------------------------

void vtkModelMetadata::SetBlockPropertyNames(int nprop, char **nms)
{
  int i;

  FREELIST(this->BlockPropertyNames, this->NumberOfBlockProperties);

  this->NumberOfBlockProperties = nprop;
  this->BlockPropertyNames = nms; 
}
void vtkModelMetadata::SetBlockPropertyValue(int *v)
{
  FREE(this->BlockPropertyValue);

  this->BlockPropertyValue = v;
}
void vtkModelMetadata::SetNodeSetPropertyNames(int nprops, char **nms)
{
  int i;

  FREELIST(this->NodeSetPropertyNames, this->NumberOfNodeSetProperties);

  this->NumberOfNodeSetProperties = nprops;
  this->NodeSetPropertyNames = nms; 
}
void vtkModelMetadata::SetNodeSetPropertyValue(int *v)
{
  FREE(this->NodeSetPropertyValue);

  this->NodeSetPropertyValue = v;
}
void vtkModelMetadata::SetSideSetPropertyNames(int nprops, char **nms)
{
  int i;

  FREELIST(this->SideSetPropertyNames, this->NumberOfSideSetProperties);

  this->NumberOfSideSetProperties = nprops;
  this->SideSetPropertyNames = nms; 
}
void vtkModelMetadata::SetSideSetPropertyValue(int *v)
{
  FREE(this->SideSetPropertyValue);

  this->SideSetPropertyValue = v;
}
//-------------------------------------------------
// Global variables
//-------------------------------------------------

void vtkModelMetadata::SetGlobalVariableNames(int num, char **n)
{
  int i;

  FREELIST(this->GlobalVariableNames, this->NumberOfGlobalVariables);

  this->GlobalVariableNames = n; 
  this->NumberOfGlobalVariables = num;
}
void vtkModelMetadata::SetGlobalVariableValue(float *f)
{
  FREE(this->GlobalVariableValue);

  this->GlobalVariableValue = f;
}
//-------------------------------------------------
// Element variables
//-------------------------------------------------

int vtkModelMetadata::FindNameOnList(char *name, char **list, int listLen)
{
  int found = -1;

  for (int i=0; i<listLen; i++)
    {
    if (!strcmp(name, list[i]))
      {
      found = i;
      break;
      }
    }

  return found;
}

void vtkModelMetadata::SetOriginalElementVariableNames(int nvars, char **names)
{
  this->FreeOriginalElementVariableNames();

  this->OriginalNumberOfElementVariables = nvars;
  this->OriginalElementVariableNames = names;
}
void vtkModelMetadata::SetElementVariableNames(int nvars, char **names)
{
  this->FreeUsedElementVariableNames();

  this->NumberOfElementVariables = nvars;
  this->MaxNumberOfElementVariables = nvars;
  this->ElementVariableNames = names;
}
void vtkModelMetadata::SetElementVariableNumberOfComponents(int *comp)
{
  FREE(this->ElementVariableNumberOfComponents);
  this->ElementVariableNumberOfComponents = comp;
}
void vtkModelMetadata::SetMapToOriginalElementVariableNames(int *map)
{
  FREE(this->MapToOriginalElementVariableNames);
  this->MapToOriginalElementVariableNames = map;
}

int vtkModelMetadata::AddUGridElementVariable(char *ugridVarName, 
                                    char *origName, int numComponents)
{
  int i;

  int maxVarNames = this->OriginalNumberOfElementVariables;

  if (maxVarNames < 1)
    {
    vtkErrorMacro(<< "Can't have grid variables if there are no file variables");
    return 1;
    }

  if (this->ElementVariableNames == NULL)
    {
    this->FreeUsedElementVariables();

    this->ElementVariableNames = new char * [maxVarNames];  // upper bound

    memset(this->ElementVariableNames, 0, sizeof (char *) * maxVarNames);

    this->NumberOfElementVariables = 0;
    this->MaxNumberOfElementVariables = maxVarNames;

    this->MapToOriginalElementVariableNames = new int [maxVarNames];
    this->ElementVariableNumberOfComponents = new int [maxVarNames];
    }
  else if (vtkModelMetadata::FindNameOnList(ugridVarName, 
       this->ElementVariableNames, this->NumberOfElementVariables) >= 0)
    {
    return 0;   // already got it
    }

  int next = this->NumberOfElementVariables;

  if (next >= this->MaxNumberOfElementVariables)
    {
    int newSize = this->MaxNumberOfElementVariables + maxVarNames;

    char **names = new char * [newSize];
    memset(names, 0, sizeof(char *) * newSize);
    int *comp = new int [newSize];
    int *map = new int [newSize];

    memcpy(names, this->ElementVariableNames, sizeof(char *)  * next);
    memcpy(comp, this->ElementVariableNumberOfComponents, sizeof(int) * next);
    memcpy(map, this->MapToOriginalElementVariableNames, sizeof(int) * next);

    this->FreeUsedElementVariables();

    this->ElementVariableNames = names;
    this->ElementVariableNumberOfComponents = comp;
    this->MapToOriginalElementVariableNames = map;

    this->MaxNumberOfElementVariables = newSize;
    }

  this->ElementVariableNames[next] = ugridVarName;
  this->ElementVariableNumberOfComponents[next] = numComponents;

  int idx = -1;

  for (i=0; i<this->OriginalNumberOfElementVariables; i++)
    {
    if (!strcmp(this->OriginalElementVariableNames[i], origName))
      {
      idx = i;
      break;
      }
    }

  this->MapToOriginalElementVariableNames[next] = idx;

  this->NumberOfElementVariables++;

  delete [] origName;

  return 0;
}
int vtkModelMetadata::RemoveUGridElementVariable(char *ugridVarName)
{
  int i;
  int maxVarNames = this->NumberOfElementVariables;

  int idx = vtkModelMetadata::FindNameOnList(ugridVarName,
               this->ElementVariableNames, maxVarNames);

  if (idx == -1) return 1;

  delete []  this->ElementVariableNames[idx];

  for (i=idx+1; i<maxVarNames; i++)
    {
    int prev = i-1;

    this->ElementVariableNames[prev] = this->ElementVariableNames[i];
    this->ElementVariableNumberOfComponents[prev] = 
         this->ElementVariableNumberOfComponents[i];
    this->MapToOriginalElementVariableNames[prev] = 
         this->MapToOriginalElementVariableNames[i];
    }

  this->ElementVariableNames[maxVarNames-1] = NULL;

  this->NumberOfElementVariables--;

  return 0;
}
void vtkModelMetadata::SetElementVariableInfo(int numOrigNames, char **origNames,
            int numNames, char **names, int *numComp, int *map)
{
  this->SetOriginalElementVariableNames(numOrigNames, origNames);
  this->SetElementVariableNames(numNames, names);
  this->SetElementVariableNumberOfComponents(numComp);
  this->SetMapToOriginalElementVariableNames(map);
}
//-------------------------------------------------
// Truth table
//-------------------------------------------------

void vtkModelMetadata::SetElementVariableTruthTable(int *n)
{
  FREE(this->ElementVariableTruthTable);
  this->AllVariablesDefinedInAllBlocks = 1;  // the default

  if (n)
    {
    this->ElementVariableTruthTable= n;
  
    int numEntries = this->NumberOfBlocks * this->OriginalNumberOfElementVariables;
  
    for (int i=0; i<numEntries; i++)
      {
      if (n[i] == 0)
        {
        this->AllVariablesDefinedInAllBlocks = 0;
        break;
        }
      }
    }
}

//-------------------------------------------------
// Node variables
//-------------------------------------------------

void vtkModelMetadata::SetOriginalNodeVariableNames(int nvars, char **names)
{
  this->FreeOriginalNodeVariableNames();

  this->OriginalNumberOfNodeVariables = nvars;
  this->OriginalNodeVariableNames = names;
}
void vtkModelMetadata::SetNodeVariableNames(int nvars, char **names)
{
  this->FreeUsedNodeVariableNames();

  this->NumberOfNodeVariables = nvars;
  this->MaxNumberOfNodeVariables = nvars;
  this->NodeVariableNames = names;
}
void vtkModelMetadata::SetNodeVariableNumberOfComponents(int *comp)
{
  FREE(this->NodeVariableNumberOfComponents);
  this->NodeVariableNumberOfComponents = comp;
}
void vtkModelMetadata::SetMapToOriginalNodeVariableNames(int *map)
{
  FREE(this->MapToOriginalNodeVariableNames);
  this->MapToOriginalNodeVariableNames = map;
}

int vtkModelMetadata::AddUGridNodeVariable(char *ugridVarName, 
                                    char *origName, int numComponents)
{
  int i;

  int maxVarNames = this->OriginalNumberOfNodeVariables;

  if (maxVarNames < 1)
    {
    vtkErrorMacro(<< "Can't have grid variables if there are no file variables");
    return 1;
    }

  if (this->NodeVariableNames == NULL)
    {
    this->FreeUsedNodeVariableNames();

    this->NodeVariableNames = new char * [maxVarNames];  // upper bound

    memset(this->NodeVariableNames, 0, sizeof (char *) * maxVarNames);

    this->NumberOfNodeVariables = 0;
    this->MaxNumberOfNodeVariables = maxVarNames;

    this->MapToOriginalNodeVariableNames = new int [maxVarNames];
    this->NodeVariableNumberOfComponents = new int [maxVarNames];
    }
  else if (vtkModelMetadata::FindNameOnList(ugridVarName, 
       this->NodeVariableNames, this->NumberOfNodeVariables) >= 0)
    {
    return 0;   // already got it
    }

  int next = this->NumberOfNodeVariables;

  if (next >= this->MaxNumberOfNodeVariables)
    {
    int newSize = this->MaxNumberOfNodeVariables + maxVarNames;

    char **names = new char * [newSize];
    memset(names, 0, sizeof(char *) * newSize);
    int *comp = new int [newSize];
    int *map = new int [newSize];

    memcpy(names, this->NodeVariableNames, sizeof(char *)  * next);
    memcpy(comp, this->NodeVariableNumberOfComponents, sizeof(int) * next);
    memcpy(map, this->MapToOriginalNodeVariableNames, sizeof(int) * next);

    this->FreeUsedNodeVariableNames();

    this->NodeVariableNames = names;
    this->NodeVariableNumberOfComponents = comp;
    this->MapToOriginalNodeVariableNames = map;

    this->MaxNumberOfNodeVariables = newSize;
    }

  this->NodeVariableNames[next] = ugridVarName;
  this->NodeVariableNumberOfComponents[next] = numComponents;

  int idx = -1;

  for (i=0; i<this->OriginalNumberOfNodeVariables; i++)
    {
    if (!strcmp(this->OriginalNodeVariableNames[i], origName))
      {
      idx = i;
      break;
      }
    }

  this->MapToOriginalNodeVariableNames[next] = idx;

  this->NumberOfNodeVariables++;

  delete [] origName;

  return 0;
}
int vtkModelMetadata::RemoveUGridNodeVariable(char *ugridVarName)
{
  int i;
  int maxVarNames = this->NumberOfNodeVariables;

  int idx = vtkModelMetadata::FindNameOnList(ugridVarName,
               this->NodeVariableNames, maxVarNames);

  if (idx == -1) return 1;

  delete []  this->NodeVariableNames[idx];

  for (i=idx+1; i<maxVarNames; i++)
    {
    int prev = i-1;

    this->NodeVariableNames[prev] = this->NodeVariableNames[i];
    this->NodeVariableNumberOfComponents[prev] = 
         this->NodeVariableNumberOfComponents[i];
    this->MapToOriginalNodeVariableNames[prev] = 
         this->MapToOriginalNodeVariableNames[i];
    }

  this->NodeVariableNames[maxVarNames-1] = NULL;

  this->NumberOfNodeVariables--;

  return 0;
}
void vtkModelMetadata::SetNodeVariableInfo(int numOrigNames, char **origNames,
            int numNames, char **names, int *numComp, int *map)
{
  this->SetOriginalNodeVariableNames(numOrigNames, origNames);
  this->SetNodeVariableNames(numNames, names);
  this->SetNodeVariableNumberOfComponents(numComp);
  this->SetMapToOriginalNodeVariableNames(map);
}

//---------------------------------------------------------------
// Pack all the metadata into an unstructured grid, or
// initialize this vtkModelMetadata object with information packed
// into an unstructured grid (i.e. unpack).
//---------------------------------------------------------------

#define SIZE_ARRAY "vtkModelMetadataSizes"
#define INT_ARRAY "vtkModelMetadataInts"
#define FLOAT_ARRAY "vtkModelMetadataFloats"
#define CHAR_ARRAY "vtkModelMetadataChars"

void vtkModelMetadata::RemoveMetadata(vtkDataSet *grid)
{
  grid->GetFieldData()->RemoveArray(SIZE_ARRAY);
  grid->GetFieldData()->RemoveArray(INT_ARRAY);
  grid->GetFieldData()->RemoveArray(FLOAT_ARRAY);
  grid->GetFieldData()->RemoveArray(CHAR_ARRAY);
}

int vtkModelMetadata::HasMetadata(vtkDataSet *grid)
{
  int hasMetadata = 0;

  if (grid)
    {
    vtkFieldData *fa = grid->GetFieldData();

    if (fa)
      {
      vtkDataArray *da = fa->GetArray(SIZE_ARRAY);
      if (da) hasMetadata = 1;
      }
    }

  return hasMetadata;
}

void vtkModelMetadata::Pack(vtkDataSet *grid)
{
  int maxStringLength, maxLineLength;

  this->CalculateMaximumLengths(maxStringLength, maxLineLength);

  vtkIntArray *sizes = this->PackSizeArray(maxStringLength, maxLineLength);
  vtkIntArray *ints = this->PackIntArray();
  vtkCharArray *chars = this->PackCharArray(maxStringLength, maxLineLength);
  vtkFloatArray *floats = this->PackFloatArray();

  vtkFieldData *fa = grid->GetFieldData();

  if (!fa)
    {
    fa = vtkFieldData::New();
    grid->SetFieldData(fa);
    fa->Delete();
    fa = grid->GetFieldData();
    }

  fa->AddArray(sizes);
  sizes->Delete();

  if (ints->GetNumberOfTuples() > 0)
    {
    fa->AddArray(ints);
    ints->Delete();
    }

  if (chars->GetNumberOfTuples() > 0)
    {
    fa->AddArray(chars);
    chars->Delete();
    }

  if (floats->GetNumberOfTuples() > 0)
    {
    fa->AddArray(floats);
    floats->Delete();
    }
}
int vtkModelMetadata::Unpack(vtkDataSet *grid, int deleteIt)
{
  vtkFieldData *fa = grid->GetFieldData();

  if (!fa)
    {
    return 1; 
    }

  vtkDataArray *da = fa->GetArray(SIZE_ARRAY);
  vtkIntArray *sizes = vtkIntArray::SafeDownCast(da);

  if (!sizes)
    {
    return 1; 
    }

  this->Reset();

  // NOTE: The size array must be unpacked before any other, because
  //  it provides information about the contents of the other arrays.
  //  We keep the sizes in a temporary object until "this" object is set.

  vtkModelMetadata *temp = vtkModelMetadata::New();

  int maxString=0;
  int maxLine=0;

  int fail = temp->InitializeFromSizeArray(sizes, maxString, maxLine);

  if (fail)
    {
    temp->Delete();
    return 1;
    }

  if (deleteIt)
    {
    fa->RemoveArray(SIZE_ARRAY);
    }

  da = fa->GetArray(INT_ARRAY);
  vtkIntArray *ints = vtkIntArray::SafeDownCast(da);

  if (ints)
    {
    fail = this->InitializeFromIntArray(temp, ints);

    if (fail)
      {
      temp->Delete();
      return 1;
      }
    if (deleteIt)
      {
      fa->RemoveArray(INT_ARRAY);
      }
    }

  da = fa->GetArray(CHAR_ARRAY);
  vtkCharArray *chars = vtkCharArray::SafeDownCast(da);

  if (chars)
    {
    fail = this->InitializeFromCharArray(temp, chars, maxString, maxLine);

    if (fail)
      {
      temp->Delete();
      return 1;
      }
    if (deleteIt)
      {
      fa->RemoveArray(CHAR_ARRAY);
      }
    }

  da = fa->GetArray(FLOAT_ARRAY);
  vtkFloatArray *floats = vtkFloatArray::SafeDownCast(da);

  if (floats)
    {
    fail = this->InitializeFromFloatArray(floats);

    if (fail)
      {
      temp->Delete();
      return 1;
      }

    if (deleteIt)
      {
      fa->RemoveArray(FLOAT_ARRAY);
      }
    }

  temp->Delete();

  return 0;
}
vtkIntArray *vtkModelMetadata::PackSizeArray(int maxStr, int maxLine)
{
// Fields stored in size array:
//  int NumberOfQARecords;
//  int NumberOfInformationLines;
//  int Dimension;
//  int NumberOfBlocks;
//  int NumberOfNodeSets;
//  int NumberOfSideSets;
//  int NumberOfBlockProperties;
//  int NumberOfNodeSetProperties;
//  int NumberOfSideSetProperties;
//  int NumberOfGlobalVariables;
//  int NumberOfElementVariables;
//  int NumberOfNodeVariables;
//  int OriginalNumberOfElementVariables;
//  int OriginalNumberOfNodeVariables;
//  int MaxStringLength
//  int MaxLineLength

  vtkIntArray *sizeInfo = vtkIntArray::New();
  sizeInfo->SetName(SIZE_ARRAY);

  sizeInfo->SetNumberOfValues(16);

  sizeInfo->SetValue(0, this->NumberOfQARecords);
  sizeInfo->SetValue(1, this->NumberOfInformationLines);
  sizeInfo->SetValue(2, this->Dimension);
  sizeInfo->SetValue(3, this->NumberOfBlocks);
  sizeInfo->SetValue(4, this->NumberOfNodeSets);
  sizeInfo->SetValue(5, this->NumberOfSideSets);
  sizeInfo->SetValue(6, this->NumberOfBlockProperties);
  sizeInfo->SetValue(7, this->NumberOfNodeSetProperties);
  sizeInfo->SetValue(8, this->NumberOfSideSetProperties);
  sizeInfo->SetValue(9, this->NumberOfGlobalVariables);
  sizeInfo->SetValue(10, this->NumberOfElementVariables);
  sizeInfo->SetValue(11, this->NumberOfNodeVariables);
  sizeInfo->SetValue(12, this->OriginalNumberOfElementVariables);
  sizeInfo->SetValue(13, this->OriginalNumberOfNodeVariables);
  sizeInfo->SetValue(14, maxStr);
  sizeInfo->SetValue(15, maxLine);

  return sizeInfo;
}
int vtkModelMetadata::InitializeFromSizeArray(vtkIntArray *ia, int &maxS, int &maxL)
{
  int nvals = ia->GetNumberOfTuples();

  if (nvals < 16) return 1;

  this->NumberOfQARecords         = ia->GetValue(0);
  this->NumberOfInformationLines  = ia->GetValue(1);
  this->Dimension                 = ia->GetValue(2);
  this->NumberOfBlocks            = ia->GetValue(3);
  this->NumberOfNodeSets          = ia->GetValue(4);
  this->NumberOfSideSets          = ia->GetValue(5);
  this->NumberOfBlockProperties   = ia->GetValue(6);
  this->NumberOfNodeSetProperties = ia->GetValue(7);
  this->NumberOfSideSetProperties = ia->GetValue(8);
  this->NumberOfGlobalVariables   = ia->GetValue(9);
  this->NumberOfElementVariables  = ia->GetValue(10);
  this->NumberOfNodeVariables     = ia->GetValue(11);
  this->OriginalNumberOfElementVariables  = ia->GetValue(12);
  this->OriginalNumberOfNodeVariables     = ia->GetValue(13);
  maxS = ia->GetValue(14);
  maxL = ia->GetValue(15);

  return 0;
}
#define __CHECK_COPY(to, from, size) \
    (from ? memcpy(to, from, size) : memset(to, 0, size));

vtkIntArray *vtkModelMetadata::PackIntArray()
{
// Fields stored in the integer array
//    3 counts: Sum of all node set sizes, sum of all side set sizes,
//              and sum of all elements per block
//    1 integer: the current time step index
//    int *BlockIds;                // NumberOfBlocks
//    int *BlockNumberOfElements;  // NumberOfBlocks
//    int *BlockNodesPerElement;   // NumberOfBlocks
//    int *BlockNumberOfAttributesPerElement;// NumberOfBlocks
//    int *BlockElementIdList;     // SumElementsPerBlock 
//    int *NodeSetIds;             // NumberOfNodeSets
//    int *NodeSetSize;            // NumberOfNodeSets
//    int *NodeSetNumberOfDistributionFactors;  // NumberOfNodeSets
//    int *NodeSetNodeIdList;                   // SumNodesPerNodeSet
//    int *SideSetIds;                          // NumberOfSideSets
//    int *SideSetSize;                         // NumberOfSideSets
//    int *SideSetNumberOfDistributionFactors;  // NumberOfSideSets
//    int *SideSetElementList;              // SumSidesPerSideSet
//    int *SideSetSideList;                 // SumSidesPerSideSet
//    int *SideSetNumDFPerSide;          // SumSidesPerSideSet
//    int *BlockPropertyValue;              // NumBlocks * NumBlockProperties
//    int *NodeSetPropertyValue;            // NumNodeSets * NumNodeSetProperties
//    int *SideSetPropertyValue;            // NumSideSets * NumSideSetProperties
//    int *ElementVariableTruthTable;  // NumBlocks * OrigNumElementVariables
//    int *ElementVariableNumberOfComponents; // NumberOfElementVariables
//    int *MapToOriginalElementVariableNames; // NumberOfElementVariables
//    int *NodeVariableNumberOfComponents;    // NumberOfNodeVariables
//    int *MapToOriginalNodeVariableNames;    // NumberOfNodeVariables

  int nblocks = this->NumberOfBlocks;
  int nnsets = this->NumberOfNodeSets;
  int nssets = this->NumberOfSideSets;
  int nblockProp = this->NumberOfBlockProperties;
  int nnsetProp = this->NumberOfNodeSetProperties;
  int nssetProp = this->NumberOfSideSetProperties;
  int nOrigEltVars = this->OriginalNumberOfElementVariables;
  int nEltVars = this->NumberOfElementVariables;
  int nNodeVars = this->NumberOfNodeVariables;

  vtkIdType nvals = 4 +
    (nblocks * 4) + this->SumElementsPerBlock +
    (nnsets * 3) + this->SumNodesPerNodeSet +
    (nssets * 3) + (this->SumSidesPerSideSet * 3) +
    (nblocks * nblockProp) +
    (nnsets * nnsetProp) +
    (nssets * nssetProp) +
    (nblocks * nOrigEltVars) +
    (nEltVars * 2) +
    (nNodeVars * 2);

  int *packed = new int [nvals];
  int *p = packed;

  p[0] = this->SumNodesPerNodeSet;
  p[1] = this->SumSidesPerSideSet;
  p[2] = this->SumElementsPerBlock;
  p[3] = this->TimeStepIndex;

  p += 4;

  if (nblocks > 0)
    {
    size_t n = sizeof(int) * nblocks;

    __CHECK_COPY(p, this->BlockIds, n)
    p += nblocks;

    __CHECK_COPY(p, this->BlockNumberOfElements, n)
    p += nblocks;

    __CHECK_COPY(p, this->BlockNodesPerElement, n)
    p += nblocks;

    __CHECK_COPY(p, this->BlockNumberOfAttributesPerElement, n)
    p += nblocks;

    __CHECK_COPY(p, this->BlockElementIdList, sizeof(int) * this->SumElementsPerBlock)
    p += this->SumElementsPerBlock;
    }

  if (nnsets > 0)
    {
    size_t n = sizeof(int) * nnsets;

    __CHECK_COPY(p, this->NodeSetIds, n);
    p += nnsets;

    __CHECK_COPY(p, this->NodeSetSize, n);
    p += nnsets;

    __CHECK_COPY(p, this->NodeSetNumberOfDistributionFactors, n);
    p += nnsets;

    __CHECK_COPY(p, this->NodeSetNodeIdList, sizeof(int) * this->SumNodesPerNodeSet);
    p += this->SumNodesPerNodeSet;
    }

  if (nssets > 0)
    {
    size_t n = sizeof(int) * nssets;

    __CHECK_COPY(p, this->SideSetIds, n);
    p += nssets;

    __CHECK_COPY(p, this->SideSetSize, n);
    p += nssets;

    __CHECK_COPY(p, this->SideSetNumberOfDistributionFactors, n);
    p += nssets;

    size_t sum = sizeof(int) * this->SumSidesPerSideSet;

    __CHECK_COPY(p, this->SideSetElementList, sum);
    p += this->SumSidesPerSideSet;

    __CHECK_COPY(p, this->SideSetSideList, sum);
    p += this->SumSidesPerSideSet;

    __CHECK_COPY(p, this->SideSetNumDFPerSide, sum);
    p += this->SumSidesPerSideSet;
    }

  if (nblockProp > 0)
    {
    __CHECK_COPY(p, this->BlockPropertyValue, 
              sizeof(int) * nblockProp * nblocks);
    p += (nblockProp * nblocks);
    }

  if (nnsetProp > 0)
    {
    __CHECK_COPY(p, this->NodeSetPropertyValue, 
              sizeof(int) * nnsetProp * nnsets);
    p += (nnsetProp * nnsets);
    }

  if (nssetProp > 0)
    {
    __CHECK_COPY(p, this->SideSetPropertyValue, 
              sizeof(int) * nssetProp * nssets);
    p += (nssetProp * nssets);
    }

  if ((nblocks > 0) && (nOrigEltVars > 0))
    {
    __CHECK_COPY(p, this->ElementVariableTruthTable, 
              sizeof(int) * nblocks * nOrigEltVars);
    p += (nblocks * nOrigEltVars);
    }

  if (nEltVars > 0)
    {
    __CHECK_COPY(p, this->ElementVariableNumberOfComponents, 
              sizeof(int) * nEltVars);
    p += nEltVars;

    __CHECK_COPY(p, this->MapToOriginalElementVariableNames, 
              sizeof(int) * nEltVars);
    p += nEltVars;
    }

  if (nNodeVars > 0)
    {
    __CHECK_COPY(p, this->NodeVariableNumberOfComponents,
              sizeof(int) * nNodeVars);
    p += nNodeVars;

    __CHECK_COPY(p, this->MapToOriginalNodeVariableNames, 
              sizeof(int) * nNodeVars);
    p += nNodeVars;
    }

  vtkIntArray *ia = vtkIntArray::New();
  ia->SetName(INT_ARRAY);
  ia->SetArray(packed, nvals, 0);

  return ia;
}
int vtkModelMetadata::InitializeFromIntArray(vtkModelMetadata *sizes, vtkIntArray *ia)
{
  int nblocks = sizes->NumberOfBlocks;
  int nnsets = sizes->NumberOfNodeSets;
  int nssets = sizes->NumberOfSideSets;
  int nblockProp = sizes->NumberOfBlockProperties;
  int nnsetProp = sizes->NumberOfNodeSetProperties;
  int nssetProp = sizes->NumberOfSideSetProperties;
  int nEltVars = sizes->NumberOfElementVariables;
  int nNodeVars = sizes->NumberOfNodeVariables;
  int ttsize = nblocks * sizes->OriginalNumberOfElementVariables;

  int *p = ia->GetPointer(0);

  int sumNodeSetSizes = p[0];
  int sumSideSetSizes = p[1];
  int sumElementIds = p[2];

  this->TimeStepIndex = p[3];

  p += 4;

  if (nblocks > 0)
    {
    int *buf = new int [nblocks];
    memcpy(buf, p, nblocks * sizeof(int));
    p += nblocks;

    this->SetNumberOfBlocks(nblocks);
    this->SetBlockIds(buf);

    buf = new int [nblocks];
    memcpy(buf, p, nblocks * sizeof(int));
    p += nblocks;

    this->SetBlockNumberOfElements(buf);

    buf = new int [nblocks];
    memcpy(buf, p, nblocks * sizeof(int));
    p += nblocks;

    this->SetBlockNodesPerElement(buf);

    buf = new int [nblocks];
    memcpy(buf, p, nblocks * sizeof(int));
    p += nblocks;

    this->SetBlockNumberOfAttributesPerElement(buf);

    buf = new int [sumElementIds];
    memcpy(buf, p, sumElementIds * sizeof(int));
    p += sumElementIds;

    this->SetBlockElementIdList(buf);
    }

  if (nnsets > 0)
    {
    int *buf = new int [nnsets];
    memcpy(buf, p, nnsets * sizeof(int));
    p += nnsets;

    this->SetNumberOfNodeSets(nnsets);
    this->SetNodeSetIds(buf);

    buf = new int [nnsets];
    memcpy(buf, p, nnsets * sizeof(int));
    p += nnsets;

    this->SetNodeSetSize(buf);

    buf = new int [nnsets];
    memcpy(buf, p, nnsets * sizeof(int));
    p += nnsets;

    this->SetNodeSetNumberOfDistributionFactors(buf);

    buf = new int [sumNodeSetSizes];
    memcpy(buf, p, sumNodeSetSizes * sizeof(int));
    p += sumNodeSetSizes;

    this->SetNodeSetNodeIdList(buf);
    }

  if (nssets > 0)
    {
    int *buf = new int [nssets];
    memcpy(buf, p, nssets * sizeof(int));
    p += nssets;

    this->SetNumberOfSideSets(nssets);
    this->SetSideSetIds(buf);

    buf = new int [nssets];
    memcpy(buf, p, nssets * sizeof(int));
    p += nssets;

    this->SetSideSetSize(buf);

    buf = new int [nssets];
    memcpy(buf, p, nssets * sizeof(int));
    p += nssets;

    this->SetSideSetNumberOfDistributionFactors(buf);

    buf = new int [sumSideSetSizes];
    memcpy(buf, p, sumSideSetSizes * sizeof(int));
    p += sumSideSetSizes;

    this->SetSideSetElementList(buf);

    buf = new int [sumSideSetSizes];
    memcpy(buf, p, sumSideSetSizes * sizeof(int));
    p += sumSideSetSizes;

    this->SetSideSetSideList(buf);

    buf = new int [sumSideSetSizes];
    memcpy(buf, p, sumSideSetSizes * sizeof(int));
    p += sumSideSetSizes;

    this->SetSideSetNumDFPerSide(buf);
    }

  if (nblockProp > 0)
    {
    int nvals = nblocks * nblockProp;

    int *buf = new int [nvals];
    memcpy(buf, p, nvals * sizeof(int));
    p += nvals ;

    this->SetBlockPropertyValue(buf);
    }

  if (nnsetProp > 0)
    {
    int nvals = nnsets * nnsetProp;

    int *buf = new int [nvals];
    memcpy(buf, p, nvals * sizeof(int));
    p += nvals ;

    this->SetNodeSetPropertyValue(buf);
    }

  if (nssetProp > 0)
    {
    int nvals = nssets * nssetProp;

    int *buf = new int [nvals];
    memcpy(buf, p, nvals * sizeof(int));
    p += nvals ;

    this->SetSideSetPropertyValue(buf);
    }

  if (ttsize > 0)
    {
    int *buf = new int [ttsize];
    memcpy(buf, p, ttsize * sizeof(int));
    p += ttsize;

    this->SetElementVariableTruthTable(buf);
    }

  if (nEltVars > 0)
   {
    int *buf1 = new int [nEltVars];
    memcpy(buf1, p, nEltVars * sizeof(int));
    p += nEltVars;

    int *buf2 = new int [nEltVars];
    memcpy(buf2, p, nEltVars * sizeof(int));
    p += nEltVars;

    this->SetElementVariableNumberOfComponents(buf1);
    this->SetMapToOriginalElementVariableNames(buf2);
   }

  if (nNodeVars > 0)
    {
    int *buf1 = new int [nNodeVars];
    memcpy(buf1, p, nNodeVars * sizeof(int));
    p += nNodeVars;

    int *buf2 = new int [nNodeVars];
    memcpy(buf2, p, nNodeVars * sizeof(int));
    p += nNodeVars;

    this->SetNodeVariableNumberOfComponents(buf1);
    this->SetMapToOriginalNodeVariableNames(buf2);
    }

  return 0;
}
char *vtkModelMetadata::WriteLines(
    char *p, int maxLines, int maxLen, char **lines)
{
  for (int i=0; i<maxLines; i++)
    {
    if (lines[i])
      {
      strcpy(p, lines[i]);
      }
    p += maxLen;
    }

  return p;
}

vtkCharArray *vtkModelMetadata::PackCharArray(int maxS, int maxL)
{
// Fields that go into the char array:
// char *Title;             // MaxLineLength
// char *QARecord[][4];     // NumberOfQARecords * 4 * MaxStringLength
// char **InformationLine;  // each record is MaxLineLength
// char **CoordinateNames;  // MaxStringLength each (at most 3 of these)
// char **BlockElementType;      // NumberOfBlocks, length MaxStringLength
// char **BlockPropertyNames;    // one per property, MaxStringLength
// char **NodeSetPropertyNames;  // one per property, MaxStringLength
// char **SideSetPropertyNames;  // one per property, MaxStringLength
// char **GlobalVariableNames;   // one per global variable, MaxStringLength
// char **OriginalElementVariableNames; // OriginalNumberOfElementVariables, MaxStr
// char **ElementVariableNames;     // NumberOfElementVariables, MaxStringLength
// char **OriginalNodeVariableNames;    // OriginalNumberOfNodeVariables, MaxStr
// char **NodeVariableNames;        // NumberOfNodeVariables, MaxStringLength
//
  vtkIdType len = 0;

  len += maxL;  // for the title

  len += (this->NumberOfQARecords * 4 * maxS);

  len += (this->NumberOfInformationLines * maxL); 

  len += (this->Dimension * maxS);
 
  len += (this->NumberOfBlocks * maxS);

  len += (this->NumberOfBlockProperties * maxS);

  len += (this->NumberOfNodeSetProperties * maxS);

  len += (this->NumberOfSideSetProperties * maxS);

  len += (this->NumberOfGlobalVariables * maxS);

  len += (this->OriginalNumberOfElementVariables * maxS);

  len += (this->NumberOfElementVariables * maxS);

  len += (this->OriginalNumberOfNodeVariables * maxS);

  len += (this->NumberOfNodeVariables * maxS);
 
  char *uc = new char [len];

  memset(uc, 0, len);

  char *p = uc;

  if (this->Title)
    {
    strcpy(p, this->Title);
    }
  else
    {
    strcpy(p, "NULL");
    }

  p += maxL;

  for (int i=0; i<this->NumberOfQARecords; i++)
    {
    for (int j=0; j<4; j++)
      {
      int l = static_cast<int>(strlen(this->QARecord[i][j]));
      if (l > maxS) l = maxS;

      memcpy(p, this->QARecord[i][j], l);

      p += maxS;
      }
    }

  p = vtkModelMetadata::WriteLines(p, this->NumberOfInformationLines,
                        maxL, this->InformationLine);

  p = vtkModelMetadata::WriteLines(p, this->Dimension,
                        maxS, this->CoordinateNames);

  p = vtkModelMetadata::WriteLines(p, this->NumberOfBlocks,
                        maxS, this->BlockElementType);

  p = vtkModelMetadata::WriteLines(p, this->NumberOfBlockProperties,
                        maxS, this->BlockPropertyNames);

  p = vtkModelMetadata::WriteLines(p, this->NumberOfNodeSetProperties,
                        maxS, this->NodeSetPropertyNames);

  p = vtkModelMetadata::WriteLines(p, this->NumberOfSideSetProperties,
                        maxS, this->SideSetPropertyNames);

  p = vtkModelMetadata::WriteLines(p, this->NumberOfGlobalVariables,
                        maxS, this->GlobalVariableNames);

  p = vtkModelMetadata::WriteLines(p, this->OriginalNumberOfElementVariables,
                        maxS, this->OriginalElementVariableNames);

  p = vtkModelMetadata::WriteLines(p, this->NumberOfElementVariables,
                        maxS, this->ElementVariableNames);

  p = vtkModelMetadata::WriteLines(p, this->OriginalNumberOfNodeVariables,
                        maxS, this->OriginalNodeVariableNames);

  p = vtkModelMetadata::WriteLines(p, this->NumberOfNodeVariables,
                        maxS, this->NodeVariableNames);

  vtkCharArray *uca = vtkCharArray::New();
  uca->SetArray(uc, len, 0);
  uca->SetName(CHAR_ARRAY);

  return uca;
}
char *vtkModelMetadata::ReadLines(
    char ***to, int maxLines, int maxLen, char *from)
{
  char **lineList = new char * [maxLines];

  for (int i=0; i<maxLines; i++)
    {
    lineList[i] = new char [maxLen+1];
    memcpy(lineList[i], from, maxLen);
    lineList[i][maxLen] = '\0';
    from += maxLen;
    }

  *to = lineList;

  return from;
}
int vtkModelMetadata::InitializeFromCharArray(vtkModelMetadata *sizes,
                          vtkCharArray *uca, int maxS, int maxL)
{
  char *uc = uca->GetPointer(0);
  char **buf = NULL;

  if (!uc)
    {
    return 1;
    }

  this->Title = new char [maxL + 1];
  memcpy(this->Title, uc, maxL);
  this->Title[maxL] = '\0';
 
  uc += maxL;

  int num = sizes->GetNumberOfQARecords();

  if (num > 0)
    {
    typedef char *p4[4];

    p4 *qaRec = new p4 [num];

    for (int i=0; i<num; i++)
      {
      for (int j=0; j<4; j++)
        {
        qaRec[i][j] = new char [maxS + 1];
        memcpy(qaRec[i][j], uc, maxS);
        qaRec[i][j][maxS] = '\0';
        uc += maxS;
        }
      }

    this->SetQARecords(num, qaRec);
    }

  num = sizes->GetNumberOfInformationLines(); 

  if (num > 0)
    {
    uc = vtkModelMetadata::ReadLines(&buf, num, maxL, uc);
    this->SetInformationLines(num, buf);
    }

  num = sizes->GetDimension();

  if (num)
    {
    uc = vtkModelMetadata::ReadLines(&buf, num, maxS, uc);
    this->SetCoordinateNames(num, buf);
    }

  if (this->NumberOfBlocks)    // set in InitializeFromIntArray
    {
    uc = vtkModelMetadata::ReadLines(&this->BlockElementType, 
                            this->NumberOfBlocks, maxS, uc);
    }

  num = sizes->GetNumberOfBlockProperties();

  if (num)
    {
    uc = vtkModelMetadata::ReadLines(&buf, num, maxS, uc);
    this->SetBlockPropertyNames(num, buf);
    }

  num = sizes->GetNumberOfNodeSetProperties();

  if (num)
    {
    uc = vtkModelMetadata::ReadLines(&buf, num, maxS, uc);
    this->SetNodeSetPropertyNames(num, buf);
    }

  num = sizes->GetNumberOfSideSetProperties();

  if (num)
    {
    uc = vtkModelMetadata::ReadLines(&buf, num, maxS, uc);
    this->SetSideSetPropertyNames(num, buf);
    }

  num = sizes->GetNumberOfGlobalVariables();

  if (num)
    {
    uc = vtkModelMetadata::ReadLines(&buf, num, maxS, uc);
    this->SetGlobalVariableNames(num, buf);
    }

  num = sizes->OriginalNumberOfElementVariables;

  if (num)
    {
    uc = vtkModelMetadata::ReadLines(&buf, num, maxS, uc);
    this->SetOriginalElementVariableNames(num, buf);
    }

  num = sizes->NumberOfElementVariables;

  if (num)
    {
    uc = vtkModelMetadata::ReadLines(&buf, num, maxS, uc);
    this->SetElementVariableNames(num, buf);
    }

  num = sizes->OriginalNumberOfNodeVariables;

  if (num)
    {
    uc = vtkModelMetadata::ReadLines(&buf, num, maxS, uc);
    this->SetOriginalNodeVariableNames(num, buf);
    }

  num = sizes->NumberOfNodeVariables;

  if (num)
    {
    uc = vtkModelMetadata::ReadLines(&buf, num, maxS, uc);
    this->SetNodeVariableNames(num, buf);
    }

  return 0;
}
vtkFloatArray *vtkModelMetadata::PackFloatArray()
{
// Fields stored in the float array:
//   4 counts: SizeBlockAttributeArray, SumDistFactPerNodeSet,
//             SumDistFactPerSideSet, and NumberOfTimeSteps
//   float *BlockAttributes;               // SizeBlockAttributeArray
//   float *NodeSetDistributionFactors;    // SumDistFactPerNodeSet
//   float *SideSetDistributionFactors;    // SumDistFactPerSideSet
//   float *GlobalVariableValue;           // NumberOfGlobalVariables
//   float *TimeStepValues;                // NumberOfTimeSteps
//

  vtkIdType len = 4 +
                  this->SizeBlockAttributeArray +
                  this->SumDistFactPerNodeSet +
                  this->SumDistFactPerSideSet +
                  this->NumberOfGlobalVariables +
                  this->NumberOfTimeSteps;

  float *f = new float [len];

  f[0] = static_cast<float>(this->SizeBlockAttributeArray);
  f[1] = static_cast<float>(this->SumDistFactPerNodeSet);
  f[2] = static_cast<float>(this->SumDistFactPerSideSet);
  f[3] = static_cast<float>(this->NumberOfTimeSteps);

  float *nextf = f + 4;

  if (this->SizeBlockAttributeArray)
    {
    memcpy(nextf, this->BlockAttributes,  
              this->SizeBlockAttributeArray * sizeof(float));
    nextf += this->SizeBlockAttributeArray;
    }

  if (this->SumDistFactPerNodeSet)
    {
    memcpy(nextf, this->NodeSetDistributionFactors, 
              this->SumDistFactPerNodeSet * sizeof(float));
    nextf += this->SumDistFactPerNodeSet;
    }

  if (this->SumDistFactPerSideSet)
    {
    memcpy(nextf, this->SideSetDistributionFactors, 
              this->SumDistFactPerSideSet * sizeof(float));
    nextf += this->SumDistFactPerSideSet;
    }

  if (this->NumberOfGlobalVariables)
    {
    memcpy(nextf, this->GlobalVariableValue, 
              this->NumberOfGlobalVariables * sizeof(float));
    nextf += this->NumberOfGlobalVariables;
    }

  if (this->NumberOfTimeSteps)
    {
    memcpy(nextf, this->TimeStepValues, 
              this->NumberOfTimeSteps* sizeof(float));
    nextf += this->NumberOfTimeSteps;
    }

  vtkFloatArray *fa = vtkFloatArray::New();
  fa->SetArray(f, len, 0);
  fa->SetName(FLOAT_ARRAY);

  return fa;
}
int vtkModelMetadata::InitializeFromFloatArray(vtkFloatArray *fa)
{
  float *f = fa->GetPointer(0);

  int sizeBlockAttributeArray = static_cast<int>(f[0]);
  int sumDistFactPerNodeSet= static_cast<int>(f[1]);
  int sumDistFactPerSideSet= static_cast<int>(f[2]);
  int numTimeSteps  = static_cast<int>(f[3]);

  f += 4;

  if ((sizeBlockAttributeArray != this->SizeBlockAttributeArray) ||
      (sumDistFactPerNodeSet != this->SumDistFactPerNodeSet) ||
      (sumDistFactPerSideSet != this->SumDistFactPerSideSet))
    {
    return 1;
    }

  if (this->SizeBlockAttributeArray)
    {
    float *buf = new float [this->SizeBlockAttributeArray];
    memcpy(buf, f, sizeof(float) * this->SizeBlockAttributeArray);
    this->SetBlockAttributes(buf);

    f += this->SizeBlockAttributeArray;
    }  

  if (this->SumDistFactPerNodeSet)
    {
    float *buf = new float [this->SumDistFactPerNodeSet];
    memcpy(buf, f, sizeof(float) * this->SumDistFactPerNodeSet);
    this->SetNodeSetDistributionFactors(buf);

    f += this->SumDistFactPerNodeSet;
    }  

  if (this->SumDistFactPerSideSet)
    {
    float *buf = new float [this->SumDistFactPerSideSet];
    memcpy(buf, f, sizeof(float) * this->SumDistFactPerSideSet);
    this->SetSideSetDistributionFactors(buf);

    f += this->SumDistFactPerSideSet;
    }  

  if (this->NumberOfGlobalVariables)
    {
    float *buf = new float [this->NumberOfGlobalVariables];
    memcpy(buf, f, sizeof(float) * this->NumberOfGlobalVariables);
    this->SetGlobalVariableValue(buf);

    f += this->NumberOfGlobalVariables;
    }  

  if (numTimeSteps)
    {
    float *buf = new float [numTimeSteps];
    memcpy(buf, f, sizeof(float) * numTimeSteps);
    this->SetTimeSteps(numTimeSteps, buf);

    f += numTimeSteps;
    }  

  return 0;
}

//---------------------------------------------------------------
// Merge and subset vtkModelMetadata objects.  Because grids get
// merged and subsetted on their journey through the VTK reader and filters.
//---------------------------------------------------------------

int vtkModelMetadata::AppendFloatLists(
    int numSubLists,
    float *id1, int *id1Idx, int id1Len,  
    float *id2, int *id2Idx, int id2Len, 
    float **idNew, int **idNewIdx, int *idNewLen)
{
  if ((id1Len == 0) && (id2Len == 0))
    {
    if (idNew)
      {
      *idNew = NULL;
      }
    if (idNewIdx)
      {
      *idNewIdx = NULL;
      }
    if (idNewLen)
      {
      *idNewLen = 0;
      }
    return 0;
    }

  int i;

  int newIdListLength = id1Len + id2Len;

  float *newIdList = new float [newIdListLength];
  int *newIdListIndex = new int [numSubLists];

  if (id1Len == 0)
    {
    memcpy(newIdList, id2, id2Len * sizeof(float));
    memcpy(newIdListIndex, id2Idx, numSubLists * sizeof(int));
    }
  else if (id2Len == 0)
    {
    memcpy(newIdList, id1, id1Len * sizeof(float));
    memcpy(newIdListIndex, id1Idx, numSubLists * sizeof(int));
    }
  else
    {
    newIdListIndex[0] = 0;
  
    int nextid = 0;
  
    for (i=0; i<numSubLists; i++)
      {
      int lastList = (i == numSubLists-1);
  
      float *ids = id1 + id1Idx[i];
      int numids = (lastList ? id1Len : id1Idx[i+1]) - id1Idx[i];
      if (numids > 0)
        {
        memcpy(newIdList + nextid, ids, numids * sizeof(float));
        nextid += numids;
        }
  
      ids = id2 + id2Idx[i];
      numids = (lastList ? id2Len : id2Idx[i+1]) - id2Idx[i];
    
      if (numids > 0)
        {
        memcpy(newIdList + nextid, ids, numids * sizeof(float));
        nextid += numids;
        }
  
      if (!lastList)
        {
        newIdListIndex[i+1] = nextid;
        }
      else
        {
        newIdListLength = nextid;
        }
      }
    }

  if (idNew)
    {
    *idNew = newIdList;
    }
  else
    {
    delete [] newIdList;
    }
  if (idNewIdx)
    {
    *idNewIdx = newIdListIndex;
    }
  else
    {
    delete [] newIdListIndex;
    }
  if (idNewLen)
    {
    *idNewLen = newIdListLength;
    }
  
  return 0;
}
int vtkModelMetadata::AppendIntegerLists(
    int numSubLists,
    int *id1, int *id1Idx, int id1Len,  
    int *id2, int *id2Idx, int id2Len, 
    int **idNew, int **idNewIdx, int *idNewLen)
{
  if ((id1Len == 0) && (id2Len == 0))
    {
    return 1;
    }

  int i;

  int newIdListLength = id1Len + id2Len;

  int *newIdList = new int [newIdListLength];
  int *newIdListIndex = new int [numSubLists];

  if (id1Len == 0)
    {
    memcpy(newIdList, id2, id2Len * sizeof(int));
    memcpy(newIdListIndex, id2Idx, numSubLists * sizeof(int));
    }
  else if (id2Len == 0)
    {
    memcpy(newIdList, id1, id1Len * sizeof(int));
    memcpy(newIdListIndex, id1Idx, numSubLists * sizeof(int));
    }
  else
    {
    newIdListIndex[0] = 0;
  
    int nextid = 0;
  
    for (i=0; i<numSubLists; i++)
      {
      int lastList = (i == numSubLists-1);
  
      int *ids = id1 + id1Idx[i];
      int numids = (lastList ? id1Len : id1Idx[i+1]) - id1Idx[i];
      if (numids > 0)
        {
        memcpy(newIdList + nextid, ids, numids * sizeof(int));
        nextid += numids;
        }
  
      ids = id2 + id2Idx[i];
      numids = (lastList ? id2Len : id2Idx[i+1]) - id2Idx[i];
    
      if (numids > 0)
        {
        memcpy(newIdList + nextid, ids, numids * sizeof(int));
        nextid += numids;
        }
  
      if (!lastList)
        {
        newIdListIndex[i+1] = nextid;
        }
      else
        {
        newIdListLength = nextid;
        }
      }
    }

  if (idNew)
    {
    *idNew = newIdList;
    }
  else
    {
    delete [] newIdList;
    }
  if (idNewIdx)
    {
    *idNewIdx = newIdListIndex;
    }
  else
    {
    delete [] newIdListIndex;
    }
  if (idNewLen)
    {
    *idNewLen = newIdListLength;
    }
  
  return 0;
}

int vtkModelMetadata::MergeIdLists(int numSubLists, 
    int *id1, int *id1Idx, int id1Len,  
      float *dist1, int *dist1Idx, int dist1Len,
    int *id2, int *id2Idx, int id2Len, 
      float *dist2, int *dist2Idx, int dist2Len,
    int **idNew, int **idNewIdx, int *idNewLen, 
      float **distNew, int **distNewIdx, int *distNewLen)
{
  if ((id1Len == 0) && (id2Len == 0))
    {
    return 1;
    }

  // Here we take two lists of IDs, and their associated lists of
  // floating point factors.  Some of the IDs in the second list may
  // be duplicates of IDs in the first list, and we need to filter
  // these out when we build the lists combining both.

  int i, id;

  int *newIdList=NULL;
  int *newIdListIndex = NULL;
  int newIdListLength = 0;

  float *newDistFact=NULL;
  int *newDistFactIndex=NULL;
  int newDistFactLength = 0;

  int maxIdListLength = id1Len + id2Len;
  int maxDistFactLength = dist1Len + dist2Len;

  newIdList = new int [maxIdListLength];
  newIdListIndex = new int [numSubLists];
  newIdListIndex[0] = 0;

  int distFact = (maxDistFactLength > 0);

  if (distFact)
    {
    newDistFact = new float [maxDistFactLength];
    newDistFactIndex = new int [numSubLists];
    newDistFactIndex[0] = 0;
    }

  if (id1Len == 0)
    {
    memcpy(newIdList, id2, sizeof(int) * id2Len);
    memcpy(newIdListIndex, id2Idx, sizeof(int) * numSubLists);
    newIdListLength = id2Len;

    if (newDistFact)
      {
      memcpy(newDistFact, dist2, sizeof(float) * dist2Len);
      memcpy(newDistFactIndex, dist2Idx, sizeof(int) * numSubLists);
      }

    newDistFactLength = dist2Len;
    }
  else if (id2Len == 0)
    {
    memcpy(newIdList, id1, sizeof(int) * id1Len);
    memcpy(newIdListIndex, id1Idx, sizeof(int) * numSubLists);
    newIdListLength = id1Len;

    if (newDistFact)
      {
      memcpy(newDistFact, dist1, sizeof(float) * dist1Len);
      memcpy(newDistFactIndex, dist1Idx, sizeof(int) * numSubLists);
      }

    newDistFactLength = dist1Len;
    }
  else
    {
    int nextid = 0;
    int nextdf = 0;
  
    float *dist = NULL;
    int numdf = 0;
  
    for (i=0; i<numSubLists; i++)
      {
      int lastList = (i == numSubLists-1);
  
      int *ids = id1 + id1Idx[i];
      int numids = (lastList ? id1Len : id1Idx[i+1]) - id1Idx[i];
      if (numids > 0)
        {
        memcpy(newIdList + nextid, ids, numids * sizeof(int));
        }
  
      nextid += numids;
  
      if (distFact)
        {
        dist = dist1 + dist1Idx[i];
        numdf = (lastList ? dist1Len : dist1Idx[i+1]) - dist1Idx[i];
        if (numdf > 0)
          {
          memcpy(newDistFact + nextdf, dist, numdf * sizeof(float));
          nextdf += numdf;
          }
        }
  
      // Make a set of the ids we've just written.  We only want to add
      // ids from list 2 if they did not exist in list 1.
  
      vtkstd::set<int> idSet;
  
      for (id=0; id < numids; id++)
        {
        idSet.insert(ids[id]); 
        }
  
      ids = id2 + id2Idx[i];
      numids = (lastList ? id2Len : id2Idx[i+1]) - id2Idx[i];
  
      if (distFact)
        {
        dist = dist2 + dist2Idx[i];
        numdf = (lastList ? dist2Len : dist2Idx[i+1]) - dist2Idx[i];
        }
      else
        {
        numdf = 0;
        }
  
      for (id=0; id < numids; id++)
        {
        vtkstd::pair<vtkstd::set<int>::iterator, bool> inserted =
  
          idSet.insert(ids[id]);
  
        if (inserted.second)    // here's a new ID
          {
          newIdList[nextid++] = ids[id];

          if (numdf > 0)
            {
            // There is either 1 or 0 distribution factors

            newDistFact[nextdf++] = dist[id];
            }
          }
        }
  
      if (!lastList)
        {
        newIdListIndex[i+1] = nextid;
        if (distFact)
          {
          newDistFactIndex[i+1] = nextdf;
          }
        }
      else
        {
        newIdListLength = nextid;
        newDistFactLength = nextdf;
        }
      }
    }

  if (idNew)
    {
    *idNew = newIdList;
    }
  else
    {
    delete [] newIdList;
    }
  if (idNewIdx)
    {
    *idNewIdx = newIdListIndex;
    }
  else
    {
    delete [] newIdListIndex;
    }
  if (idNewLen)
    {
    *idNewLen = newIdListLength;
    }

  if (distNew)
    {
    *distNew = newDistFact;
    }
  else if (newDistFact)
    {
    delete [] newDistFact;
    }
  if (distNewIdx)
    {
    *distNewIdx = newDistFactIndex;
    }
  else if (newDistFactIndex)
    {
    delete [] newDistFactIndex;
    }
  if (distNewLen)
    {
    *distNewLen = newDistFactLength;
    }
  
  return 0;
}

// This merge function serves two purposes.  It can initialize
// the global fields of the current object with the values in the
// the global fields on the object passed in.
//
// Or it can carefully merge missing global fields in to our global
// fields.  Often in Exodus files, global data is missing if it
// is irrelevant.  For example, a block name is "NULL" if the
// the file has no cells in that block.
//
// "Global" in this context means merge all fields which don't depend
// on which cells are included in the model.  (In other contexts we
// use "global" to refer to all metadata which doesn't depend on the
// cells, the time step, or the variables chosen.)
//
// TODO - We may need to write a version that detects duplicate
//   cells in the two models that are to be merged.  Maybe detecting
//   and filtering duplicate cells should be an option.

int vtkModelMetadata::MergeGlobalInformation(const vtkModelMetadata *em)
{
  int i;

  if (!this->Title && em->GetTitle())
    {
    this->SetTitle(vtkModelMetadata::StrDupWithNew(em->GetTitle()));
    }

  int num = em->GetNumberOfQARecords();

  if (this->NumberOfQARecords < num)
    {
    typedef char *p4[4];

    p4 *qaRecs = new p4 [num];
    char *name = 0;
    char *version = 0;
    char *date = 0;
    char *time = 0;

    for (i=0; i<num; i++)
      {
      em->GetQARecord(i, &name, &version, &date, &time);
      qaRecs[i][0] = vtkModelMetadata::StrDupWithNew(name);
      qaRecs[i][1] = vtkModelMetadata::StrDupWithNew(version);
      qaRecs[i][2] = vtkModelMetadata::StrDupWithNew(date);
      qaRecs[i][3] = vtkModelMetadata::StrDupWithNew(time);
      }

    this->SetQARecords(num, qaRecs);
    }

  num = em->GetNumberOfInformationLines();

  if (this->NumberOfInformationLines < num)
    {
    char **newLines;
    em->GetInformationLines(&newLines);

    char **lines = vtkModelMetadata::CopyLines(newLines, num);
    this->SetInformationLines(num, lines);
    }

  if (this->CoordinateNames == NULL)
    {
    num = em->GetDimension();
    char **lines = vtkModelMetadata::CopyLines(em->GetCoordinateNames(), num);
    this->SetCoordinateNames(num, lines);
    }

  num = em->GetNumberOfTimeSteps();

  if (this->NumberOfTimeSteps < num)
    {
    float *ts = new float [num];
    memcpy(ts, em->GetTimeStepValues(), num * sizeof(float));

    this->SetTimeSteps(num, ts);
    this->TimeStepIndex = em->TimeStepIndex;
    }

  // Assumption - Either we have no block information and are copying
  //   it from the supplied model, or the block IDs are the same and in the 
  //   same order in both models, but we may be missing some information.

  num = em->GetNumberOfBlocks();
  int nblocks = this->GetNumberOfBlocks();

  if (nblocks == 0)
    {
    this->SetNumberOfBlocks(num);

    this->SetBlockIds(
      vtkModelMetadata::CopyInts(em->GetBlockIds(), num));

    int *nvals = new int [num];
    memset(nvals , 0, sizeof(int) * num);

    this->SetBlockNumberOfElements(nvals);

    this->SetBlockElementType(
      vtkModelMetadata::CopyLines(em->GetBlockElementType(), num));

    this->SetBlockNodesPerElement(
      vtkModelMetadata::CopyInts(em->GetBlockNodesPerElement(), num));

    this->SetBlockNumberOfAttributesPerElement(
      vtkModelMetadata::CopyInts(em->GetBlockNumberOfAttributesPerElement(), num));
    }
  else if (nblocks != num)
    {
    vtkErrorMacro(<< "Merging metadata from different files");
    return 1;
    }
  else
    {
    char **types = em->GetBlockElementType();
    int *nodes = em->GetBlockNodesPerElement();
    int *atts = em->GetBlockNumberOfAttributesPerElement();

    for (i=0; i < nblocks; i++)
      {
      if (!strcmp(this->BlockElementType[i], "NULL") &&
          strcmp(em->BlockElementType[i], "NULL"))
        {
        delete [] this->BlockElementType[i];
        this->BlockElementType[i] = vtkModelMetadata::StrDupWithNew(types[i]);
        this->BlockNodesPerElement[i] = nodes[i];
        this->BlockNumberOfAttributesPerElement[i] = atts[i];
        }
      }
    }

  num = em->GetNumberOfNodeSets();

  if (this->NumberOfNodeSets < num)
    {
    int *ids = vtkModelMetadata::CopyInts(em->GetNodeSetIds(), num);

    this->SetNumberOfNodeSets(num);
    this->SetNodeSetIds(ids);
    }

  num = em->GetNumberOfSideSets();

  if (this->NumberOfSideSets < num)
    {
    int *ids = vtkModelMetadata::CopyInts(em->GetSideSetIds(), num);

    this->SetNumberOfSideSets(num);
    this->SetSideSetIds(ids);
    }

  num = em->GetNumberOfBlockProperties();
  int nblockProp = this->NumberOfBlockProperties;
  int nvals = num * this->NumberOfBlocks;

  if (nvals > 0)
    {
    if (nblockProp < num)
      {
      this->SetBlockPropertyNames(num,
        vtkModelMetadata::CopyLines(em->GetBlockPropertyNames(), num));
  
      this->SetBlockPropertyValue(
        vtkModelMetadata::CopyInts(em->GetBlockPropertyValue(), nvals));
      }
    else if (nblockProp == num)
      {
      int *myVal = this->BlockPropertyValue;
      int *newVal = em->GetBlockPropertyValue();
    
      for (i=0; i<nvals; i++)
        {
        if ((myVal[i] == 0) && (newVal[i] != 0)) myVal[i] = newVal[i];
        }
      } 
    else
      {
      vtkErrorMacro(<< "Merging metadata from different files");
      return 1;
      }
    }

  num = em->GetNumberOfNodeSetProperties();
  int nnsetProp = this->NumberOfNodeSetProperties;
  nvals = num * this->NumberOfNodeSets;

  if (nvals > 0)
    {
    if (nnsetProp < num)
      {
      this->SetNodeSetPropertyNames(num,
        vtkModelMetadata::CopyLines(em->GetNodeSetPropertyNames(), num));
  
      this->SetNodeSetPropertyValue(
        vtkModelMetadata::CopyInts(em->GetNodeSetPropertyValue(), nvals));
      }
    else if (nnsetProp == num)
      {
      int *myVal = this->NodeSetPropertyValue;
      int *newVal = em->GetNodeSetPropertyValue();
    
      for (i=0; i<nvals; i++)
        {
        if ((myVal[i] == 0) && (newVal[i] != 0)) myVal[i] = newVal[i];
        }
      } 
    else
      {
      vtkErrorMacro(<< "Merging metadata from different files");
      return 1;
      }
    }

  num = em->GetNumberOfSideSetProperties();
  int nssetProp = this->NumberOfSideSetProperties;
  nvals = num * this->NumberOfSideSets;

  if (nvals > 0)
    {
    if (nssetProp < num)
      {
      this->SetSideSetPropertyNames(num,
        vtkModelMetadata::CopyLines(em->GetSideSetPropertyNames(), num));
  
      this->SetSideSetPropertyValue(
        vtkModelMetadata::CopyInts(em->GetSideSetPropertyValue(), nvals));
      }
    else if (nssetProp == num)
      {
      int *myVal = this->SideSetPropertyValue;
      int *newVal = em->GetSideSetPropertyValue();
    
      for (i=0; i<nvals; i++)
        {
        if ((myVal[i] == 0) && (newVal[i] != 0)) myVal[i] = newVal[i];
        }
      } 
    else
      {
      vtkErrorMacro(<< "Merging metadata from different files");
      return 1;
      }
    }

  num = em->GetNumberOfGlobalVariables();

  if (num > this->NumberOfGlobalVariables)
    {
    this->SetGlobalVariableNames(num,
        vtkModelMetadata::CopyLines(em->GetGlobalVariableNames(), num));

    float *gv = new float [num];
    memcpy(gv, em->GetGlobalVariableValue(), sizeof(float) * num);

    this->SetGlobalVariableValue(gv);
    }

  num = em->GetOriginalNumberOfElementVariables();

  if (num > this->OriginalNumberOfElementVariables)
    {
    char **orig = 
      vtkModelMetadata::CopyLines(em->GetOriginalElementVariableNames(), num);

    int numvar = em->GetNumberOfElementVariables();
    char **varname = 
      vtkModelMetadata::CopyLines(em->GetElementVariableNames(), numvar);

    int *comp = 
      vtkModelMetadata::CopyInts(em->GetElementVariableNumberOfComponents(),
                                 numvar);
    int *map =  
      vtkModelMetadata::CopyInts(em->GetMapToOriginalElementVariableNames(),
                                 numvar);

    this->SetElementVariableInfo(num, orig, numvar, varname, comp, map);
    }

  num = em->GetOriginalNumberOfNodeVariables();

  if (num > this->OriginalNumberOfNodeVariables)
    {
    char **orig = 
      vtkModelMetadata::CopyLines(em->GetOriginalNodeVariableNames(), num);

    int numvar = em->GetNumberOfNodeVariables();
    char **varname = 
      vtkModelMetadata::CopyLines(em->GetNodeVariableNames(), numvar);

    int *comp = 
      vtkModelMetadata::CopyInts(em->GetNodeVariableNumberOfComponents(), numvar);
    int *map =  
      vtkModelMetadata::CopyInts(em->GetMapToOriginalNodeVariableNames(), numvar);

    this->SetNodeVariableInfo(num, orig, numvar, varname, comp, map);
    }

  int *mytt = this->ElementVariableTruthTable;
  int *newtt = em->GetElementVariableTruthTable();
  int nvars = em->GetOriginalNumberOfElementVariables();

  if (newtt)
    {
    int ttsize = this->NumberOfBlocks * nvars;

    if (mytt == NULL)
      {
      mytt = new int [ttsize];
      memcpy(mytt, newtt, ttsize * sizeof(int));
      this->SetElementVariableTruthTable(mytt);
      }
    else
      {
      for (i=0; i<ttsize; i++)
        {
        if (newtt[i] == 1) mytt[i] = 1;
        }
      }
    }

  return 0;
}

// Merge the metadata passed in to this metadata.  If this metadata
// object is empty, initialize it from the metadata passed in.
//
// ASSUMPTION:
//   The vtkModelMetadata object passed in comes from the same 
//   distributed file that this object was initialized from.  
//   It has the same variables and is for the same time step.  What
//   will be different is the cells that are represented in the
//   two models.
//
//   We assume that a given cell ID does not appear in more than one 
//   file in a distributed data set.  So when merging two ModelMetadata objects
//   we just append element lists.  However, we assume that node IDs may
//   appear in more than one file, and we filter out duplicate IDs when
//   merging the ModelMetadata data from two files.
//
// TODO - vtkDistributedDataFilter has a mode where boundary cells
//    are duplicated.  In that case, when merging one submesh's
//    metadata with another, we would have to filter out duplicate cells.
//    It would be good to have a flag that says whether this need to
//    be done or not.

int vtkModelMetadata::MergeModelMetadata(const vtkModelMetadata *em)
{
  int i, rc;

  // Merge the global information - that which doesn't depend on
  // which elements (cells) are in the two models.

  rc = this->MergeGlobalInformation(em);

  if (rc)
    {
    return 1;
    }

  // If the input object is empty, we're done

  int nBlocksNew = em->GetNumberOfBlocks();
  int *nelts = em->GetBlockNumberOfElements();
  int nCellsNew = 0;

  for (i=0; nelts && (i < nBlocksNew); i++)
    {
    nCellsNew += nelts[i];
    }

  if (nCellsNew == 0) return 0;

  // BLOCKS

  float *farray = NULL;
  int *index = NULL;
  int newSize = 0;
  int nblocks = this->NumberOfBlocks;

  this->AppendFloatLists(nblocks, 
     this->BlockAttributes, this->BlockAttributesIndex, this->SizeBlockAttributeArray,
     em->BlockAttributes, em->BlockAttributesIndex, em->SizeBlockAttributeArray,
     &farray, &index, &newSize);

  FREE(this->BlockAttributes);
  FREE(this->BlockAttributesIndex);

  this->BlockAttributes = farray;
  this->BlockAttributesIndex = index;
  this->SizeBlockAttributeArray = newSize;

  int *iarray = NULL;

  this->AppendIntegerLists(nblocks,
     this->BlockElementIdList, this->BlockElementIdListIndex, this->SumElementsPerBlock,
     em->BlockElementIdList, em->BlockElementIdListIndex, em->SumElementsPerBlock,
     &iarray, &index, &newSize);

  FREE(this->BlockElementIdList);
  FREE(this->BlockElementIdListIndex);

  this->BlockElementIdList = iarray;
  this->BlockElementIdListIndex = index;
  this->SumElementsPerBlock = newSize;

  for (i=0; i<nblocks; i++)
    {
    this->BlockNumberOfElements[i] += em->BlockNumberOfElements[i];
    }

  // NODE SETS

  if (em->SumNodesPerNodeSet > 0)
    {
    int *index2 = NULL;
    int newSize2 = 0;
    int nnsets = this->NumberOfNodeSets;

    this->MergeIdLists( nnsets,
      this->NodeSetNodeIdList, this->NodeSetNodeIdListIndex, this->SumNodesPerNodeSet,
        this->NodeSetDistributionFactors, this->NodeSetDistributionFactorIndex,
        this->SumDistFactPerNodeSet,
      em->NodeSetNodeIdList, em->NodeSetNodeIdListIndex, em->SumNodesPerNodeSet,
        em->NodeSetDistributionFactors, em->NodeSetDistributionFactorIndex,
        em->SumDistFactPerNodeSet,
      &iarray, &index, &newSize,
      &farray, &index2, &newSize2);

    FREE(this->NodeSetNodeIdList); 
    FREE(this->NodeSetNodeIdListIndex);
    FREE(this->NodeSetDistributionFactors);
    FREE(this->NodeSetDistributionFactorIndex);
  
    this->NodeSetNodeIdList = iarray; 
    this->NodeSetNodeIdListIndex = index;
    this->NodeSetDistributionFactors = farray;
    this->NodeSetDistributionFactorIndex = index2;
  
    this->SumNodesPerNodeSet = newSize;
    this->SumDistFactPerNodeSet = newSize2;

    int lastset = nnsets-1;
    int *setSize = new int [nnsets];
    int *setDF = new int [nnsets];
  
    for (i=0; i<lastset; i++)
      {
      setSize[i] = index[i+1] - index[i];
      if (index2)
        {
        setDF[i] = index2[i+1] - index2[i];
        }
      else
        {
        setDF[i] = 0;
        }
      }

    setSize[lastset] = newSize - index[lastset];

    if (index2)
      {
      setDF[lastset] = newSize2 - index2[lastset];
      }
    else
      {
      setDF[lastset] = 0;
      }

    FREE(this->NodeSetNumberOfDistributionFactors);
    this->NodeSetNumberOfDistributionFactors = setDF;

    FREE(this->NodeSetSize);
    this->NodeSetSize = setSize;
    }

  // SIDE SETS

  if (em->SumSidesPerSideSet > 0)
    {
    int nssets = this->NumberOfSideSets;
  
    this->AppendIntegerLists(nssets,
       this->SideSetElementList, this->SideSetListIndex, this->SumSidesPerSideSet,
       em->SideSetElementList, em->SideSetListIndex, em->SumSidesPerSideSet,
       &iarray, &index, &newSize);
  
    FREE(this->SideSetElementList);
    this->SideSetElementList = iarray;
  
    FREE(index);
  
    this->AppendIntegerLists(nssets,
       this->SideSetSideList, this->SideSetListIndex, this->SumSidesPerSideSet,
       em->SideSetSideList, em->SideSetListIndex, em->SumSidesPerSideSet,
       &iarray, &index, &newSize);
  
    FREE(this->SideSetSideList);
    this->SideSetSideList = iarray;
  
    FREE(index);
  
    this->AppendIntegerLists(nssets,
       this->SideSetNumDFPerSide, this->SideSetListIndex, this->SumSidesPerSideSet,
       em->SideSetNumDFPerSide, em->SideSetListIndex, em->SumSidesPerSideSet,
       &iarray, &index, &newSize);
  
    FREE(this->SideSetNumDFPerSide);
    this->SideSetNumDFPerSide= iarray;
  
    FREE(this->SideSetListIndex);
    this->SideSetListIndex = index;
  
    this->SumSidesPerSideSet = newSize;
  
    this->AppendFloatLists(nssets,
       this->SideSetDistributionFactors, this->SideSetDistributionFactorIndex, 
         this->SumDistFactPerSideSet,
       em->SideSetDistributionFactors, em->SideSetDistributionFactorIndex, 
         em->SumDistFactPerSideSet,
       &farray, &index, &newSize);
  
    FREE(this->SideSetDistributionFactors);
    FREE(this->SideSetDistributionFactorIndex);
  
    this->SideSetDistributionFactors = farray;
    this->SideSetDistributionFactorIndex = index;
    this->SumDistFactPerSideSet = newSize;
    
    int lastset = nssets - 1;
    int *setSize = new int [nssets];
    int *setDF = new int [nssets];
  
    for (i=0; i<lastset; i++)
      {
      setSize[i] = this->SideSetListIndex[i+1] - this->SideSetListIndex[i];
      if (index)
        {
        setDF[i] = index[i+1] - index[i];
        }
      else
        {
        setDF[i] = 0;
        }
      }

    setSize[lastset] = this->SumSidesPerSideSet - this->SideSetListIndex[lastset];

    if (index)
      {
      setDF[lastset] = newSize - index[lastset];
      }
    else
      {
      setDF[lastset] = 0;
      }

    FREE(this->SideSetNumberOfDistributionFactors);
    this->SideSetNumberOfDistributionFactors = setDF;

    FREE(this->SideSetSize);
    this->SideSetSize = setSize;
    }

  return 0;
}
int *vtkModelMetadata::CopyInts(int *vals, int num)
{
  int *newvals = NULL;

  if (num == 0) return newvals;

  newvals = new int [num];
  memcpy(newvals, vals, sizeof(int) * num);

  return newvals;
}
char **vtkModelMetadata::CopyLines(char **lines, int num)
{
  char **newlines = NULL;

  if (num == 0) return newlines;

  newlines = new char * [num];

  for (int i=0; i<num; i++)
    {
    newlines[i]  = vtkModelMetadata::StrDupWithNew(lines[i]);
    }

  return newlines;
}
void vtkModelMetadata::ExtractCellsFromBlockData(vtkModelMetadataSTLCloak *idset,
                          vtkModelMetadata *mmd)
{
  int i, j, k;
  int nblocks, nelts;

  if ((nblocks = this->NumberOfBlocks) < 1) return;

  if ((nelts = this->SumElementsPerBlock) < 1) return;

  char *extractElt = new char [nelts];

  int *eltIds = this->BlockElementIdList;
  float *eltAtts = this->BlockAttributes;
  int *blockSize = this->BlockNumberOfElements;
  int *blockAtts = this->BlockNumberOfAttributesPerElement;

  int *newEltIds = NULL;
  float *newEltAtts = NULL;
  int *newBlockSize = new int [nblocks];

  int numNewElts = 0;
  int numNewAtts = 0;
  int ii=0;

  for (i=0; i<nblocks; i++)
    {
    newBlockSize[i] = 0;

    for (j=0; j<blockSize[i]; j++)
      {
      vtkstd::set<int>::iterator it = idset->IntSet.find(eltIds[ii]);

      if (it == idset->IntSet.end())
        {
        extractElt[ii] = 0;
        }
      else
        {
        extractElt[ii] = 1;
        newBlockSize[i]++;
        }

      ii++;
      }

    numNewElts += newBlockSize[i];
    numNewAtts += (newBlockSize[i] * blockAtts[i]);
    }

  if (numNewElts > 0)
    {
    newEltIds = new int [numNewElts];

    if (numNewAtts > 0)
      {
      newEltAtts = new float [numNewAtts];
      }

    int *nextId = newEltIds;
    float *nextAtt = newEltAtts;

    ii=0;

    for (i=0; i<nblocks; i++)
      {
      for (j=0; j<blockSize[i]; j++)
        {
        if (extractElt[ii++])
          {
          *nextId++ = *eltIds;

          for (k=0; k<blockAtts[i]; k++)
            {
            *nextAtt++ = eltAtts[k];
            }
          }

        eltIds++;
        eltAtts += blockAtts[i];
        }
      }
    }

  mmd->SetBlockNumberOfElements(newBlockSize);

  if (newEltIds)
    {
    mmd->SetBlockElementIdList(newEltIds);

    if (newEltAtts)
      {
      mmd->SetBlockAttributes(newEltAtts);
      }
    }

  delete [] extractElt;
  
  return;
}
void vtkModelMetadata::ExtractNodesFromNodeSetData(vtkModelMetadataSTLCloak *idset,
                          vtkModelMetadata *mmd)
{
  int i, j;
  int nnsets, nnodes;

  if ((nnsets = this->NumberOfNodeSets) < 1) return;

  if ((nnodes = this->SumNodesPerNodeSet) < 1) return;

  char *extractNodes = new char [nnodes];

  int *nsIds = this->NodeSetNodeIdList;
  float *nsDF = this->NodeSetDistributionFactors;
  int *nsSize = this->NodeSetSize;
  int *nsNumDF = this->NodeSetNumberOfDistributionFactors;

  int *newnsIds = NULL;
  float *newnsDF = NULL;
  int *newnsSize = new int [nnsets];
  int *newnsNumDF = new int [nnsets];

  int numNewNodes = 0;
  int numNewDF = 0;
  int ii=0;

  for (i=0; i<nnsets; i++)
    {
    newnsSize[i] = 0;

    for (j=0; j<nsSize[i]; j++)
      {
      vtkstd::set<int>::iterator it = idset->IntSet.find(nsIds[ii]);

      if (it == idset->IntSet.end())
        {
        extractNodes[ii] = 0;
        }
      else
        {
        extractNodes[ii] = 1;
        newnsSize[i]++;
        }

      ii++;
      }

    if (nsNumDF[i] > 0) newnsNumDF[i] = newnsSize[i];
    else                newnsNumDF[i] = 0;

    numNewNodes += newnsSize[i];
    numNewDF += newnsNumDF[i];
    }

  if (numNewNodes > 0)
    {
    newnsIds = new int [numNewNodes];

    if (numNewDF > 0)
      {
      newnsDF = new float [numNewDF];
      }

    int *nextId = newnsIds;
    float *nextDF = newnsDF;
    ii = 0;

    for (i=0; i<nnsets; i++)
      {
      int hasDF = (nsNumDF[i] > 0);

      for (j=0; j<nsSize[i]; j++)
        {
        if (extractNodes[ii++])
          {
          *nextId++ = *nsIds;

          if (hasDF)
            {
            *nextDF++ = *nsDF;
            }
          }

        nsIds++;
        if (hasDF) nsDF++;
        }
      }
    }

  mmd->SetNodeSetSize(newnsSize);
  mmd->SetNodeSetNumberOfDistributionFactors(newnsNumDF);

  if (newnsIds)
    {
    mmd->SetNodeSetNodeIdList(newnsIds);

    if (newnsDF)
      {
      mmd->SetNodeSetDistributionFactors(newnsDF);
      }
    }

  delete [] extractNodes;
  
  return;
}
void vtkModelMetadata::ExtractSidesFromSideSetData(vtkModelMetadataSTLCloak *idset,
                          vtkModelMetadata *mmd)
{
  int i, j;
  int nssets, nsides;

  if ((nssets = this->NumberOfSideSets) < 1) return;

  if ((nsides = this->SumSidesPerSideSet) < 1) return;

  char *extractSides = new char [nsides];

  int *ssElts = this->SideSetElementList;
  int *ssSides = this->SideSetSideList;
  int *ssNumDFperSide = this->SideSetNumDFPerSide;
  float *ssDF = this->SideSetDistributionFactors;
  int *ssSize = this->SideSetSize;

  int *newssElts = NULL;
  int *newssSides = NULL;
  int *newssNumDFperSide = NULL;
  float *newssDF = NULL;
  int *newssSize = new int [nssets];
  int *newssNumDF = new int [nssets];

  int numNewSides = 0;
  int numNewDF = 0;
  int ii=0;

  for (i=0; i<nssets; i++)
    {
    newssSize[i] = 0;
    newssNumDF[i] = 0;

    for (j=0; j<ssSize[i]; j++)
      {
      vtkstd::set<int>::iterator it = idset->IntSet.find(ssElts[ii]);

      if (it == idset->IntSet.end())
        {
        extractSides[ii] = 0;
        }
      else
        {
        extractSides[ii] = 1;
        newssSize[i]++;
        newssNumDF[i] += ssNumDFperSide[ii];
        }

      ii++;
      }

    numNewSides += newssSize[i];
    numNewDF += newssNumDF[i];
    }

  if (numNewSides > 0)
    {
    newssElts = new int [numNewSides];
    newssSides = new int [numNewSides];
    newssNumDFperSide = new int [numNewSides];

    if (numNewDF > 0)
      {
      newssDF = new float [numNewDF];
      }

    int nextId = 0;
    int nextDF = 0;

    for (ii=0; ii<nsides; ii++)
      {
      int ndf = ssNumDFperSide[ii];

      if (extractSides[ii])
        {
        newssElts[nextId] = ssElts[ii];
        newssSides[nextId] = ssSides[ii];
        newssNumDFperSide[nextId] = ndf;

        nextId++;

        for (i=0; i<ndf; i++)
          {
          newssDF[nextDF++] = ssDF[i];
          }
        }

      ssDF += ndf;
      }
    }

  delete [] extractSides;

  mmd->SetSideSetSize(newssSize);
  mmd->SetSideSetNumberOfDistributionFactors(newssNumDF);

  if (newssElts)
    {
    mmd->SetSideSetElementList(newssElts);
    mmd->SetSideSetSideList(newssSides);
    mmd->SetSideSetNumDFPerSide(newssNumDFperSide);

    if (newssDF)
      {
      mmd->SetSideSetDistributionFactors(newssDF);
      }
    }
  
  return;
}

vtkModelMetadata *vtkModelMetadata::ExtractGlobalMetadata()
{
  vtkModelMetadata *mmd = vtkModelMetadata::New();

  mmd->MergeGlobalInformation(this);

  return mmd;
}

vtkModelMetadata *vtkModelMetadata::ExtractModelMetadata(
  vtkIdTypeArray *globalCellIdList,
  vtkDataSet *grid)
{
  int i;

  vtkModelMetadata *em = this->ExtractGlobalMetadata();

  vtkIdType ncells = globalCellIdList->GetNumberOfTuples();

  if (ncells < 1)
    {
    return em;
    }

  vtkModelMetadataSTLCloak *cellIds = 
    new vtkModelMetadataSTLCloak;     // the cells we want to extract
  vtkModelMetadataSTLCloak *nodeIds = 
    new vtkModelMetadataSTLCloak;     // the nodes they include

  vtkIdType *ids = globalCellIdList->GetPointer(0);

  for (i=0; i<ncells; i++)
    {
    cellIds->IntSet.insert(ids[i]);
    }

  ncells = cellIds->IntSet.size();

  vtkDataArray *ca = grid->GetCellData()->GetGlobalIds();
  vtkDataArray *pa = grid->GetPointData()->GetGlobalIds();

  if (!ca || !pa)
    {
    vtkErrorMacro(<< "vtkModelMetadata::ExtractModelMetadata needs id arrays");
    em->Delete();
    return NULL;
    }

  vtkIdTypeArray *ica  = vtkIdTypeArray::SafeDownCast(ca);
  vtkIdTypeArray *ipa  = vtkIdTypeArray::SafeDownCast(pa);

  if (!ica || !ipa)
    {
    vtkErrorMacro(<< "vtkModelMetadata::ExtractModelMetadata id arrays not vtkIdType");
    em->Delete();
    return NULL;
    }

  vtkIdType *gcids = ica->GetPointer(0);  // global cell ids
  vtkIdType *gpids = ipa->GetPointer(0);  // global point ids

  int gridCells = grid->GetNumberOfCells();
  vtkIdList *ptIds = vtkIdList::New();

  for (vtkIdType c = 0; c<gridCells; c++)
    {
    vtkstd::set<int>::iterator it = cellIds->IntSet.find(gcids[c]);

    if (it != cellIds->IntSet.end())
      {
      grid->GetCellPoints(c, ptIds);

      vtkIdType npoints = ptIds->GetNumberOfIds();

      for (i=0; i<npoints; i++)
        {
        nodeIds->IntSet.insert(gpids[ptIds->GetId(i)]);
        }
      }
    }

  ptIds->Delete();

  // Block information

  if (this->NumberOfBlocks)
    {
    this->ExtractCellsFromBlockData(cellIds, em);
    }

  // Node set information

  if (this->NumberOfNodeSets)
    {
    this->ExtractNodesFromNodeSetData(nodeIds, em);
    }

  // Side set information

  if (this->NumberOfSideSets)
    {
    this->ExtractSidesFromSideSetData(cellIds, em);
    }

  delete cellIds;
  delete nodeIds;

  return em;
}
char *vtkModelMetadata::StrDupWithNew(const char *s)
{
  char *newstr = NULL;

  if (s)
    {
    int len = static_cast<int>(strlen(s));
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

char *vtkModelMetadata::FindOriginalNodeVariableName(const char *name, int component)
{
  int idx = -1;

  for (int i=0; i<this->NumberOfNodeVariables; i++)
    {
    if (!strcmp(name, this->NodeVariableNames[i]))
      {
      idx = i;
      break;
      }
    }  

  if (idx < 0)
    {
    return NULL;  
    }

  int origIdx = this->MapToOriginalNodeVariableNames[idx];

  if ( (component<0) || 
       (component >= this->NodeVariableNumberOfComponents[idx]))
    {
    return NULL;
    }

  return this->OriginalNodeVariableNames[origIdx + component];
}
int vtkModelMetadata::ElementVariableIsDefinedInBlock(char *varname, int blockId)
{
  int i;
  int varIdx = -1;

  if (this->AllVariablesDefinedInAllBlocks)
    {
    return 1;
    }

  for (i=0; i<this->OriginalNumberOfElementVariables; i++)
    {
    if (!strcmp(varname, this->OriginalElementVariableNames[i]))
      {
      varIdx = i;
      break;
      }
    }

  int blockIdx = this->GetBlockLocalIndex(blockId);

  if ( (blockIdx<0) || (varIdx<0))
    {
    return 1;   // by default we say everything is defined
    }

  int loc = (blockIdx * this->OriginalNumberOfElementVariables) + varIdx;

  return (int)this->ElementVariableTruthTable[loc];
}

char *vtkModelMetadata::FindOriginalElementVariableName(const char *name, int component)
{
  int idx = -1;

  for (int i=0; i<this->NumberOfElementVariables; i++)
    {
    if (!strcmp(name, this->ElementVariableNames[i]))
      {
      idx = i;
      break;
      }
    }  

  if (idx < 0)
    {
    return NULL;  
    }

  int origIdx = this->MapToOriginalElementVariableNames[idx];

  if ( (component<0) || 
       (component >= this->ElementVariableNumberOfComponents[idx]))
    {
    return NULL;
    }

  return this->OriginalElementVariableNames[origIdx + component];
}
//-------------------------------------
// Display contents for debugging
//-------------------------------------

void vtkModelMetadata::ShowFloats(const char *what, int num, float *f)
{
  if (num < 1) return;
  if (!f) return;
  cout << what << endl;
  for (int i=0; i<num; i++)
    {
    if (i && (i % 10 == 0)) cout << endl;
    cout << " " << f[i];
    }
  cout << endl;
}
void vtkModelMetadata::ShowLines(const char *what, int num, char **l)
{
  if (num < 1) return;
  if (!l) return;
  cout << what << endl;
  for (int i=0; i<num; i++)
    {
    if (l[i]) cout << "  " << l[i] << endl;
    }
}
void vtkModelMetadata::ShowIntArray(const char *what, int numx, int numy, int *id)
{
  if (numx < 1) return;
  if (numy < 1) return;
  if (id == NULL) return;

  cout << what << endl;
  for (int x=0; x<numx; x++)
    {
    for (int y=0; y<numy; y++)
      {
      cout << " " << *id++;
      }
    cout << endl;
    }
  cout << endl;
}
void vtkModelMetadata::ShowInts(const char *what, int num, int *id)
{
  if (num < 1) return;
  if (!id) return;
  cout << what << endl;
  for (int i=0; i<num; i++)
    {
    if (i && (i % 10 == 0)) cout << endl;
    cout << " " << id[i];
    }
  cout << endl;
}
void vtkModelMetadata::ShowListsOfInts(const char *what, int *list, 
                       int nlists, int *idx, int len, int verbose)
{
  int i, j, ii;
  if (len == 0) return;

  cout << what << endl;
  for (i=0; i<nlists; i++)
    {
    int start = idx[i];
    int end = ((i == nlists-1) ? len : idx[i+1]);

    cout << i << ") ";

    for (j=start,ii=0; j < end; j++,ii++)
      {
      if (ii && ((ii%20)==0))
        {
        if (verbose)
          {
          cout << endl;
          } 
        else
          {
          cout << "..." ;
          break;
          } 
        }
      cout << list[j] << " "; 
      }
    cout << endl;
    }
}
void vtkModelMetadata::ShowListsOfFloats(const char *what, float *list, 
                              int nlists, int *idx, int len, int verbose)
{
  int i, j, ii;
  if (len == 0) return;

  cout << what << endl;
  for (i=0; i<nlists; i++)
    {
    int start = idx[i];
    int end = ((i == nlists-1) ? len : idx[i+1]);

    cout << i << ") ";

    for (j=start,ii=0; j < end; j++,ii++)
      {
      if (ii && ((ii%20)==0))
        {
        if (verbose)
          {
          cout << endl;
          } 
        else
          {
          cout << "...";
          break;
          } 
        }
      cout << list[j] << " "; 
      }
    cout << endl;
    }
}

void vtkModelMetadata::PrintLocalInformation()
{
  int verbose = 0;

  // Only print out lists of element IDs, distribution factors, node
  // IDs and so on if VERBOSE_TESTING is defined in the environment.
  // You only want to see these for very small test files.

  char *val = getenv("VERBOSE_TESTING");
  if (val) verbose = 1;
  val = getenv("VERY_VERBOSE_TESTING");
  if (val) verbose = 2;

  cout << "Metadata local information" << endl;
  cout << "========================================" << endl;

  cout << "Time step (starting with 0): " << this->TimeStepIndex << endl;

  this->ShowInts("BlockNumberOfElements", this->NumberOfBlocks, 
                 this->BlockNumberOfElements);

  if (verbose)
    {
    // Only show these for really small data sets.

    this->ShowListsOfInts("BlockElementIdList", this->BlockElementIdList, 
                         this->NumberOfBlocks, this->BlockElementIdListIndex, 
                         this->SumElementsPerBlock, (verbose>1));
    this->ShowListsOfFloats("BlockAttributes", this->BlockAttributes, 
                        this->NumberOfBlocks, this->BlockAttributesIndex, 
                        this->SizeBlockAttributeArray, (verbose>1));
    }

  this->ShowInts("NodeSetSize", this->NumberOfNodeSets, this->NodeSetSize);
  this->ShowInts("NodeSetNumberOfDistributionFactors", this->NumberOfNodeSets, 
                  this->NodeSetNumberOfDistributionFactors);

  if (verbose)
    {
  this->ShowListsOfInts("NodeSetNodeIdList", this->NodeSetNodeIdList, 
                         this->NumberOfNodeSets, this->NodeSetNodeIdListIndex, 
                         this->SumNodesPerNodeSet, (verbose>1));
  this->ShowListsOfFloats("NodeSetDistributionFactors", 
                        this->NodeSetDistributionFactors, 
                        this->NumberOfNodeSets, 
                        this->NodeSetDistributionFactorIndex,
                        this->SumDistFactPerNodeSet, (verbose>1));
    }

  this->ShowInts("SideSetSize", this->NumberOfSideSets, this->SideSetSize);
  this->ShowInts("SideSetNumberOfDistributionFactors", this->NumberOfSideSets, 
                  this->SideSetNumberOfDistributionFactors);

  if (verbose)
    {
    this->ShowListsOfInts("SideSetElementList", this->SideSetElementList, 
                         this->NumberOfSideSets, this->SideSetListIndex, 
                         this->SumSidesPerSideSet, (verbose>1));
    this->ShowListsOfInts("SideSetSideList", this->SideSetSideList, 
                         this->NumberOfSideSets, this->SideSetListIndex, 
                         this->SumSidesPerSideSet, (verbose>1));
    this->ShowListsOfInts("SideSetNumDFPerSide", this->SideSetNumDFPerSide, 
                         this->NumberOfSideSets, this->SideSetListIndex, 
                         this->SumSidesPerSideSet, (verbose>1));
    this->ShowListsOfFloats("SideSetDistributionFactors", 
                        this->SideSetDistributionFactors, 
                        this->NumberOfSideSets, 
                        this->SideSetDistributionFactorIndex,
                        this->SumDistFactPerSideSet, (verbose>1));
    } 

  this->ShowFloats("GlobalVariables", this->NumberOfGlobalVariables, this->GlobalVariableValue);

  cout << "NumberOfElementVariables " << this->NumberOfElementVariables << endl;
  this->ShowLines("ElementVariableNames", this->NumberOfElementVariables, this->ElementVariableNames);
  this->ShowInts("ElementVariableNumberOfComponents", this->NumberOfElementVariables, this->ElementVariableNumberOfComponents);
  this->ShowInts("MapToOriginalElementVariableNames", this->NumberOfElementVariables, this->MapToOriginalElementVariableNames);

  cout << "NumberOfNodeVariables " << this->NumberOfNodeVariables << endl;
  this->ShowLines("NodeVariableNames", this->NumberOfNodeVariables, this->NodeVariableNames);
  this->ShowInts("NodeVariableNumberOfComponents", this->NumberOfNodeVariables, this->NodeVariableNumberOfComponents);
  this->ShowInts("MapToOriginalNodeVariableNames", this->NumberOfNodeVariables, this->MapToOriginalNodeVariableNames);
}

void vtkModelMetadata::PrintGlobalInformation()
{
  int i,j;

  cout << "Metadata global information" << endl;
  cout << "========================================" << endl;

  if (this->Title) cout << "Title: "  << this->Title << endl;

  if (this->NumberOfQARecords)
    {
    cout << "QA Records:" << endl;

    char *name = 0;
    char *ver = 0;
    char *date = 0;
    char *time = 0;

    for (i=0; i<this->NumberOfQARecords; i++)
      {
      this->GetQARecord(i, &name, &ver, &date, &time);
      cout << "  " << name << " " << ver << " " << date << " " << time << endl; 
      }
    }

  this->ShowLines("InformationLines", 
         this->NumberOfInformationLines, this->InformationLine);

  this->ShowLines("CoordinateNames", this->Dimension, this->CoordinateNames);

  cout << "NumberOfTimeSteps " << this->NumberOfTimeSteps << endl;
  this->ShowFloats("TimeStepValues", this->NumberOfTimeSteps, this->TimeStepValues);

  cout << "NumberOfBlocks " << this->NumberOfBlocks << endl;
  this->ShowInts("BlockIds", this->NumberOfBlocks, this->BlockIds);
  this->ShowLines("BlockElementType", this->NumberOfBlocks, this->BlockElementType);
  this->ShowInts("BlockNodesPerElement", this->NumberOfBlocks, this->BlockNodesPerElement);
  this->ShowInts("BlockNumberOfAttributesPerElement", this->NumberOfBlocks, this->BlockNumberOfAttributesPerElement);

  cout << "NumberOfNodeSets " << this->NumberOfNodeSets << endl;
  this->ShowInts("NodeSetIds", this->NumberOfNodeSets, this->NodeSetIds);

  cout << "NumberOfSideSets " << this->NumberOfSideSets << endl;
  this->ShowInts("SideSetIds", this->NumberOfSideSets, this->SideSetIds);

  cout << "NumberOfBlockProperties " << this->NumberOfBlockProperties << endl;
  this->ShowLines("BlockPropertyNames", this->NumberOfBlockProperties, this->BlockPropertyNames);
  this->ShowIntArray("BlockPropertyValue", this->NumberOfBlocks, this->NumberOfBlockProperties, this->BlockPropertyValue);

  cout << "NumberOfNodeSetProperties " << this->NumberOfNodeSetProperties << endl;
  this->ShowLines("NodeSetPropertyNames", this->NumberOfNodeSetProperties, this->NodeSetPropertyNames);
  this->ShowIntArray("NodeSetPropertyValue", this->NumberOfNodeSets, this->NumberOfNodeSetProperties, this->NodeSetPropertyValue);

  cout << "NumberOfSideSetProperties " << this->NumberOfSideSetProperties << endl;
  this->ShowLines("SideSetPropertyNames", this->NumberOfSideSetProperties, this->SideSetPropertyNames);
  this->ShowIntArray("SideSetPropertyValue", this->NumberOfSideSets, this->NumberOfSideSetProperties, this->SideSetPropertyValue);

  cout << "NumberOfGlobalVariables " << this->NumberOfGlobalVariables << endl;
  this->ShowLines("GlobalVariableNames", this->NumberOfGlobalVariables, this->GlobalVariableNames);

  cout << "OriginalNumberOfElementVariables " << this->OriginalNumberOfElementVariables << endl;
  this->ShowLines("OriginalElementVariableNames", this->OriginalNumberOfElementVariables, this->OriginalElementVariableNames);

  cout << "OriginalNumberOfNodeVariables " << this->OriginalNumberOfNodeVariables << endl;
  this->ShowLines("OriginalNodeVariableNames", this->OriginalNumberOfNodeVariables, this->OriginalNodeVariableNames);

  int *tt = this->ElementVariableTruthTable;
  int nblocks = this->NumberOfBlocks;
  int nelts = this->OriginalNumberOfElementVariables;
  int ttsize = nblocks * nelts;

  if (tt && ttsize)
    {
    cout << "Block/Element variable truth table" << endl;
    for (i=0; i<nblocks; i++)
      {
      cout <<  "block " << i << ", ";
      for (j=0; j<nelts; j++)
        {
        cout << *tt++ << " ";
        }
      cout << endl;
      }
    }

  cout << "========================================" << endl;
}

int vtkModelMetadata::CalculateMaximumLengths(int &maxString, int &maxLine) 
{
  int i;
  maxLine = 0;
  maxString = 0;

  // Exodus file strings have a bounded length.  The bounds
  // MAX_STR_LENGTH and MAX_LINE_LENGTH are in the exodusII
  // header file.  However the vtkModelMetadata class must
  // not require the Exodus library.  It is used by classes
  // that are ExodusModel-aware, but must work in non Exodus
  // environments.  (Like vtkEnSightWriter).  It also may be
  // used by other dataset file formats in the future.  So we
  // need to deduce a fixed string length and line length.

  int sizeLine = (this->Title ? static_cast<int>(strlen(this->Title)) : 0);
  maxLine = ((sizeLine > maxLine) ? sizeLine : maxLine);

  for (i=0; i<this->NumberOfInformationLines; i++)
    {
    sizeLine = (this->InformationLine[i] ? 
                static_cast<int>(strlen(this->InformationLine[i])) : 0);
    maxLine = ((sizeLine > maxLine) ? sizeLine : maxLine);
    }

  for (i=0; i<this->NumberOfQARecords; i++)
    {
    sizeLine = (this->QARecord[i][0] ? 
                static_cast<int>(strlen(this->QARecord[i][0])) : 0);
    maxString = (sizeLine > maxString) ? sizeLine : maxString;

    sizeLine = (this->QARecord[i][1] ? 
                static_cast<int>(strlen(this->QARecord[i][1])) : 0);
    maxString = (sizeLine > maxString) ? sizeLine : maxString;

    sizeLine = (this->QARecord[i][2] ? 
                static_cast<int>(strlen(this->QARecord[i][2])) : 0);
    maxString = (sizeLine > maxString) ? sizeLine : maxString;

    sizeLine = (this->QARecord[i][3] ? 
                static_cast<int>(strlen(this->QARecord[i][3])) : 0);
    maxString = (sizeLine > maxString) ? sizeLine : maxString;
    }

  for (i=0; i<this->Dimension; i++)
    {
    sizeLine = (this->CoordinateNames[i] ? 
                static_cast<int>(strlen(this->CoordinateNames[i])) : 0);
    maxString = (sizeLine > maxString) ? sizeLine : maxString;
    }

  for (i=0; i<this->NumberOfBlocks; i++)
    {
    sizeLine = (this->BlockElementType[i] ? 
                static_cast<int>(strlen(this->BlockElementType[i])) : 0);
    maxString = (sizeLine > maxString) ? sizeLine : maxString;
    }

  for (i=0; i<this->NumberOfBlockProperties; i++)
    {
    sizeLine = (this->BlockPropertyNames[i] ? 
                static_cast<int>(strlen(this->BlockPropertyNames[i])) : 0);
    maxString = (sizeLine > maxString) ? sizeLine : maxString;
    }

  for (i=0; i<this->NumberOfNodeSetProperties; i++)
    {
    sizeLine = (this->NodeSetPropertyNames[i] ? 
                static_cast<int>(strlen(this->NodeSetPropertyNames[i])) : 0);
    maxString = (sizeLine > maxString) ? sizeLine : maxString;
    }

  for (i=0; i<this->NumberOfSideSetProperties; i++)
    {
    sizeLine = (this->SideSetPropertyNames[i] ? 
                static_cast<int>(strlen(this->SideSetPropertyNames[i])) : 0);
    maxString = (sizeLine > maxString) ? sizeLine : maxString;
    }

  for (i=0; i<this->NumberOfGlobalVariables; i++)
    {
    sizeLine = (this->GlobalVariableNames[i] ? 
                static_cast<int>(strlen(this->GlobalVariableNames[i])) : 0);
    maxString = (sizeLine > maxString) ? sizeLine : maxString;
    }

  // Figure the node and element variable name lengths into the calculations.
  // Note: sizeLine++ is necessary (for the null-terminating char?)

  for (i=0; i<this->NumberOfNodeVariables; i++)
    {
    sizeLine = (this->NodeVariableNames[i] ? 
                static_cast<int>(strlen(this->NodeVariableNames[i])) : 0);
    sizeLine++;
    maxString = (sizeLine > maxString) ? sizeLine : maxString;
    }

  for (i=0; i<this->NumberOfElementVariables; i++)
    {
    sizeLine = (this->ElementVariableNames[i] ? 
                static_cast<int>(strlen(this->ElementVariableNames[i])) : 0);
    sizeLine++;
    maxString = (sizeLine > maxString) ? sizeLine : maxString;
    }

  return 0;
}

void vtkModelMetadata::PrintSelf(ostream& os, vtkIndent indent)
{
  int i;
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Title: " << (this->Title?this->Title:"(none)") << endl;
  os << indent << "NumberOfQARecords: " <<
                   this->NumberOfQARecords << endl;
  os << indent << "NumberOfInformationLines: " <<
                   this->NumberOfInformationLines << endl;
  os << indent << "Dimension: " <<
                   this->Dimension << endl;
  os << indent << "CoordinateNames: " << endl;
  for(i=0;i<this->Dimension;i++)
    {
    os << indent << "-" << (this->CoordinateNames[i]?this->CoordinateNames[i]:"(none)") << endl;
    }
  os << indent << "NumberOfBlocks: " <<
                   this->NumberOfBlocks << endl;
  os << indent << "NumberOfNodeSets: " <<
                   this->NumberOfNodeSets << endl;
  os << indent << "NodeSetIds: ";
  for(i=0;i<this->NumberOfNodeSets;i++)
    {
    os << this->NodeSetIds[i] << " ";
    }
  os << endl;
  if (this->NodeSetSize)
    {
    os << indent << "NodeSetSize: ";
    for(i=0;i<this->NumberOfNodeSets;i++)
      {
      os << this->NodeSetSize[i] << " ";
      }
    os << endl;
    }
  os << indent << "NodeSetNodeIdList: ";
  for(i=0;i<this->SumNodesPerNodeSet;i++)
    {
    os << this->NodeSetNodeIdList[i] << " ";
    }
  os << endl;
//  os << indent << "NodeSetNumberOfDistributionFactors: " <<
//                   (this->NodeSetNumberOfDistributionFactors?this->NodeSetNumberOfDistributionFactors:"(none)") << endl;
  os << indent << "NodeSetDistributionFactors: ";
  for(i=0;i<this->SumDistFactPerNodeSet;i++)
    {
    os << this->NodeSetDistributionFactors[i] << " ";
    }
  os << endl;
  os << indent << "NumberOfSideSets: " <<
                   this->NumberOfSideSets << endl;
  os << indent << "SideSetIds: ";
  for(i=0;i<this->NumberOfSideSets;i++)
    {
    os << this->SideSetIds[i] << " ";
    }
  os << endl;
  if (this->SideSetSize)
    {
    os << indent << "SideSetSize: ";
    for(i=0;i<this->NumberOfSideSets;i++)
      {
      os << this->SideSetSize[i] << " ";
      }
    os << endl;
    }
//  os << indent << "SideSetNumberOfDistributionFactors: " <<
//                  (this->SideSetNumberOfDistributionFactors?this->SideSetNumberOfDistributionFactors:"(none)" << endl;
  os << indent << "SideSetElementList: ";
  for(i=0;i<this->SumSidesPerSideSet;i++)
    {
    os << this->SideSetElementList[i] << " ";
    }
  os << endl;
  os << indent << "SideSetSideList: ";
  for(i=0;i<this->SumSidesPerSideSet;i++)
    {
    os << this->SideSetSideList[i] << " ";
    }
  os << endl;
  os << indent << "SideSetNumDFPerSide: ";
  for(i=0;i<this->SumSidesPerSideSet;i++)
    {
    os << this->SideSetNumDFPerSide[i] << " ";
    }
  os << endl;
  os << indent << "SideSetDistributionFactors: ";
  for(i=0;i<this->SumDistFactPerSideSet;i++)
    {
    os << this->SideSetDistributionFactors[i] << " ";
    }
  os << endl;
  os << indent << "NumberOfBlockProperties: " <<
                   this->NumberOfBlockProperties << endl;
  os << indent << "BlockPropertyNames: ";
  for(i=0;i<this->NumberOfBlockProperties;i++)
    {
    os << indent << "-" << (this->BlockPropertyNames[i]?this->BlockPropertyNames[i]:"(none)") << endl;
    }
//  os << indent << "BlockPropertyValue: " <<
//                   (this->BlockPropertyValue?this->BlockPropertyValue:"(none)") << endl;
  os << indent << "NumberOfNodeSetProperties: " <<
                   this->NumberOfNodeSetProperties << endl;
  os << indent << "NodeSetPropertyNames: ";
  for(i=0;i<this->NumberOfNodeSetProperties;i++)
    {
    os << indent << "-" << (this->NodeSetPropertyNames[i]?this->NodeSetPropertyNames[i]:"(none)") << endl;
    }
//  os << indent << "NodeSetPropertyValue: " <<
//                  (this->NodeSetPropertyValue?this->NodeSetPropertyValue:"(none)") << endl;
  os << indent << "NumberOfSideSetProperties: " <<
                   this->NumberOfSideSetProperties << endl;
  os << indent << "SideSetPropertyNames: ";
  for(i=0;i<this->NumberOfSideSetProperties;i++)
    {
    os << indent << "-" << (this->SideSetPropertyNames[i]?this->SideSetPropertyNames[i]:"(none)") << endl;
    }
//  os << indent << "SideSetPropertyValue: " <<
//                  (this->SideSetPropertyValue?this->SideSetPropertyValue:"(none)") << endl;
  os << indent << "NumberOfElementVariables: " <<
                   this->NumberOfElementVariables << endl;
  os << indent << "ElementVariableNames: ";
  for(i=0;i<this->MaxNumberOfElementVariables;i++)
    {
    os << indent << "-" << (this->ElementVariableNames[i]?this->ElementVariableNames[i]:"(none)") << endl;
    }
  os << indent << "NumberOfNodeVariables: " <<
                   this->NumberOfNodeVariables << endl;
  os << indent << "NodeVariableNames: ";
  for(i=0;i<this->NumberOfNodeVariables;i++)
    {
    os << indent << "-" << (this->NodeVariableNames[i]?this->NodeVariableNames[i]:"(none)") << endl;
    }
//  os << indent << "ElementVariableTruthTable: " <<
//                  (this->ElementVariableTruthTable?this->ElementVariableTruthTable:"(none)") << endl;
  os << indent << "TimeStepIndex: " <<
                   this->TimeStepIndex << endl;
  os << indent << "AllVariablesDefinedInAllBlocks: " <<
                   this->AllVariablesDefinedInAllBlocks << endl;
}
