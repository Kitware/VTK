// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkClipDataSet.h"

#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkClipVolume.h"
#include "vtkExecutive.h"
#include "vtkFloatArray.h"
#include "vtkGenericCell.h"
#include "vtkImageData.h"
#include "vtkImplicitFunction.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergePoints.h"
#include "vtkNonLinearCell.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyhedron.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkClipDataSet);
vtkCxxSetObjectMacro(vtkClipDataSet, ClipFunction, vtkImplicitFunction);

//------------------------------------------------------------------------------
// Construct with user-specified implicit function; InsideOut turned off; value
// set to 0.0; and generate clip scalars turned off.
vtkClipDataSet::vtkClipDataSet(vtkImplicitFunction* cf)
{
  this->ClipFunction = cf;
  this->InsideOut = 0;
  this->Locator = nullptr;
  this->Value = 0.0;
  this->UseValueAsOffset = true;
  this->GenerateClipScalars = 0;
  this->OutputPointsPrecision = DEFAULT_PRECISION;

  this->GenerateClippedOutput = 0;
  this->MergeTolerance = 0.01;

  this->SetNumberOfOutputPorts(2);
  vtkUnstructuredGrid* output2 = vtkUnstructuredGrid::New();
  this->GetExecutive()->SetOutputData(1, output2);
  output2->Delete();

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);

  // Setup a callback for the internal readers to report progress.
  this->InternalProgressObserver = vtkCallbackCommand::New();
  this->InternalProgressObserver->SetCallback(&vtkClipDataSet::InternalProgressCallbackFunction);
  this->InternalProgressObserver->SetClientData(this);
}

//------------------------------------------------------------------------------
vtkClipDataSet::~vtkClipDataSet()
{
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
  this->SetClipFunction(nullptr);
  this->InternalProgressObserver->Delete();
}

//------------------------------------------------------------------------------
void vtkClipDataSet::InternalProgressCallbackFunction(
  vtkObject* arg, unsigned long, void* clientdata, void*)
{
  reinterpret_cast<vtkClipDataSet*>(clientdata)
    ->InternalProgressCallback(static_cast<vtkAlgorithm*>(arg));
}

//------------------------------------------------------------------------------
void vtkClipDataSet::InternalProgressCallback(vtkAlgorithm* algorithm)
{
  float progress = algorithm->GetProgress();
  this->UpdateProgress(progress);
}

//------------------------------------------------------------------------------
// Overload standard modified time function. If Clip functions is modified,
// then this object is modified as well.
vtkMTimeType vtkClipDataSet::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType time;

  if (this->ClipFunction != nullptr)
  {
    time = this->ClipFunction->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  if (this->Locator != nullptr)
  {
    time = this->Locator->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//------------------------------------------------------------------------------
vtkUnstructuredGrid* vtkClipDataSet::GetClippedOutput()
{
  if (!this->GenerateClippedOutput)
  {
    return nullptr;
  }
  return vtkUnstructuredGrid::SafeDownCast(this->GetExecutive()->GetOutputData(1));
}

//------------------------------------------------------------------------------
//
// Clip through data generating surface.
//
int vtkClipDataSet::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* realInput = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  // We have to create a copy of the input because clip requires being
  // able to InterpolateAllocate point data from the input that is
  // exactly the same as output. If the input arrays and output arrays
  // are different vtkCell3D's Clip will fail. By calling InterpolateAllocate
  // here, we make sure that the output will look exactly like the output
  // (unwanted arrays will be eliminated in InterpolateAllocate). The
  // last argument of InterpolateAllocate makes sure that arrays are shallow
  // copied from realInput to input.
  vtkSmartPointer<vtkDataSet> input;
  input.TakeReference(realInput->NewInstance());
  input->CopyStructure(realInput);
  input->GetCellData()->PassData(realInput->GetCellData());
  input->GetPointData()->InterpolateAllocate(realInput->GetPointData(), 0, 0, 1);

  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkUnstructuredGrid* clippedOutput = this->GetClippedOutput();

  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  vtkPointData *inPD = input->GetPointData(), *outPD = output->GetPointData();
  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD[2];
  vtkSmartPointer<vtkPoints> newPoints;
  vtkSmartPointer<vtkFloatArray> cellScalars;
  vtkDataArray* clipScalars;
  vtkPoints* cellPts;
  vtkIdList* cellIds;
  double s;
  vtkIdType npts;
  const vtkIdType* pts;
  vtkIdType i;
  int j;
  vtkIdType estimatedSize;
  vtkSmartPointer<vtkUnsignedCharArray> types[2];
  types[0] = types[1] = nullptr;
  int numOutputs = 1;

  outCD[0] = nullptr;
  outCD[1] = nullptr;

  vtkDebugMacro(<< "Clipping dataset");

  int inputObjectType = input->GetDataObjectType();

  // if we have volumes
  if (inputObjectType == VTK_STRUCTURED_POINTS || inputObjectType == VTK_IMAGE_DATA)
  {
    int dimension;
    int* dims = vtkImageData::SafeDownCast(input)->GetDimensions();
    for (dimension = 3, i = 0; i < 3; i++)
    {
      if (dims[i] <= 1)
      {
        dimension--;
      }
    }
    if (dimension >= 3)
    {
      this->ClipVolume(input, output);
      return 1;
    }
  }

  // Initialize self; create output objects
  //
  if (numPts < 1)
  {
    vtkDebugMacro(<< "No data to clip");
    return 1;
  }

  if (!this->ClipFunction && this->GenerateClipScalars)
  {
    vtkErrorMacro(<< "Cannot generate clip scalars if no clip function defined");
    return 1;
  }

  if (numCells < 1)
  {
    return this->ClipPoints(input, output, inputVector);
  }

  // allocate the output and associated helper classes
  estimatedSize = numCells;
  estimatedSize = estimatedSize / 1024 * 1024; // multiple of 1024
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }
  cellScalars = vtkSmartPointer<vtkFloatArray>::New();
  cellScalars->Allocate(VTK_CELL_SIZE);
  vtkSmartPointer<vtkCellArray> conn[2];
  conn[0] = conn[1] = nullptr;
  conn[0] = vtkSmartPointer<vtkCellArray>::New();
  conn[0]->AllocateEstimate(estimatedSize, 1);
  conn[0]->InitTraversal();
  types[0] = vtkSmartPointer<vtkUnsignedCharArray>::New();
  types[0]->Allocate(estimatedSize, estimatedSize / 2);
  if (this->GenerateClippedOutput)
  {
    numOutputs = 2;
    conn[1] = vtkSmartPointer<vtkCellArray>::New();
    conn[1]->AllocateEstimate(estimatedSize, 1);
    conn[1]->InitTraversal();
    types[1] = vtkSmartPointer<vtkUnsignedCharArray>::New();
    types[1]->Allocate(estimatedSize, estimatedSize / 2);
  }
  newPoints = vtkSmartPointer<vtkPoints>::New();

  // set precision for the points in the output
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    vtkPointSet* inputPointSet = vtkPointSet::SafeDownCast(input);
    if (inputPointSet)
    {
      newPoints->SetDataType(inputPointSet->GetPoints()->GetDataType());
    }
    else
    {
      newPoints->SetDataType(VTK_FLOAT);
    }
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPoints->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPoints->SetDataType(VTK_DOUBLE);
  }

  newPoints->Allocate(numPts, numPts / 2);

  // locator used to merge potentially duplicate points
  if (this->Locator == nullptr)
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion(newPoints, input->GetBounds());

  // Determine whether we're clipping with input scalars or a clip function
  // and do necessary setup.
  if (this->ClipFunction)
  {
    vtkFloatArray* tmpScalars = vtkFloatArray::New();
    tmpScalars->SetNumberOfTuples(numPts);
    tmpScalars->SetName("ClipDataSetScalars");
    inPD = vtkPointData::New();
    inPD->ShallowCopy(input->GetPointData()); // copies original
    if (this->GenerateClipScalars)
    {
      inPD->SetScalars(tmpScalars);
    }
    double pt[3];
    for (i = 0; i < numPts; i++)
    {
      input->GetPoint(i, pt);
      tmpScalars->SetValue(i, this->ClipFunction->FunctionValue(pt));
    }
    clipScalars = tmpScalars;
  }
  else // using input scalars
  {
    clipScalars = this->GetInputArrayToProcess(0, inputVector);
    if (!clipScalars)
    {
      // When processing composite datasets with partial arrays, this warning is
      // not applicable, hence disabling it.
      // vtkErrorMacro(<<"Cannot clip without clip function or input scalars");
      return 1;
    }
  }

  // Refer to BUG #8494 and BUG #11016. I cannot see any reason why one would
  // want to turn CopyScalars Off. My understanding is that this was done to
  // avoid copying of "ClipDataSetScalars" to the output when
  // this->GenerateClipScalars is false. But, if GenerateClipScalars is false,
  // then "ClipDataSetScalars" is not added as scalars to the input at all
  // (refer to code above) so it's a non-issue. Leaving CopyScalars untouched
  // i.e. ON avoids dropping of arrays (#8484) as well as segfaults (#11016).
  // if ( !this->GenerateClipScalars &&
  //  !this->GetInputArrayToProcess(0,inputVector))
  //  {
  //  outPD->CopyScalarsOff();
  //  }
  // else
  //  {
  //  outPD->CopyScalarsOn();
  //  }
  vtkDataSetAttributes* tempDSA = vtkDataSetAttributes::New();
  tempDSA->InterpolateAllocate(inPD, 1, 2);
  outPD->InterpolateAllocate(inPD, estimatedSize, estimatedSize / 2);
  tempDSA->Delete();
  outCD[0] = output->GetCellData();
  outCD[0]->CopyAllocate(inCD, estimatedSize, estimatedSize / 2);
  if (this->GenerateClippedOutput)
  {
    outCD[1] = clippedOutput->GetCellData();
    outCD[1]->CopyAllocate(inCD, estimatedSize, estimatedSize / 2);
  }

  // Process all cells and clip each in turn
  //
  bool abort = false;
  vtkIdType updateTime = numCells / 20 + 1; // update roughly every 5%
  vtkSmartPointer<vtkGenericCell> cell = vtkSmartPointer<vtkGenericCell>::New();
  int num[2];
  num[0] = num[1] = 0;
  int numNew[2];
  numNew[0] = numNew[1] = 0;
  bool sameCell[2] = { false, false };

  for (vtkIdType cellId = 0; cellId < numCells && !abort; cellId++)
  {
    if (!(cellId % updateTime))
    {
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
      abort = this->CheckAbort();
    }

    input->GetCell(cellId, cell);
    cellPts = cell->GetPoints();
    cellIds = cell->GetPointIds();
    npts = cellPts->GetNumberOfPoints();
    vtkNonLinearCell* nonLinearCell = vtkNonLinearCell::SafeDownCast(cell->GetRepresentativeCell());

    // evaluate implicit cutting function
    for (i = 0; i < npts; i++)
    {
      s = clipScalars->GetComponent(cellIds->GetId(i), 0);
      cellScalars->InsertTuple(i, &s);
    }

    double value = 0.0;
    if (this->UseValueAsOffset || !this->ClipFunction)
    {
      value = this->Value;
    }

    // perform the clipping
    for (i = 0; i < numOutputs; ++i)
    {
      if (this->StableClipNonLinear && nonLinearCell != nullptr)
      {
        sameCell[i] = nonLinearCell->StableClip(value, cellScalars, this->Locator, conn[i], inPD,
          outPD, inCD, cellId, outCD[i], this->InsideOut);
        numNew[i] = conn[i]->GetNumberOfCells() - num[i];
        num[i] = conn[i]->GetNumberOfCells();
      }
      else
      {
        cell->Clip(value, cellScalars, this->Locator, conn[i], inPD, outPD, inCD, cellId, outCD[i],
          this->InsideOut);
        numNew[i] = conn[i]->GetNumberOfCells() - num[i];
        num[i] = conn[i]->GetNumberOfCells();
        sameCell[i] = false;
      }
    }

    auto getCellType = [](vtkGenericCell* gCell, vtkIdType nPts, bool isSameCell)
    {
      if (isSameCell)
      {
        return static_cast<VTKCellType>(gCell->GetCellType());
      }
      else if (gCell->GetCellType() == VTK_POLYHEDRON)
      {
        return VTK_POLYHEDRON;
      }
      else
      {
        switch (gCell->GetCellDimension())
        {
          case 0: // points are generated--------------------------------
            return (nPts > 1 ? VTK_POLY_VERTEX : VTK_VERTEX);

          case 1: // lines are generated---------------------------------
            return (nPts > 2 ? VTK_POLY_LINE : VTK_LINE);

          case 2: // polygons are generated------------------------------
            return (nPts == 3 ? VTK_TRIANGLE : (nPts == 4 ? VTK_QUAD : VTK_POLYGON));

          case 3: // tetrahedra or wedges are generated------------------
            return (nPts == 4 ? VTK_TETRA : VTK_WEDGE);

          default:
            vtkErrorWithObjectMacro(nullptr, "Dimension cannot be lower than 0 or higher than 3");
            break;
        }
      }

      return VTK_EMPTY_CELL;
    };

    for (i = 0; i < numOutputs; i++)
    {
      for (j = 0; j < numNew[i]; j++)
      {
        conn[i]->GetNextCell(npts, pts);
        types[i]->InsertNextValue(getCellType(cell, npts, sameCell[i]));
      }
    }
  }

  if (this->ClipFunction)
  {
    clipScalars->Delete();
    inPD->Delete();
  }
  output->SetPoints(newPoints);
  output->SetCells(types[0], conn[0]);

  if (this->GenerateClippedOutput)
  {
    clippedOutput->SetPoints(newPoints);
    clippedOutput->SetCells(types[1], conn[1]);
  }

  this->Locator->Initialize(); // release any extra memory
  output->Squeeze();

  return 1;
}

//------------------------------------------------------------------------------
int vtkClipDataSet::ClipPoints(
  vtkDataSet* input, vtkUnstructuredGrid* output, vtkInformationVector** inputVector)
{
  vtkPoints* outPoints = vtkPoints::New();

  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();

  vtkIdType numPts = input->GetNumberOfPoints();

  outPD->CopyAllocate(inPD, numPts / 2, numPts / 4);

  double value = 0.0;
  if (this->UseValueAsOffset || !this->ClipFunction)
  {
    value = this->Value;
  }
  if (this->ClipFunction)
  {
    double pt[3];
    for (vtkIdType i = 0; i < numPts; i++)
    {
      input->GetPoint(i, pt);
      double fv = this->ClipFunction->FunctionValue(pt);
      int addPoint = 0;
      if (this->InsideOut)
      {
        if (fv <= value)
        {
          addPoint = 1;
        }
      }
      else
      {
        if (fv > value)
        {
          addPoint = 1;
        }
      }
      if (addPoint)
      {
        vtkIdType id = outPoints->InsertNextPoint(pt);
        outPD->CopyData(inPD, i, id);
      }
    }
  }
  else
  {
    vtkDataArray* clipScalars = this->GetInputArrayToProcess(0, inputVector);
    if (clipScalars)
    {
      double pt[3];
      for (vtkIdType i = 0; i < numPts; i++)
      {
        int addPoint = 0;
        double fv;
        clipScalars->GetTuple(i, &fv);
        if (this->InsideOut)
        {
          if (fv <= value)
          {
            addPoint = 1;
          }
        }
        else
        {
          if (fv > value)
          {
            addPoint = 1;
          }
        }
        if (addPoint)
        {
          input->GetPoint(i, pt);
          vtkIdType id = outPoints->InsertNextPoint(pt);
          outPD->CopyData(inPD, i, id);
        }
      }
    }
  }

  output->SetPoints(outPoints);
  outPoints->Delete();

  return 1;
}

//------------------------------------------------------------------------------
// Specify a spatial locator for merging points. By default,
// an instance of vtkMergePoints is used.
void vtkClipDataSet::SetLocator(vtkIncrementalPointLocator* locator)
{
  if (this->Locator == locator)
  {
    return;
  }

  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }

  if (locator)
  {
    locator->Register(this);
  }

  this->Locator = locator;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkClipDataSet::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = vtkMergePoints::New();
    this->Locator->Register(this);
    this->Locator->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkClipDataSet::ClipVolume(vtkDataSet* input, vtkUnstructuredGrid* output)
{
  vtkClipVolume* clipVolume = vtkClipVolume::New();

  clipVolume->AddObserver(vtkCommand::ProgressEvent, this->InternalProgressObserver);

  // We cannot set the input directly.  This messes up the partitioning.
  // output->UpdateNumberOfPieces gets set to 1.
  vtkImageData* tmp = vtkImageData::New();
  tmp->ShallowCopy(vtkImageData::SafeDownCast(input));

  clipVolume->SetInputData(tmp);
  double value = 0.0;
  if (this->UseValueAsOffset || !this->ClipFunction)
  {
    value = this->Value;
  }
  clipVolume->SetValue(value);
  clipVolume->SetInsideOut(this->InsideOut);
  clipVolume->SetClipFunction(this->ClipFunction);
  clipVolume->SetGenerateClipScalars(this->GenerateClipScalars);
  clipVolume->SetGenerateClippedOutput(this->GenerateClippedOutput);
  clipVolume->SetMergeTolerance(this->MergeTolerance);
  clipVolume->SetDebug(this->Debug);
  clipVolume->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
  clipVolume->SetContainerAlgorithm(this);
  clipVolume->Update();

  clipVolume->RemoveObserver(this->InternalProgressObserver);
  vtkUnstructuredGrid* clipOutput = clipVolume->GetOutput();

  output->CopyStructure(clipOutput);
  output->GetPointData()->ShallowCopy(clipOutput->GetPointData());
  output->GetCellData()->ShallowCopy(clipOutput->GetCellData());
  clipVolume->Delete();
  tmp->Delete();
}

//------------------------------------------------------------------------------
int vtkClipDataSet::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkClipDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Merge Tolerance: " << this->MergeTolerance << "\n";
  if (this->ClipFunction)
  {
    os << indent << "Clip Function: " << this->ClipFunction << "\n";
  }
  else
  {
    os << indent << "Clip Function: (none)\n";
  }
  os << indent << "InsideOut: " << (this->InsideOut ? "On\n" : "Off\n");
  os << indent << "Value: " << this->Value << "\n";
  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }

  os << indent << "Generate Clip Scalars: " << (this->GenerateClipScalars ? "On\n" : "Off\n");

  os << indent << "Generate Clipped Output: " << (this->GenerateClippedOutput ? "On\n" : "Off\n");

  os << indent << "UseValueAsOffset: " << (this->UseValueAsOffset ? "On\n" : "Off\n");

  os << indent << "Precision of the output points: " << this->OutputPointsPrecision << "\n";
}
VTK_ABI_NAMESPACE_END
