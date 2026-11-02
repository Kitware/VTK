// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLUnstructuredGridReader.h"

#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkExecutive.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSideData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridSideMap.h"
#include "vtkUpdateCellsV8toV9.h"
#include "vtkXMLDataElement.h"

#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLUnstructuredGridReader);

//------------------------------------------------------------------------------
vtkXMLUnstructuredGridReader::vtkXMLUnstructuredGridReader()
{
  this->CellElements = nullptr;
  this->NumberOfCells = nullptr;
  this->CellsTimeStep = -1;
  this->CellsOffset = static_cast<unsigned long>(-1); // almost invalid state
  this->SideDataArraySelection = vtkDataArraySelection::New();
  this->SideDataArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);
}

//------------------------------------------------------------------------------
vtkXMLUnstructuredGridReader::~vtkXMLUnstructuredGridReader()
{
  if (this->NumberOfPieces)
  {
    this->DestroyPieces();
  }
  this->SideDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->SideDataArraySelection->Delete();
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SideDataArraySelection: " << this->SideDataArraySelection << "\n";
}

//------------------------------------------------------------------------------
int vtkXMLUnstructuredGridReader::GetNumberOfSideArrays()
{
  return this->SideDataArraySelection->GetNumberOfArrays();
}

//------------------------------------------------------------------------------
const char* vtkXMLUnstructuredGridReader::GetSideArrayName(int index)
{
  return this->SideDataArraySelection->GetArrayName(index);
}

//------------------------------------------------------------------------------
int vtkXMLUnstructuredGridReader::GetSideArrayStatus(const char* name)
{
  return this->SideDataArraySelection->ArrayIsEnabled(name);
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredGridReader::SetSideArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->SideDataArraySelection->EnableArray(name);
  }
  else
  {
    this->SideDataArraySelection->DisableArray(name);
  }
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredGridReader::SetupOutputInformation(vtkInformation* outInfo)
{
  this->Superclass::SetupOutputInformation(outInfo);
  if (this->InformationError || !this->SideDataElements)
  {
    return;
  }
  vtkXMLDataElement* eSideData = this->SideDataElements[0];
  this->SetDataArraySelections(eSideData, this->SideDataArraySelection);

  // The number of sides is that of the arrays, since the numbering is only
  // made when the cells are read.
  vtkIdType numberOfSides = -1;
  if (eSideData && eSideData->GetNumberOfNestedElements() > 0)
  {
    eSideData->GetNestedElement(0)->GetScalarAttribute("NumberOfTuples", numberOfSides);
  }
  vtkInformationVector* infoVector = nullptr;
  if (!this->SetFieldDataInfo(
        eSideData, vtkDataObject::FIELD_ASSOCIATION_SIDES, numberOfSides, infoVector))
  {
    return;
  }
  if (infoVector)
  {
    outInfo->Set(vtkDataObject::SIDE_DATA_VECTOR(), infoVector);
    infoVector->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredGridReader::CopyOutputInformation(vtkInformation* outInfo, int port)
{
  this->Superclass::CopyOutputInformation(outInfo, port);
  vtkInformation* localInfo = this->GetExecutive()->GetOutputInformation(port);
  if (localInfo->Has(vtkDataObject::SIDE_DATA_VECTOR()))
  {
    outInfo->CopyEntry(localInfo, vtkDataObject::SIDE_DATA_VECTOR());
  }
}

//------------------------------------------------------------------------------
vtkUnstructuredGrid* vtkXMLUnstructuredGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//------------------------------------------------------------------------------
vtkUnstructuredGrid* vtkXMLUnstructuredGridReader::GetOutput(int idx)
{
  return vtkUnstructuredGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//------------------------------------------------------------------------------
const char* vtkXMLUnstructuredGridReader::GetDataSetName()
{
  return "UnstructuredGrid";
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredGridReader::GetOutputUpdateExtent(
  int& piece, int& numberOfPieces, int& ghostLevel)
{
  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numberOfPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredGridReader::SetupOutputTotals()
{
  this->Superclass::SetupOutputTotals();
  // Find the total size of the output.
  int i;
  this->TotalNumberOfCells = 0;
  for (i = this->StartPiece; i < this->EndPiece; ++i)
  {
    this->TotalNumberOfCells += this->NumberOfCells[i];
  }

  // Data reading will start at the beginning of the output.
  this->StartCell = 0;
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredGridReader::SetupPieces(int numPieces)
{
  this->Superclass::SetupPieces(numPieces);
  this->NumberOfCells = new vtkIdType[numPieces];
  this->CellElements = new vtkXMLDataElement*[numPieces];
  this->SideDataElements = new vtkXMLDataElement*[numPieces];
  this->SideIdsElements = new vtkXMLDataElement*[numPieces];
  for (int i = 0; i < numPieces; ++i)
  {
    this->CellElements[i] = nullptr;
    this->SideDataElements[i] = nullptr;
    this->SideIdsElements[i] = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredGridReader::DestroyPieces()
{
  delete[] this->CellElements;
  delete[] this->NumberOfCells;
  delete[] this->SideDataElements;
  delete[] this->SideIdsElements;
  this->SideDataElements = nullptr;
  this->SideIdsElements = nullptr;
  this->Superclass::DestroyPieces();
}

//------------------------------------------------------------------------------
vtkIdType vtkXMLUnstructuredGridReader::GetNumberOfCellsInPiece(int piece)
{
  return this->NumberOfCells[piece];
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredGridReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();

  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(this->GetCurrentOutput());

  // Setup the output's cell arrays.
  vtkNew<vtkUnsignedCharArray> cellTypes;
  cellTypes->SetNumberOfTuples(this->GetNumberOfCells());
  cellTypes->FillValue(VTK_EMPTY_CELL);
  vtkNew<vtkCellArray> outCells;

  output->SetCells(cellTypes, outCells);
}

//------------------------------------------------------------------------------
int vtkXMLUnstructuredGridReader::ReadPiece(vtkXMLDataElement* ePiece)
{
  if (!this->Superclass::ReadPiece(ePiece))
  {
    return 0;
  }
  int i;

  if (!ePiece->GetScalarAttribute("NumberOfCells", this->NumberOfCells[this->Piece]))
  {
    vtkErrorMacro("Piece " << this->Piece << " is missing its NumberOfCells attribute.");
    this->NumberOfCells[this->Piece] = 0;
    return 0;
  }

  // Find the Cells, SideData and SideIds elements in the piece.
  this->CellElements[this->Piece] = nullptr;
  this->SideDataElements[this->Piece] = nullptr;
  this->SideIdsElements[this->Piece] = nullptr;
  for (i = 0; i < ePiece->GetNumberOfNestedElements(); ++i)
  {
    vtkXMLDataElement* eNested = ePiece->GetNestedElement(i);
    if ((strcmp(eNested->GetName(), "Cells") == 0) && (eNested->GetNumberOfNestedElements() > 0))
    {
      this->CellElements[this->Piece] = eNested;
    }
    else if (strcmp(eNested->GetName(), "SideData") == 0)
    {
      this->SideDataElements[this->Piece] = eNested;
    }
    else if (strcmp(eNested->GetName(), "SideIds") == 0)
    {
      this->SideIdsElements[this->Piece] = eNested;
    }
  }

  if (!this->CellElements[this->Piece] && this->NumberOfCells[this->Piece] > 0)
  {
    vtkErrorMacro("A piece is missing its Cells element.");
    return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredGridReader::ReadXMLData()
{
  this->Superclass::ReadXMLData();
  this->ReadSides();
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredGridReader::ReadSides()
{
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(this->GetCurrentOutput());
  if (!output || this->AbortExecute || this->DataError || !this->SideDataElements)
  {
    return;
  }
  bool present = false;
  for (int piece = this->StartPiece; piece < this->EndPiece; ++piece)
  {
    present = present || this->SideDataElements[piece] || this->SideIdsElements[piece];
  }
  if (!present)
  {
    return;
  }
  if (this->EndPiece - this->StartPiece != 1)
  {
    vtkWarningMacro("Side data is read only when one piece is read into the output, so the side "
                    "data of pieces "
      << this->StartPiece << " to " << this->EndPiece - 1 << " was skipped.");
    return;
  }

  // Of the arrays of an element, the one for the time step being read.
  auto forThisTimeStep = [this](vtkXMLDataElement* eArray)
  {
    int timeStep = 0;
    return !eArray->GetScalarAttribute("TimeStep", timeStep) || timeStep == this->CurrentTimeStep;
  };

  const int piece = this->StartPiece;
  if (vtkXMLDataElement* eIds = this->SideIdsElements[piece])
  {
    vtkIdType numberOfSides = 0;
    vtkIdType numberOfHalfSides = 0;
    vtkXMLDataElement* eArray = nullptr;
    for (int i = 0; i < eIds->GetNumberOfNestedElements() && !eArray; ++i)
    {
      eArray = forThisTimeStep(eIds->GetNestedElement(i)) ? eIds->GetNestedElement(i) : nullptr;
    }
    vtkSmartPointer<vtkAbstractArray> ids =
      eArray ? vtk::TakeSmartPointer(this->CreateArray(eArray)) : nullptr;
    vtkDataArray* sideIds = vtkDataArray::SafeDownCast(ids);
    if (!eIds->GetScalarAttribute("NumberOfSides", numberOfSides) ||
      !eIds->GetScalarAttribute("NumberOfHalfSides", numberOfHalfSides) || !sideIds)
    {
      vtkErrorMacro("The SideIds element of piece " << piece << " is incomplete.");
      this->DataError = 1;
      return;
    }
    sideIds->SetNumberOfTuples(numberOfHalfSides);
    if (!this->ReadArrayValues(eArray, 0, sideIds, 0, numberOfHalfSides))
    {
      vtkErrorMacro("Cannot read the side ids of piece " << piece << ".");
      this->DataError = 1;
      return;
    }
    if (!output->SetSideIds(sideIds, numberOfSides))
    {
      vtkErrorMacro("The side ids of piece " << piece << " do not fit its cells.");
      this->DataError = 1;
      return;
    }
  }

  if (vtkXMLDataElement* eSideData = this->SideDataElements[piece])
  {
    vtkSideData* sideData = output->GetSideData();
    const vtkIdType numberOfSides = output->GetNumberOfSides();
    for (int i = 0; i < eSideData->GetNumberOfNestedElements(); ++i)
    {
      vtkXMLDataElement* eArray = eSideData->GetNestedElement(i);
      const char* name = eArray->GetAttribute("Name");
      if (!forThisTimeStep(eArray) || !name || !this->SideDataArraySelection->ArrayIsEnabled(name))
      {
        continue;
      }
      vtkSmartPointer<vtkAbstractArray> array = vtk::TakeSmartPointer(this->CreateArray(eArray));
      if (!array)
      {
        vtkErrorMacro("Cannot create a side data array of piece " << piece << ".");
        this->DataError = 1;
        return;
      }
      array->SetNumberOfTuples(numberOfSides);
      if (!this->ReadArrayValues(
            eArray, 0, array, 0, numberOfSides * array->GetNumberOfComponents()))
      {
        vtkErrorMacro("Cannot read side data array \""
          << array->GetName() << "\" of piece " << piece << ", which should have " << numberOfSides
          << " tuples, one per side.");
        this->DataError = 1;
        return;
      }
      sideData->AddArray(array);
    }
    this->ReadAttributeIndices(eSideData, sideData);
  }
}

//------------------------------------------------------------------------------
void vtkXMLUnstructuredGridReader::SetupNextPiece()
{
  this->Superclass::SetupNextPiece();
  this->StartCell += this->NumberOfCells[this->Piece];
}

//------------------------------------------------------------------------------
int vtkXMLUnstructuredGridReader::ReadPieceData()
{
  // The amount of data read by the superclass's ReadPieceData comes
  // from point/cell data and point specifications (we read cell
  // specifications here).
  vtkIdType superclassPieceSize =
    ((this->NumberOfPointArrays + 1) * this->GetNumberOfPointsInPiece(this->Piece) +
      this->NumberOfCellArrays * this->GetNumberOfCellsInPiece(this->Piece));

  // Total amount of data in this piece comes from cell/face data arrays.
  // Three of them are for standard vtkUnstructuredGrid cell specification:
  // connectivities, offsets and types. Two optional arrays are for face
  // specification of polyhedron cells: faces and face offsets.
  // Note: We don't know exactly the array size of cell connectivities and
  // faces until we actually read the file. The following progress computation
  // assumes that each array cost the same time to read.
  vtkIdType totalPieceSize = superclassPieceSize + 5 * this->GetNumberOfCellsInPiece(this->Piece);
  if (totalPieceSize == 0)
  {
    totalPieceSize = 1;
  }

  // Split the progress range based on the approximate fraction of
  // data that will be read by each step in this method.  The cell
  // specification reads two arrays, and then the cell types array is
  // one more.
  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);
  float fractions[5] = { 0, float(superclassPieceSize) / totalPieceSize,
    ((float(superclassPieceSize) + 2 * this->GetNumberOfCellsInPiece(this->Piece)) /
      totalPieceSize),
    ((float(superclassPieceSize) + 3 * this->GetNumberOfCellsInPiece(this->Piece)) /
      totalPieceSize),
    1 };

  // Set the range of progress for the superclass.
  this->SetProgressRange(progressRange, 0, fractions);

  // Let the superclass read its data.
  if (!this->Superclass::ReadPieceData())
  {
    return 0;
  }

  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(this->GetCurrentOutput());

  // Set the range of progress for the cell specifications.
  this->SetProgressRange(progressRange, 1, fractions);

  // Read the Cells.
  vtkXMLDataElement* eCells = this->CellElements[this->Piece];
  if (!eCells)
  {
    if (this->NumberOfCells[this->Piece] > 0)
    {
      vtkErrorMacro("Cannot find cell arrays in piece " << this->Piece);
      return 0;
    }

    // An empty grid with no cells is allowed to have the DataArray child elements missing
    this->UpdateProgressDiscrete(progressRange[1]);
    return 1;
  }

  //  int needToRead = this->CellsNeedToReadTimeStep(eNested,
  //    this->CellsTimeStep, this->CellsOffset);
  //  if( needToRead )
  {
    // Read the array.
    if (!this->ReadCellArray(
          this->NumberOfCells[this->Piece], this->TotalNumberOfCells, eCells, output->GetCells()))
    {
      return 0;
    }
  }

  // Set the range of progress for the cell types.
  this->SetProgressRange(progressRange, 2, fractions);

  // Read the corresponding cell types.
  vtkIdType numberOfCells = this->NumberOfCells[this->Piece];
  if (numberOfCells > 0)
  {
    vtkXMLDataElement* eTypes = this->FindDataArrayWithName(eCells, "types");
    if (!eTypes)
    {
      vtkErrorMacro("Cannot read cell types from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"types\" array could not be found.");
      return 0;
    }
    vtkAbstractArray* ac2 = this->CreateArray(eTypes);
    vtkDataArray* c2 = vtkArrayDownCast<vtkDataArray>(ac2);
    if (!c2 || (c2->GetNumberOfComponents() != 1))
    {
      vtkErrorMacro("Cannot read cell types from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"types\" array could not be created"
        << " with one component.");
      if (ac2)
      {
        ac2->Delete();
      }
      return 0;
    }
    c2->SetNumberOfTuples(numberOfCells);
    if (!this->ReadArrayValues(eTypes, 0, c2, 0, numberOfCells))
    {
      vtkErrorMacro("Cannot read cell types from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"types\" array is not long enough.");
      return 0;
    }
    vtkUnsignedCharArray* cellTypes = this->ConvertToUnsignedCharArray(c2);
    if (!cellTypes)
    {
      vtkErrorMacro("Cannot read cell types from "
        << eCells->GetName() << " in piece " << this->Piece
        << " because the \"types\" array could not be converted"
        << " to a vtkUnsignedCharArray.");
      return 0;
    }

    // Copy the cell type data.
    std::copy_n(cellTypes->GetPointer(0), numberOfCells,
      vtkUnsignedCharArray::FastDownCast(output->GetCellTypes())->GetPointer(this->StartCell));

    // Permute node numbering on higher order hexahedra for legacy files (see
    // https://gitlab.kitware.com/vtk/vtk/-/merge_requests/6678 )
    if (this->GetFileMajorVersion() < 2 ||
      (this->GetFileMajorVersion() == 2 && this->GetFileMinorVersion() < 1))
    {
      vtkUpdateCellsV8toV9(output);
    }

    cellTypes->Delete();
  }

  // Set the range of progress for the faces.
  this->SetProgressRange(progressRange, 3, fractions);

  if (this->GetFileMajorVersion() < 2 ||
    (this->GetFileMajorVersion() == 2 && this->GetFileMinorVersion() < 3))
  {
    //
    // Read face array. Used for polyhedron mesh support. First need to
    // check if faces and faceoffsets arrays are available in this piece.
    if (!this->FindDataArrayWithName(eCells, "faces") ||
      !this->FindDataArrayWithName(eCells, "faceoffsets"))
    {
      if (vtkPolyhedronCellArray* polyhedra = output->GetPolyhedronCellArray())
      {
        // This piece doesn't have any polyhedron but other pieces that
        // we've already processed do, so its cells are added without face.
        polyhedra->InsertEmptyCells(numberOfCells);
      }
      return 1;
    }

    // By default vtkUnstructuredGrid does not contain face information, which is
    // only used by polyhedron cells. If so far no polyhedron cells have been
    // added, its polyhedron cell array is nullptr. In this case, we need to
    // initialize it, with the previous non-polyhedron cells without face.
    if (!output->GetPolyhedronCellArray())
    {
      output->InitializeFacesRepresentation(this->StartCell);
    }

    // Read face arrays.
    vtkPolyhedronCellArray* polyhedra = output->GetPolyhedronCellArray();
    if (!this->ReadFaceCellArray(this->NumberOfCells[this->Piece], eCells, polyhedra->GetFaces(),
          polyhedra->GetFaceLocations()))
    {
      return 0;
    }
  }
  else
  {
    // Support new file layout
    //
    // Read face array. Used for polyhedron mesh support. First need to
    // check if faces and faceoffsets arrays are available in this piece.
    if (!this->FindDataArrayWithName(eCells, "face_connectivity") ||
      !this->FindDataArrayWithName(eCells, "face_offsets") ||
      !this->FindDataArrayWithName(eCells, "polyhedron_to_faces") ||
      !this->FindDataArrayWithName(eCells, "polyhedron_offsets"))
    {
      if (vtkPolyhedronCellArray* polyhedra = output->GetPolyhedronCellArray())
      {
        // This piece doesn't have any polyhedron but other pieces that
        // we've already processed do, so its cells are added without face.
        polyhedra->InsertEmptyCells(numberOfCells);
      }
      return 1;
    }

    // By default vtkUnstructuredGrid does not contain face information, which is
    // only used by polyhedron cells. If so far no polyhedron cells have been
    // added, its polyhedron cell array is nullptr. In this case, we need to
    // initialize it, with the previous non-polyhedron cells without face.
    if (!output->GetPolyhedronCellArray())
    {
      output->InitializeFacesRepresentation(this->StartCell);
    }

    // Read face arrays.
    vtkPolyhedronCellArray* polyhedra = output->GetPolyhedronCellArray();
    if (!this->ReadPolyhedronCellArray(this->NumberOfCells[this->Piece], eCells,
          polyhedra->GetFaces(), polyhedra->GetFaceLocations()))
    {
      return 0;
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkXMLUnstructuredGridReader::ReadArrayForCells(
  vtkXMLDataElement* da, vtkAbstractArray* outArray)
{
  vtkIdType startCell = this->StartCell;
  vtkIdType numCells = this->NumberOfCells[this->Piece];
  vtkIdType components = outArray->GetNumberOfComponents();
  return this->ReadArrayValues(da, startCell * components, outArray, 0, numCells * components);
}

//------------------------------------------------------------------------------
int vtkXMLUnstructuredGridReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
  return 1;
}
VTK_ABI_NAMESPACE_END
