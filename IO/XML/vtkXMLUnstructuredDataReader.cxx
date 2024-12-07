// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLUnstructuredDataReader.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLUnstructuredPolyReader.txx"

#include <cassert>
#include <tuple>
#include <utility>

VTK_ABI_NAMESPACE_BEGIN
namespace
{
// Ensures that array1 and array2 are of the same type. Returns a pair with
// array type selected as follows:
// * if both arrays have different element bytesize, the one with the larger size
//   is chosen
// * if both arrays have different element types, then the type for the second
//   argument is chosen
std::pair<vtkSmartPointer<vtkDataArray>, vtkSmartPointer<vtkDataArray>> MatchArrayTypes(
  vtkSmartPointer<vtkDataArray> array1, vtkSmartPointer<vtkDataArray> array2)
{
  if (array1->GetElementComponentSize() > array2->GetElementComponentSize())
  {
    auto new2 = vtk::TakeSmartPointer(array1->NewInstance());
    new2->DeepCopy(array2);
    return std::make_pair(array1, new2);
  }
  else if (array2->GetElementComponentSize() > array1->GetElementComponentSize() ||
    array1->GetDataType() != array2->GetDataType())
  {
    auto new1 = vtk::TakeSmartPointer(array2->NewInstance());
    new1->DeepCopy(array1);
    return std::make_pair(new1, array2);
  }
  else
  {
    return std::make_pair(array1, array2);
  }
}

}

//------------------------------------------------------------------------------
vtkXMLUnstructuredDataReader::vtkXMLUnstructuredDataReader()
{
  this->PointElements = nullptr;
  this->NumberOfPoints = nullptr;
  this->TotalNumberOfPoints = 0;
  this->TotalNumberOfCells = 0;

  this->PointsTimeStep = -1; // invalid state
  this->CellArrayTimeStepRead = -1;
  this->CanReadCellArray = true;
  this->CellArrayCachedFileName = nullptr;
  this->CellArrayCachedInputString = nullptr;
  this->PointsOffset = static_cast<unsigned long>(-1);
}

//------------------------------------------------------------------------------
vtkXMLUnstructuredDataReader::~vtkXMLUnstructuredDataReader()
{
  if (this->NumberOfPieces)
  {
    this->DestroyPieces();
  }
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkPointSet* vtkXMLUnstructuredDataReader::GetOutputAsPointSet()
{
  return vtkPointSet::SafeDownCast(this->GetOutputDataObject(0));
}

//------------------------------------------------------------------------------
vtkXMLDataElement* vtkXMLUnstructuredDataReader ::FindDataArrayWithName(
  vtkXMLDataElement* eParent, const char* name)
{
  // Find a nested element that represents a data array with the given name.
  // and proper TimeStep
  int i;
  for (i = 0; i < eParent->GetNumberOfNestedElements(); ++i)
  {
    vtkXMLDataElement* eNested = eParent->GetNestedElement(i);
    if (strcmp(eNested->GetName(), "DataArray") == 0)
    {
      const char* aName = eNested->GetAttribute("Name");
      if (aName && (strcmp(aName, name) == 0))
      {
        int numTimeSteps =
          eNested->GetVectorAttribute("TimeStep", this->NumberOfTimeSteps, this->TimeSteps);
        assert(numTimeSteps <= this->NumberOfTimeSteps);
        // Check if CurrentTimeStep is in the array and particular field is also:
        int isCurrentTimeInArray =
          vtkXMLReader::IsTimeStepInArray(this->CurrentTimeStep, this->TimeSteps, numTimeSteps);
        // If no time is specified or if time is specified and match then read
        if (!numTimeSteps || isCurrentTimeInArray)
        {
          return eNested;
        }
      }
    }
  }
  return nullptr;
}

//------------------------------------------------------------------------------
vtkIdTypeArray* vtkXMLUnstructuredDataReader::ConvertToIdTypeArray(vtkDataArray* a)
{
  // If it is already a vtkIdTypeArray, just return it.
  vtkIdTypeArray* ida = vtkArrayDownCast<vtkIdTypeArray>(a);
  if (ida)
  {
    return ida;
  }

  // Need to convert the data.
  ida = vtkIdTypeArray::New();
  ida->ShallowCopy(a);
  a->Delete();
  return ida;
}

//------------------------------------------------------------------------------
vtkUnsignedCharArray* vtkXMLUnstructuredDataReader::ConvertToUnsignedCharArray(vtkDataArray* a)
{
  // If it is already a vtkUnsignedCharArray, just return it.
  vtkUnsignedCharArray* uca = vtkArrayDownCast<vtkUnsignedCharArray>(a);
  if (uca)
  {
    return uca;
  }

  // Need to convert the data.
  uca = vtkUnsignedCharArray::New();
  uca->ShallowCopy(a);
  a->Delete();
  return uca;
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::SetupOutputTotals()
{
  this->TotalNumberOfPoints = 0;
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
  {
    this->TotalNumberOfPoints += this->NumberOfPoints[i];
  }
  this->StartPoint = 0;
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::SetupNextPiece()
{
  this->StartPoint += this->NumberOfPoints[this->Piece];
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::SetupUpdateExtent(int piece, int numberOfPieces, int ghostLevel)
{
  this->UpdatePieceId = piece;
  this->UpdateNumberOfPieces = numberOfPieces;
  this->UpdateGhostLevel = ghostLevel;

  // If more pieces are requested than available, just return empty
  // pieces for the extra ones.
  if (this->UpdateNumberOfPieces > this->NumberOfPieces)
  {
    this->UpdateNumberOfPieces = this->NumberOfPieces;
  }

  // Find the range of pieces to read.
  if (this->UpdatePieceId < this->UpdateNumberOfPieces)
  {
    this->StartPiece = ((this->UpdatePieceId * this->NumberOfPieces) / this->UpdateNumberOfPieces);
    this->EndPiece =
      (((this->UpdatePieceId + 1) * this->NumberOfPieces) / this->UpdateNumberOfPieces);
  }
  else
  {
    this->StartPiece = 0;
    this->EndPiece = 0;
  }

  // Find the total size of the output.
  this->SetupOutputTotals();
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::ReadXMLData()
{
  // Get the update request.
  int piece;
  int numberOfPieces;
  int ghostLevel;
  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numberOfPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  vtkDebugMacro(
    "Updating piece " << piece << " of " << numberOfPieces << " with ghost level " << ghostLevel);

  // Setup the range of pieces that will be read.
  this->SetupUpdateExtent(piece, numberOfPieces, ghostLevel);

  // If there are no data to read, stop now.
  if (this->StartPiece == this->EndPiece)
  {
    return;
  }

  vtkDebugMacro(
    "Reading piece range [" << this->StartPiece << ", " << this->EndPiece << ") from file.");

  // Let superclasses read data.  This also allocates output data.
  this->Superclass::ReadXMLData();

  // Split current progress range based on fraction contributed by
  // each piece.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);

  // Calculate the cumulative fraction of data contributed by each
  // piece (for progress).
  float* fractions = new float[this->EndPiece - this->StartPiece + 1];
  int i;
  fractions[0] = 0;
  for (i = this->StartPiece; i < this->EndPiece; ++i)
  {
    int index = i - this->StartPiece;
    fractions[index + 1] =
      (fractions[index] + this->GetNumberOfPointsInPiece(i) + this->GetNumberOfCellsInPiece(i));
  }
  if (fractions[this->EndPiece - this->StartPiece] == 0)
  {
    fractions[this->EndPiece - this->StartPiece] = 1;
  }
  for (i = this->StartPiece; i < this->EndPiece; ++i)
  {
    int index = i - this->StartPiece;
    fractions[index + 1] = fractions[index + 1] / fractions[this->EndPiece - this->StartPiece];
  }

  // Read the data needed from each piece.
  for (i = this->StartPiece; (i < this->EndPiece && !this->AbortExecute && !this->DataError); ++i)
  {
    // Set the range of progress for this piece.
    this->SetProgressRange(progressRange, i - this->StartPiece, fractions);

    if (!this->Superclass::ReadPieceData(i))
    {
      // An error occurred while reading the piece.
      this->DataError = 1;
    }
    this->SetupNextPiece();
  }
  // Prior to major version 2, The vtkGhostType array was named vtkGhostLevels. Since the array has
  // been already added, we need to re-add it to the output, so that the ghost array for point/cell
  // data is correctly cached.
  if (this->GetFileMajorVersion() < 2)
  {
    vtkDataSet* output = vtkDataSet::SafeDownCast(this->GetCurrentOutput());
    vtkPointData* pointData = output->GetPointData();
    vtkCellData* cellData = output->GetCellData();
    if (pointData->HasArray("vtkGhostType"))
    {
      pointData->AddArray(pointData->GetArray("vtkGhostType"));
    }
    if (cellData->HasArray("vtkGhostType"))
    {
      cellData->AddArray(cellData->GetArray("vtkGhostType"));
    }
  }

  delete[] fractions;
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::SetupPieces(int numPieces)
{
  this->Superclass::SetupPieces(numPieces);
  this->NumberOfPoints = new vtkIdType[numPieces];
  this->PointElements = new vtkXMLDataElement*[numPieces];
  for (int i = 0; i < numPieces; ++i)
  {
    this->PointElements[i] = nullptr;
    this->NumberOfPoints[i] = 0;
  }
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::DestroyPieces()
{
  delete[] this->PointElements;
  delete[] this->NumberOfPoints;
  this->PointElements = nullptr;
  this->NumberOfPoints = nullptr;
  this->Superclass::DestroyPieces();
}

//------------------------------------------------------------------------------
vtkIdType vtkXMLUnstructuredDataReader::GetNumberOfPoints()
{
  return this->TotalNumberOfPoints;
}

//------------------------------------------------------------------------------
vtkIdType vtkXMLUnstructuredDataReader::GetNumberOfCells()
{
  return this->TotalNumberOfCells;
}

//------------------------------------------------------------------------------
vtkIdType vtkXMLUnstructuredDataReader::GetNumberOfPieces()
{
  return this->NumberOfPieces;
}

//------------------------------------------------------------------------------
vtkIdType vtkXMLUnstructuredDataReader::GetNumberOfPointsInPiece(int piece)
{
  return this->NumberOfPoints[piece];
}

//------------------------------------------------------------------------------
// Note that any changes (add or removing information) made to this method
// should be replicated in CopyOutputInformation
void vtkXMLUnstructuredDataReader::SetupOutputInformation(vtkInformation* outInfo)
{
  this->Superclass::SetupOutputInformation(outInfo);

  if (this->NumberOfPieces > 1)
  {
    outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  }
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::CopyOutputInformation(vtkInformation* outInfo, int port)
{
  this->Superclass::CopyOutputInformation(outInfo, port);
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredDataReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();

  // Create the points array.
  vtkPoints* points = vtkPoints::New();

  // Use the configuration of the first piece since all are the same.
  vtkXMLDataElement* ePoints = this->PointElements[0];
  if (ePoints)
  {
    // Non-zero volume.
    vtkAbstractArray* aa = this->CreateArray(ePoints->GetNestedElement(0));
    vtkDataArray* a = vtkArrayDownCast<vtkDataArray>(aa);
    if (a)
    {
      // Allocate the points array.
      a->SetNumberOfTuples(this->GetNumberOfPoints());
      points->SetData(a);
      a->Delete();
    }
    else
    {
      if (aa)
      {
        aa->Delete();
      }
      this->DataError = 1;
    }
  }
  else if (this->NumberOfPoints[0] > 0)
  {
    vtkWarningMacro(
      "No Points element available in first piece found in file. Reading file may fail.");
  }

  vtkPointSet::SafeDownCast(this->GetCurrentOutput())->SetPoints(points);
  points->Delete();
}

//------------------------------------------------------------------------------
int vtkXMLUnstructuredDataReader::ReadPiece(vtkXMLDataElement* ePiece)
{
  if (!this->Superclass::ReadPiece(ePiece))
  {
    return 0;
  }

  if (!ePiece->GetScalarAttribute("NumberOfPoints", this->NumberOfPoints[this->Piece]))
  {
    vtkErrorMacro("Piece " << this->Piece << " is missing its NumberOfPoints attribute.");
    this->NumberOfPoints[this->Piece] = 0;
    return 0;
  }

  // Find the Points element in the piece.
  int i;
  this->PointElements[this->Piece] = nullptr;
  for (i = 0; i < ePiece->GetNumberOfNestedElements(); ++i)
  {
    vtkXMLDataElement* eNested = ePiece->GetNestedElement(i);
    if (strcmp(eNested->GetName(), "Points") == 0)
    {
      // make sure the XML file is somehow valid:
      if ((this->NumberOfTimeSteps > 0 && eNested->GetNumberOfNestedElements() >= 1) ||
        (this->NumberOfTimeSteps == 0 && eNested->GetNumberOfNestedElements() == 1))
      {
        this->PointElements[this->Piece] = eNested;
      }
    }
  }

  // If there are some points, we require a Points element.
  if (!this->PointElements[this->Piece] && (this->NumberOfPoints[this->Piece] > 0))
  {
    vtkErrorMacro("A piece is missing its Points element "
                  "or element does not have exactly 1 array.");
    return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkXMLUnstructuredDataReader::ReadPieceData()
{
  // The amount of data read by the superclass's ReadPieceData comes
  // from point/cell data (we read point specifications here).
  vtkIdType superclassPieceSize =
    (this->NumberOfPointArrays * this->GetNumberOfPointsInPiece(this->Piece) +
      this->NumberOfCellArrays * this->GetNumberOfCellsInPiece(this->Piece));

  // Total amount of data in this piece comes from point/cell data
  // arrays and the point specifications themselves.
  vtkIdType totalPieceSize = superclassPieceSize + 1 * this->GetNumberOfPointsInPiece(this->Piece);
  if (totalPieceSize == 0)
  {
    totalPieceSize = 1;
  }

  // Split the progress range based on the approximate fraction of
  // data that will be read by each step in this method.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);
  float fractions[3] = { 0, float(superclassPieceSize) / totalPieceSize, 1 };

  // Set the range of progress for the superclass.
  this->SetProgressRange(progressRange, 0, fractions);

  // Let the superclass read its data.
  if (!this->Superclass::ReadPieceData())
  {
    return 0;
  }

  vtkPointSet* output = vtkPointSet::SafeDownCast(this->GetCurrentOutput());

  // Set the range of progress for the Points.
  this->SetProgressRange(progressRange, 1, fractions);

  // Read the points array.
  vtkXMLDataElement* ePoints = this->PointElements[this->Piece];
  if (ePoints)
  {
    for (int i = 0; (i < ePoints->GetNumberOfNestedElements() && !this->AbortExecute); ++i)
    {
      vtkXMLDataElement* eNested = ePoints->GetNestedElement(i);
      if (strcmp(eNested->GetName(), "DataArray") != 0 && strcmp(eNested->GetName(), "Array") != 0)
      {
        vtkErrorMacro("Invalid Array.");
        this->DataError = 1;
        return 0;
      }
      int needToRead = this->PointsNeedToReadTimeStep(eNested);
      if (needToRead)
      {
        // Read the array. Test for abort before and after the read. Before
        // so that we can skip the read, after to prevent unwanted error
        // messages.
        if (!this->AbortExecute &&
          !this->ReadArrayForPoints(eNested, output->GetPoints()->GetData()) && !this->AbortExecute)
        {
          vtkErrorMacro("Cannot read points array from "
            << ePoints->GetName() << " in piece " << this->Piece
            << ".  The data array in the element may be too short.");
          return 0;
        }
      }
    }
  }

  return 1;
}

namespace
{

// We just need this to use the ArrayDispatch mechanism to evaluate
// whether the offset type array is valid, so the functor does nothing.
struct ValidateOffsetsType
{
  template <typename ArrayT>
  void operator()(ArrayT*)
  {
  }
};

struct ValidateOffsets
{
  bool Valid{ false };
  vtkIdType ConnSize{ 0 };

  template <typename ArrayT>
  void operator()(ArrayT* offsets)
  {
    using ValueType = vtk::GetAPIType<ArrayT>;

    auto range = vtk::DataArrayValueRange<1>(offsets);

    if (range.size() == 0)
    {
      this->Valid = false;
      return;
    }

    // First offset must be zero:
    if (*range.cbegin() != 0)
    {
      this->Valid = false;
      return;
    }

    // Ensure that offsets are increasing:
    auto it = std::adjacent_find(range.cbegin(), range.cend(),
      [](const ValueType a, const ValueType b) -> bool { return b < a; });

    this->Valid = it == range.cend();

    if (this->Valid)
    { // The last entry in the offsets is the size of the connectivity
      this->ConnSize = static_cast<vtkIdType>(*(range.cend() - 1));
    }
  }
};

struct ConstructCellArray
{
  vtkCellArray* CellArray;
  vtkDataArray* Connectivity;
  bool ConnectivityIsValid;

  ConstructCellArray(vtkCellArray* cellArray, vtkDataArray* connectivity)
    : CellArray{ cellArray }
    , Connectivity{ connectivity }
    , ConnectivityIsValid{ false }
  {
  }

  template <typename ArrayT>
  void operator()(ArrayT* offsets)
  {
    // Connectivity should have the same type as offsets:
    ArrayT* conn = vtkArrayDownCast<ArrayT>(this->Connectivity);
    if (!conn)
    {
      this->ConnectivityIsValid = false;
      return;
    }

    this->CellArray->SetData(offsets, conn);
    this->ConnectivityIsValid = true;
  }
};

struct QuickCheckOffsets
{
  bool Valid{ false };

  template <typename ArrayT>
  void operator()(ArrayT* offsets)
  {
    using ValueType = vtk::GetAPIType<ArrayT>;
    auto range = vtk::DataArrayValueRange<1>(offsets);

    if (range.size() == 0)
    {
      this->Valid = false;
      return;
    }

    // Ensure that offsets are increasing:
    auto it = std::adjacent_find(range.cbegin(), range.cend(),
      [](const ValueType a, const ValueType b) -> bool { return b < a; });

    this->Valid = it == range.cend();
  }
};

} // end anon namespace

//------------------------------------------------------------------------------
int vtkXMLUnstructuredDataReader::ReadCellArray(vtkIdType numberOfCells,
  vtkIdType vtkNotUsed(totalNumberOfCells), vtkXMLDataElement* eCells, vtkCellArray* outCells)
{
  // This part is here to determine if we need to read the cell array.
  // If the cell array was already generated in a previous time step, there's no need recreating it.
  // However, within a same time step, multiple pieces can be present. We need to read them all.
  // The logic works as follows:
  // * If there's no output geometry yet, it means we need to create it. We save the current time
  //   step to know in which time step the cell array is being read.
  // * If there's already geometry, we either are in a new time step and nothing needs to be done,
  //   or we're still in the time step used to create the geometry in a new piece and we need to
  //   read it.
  // We quit the "create geometry mode" if the cell array time step becomes different than the
  // current time step.
  if ((this->ReadFromInputString &&
        this->CellArrayCachedInputString != this->InputString.c_str()) ||
    (!this->ReadFromInputString && this->CellArrayCachedFileName != this->FileName))
  {
    // We wipe the geometry if a new file or new input string is to be read.
    outCells->Initialize();
    if (this->ReadFromInputString)
    {
      this->CellArrayCachedInputString = this->InputString.c_str();
    }
    else
    {
      this->CellArrayCachedFileName = this->FileName;
    }
  }
  if (outCells->GetNumberOfCells() == 0)
  {
    this->CellArrayTimeStepRead = this->CurrentTimeStep;
    this->CanReadCellArray = true;
  }
  if (this->CellArrayTimeStepRead != this->CurrentTimeStep)
  {
    this->CanReadCellArray = false;
  }
  if (!this->CanReadCellArray || numberOfCells <= 0)
  {
    return 1;
  }
  else
  {
    if (!eCells)
    {
      return 0;
    }
  }

  // Split progress range into 1/5 for offsets array and 4/5 for
  // connectivity array.  This assumes an average of 4 points per
  // cell.  Unfortunately, we cannot know the length of the
  // connectivity array ahead of time to calculate the real fraction.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);
  float fractions[3] = { 0, 0.2f, 1 };

  // Set range of progress for offsets array.
  this->SetProgressRange(progressRange, 0, fractions);

  // Read the cell offsets.
  if (this->AbortExecute)
  {
    return 0;
  }

  // ------------------------ Read offsets -------------------------------------
  vtkSmartPointer<vtkDataArray> cellOffsets;
  {
    vtkXMLDataElement* eOffsets = this->FindDataArrayWithName(eCells, "offsets");
    if (!eOffsets && !this->AbortExecute)
    {
      vtkErrorMacro("Cannot read cell offsets from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"offsets\" array could not be found.");
      return 0;
    }

    if (this->AbortExecute)
    {
      return 0;
    }

    auto aOffsets = vtk::TakeSmartPointer(this->CreateArray(eOffsets));
    if (!aOffsets)
    {
      vtkErrorMacro("Cell offsets array missing from " << eCells->GetName());
      return 0;
    }

    if (!aOffsets->IsA("vtkDataArray"))
    {
      vtkErrorMacro("Cannot cast cell offsets from " << eCells->GetName() << " to vtkDataArray.");
      return 0;
    }

    cellOffsets = vtkArrayDownCast<vtkDataArray>(aOffsets.Get());

    if (cellOffsets->GetNumberOfComponents() != 1)
    {
      vtkErrorMacro("Cannot read cell offsets from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"offsets\" array could not be created"
        << " with one component.");
      return 0;
    }

    // The file format skips the first 0 in the offsets array, so set the first
    // value in the array to 0 and read the data into the array starting at
    // index 1.
    cellOffsets->SetNumberOfTuples(numberOfCells + 1);
    cellOffsets->SetComponent(0, 0, 0);
    if (!this->ReadArrayValues(eOffsets, 1, cellOffsets, 0, numberOfCells, CELL_DATA) &&
      !this->AbortExecute)
    {
      vtkErrorMacro("Cannot read cell offsets from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"offsets\" array is not long enough.");
      return 0;
    }
  }

  if (this->AbortExecute)
  {
    return 0;
  }

  // Validate the offsets
  ValidateOffsets offsetValidator;
  using SupportedArrays = vtkCellArray::InputArrayList;

  // Convert array to supported type if necessary
  using Dispatch = vtkArrayDispatch::DispatchByArray<SupportedArrays>;
  bool offsetsNeedConversion = !Dispatch::Execute(cellOffsets, ValidateOffsetsType{});
  if (offsetsNeedConversion)
  {
    // Use a vtkCellArray::ArrayType64 to ensure we can represent the incoming offset array type.
    vtkSmartPointer<vtkCellArray::ArrayType64> newArray =
      vtkSmartPointer<vtkCellArray::ArrayType64>::New();

    // DeepCopy takes care of the type conversion.
    newArray->DeepCopy(cellOffsets);
    cellOffsets = newArray;
  }

  if (!Dispatch::Execute(cellOffsets, offsetValidator))
  {
    vtkErrorMacro(
      "Error reading cell offsets: Unsupported array type: " << cellOffsets->GetClassName());
    return 0;
  }
  if (!offsetValidator.Valid)
  {
    vtkErrorMacro("Cannot read cell connectivity from "
      << eCells->GetName() << " in piece " << this->Piece << " because the \"offsets\" array is"
      << " not monotonically increasing or starts with a"
      << " value other than 0.");
    return 0;
  }

  // Set range of progress for connectivity array.
  this->SetProgressRange(progressRange, 1, fractions);

  const vtkIdType connLength = offsetValidator.ConnSize;

  // ------------------------ Read connectivity---------------------------------
  vtkSmartPointer<vtkDataArray> conn;
  {
    vtkXMLDataElement* eConn = this->FindDataArrayWithName(eCells, "connectivity");
    if (!eConn)
    {
      vtkErrorMacro("Cannot read cell connectivity from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"connectivity\" array could not be found.");
      return 0;
    }

    if (this->AbortExecute)
    {
      return 0;
    }

    auto aConn = vtk::TakeSmartPointer(this->CreateArray(eConn));
    if (!aConn)
    {
      vtkErrorMacro("Cell connectivity array missing from " << eCells->GetName());
      return 0;
    }

    if (!aConn->IsA("vtkDataArray"))
    {
      vtkErrorMacro(
        "Cannot cast cell connectivity from " << eCells->GetName() << " to vtkDataArray.");
      return 0;
    }
    conn = vtkArrayDownCast<vtkDataArray>(aConn.Get());

    if (conn->GetNumberOfComponents() != 1)
    {
      vtkErrorMacro("Cannot read cell connectivity from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"connectivity\" array could not be created"
        << " with one component.");
      return 0;
    }

    conn->SetNumberOfTuples(connLength);

    if (this->AbortExecute)
    {
      return 0;
    }

    if (!this->ReadArrayValues(eConn, 0, conn, 0, connLength, CELL_DATA) && !this->AbortExecute)
    {
      vtkErrorMacro("Cannot read cell connectivity from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"connectivity\" array is not long enough.");
      return 0;
    }

    if (this->AbortExecute)
    {
      return 0;
    }
  }

  // Ensure that `conn` and `cellOffsets` arrays have same types.
  std::tie(conn, cellOffsets) = MatchArrayTypes(conn, cellOffsets);

  //------------------- Construct vtkCellArray ---------------------------------

  if (outCells->GetNumberOfCells() == 0)
  { // First execution: Directly construct output cell array:
    ConstructCellArray builder{ outCells, conn };
    if (!Dispatch::Execute(cellOffsets, builder))
    {
      vtkErrorMacro(
        "Cannot read cell data from " << eCells->GetName() << ". Offset array type is invalid.");
      return 0;
    }

    if (!builder.ConnectivityIsValid)
    {
      vtkErrorMacro("Cannot read cell data from "
        << eCells->GetName()
        << ". Offsets and connectivity arrays must be the same "
           "type.");
      return 0;
    }
  }
  else
  { // Construct a temporary vtkCellArray that holds the arrays, and then
    // append to the input outCells.
    vtkNew<vtkCellArray> tmpCells;
    ConstructCellArray builder{ tmpCells, conn };
    if (!Dispatch::Execute(cellOffsets, builder))
    {
      vtkErrorMacro(
        "Cannot read cell data from " << eCells->GetName() << ". Offset array type is invalid.");
      return 0;
    }

    if (!builder.ConnectivityIsValid)
    {
      vtkErrorMacro("Cannot read cell data from "
        << eCells->GetName()
        << ". Offsets and connectivity arrays must be the same "
           "type.");
      return 0;
    }

    outCells->Append(tmpCells, this->StartPoint);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkXMLUnstructuredDataReader::ReadPolyhedronCellArray(vtkIdType numberOfCells,
  vtkXMLDataElement* eCells, vtkCellArray* outFaces, vtkCellArray* outFaceOffsets)
{
  if (numberOfCells <= 0)
  {
    return 1;
  }
  else
  {
    if (!eCells || !outFaces || !outFaceOffsets)
    {
      return 0;
    }
  }

  // Split progress range
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);
  float fractions[5] = { 0, 0.2f, 0.4f, 0.6f, 1 };

  // Set range of progress for offsets array.
  this->SetProgressRange(progressRange, 0, fractions);

  //
  // Start by reading polyhedron_to_face arrays
  //
  // ------------------------ Read polyhedron_to_face offsets -------------------------------------
  vtkSmartPointer<vtkDataArray> polyhedronCellOffsets;
  {
    vtkXMLDataElement* polyOffsets = this->FindDataArrayWithName(eCells, "polyhedron_offsets");
    if (!polyOffsets && !this->AbortExecute)
    {
      vtkErrorMacro("Cannot read polyhedron cell offsets from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"polyhedron_offsets\" array could not be found.");
      return 0;
    }

    if (this->AbortExecute)
    {
      return 0;
    }

    auto aOffsets = vtk::TakeSmartPointer(this->CreateArray(polyOffsets));
    if (!aOffsets)
    {
      vtkErrorMacro("Polyhedron cell offsets array missing from " << eCells->GetName());
      return 0;
    }

    if (!aOffsets->IsA("vtkDataArray"))
    {
      vtkErrorMacro(
        "Cannot cast polyhedron cell offsets from " << eCells->GetName() << " to vtkDataArray.");
      return 0;
    }

    polyhedronCellOffsets = vtkArrayDownCast<vtkDataArray>(aOffsets.Get());

    if (polyhedronCellOffsets->GetNumberOfComponents() != 1)
    {
      vtkErrorMacro("Cannot read polyhedron cell offsets from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"polyhedron_offsets\" array could not be created"
        << " with one component.");
      return 0;
    }

    // The file format skips the first 0 in the offsets array, so set the first
    // value in the array to 0 and read the data into the array starting at
    // index 1.
    polyhedronCellOffsets->SetNumberOfTuples(numberOfCells + 1);
    polyhedronCellOffsets->SetComponent(0, 0, 0);
    if (!this->ReadArrayValues(polyOffsets, 1, polyhedronCellOffsets, 0, numberOfCells) &&
      !this->AbortExecute)
    {
      vtkErrorMacro("Cannot read polyhedorn cell offsets from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"polyhedron_offsets\" array is not long enough.");
      return 0;
    }
  }

  if (this->AbortExecute)
  {
    return 0;
  }

  // Validate the offsets
  ValidateOffsets offsetValidator;
  using SupportedArrays = vtkCellArray::InputArrayList;

  // Convert array to supported type if necessary
  using Dispatch = vtkArrayDispatch::DispatchByArray<SupportedArrays>;
  bool offsetsNeedConversion = !Dispatch::Execute(polyhedronCellOffsets, ValidateOffsetsType{});
  if (offsetsNeedConversion)
  {
    // Use a vtkCellArray::ArrayType64 to ensure we can represent the incoming offset array type.
    vtkSmartPointer<vtkCellArray::ArrayType64> newArray =
      vtkSmartPointer<vtkCellArray::ArrayType64>::New();

    // DeepCopy takes care of the type conversion.
    newArray->DeepCopy(polyhedronCellOffsets);
    polyhedronCellOffsets = newArray;
  }

  if (!Dispatch::Execute(polyhedronCellOffsets, offsetValidator))
  {
    vtkErrorMacro("Error reading cell offsets: Unsupported array type: "
      << polyhedronCellOffsets->GetClassName());
    return 0;
  }
  if (!offsetValidator.Valid)
  {
    vtkErrorMacro("Cannot read polyhedron cell connectivity from "
      << eCells->GetName() << " in piece " << this->Piece
      << " because the \"polyhedron_offsets\" array is"
      << " not monotonically increasing or starts with a"
      << " value other than 0.");
    return 0;
  }

  // Set range of progress for connectivity array.
  this->SetProgressRange(progressRange, 1, fractions);

  const vtkIdType connLength = offsetValidator.ConnSize;

  // ------------------------ Read polyhedron_to_faces connectivity---------------------------------
  vtkSmartPointer<vtkDataArray> polyToFaces;
  {
    vtkXMLDataElement* ePolyConn = this->FindDataArrayWithName(eCells, "polyhedron_to_faces");
    if (!ePolyConn)
    {
      vtkErrorMacro("Cannot read polyedron cell connectivity from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"polyhedron_to_faces\" array could not be found.");
      return 0;
    }

    if (this->AbortExecute)
    {
      return 0;
    }

    auto aConn = vtk::TakeSmartPointer(this->CreateArray(ePolyConn));
    if (!aConn)
    {
      vtkErrorMacro("Polyhedron connectivity array missing from " << eCells->GetName());
      return 0;
    }

    if (!aConn->IsA("vtkDataArray"))
    {
      vtkErrorMacro("Cannot cast polyhedron cell connectivity from " << eCells->GetName()
                                                                     << " to vtkDataArray.");
      return 0;
    }
    polyToFaces = vtkArrayDownCast<vtkDataArray>(aConn.Get());

    if (polyToFaces->GetNumberOfComponents() != 1)
    {
      vtkErrorMacro("Cannot read polyhedron cell connectivity from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"polyhedron_to_faces\" array could not be created"
        << " with one component.");
      return 0;
    }

    polyToFaces->SetNumberOfTuples(connLength);

    if (this->AbortExecute)
    {
      return 0;
    }

    if (!this->ReadArrayValues(ePolyConn, 0, polyToFaces, 0, connLength, CELL_DATA) &&
      !this->AbortExecute)
    {
      vtkErrorMacro("Cannot read cell connectivity from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"polyhedron_to_faces\" array is not long enough.");
      return 0;
    }

    if (this->AbortExecute)
    {
      return 0;
    }
  }

  // Ensure that `polyhedron_to_faces` and `polyhedron_offsets` arrays have same types.
  std::tie(polyToFaces, polyhedronCellOffsets) =
    MatchArrayTypes(polyToFaces, polyhedronCellOffsets);

  // Now determine/check the "range" of faces to read to build polyhedron cells ?
  vtkIdType faceRange[2];
  if (!vtkXMLUnstructuredDataReaderPrivate::FindPolyFaceRange(faceRange, polyToFaces))
  {
    vtkErrorMacro("Cannot determine polyhedron face range to read"
      << " from polyhedron_to_faces array.");
    return 0;
  }
  vtkIdType numberOfFacesToRead = faceRange[1] - faceRange[0] + 1;
  if (numberOfFacesToRead < 1)
  {
    return 0;
  }
  // Set range of progress for face_offset array.
  this->SetProgressRange(progressRange, 2, fractions);
  //
  // Read the face_offsets array
  //
  // ------------------------ Read face_offsets offsets -------------------------------------
  vtkSmartPointer<vtkDataArray> faceOffsets;
  {
    vtkXMLDataElement* eFaceOffsets = this->FindDataArrayWithName(eCells, "face_offsets");
    if (!eFaceOffsets && !this->AbortExecute)
    {
      vtkErrorMacro("Cannot read face offsets from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"face_offsets\" array could not be found.");
      return 0;
    }

    if (this->AbortExecute)
    {
      return 0;
    }

    auto aOffsets = vtk::TakeSmartPointer(this->CreateArray(eFaceOffsets));
    if (!aOffsets)
    {
      vtkErrorMacro("Polyhedron faces cell offsets array missing from " << eCells->GetName());
      return 0;
    }

    if (!aOffsets->IsA("vtkDataArray"))
    {
      vtkErrorMacro(
        "Cannot cast polyhedron faces offsets from " << eCells->GetName() << " to vtkDataArray.");
      return 0;
    }

    faceOffsets = vtkArrayDownCast<vtkDataArray>(aOffsets.Get());

    if (faceOffsets->GetNumberOfComponents() != 1)
    {
      vtkErrorMacro("Cannot read polyhedron faces offsets from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"face_offsets\" array could not be created"
        << " with one component.");
      return 0;
    }

    faceOffsets->SetNumberOfTuples(numberOfFacesToRead + 1);
    if (faceRange[0] == 0)
    {
      // The file format skips the first 0 in the offsets array, so set the first
      // value in the array to 0 and read the data into the array starting at
      // index 1.
      faceOffsets->SetComponent(0, 0, 0);
      if (!this->ReadArrayValues(eFaceOffsets, 1, faceOffsets, 0, numberOfFacesToRead) &&
        !this->AbortExecute)
      {
        vtkErrorMacro("Cannot read polyhedron face offsets from "
          << eCells->GetName() << " in piece " << this->Piece
          << " because the \"face_offsets\" array is not long enough.");
        return 0;
      }
    }
    else
    {
      // We are doing a partial reading of the offsets.
      // The file format skips the first 0 in the offsets array so the starting read point has a -1
      // offset. The number of values to read is numberOfFacesToRead+1 since the first offset is
      // also read.
      if (!this->ReadArrayValues(
            eFaceOffsets, 0, faceOffsets, faceRange[0] - 1, numberOfFacesToRead + 1) &&
        !this->AbortExecute)
      {
        vtkErrorMacro("Cannot read polyhedron face offsets from "
          << eCells->GetName() << " in piece " << this->Piece
          << " because the \"face_offsets\" array is not long enough.");
        return 0;
      }
    }
  }

  if (this->AbortExecute)
  {
    return 0;
  }

  // Convert array to supported type if necessary
  using Dispatch = vtkArrayDispatch::DispatchByArray<SupportedArrays>;
  bool faceOffsetsNeedConversion = !Dispatch::Execute(faceOffsets, ValidateOffsetsType{});
  if (faceOffsetsNeedConversion)
  {
    // Use a vtkCellArray::ArrayType64 to ensure we can represent the incoming offset array type.
    vtkSmartPointer<vtkCellArray::ArrayType64> newArray =
      vtkSmartPointer<vtkCellArray::ArrayType64>::New();

    // DeepCopy takes care of the type conversion.
    newArray->DeepCopy(faceOffsets);
    faceOffsets = newArray;
  }

  // Light validation of offsets and full validation later after face connectivity reading.
  QuickCheckOffsets offsetChecker;
  if (!Dispatch::Execute(faceOffsets, offsetChecker))
  {
    vtkErrorMacro(
      "Error reading face offsets: Unsupported array type: " << faceOffsets->GetClassName());
    return 0;
  }
  if (!offsetChecker.Valid)
  {
    vtkErrorMacro("Cannot read polyhedron face connectivity from "
      << eCells->GetName() << " in piece " << this->Piece
      << " because the \"face_offsets\" array is"
      << " not increasing from start to end.");
    return 0;
  }

  // Get First and last value of faceOffsets
  vtkIdType faceConnectivityRange[2];
  if (auto faceOffsetsArr = vtkTypeInt64Array::SafeDownCast(faceOffsets))
  {
    faceConnectivityRange[0] = static_cast<vtkIdType>(faceOffsetsArr->GetValue(0));
    faceConnectivityRange[1] =
      static_cast<vtkIdType>(faceOffsetsArr->GetValue(faceOffsets->GetNumberOfValues() - 1));
  }
  else if (auto faceOffsetsArr32 = vtkTypeInt32Array::SafeDownCast(faceOffsets))
  {
    faceConnectivityRange[0] = static_cast<vtkIdType>(faceOffsetsArr32->GetValue(0));
    faceConnectivityRange[1] =
      static_cast<vtkIdType>(faceOffsetsArr32->GetValue(faceOffsets->GetNumberOfValues() - 1));
  }
  else
  {
    return 0;
  }
  const vtkIdType faceConnSize = faceConnectivityRange[1] - faceConnectivityRange[0];

  // Set range of progress for face_connectivity array.
  this->SetProgressRange(progressRange, 3, fractions);
  // ------------------------ Read face connectivity---------------------------------
  vtkSmartPointer<vtkDataArray> faceConnectivity;
  {
    vtkXMLDataElement* eFaceConn = this->FindDataArrayWithName(eCells, "face_connectivity");
    if (!eFaceConn)
    {
      vtkErrorMacro("Cannot read polyhedron faces connectivity from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"face_connectivity\" array could not be found.");
      return 0;
    }

    if (this->AbortExecute)
    {
      return 0;
    }

    auto aConn = vtk::TakeSmartPointer(this->CreateArray(eFaceConn));
    if (!aConn)
    {
      vtkErrorMacro("Polyhedron face connectivity array missing from " << eCells->GetName());
      return 0;
    }

    if (!aConn->IsA("vtkDataArray"))
    {
      vtkErrorMacro("Cannot cast polyhedron face connectivity from " << eCells->GetName()
                                                                     << " to vtkDataArray.");
      return 0;
    }
    faceConnectivity = vtkArrayDownCast<vtkDataArray>(aConn.Get());

    if (faceConnectivity->GetNumberOfComponents() != 1)
    {
      vtkErrorMacro("Cannot read polyhedron face connectivity from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"face_connectivity\" array could not be created"
        << " with one component.");
      return 0;
    }

    faceConnectivity->SetNumberOfTuples(faceConnSize); // faceConnLength

    if (this->AbortExecute)
    {
      return 0;
    }

    if (!this->ReadArrayValues(
          eFaceConn, 0, faceConnectivity, faceConnectivityRange[0], faceConnSize, CELL_DATA) &&
      !this->AbortExecute)
    {
      vtkErrorMacro("Cannot read cell connectivity from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"face_connectivity\" array is not long enough.");
      return 0;
    }

    if (this->AbortExecute)
    {
      return 0;
    }
  }

  // Ensure that `faceConnectivity` and `faceOffsets` arrays have same types.
  std::tie(faceConnectivity, faceOffsets) = MatchArrayTypes(faceConnectivity, faceOffsets);

  // Now clean the FaceOffsets and PolyToFaces in case of partial reading
  // i.e. reading not starting from 0.
  if (!vtkXMLUnstructuredDataReaderPrivate::RebaseOffset(faceOffsets))
  {
    vtkErrorMacro("Cannot clean face_offsets array.");
    return 0;
  }
  // Now faceOffsets should be valid and faceOffsets + faceConnectivity can be converted to
  // vtkCellArray Rebase the poly_to_face array if needed
  if (faceRange[0] != 0)
  {
    if (!vtkXMLUnstructuredDataReaderPrivate::RebasePolyFaces(polyToFaces, faceRange[0]))
    {
      vtkErrorMacro("Cannot clean polyhedron_to_faces array.");
      return 0;
    }
  }
  // Now complete validation of Face Offsets...
  if (!Dispatch::Execute(faceOffsets, offsetValidator))
  {
    vtkErrorMacro(
      "Error reading face offsets: Unsupported array type: " << faceOffsets->GetClassName());
    return 0;
  }
  if (!offsetValidator.Valid)
  {
    vtkErrorMacro("Cannot read face cell connectivity from "
      << eCells->GetName() << " in piece " << this->Piece
      << " because the \"face_offsets\" array is"
      << " not monotonically increasing or starts with a"
      << " value other than 0.");
    return 0;
  }

  // Set range of progress for connectivity array.
  this->SetProgressRange(progressRange, 1, fractions);

  // Now we are ready to assemble everything
  //-------------------------------------------------
  // Check that the output is coherent
  if (outFaceOffsets->GetNumberOfCells() == 0 && outFaces->GetNumberOfCells() != 0)
  {
    vtkErrorMacro(
      "Cannot read polyhedron cell" << eCells->GetName() << ". Polyhedron output is not coherent.");
    return 0;
  }
  //
  if (outFaceOffsets->GetNumberOfCells() == 0)
  {
    // First execution: Directly construct output cell array:
    ConstructCellArray builderPoly{ outFaceOffsets, polyToFaces };
    if (!Dispatch::Execute(polyhedronCellOffsets, builderPoly))
    {
      vtkErrorMacro("Cannot read cell data from " << eCells->GetName()
                                                  << ". polyhedron_offsets array type is invalid.");
      return 0;
    }

    if (!builderPoly.ConnectivityIsValid)
    {
      vtkErrorMacro("Cannot read polyhedron cell data from "
        << eCells->GetName()
        << ". polyhedron_offsets and polyhedron_to_faces arrays must be the same "
           "type.");
      return 0;
    }

    ConstructCellArray builderFaces{ outFaces, faceConnectivity };
    if (!Dispatch::Execute(faceOffsets, builderFaces))
    {
      vtkErrorMacro("Cannot read faces data from " << eCells->GetName()
                                                   << ". face_offsets array type is invalid.");
      return 0;
    }

    if (!builderFaces.ConnectivityIsValid)
    {
      vtkErrorMacro("Cannot read polyhedron cell data from "
        << eCells->GetName()
        << ". polyhedron_offsets and polyhedron_to_faces arrays must be the same "
           "type.");
      return 0;
    }
  }
  else
  {
    vtkIdType faceNumOffset = outFaces->GetNumberOfCells();

    // Construct a temporary vtkCellArray that holds the arrays, and then
    // append to the input outCells.

    vtkNew<vtkCellArray> tmpFaces;
    ConstructCellArray builderFaces{ tmpFaces, faceConnectivity };
    if (!Dispatch::Execute(faceOffsets, builderFaces))
    {
      vtkErrorMacro("Cannot read faces data from " << eCells->GetName()
                                                   << ". face_offsets array type is invalid.");
      return 0;
    }

    if (!builderFaces.ConnectivityIsValid)
    {
      vtkErrorMacro("Cannot read polyhedron cell data from "
        << eCells->GetName()
        << ". polyhedron_offsets and polyhedron_to_faces arrays must be the same "
           "type.");
      return 0;
    }
    outFaces->Append(tmpFaces);

    vtkNew<vtkCellArray> tmpCells;
    ConstructCellArray builderPoly{ tmpCells, polyToFaces };
    if (!Dispatch::Execute(polyhedronCellOffsets, builderPoly))
    {
      vtkErrorMacro("Cannot read cell data from " << eCells->GetName()
                                                  << ". polyhedron_offsets array type is invalid.");
      return 0;
    }

    if (!builderPoly.ConnectivityIsValid)
    {
      vtkErrorMacro("Cannot read polyhedron cell data from "
        << eCells->GetName()
        << ". polyhedron_offsets and poly_to_faces arrays must be the same "
           "type.");
      return 0;
    }

    outFaceOffsets->Append(tmpCells, faceNumOffset);
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkXMLUnstructuredDataReader::ReadFaceArray(vtkIdType numberOfCells, vtkXMLDataElement* eCells,
  vtkIdTypeArray* outFaces, vtkIdTypeArray* outFaceOffsets)
{
  if (numberOfCells <= 0)
  {
    return 1;
  }
  else
  {
    if (!eCells || !outFaces || !outFaceOffsets)
    {
      return 0;
    }
  }

  // Split progress range into 1/5 for faces array and 4/5 for
  // faceoffsets array.  This assumes an average of 4 points per
  // face.  Unfortunately, we cannot know the length ahead of time
  // to calculate the real fraction.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);
  float fractions[3] = { 0, 0.2f, 1 };

  // Set range of progress for offsets array.
  this->SetProgressRange(progressRange, 0, fractions);

  // Read the cell offsets.
  vtkXMLDataElement* efaceOffsets = this->FindDataArrayWithName(eCells, "faceoffsets");
  if (!efaceOffsets)
  {
    vtkErrorMacro("Cannot read face offsets from "
      << eCells->GetName() << " in piece " << this->Piece
      << " because the \"faceoffsets\" array could not be found.");
    return 0;
  }
  vtkAbstractArray* ac1 = this->CreateArray(efaceOffsets);
  vtkDataArray* c1 = vtkArrayDownCast<vtkDataArray>(ac1);
  if (!c1 || (c1->GetNumberOfComponents() != 1))
  {
    vtkErrorMacro("Cannot read face offsets from "
      << eCells->GetName() << " in piece " << this->Piece
      << " because the \"faceoffsets\" array could not be created"
      << " with one component.");
    if (ac1)
    {
      ac1->Delete();
    }
    return 0;
  }
  c1->SetNumberOfTuples(numberOfCells);
  if (!this->ReadArrayValues(efaceOffsets, 0, c1, 0, numberOfCells))
  {
    vtkErrorMacro("Cannot read face offsets from "
      << eCells->GetName() << " in piece " << this->Piece
      << " because the \"faceoffsets\" array is not long enough.");
    return 0;
  }
  vtkIdTypeArray* faceOffsets = this->ConvertToIdTypeArray(c1);
  if (!faceOffsets)
  {
    vtkErrorMacro("Cannot read cell offsets from " << eCells->GetName() << " in piece "
                                                   << this->Piece
                                                   << " because the \"offsets\" array could not be"
                                                   << " converted to a vtkIdTypeArray.");
    return 0;
  }

  // Note that faceOffsets[i] points to the end of the i-th cell's faces + 1. We
  // now Extract the size of the faces array from faceOffsets array. We compute
  // it by subtracting the size of outFaces from the maximum value of faceOffset
  // array element. The faceOffsets array is incremental, but contains -1 to
  // indicate a non-polyhedron cell.
  vtkIdType* faceoffsetPtr = faceOffsets->GetPointer(0);
  vtkIdType maxOffset = -1;
  for (vtkIdType i = numberOfCells - 1; i >= 0; i--)
  {
    if (faceoffsetPtr[i] > -1)
    {
      maxOffset = faceoffsetPtr[i];
      break;
    }
  }

  // Paraview-BUG-13892. The facesArrayLength here should be relative
  // to the current piece being read, NOT the outFaces already read.
  vtkIdType facesArrayLength = maxOffset;

  // special handling of the case of all non-polyhedron cells
  if (facesArrayLength <= 0)
  {
    faceOffsets->Delete();
    return 1;
  }

  // Set range of progress for faces array.
  this->SetProgressRange(progressRange, 1, fractions);

  // Read the faces array.
  vtkXMLDataElement* efaces = this->FindDataArrayWithName(eCells, "faces");
  if (!efaces)
  {
    vtkErrorMacro("Cannot read faces from " << eCells->GetName() << " in piece " << this->Piece
                                            << " because the \"faces\" array could not be found.");
    faceOffsets->Delete();
    return 0;
  }
  vtkAbstractArray* ac0 = this->CreateArray(efaces);
  vtkDataArray* c0 = vtkArrayDownCast<vtkDataArray>(ac0);
  if (!c0 || (c0->GetNumberOfComponents() != 1))
  {
    vtkErrorMacro("Cannot read faces from " << eCells->GetName() << " in piece " << this->Piece
                                            << " because the \"faces\" array could not be created"
                                            << " with one component.");
    faceOffsets->Delete();
    if (ac0)
    {
      ac0->Delete();
    }
    return 0;
  }
  c0->SetNumberOfTuples(facesArrayLength);
  if (!this->ReadArrayValues(efaces, 0, c0, 0, facesArrayLength))
  {
    vtkErrorMacro("Cannot read faces from " << eCells->GetName() << " in piece " << this->Piece
                                            << " because the \"faces\" array is not long enough.");
    faceOffsets->Delete();
    return 0;
  }
  vtkIdTypeArray* faces = this->ConvertToIdTypeArray(c0);
  if (!faces)
  {
    vtkErrorMacro("Cannot read faces from " << eCells->GetName() << " in piece " << this->Piece
                                            << " because the \"faces\" array could not be"
                                            << " converted to a vtkIdTypeArray.");
    faceOffsets->Delete();
    return 0;
  }

  // Copy the contents of the faceoffsets array.
  // Note that faceOffsets[i] points to the end of the i-th cell + 1. While
  // vtkUnstructuredGrid::FaceLocations[i] points to the beginning of the
  // i-th cell. Need to convert. Also note that for both arrays, a
  // non-polyhedron cell has a offset of -1.
  vtkIdType* facesPtr = faces->GetPointer(0);
  vtkIdType startLoc = outFaceOffsets->GetNumberOfTuples();
  vtkIdType* outFaceOffsetsPtr = outFaceOffsets->WritePointer(startLoc, numberOfCells);
  vtkIdType currLoc = startLoc;

  // (Paraview-BUG-13892)
  // outFaceOffsets should point to the offset location in the outFaces array
  vtkIdType currFaceLoc = outFaces->GetNumberOfTuples();
  for (vtkIdType i = 0; i < numberOfCells; ++i, ++outFaceOffsetsPtr)
  {
    if (faceoffsetPtr[i] < 0)
    {
      *outFaceOffsetsPtr = -1;
    }
    else
    {
      *outFaceOffsetsPtr = currFaceLoc;
      // find next offset
      // read numberOfFaces in a cell
      vtkIdType numberOfCellFaces = facesPtr[currLoc - startLoc];
      currLoc += 1;
      currFaceLoc++;
      for (vtkIdType j = 0; j < numberOfCellFaces; j++)
      {
        // read numberOfPoints in a face
        vtkIdType tmpLoc = currLoc - startLoc;
        vtkIdType numberOfFacePoints = facesPtr[tmpLoc];
        // update the point ids with StartPoint (Paraview-BUG-13892)
        if (this->StartPoint > 0)
        {
          for (vtkIdType pidx = tmpLoc + 1; pidx < tmpLoc + 1 + numberOfFacePoints; pidx++)
          {
            facesPtr[pidx] += this->StartPoint;
          }
        }
        currLoc += numberOfFacePoints + 1;
        currFaceLoc += numberOfFacePoints + 1;
      }
    }
  }

  // sanity check
  if (currLoc - startLoc != facesArrayLength)
  {
    vtkErrorMacro("Cannot read faces from " << eCells->GetName() << " in piece " << this->Piece
                                            << " because the \"faces\" and"
                                            << " \"faceoffsets\" arrays don't match.");
    faceOffsets->Delete();
    return 0;
  }

  // Copy the contents of the faces array.
  startLoc = outFaces->GetNumberOfTuples();
  vtkIdType length = faces->GetNumberOfTuples();
  vtkIdType* outFacesPtr = outFaces->WritePointer(startLoc, length);
  for (vtkIdType i = 0; i < length; ++i, ++outFacesPtr)
  {
    *outFacesPtr = facesPtr[i];
  }

  faces->Delete();
  faceOffsets->Delete();

  return 1;
}

//------------------------------------------------------------------------------
int vtkXMLUnstructuredDataReader::ReadFaceCellArray(vtkIdType numberOfCells,
  vtkXMLDataElement* eCells, vtkCellArray* outFaces, vtkCellArray* outFaceOffsets)
{
  if (numberOfCells <= 0)
  {
    return 1;
  }
  else
  {
    if (!eCells || !outFaces || !outFaceOffsets)
    {
      return 0;
    }
  }

  // Split progress range into 1/5 for faces array and 4/5 for
  // faceoffsets array.  This assumes an average of 4 points per
  // face.  Unfortunately, we cannot know the length ahead of time
  // to calculate the real fraction.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);
  float fractions[3] = { 0, 0.2f, 1 };

  // Set range of progress for offsets array.
  this->SetProgressRange(progressRange, 0, fractions);

  // Read the cell offsets.
  vtkXMLDataElement* efaceOffsets = this->FindDataArrayWithName(eCells, "faceoffsets");
  if (!efaceOffsets)
  {
    vtkErrorMacro("Cannot read face offsets from "
      << eCells->GetName() << " in piece " << this->Piece
      << " because the \"faceoffsets\" array could not be found.");
    return 0;
  }
  vtkAbstractArray* ac1 = this->CreateArray(efaceOffsets);
  vtkDataArray* c1 = vtkArrayDownCast<vtkDataArray>(ac1);
  if (!c1 || (c1->GetNumberOfComponents() != 1))
  {
    vtkErrorMacro("Cannot read face offsets from "
      << eCells->GetName() << " in piece " << this->Piece
      << " because the \"faceoffsets\" array could not be created"
      << " with one component.");
    if (ac1)
    {
      ac1->Delete();
    }
    return 0;
  }
  c1->SetNumberOfTuples(numberOfCells);
  if (!this->ReadArrayValues(efaceOffsets, 0, c1, 0, numberOfCells))
  {
    vtkErrorMacro("Cannot read face offsets from "
      << eCells->GetName() << " in piece " << this->Piece
      << " because the \"faceoffsets\" array is not long enough.");
    return 0;
  }
  vtkIdTypeArray* faceOffsets = this->ConvertToIdTypeArray(c1);
  if (!faceOffsets)
  {
    vtkErrorMacro("Cannot read cell offsets from " << eCells->GetName() << " in piece "
                                                   << this->Piece
                                                   << " because the \"offsets\" array could not be"
                                                   << " converted to a vtkIdTypeArray.");
    return 0;
  }

  // Note that faceOffsets[i] points to the end of the i-th cell's faces + 1. We
  // now Extract the size of the faces array from faceOffsets array. We compute
  // it by subtracting the size of outFaces from the maximum value of faceOffset
  // array element. The faceOffsets array is incremental, but contains -1 to
  // indicate a non-polyhedron cell.
  vtkIdType* faceoffsetPtr = faceOffsets->GetPointer(0);
  vtkIdType maxOffset = -1;
  for (vtkIdType i = numberOfCells - 1; i >= 0; i--)
  {
    if (faceoffsetPtr[i] > -1)
    {
      maxOffset = faceoffsetPtr[i];
      break;
    }
  }

  // Paraview-BUG-13892. The facesArrayLength here should be relative
  // to the current piece being read, NOT the outFaces already read.
  vtkIdType facesArrayLength = maxOffset;

  // special handling of the case of all non-polyhedron cells
  if (facesArrayLength <= 0)
  {
    faceOffsets->Delete();
    return 1;
  }

  // Set range of progress for faces array.
  this->SetProgressRange(progressRange, 1, fractions);

  // Read the faces array.
  vtkXMLDataElement* efaces = this->FindDataArrayWithName(eCells, "faces");
  if (!efaces)
  {
    vtkErrorMacro("Cannot read faces from " << eCells->GetName() << " in piece " << this->Piece
                                            << " because the \"faces\" array could not be found.");
    faceOffsets->Delete();
    return 0;
  }
  vtkAbstractArray* ac0 = this->CreateArray(efaces);
  vtkDataArray* c0 = vtkArrayDownCast<vtkDataArray>(ac0);
  if (!c0 || (c0->GetNumberOfComponents() != 1))
  {
    vtkErrorMacro("Cannot read faces from " << eCells->GetName() << " in piece " << this->Piece
                                            << " because the \"faces\" array could not be created"
                                            << " with one component.");
    faceOffsets->Delete();
    if (ac0)
    {
      ac0->Delete();
    }
    return 0;
  }
  c0->SetNumberOfTuples(facesArrayLength);
  if (!this->ReadArrayValues(efaces, 0, c0, 0, facesArrayLength))
  {
    vtkErrorMacro("Cannot read faces from " << eCells->GetName() << " in piece " << this->Piece
                                            << " because the \"faces\" array is not long enough.");
    faceOffsets->Delete();
    return 0;
  }
  vtkIdTypeArray* faces = this->ConvertToIdTypeArray(c0);
  if (!faces)
  {
    vtkErrorMacro("Cannot read faces from " << eCells->GetName() << " in piece " << this->Piece
                                            << " because the \"faces\" array could not be"
                                            << " converted to a vtkIdTypeArray.");
    faceOffsets->Delete();
    return 0;
  }

  // Copy the contents of the faceoffsets array.
  // Note that faceOffsets[i] points to the end of the i-th cell + 1. While
  // vtkUnstructuredGrid::FaceLocations[i] points to the beginning of the
  // i-th cell. Need to convert. Also note that for both arrays, a
  // non-polyhedron cell has a offset of -1.
  vtkIdType* facesPtr = faces->GetPointer(0);

  vtkIdType currLoc = 0;
  vtkIdType currFaceNum = outFaces->GetNumberOfCells();
  for (vtkIdType i = 0; i < numberOfCells; ++i)
  {
    if (faceoffsetPtr[i] < 0)
    {
      outFaceOffsets->InsertNextCell(0);
    }
    else
    {
      // find next offset
      // read numberOfFaces in a cell
      vtkIdType numberOfCellFaces = facesPtr[currLoc];
      currLoc += 1;
      outFaceOffsets->InsertNextCell(static_cast<int>(numberOfCellFaces));
      for (vtkIdType j = 0; j < numberOfCellFaces; j++)
      {
        // read numberOfPoints in a face
        vtkIdType numberOfFacePoints = facesPtr[currLoc];
        // update the point ids with StartPoint (Paraview-BUG-13892)
        if (this->StartPoint > 0)
        {
          for (vtkIdType pidx = currLoc + 1; pidx < currLoc + 1 + numberOfFacePoints; pidx++)
          {
            facesPtr[pidx] += this->StartPoint;
          }
        }
        currLoc += numberOfFacePoints + 1;
        outFaceOffsets->InsertCellPoint(currFaceNum++);
      }
    }
  }

  // sanity check
  if (currLoc != facesArrayLength)
  {
    vtkErrorMacro("Cannot read faces from " << eCells->GetName() << " in piece " << this->Piece
                                            << " because the \"faces\" and"
                                            << " \"faceoffsets\" arrays don't match.");
    faceOffsets->Delete();
    return 0;
  }

  // Copy the contents of the faces array.
  currLoc = 0;
  for (vtkIdType i = 0; i < numberOfCells; ++i)
  {
    if (faceoffsetPtr[i] < 0)
    {
      continue;
    }
    // read numberOfFaces in a cell
    vtkIdType numberOfCellFaces = facesPtr[currLoc++];
    for (vtkIdType j = 0; j < numberOfCellFaces; j++)
    {
      // read numberOfPoints in a face
      vtkIdType numberOfFacePoints = facesPtr[currLoc++];
      outFaces->InsertNextCell(static_cast<int>(numberOfFacePoints));
      for (vtkIdType ptidx = 0; ptidx < numberOfFacePoints; ptidx++)
      {
        outFaces->InsertCellPoint(facesPtr[currLoc++]);
      }
    }
  }

  faces->Delete();
  faceOffsets->Delete();

  return 1;
}

//------------------------------------------------------------------------------
int vtkXMLUnstructuredDataReader::ReadArrayForPoints(
  vtkXMLDataElement* da, vtkAbstractArray* outArray)
{
  vtkIdType startPoint = this->StartPoint;
  vtkIdType numPoints = this->NumberOfPoints[this->Piece];
  vtkIdType components = outArray->GetNumberOfComponents();
  return this->ReadArrayValues(
    da, startPoint * components, outArray, 0, numPoints * components, POINT_DATA);
}

//------------------------------------------------------------------------------
int vtkXMLUnstructuredDataReader::PointsNeedToReadTimeStep(vtkXMLDataElement* eNested)
{
  // Easy case no timestep:
  int numTimeSteps =
    eNested->GetVectorAttribute("TimeStep", this->NumberOfTimeSteps, this->TimeSteps);
  assert(numTimeSteps <= this->NumberOfTimeSteps);
  if (!numTimeSteps && !this->NumberOfTimeSteps)
  {
    assert(this->PointsTimeStep == -1); // No timestep in this file
    return 1;
  }
  // else TimeStep was specified but no TimeValues associated were found
  assert(this->NumberOfTimeSteps);

  // case numTimeSteps > 1
  int isCurrentTimeInArray =
    vtkXMLReader::IsTimeStepInArray(this->CurrentTimeStep, this->TimeSteps, numTimeSteps);
  if (!isCurrentTimeInArray && numTimeSteps)
  {
    return 0;
  }
  // we know that time steps are specified and that CurrentTimeStep is in the array
  // we need to figure out if we need to read the array or if it was forwarded
  // Need to check the current 'offset'
  unsigned long offset;
  if (eNested->GetScalarAttribute("offset", offset))
  {
    if (this->PointsOffset != offset)
    {
      // save the pointsOffset we are about to read
      assert(this->PointsTimeStep == -1); // cannot have mixture of binary and appended
      this->PointsOffset = offset;
      return 1;
    }
  }
  else
  {
    // No offset is specified this is a binary file
    // First thing to check if numTimeSteps == 0:
    if (!numTimeSteps && this->NumberOfTimeSteps && this->PointsTimeStep == -1)
    {
      // Update last PointsTimeStep read
      this->PointsTimeStep = this->CurrentTimeStep;
      return 1;
    }
    int isLastTimeInArray =
      vtkXMLReader::IsTimeStepInArray(this->PointsTimeStep, this->TimeSteps, numTimeSteps);
    // If no time is specified or if time is specified and match then read
    if (isCurrentTimeInArray && !isLastTimeInArray)
    {
      // CurrentTimeStep is in TimeSteps but Last is not := need to read
      // Update last PointsTimeStep read
      this->PointsTimeStep = this->CurrentTimeStep;
      return 1;
    }
  }
  // all other cases we don't need to read:
  return 0;
}

//------------------------------------------------------------------------------
// Returns true if we need to read the data for the current time step
int vtkXMLUnstructuredDataReader::CellsNeedToReadTimeStep(
  vtkXMLDataElement* eNested, int& cellstimestep, unsigned long& cellsoffset)
{
  // Easy case no timestep:
  int numTimeSteps =
    eNested->GetVectorAttribute("TimeStep", this->NumberOfTimeSteps, this->TimeSteps);
  assert(numTimeSteps <= this->NumberOfTimeSteps);
  if (!numTimeSteps && !this->NumberOfTimeSteps)
  {
    assert(cellstimestep == -1); // No timestep in this file
    return 1;
  }
  // else TimeStep was specified but no TimeValues associated were found
  assert(!this->NumberOfTimeSteps);

  // case numTimeSteps > 1
  int isCurrentTimeInArray =
    vtkXMLReader::IsTimeStepInArray(this->CurrentTimeStep, this->TimeSteps, numTimeSteps);
  if (!isCurrentTimeInArray && numTimeSteps)
  {
    return 0;
  }
  // we know that time steps are specified and that CurrentTimeStep is in the array
  // we need to figure out if we need to read the array or if it was forwarded
  // Need to check the current 'offset'
  unsigned long offset;
  if (eNested->GetScalarAttribute("offset", offset))
  {
    if (cellsoffset != offset)
    {
      // save the cellsOffset we are about to read
      assert(cellstimestep == -1); // cannot have mixture of binary and appended
      cellsoffset = offset;
      return 1;
    }
  }
  else
  {
    // No offset is specified this is a binary file
    // First thing to check if numTimeSteps == 0:
    if (!numTimeSteps && this->NumberOfTimeSteps && cellstimestep == -1)
    {
      // Update last PointsTimeStep read
      cellstimestep = this->CurrentTimeStep;
      return 1;
    }
    int isLastTimeInArray =
      vtkXMLReader::IsTimeStepInArray(cellstimestep, this->TimeSteps, numTimeSteps);
    // If no time is specified or if time is specified and match then read
    if (isCurrentTimeInArray && !isLastTimeInArray)
    {
      // CurrentTimeStep is in TimeSteps but Last is not := need to read
      // Update last cellstimestep read
      cellstimestep = this->CurrentTimeStep;
      return 1;
    }
  }
  // all other cases we don't need to read:
  return 0;
}
VTK_ABI_NAMESPACE_END
