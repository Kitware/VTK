
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
#include "vtkStringArray.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkIdList.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include <ctime>

vtkStandardNewMacro(vtkModelMetadata);

#include <set>
#include <map>
#include <algorithm>

class vtkModelMetadataSTLCloak
{
public:
  std::set<int> IntSet;
  std::map<int, int> IntMap;
};


#undef FREE
#undef FREELIST

#define FREE(x) \
    {           \
    delete [] x;\
    x = NULL;   \
    }

#define FREELIST(x, len)       \
  if (x && (len))              \
  {                          \
    for (i=0; i<(len); i++)    \
    {                        \
      delete [] x[i];          \
    }                        \
    delete [] x;               \
    x = NULL;                  \
  }

void vtkModelMetadata::InitializeAllMetadata()
{
  this->Title = NULL;

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
  this->SetInformationLines(0, NULL);

  this->SetCoordinateNames(0, NULL);
  this->SetTimeSteps(0, NULL);

  this->SetBlockIds(NULL);
  this->SetBlockElementType(NULL);
  this->SetBlockNodesPerElement(NULL);
  this->SetBlockNumberOfAttributesPerElement(NULL);

  delete this->BlockIdIndex;
  this->BlockIdIndex = NULL;

  this->SetNodeSetNames(NULL);
  this->SetNodeSetIds(NULL);
  this->SetSideSetNames(NULL);
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
  FREE(this->BlockAttributesIndex);
  FREE(this->BlockElementIdListIndex);
  FREE(this->NodeSetDistributionFactorIndex);
  FREE(this->NodeSetIds);
  FREE(this->NodeSetNodeIdListIndex);
  FREE(this->NodeSetNumberOfDistributionFactors);
  FREE(this->NodeSetSize);
  FREE(this->SideSetDistributionFactorIndex);
  FREE(this->SideSetListIndex);
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

int vtkModelMetadata::GetInformationLines(char ***lines) const
{
  *lines = this->InformationLine;

  return this->NumberOfInformationLines;
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

  delete [] this->BlockAttributesIndex;
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

  delete [] this->BlockElementIdListIndex;
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
void vtkModelMetadata::SetNodeSetSize(int *n)
{
  FREE(this->NodeSetSize);

  this->NodeSetSize = n;
}
void vtkModelMetadata::SetNodeSetNodeIdList(int *n)
{
  FREE(this->NodeSetNodeIdList);

  this->NodeSetNodeIdList = n;
}
void vtkModelMetadata::SetNodeSetNumberOfDistributionFactors(int *n)
{
  FREE(this->NodeSetNumberOfDistributionFactors);

  this->NodeSetNumberOfDistributionFactors = n;
}
void vtkModelMetadata::SetNodeSetDistributionFactors(float *d)
{
  FREE(this->NodeSetDistributionFactors);

  this->NodeSetDistributionFactors = d;
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

int vtkModelMetadata::SetSideSetSize(int *size)
{
  FREE(this->SideSetSize);

  if (size)
  {
    this->SideSetSize = size;
  }

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

  delete [] this->SideSetDistributionFactorIndex;
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

void vtkModelMetadata::SetNodeVariableInfo(int numOrigNames, char **origNames,
            int numNames, char **names, int *numComp, int *map)
{
  this->SetOriginalNodeVariableNames(numOrigNames, origNames);
  this->SetNodeVariableNames(numNames, names);
  this->SetNodeVariableNumberOfComponents(numComp);
  this->SetMapToOriginalNodeVariableNames(map);
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
  os << indent << "NodeSetNames: ";
  for(i=0; this->NodeSetNames && (i<this->NodeSetNames->GetNumberOfValues()); i++)
  {
    os << this->NodeSetNames->GetValue (i) << " ";
  }
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
  os << indent << "SideSetNames: ";
  for(i=0;this->SideSetNames && (i<this->SideSetNames->GetNumberOfValues()); i++)
  {
    os << this->SideSetNames->GetValue (i) << " ";
  }
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
