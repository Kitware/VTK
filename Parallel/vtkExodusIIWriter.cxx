/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExodusIIWriter.cxx

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

#include "vtkToolkits.h"
#include "vtkObjectFactory.h"
#include "vtkExodusIIWriter.h"
#include "vtkThreshold.h"
#include "vtkModelMetadata.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkShortArray.h"
#include "vtkIdTypeArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkCellArray.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#ifdef VTK_USE_PARALLEL
#include "vtkMultiProcessController.h"
#endif
#include "vtkstd/map"

#include <netcdf.h>
#include <exodusII.h>
#include <time.h>

// TODO - check for errors when there are no element or node variables

// TODO - maybe check for ghost cells and omit them, or include them if
//   the user says to do so

// If we are building against a slightly older VTK version,
// these cell types are not defined, and won't occur in the input

#ifndef VTK_QUADRATIC_WEDGE
 #define VTK_PENTAGONAL_PRISM 15
 #define VTK_HEXAGONAL_PRISM  16
 #define VTK_QUADRATIC_WEDGE      26
 #define VTK_QUADRATIC_PYRAMID    27
#endif

#define FREE(x) if (x) {delete [] x; x = NULL;}
#define FREELIST(x, len) \
{                \
  if (x)         \
    {            \
    for (i=0; i<len; i++) \
      {          \
      if (x[i]) delete [] x[i]; \
      }          \
    delete [] x; \
    }            \
  x = NULL;      \
}
#define FREEOBJECTLIST(x, len) \
{                \
  if (x)         \
    {            \
    for (i=0; i<len; i++) \
      {          \
      if (x[i]) x[i]->Delete(); \
      }          \
    delete [] x; \
    }            \
  x = NULL;      \
}

vtkCxxRevisionMacro(vtkExodusIIWriter, "1.6");
vtkStandardNewMacro(vtkExodusIIWriter);
vtkCxxSetObjectMacro(vtkExodusIIWriter, ModelMetadata, vtkModelMetadata);

//----------------------------------------------------------------------------

vtkExodusIIWriter::vtkExodusIIWriter()
{
  this->FileName = NULL;
  this->MyFileName = NULL;
  this->ModelMetadata = NULL;

  this->PassDoubles = 0;
  this->StoreDoubles = -1;   // flag that this is not set
  this->fid = -1;

  this->InputBlockIdsLength = 0;
  this->InputBlockIds = NULL;

  this->InputNumberOfTimeSteps = 0;
  this->InputCurrentTimeStep = -1;
  this->InputTimeStepValues = NULL;
  this->LastTimeStepWritten = -1;

  this->BlockIdArrayName = NULL;
  this->GlobalElementIdArrayName = NULL;
  this->GlobalNodeIdArrayName = NULL;

  this->BlockIdList = NULL;
  this->GlobalElementIdList = NULL;
  this->GlobalNodeIdList = NULL;

  this->LocalBlockIndexMap = NULL;
  this->LocalElementIdMap = NULL;
  this->LocalNodeIdMap = NULL;

  this->WriteOutBlockIdArray = 0;
  this->WriteOutGlobalElementIdArray = 0;
  this->WriteOutGlobalNodeIdArray = 0;

  this->InitializeBlockLists();
  this->InitializeVariableArrayNames();

  this->BlockElementVariableTruthTable = NULL;
  this->AllVariablesDefinedInAllBlocks = 0;

  this->NumberOfProcesses = 1;
  this->MyRank = 0;
  this->MyInput = NULL;

  this->GhostLevel = 0;
  this->ErrorStatus = 0;

  // ATTRIBUTE EDITOR
  this->EditedVariableName = NULL;
  this->EditorFlag = 0;
}
vtkExodusIIWriter::~vtkExodusIIWriter()
{
  if (!this->BlockIdArrayName && this->BlockIdList)
    {
    FREE(this->BlockIdList);  // I created this
    }

  this->SetFileName(NULL);
  this->SetMyFileName(NULL);
  this->SetModelMetadata(NULL);
  this->SetBlockIdArrayName(NULL);
  this->SetGlobalElementIdArrayName(NULL);
  this->SetGlobalNodeIdArrayName(NULL);

  this->SetTimeStepValues(0, NULL);

  this->SetAllBlockIds(0, NULL);

  if (this->LocalElementIdMap) delete this->LocalElementIdMap;
  if (this->LocalNodeIdMap) delete this->LocalNodeIdMap;

  this->ClearBlockLists();
  this->ClearVariableArrayNames();

  FREE(this->BlockElementVariableTruthTable);

  if (this->MyInput)
    {
    this->MyInput->UnRegister(this);
    this->MyInput->Delete();
    }

  // ATTRIBUTE EDITOR
  if(this->EditedVariableName)
    {
    delete [] this->EditedVariableName;
    }
}

int vtkExodusIIWriter::FillInputPortInformation( int vtkNotUsed(port), vtkInformation* info )
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
// Input vtkUnstructuredGrid
//----------------------------------------------------------------------------
vtkUnstructuredGrid *vtkExodusIIWriter::GetInput()
{
  vtkUnstructuredGrid *ug = NULL;

  if (this->MyInput)
    {
    ug = this->MyInput;
    }
  else
    {
    ug = vtkUnstructuredGrid::SafeDownCast(this->Superclass::GetInput());
    }

  return ug;
}
void vtkExodusIIWriter::SetInput(vtkUnstructuredGrid *ug)
{
  this->Superclass::SetInput(ug);
}
void vtkExodusIIWriter::SetPassDoubles()
{
  int i;
  vtkUnstructuredGrid *ug = this->GetInput();
  if (!ug) return;

  // Determine whether we should pass single or double precision
  // floats to the Exodus Library.  We'll look through the arrays 
  // and points in the input and pick the precision of the
  // first float we see.

  int da = -1;

  vtkCellData *cd = ug->GetCellData();
  if (cd)
    {
    int numCellArrays = cd->GetNumberOfArrays();
    for (i=0; i<numCellArrays; i++)
      {
      vtkDataArray *a = cd->GetArray(i);
      int type = a->GetDataType();
  
      if (type == VTK_DOUBLE)
        {
        da = 1;
        break;
        }
      else if (type == VTK_FLOAT)
        {
        da = 0;
        break;
        }
      }
    }
  
  if (da < 0)
    {
    vtkPointData *pd = ug->GetPointData();
    if (pd)
      {
      int numPtArrays = pd->GetNumberOfArrays();
      for (i=0; i<numPtArrays; i++)
        {
        vtkDataArray *a = pd->GetArray(i);
        int type = a->GetDataType();
    
        if (type == VTK_DOUBLE)
          {
          da = 1;
          break;
          }
        else if (type == VTK_FLOAT)
          {
          da = 0;
          break;
          }
        }
      }
    }

  if (da < 0)
    {
    vtkPoints *pts = ug->GetPoints();

    if (pts)
      {
      int type = pts->GetDataType();
      if (type == VTK_DOUBLE)
        {
        da = 1;
        }
      else if (type == VTK_FLOAT)
        {
        da = 0;
        }
      }
    }

  this->PassDoubles = 0;
 
  if (da == 1)
    {
    this->PassDoubles = 1;
    }

  if (this->StoreDoubles < 0)
    {
    // The default is to store in the 
    // same precision that appears in the input.

    this->StoreDoubles = this->PassDoubles;   
    }
}
void vtkExodusIIWriter::RemoveGhostCells()
{
  if (this->MyInput)
    {
    this->MyInput->UnRegister(this);
    this->MyInput->Delete();
    this->MyInput = NULL;
    }

  vtkUnstructuredGrid *ug = vtkUnstructuredGrid::New();
  ug->ShallowCopy(this->GetInput());

  vtkDataArray *da = ug->GetCellData()->GetArray("vtkGhostLevels");

  if (da)
    {
    vtkThreshold *t = vtkThreshold::New();
    t->SetInput(ug);
    t->ThresholdByLower(0);
    t->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "vtkGhostLevels");

    t->Update();

    this->MyInput = t->GetOutput();
    this->MyInput->Register(this);
    t->Delete();

    this->MyInput->GetCellData()->RemoveArray("vtkGhostLevels");
    this->MyInput->GetPointData()->RemoveArray("vtkGhostLevels");

    this->GhostLevel = 1;
    }
  else
    {
    this->GhostLevel = 0;
    }
    
  ug->Delete();
}
//----------------------------------------------------------------------------
// Values that can be set if there is no metadata
//----------------------------------------------------------------------------
void vtkExodusIIWriter::SetTimeStepValues(int n, float *f)
{
  if (this->InputTimeStepValues)  
    {
    delete [] this->InputTimeStepValues;
    this->InputTimeStepValues = NULL;
    }

  this->InputNumberOfTimeSteps = 0;

  if (n < 1) return;

  this->InputNumberOfTimeSteps = n;
  
  this->InputTimeStepValues = new float [n];

  if (f)
    {
    memcpy(this->InputTimeStepValues, f, n * sizeof(float));
    }
}
void vtkExodusIIWriter::SetCurrentTimeStep(int ts)
{
  this->InputCurrentTimeStep = ts;
}
void vtkExodusIIWriter::SetAllBlockIds(int numEntries, int *blockIds)
{
  if (this->InputBlockIds)
    {
    delete [] this->InputBlockIds;
    this->InputBlockIds = NULL;
    this->InputBlockIdsLength = 0;
    }

  if (numEntries <= 0) return;

  this->InputBlockIdsLength = numEntries;
  this->InputBlockIds = new int [numEntries];

  if (blockIds)
    {
    memcpy(this->InputBlockIds, blockIds, numEntries * sizeof(int));
    }
}
//------------------------------------------------------------------
//------------------------------------------------------------------

void vtkExodusIIWriter::WriteData()
{
  int rc = 0;
  if ( !this->FileName )
    {
    vtkErrorMacro("No FileName specified.");
    return;
    }

  this->SetPassDoubles();   // does input contain floats or doubles

  this->RemoveGhostCells();

  // Make sure we have all necessary information.  If there is no
  // vtkModelMetadata object, create one with reasonable defaults if
  // that is possible.

  rc = this->CheckParameters();

  if (rc)
    {
    return;
    }

// ATTRIBUTE EDITOR
  if(this->EditorFlag)
    {
    rc = this->OpenExodusFile();
    
    if(rc)
      {
      vtkErrorMacro(<< "vtkExodusIIWriter::WriteData can't write timestep");
      goto doneError;
      }
    else
      {
      goto writeData;
      }
    }


  if (this->LastTimeStepWritten >= 0)
    {
    this->OpenExodusFile();

    rc = this->WriteNextTimeStep();

    if (rc)
      {
      goto doneError;
      }

    goto done;
    }

  rc = this->CreateNewExodusFile();

  if (rc)
    {
    vtkErrorMacro(<< "vtkExodusIIWriter::WriteData can't create exodus file");
    goto doneError;
    }


writeData:

  rc = this->WriteInitializationParameters();

  if (rc)
    {
    vtkErrorMacro(<< "vtkExodusIIWriter::WriteData init params");
    goto doneError;
    }

  rc = this->WriteQARecords();

  if (rc)
    {
    vtkErrorMacro(<< "vtkExodusIIWriter::WriteData QA records");
    goto doneError;
    }

  rc = this->WriteInformationRecords();

  if (rc)
    {
    vtkErrorMacro(<< "vtkExodusIIWriter::WriteData information records");
    goto doneError;
    }

  rc = this->WritePoints();

  if (rc)
    {
    vtkErrorMacro(<< "vtkExodusIIWriter::WriteData points");
    goto doneError;
    }

  rc = this->WriteCoordinateNames();

  if (rc)
    {
    vtkErrorMacro(<< "vtkExodusIIWriter::WriteData coordinate names");
    goto doneError;
    }

  rc = this->WriteGlobalPointIds();

  if (rc)
    {
    vtkErrorMacro(<< "vtkExodusIIWriter::WriteData global point IDs");
    goto doneError;
    }

  rc = this->WriteBlockInformation();

  if (rc)
    {
    vtkErrorMacro(<< "vtkExodusIIWriter::WriteData block information");
    goto doneError;
    }

  rc = this->WriteGlobalElementIds();

  if (rc)
    {
    vtkErrorMacro(<< "vtkExodusIIWriter::WriteData global element IDs");
    goto doneError;
    }


  rc = this->WriteVariableArrayNames();

  if (rc)
    {
    vtkErrorMacro(<< "vtkExodusIIWriter::WriteData variable array names"); 
    goto doneError;
    }

  rc = this->WriteNodeSetInformation();

  if (rc)
    {
    vtkErrorMacro(<< "vtkExodusIIWriter::WriteData can't node sets");
    goto doneError;
    }

  rc = this->WriteSideSetInformation();

  if (rc)
    {
    vtkErrorMacro(<< "vtkExodusIIWriter::WriteData can't side sets");
    goto doneError;
    }

  rc = this->WriteProperties();

  if (rc)
    {
    vtkErrorMacro(<< "vtkExodusIIWriter::WriteData can't properties");
    goto doneError;
    }

  rc = this->WriteNextTimeStep();

  if (rc)
    {
    vtkErrorMacro(<< "vtkExodusIIWriter::WriteData results");
    goto doneError;
    }

  goto done;

doneError:

  this->SetErrorStatus(1);

done:
  this->SetModelMetadata(NULL);
  this->CloseExodusFile();
}

//------------------------------------------------------------------
// Check input parameters and set reasonable defaults
//------------------------------------------------------------------
int vtkExodusIIWriter::CheckParameters()
{
  vtkUnstructuredGrid *input = this->GetInput();

  // need input

  if (input == NULL)
    {
    vtkErrorMacro(<< "vtkExodusIIWriter, no input");
    return 1;
    }

  this->NumberOfProcesses = 1;
  this->MyRank = 0;

#ifdef VTK_USE_PARALLEL
  vtkMultiProcessController *c = vtkMultiProcessController::GetGlobalController();
  if (c)
    {
    this->NumberOfProcesses = c->GetNumberOfProcesses();
    this->MyRank = c->GetLocalProcessId();
    }

  if (this->GhostLevel > 0)
    {
    vtkWarningMacro(<< "ExodusIIWriter ignores ghost level request");
    }
#endif

  // What id arrays do we have

  int HaveGlobalElementIdArray = 0;
  int HaveGlobalNodeIdArray = 0;
  int HaveBlockIdArray = 0;

  vtkDataArray *da = NULL;
  vtkCellData *cd = input->GetCellData();

  if (this->BlockIdArrayName)
    {
    da = cd->GetArray(this->BlockIdArrayName);

    if (da)
      {
      HaveBlockIdArray = 1;
      }
    else
      {
      this->SetBlockIdArrayName(NULL);
      }
    }

  if (!HaveBlockIdArray)
    {
    da = cd->GetArray("BlockId");

    if (da)
      {
      this->SetBlockIdArrayName("BlockId");
      HaveBlockIdArray = 1;
      }
    }

  if (HaveBlockIdArray)
    {
    da = cd->GetArray(this->BlockIdArrayName);
    vtkIntArray *ia = vtkIntArray::SafeDownCast(da);
    if (!ia)
      {
      vtkErrorMacro(<< "vtkExodusIIWriter, block ID array is not an integer array");
      return 1;
      }
    this->BlockIdList = ia->GetPointer(0);
    }

  if (this->GlobalElementIdArrayName)
    {
    da = cd->GetArray(this->GlobalElementIdArrayName);

    if (da)
      {
      HaveGlobalElementIdArray = 1;
      }
    else
      {
      this->SetGlobalElementIdArrayName(NULL);
      }
    }

  if (!HaveGlobalElementIdArray)
    {
    da = cd->GetArray("GlobalElementId");

    if (da)
      {
      this->SetGlobalElementIdArrayName("GlobalElementId");
      HaveGlobalElementIdArray = 1;
      }
    }

  if (HaveGlobalElementIdArray)
    {
    da = cd->GetArray(this->GlobalElementIdArrayName);
    vtkIntArray *ia = vtkIntArray::SafeDownCast(da);
    if (!ia)
      {
      vtkWarningMacro(<< 
        "vtkExodusIIWriter, element ID array is not an integer array, ignoring it");
      this->GlobalElementIdList = NULL; 
      HaveGlobalElementIdArray = 0;
      }
    else
      {
      this->GlobalElementIdList = ia->GetPointer(0);
      }
    }

  vtkPointData *pd = input->GetPointData();

  if (this->GlobalNodeIdArrayName)
    {
    da = pd->GetArray(this->GlobalNodeIdArrayName);

    if (da)
      {
      HaveGlobalNodeIdArray = 1;
      }
    else
      {
      this->SetGlobalNodeIdArrayName(NULL);
      }
    }

  if (!HaveGlobalNodeIdArray)
    {
    da = pd->GetArray("GlobalNodeId");

    if (da)
      {
      this->SetGlobalNodeIdArrayName("GlobalNodeId");
      HaveGlobalNodeIdArray = 1;
      }
    }

  if (HaveGlobalNodeIdArray)
    {
    da = pd->GetArray(this->GlobalNodeIdArrayName);
    vtkIntArray *ia = vtkIntArray::SafeDownCast(da);
    if (!ia)
      {
      vtkWarningMacro(<< 
        "vtkExodusIIWriter, node ID array is not an integer array, ignoring it");
      this->GlobalNodeIdList = 0;
      HaveGlobalNodeIdArray = 0;
      }
    this->GlobalNodeIdList = ia->GetPointer(0);
    }

  if (this->GetModelMetadata())
    {
    // All of the information we'll need is in the ModelMetadata

    return 0;
    }

  int hasPackedMetadata = vtkModelMetadata::HasMetadata(input);

  if (hasPackedMetadata)
    {
    // All the metadata has been packed into field arrays of the ugrid,
    // probably by the vtkExodusReader or vtkPExodusReader.
 
    vtkModelMetadata *mmd = vtkModelMetadata::New();
    mmd->Unpack(input, 1);

    this->SetModelMetadata(mmd);
    mmd->Delete();

    return 0;
    }

  int rc = this->CreateExodusModel();  // use sensible defaults

  if (rc)
    {
    return 1;
    }

  return 0;
}

vtkModelMetadata* vtkExodusIIWriter::GetOrCreateModelMetadata()
{
  this->CheckParameters();
  return this->GetModelMetadata();
}

int vtkExodusIIWriter::CreateExodusModel()
{
  // There is no metadata associated with this input.  If we have enough
  // information, we create reasonable defaults.

  int i, rc;

  if ((this->NumberOfProcesses > 1) && 
      ((!this->InputBlockIds)  || (!this->BlockIdList)))
    {
    // Parallel apps must have a global list of all block IDs, plus a
    // list of block IDs for each cell.

    vtkErrorMacro(<< "Can't proceed without metadata.  Go back and request metadata from reader.");
    return 1; 
    }

  vtkModelMetadata *em = vtkModelMetadata::New();

  char *title = new char [MAX_LINE_LENGTH + 1];
  time_t currentTime = time(NULL);
  struct tm *td = localtime(&currentTime);
  char *stime = asctime(td);

  sprintf(title, "Created by vtkExodusIIWriter, %s", stime);

  em->SetTitle(title);

  char **dimNames = new char * [3];
  dimNames[0] = vtkExodusIIWriter::StrDupWithNew("X");
  dimNames[1] = vtkExodusIIWriter::StrDupWithNew("Y");
  dimNames[2] = vtkExodusIIWriter::StrDupWithNew("Z");
  em->SetCoordinateNames(3, dimNames);

  if (this->InputTimeStepValues)
    {
    em->SetTimeSteps(this->InputNumberOfTimeSteps, this->InputTimeStepValues);
    }

  if (this->InputBlockIds && this->BlockIdList)
    {
    rc = this->CreateBlockIdInformation(em);
    }
  else
    {
    rc = this->CreateBlockIdInformationFromCellTypes(em);  // single process only
    }

  if (rc)
    {
    return 1;
    }

  vtkUnstructuredGrid *ug = this->GetInput();

  vtkCellData *cd = ug->GetCellData();
  int narrays = cd->GetNumberOfArrays();
  int nflattened = 0;
  char **nms = NULL;
  char **flattenedNames = NULL;

  if (narrays > 0)
    {
    nms = new char * [narrays];
    int *numComponents = new int [narrays];
    int *mapToOriginal = new int [narrays];
    nflattened = 0;

    for (i=0; i<narrays; i++)
      {
      nms[i] = vtkExodusIIWriter::StrDupWithNew(cd->GetArray(i)->GetName());
      numComponents[i] = cd->GetArray(i)->GetNumberOfComponents();
      mapToOriginal[i] = nflattened;
      nflattened += numComponents[i];
      }

    flattenedNames = vtkExodusIIWriter::FlattenOutVariableNames(narrays,
      nflattened, nms, numComponents);

    em->SetElementVariableInfo(nflattened, flattenedNames,  
      narrays, nms, numComponents, mapToOriginal);

    int nblocks = em->GetNumberOfBlocks(); // computed in CreateBlockIdInfo*
    int *blockSize = em->GetBlockNumberOfElements();

    int *tt = new int [nblocks * nflattened];


    int index = 0;
    for (int blockNum=0; blockNum<nblocks; blockNum++) 
      {
      for (int j=0; j<nflattened; j++) 
        {
        if (blockSize[blockNum] > 0) 
          {
          tt[index++] = 1;  // truth table, all ON by default
          }
        else 
          {
          tt[index++] = 0;
          }
        }
      }
    em->SetElementVariableTruthTable(tt);
    }

  vtkPointData *pd = ug->GetPointData();
  narrays = pd->GetNumberOfArrays();
  nflattened = 0;
  nms = NULL;
  flattenedNames = NULL;

  if (narrays > 0)
    {
    nms = new char * [narrays];
    int *numComponents = new int [narrays];
    int *mapToOriginal = new int [narrays];
    nflattened = 0;

    for (i=0; i<narrays; i++)
      {
      nms[i] = vtkExodusIIWriter::StrDupWithNew(pd->GetArray(i)->GetName());
      numComponents[i] = pd->GetArray(i)->GetNumberOfComponents();
      mapToOriginal[i] = nflattened;
      nflattened += numComponents[i];
      }

    flattenedNames = vtkExodusIIWriter::FlattenOutVariableNames(narrays,
                            nflattened, nms, numComponents);

    em->SetNodeVariableInfo(nflattened, flattenedNames,  
          narrays, nms, numComponents, mapToOriginal);
    }

  this->SetModelMetadata(em);
  em->Delete();

  return 0;
}
char **vtkExodusIIWriter::FlattenOutVariableNames(
             int narrays, int nScalarArrays, char **nms, int *numComponents)
{
  int i;
  char **newNames = new char * [nScalarArrays];
  char **nextName = newNames;

  for (i=0; i<narrays; i++)
    {
    if (strlen(nms[i]) > MAX_STR_LENGTH-2) 
      {
      nms[i][MAX_STR_LENGTH-2] = '\0';
      }

    vtkExodusIIWriter::CreateNamesForScalarArrays(nms[i], nextName, numComponents[i]);

    nextName += numComponents[i];
    }

  return newNames;
}
void vtkExodusIIWriter::CreateNamesForScalarArrays(const char *root, char **nms, int numComponents)
{
  int next = 0;

  if (numComponents == 1)
    {
    nms[next++] = vtkExodusIIWriter::StrDupWithNew(root);
    }
  else if (numComponents <= 3)
    {
    char *n = new char [MAX_STR_LENGTH + 1];
    sprintf(n, "%s_X", root);
    nms[next++] = n;

    n = new char [MAX_STR_LENGTH + 1];
    sprintf(n, "%s_Y", root);
    nms[next++] = n;

    if (numComponents == 3)
      {
      n = new char [MAX_STR_LENGTH + 1];
      sprintf(n, "%s_Z", root);
      nms[next++] = n;
      }
    }
  else
    {
    for (int j=0; j<numComponents; j++)
      {
      char *n = new char [MAX_STR_LENGTH + 1];
      sprintf(n, "%s_%d", root, j);
      nms[next++] = n;
      }
    }
}
int vtkExodusIIWriter::FindCellType(int blockId, int *blockIdList, 
                                    unsigned char *cellTypeList, int nCells)
{
  int cellType = -1;

  for (int i=0; i<nCells; i++)
    {
    if (blockIdList[i] == blockId)
      {
      cellType = (int)(cellTypeList[i]);
      break;
      }
    }

  return cellType;
}

int vtkExodusIIWriter::CreateBlockIdInformation(vtkModelMetadata *em)
{
  vtkUnstructuredGrid *ug = this->GetInput();
  vtkUnsignedCharArray *cellTypes = ug->GetCellTypesArray();
  vtkIdType ncells = ug->GetNumberOfCells();

  int nblocks = this->InputBlockIdsLength;
  int i;

  if (nblocks < 1) return 1;

  em->SetNumberOfBlocks(nblocks);
  em->SetBlockIds(this->InputBlockIds);

  char **blockNames = new char * [nblocks];
  int *numElements = new int [nblocks];
  int *numNodesPerElement = new int [nblocks];
  int *numAttributes = new int [nblocks];

  vtkstd::map<int, int> idxMap;
  vtkstd::map<int, int>::iterator it;

  for (i=0; i<nblocks; i++)
    {
    int id = this->InputBlockIds[i];

    idxMap.insert(vtkstd::map<int,int>::value_type(id, i));

    int cellType = 
      vtkExodusIIWriter::FindCellType(id, this->BlockIdList, 
                                      cellTypes->GetPointer(0), ncells);

    blockNames[i] = vtkExodusIIWriter::GetCellTypeName(cellType);
    numElements[i] = 0;
    numNodesPerElement[i] = 0;
    numAttributes[i] = 0;
    }

  int *bids = this->BlockIdList;

  int err=0;

  for (i=0; i<ncells; i++)
    {
    int blockId = bids[i];

    it = idxMap.find(blockId);

    if (it == idxMap.end())
      {
      err=1;
      vtkErrorMacro(<< 
        "Block ID in array is not found on global block ID list");
      break;
      }

    int idx = it->second;

    int numNodes = ug->GetCell(i)->GetNumberOfPoints();      

    if (numElements[idx] == 0)
      {
      numNodesPerElement[idx] = numNodes;
      }
    else if (numNodes != numNodesPerElement[idx])
      {
      err=1; 
      vtkErrorMacro(<< 
        "Each cell in a block must have the same number of points");
      break;
      }

    numElements[idx]++;
    }

  if (err)
    {
    FREELIST(blockNames, nblocks);
    delete [] numElements;
    delete [] numNodesPerElement;
    delete [] numAttributes;
    }
  else
    {
    em->SetBlockElementType(blockNames);
    em->SetBlockNumberOfElements(numElements);
    em->SetBlockNodesPerElement(numNodesPerElement);
    em->SetBlockNumberOfAttributesPerElement(numAttributes);
    }

  return err;
}
int vtkExodusIIWriter::CreateBlockIdInformationFromCellTypes(vtkModelMetadata *em)
{
  int i;
  vtkUnstructuredGrid *ug = this->GetInput();
  int ncells = ug->GetNumberOfCells();
  vtkUnsignedCharArray *cellTypes = ug->GetCellTypesArray();
  unsigned char *ct = cellTypes->GetPointer(0);
  int nTypes = 0;

  vtkstd::map<int, int> idxMap;
  vtkstd::map<int, int>::iterator it;

  for (i=0; i<ncells; i++)
    {
    vtkstd::pair<vtkstd::map<int,int>::iterator, bool> inserted = 
      idxMap.insert(vtkstd::map<int,int>::value_type((int)ct[i], nTypes));

    if (inserted.second)
      {
      nTypes++;
      }
    }

  int *types = new int [nTypes];

  int min = 1;

  for (it = idxMap.begin(); it != idxMap.end(); ++it)
    {
    int typeNum = (int)it->first;
    int idx = it->second;

    types[idx] = typeNum;

    if (typeNum < min)
      {
      min = typeNum;
      }
    }

  // slight problem - block IDs must be 1 or greater

  int offset = 0;

  if (min < 1)
    {
    offset = (min * -1) + 1;
    }

  char **blockNames = new char * [nTypes];
  int *numElements = new int [nTypes];
  int *numNodesPerElement = new int [nTypes];
  int *numAttributes = new int [nTypes];

  for (i=0; i<nTypes; i++)
    {
    blockNames[i] = vtkExodusIIWriter::GetCellTypeName(types[i]);
    numElements[i] = 0;
    numNodesPerElement[i] = 0;
    numAttributes[i] = 0;

    types[i] += offset;
    }

  em->SetNumberOfBlocks(nTypes);
  em->SetBlockIds(types);

  this->BlockIdList = new int [ncells];

  int err = 0;

  for (i=0; i<ncells; i++)
    {
    int cellType = (int)ct[i];

    it = idxMap.find(cellType);

    int idx = it->second;

    int numNodes = ug->GetCell(i)->GetNumberOfPoints();      

    if (numElements[idx] == 0)
      {
      numNodesPerElement[idx] = numNodes;
      }
    else if (numNodes != numNodesPerElement[idx])
      {
      vtkErrorMacro(<<
        "Exodus writer fails when same cell types have different number of nodes");
      err = 1;
      break;
      }

    this->BlockIdList[i] = cellType + offset;
    numElements[idx]++;
    }

  if (err)
    {
    FREELIST(blockNames, nTypes);
    delete [] numElements;
    delete [] numNodesPerElement;
    delete [] numAttributes;
    }
  else
    {
    em->SetBlockElementType(blockNames);
    em->SetBlockNumberOfElements(numElements);
    em->SetBlockNodesPerElement(numNodesPerElement);
    em->SetBlockNumberOfAttributesPerElement(numAttributes);
    }

  return err;
}
char *vtkExodusIIWriter::GetCellTypeName(int t)
{
  if (MAX_STR_LENGTH < 32) return NULL;
  char *nm = new char [MAX_STR_LENGTH + 1];

  switch (t)
    {
    case VTK_EMPTY_CELL:
      strcpy(nm, "empty cell");
      break;
    case VTK_VERTEX:
      strcpy(nm, "vertex");
      break;
    case VTK_POLY_VERTEX:
      strcpy(nm, "polyvertex");
      break;
    case VTK_LINE:
      strcpy(nm, "line");
      break;
    case VTK_POLY_LINE:
      strcpy(nm, "polyline");
      break;
    case VTK_TRIANGLE:
      strcpy(nm, "TRIANGLE");
      break;
    case VTK_TRIANGLE_STRIP:
      strcpy(nm, "triangle strip");
      break;
    case VTK_POLYGON:
      strcpy(nm, "polygon");
      break;
    case VTK_PIXEL:
      strcpy(nm, "pixel");
      break;
    case VTK_QUAD:
      strcpy(nm, "quad");
      break;
    case VTK_TETRA:
      strcpy(nm, "TETRA");
      break;
    case VTK_VOXEL :
      strcpy(nm, "voxel");
      break;
    case VTK_HEXAHEDRON:
      strcpy(nm, "HEX");
      break;
    case VTK_WEDGE:
      strcpy(nm, "wedge");
      break;
    case VTK_PYRAMID:
      strcpy(nm, "pyramid");
      break;
    case VTK_PENTAGONAL_PRISM:
      strcpy(nm, "pentagonal prism");
      break;
    case VTK_HEXAGONAL_PRISM:
      strcpy(nm, "hexagonal prism");
      break;
    case VTK_QUADRATIC_EDGE:
      strcpy(nm, "quadratic edge");
      break;
    case VTK_QUADRATIC_TRIANGLE:
      strcpy(nm, "quadratic triangle");
      break;
    case VTK_QUADRATIC_QUAD:
      strcpy(nm, "quadratic quad");
      break;
    case VTK_QUADRATIC_TETRA:
      strcpy(nm, "quadratic tetra");
      break;
    case VTK_QUADRATIC_HEXAHEDRON:
      strcpy(nm, "quadratic hexahedron");
      break;
    case VTK_QUADRATIC_WEDGE:
      strcpy(nm, "quadratic wedge");
      break;
    case VTK_QUADRATIC_PYRAMID:
      strcpy(nm, "quadratic pyramid");
      break;
    case VTK_CONVEX_POINT_SET:
      strcpy(nm, "convex point set");
      break;
    case VTK_PARAMETRIC_CURVE:
      strcpy(nm, "parametric curve");
      break;
    case VTK_PARAMETRIC_SURFACE:
      strcpy(nm, "parametric surface");
      break;
    case VTK_PARAMETRIC_TRI_SURFACE:
      strcpy(nm, "parametric tri surface");
      break;
    case VTK_PARAMETRIC_QUAD_SURFACE:
      strcpy(nm, "parametric quad surface");
      break;
    case VTK_PARAMETRIC_TETRA_REGION:
      strcpy(nm, "parametric tetra region");
      break;
    case VTK_PARAMETRIC_HEX_REGION:
      strcpy(nm, "paramertric hex region");
      break;
    default:
      strcpy(nm, "unknown cell type");
      break;
    }

  return nm;
}
//------------------------------------------------------------------
// Open or create an Exodus II file
//------------------------------------------------------------------
void vtkExodusIIWriter::CloseExodusFile()
{
  if (this->fid >= 0)
    {
    ex_close(this->fid);
    this->fid = -1;
    return;
    }
}
int vtkExodusIIWriter::OpenExodusFile()
{
  this->CloseExodusFile();

  int compWordSize= (this->PassDoubles ? sizeof(double) : sizeof(float));
  int IOWordSize = (this->StoreDoubles ? sizeof(double) : sizeof(float));
  float version = 0.0;

  // ATTRIBUTE EDITOR
  if(this->EditorFlag && this->FileName)
    {
    this->SetMyFileName(this->GetFileName());
    }

  this->fid = ex_open(this->MyFileName, EX_WRITE, &compWordSize, &IOWordSize,
                      &version);

  int fail = (this->fid < 0);

  return fail;
}
int vtkExodusIIWriter::CreateNewExodusFile()
{
  if (this->NumberOfProcesses == 1)
    {
    if (this->FileName)
      {
      this->SetMyFileName(this->GetFileName());
      }
    else
      {
      this->SetMyFileName("./ExodusIIWriter.out.exo");
      }
    }
  else
    {
    char *nm = new char [1024];
    if (this->FileName == NULL)
      {
      sprintf(nm, "./ExodusIIWriter.exo.%04d.%04d", 
              this->NumberOfProcesses, this->MyRank);
      }
    else
      {
      sprintf(nm, "%s.%04d.%04d", this->FileName, 
                this->NumberOfProcesses, this->MyRank);
      }
    this->SetMyFileName(nm);
    delete [] nm;
    }

  int compWordSize= (this->PassDoubles ? sizeof(double) : sizeof(float));
  int IOWordSize = (this->StoreDoubles ? sizeof(double) : sizeof(float));

  this->fid = ex_create(this->MyFileName, EX_CLOBBER, &compWordSize, &IOWordSize);

  int fail = (this->fid < 0);

  return fail;
}
//========================================================================
//   MAPPINGS
//   Convert local to global IDs and vice versa
//========================================================================

int vtkExodusIIWriter::GetBlockLocalIndex(int id)
{
  if (!this->LocalBlockIndexMap)
    {
    this->LocalBlockIndexMap = new vtkstd::map<int, int>;

    for (int i=0; i<this->NumberOfElementBlocks; i++)
      {
      int gid = this->BlockIds[i];

      this->LocalBlockIndexMap->insert(vtkstd::map<int,int>::value_type(gid, i));
      }
    }

  vtkstd::map<int,int>::iterator mapit = this->LocalBlockIndexMap->find(id);

  if (mapit == this->LocalBlockIndexMap->end())
    {
    return -1;
    }
  else
    {
    return mapit->second;
    }
}
int vtkExodusIIWriter::GetElementLocalId(int id)
{
  if (!this->LocalElementIdMap)
    {
    vtkUnstructuredGrid *ug = this->GetInput();
    int ncells = ug->GetNumberOfCells();

    this->LocalElementIdMap = new vtkstd::map<int, int>;

    for (int i=0; i<ncells; i++)
      {
      int gid = this->GlobalElementIdList[i];

      this->LocalElementIdMap->insert(vtkstd::map<int,int>::value_type(gid, i));
      }
    }

  vtkstd::map<int,int>::iterator mapit = this->LocalElementIdMap->find(id);

  if (mapit == this->LocalElementIdMap->end())
    {
    return -1;
    }
  else
    {
    return mapit->second;
    }
}
int vtkExodusIIWriter::GetNodeLocalId(int id)
{
  if (!this->LocalNodeIdMap)
    {
    vtkUnstructuredGrid *ug = this->GetInput();
    int npoints = ug->GetNumberOfPoints();

    this->LocalNodeIdMap = new vtkstd::map<int, int>;

    for (int i=0; i<npoints; i++)
      {
      int gid = this->GlobalNodeIdList[i];

      this->LocalNodeIdMap->insert(vtkstd::map<int,int>::value_type(gid, i));
      }
    }

  vtkstd::map<int,int>::iterator mapit = this->LocalNodeIdMap->find(id);

  if (mapit == this->LocalNodeIdMap->end())
    {
    return -1;
    }
  else
    {
    return mapit->second;
    }
}
//========================================================================
//   VARIABLE ARRAYS:
//   CONVERT VECTOR ARRAYS TO APPROPRIATELY NAMED SCALAR ARRAYS
//========================================================================

void vtkExodusIIWriter::InitializeVariableArrayNames()
{
  this->InputElementArrayNames = NULL;
  this->OutputElementArrayNames = NULL;
  this->InputElementArrayComponent = NULL;
  this->NumberOfScalarElementArrays = 0;

  this->InputNodeArrayNames = NULL;
  this->OutputNodeArrayNames = NULL;
  this->InputNodeArrayComponent = NULL;
  this->NumberOfScalarNodeArrays = 0;
}
void vtkExodusIIWriter::ClearVariableArrayNames()
{
  int i;
  int n = this->NumberOfScalarElementArrays;

  FREELIST(this->InputElementArrayNames, n);
  FREELIST(this->OutputElementArrayNames, n);
  FREE(this->InputElementArrayComponent);
  this->NumberOfScalarElementArrays = 0;

  n = this->NumberOfScalarNodeArrays;

  FREELIST(this->InputNodeArrayNames, n);
  FREELIST(this->OutputNodeArrayNames, n);
  FREE(this->InputNodeArrayComponent);
  this->NumberOfScalarNodeArrays = 0;
}

int vtkExodusIIWriter::WriteVariableArrayNames()
{
  int i, j;
  int rc = 0;

  this->ClearVariableArrayNames();

  vtkUnstructuredGrid *ug = this->GetInput();
  vtkModelMetadata *mmd = this->GetModelMetadata();

  //  1. We convert vector arrays to individual scalar arrays, using 
  //     their original names if we have those.
  //  2. For the element variables, create the element/block truth table.

  int checkAndSkipEltIds = 0;
  int checkAndSkipNodeIds = 0;
  int checkAndSkipBlockIds = 0;

  if (this->GlobalElementIdArrayName && !this->WriteOutGlobalElementIdArray)
    {
    checkAndSkipEltIds = 1;
    }
  if (this->GlobalNodeIdArrayName && !this->WriteOutGlobalNodeIdArray)
    {
    checkAndSkipNodeIds = 1;
    }
  if (this->BlockIdArrayName && !this->WriteOutBlockIdArray)
    {
    checkAndSkipBlockIds = 1;
    }

  // CELL (ELEMENT) VARIABLES

  vtkCellData *cd = ug->GetCellData();
  int numCellArrays = cd->GetNumberOfArrays();
  int numCellScalars = 0;
  int *skipCellArray = NULL;

  if (numCellArrays > 0)
    {
    skipCellArray = new int [numCellArrays];
    memset(skipCellArray, 0, sizeof(int) * numCellArrays);

    for (i=0; i<numCellArrays; i++)
      {
      if (checkAndSkipEltIds)
        {
        const char *nm = cd->GetArray(i)->GetName();
        if (!strcmp(nm, this->GlobalElementIdArrayName))
          {
          skipCellArray[i] = 1;
          continue;
          }
        }
      if (checkAndSkipBlockIds)
        {
        const char *nm = cd->GetArray(i)->GetName();
        if (!strcmp(nm, this->BlockIdArrayName))
          {
          skipCellArray[i] = 1;
          continue;
          }
        }

      int numComponents = cd->GetArray(i)->GetNumberOfComponents();
      numCellScalars += numComponents;
      }

    if (numCellScalars > 0)
      {
      this->InputElementArrayNames = new char * [numCellScalars];
      this->OutputElementArrayNames = new char * [numCellScalars];
      this->InputElementArrayComponent = new int [numCellScalars];
    
      char **tempNames = new char * [numCellScalars];
    
      int nextScalar = 0;
    
      for (i=0; i<numCellArrays; i++)
        {
        if (skipCellArray[i]) continue;

        vtkDataArray *da = cd->GetArray(i);
    
        this->SetNewElementVariableNames(da, tempNames);
    
        int numComponents = da->GetNumberOfComponents();
        const char *arrayName = da->GetName();

        for (j=0; j<numComponents; j++)
          {
          this->InputElementArrayComponent[nextScalar] = j;
          this->InputElementArrayNames[nextScalar] = 
            vtkExodusIIWriter::StrDupWithNew(arrayName);
          this->OutputElementArrayNames[nextScalar] = tempNames[j];

          nextScalar++;
          }
        }
      delete [] tempNames;
      }
    delete [] skipCellArray;
    }

  this->NumberOfScalarElementArrays = numCellScalars;

  // BLOCK/ELEMENT TRUTH TABLE

  int nblocks = mmd->GetNumberOfBlocks();
  int nelementVars = this->NumberOfScalarElementArrays;
  int ttsize = nblocks * nelementVars;
  int allDefined = 1;
  int ttValue = 0;

  if (ttsize > 0)
    {
    this->BlockElementVariableTruthTable = new int [ttsize];
    int *tt = this->BlockElementVariableTruthTable;
  
    for (i=0; i<nblocks; i++)
      {
      for (j=0; j<nelementVars; j++)
        {
        if (this->NumberOfElementsPerBlock[i] == 0)
          {
          ttValue = 0;
          }
        else
          {
          ttValue = mmd->ElementVariableIsDefinedInBlock(
                  this->OutputElementArrayNames[j], this->BlockIds[i]);
          }
  
        *tt++ = ttValue;
  
        if (allDefined && (ttValue == 0))
          {
          allDefined = 0;
          }
        }
      }
    }

  this->AllVariablesDefinedInAllBlocks = allDefined;

  if (numCellScalars > 0 && this->EditorFlag == 0)
    {
    rc = ex_put_var_param(this->fid, "E", numCellScalars);

    if (rc == 0)
      {
      rc = ex_put_var_names(this->fid, "E", numCellScalars, 
                            this->OutputElementArrayNames);

      if (rc == 0)
        {
        rc = ex_put_elem_var_tab(this->fid, this->NumberOfElementBlocks,
                  numCellScalars, this->BlockElementVariableTruthTable);
        }
      }

    if (rc < 0)
      {
      vtkErrorMacro(<< 
        "vtkExodusIIWriter::WriteVariableArrayNames cell variables");
      return 1;
      }
    }

  // POINT (NODE) VARIABLES

  vtkPointData *pd = ug->GetPointData();
  int numPointArrays = pd->GetNumberOfArrays();
  int numPointScalars = 0;
  int *skipPointArray = NULL;

  if (numPointArrays > 0)
    {
    skipPointArray = new int [numPointArrays];
    memset(skipPointArray, 0, sizeof(int) * numPointArrays);

    for (i=0; i<numPointArrays; i++)
      {
      if (checkAndSkipNodeIds)
        {
        const char *nm = pd->GetArray(i)->GetName();
        if (!strcmp(nm, this->GlobalNodeIdArrayName))
          {
          skipPointArray[i] = 1;
          continue;
          }
        }
        
      int numComponents = pd->GetArray(i)->GetNumberOfComponents();
      numPointScalars += numComponents;
      }

    if (numPointScalars > 0)
      {
      this->InputNodeArrayNames = new char * [numPointScalars];
      this->OutputNodeArrayNames = new char * [numPointScalars];
      this->InputNodeArrayComponent = new int [numPointScalars];
    
      char **tempNames = new char * [numPointScalars];
    
      int nextScalar = 0;
    
      for (i=0; i<numPointArrays; i++)
        {
        if (skipPointArray[i]) continue;

        vtkDataArray *da = pd->GetArray(i);

        this->SetNewNodeVariableNames(da, tempNames);

        int numComponents = da->GetNumberOfComponents();
        const char *arrayName = da->GetName();

        for (j=0; j<numComponents; j++)
          {
          this->InputNodeArrayComponent[nextScalar] = j;
          this->InputNodeArrayNames[nextScalar] =
            vtkExodusIIWriter::StrDupWithNew(arrayName);
          this->OutputNodeArrayNames[nextScalar] = tempNames[j];

          nextScalar++;
          }
        }
  
      delete [] tempNames;
      }

    delete [] skipPointArray;
    }

  this->NumberOfScalarNodeArrays = numPointScalars;

  if (numPointScalars > 0 && this->EditorFlag == 0)
    {
    rc = ex_put_var_param(this->fid, "N", numPointScalars);

    if (rc == 0)
      {
      rc = ex_put_var_names(this->fid, "N", numPointScalars, 
                              this->OutputNodeArrayNames);
      }
      
    if (rc < 0)
      {
      vtkErrorMacro(<< 
        "vtkExodusIIWriter::WriteVariableArrayNames point variables");
      return 1;
      }
    }

  // GLOBAL VARIABLES
          
  int ngvars = mmd->GetNumberOfGlobalVariables();
    
  if (ngvars > 0 && this->EditorFlag == 0)
    {
    char **names = mmd->GetGlobalVariableNames();

    rc = ex_put_var_param(this->fid, "G", ngvars);

    if (rc == 0)
      {
      rc = ex_put_var_names(this->fid, "G", ngvars, names);
      }

    if (rc < 0)
      {
      vtkErrorMacro(<< 
        "vtkExodusIIWriter::WriteVariableArrayNames global variables");
      return 1;
      }
    }

  return 0;
}
void vtkExodusIIWriter::SetNewNodeVariableNames(vtkDataArray *da, char **nm)
{
  int i;
  int numComp = da->GetNumberOfComponents();

  vtkModelMetadata *em = this->GetModelMetadata();

  const char *arrayName = da->GetName();

  if (numComp == 1)
    {
    nm[0] = vtkExodusIIWriter::StrDupWithNew(arrayName);
    return;
    }

  char *orig = em->FindOriginalNodeVariableName(arrayName, 0);

  if (!orig)
    {
    // just make up sensible names for the component arrays
    vtkExodusIIWriter::CreateNamesForScalarArrays(arrayName, nm, numComp);
    return;
    }

  int err = 0;

  for (i=0; i<numComp; i++)
    {
    nm[i] = NULL;
    }

  if (orig)
    {
    nm[0] = vtkExodusIIWriter::StrDupWithNew(orig);

    for (int comp=1; comp < numComp; comp++)
      {
      orig = em->FindOriginalNodeVariableName(arrayName, comp);

      if (!orig )
        {
        err = 1;   // we'll just have to make up names
        break;
        }
      nm[comp] = vtkExodusIIWriter::StrDupWithNew(orig);
      }
    }

  if (err)
    {
    for (i=0; i<numComp; i++)
      {
      if (nm[i]) delete [] nm[i];
      nm[i] = NULL;
      }
    vtkExodusIIWriter::CreateNamesForScalarArrays(arrayName, nm, numComp);
    }

  return;
}
void vtkExodusIIWriter::SetNewElementVariableNames(vtkDataArray *da, char **nm)
{
  int i;
  int numComp = da->GetNumberOfComponents();

  vtkModelMetadata *em = this->GetModelMetadata();

  const char *arrayName = da->GetName();

  if (numComp == 1)
    {
    nm[0] = vtkExodusIIWriter::StrDupWithNew(arrayName);
    return;
    }

  char *orig = em->FindOriginalElementVariableName(arrayName, 0);

  if (!orig)
    {
    // just make up sensible names for the component arrays
    vtkExodusIIWriter::CreateNamesForScalarArrays(arrayName, nm, numComp);
    return;
    }

  int err = 0;

  for (i=0; i<numComp; i++)
    {
    nm[i] = NULL;
    }

  if (orig)
    {
    nm[0] = vtkExodusIIWriter::StrDupWithNew(orig);

    for (int comp=1; comp < numComp; comp++)
      {
      orig = em->FindOriginalElementVariableName(arrayName, comp);

      if (!orig )
        {
        err = 1;   // we'll just have to make up names
        break;
        }
      nm[comp] = vtkExodusIIWriter::StrDupWithNew(orig);
      }
    }

  if (err)
    {
    for (i=0; i<numComp; i++)
      {
      if (nm[i]) delete [] nm[i];
      nm[i] = NULL;
      }
    vtkExodusIIWriter::CreateNamesForScalarArrays(arrayName, nm, numComp);
    }

  return;
}
//========================================================================
//   VARIABLE ARRAYS:
//   Write out the results data for one time step.
//========================================================================

#define COPY_COMPONENT(arrayType, varType, castType)  \
{                                                     \
  arrayType *a = arrayType::SafeDownCast(da);         \
  varType *p = a->GetPointer(0);                      \
  if (idx == NULL)                                    \
    {                                                 \
    p += comp;                                        \
    for (i=0; i<nvals; i++)                           \
      {                                               \
      val[i] = (castType)(*p);                        \
      p += numComp;                                   \
      }                                               \
    }                                                 \
  else                                                \
    {                                                 \
    for (i=0; i<nvals; i++)                           \
      {                                               \
      int which = idx[i];                             \
      val[i] = (castType)p[(which*numComp) + comp];   \
      }                                               \
    }                                                 \
}

double *vtkExodusIIWriter::ExtractComponentD(vtkDataArray *da, int comp, int *idx)
{
  int i;
  int numComp = da->GetNumberOfComponents();
  if (numComp <= comp) return NULL;

  int nvals = da->GetNumberOfTuples();

  if (nvals < 1) return NULL;

  int type = da->GetDataType();

  if ((type == VTK_DOUBLE) && (numComp == 1) && (idx == NULL))
    {
    vtkDoubleArray *a = vtkDoubleArray::SafeDownCast(da);
    return a->GetPointer(0);
    }

  // converting to native type is much faster than doing GetTuple

  double *val = new double [nvals];

  switch (type)
    { 
    case VTK_DOUBLE:
      COPY_COMPONENT(vtkDoubleArray, double, double)
      break;

    case VTK_CHAR:
      COPY_COMPONENT(vtkCharArray, char, double)
      break;

    case VTK_UNSIGNED_CHAR:
      COPY_COMPONENT(vtkUnsignedCharArray, unsigned char, double)
      break;

    case VTK_SHORT:
      COPY_COMPONENT(vtkShortArray, short, double)
      break;

    case VTK_UNSIGNED_SHORT:
      COPY_COMPONENT(vtkUnsignedShortArray, unsigned short, double)
      break;

    case VTK_INT:
      COPY_COMPONENT(vtkIntArray, int, double)
      break;

    case VTK_UNSIGNED_INT:
      COPY_COMPONENT(vtkUnsignedIntArray, unsigned int, double)
      break;

    case VTK_LONG:
      COPY_COMPONENT(vtkLongArray, long, double)
      break;

    case VTK_UNSIGNED_LONG:
      COPY_COMPONENT(vtkUnsignedLongArray, unsigned long, double)
      break;

    case VTK_FLOAT:
      COPY_COMPONENT(vtkFloatArray, float, double)
      break;

    case VTK_ID_TYPE:
      COPY_COMPONENT(vtkIdTypeArray, vtkIdType, double)
      break;

    default:
      vtkErrorMacro(<< "vtkExodusIIWriter::ExtractComponentD bad type");
      break;
    }

  return val;
}
float *vtkExodusIIWriter::ExtractComponentF(vtkDataArray *da, int comp, int *idx)
{
  int i;
  int numComp = da->GetNumberOfComponents();
  if (numComp < comp) return NULL;

  int nvals = da->GetNumberOfTuples();

  if (nvals < 1) return NULL;

  float *val = new float [nvals];

  int type = da->GetDataType();

  if ((type == VTK_FLOAT) && (numComp == 1) && (idx == NULL))
    {
    vtkFloatArray *a = vtkFloatArray::SafeDownCast(da);
    return a->GetPointer(0);
    }

  // converting to native type is much faster than doing GetTuple

  switch (type)
    { 
    case VTK_FLOAT:
      COPY_COMPONENT(vtkFloatArray, float, float)
      break;

    case VTK_CHAR:
      COPY_COMPONENT(vtkCharArray, char, float)
      break;

    case VTK_UNSIGNED_CHAR:
      COPY_COMPONENT(vtkUnsignedCharArray, unsigned char, float)
      break;

    case VTK_SHORT:
      COPY_COMPONENT(vtkShortArray, short, float)
      break;

    case VTK_UNSIGNED_SHORT:
      COPY_COMPONENT(vtkUnsignedShortArray, unsigned short, float)
      break;

    case VTK_INT:
      COPY_COMPONENT(vtkIntArray, int, float)
      break;

    case VTK_UNSIGNED_INT:
      COPY_COMPONENT(vtkUnsignedIntArray, unsigned int, float)
      break;

    case VTK_LONG:
      COPY_COMPONENT(vtkLongArray, long, float)
      break;

    case VTK_UNSIGNED_LONG:
      COPY_COMPONENT(vtkUnsignedLongArray, unsigned long, float)
      break;

    case VTK_DOUBLE:
      COPY_COMPONENT(vtkDoubleArray, double, float)
      break;

    case VTK_ID_TYPE:
      COPY_COMPONENT(vtkIdTypeArray, vtkIdType, float)
      break;

    default:
      vtkErrorMacro(<< "vtkExodusIIWriter::ExtractComponentF bad type");
      break;
    }

  return val;
}


int vtkExodusIIWriter::ExtractComponentForEditorD(vtkDataArray *da, vtkDoubleArray *editedArray, vtkIntArray *idArray, int comp, int *idx)
{
  int numComp = da->GetNumberOfComponents();
  if (numComp <= comp) return 0;

  int nvals = da->GetNumberOfTuples();

  if (nvals < 1) return 0;

  int type = da->GetDataType();

  if ((type == VTK_DOUBLE) && (numComp == 1) && (idx == NULL))
    {
    vtkDoubleArray *a = vtkDoubleArray::SafeDownCast(da);
    editedArray->DeepCopy(a);

    if(idArray)
      {
      int j=0;
      float myVal;
      while( j < nvals )
        {
        myVal = a->GetValue(j);
        editedArray->SetValue(idArray->GetValue(j),myVal);
        j++;
        }
      }
    }
  else
    {
    return 0;
    }

  return 1;
}

int vtkExodusIIWriter::ExtractComponentForEditorF(vtkDataArray *da, vtkFloatArray *editedArray, vtkIntArray *idArray, int comp, int *idx)
{
  int numComp = da->GetNumberOfComponents();
  if (numComp < comp) return 0;

  int nvals = da->GetNumberOfTuples();

  if (nvals < 1) return 0;

  int type = da->GetDataType();

  if ((type == VTK_FLOAT) && (numComp == 1) && (idx == NULL))
    {
    vtkFloatArray *a = vtkFloatArray::SafeDownCast(da);
    editedArray->DeepCopy(a);

    if(idArray)
      {
      int j=0;
      float myVal;
      while( j < nvals )
        {
        myVal = a->GetValue(j);
        editedArray->SetValue(idArray->GetValue(j),myVal);
        j++;
        }
      }
    }
  else
    {
    return 0;
    }

  return 1;
}



int vtkExodusIIWriter::GetTimeStepIndex()
{
  int ts = -1;

  if (this->InputCurrentTimeStep >= 0)
    {
    // User told writer which time step to call this results
    // data when we write it out.

    ts = this->InputCurrentTimeStep;
    }
  else if (this->GetModelMetadata()->GetTimeStepValues())
    {
    // The time step index is in the metadata, the same index as
    // when the file was read in.

    ts = this->GetModelMetadata()->GetTimeStepIndex();
    }

  if (ts < 0)
    {
    // We don't have metadata and the user didn't specify anything
    // to the writer.  We just start at 1 and go up by 1 for each
    // write.

    ts = this->LastTimeStepWritten + 1;
    }

  this->LastTimeStepWritten = ts;

  return ts;
}

float vtkExodusIIWriter::GetTimeStepValue(int ts)
{
  float val = (float)ts;   // default

  vtkModelMetadata *mmd = this->GetModelMetadata();

  float *tsv = mmd->GetTimeStepValues();
  int numts = mmd->GetNumberOfTimeSteps();
  int realTimeStep = mmd->GetTimeStepIndex();

  if (numts > 0)
    {
    if (realTimeStep >= 0)
      {
      // It doesn't matter what time step we are saying it is
      // in the output file, this is the actual time stamp
      // associated with this results data.

      val = tsv[realTimeStep];
      }
    else if ((ts >= 0) && (ts < numts))
      {
      // We didn't have vtkModelMetadata, but the user of this writer
      // input a list of time values. Here's the value for this time 
      // step index.

      val = tsv[ts];
      }
    else if (ts >= numts)
      {
      // We didn't have vtkModelMetadata, but the user of this writer
      // input a list of 1 or more time values.  Here's the value for 
      // this time step index if we extrapolate from the last value.

      int extra = ts - numts + 1;
      float endOfTime = tsv[numts - 1];
      float diff = endOfTime;
      if (numts >= 2)
        {
        diff -= tsv[numts - 2];
        }

      val = endOfTime + (extra * diff);
      }
    }

  return val;
}

int vtkExodusIIWriter::WriteNextTimeStep()
{
  int i, idIdx;
  int rc = 0;

  vtkModelMetadata *mmd = this->GetModelMetadata();

  int ts = this->GetTimeStepIndex();
  float tsv = this->GetTimeStepValue(ts);

  if (this->PassDoubles)
    {
    double dtsv = (double)tsv;
    rc = ex_put_time(this->fid, ts + 1, &dtsv);
    }
  else
    {
    rc = ex_put_time(this->fid, ts + 1, &tsv);
    }

  if (rc < 0)
    {
    vtkErrorMacro(<< "vtkExodusIIWriter::WriteNextTimeStep time step values");
    return 1;
    }

  vtkUnstructuredGrid *ug = this->GetInput();
  int nblocks = this->NumberOfElementBlocks;
  int npoints = ug->GetNumberOfPoints();
  int nCellArrays = this->NumberOfScalarElementArrays;
  int nPointArrays = this->NumberOfScalarNodeArrays;
  int nGlobalVariables = mmd->GetNumberOfGlobalVariables();

  // CELL VARIABLES

  for (i=0; i<nCellArrays; i++)
    {
    char *nameIn = this->InputElementArrayNames[i];
    int component = this->InputElementArrayComponent[i];

    // ATTRIBUTE EDITOR
    int varIndex = i;
    if(this->EditorFlag && this->EditedVariableName && strcmp(this->EditedVariableName,nameIn)) 
      {
      continue;
      }
    // get the real variable index used in the exodus file
    char **names = this->ModelMetadata->GetOriginalElementVariableNames();
    for(int j=0;j<this->ModelMetadata->GetOriginalNumberOfElementVariables();j++)
      {
      if(!strcmp(names[j],nameIn))
        {
        varIndex = j;
        }
      }

    vtkDataArray *da = ug->GetCellData()->GetArray(nameIn);

    int type = da->GetDataType();
    int size = da->GetNumberOfComponents();

    // We may be able to pass the Exodus library a pointer right into
    // the input's variable arrays.  If not (we had to massage the arrays
    // in some way) then deleteIt is TRUE, meaning we created a copy to
    // be deleted.

    int deleteIt = ((size > 1) ||
                    ((type != VTK_DOUBLE) && this->PassDoubles) ||
                    ((type != VTK_FLOAT) && !this->PassDoubles) ||
                    (this->ElementIndex != NULL));

    if (this->PassDoubles)
      {
      // ATTRIBUTE EDITOR
      if(this->EditorFlag)
        {
        vtkDoubleArray *editedArray = vtkDoubleArray::New();
        vtkIntArray *idArray = vtkIntArray::SafeDownCast(ug->GetPointData()->GetArray("GlobalElementId"));
        if(this->ExtractComponentForEditorD(da, editedArray, idArray, component, this->ElementIndex))
          {
          double *vars = editedArray->GetPointer(0);

          for (idIdx=0; idIdx < nblocks; idIdx++)
            {
            int numElts = this->NumberOfElementsPerBlock[idIdx];
            if (numElts < 1) continue;   // no cells in this block

            int defined = this->BlockVariableTruthValue(idIdx, i);
            if (!defined) continue;    // var undefined in this block

            int id = this->BlockIds[idIdx];
            int first = this->BlockElementStart[idIdx]; 

            rc = ex_put_elem_var(this->fid, ts + 1, varIndex + 1, id, numElts, vars + first);

            if (rc < 0)
              {
              vtkErrorMacro(<< "vtkExodusIIWriter::WriteNextTimeStep ex_put_elem_var");
              return 1;
              }
            }
          }
        editedArray->Delete();
        }
      else
        {
        double *vars = this->ExtractComponentD(da, component, this->ElementIndex);

        for (idIdx=0; idIdx < nblocks; idIdx++)
          {
          int numElts = this->NumberOfElementsPerBlock[idIdx];
          if (numElts < 1) continue;   // no cells in this block

          int defined = this->BlockVariableTruthValue(idIdx, i);
          if (!defined) continue;    // var undefined in this block

          int id = this->BlockIds[idIdx];
          int first = this->BlockElementStart[idIdx]; 

          rc = ex_put_elem_var(this->fid, ts + 1, i + 1, id, numElts, vars + first);

          if (rc < 0)
            {
            vtkErrorMacro(<< "vtkExodusIIWriter::WriteNextTimeStep ex_put_elem_var");
            return 1;
            }
          }

        if (deleteIt)
          {
          delete [] vars;
          }
        }
      }
    else
      {
      // ATTRIBUTE EDITOR
      if(this->EditorFlag)
        {
        vtkFloatArray *editedArray = vtkFloatArray::New();
        vtkIntArray *idArray = vtkIntArray::SafeDownCast(ug->GetPointData()->GetArray("GlobalElementId"));
        if(this->ExtractComponentForEditorF(da, editedArray, idArray, component, this->ElementIndex))
          {
          float *vars = editedArray->GetPointer(0);

          for (idIdx=0; idIdx < nblocks; idIdx++)
            {
            int numElts = this->NumberOfElementsPerBlock[idIdx];
            if (numElts < 1) continue;   // no cells in this block

            int defined = this->BlockVariableTruthValue(idIdx, i);
            if (!defined) continue;    // var undefined in this block

            int id = this->BlockIds[idIdx];
            int first = this->BlockElementStart[idIdx];

            rc = ex_put_elem_var(this->fid, ts + 1, varIndex + 1, id, numElts, vars + first);

            if (rc < 0)
              {
              vtkErrorMacro(<< "vtkExodusIIWriter::WriteNextTimeStep ex_put_elem_var");
              return 1;
              }
            }
          }
        editedArray->Delete();
        }
      else
        {
        float *vars = this->ExtractComponentF(da, component, this->ElementIndex);

        for (idIdx=0; idIdx < nblocks; idIdx++)
          {
          int numElts = this->NumberOfElementsPerBlock[idIdx];
          if (numElts < 1) continue;   // no cells in this block

          int defined = this->BlockVariableTruthValue(idIdx, i);
          if (!defined) continue;    // var undefined in this block

          int id = this->BlockIds[idIdx];
          int first = this->BlockElementStart[idIdx];

          rc = ex_put_elem_var(this->fid, ts + 1, i + 1, id, numElts, vars + first);

          if (rc < 0)
            {
            vtkErrorMacro(<< "vtkExodusIIWriter::WriteNextTimeStep ex_put_elem_var");
            return 1;
            }
          }
        if (deleteIt)
          {
          delete [] vars;
          }
        }
      }
    }

  // POINT VARIABLES

  for (i=0; i<nPointArrays; i++)
    {
    char *nameIn = this->InputNodeArrayNames[i];
    int component = this->InputNodeArrayComponent[i];

    // ATTRIBUTE EDITOR
    // if we are writing a single variable and the names do not match, don't write
    int varIndex = i;

    if(this->EditorFlag && this->EditedVariableName && strcmp(this->EditedVariableName,nameIn)) 
      {
      continue;
      }
    // get the real variable index used in the exodus file
    char **names = this->ModelMetadata->GetOriginalNodeVariableNames();
    for(int j=0;j<this->ModelMetadata->GetOriginalNumberOfNodeVariables();j++)
      {
      if(!strcmp(names[j],nameIn))
        {
        varIndex = j;
        }
      }

    vtkDataArray *da = ug->GetPointData()->GetArray(nameIn);

    int type = da->GetDataType();
    int size = da->GetNumberOfComponents();

    int deleteIt = ((size > 1) ||
                    ((type != VTK_DOUBLE) && this->PassDoubles) ||
                    ((type != VTK_FLOAT) && !this->PassDoubles));

    if (this->PassDoubles)
      {
      // ATTRIBUTE EDITOR
      if(this->EditorFlag)
        {
        vtkDoubleArray *editedArray = vtkDoubleArray::New();
        vtkIntArray *idArray = vtkIntArray::SafeDownCast(ug->GetPointData()->GetArray("InternalNodeId"));
        if(this->ExtractComponentForEditorD(da, editedArray, idArray, component, NULL))
          {
          double *vars = editedArray->GetPointer(0);
          rc = ex_put_nodal_var(this->fid, ts + 1, varIndex + 1, npoints, vars);
          }
        editedArray->Delete();
        }
      else
        {
        double *vars = this->ExtractComponentD(da, component, NULL);
        rc = ex_put_nodal_var(this->fid, ts + 1, i + 1, npoints, vars);
        if (deleteIt)
          {
          delete [] vars;
          }
        }
      }
    else
      {
      // ATTRIBUTE EDITOR
      if(this->EditorFlag)
        {
        vtkFloatArray *editedArray = vtkFloatArray::New();
        vtkIntArray *idArray = vtkIntArray::SafeDownCast(ug->GetPointData()->GetArray("InternalNodeId"));
        if(this->ExtractComponentForEditorF(da, editedArray, idArray, component, NULL))
          {
          float *vars = editedArray->GetPointer(0);
          rc = ex_put_nodal_var(this->fid, ts + 1, varIndex + 1, npoints, vars);
          }
        editedArray->Delete();
        }
      else
        {
        float *vars = this->ExtractComponentF(da, component, NULL);
        rc = ex_put_nodal_var(this->fid, ts + 1, i + 1, npoints, vars);
        if (deleteIt)
          {
          delete [] vars;
          }
        }
      }

    if (rc < 0)
      {
      vtkErrorMacro(<< "vtkExodusIIWriter::WriteNextTimeStep ex_put_nodal_var");
      return 1;
      }
    }

  // GLOBAL VARIABLES

  if (nGlobalVariables > 0)
    {
    float *vals = mmd->GetGlobalVariableValue();

    if (this->PassDoubles)
      {
      double *dvals = new double [nGlobalVariables];
      for (int ii=0; ii<nGlobalVariables; ii++)
        {
        dvals[ii] = (double)vals[ii];
        }

      rc = ex_put_glob_vars(this->fid, ts + 1, nGlobalVariables, dvals);

      delete [] dvals;
      }
    else
      {
      rc = ex_put_glob_vars(this->fid, ts + 1, nGlobalVariables, vals);
      }

    if (rc < 0)
      {
      vtkErrorMacro(<< "vtkExodusIIWriter::WriteNextTimeStep ex_put_glob_vars");
      return 1;
      }
    }

  return 0;
}
int vtkExodusIIWriter::BlockVariableTruthValue(int blockIdx, int varIdx)
{
  int tt=0;
  int nvars = this->NumberOfScalarElementArrays;
  int nblocks = this->NumberOfElementBlocks;

  if (this->AllVariablesDefinedInAllBlocks)
    {
    tt = 1;
    } 
  else if ( (blockIdx >= 0) && (blockIdx < nblocks) &&
       (varIdx >= 0) && (varIdx < nvars))
    {
    tt = this->BlockElementVariableTruthTable[(blockIdx * nvars) + varIdx];
    }
  else
    {
    vtkErrorMacro(<< "vtkExodusIIWriter::BlockVariableTruthValue invalid index");
    }

  return tt;
}

//-----------------------------------------------------------------------
// Properties
//-----------------------------------------------------------------------
int vtkExodusIIWriter::WriteProperties()
{
  int rc = 0;
  int i;

  vtkModelMetadata *em = this->GetModelMetadata();

  int nbprop = em->GetNumberOfBlockProperties();
  int nnsprop = em->GetNumberOfNodeSetProperties();
  int nssprop = em->GetNumberOfSideSetProperties();

  if (nbprop)
    {
    char **names = em->GetBlockPropertyNames();

    // Exodus library "feature".  By convention there is a property
    // array called "ID", the value of which is the ID of the block,
    // node set or side set.  This property is special.  For example,
    // if you change the property value for a block, that block's
    // block ID is changed.  I had no idea *how* special this property
    // was, however.  If you use ex_put_prop_names to tell the library
    // what your property names are, and "ID" happens to be one of those
    // names, then the library fills out the whole property array for
    // you.  Then if you follow this call with ex_put_prop_array for
    // each property array, including "ID", you get *two* arrays named
    // "ID".  This is not documented, and totally unexpected.
    //
    // ex_put_prop_names is not required, it's just more efficient to
    // call it before all the ex_put_prop_array calls.  So we are
    // not going to call it.
    //
    // rc = ex_put_prop_names(this->fid, EX_ELEM_BLOCK, nbprop, names);

    if (rc >= 0 && this->EditorFlag == 0)
      {
      int *values = em->GetBlockPropertyValue();
  
      for (i=0; i<nbprop; i++)
        {
        rc = ex_put_prop_array(this->fid, EX_ELEM_BLOCK, names[i], values);
        if (rc) break;
        values += this->NumberOfElementBlocks;
        }
      }
    }

  if (!rc && nnsprop)
    {
    char **names = em->GetNodeSetPropertyNames();
    int nnsets = em->GetNumberOfNodeSets();

    // rc = ex_put_prop_names(this->fid, EX_NODE_SET, nnsprop, names);

    if (rc >= 0 && this->EditorFlag == 0)
      {
      int *values = em->GetNodeSetPropertyValue();

      for (i=0; i<nnsprop; i++)
        {
        rc = ex_put_prop_array(this->fid, EX_NODE_SET, names[i], values);
        if (rc) break;
        values += nnsets;
        }
      }
    }

  if (!rc && nssprop)
    {
    char **names = em->GetSideSetPropertyNames();
    int nssets = em->GetNumberOfSideSets();

    // rc = ex_put_prop_names(this->fid, EX_SIDE_SET, nssprop, names);

    if (rc >= 0 && this->EditorFlag == 0)
      {
      int *values = em->GetSideSetPropertyValue();

      for (i=0; i<nssprop; i++)
        {
        rc = ex_put_prop_array(this->fid, EX_SIDE_SET, names[i], values);
        if (rc) break;
        values += nssets;
        }
      }
    }

  int fail = (rc < 0);

  return fail;
}
//-----------------------------------------------------------------------
// Side sets and node sets
//-----------------------------------------------------------------------
int vtkExodusIIWriter::WriteSideSetInformation()
{
  int rc= 0;
  int i, j, k;
 
  vtkModelMetadata *em = this->GetModelMetadata();

  int nssets = em->GetNumberOfSideSets();

  if (nssets < 1) return 0;

  // If cells are written out to file in a different order than
  // they appear in the input, we need a mapping from their internal
  // id in the input to their internal id in the output.

  vtkstd::map<int, int> newElementId;
  vtkstd::map<int, int>::iterator idIt;

  if (this->ElementIndex)
    {
    vtkUnstructuredGrid *ug = this->GetInput();
    int ncells = ug->GetNumberOfCells();

    for (i=0; i<ncells; i++)
      {
      newElementId.insert(vtkstd::map<int,int>::value_type(this->ElementIndex[i], i));
      }
    }

  int nids = em->GetSumSidesPerSideSet();

  if (nids < 1 && this->EditorFlag == 0)
    {
    int *buf = new int [nssets];

    memset(buf, 0, sizeof(int) * nssets);

    rc = ex_put_concat_side_sets(this->fid, em->GetSideSetIds(),
              buf, buf, buf, buf, NULL, NULL, NULL);

    delete [] buf;

    return (rc < 0);
    }

  int *ssSize = new int [nssets];
  int *ssNumDF = new int [nssets];
  int *ssIdIdx = new int [nssets];
  int *ssDFIdx = new int [nssets];

  int ndf = em->GetSumDistFactPerSideSet();

  int *idBuf = new int [nids];
  int *sideBuf = new int [nids];
  float *dfBuf = NULL;
  double *dfBufD = NULL;

  if (ndf)
    {
    if (this->PassDoubles)
      {
      dfBufD = new double [ndf];
      }
    else
      {
      dfBuf = new float [ndf];
      }
    }
  
  int *emSsSize = em->GetSideSetSize();
  int *emIdIdx = em->GetSideSetListIndex();
  int *emDFIdx = em->GetSideSetDistributionFactorIndex();

  int nextId = 0;
  int nextDF = 0;
  int outputInternalId = 0;

  for (i=0; i<nssets; i++)
    {
    ssSize[i] = 0;
    ssNumDF[i] = 0;

    ssIdIdx[i] = nextId;
    ssDFIdx[i] = nextDF;

    if (emSsSize[i] == 0) continue;

    int *ids = em->GetSideSetElementList() + emIdIdx[i];
    int *sides = em->GetSideSetSideList() + emIdIdx[i];

    int *numDFPerSide = em->GetSideSetNumDFPerSide() + emIdIdx[i];
    float *df = NULL;

    if (ndf > 0)
      {
      df = em->GetSideSetDistributionFactors() + emDFIdx[i];
      }

    for (j=0; j< emSsSize[i]; j++)
      {
      // Have to check if this element is still in the ugrid.
      // It may have been deleted since the ExodusModel was created.

      int lid = this->GetElementLocalId(ids[j]);

      if (lid >= 0)
        {
        ssSize[i]++;

        if (this->ElementIndex)
          {
          idIt = newElementId.find(lid);
          outputInternalId = idIt->second + 1;
          }
        else 
          {
          outputInternalId = lid + 1;
          }

        idBuf[nextId] = outputInternalId;
        sideBuf[nextId] = sides[j];

        nextId++;
  
        if (numDFPerSide[j] > 0)
          {
          ssNumDF[i] += numDFPerSide[j];
  
          if (this->PassDoubles)
            {
            for (k=0; k < numDFPerSide[j]; k++)
              {
              dfBufD[nextDF++] = (double)df[k];
              }
            }
          else
            {
            for (k=0; k < numDFPerSide[j]; k++)
              {
              dfBuf[nextDF++] = df[k];
              }
            }
          }
        }

      if (df) df += numDFPerSide[j];
      }
    }

  if(this->EditorFlag == 0)
    {
    if (this->PassDoubles)
      {
      rc = ex_put_concat_side_sets(this->fid, em->GetSideSetIds(),
              ssSize, ssNumDF, ssIdIdx, ssDFIdx, idBuf, sideBuf, dfBufD);
      }
    else
      {
      rc = ex_put_concat_side_sets(this->fid, em->GetSideSetIds(),
              ssSize, ssNumDF, ssIdIdx, ssDFIdx, idBuf, sideBuf, dfBuf);
      }
    }

  delete [] ssSize;
  delete [] ssNumDF;
  delete [] ssIdIdx;
  delete [] ssDFIdx;
  delete [] idBuf;
  delete [] sideBuf;
  if (dfBuf) delete [] dfBuf;
  else if (dfBufD) delete [] dfBufD;

  int fail = (rc < 0);

  return fail;
}
int vtkExodusIIWriter::WriteNodeSetInformation()
{
  int rc = 0;
  int i, j;
 
  vtkModelMetadata *em = this->GetModelMetadata();

  int nnsets = em->GetNumberOfNodeSets();

  if (nnsets < 1) return 0;

  int nids = em->GetSumNodesPerNodeSet();

  if (nids < 1 && this->EditorFlag == 0)
    {
    int *buf = new int [nnsets];

    memset(buf, 0, sizeof(int) * nnsets);

    rc = ex_put_concat_node_sets(this->fid, em->GetNodeSetIds(),
              buf, buf, buf, buf, NULL, NULL);

    delete [] buf;

    return (rc < 0);
    }

  int *nsSize = new int [nnsets];
  int *nsNumDF = new int [nnsets];
  int *nsIdIdx = new int [nnsets];
  int *nsDFIdx = new int [nnsets];

  int ndf = em->GetSumDistFactPerNodeSet();

  int *idBuf = new int [nids];
  float *dfBuf = NULL;
  double *dfBufD = NULL;

  if (ndf)
    {
    if (this->PassDoubles)
      {
      dfBufD = new double [ndf];
      }
    else
      {
      dfBuf = new float [ndf];
      }
    }
  
  int *emNsSize = em->GetNodeSetSize();
  int *emNumDF = em->GetNodeSetNumberOfDistributionFactors();
  int *emIdIdx = em->GetNodeSetNodeIdListIndex();
  int *emDFIdx = em->GetNodeSetDistributionFactorIndex();

  int nextId = 0;
  int nextDF = 0;

  for (i=0; i<nnsets; i++)
    {
    nsSize[i] = 0;
    nsNumDF[i] = 0;

    nsIdIdx[i] = nextId;
    nsDFIdx[i] = nextDF;

    int *ids = em->GetNodeSetNodeIdList() + emIdIdx[i];
    float *df = em->GetNodeSetDistributionFactors() + emDFIdx[i];

    for (j=0; j< emNsSize[i]; j++)
      {
      // Have to check if this node is still in the ugrid.
      // It may have been deleted since the ExodusModel was created.

      int lid = this->GetNodeLocalId(ids[j]);

      if (lid < 0) continue;

      nsSize[i]++;
      idBuf[nextId++] = lid + 1;

      if (emNumDF[i] > 0)
        {
        nsNumDF[i]++;
 
        if (this->PassDoubles)
          {
          dfBufD[nextDF++] = (double)df[j];
          }
        else
          {
          dfBuf[nextDF++] = df[j];
          }
        }
      }
    }

  if(this->EditorFlag == 0)
    {
    if (this->PassDoubles)
      {
      rc = ex_put_concat_node_sets(this->fid, em->GetNodeSetIds(),
                nsSize, nsNumDF, nsIdIdx, nsDFIdx, idBuf, dfBufD);
      }
    else
      {
      rc = ex_put_concat_node_sets(this->fid, em->GetNodeSetIds(),
                nsSize, nsNumDF, nsIdIdx, nsDFIdx, idBuf, dfBuf);
      }
    }

  delete [] nsSize;
  delete [] nsNumDF;
  delete [] nsIdIdx;
  delete [] nsDFIdx;
  delete [] idBuf;
  if (dfBuf) delete [] dfBuf;
  else if (dfBufD) delete [] dfBufD;

  int fail = (rc < 0);

  return fail;
}

//---------------------------------------------------------
// Points and point IDs, element IDs
//---------------------------------------------------------
int vtkExodusIIWriter::WriteCoordinateNames()
{
  vtkModelMetadata *em = this->GetModelMetadata();

  if(this->EditorFlag == 1)
    {
    return 0;
    }

  int rc = ex_put_coord_names(this->fid, em->GetCoordinateNames());

  int fail = (rc < 0);

  return fail;
}
int vtkExodusIIWriter::WriteGlobalPointIds()
{
  int rc = 0;

  if(this->EditorFlag == 1)
    {
    return 0;
    }

  if (this->GlobalNodeIdList)
    {
    rc = ex_put_node_num_map(this->fid, this->GlobalNodeIdList);
    }

  int fail = ( rc < 0);

  return fail;
}
int vtkExodusIIWriter::WriteGlobalElementIds()
{
  int rc = 0;

  if(this->EditorFlag == 1)
    {
    return 0;
    }

  int *ids = this->GlobalElementIdList;

  if (ids)
    {
    if (this->ElementIndex)
      {
      vtkUnstructuredGrid *ug = this->GetInput();
      int ncells = ug->GetNumberOfCells();
  
      int *newIds = new int [ncells];
  
      for (int i=0; i<ncells; i++)
        {
        newIds[i] = ids[this->ElementIndex[i]];
        }

      ids = newIds;
      }
  
    rc = ex_put_elem_num_map(this->fid, ids);
  
    if (ids != this->GlobalElementIdList)
      {
      delete [] ids;
      }
    }

  int fail = ( rc < 0);

  return fail;
}
int vtkExodusIIWriter::WritePoints()
{
  vtkUnstructuredGrid *ug = this->GetInput();
  if (!ug) return 1;

  if(this->EditorFlag == 1)
    {
    return 0;
    }

  vtkPoints *pts = ug->GetPoints();
  int npts = pts->GetNumberOfPoints();
  vtkDataArray *da = pts->GetData();

  int type = pts->GetDataType();

  int i = 0; 
  int fail = 0;

  if (this->PassDoubles)
    {
    double *px = new double [npts];
    double *py = new double [npts];
    double *pz = new double [npts];

    if (type == VTK_DOUBLE)
      {
      vtkDoubleArray *a = vtkDoubleArray::SafeDownCast(da);
      double *p = a->GetPointer(0);

      for (i=0; i<npts; i++)
        {
        px[i] = p[0];
        py[i] = p[1];
        pz[i] = p[2];
        p += 3;
        }
      }
    else
      {
      for (i=0; i<npts; i++)
        {
        px[i] = da->GetComponent(i, 0);
        py[i] = da->GetComponent(i, 1);
        pz[i] = da->GetComponent(i, 2);
        }
      }

    int rc = ex_put_coord(this->fid, px, py, pz);

    fail = (rc < 0);
   
    delete [] px;
    delete [] py;
    delete [] pz;
    }
  else 
    {
    float *px = new float [npts];
    float *py = new float [npts];
    float *pz = new float [npts];

    if (type == VTK_FLOAT)
      {
      vtkFloatArray *a = vtkFloatArray::SafeDownCast(da);
      float *p = a->GetPointer(0);

      for (i=0; i<npts; i++)
        {
        px[i] = p[0];
        py[i] = p[1];
        pz[i] = p[2];
        p += 3;
        }
      }
    else
      {
      vtkDoubleArray *a = vtkDoubleArray::SafeDownCast(da);
      double *p = a->GetPointer(0);

      for (i=0; i<npts; i++)
        {
        px[i] = (float)*p++;
        py[i] = (float)*p++;
        pz[i] = (float)*p++;
        }
      }

    int rc = ex_put_coord(this->fid, px, py, pz);

    fail = (rc < 0);
   
    delete [] px;
    delete [] py;
    delete [] pz;
    }

  return fail;
}

//---------------------------------------------------------
// Initialization, QA, Title, information records
//---------------------------------------------------------
int vtkExodusIIWriter::WriteQARecords()
{
  if(this->EditorFlag == 1)
    {
    return 0;
    }

  vtkModelMetadata *em = this->GetModelMetadata();

  int nrecs = em->GetNumberOfQARecords();

  if (nrecs > 0)
    {
    typedef char *p4[4];

    p4 *qarecs = new p4 [nrecs];

    for (int i=0; i<nrecs; i++)
      {
      em->GetQARecord(i, &qarecs[i][0], &qarecs[i][1], &qarecs[i][2], &qarecs[i][3]);
      }
    ex_put_qa(this->fid, nrecs, qarecs);

    delete [] qarecs;
    }

  return 0;
}
int vtkExodusIIWriter::WriteInformationRecords()
{
  if(this->EditorFlag == 1)
    {
    return 0;
    }

  vtkModelMetadata *em = this->GetModelMetadata();

  int nlines = em->GetNumberOfInformationLines();

  if (nlines > 0)
    {
    char **lines = NULL;

    em->GetInformationLines(&lines);

    ex_put_info(this->fid, nlines, lines);
    }

  return 0;
}

int vtkExodusIIWriter::WriteInitializationParameters()
{
  vtkUnstructuredGrid *ug = this->GetInput();
  if (!ug) return 1;

  if(this->EditorFlag == 1)
    {
    return 0;
    }

  vtkModelMetadata *em = this->GetModelMetadata();

  int dim = em->GetDimension();
  int nnodes = ug->GetNumberOfPoints();
  int ncells = ug->GetNumberOfCells();
  int nnsets = em->GetNumberOfNodeSets();
  int nssets = em->GetNumberOfSideSets();
  char *title = em->GetTitle();
  int numBlocks = em->GetNumberOfBlocks();

  int rc = ex_put_init(this->fid, title, dim, nnodes, ncells, 
                       numBlocks, nnsets, nssets);

  int fail = (rc < 0);
  
  return fail;
}
//========================================================================
// BLOCKS 
//========================================================================
void vtkExodusIIWriter::InitializeBlockLists()
{
  this->NumberOfElementBlocks = 0;
  this->BlockIds = NULL;
  this->BlockElementType = NULL;
  this->BlockElementStart = NULL;
  this->ElementIndex = NULL;
  this->NumberOfElementsPerBlock  = NULL;
  this->NumberOfNodesPerElementInBlock = NULL;
  this->NumberOfAttributesPerElementInBlock = NULL;
  this->BlockElementAttributesF = NULL;
  this->BlockElementAttributesD = NULL;
  this->BlockElementConnectivity = NULL;
}
void vtkExodusIIWriter::ClearBlockLists()
{
  int i;
  int len = this->NumberOfElementBlocks;

  if (len == 0) return;

  FREELIST(this->BlockElementType, len);
  FREELIST(this->BlockElementAttributesF, len);
  FREELIST(this->BlockElementAttributesD, len);
  FREELIST(this->BlockElementConnectivity, len);

  FREE(this->BlockIds);
  FREE(this->BlockElementStart);
  FREE(this->ElementIndex);
  FREE(this->NumberOfElementsPerBlock);
  FREE(this->NumberOfNodesPerElementInBlock);
  FREE(this->NumberOfAttributesPerElementInBlock);

  if (this->LocalBlockIndexMap)
    {
    delete this->LocalBlockIndexMap;
    this->LocalBlockIndexMap = NULL;
    }

  this->NumberOfElementBlocks = 0;
}
int vtkExodusIIWriter::WriteBlockInformation()
{
  int i, j, rc;
  vtkUnstructuredGrid *ug = this->GetInput();
  vtkModelMetadata *em = this->GetModelMetadata();

  this->ClearBlockLists();

  int nblocks = em->GetNumberOfBlocks();
  int ncells = ug->GetNumberOfCells();

  int *ids = em->GetBlockIds();
  char **blockNames = em->GetBlockElementType();
  int *nodesPerElement = em->GetBlockNodesPerElement();
  int *numAttributes = em->GetBlockNumberOfAttributesPerElement();

  this->NumberOfElementBlocks = nblocks;

  this->BlockIds = new int [nblocks];
  this->BlockElementType = new char * [nblocks];
  this->BlockElementStart = new int [nblocks];
  this->NumberOfElementsPerBlock = new int [nblocks];
  this->NumberOfNodesPerElementInBlock = new int [nblocks];
  this->NumberOfAttributesPerElementInBlock = new int [nblocks];

  this->BlockElementConnectivity = new int * [nblocks];

  if (this->PassDoubles)
    {
    this->BlockElementAttributesD = new double * [nblocks];
    }
  else
    {
    this->BlockElementAttributesF = new float * [nblocks];
    }

  for (i=0; i<nblocks; i++)
    {
    this->NumberOfElementsPerBlock[i] = 0;
    this->BlockElementStart[i] = -1;

    this->BlockIds[i] = (int)ids[i];
    this->BlockElementType[i] = vtkExodusIIWriter::StrDupWithNew(blockNames[i]);
    this->NumberOfNodesPerElementInBlock[i] = nodesPerElement[i];
    this->NumberOfAttributesPerElementInBlock[i] = numAttributes[i];

    this->BlockElementConnectivity[i] = NULL;

    if (this->PassDoubles)
      {
      this->BlockElementAttributesD[i] = NULL;
      }
    else
      {
      this->BlockElementAttributesF[i] = NULL;
      }
    }

  // Count the number of elements in each block - it's not necessarily
  // the number in the ExodusModel because some cells may have been
  // deleted.
  //
  // The elements in the input may not be in order by block, but we must
  // write element IDs and element variables out to the Exodus file in
  // order by block.  Create a mapping if necessary, for an ordering by
  // block to the ordering found in the input unstructured grid.

  int lastId = -1;
  int idx = -1;
  int needMapping = 0;

  for (i=0; i<ncells; i++)
    {
    int blockId = this->BlockIdList[i];

    if (blockId != lastId)
      {
      idx = this->GetBlockLocalIndex(blockId);
      }

    this->NumberOfElementsPerBlock[idx]++;

    if (!needMapping)
      {
      if (blockId != lastId)
        {
        int start = this->BlockElementStart[idx];
        if (start == -1)
          {
          this->BlockElementStart[idx] = i;
          }
        else
          {
          needMapping = 1;
          }
        }
      }
    lastId = blockId;
    }

  if (needMapping)
    {
    // Element variables and global IDs are mixed up, not grouped
    // by block as they must be when written out.

    this->ElementIndex = new int [ncells];

    this->BlockElementStart[0] = 0;

    for (i=1; i<nblocks; i++)
      {
      this->BlockElementStart[i] = 
        this->BlockElementStart[i-1] + this->NumberOfElementsPerBlock[i-1];
      }

    int *blockCount = new int [nblocks];
    memset(blockCount, 0, sizeof(int) * nblocks);

    lastId = -1;
    idx = -1;

    for (i=0; i<ncells; i++)
      {
      int blockId = this->BlockIdList[i];

      if (blockId != lastId)
        {
        idx = this->GetBlockLocalIndex(blockId);
        }

      int which = this->BlockElementStart[idx] + blockCount[idx]++;

      this->ElementIndex[which] = i;

      lastId = blockId;
      }

    delete [] blockCount;
    }

  // Write out the connectivity array and the attribute array.

  int *nodeCount = new int [nblocks];
  int *attCount = new int [nblocks];

  // For each block, a map from element global ID to it's location
  // within it's block in the ExodusModel object.

  vtkstd::map<int, int> **eltIdx = new vtkstd::map<int, int> * [nblocks];
  vtkstd::map<int, int>::iterator eltIt;

  for (i=0; i<nblocks; i++)
    {
    nodeCount[i] = 0;
    attCount[i] = 0;
    eltIdx[i] = NULL;

    int numElts = this->NumberOfElementsPerBlock[i];
    int numAtts = this->NumberOfAttributesPerElementInBlock[i];
    int numNodes = this->NumberOfNodesPerElementInBlock[i];

    if (numElts > 0)
      {
      this->BlockElementConnectivity[i] = new int [numElts * numNodes];

      if (numAtts > 0)
        {
        if (this->PassDoubles)
          {
          this->BlockElementAttributesD[i] = new double [numElts * numAtts];
          }
        else
          {
          this->BlockElementAttributesF[i] = new float [numElts * numAtts];
          }

        eltIdx[i] = this->BuildBlockElementSearchStructure(i);
        }
      }
    }

  vtkCellArray *ca = ug->GetCells();
  vtkIdType *ptIds = ca->GetPointer();
  vtkIdTypeArray *loca = ug->GetCellLocationsArray();
  vtkIdType *loc = loca->GetPointer(0);

  float *att = em->GetBlockAttributes();
  int *attIdx = em->GetBlockAttributesIndex();

  int skipAttributes=0;
  lastId = -1;
  idx = -1;

  for (i=0; i<ncells; i++)
    {
    int nextIdx = (this->ElementIndex ? this->ElementIndex[i] : i);

    int blockId = this->BlockIdList[nextIdx];

    if (blockId != lastId)
      {
      idx = this->GetBlockLocalIndex(blockId);
      lastId = blockId;
      }

    // the block connectivity array

    vtkIdType ptListIdx = loc[nextIdx];

    vtkIdType npts = ptIds[ptListIdx++];

    for (vtkIdType p=0; p<npts; p++)
      {
      int ExodusPointId = (int)(ptIds[ptListIdx++]) + 1;
      this->BlockElementConnectivity[idx][nodeCount[idx]++] = ExodusPointId;
      }

    // the block element attributes

    int numAtts = this->NumberOfAttributesPerElementInBlock[idx];

    if ((numAtts == 0) || skipAttributes) continue;

    if (!this->GlobalElementIdList)
      {
      vtkWarningMacro(<< 
      "Exodus writer must omit element block attributes, because there are no global element IDs");
      skipAttributes = 1;
      break;
      }

    int globalId = this->GlobalElementIdList[nextIdx];
    eltIt = eltIdx[idx]->find(globalId);

    if (eltIt == eltIdx[idx]->end())
      {
      vtkWarningMacro(<< 
      "Exodus writer must omit element block attributes, because new elements were added");
      skipAttributes = 1;
      break;
      }

    int where = eltIt->second;

    float *eltAtt = att +              // list of all blocks attribute values
                    attIdx[idx] +      // location for this block
                    (where * numAtts); // location for the element in the block

    if (this->PassDoubles)
      {
      for (j=0; j<numAtts; j++)
        {
        this->BlockElementAttributesD[idx][attCount[idx]++] = (double)eltAtt[j];
        }
      }
    else
      {
      for (j=0; j<numAtts; j++)
        {
        this->BlockElementAttributesF[idx][attCount[idx]++] = eltAtt[j];
        }
      }
    }

  for (i=0; i<nblocks; i++)
    {
    if (eltIdx[i]) delete eltIdx[i];
    }

  delete [] eltIdx;
  delete [] nodeCount;
  delete [] attCount;

  if (skipAttributes)
    {
    for (i=0; i<nblocks; i++)
      {
      this->NumberOfAttributesPerElementInBlock[i] = 0;
      FREELIST(this->BlockElementAttributesD, nblocks);
      FREELIST(this->BlockElementAttributesF, nblocks);
      }
    }

  if(this->EditorFlag == 1)
    {
    return 0;
    }

  // Now, finally, write out the block information

  int fail = 0;

  for (i=0; i<nblocks; i++)
    {
    rc = ex_put_elem_block(this->fid, this->BlockIds[i],
                this->BlockElementType[i], this->NumberOfElementsPerBlock[i],
                this->NumberOfNodesPerElementInBlock[i], 
                this->NumberOfAttributesPerElementInBlock[i]);

    if (rc < 0)
      {
      fail = 1;
      break;
      }
    }

  if (fail)
    {
    return 1;
    }

  for (i=0; i<nblocks; i++)
    {
    if (this->NumberOfElementsPerBlock[i] > 0)
      {
      rc = ex_put_elem_conn(this->fid, this->BlockIds[i],
                         this->BlockElementConnectivity[i]);

      if (rc < 0)
        {
        fail = 1;
        break;
        }

      if (this->NumberOfAttributesPerElementInBlock[i] > 0)
        {
        if (this->PassDoubles)
          {
          rc = ex_put_elem_attr(this->fid, this->BlockIds[i],
                          this->BlockElementAttributesD[i]);
          }
        else
          {
          rc = ex_put_elem_attr(this->fid, this->BlockIds[i],
                          this->BlockElementAttributesF[i]);
          }

        if (rc < 0)
          {
          fail = 1;
          break;
          }
        }
      }
    }
  
  return fail;
}
vtkstd::map<int, int> *vtkExodusIIWriter::BuildBlockElementSearchStructure(int block)
{
  // Create a map from an element global ID to it's location in
  // the block array in the ExodusModel object.

  vtkModelMetadata *em = this->GetModelMetadata();

  int *blockElts = em->GetBlockElementIdList();
  int *numElts = em->GetBlockNumberOfElements();
  int *listIdx = em->GetBlockElementIdListIndex();

  vtkstd::map<int, int> *eltMap = new vtkstd::map<int, int>;

  int blockSize = numElts[block];

  int *eltIds = blockElts + listIdx[block]; 

  for (int i=0; i<blockSize; i++)
    {
    eltMap->insert(vtkstd::map<int,int>::value_type(eltIds[i], i));
    }

  return eltMap;
}

//----------------------------------------------------------------------------
char *vtkExodusIIWriter::StrDupWithNew(const char *s)
{
  char *newstr = NULL;

  if (s)
    {
    int len = strlen(s);
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
//----------------------------------------------------------------------------
void vtkExodusIIWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkWriter::PrintSelf(os,indent);

  if (this->FileName)
    {
    os << indent << "FileName " << this->FileName << "\n";
    }
  if (this->MyFileName)
    {
    os << indent << "MyFileName " << this->MyFileName << "\n";
    }
  os << indent << "ErrorStatus " << this->ErrorStatus << endl;
  os << indent << "StoreDoubles " << this->StoreDoubles << endl;
  os << indent << "GhostLevel " << this->GhostLevel << endl;

  if (this->BlockIdArrayName)
    {
    os << indent << "BlockIdArrayName " << this->BlockIdArrayName << endl;
    }
  if (this->GlobalNodeIdArrayName)
    {
    os << indent << "GlobalNodeIdArrayName " << this->GlobalNodeIdArrayName << endl;
    }
  if (this->GlobalElementIdArrayName)
    {
    os << indent << "GlobalNodeIdArrayName " << this->GlobalNodeIdArrayName << endl;
    }

  os << indent << "WriteOutBlockIdArray " << this->WriteOutBlockIdArray << endl;
  os << indent << "WriteOutGlobalNodeIdArray " << this->WriteOutGlobalNodeIdArray << endl;
  os << indent << "WriteOutGlobalElementIdArray " << this->WriteOutGlobalElementIdArray << endl;

  os << indent << "ModelMetadata " << this->ModelMetadata << endl;
}
