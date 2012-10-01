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

#include "vtkExodusIIWriter.h"
#include "vtkObjectFactory.h"
#include "vtkModelMetadata.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkDoubleArray.h"
#include "vtkDataObject.h"
#include "vtkFieldData.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkUnstructuredGrid.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkIdList.h"
#include "vtkThreshold.h"
#include "vtkIntArray.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkToolkits.h" // for VTK_USE_PARALLEL

#include "vtk_exodusII.h"
#include <time.h>
#include <ctype.h>

vtkObjectFactoryNewMacro (vtkExodusIIWriter);
vtkCxxSetObjectMacro (vtkExodusIIWriter, ModelMetadata, vtkModelMetadata);

//----------------------------------------------------------------------------

vtkExodusIIWriter::vtkExodusIIWriter ()
{
  this->fid = -1;
  this->FileName = 0;

  this->StoreDoubles = -1;
  this->GhostLevel = 0;
  this->WriteOutBlockIdArray = 0;
  this->WriteOutGlobalElementIdArray = 0;
  this->WriteOutGlobalNodeIdArray = 0;
  this->WriteAllTimeSteps = 0;
  this->BlockIdArrayName = 0;
  this->ModelMetadata = 0;

  this->NumberOfTimeSteps = 0;
  this->TimeValues = 0;
  this->CurrentTimeIndex = 0;
  this->FileTimeOffset = 0;

  this->AtLeastOneGlobalElementIdList = 0;
  this->AtLeastOneGlobalNodeIdList = 0;

  this->NumPoints = 0;
  this->NumCells = 0;

  this->PassDoubles = 1;

  this->BlockElementVariableTruthTable = 0;

  this->LocalNodeIdMap = 0;
  this->LocalElementIdMap = 0;

}

vtkExodusIIWriter::~vtkExodusIIWriter ()
{
  this->SetModelMetadata(0); // kill the reference if its there

  if (this->FileName)
    {
    delete [] this->FileName;
    }
  if (this->BlockIdArrayName)
    {
    delete [] this->BlockIdArrayName;
    }
  if (this->TimeValues)
    {
    this->TimeValues->Delete ();
    }
  if (this->BlockElementVariableTruthTable)
    {
    delete [] this->BlockElementVariableTruthTable;
    }
  for (size_t i = 0; i < this->BlockIdList.size (); i ++)
    {
    this->BlockIdList[i]->UnRegister (this);
    }
}

void vtkExodusIIWriter::PrintSelf (ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "FileName " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "StoreDoubles " << this->StoreDoubles << endl;
  os << indent << "GhostLevel " << this->GhostLevel << endl;
  os << indent << "WriteOutBlockIdArray " << this->WriteOutBlockIdArray << endl;
  os << indent << "WriteOutGlobalNodeIdArray " << this->WriteOutGlobalNodeIdArray << endl;
  os << indent << "WriteOutGlobalElementIdArray " << this->WriteOutGlobalElementIdArray << endl;
  os << indent << "WriteAllTimeSteps " << this->WriteAllTimeSteps << endl;
  os << indent << "BlockIdArrayName " << (this->BlockIdArrayName ? this->BlockIdArrayName : "(none)") << endl;
  os << indent << "ModelMetadata " << (this->ModelMetadata ? "" : "(none)") << endl;
  if (this->ModelMetadata)
    {
    this->ModelMetadata->PrintSelf (os, indent.GetNextIndent ());
    }
}

//----------------------------------------------------------------------------
int vtkExodusIIWriter::ProcessRequest (
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    return this->RequestInformation(request, inputVector, outputVector);
    }
  else if(request->Has(
      vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    // get the requested update extent
    if(!this->TimeValues)
      {
      this->TimeValues = vtkDoubleArray::New();
      vtkInformation *info = inputVector[0]->GetInformationObject(0);
      double *data = info->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      int len = info->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      this->TimeValues->SetNumberOfValues (len);
      for (int i = 0; i < len; i ++)
        {
        this->TimeValues->SetValue (i, data[i]);
        }
      }
    if (this->WriteAllTimeSteps)
      {
      if(this->TimeValues->GetPointer(0))
        {
        double timeReq= this->TimeValues->GetValue(this->CurrentTimeIndex);
        inputVector[0]->GetInformationObject(0)->Set
        ( vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),  timeReq);
        }
      }
    return 1;
    }
  // generate the data
  else if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    return this->RequestData(request, inputVector, outputVector);
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkExodusIIWriter::RequestInformation (
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  if ( inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) )
    {
    this->NumberOfTimeSteps =
      inInfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
    }
  else
    {
    this->NumberOfTimeSteps = 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkExodusIIWriter::FillInputPortInformation (
  int vtkNotUsed (port),
  vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),
    "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkExodusIIWriter::RequestData (
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  if (!this->FileName)
    {
    return 1;
    }

  vtkInformation* inInfo = inputVector[0]->GetInformationObject (0);
  this->OriginalInput = vtkDataObject::SafeDownCast (
    inInfo->Get(vtkDataObject::DATA_OBJECT ()));

  // is this the first request
  if (this->CurrentTimeIndex == 0 && this->WriteAllTimeSteps)
    {
    // Tell the pipeline to start looping.
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    }

  this->WriteData ();

  this->CurrentTimeIndex ++;
  if (this->CurrentTimeIndex >= this->NumberOfTimeSteps)
    {
    this->CloseExodusFile ();
    this->CurrentTimeIndex = 0;
    if (this->WriteAllTimeSteps)
      {
      // Tell the pipeline to start looping.
      request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 0);
      }
    }
  // still close out the file after each step written.
  if (!this->WriteAllTimeSteps)
    {
    this->CloseExodusFile ();
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkExodusIIWriter::WriteData ()
{
  this->NewFlattenedInput.clear ();
  // Is it safe to assume this is the same?
  bool newHierarchy = false;
  if (!this->FlattenHierarchy (this->OriginalInput, newHierarchy))
    {
    vtkErrorMacro (
      "vtkExodusIIWriter::WriteData Unable to flatten hierarchy");
    return;
    }
  if (this->FlattenedInput.size () != this->NewFlattenedInput.size ())
    {
    newHierarchy = true;
    }

  // Copies over the new results data in the new objects
  this->FlattenedInput = this->NewFlattenedInput;

  this->RemoveGhostCells ();

  // move check parameters up here and then if there's a change, new file.
  if (this->WriteAllTimeSteps && !newHierarchy)
    {
    if (!this->WriteNextTimeStep ())
      {
      vtkErrorMacro("vtkExodusIIWriter::WriteData results");
      }
    return;
    }
  else
    {
    // Close out the old file, if we have one
    if (this->CurrentTimeIndex > 0)
      {
      this->CloseExodusFile ();
      }


    // The file has changed, initialize new file
    if (!this->CheckParameters ())
      {
      return;
      }

    // TODO this should increment a counter
    if (!this->CreateNewExodusFile ())
      {
      vtkErrorMacro (
        "vtkExodusIIWriter: WriteData can't create exodus file");
      return;
      }

    if (!this->WriteInitializationParameters ())
      {
      vtkErrorMacro(<< "vtkExodusIIWriter::WriteData init params");
      return;
      }

    if (!this->WriteQARecords ())
      {
      vtkErrorMacro("vtkExodusIIWriter::WriteData QA records");
      return;
      }

    if (!this->WriteInformationRecords())
      {
      vtkErrorMacro("vtkExodusIIWriter::WriteData information records");
      return;
      }

    if (!this->WritePoints())
      {
      vtkErrorMacro("vtkExodusIIWriter::WriteData points");
      return;
      }

    if (!this->WriteCoordinateNames())
      {
      vtkErrorMacro("vtkExodusIIWriter::WriteData coordinate names");
      return;
      }

    if (!this->WriteGlobalPointIds())
      {
      vtkErrorMacro("vtkExodusIIWriter::WriteData global point IDs");
      return;
      }

    if (!this->WriteBlockInformation())
      {
      vtkErrorMacro("vtkExodusIIWriter::WriteData block information");
      return;
      }

    if (!this->WriteGlobalElementIds())
      {
      vtkErrorMacro("vtkExodusIIWriter::WriteData global element IDs");
      return;
      }

    if (!this->WriteVariableArrayNames())
      {
      vtkErrorMacro("vtkExodusIIWriter::WriteData variable array names");
      return;
      }

    if (!this->WriteNodeSetInformation())
      {
      vtkErrorMacro("vtkExodusIIWriter::WriteData can't node sets");
      return;
      }

    if (!this->WriteSideSetInformation())
      {
      vtkErrorMacro("vtkExodusIIWriter::WriteData can't side sets");
      return;
      }

    if (!this->WriteProperties())
      {
      vtkErrorMacro("vtkExodusIIWriter::WriteData can't properties");
      return;
      }

    if (!this->WriteNextTimeStep ())
      {
      vtkErrorMacro("vtkExodusIIWriter::WriteData results");
      return;
      }
    }
}

//----------------------------------------------------------------------------
char *vtkExodusIIWriter::StrDupWithNew(const char *s)
{
  char *newstr = NULL;

  if (s)
    {
    size_t len = strlen(s);
    newstr = new char [len + 1];
    strcpy(newstr, s);
    }

  return newstr;
}

//----------------------------------------------------------------------------
void vtkExodusIIWriter::StringUppercase(std::string& str)
{
  for (size_t i=0; i<str.size (); i++)
    {
    str[i] = toupper(str[i]);
    }
}

//----------------------------------------------------------------------------
int vtkExodusIIWriter::FlattenHierarchy (vtkDataObject* input, bool& changed)
{
  if (input->IsA ("vtkCompositeDataSet"))
    {
    vtkCompositeDataSet* castObj = vtkCompositeDataSet::SafeDownCast(input);
    vtkCompositeDataIterator* iter = castObj->NewIterator ();
    for (iter->InitTraversal ();
         !iter->IsDoneWithTraversal ();
         iter->GoToNextItem ())
      {
      if (!this->FlattenHierarchy (iter->GetCurrentDataObject (), changed))
        {
        return 0;
        }
      }
    iter->Delete ();
    }
  else if (input->IsA ("vtkDataSet"))
    {
    vtkSmartPointer<vtkUnstructuredGrid> output =
        vtkSmartPointer<vtkUnstructuredGrid>::New ();
    if (input->IsA ("vtkUnstructuredGrid"))
      {
      output->ShallowCopy (input);
      }
    else
      {
      vtkDataSet* castObj = vtkDataSet::SafeDownCast (input);

      output->GetPointData ()->ShallowCopy (castObj->GetPointData ());
      output->GetCellData ()->ShallowCopy (castObj->GetCellData ());

      vtkIdType numPoints = castObj->GetNumberOfPoints ();
      vtkSmartPointer<vtkPoints> outPoints = vtkSmartPointer<vtkPoints>::New ();
      outPoints->SetNumberOfPoints (numPoints);
      for (vtkIdType i = 0; i < numPoints; i ++)
        {
        outPoints->SetPoint (i, castObj->GetPoint (i));
        }
      output->SetPoints (outPoints);

      int numCells = castObj->GetNumberOfCells ();
      output->Allocate (numCells);
      vtkIdList* ptIds = vtkIdList::New ();
      for (int i = 0; i < numCells; i ++)
        {
        castObj->GetCellPoints (i, ptIds);
        output->InsertNextCell (castObj->GetCellType (i), ptIds);
        }
      ptIds->Delete ();
      }
    // check to see if we need a new exodus file
    // because the element or node count needs to be changed
    size_t checkIndex = this->NewFlattenedInput.size ();
    if (this->FlattenedInput.size () > checkIndex)
      {
      int numPoints = this->FlattenedInput[checkIndex]->GetNumberOfPoints ();
      int numCells = this->FlattenedInput[checkIndex]->GetNumberOfCells ();
      if (numPoints != output->GetNumberOfPoints () ||
          numCells != output->GetNumberOfCells ())
        {
        changed = true;
        }
      }
    else
      {
      changed = true;
      }
    this->NewFlattenedInput.push_back (output);
    }
  else
    {
    vtkErrorMacro (<< "Incorrect class type " << input->GetClassName () << " on input");
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkExodusIIWriter::CreateNewExodusFile()
{
  int compWordSize= (this->PassDoubles ? sizeof(double) : sizeof(float));
  int IOWordSize = (this->StoreDoubles ? sizeof(double) : sizeof(float));


  if (this->NumberOfProcesses == 1)
    {
    if (this->CurrentTimeIndex == 0)
      {
      this->fid = ex_create(this->FileName, EX_CLOBBER, &compWordSize, &IOWordSize);
      if (fid <= 0)
        {
        vtkErrorMacro (
          << "vtkExodusIIWriter: CreateNewExodusFile can't create "
          << this->FileName);
        }
      }
    else
      {
      char *myFileName = new char [1024];
      sprintf(myFileName, "%s_%06d", this->FileName, this->CurrentTimeIndex);
      this->fid = ex_create(myFileName, EX_CLOBBER, &compWordSize, &IOWordSize);
      if (fid <= 0)
        {
        vtkErrorMacro (
          << "vtkExodusIIWriter: CreateNewExodusFile can't create "
          << myFileName);
        }
      delete [] myFileName;
      }
    }
  else
    {
    char *myFileName = new char [1024];
    if (this->CurrentTimeIndex == 0)
      {
      sprintf(myFileName, "%s.%d.%d", this->FileName, this->NumberOfProcesses, this->MyRank);
      }
    else
      {
      sprintf(myFileName, "%s_%06d.%d.%d",
          this->FileName, this->CurrentTimeIndex, this->NumberOfProcesses, this->MyRank);
      }
    this->fid = ex_create(myFileName, EX_CLOBBER, &compWordSize, &IOWordSize);
    if (fid <= 0)
      {
      vtkErrorMacro (
        << "vtkExodusIIWriter: CreateNewExodusFile can't create "
        << myFileName);
      }
    delete [] myFileName;
    }

  // FileTimeOffset makes the time in the file relative
  // e.g., if the CurrentTimeIndex for this file is 4 and goes through 6, the
  // file will write them as 0 1 2 instead of 4 5 6
  this->FileTimeOffset = this->CurrentTimeIndex;
  return this->fid > 0;
}
//
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

//----------------------------------------------------------------------------
int vtkExodusIIWriter::IsDouble ()
{
  // Determine whether we should pass single or double precision
  // floats to the Exodus Library.  We'll look through the arrays
  // and points in the input and pick the precision of the
  // first float we see.

  for (size_t i = 0; i < this->FlattenedInput.size (); i ++)
    {

    vtkCellData *cd = this->FlattenedInput[i]->GetCellData();
    if (cd)
      {
      int numCellArrays = cd->GetNumberOfArrays();
      for (int j=0; j<numCellArrays; j++)
        {
        vtkDataArray *a = cd->GetArray(j);
        int type = a->GetDataType();

        if (type == VTK_DOUBLE)
          {
          return 1;
          }
        else if (type == VTK_FLOAT)
          {
          return 0;
          }
        }
      }

    vtkPointData *pd = this->FlattenedInput[i]->GetPointData();
    if (pd)
      {
      int numPtArrays = pd->GetNumberOfArrays();
      for (int j=0; j<numPtArrays; j++)
        {
        vtkDataArray *a = pd->GetArray(j);
        int type = a->GetDataType();

        if (type == VTK_DOUBLE)
          {
          return 1;
          }
        else if (type == VTK_FLOAT)
          {
          return 0;
          }
        }
      }

    }
  return -1;
}

//----------------------------------------------------------------------------
void vtkExodusIIWriter::RemoveGhostCells()
{
  for (size_t i = 0; i < this->FlattenedInput.size (); i ++)
    {
    vtkDataArray *da = this->FlattenedInput[i]->GetCellData()->GetArray("vtkGhostLevels");

    if (da)
      {
      vtkThreshold *t = vtkThreshold::New();
      t->SetInputData(this->FlattenedInput[i]);
      t->ThresholdByLower(0);
      t->SetInputArrayToProcess(
        0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "vtkGhostLevels");

      t->Update();

      this->FlattenedInput[i] = vtkSmartPointer<vtkUnstructuredGrid>(t->GetOutput());
      t->Delete();

      this->FlattenedInput[i]->GetCellData()->RemoveArray("vtkGhostLevels");
      this->FlattenedInput[i]->GetPointData()->RemoveArray("vtkGhostLevels");

      this->GhostLevel = 1;
      }
    else
      {
      this->GhostLevel = 0;
      }

    }
}

//----------------------------------------------------------------------------
int vtkExodusIIWriter::CheckParametersInternal (int _NumberOfProcesses, int _MyRank)
 {
   if (!this->FileName)
     {
     vtkErrorMacro("No filename specified.");
     return 0;
     }

   this->PassDoubles = this->IsDouble ();
   if (this->PassDoubles < 0)
     {
     // Can't find float types in input, assume doubles
     this->PassDoubles = 1;
     }

   if (this->StoreDoubles < 0)
     {
     // The default is to store in the
     // same precision that appears in the input.

     this->StoreDoubles = this->PassDoubles;
     }

   this->NumberOfProcesses = _NumberOfProcesses;
   this->MyRank = _MyRank;

  if (!this->CheckInputArrays ())
    {
    return 0;
    }

  if (!this->ConstructBlockInfoMap ())
    {
    return 0;
    }

  if (!this->ConstructVariableInfoMaps ())
    {
    return 0;
    }

  if (!this->GetModelMetadata())
    {
    /* TODO recover packed meta data
    if (vtkModelMetadata::HasMetadata ())
      {
      // All the metadata has been packed into field arrays of the ugrid,
      // probably by the vtkExodusReader or vtkPExodusReader.

      vtkModelMetadata *mmd = vtkModelMetadata::New();
      mmd->Unpack(this->FlattenedInput[i], 1);

      this->SetModelMetadata(mmd);
      mmd->Delete();
      }
    else if (!this->CreateExodusModel())  // use sensible defaults
      {
      return 0;
      }
    */
    if (!this->CreateDefaultMetadata ())
      {
      return 0;
      }
    }

  if (!this->ParseMetadata ())
    {
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkExodusIIWriter::CheckParameters ()
{
  return this->CheckParametersInternal(1, 0);
}

int vtkExodusIIWriter::CheckInputArrays ()
{
  this->BlockIdList.resize (this->FlattenedInput.size ());
  this->GlobalElementIdList.resize (this->FlattenedInput.size ());
  this->AtLeastOneGlobalElementIdList = 0;
  this->GlobalNodeIdList.resize (this->FlattenedInput.size ());
  this->AtLeastOneGlobalNodeIdList = 0;

  this->NumPoints = 0;
  this->NumCells = 0;
  this->MaxId = 0;

  size_t i;
  for (i = 0; i < this->FlattenedInput.size (); i ++)
    {
    this->NumPoints += this->FlattenedInput[i]->GetNumberOfPoints ();
    int numCells = this->FlattenedInput[i]->GetNumberOfCells ();
    this->NumCells += numCells;

    vtkCellData *cd = this->FlattenedInput[i]->GetCellData();
    vtkPointData *pd = this->FlattenedInput[i]->GetPointData();

    // Trying to find block id
    vtkDataArray *da = 0;
    if (this->BlockIdArrayName)
      {
      da = cd->GetArray(this->BlockIdArrayName);
      }
    if (!da)
      {
      da = cd->GetArray("ObjectId");
      if (da)
        {
        this->SetBlockIdArrayName("ObjectId");
        }
      else
        {
        da = cd->GetArray("ElementBlockIds");
        if (da)
          {
          this->SetBlockIdArrayName("ElementBlockIds");
          }
        else
          {
          this->SetBlockIdArrayName(0);
          if ((this->NumberOfProcesses > 1))
            {
            // Parallel apps must have a global list of all block IDs, plus a
            // list of block IDs for each cell.
            vtkWarningMacro(<< "Attempting to proceed without metadata");
            }
          }
        }
      }

    if (da)
      {
      vtkIntArray *ia = vtkIntArray::SafeDownCast(da);
      if (!ia)
        {
        vtkErrorMacro(<< "vtkExodusIIWriter, block ID array is not an integer array");
        return 1;
        }
      this->BlockIdList[i] = ia;
      this->BlockIdList[i]->Register (this);
      // computing the max known id in order to create unique fill in values below
      for (int j = 0; j < numCells; j ++)
        {
        if (this->BlockIdList[i]->GetValue (j) > MaxId)
          {
          MaxId = this->BlockIdList[i]->GetValue (j);
          }
        }
      }
    else
      {
      // Will fill in below
      this->BlockIdList[i] = 0;
      }

    // Trying to find global element id
    da = cd->GetGlobalIds();
    if (this->WriteOutGlobalElementIdArray && da)
      {
      vtkIdTypeArray *ia = vtkIdTypeArray::SafeDownCast(da);
      if (!ia)
        {
        vtkWarningMacro(<<
          "vtkExodusIIWriter, element ID array is not an Id array, ignoring it");
        this->GlobalElementIdList[i] = NULL;
        }
      else
        {
        this->GlobalElementIdList[i] = ia->GetPointer(0);
        this->AtLeastOneGlobalElementIdList = 1;
        }
      }

    // Trying to find global node id
    da = pd->GetGlobalIds();
    if (da)
      {
      vtkIdTypeArray *ia = vtkIdTypeArray::SafeDownCast(da);
      if (!ia)
        {
        vtkWarningMacro(<<
          "vtkExodusIIWriter, node ID array is not an Id array, ignoring it");
        this->GlobalNodeIdList[i] = 0;
        }
      else
        {
        this->GlobalNodeIdList[i] = ia->GetPointer(0);
        this->AtLeastOneGlobalNodeIdList = 1;
        }
      }
    else
      {
      this->GlobalNodeIdList[i] = 0;
      }
    }

  return 1;
}

int vtkExodusIIWriter::ConstructBlockInfoMap ()
{
  // The elements in the input may not be in order by block, but we must
  // write element IDs and element variables out to the Exodus file in
  // order by block.  Create a mapping if necessary, for an ordering by
  // block to the ordering found in the input unstructured grid.
  this->CellToElementOffset.resize (this->FlattenedInput.size ());
  this->BlockInfoMap.clear ();
  for (size_t i = 0; i < this->FlattenedInput.size (); i ++)
    {
    // If we weren't explicitly given the block ids, try to extract them from the
    // block id array embedded in the cell data.
    vtkIdType ncells = this->FlattenedInput[i]->GetNumberOfCells();
    if (!this->BlockIdList[i])
      {
      vtkIntArray *ia = vtkIntArray::New ();
      ia->SetNumberOfValues (ncells);
      for (int j = 0; j < ncells; j ++)
        {
        ia->SetValue (j, this->FlattenedInput[i]->GetCellType(j) + MaxId);
        }

      // Pretend we had it in the metadata
      this->BlockIdList[i] = ia;
      this->BlockIdList[i]->Register (this);
      ia->Delete ();

      // Also increment the MaxId so we can keep it unique
      MaxId += VTK_NUMBER_OF_CELL_TYPES;
      }

    // Compute all the block information mappings.
    this->CellToElementOffset[i].resize (ncells);
    for(int j=0; j<ncells; j++)
      {
      std::map<int, Block>::iterator iter =
        this->BlockInfoMap.find (this->BlockIdList[i]->GetValue (j));
                                 // shift by 1 in case there's a 0  This was removed because it changes the user supplied block ids in the metadata.
      if (iter == this->BlockInfoMap.end ())
        {
        this->CellToElementOffset[i][j] = 0;
        Block& b = this->BlockInfoMap[this->BlockIdList[i]->GetValue (j)];//CLM
        b.Type = this->FlattenedInput[i]->GetCellType (j);
        b.NumElements = 1;
        b.ElementStartIndex = 0;
        switch (b.Type)
          {
          case VTK_POLY_LINE:
          case VTK_POLYGON:
          case VTK_POLYHEDRON:
            // this block contains variable numbers of nodes per element
            b.NodesPerElement = 0;
            b.EntityCounts = std::vector<int>(ncells);
            b.EntityCounts[0] = this->FlattenedInput[i]->GetCell (j)->GetNumberOfPoints ();
            b.EntityNodeOffsets = std::vector<int>(ncells);
            b.EntityNodeOffsets[0] = 0;
            break;
          default:
            b.NodesPerElement = this->FlattenedInput[i]->GetCell (j)->GetNumberOfPoints ();
          };

        // TODO this could be a push if i is different.
        b.GridIndex = i;

        // This may get pulled from the meta data below,
        // but if not, default reasonably to 0
        b.NumAttributes = 0;
        b.BlockAttributes = 0;
        }
      else
        {
        // TODO we should be able to deal with this, not just error out
        if (iter->second.GridIndex != i)
          {
          vtkWarningMacro ("Block ids are not unique across the hierarchy ");
          }

        this->CellToElementOffset[i][j] = iter->second.NumElements;
        int index = iter->second.NumElements;
        if (iter->second.NodesPerElement == 0)
          {
          iter->second.EntityCounts[index] =
                  this->FlattenedInput[i]->GetCell (j)->GetNumberOfPoints ();
          iter->second.EntityNodeOffsets[index] =
                  iter->second.EntityNodeOffsets[index - 1] +
                  iter->second.EntityCounts[index - 1];
          }
        iter->second.NumElements ++;
        }
      }
    }

  this->CheckBlockInfoMap();

  // Find the ElementStartIndex and the output order
  std::map<int, Block>::iterator iter;
  int runningCount = 0;
  int index = 0;
  for (iter = this->BlockInfoMap.begin ();
       iter != this->BlockInfoMap.end ();
       iter ++)
    {
    iter->second.ElementStartIndex = runningCount;
    runningCount += iter->second.NumElements;

    iter->second.OutputIndex = index;
    index ++;
    }

  return 1;
}

int vtkExodusIIWriter::ConstructVariableInfoMaps ()
{
  // Create the variable info maps.
  this->NumberOfScalarGlobalArrays = 0;
  this->NumberOfScalarElementArrays = 0;
  this->NumberOfScalarNodeArrays = 0;
  this->BlockVariableMap.clear ();
  this->NodeVariableMap.clear ();
  for (size_t i = 0; i < this->FlattenedInput.size (); i ++)
    {
    vtkFieldData *fd = this->FlattenedInput[i]->GetFieldData ();
    for (int j = 0; j < fd->GetNumberOfArrays (); j ++)
      {
      char *name = 0;
      if (fd->GetArray(j))
        {
        name = fd->GetArray(j)->GetName();
        }
      if (name == 0)
        {
        vtkWarningMacro ("Array in input field data has Null name, cannot output it");
        continue;
        }
      std::string upper (name);
      this->StringUppercase(upper);
      if (strncmp(upper.c_str (),"QA_RECORD",9) == 0)
        {
        continue;
        }
      if (strncmp(upper.c_str (),"INFO_RECORD",11) == 0)
        {
        continue;
        }
      if (strncmp(upper.c_str (),"ELEMENTBLOCKIDS",15) == 0)
        {
        continue;
        }
      int numComp = fd->GetArray(j)->GetNumberOfComponents ();
      std::map<std::string, VariableInfo>::const_iterator iter =
        this->GlobalVariableMap.find (name);
      if (iter == this->GlobalVariableMap.end ())
        {
        VariableInfo &info = this->GlobalVariableMap[name];
        info.NumComponents = numComp;
        info.OutNames.resize (info.NumComponents);
        info.ScalarOutOffset = this->NumberOfScalarGlobalArrays;
        this->NumberOfScalarGlobalArrays += numComp;
        }
      else if (iter->second.NumComponents != numComp)
        {
        vtkErrorMacro (
          "Disagreement in the hierarchy for the number of components in " <<
          name);
        return 0;
        }
      }

    vtkCellData *cd = this->FlattenedInput[i]->GetCellData();
    for (int j = 0; j < cd->GetNumberOfArrays(); j ++)
      {
      char *name = 0;
      if (cd->GetArray(j))
        {
        name = cd->GetArray(j)->GetName();
        }
      if (name == 0)
        {
        vtkWarningMacro ("Array in input cell data has Null name, cannot output it");
        continue;
        }
      std::string upper (name);
      this->StringUppercase(upper);

      if (!this->WriteOutGlobalElementIdArray &&
          cd->IsArrayAnAttribute(j) == vtkDataSetAttributes::GLOBALIDS)
        {
        continue;
        }
      if (!this->WriteOutBlockIdArray &&
          this->BlockIdArrayName && strcmp (name, this->BlockIdArrayName) == 0)
        {
        continue;
        }
      if (strncmp(upper.c_str (),"PEDIGREE",8) == 0)
        {
        continue;
        }

      int numComp = cd->GetArray(j)->GetNumberOfComponents ();
      std::map<std::string, VariableInfo>::const_iterator iter =
        this->BlockVariableMap.find (name);
      if (iter == this->BlockVariableMap.end ())
        {
        VariableInfo &info = this->BlockVariableMap[name];
        info.NumComponents = numComp;
        info.OutNames.resize (info.NumComponents);
        info.ScalarOutOffset = this->NumberOfScalarElementArrays;
        this->NumberOfScalarElementArrays += numComp;
        }
      else if (iter->second.NumComponents != numComp)
        {
        vtkErrorMacro (
          "Disagreement in the hierarchy for the number of components in " <<
          name);
        return 0;
        }
      }

    vtkPointData *pd = this->FlattenedInput[i]->GetPointData();
    for (int j = 0; j < pd->GetNumberOfArrays(); j ++)
      {
      char *name = 0;
      if (pd->GetArray(j))
        {
        name = pd->GetArray(j)->GetName();
        }
      if (name == 0)
        {
        vtkWarningMacro ("Array in input point data has Null name, cannot output it");
        continue;
        }
      std::string upper (name);
      this->StringUppercase(upper);

      // Is this array displacement?
      // If it is and we are not writing all the timesteps,
      // do not write out. It would mess up the geometry the
      // next time the file was read in.
      if (!this->WriteOutGlobalNodeIdArray &&
          pd->IsArrayAnAttribute(j) == vtkDataSetAttributes::GLOBALIDS)
        {
        continue;
        }
      if (strncmp(upper.c_str (),"PEDIGREE",8) == 0)
        {
        continue;
        }
      // Is this array displacement?
      // If it is and we are not writing all the timesteps,
      // do not write out. It would mess up the geometry the
      // next time the file was read in.
      if (!this->WriteAllTimeSteps && strncmp(upper.c_str (),"DIS",3) == 0)
        {
        continue;
        }

      int numComp = pd->GetArray(j)->GetNumberOfComponents ();
      std::map<std::string, VariableInfo>::const_iterator iter =
        this->NodeVariableMap.find (name);
      if (iter == this->NodeVariableMap.end ())
        {
        VariableInfo &info = this->NodeVariableMap[name];
        info.NumComponents = numComp;
        info.OutNames.resize (info.NumComponents);
        info.ScalarOutOffset = this->NumberOfScalarNodeArrays;
        this->NumberOfScalarNodeArrays += info.NumComponents;
        }
      else if (iter->second.NumComponents != numComp)
        {
        vtkErrorMacro (
          "Disagreement in the hierarchy for the number of components in " <<
          name);
        return 0;
        }
      }
    }

  // BLOCK/ELEMENT TRUTH TABLE
  size_t ttsize = this->BlockInfoMap.size () * this->NumberOfScalarElementArrays;
  if (ttsize > 0)
    {
    this->BlockElementVariableTruthTable = new int [ttsize];
    int index = 0;
    std::map<int, Block>::const_iterator blockIter;
    for (blockIter = this->BlockInfoMap.begin ();
         blockIter != this->BlockInfoMap.end ();
         blockIter ++)
      {
      std::map<std::string, VariableInfo>::const_iterator varIter;
      for (varIter = this->BlockVariableMap.begin ();
           varIter != this->BlockVariableMap.end ();
           varIter ++)
        {
        vtkCellData *cd =
          this->FlattenedInput[blockIter->second.GridIndex]->GetCellData();
        int truth = 0;
        if (cd)
          {
          truth = 1;
          }
        for (int c = 0; c < varIter->second.NumComponents; c ++)
          {
            this->BlockElementVariableTruthTable[index++] = truth;
          }
        }
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkExodusIIWriter::CreateDefaultMetadata ()
{
  // There is no metadata associated with this input.  If we have enough
  // information, we create reasonable defaults.

  vtkModelMetadata *em = vtkModelMetadata::New();

  char *title = new char [MAX_LINE_LENGTH + 1];
  time_t currentTime = time(NULL);
  struct tm *td = localtime(&currentTime);
  char *stime = asctime(td);

  sprintf(title, "Created by vtkExodusIIWriter, %s", stime);

  em->SetTitle(title);

  delete [] title;

  char **dimNames = new char * [3];
  dimNames[0] = vtkExodusIIWriter::StrDupWithNew("X");
  dimNames[1] = vtkExodusIIWriter::StrDupWithNew("Y");
  dimNames[2] = vtkExodusIIWriter::StrDupWithNew("Z");
  em->SetCoordinateNames(3, dimNames);

  if (!this->CreateBlockIdMetadata(em))
    {
    return 0;
    }

  if (!this->CreateBlockVariableMetadata(em))
    {
    return 0;
    }
  this->SetModelMetadata(em);
  em->Delete();

  return 1;
}

//----------------------------------------------------------------------------
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
      strcpy(nm, "sphere");
      break;
    case VTK_POLY_VERTEX:
      strcpy(nm, "sup");
      break;
    case VTK_LINE:
      strcpy(nm, "edge");
      break;
    case VTK_POLY_LINE:
      strcpy(nm, "NSIDED");
      break;
    case VTK_TRIANGLE:
      strcpy(nm, "TRIANGLE");
      break;
    case VTK_TRIANGLE_STRIP:
      strcpy(nm, "TRIANGLE");
      break;
    case VTK_POLYGON:
      strcpy(nm, "NSIDED");
      break;
    case VTK_POLYHEDRON:
      strcpy(nm, "NFACED");
      break;
    case VTK_PIXEL:
      strcpy(nm, "sphere");
      break;
    case VTK_QUAD:
      strcpy(nm, "quad");
      break;
    case VTK_TETRA:
      strcpy(nm, "TETRA");
      break;
    case VTK_VOXEL :
      strcpy(nm, "HEX");
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
      strcpy(nm, "edge");
      break;
    case VTK_QUADRATIC_TRIANGLE:
      strcpy(nm, "triangle");
      break;
    case VTK_QUADRATIC_QUAD:
      strcpy(nm, "quad");
      break;
    case VTK_QUADRATIC_TETRA:
      strcpy(nm, "tetra");
      break;
    case VTK_QUADRATIC_HEXAHEDRON:
      strcpy(nm, "hexahedron");
      break;
    case VTK_QUADRATIC_WEDGE:
      strcpy(nm, "wedge");
      break;
    case VTK_QUADRATIC_PYRAMID:
      strcpy(nm, "pyramid");
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

//----------------------------------------------------------------------------
int vtkExodusIIWriter::CreateBlockIdMetadata(vtkModelMetadata *em)
{
  // vtkModelMetadata frees the memory when its done so we need to create a copy
  size_t nblocks = this->BlockInfoMap.size ();
  if (nblocks < 1) return 1;
  em->SetNumberOfBlocks(static_cast<int>(nblocks));

  int *blockIds = new int [nblocks];
  char **blockNames = new char * [nblocks];
  int *numElements = new int [nblocks];
  int *numNodesPerElement = new int [nblocks];
  int *numAttributes = new int [nblocks];

  std::map<int, Block>::const_iterator iter;
  for(iter = this->BlockInfoMap.begin();
      iter != this->BlockInfoMap.end();
      iter++)
    {
    int index = iter->second.OutputIndex;
    blockIds[index] = iter->first;
    blockNames[index] = vtkExodusIIWriter::GetCellTypeName (iter->second.Type);
    numElements[index] = iter->second.NumElements;
    numNodesPerElement[index] = iter->second.NodesPerElement;
    numAttributes[index] = 0;
    }
  em->SetBlockIds(blockIds);
  em->SetBlockElementType(blockNames);
  em->SetBlockNumberOfElements(numElements);
  em->SetBlockNodesPerElement(numNodesPerElement);
  em->SetBlockNumberOfAttributesPerElement(numAttributes);
  return 1;
}

//----------------------------------------------------------------------------
int vtkExodusIIWriter::CreateBlockVariableMetadata (vtkModelMetadata *em)
{
  size_t narrays = this->GlobalVariableMap.size ();
  char **flattenedNames = NULL;
  if (narrays > 0)
    {
    flattenedNames = vtkExodusIIWriter::FlattenOutVariableNames(
                   this->NumberOfScalarGlobalArrays, this->GlobalVariableMap);
    em->SetGlobalVariableNames (this->NumberOfScalarGlobalArrays, flattenedNames);
    }

  narrays = this->BlockVariableMap.size ();
  char **nms = NULL;
  if (narrays > 0)
    {
    nms = new char * [narrays];
    int *numComponents = new int [narrays];
    int *scalarIndex = new int [narrays];

    int index = 0;
    std::map<std::string, VariableInfo>::const_iterator var_iter;
    for (var_iter = this->BlockVariableMap.begin ();
         var_iter != this->BlockVariableMap.end ();
         var_iter ++)
      {
      nms[index] =
        vtkExodusIIWriter::StrDupWithNew (var_iter->first.c_str ());
      numComponents[index] = var_iter->second.NumComponents;
      scalarIndex[index] = var_iter->second.ScalarOutOffset;
      index ++;
      }

    flattenedNames = vtkExodusIIWriter::FlattenOutVariableNames(
                   this->NumberOfScalarElementArrays, this->BlockVariableMap);

    // these variables are now owned by em.  No deletion
    em->SetElementVariableInfo(this->NumberOfScalarElementArrays,
                  flattenedNames, static_cast<int>(narrays), nms,
                  numComponents, scalarIndex);
    }

  narrays = this->NodeVariableMap.size ();
  if (narrays > 0)
    {
    nms = new char * [narrays];
    int *numComponents = new int [narrays];
    int *scalarOutOffset = new int [narrays];

    int index = 0;
    std::map<std::string, VariableInfo>::const_iterator iter;
    for (iter = this->NodeVariableMap.begin ();
         iter != this->NodeVariableMap.end ();
         iter ++)
      {
      nms[index] =
            vtkExodusIIWriter::StrDupWithNew (iter->first.c_str ());
      numComponents[index] = iter->second.NumComponents;
      scalarOutOffset[index] = iter->second.ScalarOutOffset;
      index ++;
      }

      flattenedNames = vtkExodusIIWriter::FlattenOutVariableNames(
                    this->NumberOfScalarNodeArrays, this->NodeVariableMap);

      em->SetNodeVariableInfo(this->NumberOfScalarNodeArrays, flattenedNames,
          static_cast<int>(narrays), nms, numComponents, scalarOutOffset);
    }
  return 1;
}


//----------------------------------------------------------------------------
int vtkExodusIIWriter::ParseMetadata ()
{
  vtkModelMetadata* em = this->GetModelMetadata ();
  int nblocks = em->GetNumberOfBlocks ();
  int *ids = em->GetBlockIds();
  int *numAttributes = em->GetBlockNumberOfAttributesPerElement();
  float *att = em->GetBlockAttributes();
  int *attIdx = em->GetBlockAttributesIndex();
  // Extract the attribute data from the meta model.
  for (int n = 0; n < nblocks; n ++)
    {
    std::map<int, Block>::iterator iter = this->BlockInfoMap.find (ids[n]);
    if (iter == this->BlockInfoMap.end ())
      {
      vtkErrorMacro (<< "Unknown id " << ids[n] << " found in meta data");
      return 0;
      }
    iter->second.NumAttributes = numAttributes[n];
    iter->second.BlockAttributes = att + attIdx[n];
    }

  this->ConvertVariableNames (this->GlobalVariableMap);
  this->ConvertVariableNames (this->BlockVariableMap);
  this->ConvertVariableNames (this->NodeVariableMap);
  return 1;
}

//----------------------------------------------------------------------------
int vtkExodusIIWriter::WriteInitializationParameters()
{
  vtkModelMetadata *em = this->GetModelMetadata();

  int dim = em->GetDimension();
  int nnsets = em->GetNumberOfNodeSets();
  int nssets = em->GetNumberOfSideSets();
  const char *title = em->GetTitle();
  int numBlocks = em->GetNumberOfBlocks();
  int rc = ex_put_init(this->fid, title, dim, this->NumPoints, this->NumCells,
                       numBlocks, nnsets, nssets);
  return rc >= 0;
}
//
//---------------------------------------------------------
// Initialization, QA, Title, information records
//---------------------------------------------------------
int vtkExodusIIWriter::WriteQARecords()
{
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

  return 1;
}

//----------------------------------------------------------------------------
int vtkExodusIIWriter::WriteInformationRecords()
{

  vtkModelMetadata *em = this->GetModelMetadata();

  int nlines = em->GetNumberOfInformationLines();

  if (nlines > 0)
    {
    char **lines = NULL;

    em->GetInformationLines(&lines);

    ex_put_info(this->fid, nlines, lines);
    }

  return 1;
}

template<typename T>
int vtkExodusIIWriterWritePoints (
    std::vector< vtkSmartPointer<vtkUnstructuredGrid> > input,
    int numPoints, int fid)
{
    T *px = new T [numPoints];
    T *py = new T [numPoints];
    T *pz = new T [numPoints];

    int array_index = 0;
    for (size_t i = 0; i < input.size (); i ++)
      {
      vtkPoints *pts = input[i]->GetPoints ();
      if (pts)
        {
        int npts = pts->GetNumberOfPoints ();
        vtkDataArray *da = pts->GetData ();
        for (int j = 0; j < npts; j ++)
          {
          px[array_index] = da->GetComponent(j, 0);
          py[array_index] = da->GetComponent(j, 1);
          pz[array_index] = da->GetComponent(j, 2);
          array_index ++;
          }
        }
      }

    int rc = ex_put_coord(fid, px, py, pz);

    delete [] px;
    delete [] py;
    delete [] pz;

    return rc >= 0;
}

//----------------------------------------------------------------------------
int vtkExodusIIWriter::WritePoints()
{
  if (this->PassDoubles)
    {
    return vtkExodusIIWriterWritePoints<double>(
                    this->FlattenedInput, this->NumPoints, this->fid);
    }
  else
    {
    return vtkExodusIIWriterWritePoints<float>(
                    this->FlattenedInput, this->NumPoints, this->fid);
    }
}

//---------------------------------------------------------
// Points and point IDs, element IDs
//---------------------------------------------------------
int vtkExodusIIWriter::WriteCoordinateNames()
{
  vtkModelMetadata *em = this->GetModelMetadata();

  int rc = ex_put_coord_names(this->fid, em->GetCoordinateNames());

  return rc >= 0;
}

//----------------------------------------------------------------------------
int vtkExodusIIWriter::WriteGlobalPointIds()
{
  if (!this->AtLeastOneGlobalNodeIdList)
    {
    return 1;
    }
  int *copyOfIds = new int [this->NumPoints];
  int index = 0;
  for (size_t i = 0; i < this->FlattenedInput.size (); i ++)
    {
    vtkIdType npoints = this->FlattenedInput[i]->GetNumberOfPoints();

    vtkIdType *ids = this->GlobalNodeIdList[i];
    if (ids)
      {
      for (int j=0; j<npoints; j++)
        {
        copyOfIds[index++] = static_cast<int>(ids[j]);
        }
      }
    else
      {
      for (int j=0; j<npoints; j++)
        {
        copyOfIds[index++] = 0;
        }
      }
    }
  int rc = ex_put_node_num_map(this->fid, copyOfIds);
  delete [] copyOfIds;

  return rc >= 0;
}

//----------------------------------------------------------------------------
int vtkExodusIIWriter::WriteBlockInformation()
{
  int rc;

  std::map<int, Block>::const_iterator blockIter;
  size_t nblocks = this->BlockInfoMap.size ();

  std::vector<int*> connectivity (nblocks);

  // Use this to copy the attributes into if we need it.
  std::vector<double*> attributesD;
  if (this->PassDoubles)
    {
    attributesD.resize (nblocks);
    }

  // For each block, a map from element global ID to it's location
  // within it's block in the ExodusModel object.
  for (blockIter = this->BlockInfoMap.begin ();
       blockIter != this->BlockInfoMap.end ();
       blockIter ++)
    {
    int outputIndex = blockIter->second.OutputIndex;
    int numElts = blockIter->second.NumElements;
    int numAtts = blockIter->second.NumAttributes;
    int numNodes = blockIter->second.NodesPerElement;

    int numPoints;
    if (numNodes == 0)
      {
      numPoints = blockIter->second.EntityNodeOffsets[numElts - 1]
                + blockIter->second.EntityCounts[numElts - 1];
      }
    else
      {
      numPoints = numElts * numNodes;
      }

    if (numElts > 0)
      {
      connectivity[outputIndex] = new int [numPoints];

      if (numAtts > 0)
        {
        if (this->PassDoubles)
          {
          attributesD[outputIndex] = new double [numElts * numAtts];
          }
        }
      }
    else
      {
      connectivity[outputIndex] = 0;
      attributesD[outputIndex] = 0;
      }
    }

  // Prepare the pointers as flat arrays for Exodus
  // connectivity and attributes, if we need doubles.
  int pointOffset = 0;
  for (size_t i = 0; i < this->FlattenedInput.size (); i ++)
    {
    vtkCellArray *ca = this->FlattenedInput[i]->GetCells();
    vtkIdType *ptIds = 0;
    if (ca)
      {
      ptIds = ca->GetPointer ();
      }
    vtkIdTypeArray *loca = this->FlattenedInput[i]->GetCellLocationsArray();
    vtkIdType *loc = 0;
    if (loca)
      {
      loc = loca->GetPointer(0);
      }

    int ncells = this->FlattenedInput[i]->GetNumberOfCells();
    for (int j = 0; j < ncells; j++)
      {
      int blockId = this->BlockIdList[i]->GetValue(j);//CLM
      int blockOutIndex = this->BlockInfoMap[blockId].OutputIndex;

      int nodesPerElement = this->BlockInfoMap[blockId].NodesPerElement;
      vtkIdType elementOffset = this->CellToElementOffset[i][j];
      int offset;
      if (nodesPerElement == 0)
        {
        offset = this->BlockInfoMap[blockId].EntityNodeOffsets[j];
        }
      else
        {
        offset = elementOffset * nodesPerElement;
        }

      // the block connectivity array
      vtkIdType ptListIdx = loc[j];
      vtkIdType npts = ptIds[ptListIdx++];

      switch (this->FlattenedInput[i]->GetCellType (j))
        {
        case VTK_VOXEL: // reorder to exodus HEX type
          connectivity[blockOutIndex][offset + 0] = pointOffset + (int) ptIds[ptListIdx++] + 1;
          connectivity[blockOutIndex][offset + 1] = pointOffset + (int) ptIds[ptListIdx++] + 1;
          connectivity[blockOutIndex][offset + 3] = pointOffset + (int) ptIds[ptListIdx++] + 1;
          connectivity[blockOutIndex][offset + 2] = pointOffset + (int) ptIds[ptListIdx++] + 1;
          connectivity[blockOutIndex][offset + 4] = pointOffset + (int) ptIds[ptListIdx++] + 1;
          connectivity[blockOutIndex][offset + 5] = pointOffset + (int) ptIds[ptListIdx++] + 1;
          connectivity[blockOutIndex][offset + 7] = pointOffset + (int) ptIds[ptListIdx++] + 1;
          connectivity[blockOutIndex][offset + 6] = pointOffset + (int) ptIds[ptListIdx++] + 1;
          break;
        default:
          for (vtkIdType p=0; p<npts; p++)
            {
            int ExodusPointId = pointOffset + (int) ptIds[ptListIdx++] + 1;
            connectivity[blockOutIndex][offset + p] = ExodusPointId;
            }
        }

      // the block element attributes
      float *att = this->BlockInfoMap[blockId].BlockAttributes;

      int numAtts = this->BlockInfoMap[blockId].NumAttributes;

      if ((numAtts == 0) || (att == 0)) continue;

      int attOff = (elementOffset * numAtts); // location for the element in the block

      if (this->PassDoubles)
        {
        for (int k = 0; k <numAtts; k++)
          {
          int off = attOff + k;
          // TODO verify the assumption that ModelMetadata stores
          // elements in the same order we do
          // Could probably use the global node id? but how global is global.
          attributesD[blockOutIndex][off] = static_cast<double>(att[off]);
          }
        }
      }
    pointOffset += this->FlattenedInput[i]->GetNumberOfPoints ();
    }

  // Now, finally, write out the block information
  int fail = 0;
  for (blockIter = this->BlockInfoMap.begin ();
       blockIter != this->BlockInfoMap.end ();
       blockIter ++)
    {
    char *name = vtkExodusIIWriter::GetCellTypeName (blockIter->second.Type);
    if (blockIter->second.NodesPerElement == 0)
      {
      int numElts = blockIter->second.NumElements;
      int numPoints = blockIter->second.EntityNodeOffsets[numElts - 1]
                    + blockIter->second.EntityCounts[numElts - 1];
      rc = ex_put_elem_block(this->fid, blockIter->first,
                  name,
                  blockIter->second.NumElements,
                  numPoints,
                  blockIter->second.NumAttributes);
      }
    else
      {
      rc = ex_put_elem_block(this->fid, blockIter->first,
                  name,
                  blockIter->second.NumElements,
                  blockIter->second.NodesPerElement,
                  blockIter->second.NumAttributes);
      }
    delete [] name;
    if (rc < 0)
      {
      vtkErrorMacro (<< "Problem adding block with id " << blockIter->first);
      continue;
      }

    if (blockIter->second.NumElements > 0)
      {
      rc = ex_put_elem_conn(this->fid, blockIter->first,
                      connectivity[blockIter->second.OutputIndex]);

      if (rc < 0)
        {
        vtkErrorMacro (<< "Problem writing connectivity " << blockIter->first);
        continue;
        }

      if (blockIter->second.NumAttributes != 0)
        {
        if (this->PassDoubles)
          {
          rc = ex_put_elem_attr(this->fid, blockIter->first,
                          attributesD[blockIter->second.OutputIndex]);
          }
        else
          {
          // TODO verify the assumption that ModelMetadata stores
          // elements in the same order we do
          rc = ex_put_elem_attr(this->fid, blockIter->first,
                            blockIter->second.BlockAttributes);
          }

        if (rc < 0)
          {
          continue;
          }
        }

      if (blockIter->second.NodesPerElement == 0)
        {
        rc = ex_put_entity_count_per_polyhedra (this->fid, EX_ELEM_BLOCK,
                     blockIter->first, &(blockIter->second.EntityCounts[0]));
        }
      }
    }

  for (size_t n = 0; n < nblocks; n ++)
    {
    if (connectivity[n])
      {
      delete [] connectivity[n];
      }
    if (this->PassDoubles && attributesD[n])
      {
      delete [] attributesD[n];
      }
    }
  return !fail;
}

//----------------------------------------------------------------------------
int vtkExodusIIWriter::WriteGlobalElementIds()
{
  int rc = 0;

  //if (sizeof(vtkIdType) != sizeof(int))
  //  {
  //  vtkErrorMacro(<<"vtkExodusIIWriter::WriteGlobalElementIds cannot convert vtkIdType to int.");
  //  return -1;
  //  }

  if (this->AtLeastOneGlobalElementIdList)
    {
    int *copyOfIds = new int [this->NumCells];
    memset (copyOfIds, 0, sizeof (int) * this->NumCells);
    for (size_t i = 0; i < this->FlattenedInput.size (); i ++)
      {
      vtkIdType *ids = this->GlobalElementIdList[i];
      if (ids)
        {
        int ncells = this->FlattenedInput[i]->GetNumberOfCells();
        for (int j=0; j<ncells; j++)
          {
          int blockId = this->BlockIdList[i]->GetValue (j);
          int start = this->BlockInfoMap[blockId].ElementStartIndex;
          vtkIdType offset = this->CellToElementOffset[i][j];
          copyOfIds[start + offset] = static_cast<int>(ids[j]);
          }
        }
      }
    rc = ex_put_elem_num_map(this->fid, copyOfIds);
    delete [] copyOfIds;
    }

  return rc >= 0;
}

//----------------------------------------------------------------------------
int vtkExodusIIWriter::WriteVariableArrayNames()
{
  int rc = 0;

  //  1. We convert vector arrays to individual scalar arrays, using
  //     their original names if we have those.
  //  2. For the element variables, create the element/block truth table.

  // GLOBAL VARIABLES
  const char **outputArrayNames;
  if (this->NumberOfScalarGlobalArrays > 0)
    {
    outputArrayNames = new const char * [this->NumberOfScalarGlobalArrays];
    std::map<std::string, VariableInfo>::const_iterator iter;
    for (iter = this->GlobalVariableMap.begin ();
         iter != this->GlobalVariableMap.end ();
         iter ++)
      {
      int off = iter->second.ScalarOutOffset;
      for (int j=0; j< iter->second.NumComponents; j++)
        {
        outputArrayNames[off + j] = iter->second.OutNames[j].c_str ();
        }
      }

    rc = ex_put_var_param(this->fid, "G", this->NumberOfScalarGlobalArrays);
    if (rc < 0)
      {
      vtkErrorMacro(<<
        "vtkExodusIIWriter::WriteVariableArrayNames cell variables");
      return 0;
      }

    rc = ex_put_var_names(this->fid, "G", this->NumberOfScalarGlobalArrays,
                          (char **)outputArrayNames);
                          // This should be treating this read only... hopefully
    if (rc < 0)
      {
      vtkErrorMacro(<<
        "vtkExodusIIWriter::WriteVariableArrayNames cell variables");
      return 0;
      }

    delete [] outputArrayNames;
    }

  // CELL (ELEMENT) VARIABLES
  if (this->NumberOfScalarElementArrays > 0 && this->NumCells > 0)
    {
    outputArrayNames = new const char * [this->NumberOfScalarElementArrays];

    std::map<std::string, VariableInfo>::const_iterator iter;
    for (iter = this->BlockVariableMap.begin ();
         iter != this->BlockVariableMap.end ();
         iter ++)
      {
      int off = iter->second.ScalarOutOffset;
      for (int j=0; j< iter->second.NumComponents; j++)
        {
        outputArrayNames[off + j] = iter->second.OutNames[j].c_str ();
        }
      }

    rc = ex_put_var_param(this->fid, "E", this->NumberOfScalarElementArrays);
    if (rc < 0)
      {
      vtkErrorMacro(<<
        "vtkExodusIIWriter::WriteVariableArrayNames cell variables");
      return 0;
      }

    rc = ex_put_var_names(this->fid, "E", this->NumberOfScalarElementArrays,
                          (char **)outputArrayNames);
                          // This should be treating this read only... hopefully
    if (rc < 0)
      {
      vtkErrorMacro(<<
        "vtkExodusIIWriter::WriteVariableArrayNames cell variables");
      return 0;
      }

    rc = ex_put_elem_var_tab(this->fid,
                             static_cast<int>(this->BlockInfoMap.size ()),
                             this->NumberOfScalarElementArrays,
                             this->BlockElementVariableTruthTable);
    if (rc < 0)
      {
      vtkErrorMacro(<<
        "vtkExodusIIWriter::WriteVariableArrayNames cell variables");
      return 0;
      }

    delete [] outputArrayNames;
    }

  // POINT (NODE) VARIABLES
  if (this->NumberOfScalarNodeArrays > 0 && this->NumPoints > 0)
    {
    outputArrayNames = new const char * [this->NumberOfScalarNodeArrays];

    std::map<std::string, VariableInfo>::const_iterator iter;
    for (iter = this->NodeVariableMap.begin ();
         iter != this->NodeVariableMap.end ();
         iter ++)
      {
      int off = iter->second.ScalarOutOffset;
      for (int j=0; j<iter->second.NumComponents; j++)
        {
        if (iter->second.OutNames[j].size () > MAX_STR_LENGTH)
          {
          outputArrayNames[off + j] =
            iter->second.OutNames[j].substr (0, MAX_STR_LENGTH - 1).c_str ();
          }
        else
          {
          outputArrayNames[off + j] = iter->second.OutNames[j].c_str ();
          }
        }
      }

    rc = ex_put_var_param(this->fid, "N", this->NumberOfScalarNodeArrays);

    if (rc < 0)
      {
      vtkErrorMacro(<<
        "vtkExodusIIWriter::WriteVariableArrayNames " <<
        "failure to write " << this->NumberOfScalarNodeArrays << " arrays");
      return 0;
      }

    rc = ex_put_var_names(this->fid, "N", this->NumberOfScalarNodeArrays,
                          (char **)outputArrayNames);
                          // This should not save references... hopefully
    if (rc < 0)
      {
      vtkErrorMacro(<<
        "vtkExodusIIWriter::WriteVariableArrayNames " <<
        "failure to write the array names");
      return 0;
      }

    delete [] outputArrayNames;
    }

  // GLOBAL VARIABLES
/*
  int ngvars = mmd->GetNumberOfGlobalVariables();

  if (ngvars > 0)
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
      return 0;
      }
    }
*/
  return 1;
}

//----------------------------------------------------------------------------
void vtkExodusIIWriter::ConvertVariableNames(
             std::map<std::string, VariableInfo>& variableMap)
{
  std::map<std::string, VariableInfo>::iterator varIter;
  // Global output variable names
  for (varIter = variableMap.begin ();
       varIter != variableMap.end ();
       varIter ++)
    {
    int numComp = varIter->second.NumComponents;
    if (numComp == 1)
      {
      varIter->second.OutNames[0] = std::string (varIter->first);
      }
    else
      {
      for (int component = 0; component < numComp; component ++)
        {
        varIter->second.OutNames[component] =
                CreateNameForScalarArray (varIter->first.c_str (),
                                          component,
                                          numComp);
        }
      }
    }
}

char **vtkExodusIIWriter::FlattenOutVariableNames(
             int nScalarArrays,
             const std::map<std::string, VariableInfo>& variableMap)
{
  char **newNames = new char * [nScalarArrays];

  std::map<std::string, VariableInfo>::const_iterator iter;
  for (iter = variableMap.begin ();
       iter != variableMap.end ();
       iter ++)
    {
    for (int component = 0; component < iter->second.NumComponents; component ++)
      {
      int index = iter->second.ScalarOutOffset + component;
      newNames[index] =
          StrDupWithNew (
              CreateNameForScalarArray(iter->first.c_str (),
                                       component,
                                       iter->second.NumComponents).c_str ());
      }
    }

  return newNames;
}

//----------------------------------------------------------------------------
std::string vtkExodusIIWriter::CreateNameForScalarArray(
  const char *root, int component, int numComponents)
{
  // Naming conventions chosen to match ExodusIIReader expectations
  if (component >= numComponents)
    {
    vtkErrorMacro ("CreateNameForScalarArray: Component out of range");
    return std::string ();
    }
  if (numComponents == 1)
    {
    return std::string (root);
    }
  else if (numComponents <= 2)
    {
    std::string s (root);
    // Adjust for Exodus' MAX_STR_LENGTH
    if (s.size () > MAX_STR_LENGTH - 2)
      {
      s = s.substr (0, MAX_STR_LENGTH - 3);
      }
    switch (component)
      {
      case 0:
        s.append ("_R");
        break;
      case 1:
        s.append ("_Z");
        break;
      }
    return s;
    }
  else if (numComponents <= 3)
    {
    std::string s (root);
    // Adjust for Exodus' MAX_STR_LENGTH
    if (s.size () > MAX_STR_LENGTH - 1)
      {
      s = s.substr (0, MAX_STR_LENGTH - 2);
      }
    switch (component)
      {
      case 0:
        s.append ("X");
        break;
      case 1:
        s.append ("Y");
        break;
      case 2:
        s.append ("Z");
        break;
      }
    return s;
    }
  else if (numComponents <= 6)
    {
    std::string s (root);
    // Adjust for Exodus' MAX_STR_LENGTH
    if (s.size () > MAX_STR_LENGTH - 2)
      {
      s = s.substr (0, MAX_STR_LENGTH - 3);
      }
    switch (component)
      {
      case 0:
        s.append ("XX");
        break;
      case 1:
        s.append ("XY");
        break;
      case 2:
        s.append ("XZ");
        break;
      case 3:
        s.append ("YY");
        break;
      case 4:
        s.append ("YZ");
        break;
      case 5:
        s.append ("ZZ");
        break;
      }
    return s;
    }
  else
    {
    std::string s (root);
    // Adjust for Exodus' MAX_STR_LENGTH
    if (s.size () > MAX_STR_LENGTH - 10)
      {
      s = s.substr (0, MAX_STR_LENGTH - 11);
      }
    // assume largest for 32 bit decimal representation
    char n[10];
    sprintf (n, "%10d", component);
    s.append (n);
    return s;
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkExodusIIWriter::GetNodeLocalId(vtkIdType id)
{
  if (!this->LocalNodeIdMap)
    {
    this->LocalNodeIdMap = new std::map<vtkIdType, vtkIdType>;
    vtkIdType index = 0;
    for (size_t i = 0; i < this->FlattenedInput.size (); i ++)
      {
      vtkIdType npoints = this->FlattenedInput[i]->GetNumberOfPoints();
      vtkIdType *ids = this->GlobalNodeIdList[i];
      if (ids)
        {
        for (int j=0; j<npoints; j++)
          {
          this->LocalNodeIdMap->insert(
            std::map<vtkIdType,vtkIdType>::value_type(ids[j], index));
          index ++;
          }
        }
      else
        {
        index += npoints;
        }
      }
    }

  std::map<vtkIdType,vtkIdType>::iterator mapit =
    this->LocalNodeIdMap->find(id);

  if (mapit == this->LocalNodeIdMap->end())
    {
    return -1;
    }
  else
    {
    return mapit->second;
    }
}

//-----------------------------------------------------------------------
// Side sets and node sets
//----------------------------------------------------------------------------
int vtkExodusIIWriter::WriteNodeSetInformation()
{
  int rc = 0;
  int i, j;

  vtkModelMetadata *em = this->GetModelMetadata();

  int nnsets = em->GetNumberOfNodeSets();

  if (nnsets < 1) return 1;

  int nids = em->GetSumNodesPerNodeSet();

  if (nids < 1 || !this->AtLeastOneGlobalNodeIdList)
    {
    int *buf = new int [nnsets];

    memset(buf, 0, sizeof(int) * nnsets);

    rc = ex_put_concat_node_sets(this->fid, em->GetNodeSetIds(),
              buf, buf, buf, buf, NULL, NULL);

    delete [] buf;

    return (rc >= 0);
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

  delete [] nsSize;
  delete [] nsNumDF;
  delete [] nsIdIdx;
  delete [] nsDFIdx;
  delete [] idBuf;
  if (dfBuf) delete [] dfBuf;
  else if (dfBufD) delete [] dfBufD;

  return (rc >= 0);
}

//----------------------------------------------------------------------------
vtkIdType vtkExodusIIWriter::GetElementLocalId(vtkIdType id)
{
  if (!this->LocalElementIdMap)
    {
    this->LocalElementIdMap = new std::map<vtkIdType, vtkIdType>;
    for (size_t i = 0; i < this->FlattenedInput.size (); i ++)
      {
      if (this->GlobalElementIdList[i])
        {
        vtkIdType ncells = this->FlattenedInput[i]->GetNumberOfCells();
        for (vtkIdType j=0; j<ncells; j++)
          {
          vtkIdType gid = this->GlobalElementIdList[i][j];
          int offset = this->CellToElementOffset[i][j];
          int start = this->BlockInfoMap[BlockIdList[i]->GetValue (j)].ElementStartIndex;
          this->LocalElementIdMap->insert(
            std::map<vtkIdType, vtkIdType>::value_type(gid, start + offset));
          }
        }
      }
    }

  std::map<vtkIdType,vtkIdType>::iterator mapit = this->LocalElementIdMap->find(id);
  if (mapit == this->LocalElementIdMap->end())
    {
      return -1;
    }
  else
    {
      return mapit->second;
    }
}

//-----------------------------------------------------------------------
int vtkExodusIIWriter::WriteSideSetInformation()
{
  int i, j, k;
  int rc= 0;

  vtkModelMetadata *em = this->GetModelMetadata();

  int nssets = em->GetNumberOfSideSets();

  if (nssets < 1) return 1;

  // Cells are written out to file in a different order than
  // they appear in the input. We need a mapping from their internal
  // id in the input to their internal id in the output.

  std::map<int, int>::iterator idIt;

  int nids = em->GetSumSidesPerSideSet();

  if (nids < 1)
    {
    int *buf = new int [nssets];

    memset(buf, 0, sizeof(int) * nssets);

    rc = ex_put_concat_side_sets(this->fid, em->GetSideSetIds(),
              buf, buf, buf, buf, NULL, NULL, NULL);

    delete [] buf;

    return (rc >= 0);
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

        idBuf[nextId] = lid+1;
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

  delete [] ssSize;
  delete [] ssNumDF;
  delete [] ssIdIdx;
  delete [] ssDFIdx;
  delete [] idBuf;
  delete [] sideBuf;
  if (dfBuf) delete [] dfBuf;
  else if (dfBufD) delete [] dfBufD;

  return rc >= 0;
}


//----------------------------------------------------------------------------
int vtkExodusIIWriter::BlockVariableTruthValue(int blockIdx, int varIdx)
{
  int tt=0;
  int nvars = this->NumberOfScalarElementArrays;
  int nblocks = static_cast<int>(this->BlockInfoMap.size ());

  if ( (blockIdx >= 0) && (blockIdx < nblocks) &&
       (varIdx >= 0) && (varIdx < nvars))
    {
    tt = this->BlockElementVariableTruthTable[(blockIdx * nvars) + varIdx];
    }
  else
    {
    vtkWarningMacro(<< "vtkExodusIIWriter::BlockVariableTruthValue invalid index");
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

    if (rc >= 0)
      {
      int *values = em->GetBlockPropertyValue();

      for (i=0; i<nbprop; i++)
        {
        rc = ex_put_prop_array(this->fid, EX_ELEM_BLOCK, names[i], values);
        if (rc) break;
        // TODO Handle the addition of Blocks not known by the metadata
        values += this->BlockInfoMap.size ();
        }
      }
    }

  if (!rc && nnsprop)
    {
    char **names = em->GetNodeSetPropertyNames();
    int nnsets = em->GetNumberOfNodeSets();

    // rc = ex_put_prop_names(this->fid, EX_NODE_SET, nnsprop, names);

    if (rc >= 0)
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

    if (rc >= 0)
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

  return (rc >= 0);
}

//========================================================================
//   VARIABLE ARRAYS:
//========================================================================
template <typename iterT>
double vtkExodusIIWriterGetComponent (iterT* it, vtkIdType ind)
{
  vtkVariant v(it->GetValue (ind));
  return v.ToDouble ();
}

//----------------------------------------------------------------------------
double vtkExodusIIWriter::ExtractGlobalData (const char *name, int comp, int ts)
{
  double ret = 0.0;
  for (size_t i = 0; i < this->FlattenedInput.size (); i ++)
    {
    // find the first block that matches this global data.  Assumes it's global.
    vtkDataArray *da = this->FlattenedInput[i]->GetFieldData ()->GetArray (name);
    if (da)
      {
      int numTup = da->GetNumberOfTuples ();
      if (numTup == 1)
        {
        ret = da->GetComponent (0, comp);
        }
      // Exodus doesn't support multiple tuples on the global values.
      // But the ExodusIIReader reads all timesteps into the field array
      // at every time step.  This will assume that if we have multiple tuples
      // in the array they are from an exodus file so we'll output them
      // back as expected on another read.  Not perfect...
      else if (ts < numTup)
        {
        ret = da->GetComponent (ts, comp);
        }
      }
    }
  return ret;
}

//----------------------------------------------------------------------------
void vtkExodusIIWriter::ExtractCellData (const char *name, int comp,
                                         vtkDataArray *buffer)
{
  buffer->SetNumberOfTuples (this->NumCells);
  for (size_t i = 0; i < this->FlattenedInput.size (); i ++)
    {
    vtkDataArray *da = this->FlattenedInput[i]->GetCellData ()->GetArray (name);
    int ncells = this->FlattenedInput[i]->GetNumberOfCells ();
    if (da)
      {
      vtkArrayIterator *arrayIter = da->NewIterator ();
      vtkIdType ncomp = da->GetNumberOfComponents ();
      for (vtkIdType j = 0; j < ncells; j ++)
        {
        std::map<int, Block>::const_iterator blockIter =
                this->BlockInfoMap.find (this->BlockIdList[i]->GetValue (j));
        if (blockIter == this->BlockInfoMap.end ())
          {
          vtkWarningMacro ("vtkExodusIIWriter: The block id map has come out of sync");
          continue;
          }
        int index = blockIter->second.ElementStartIndex +
                CellToElementOffset[i][j];
        switch (da->GetDataType ())
          {
          vtkArrayIteratorTemplateMacro(
            buffer->SetTuple1 (index,
                               vtkExodusIIWriterGetComponent (
                                    static_cast<VTK_TT*>(arrayIter),
                                    j * ncomp + comp)));
          }
        }
      arrayIter->Delete ();
      }
    else
      {
      for (vtkIdType j = 0; j < ncells; j ++)
        {
        std::map<int, Block>::const_iterator blockIter =
                this->BlockInfoMap.find (this->BlockIdList[i]->GetValue (j));
        if (blockIter == this->BlockInfoMap.end ())
          {
          vtkWarningMacro ("vtkExodusIIWriter: The block id map has come out of sync");
          continue;
          }
        int index = blockIter->second.ElementStartIndex +
                CellToElementOffset[i][j];
        buffer->SetTuple1 (index, 0);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkExodusIIWriter::ExtractPointData (const char *name, int comp,
                                          vtkDataArray* buffer)
{
  buffer->SetNumberOfTuples (this->NumPoints);
  int index = 0;
  for (size_t i = 0; i < this->FlattenedInput.size (); i ++)
    {
    vtkDataArray *da = this->FlattenedInput[i]->GetPointData ()->GetArray (name);
    if (da)
      {
      vtkArrayIterator *iter = da->NewIterator ();
      vtkIdType ncomp = da->GetNumberOfComponents ();
      vtkIdType nvals = ncomp * da->GetNumberOfTuples ();
      for (vtkIdType j = comp; j < nvals; j += ncomp)
        {
        switch (da->GetDataType ())
          {
          vtkArrayIteratorTemplateMacro(
            buffer->SetTuple1 (index++,
                               vtkExodusIIWriterGetComponent (
                                        static_cast<VTK_TT*>(iter), j)));
          }
        }
      iter->Delete ();
      }
    else
      {
      vtkIdType nvals = this->FlattenedInput[i]->GetNumberOfPoints ();
      for (vtkIdType j = 0; j < nvals; j ++)
        {
        buffer->SetTuple1 (index ++, 0);
        }
      }
    }
}

//----------------------------------------------------------------------------
int vtkExodusIIWriter::WriteGlobalData (int timestep, vtkDataArray *buffer)
{
  std::map<std::string, VariableInfo>::const_iterator varIter;
  buffer->Initialize ();
  buffer->SetNumberOfComponents (1);
  buffer->SetNumberOfTuples (this->NumberOfScalarGlobalArrays);
  for (varIter = this->GlobalVariableMap.begin ();
       varIter != this->GlobalVariableMap.end ();
       varIter ++)
    {
    const char *nameIn = varIter->first.c_str ();
    int numComp = varIter->second.NumComponents;
    for (int component = 0; component < numComp; component ++)
      {
      double val = this->ExtractGlobalData (nameIn, component, timestep);
      int varOutIndex = varIter->second.ScalarOutOffset + component;
      buffer->SetComponent (varOutIndex, 0, val);
      }
    }
  int rc;
  if (buffer->IsA ("vtkDoubleArray"))
    {
    vtkDoubleArray *da = vtkDoubleArray::SafeDownCast (buffer);
    rc = ex_put_glob_vars (this->fid, timestep + 1,
                  this->NumberOfScalarGlobalArrays, da->GetPointer (0));
    }
  else /* (buffer->IsA ("vtkFloatArray")) */
    {
    vtkFloatArray *fa = vtkFloatArray::SafeDownCast (buffer);
    rc = ex_put_glob_vars (this->fid, timestep + 1,
                  this->NumberOfScalarGlobalArrays, fa->GetPointer (0));
    }
  if (rc < 0)
    {
    vtkErrorMacro(<< "vtkExodusIIWriter::WriteNextTimeStep glob vars");
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkExodusIIWriter::WriteCellData (int timestep, vtkDataArray *buffer)
{
  std::map<std::string, VariableInfo>::const_iterator varIter;
  for (varIter = this->BlockVariableMap.begin ();
       varIter != this->BlockVariableMap.end ();
       varIter ++)
    {
    const char *nameIn = varIter->first.c_str ();
    int numComp = varIter->second.NumComponents;

    for (int component = 0; component < numComp; component ++)
      {
      buffer->Initialize ();
      this->ExtractCellData(nameIn, component, buffer);
      int varOutIndex = varIter->second.ScalarOutOffset + component;

      std::map<int, Block>::const_iterator blockIter;
      for (blockIter = this->BlockInfoMap.begin ();
           blockIter != this->BlockInfoMap.end ();
           blockIter ++)
        {
        int numElts = blockIter->second.NumElements;
        if (numElts < 1) continue;   // no cells in this block

        int defined = this->BlockVariableTruthValue(
                        blockIter->second.OutputIndex, varOutIndex);
        if (!defined) continue;    // var undefined in this block

        int id = blockIter->first;
        int start = blockIter->second.ElementStartIndex;

        int rc;
        if (buffer->IsA ("vtkDoubleArray"))
          {
          vtkDoubleArray *da = vtkDoubleArray::SafeDownCast (buffer);
          rc = ex_put_elem_var(this->fid, timestep + 1, varOutIndex + 1, id,
                                   numElts, da->GetPointer(start));
          }
        else /* (buffer->IsA ("vtkFloatArray")) */
          {
          vtkFloatArray *fa = vtkFloatArray::SafeDownCast (buffer);
          rc = ex_put_elem_var(this->fid, timestep + 1, varOutIndex + 1, id,
                                   numElts, fa->GetPointer(start));
          }

        if (rc < 0)
          {
          vtkErrorMacro(<< "vtkExodusIIWriter::WriteNextTimeStep ex_put_elem_var");
          return 0;
          }
        }
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkExodusIIWriter::WritePointData (int timestep, vtkDataArray *buffer)
{
  if (this->NumPoints == 0)
    {
    return 1;
    }
  std::map<std::string, VariableInfo>::const_iterator varIter;
  for (varIter = this->NodeVariableMap.begin ();
       varIter != this->NodeVariableMap.end ();
       varIter ++)
    {
    const char *nameIn = varIter->first.c_str ();
    int numComp = varIter->second.NumComponents;
    for (int component = 0; component < numComp; component ++)
      {
      buffer->Initialize ();
      this->ExtractPointData(nameIn, component, buffer);
      int varOutIndex = varIter->second.ScalarOutOffset + component;
      int rc;
      if (buffer->IsA ("vtkDoubleArray"))
        {
        vtkDoubleArray *da = vtkDoubleArray::SafeDownCast (buffer);
        rc = ex_put_nodal_var(this->fid, timestep + 1, varOutIndex + 1,
                                  this->NumPoints, da->GetPointer (0));
        }
      else /* (buffer->IsA ("vtkFloatArray")) */
        {
        vtkFloatArray *fa = vtkFloatArray::SafeDownCast (buffer);
        rc = ex_put_nodal_var(this->fid, timestep + 1, varOutIndex + 1,
                                  this->NumPoints, fa->GetPointer (0));
        }

      if (rc < 0)
        {
        vtkErrorMacro(<< "vtkExodusIIWriter::WriteNextTimeStep ex_put_nodal_var");
        return 0;
        }
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkExodusIIWriter::WriteNextTimeStep()
{
  int rc = 0;

  int ts = this->CurrentTimeIndex - this->FileTimeOffset;
  float tsv = (this->TimeValues->GetNumberOfTuples() > 0 ?
               this->TimeValues->GetValue(this->CurrentTimeIndex):
               0.0);

  vtkDataArray *buffer;
  if (this->PassDoubles)
    {
    double dtsv = (double)tsv;
    rc = ex_put_time(this->fid, ts + 1, &dtsv);
    if (rc < 0)
      {
      vtkErrorMacro(<< "vtkExodusIIWriter::WriteNextTimeStep time step values"
                    << " fid " << this->fid << " ts " << ts + 1 << " tsv " << tsv);
      return 0;
      }
    buffer = vtkDoubleArray::New ();
    }
  else
    {
    rc = ex_put_time(this->fid, ts + 1, &tsv);
    if (rc < 0)
      {
      vtkErrorMacro(<< "vtkExodusIIWriter::WriteNextTimeStep time step values"
                    << " fid " << this->fid << " ts " << ts + 1 << " tsv " << tsv);
      return 0;
      }
    buffer = vtkFloatArray::New ();
    }

  // Buffer is used to help these determine the type of the data to write out
  if (!this->WriteGlobalData (ts, buffer))
    {
    return 0;
    }
  if (!this->WriteCellData (ts, buffer))
    {
    return 0;
    }
  if (!this->WritePointData (ts, buffer))
    {
    return 0;
    }

  buffer->Delete ();

  return 1;
}

void vtkExodusIIWriter::CheckBlockInfoMap()
{
  // no op for serial version
}
