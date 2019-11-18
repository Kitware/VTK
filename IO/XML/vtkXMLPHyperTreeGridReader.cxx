/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPHyperTreeGridReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPHyperTreeGridReader.h"

#include "vtkCallbackCommand.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeCursor.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLHyperTreeGridReader.h"

#include <cassert>
#include <sstream>

vtkStandardNewMacro(vtkXMLPHyperTreeGridReader);

//----------------------------------------------------------------------------
vtkXMLPHyperTreeGridReader::vtkXMLPHyperTreeGridReader()
{
  this->PieceReaders = nullptr;
  this->TotalNumberOfPoints = 0;
  this->PieceStartIndex = 0;
}

//----------------------------------------------------------------------------
vtkXMLPHyperTreeGridReader::~vtkXMLPHyperTreeGridReader()
{
  if (this->NumberOfPieces)
  {
    this->DestroyPieces();
  }
}

//----------------------------------------------------------------------------
void vtkXMLPHyperTreeGridReader::CopyOutputInformation(vtkInformation* outInfo, int port)
{
  vtkInformation* localInfo = this->GetExecutive()->GetOutputInformation(port);

  if (localInfo->Has(CAN_HANDLE_PIECE_REQUEST()))
  {
    outInfo->CopyEntry(localInfo, CAN_HANDLE_PIECE_REQUEST());
  }
}

//----------------------------------------------------------------------------
void vtkXMLPHyperTreeGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkHyperTreeGrid* vtkXMLPHyperTreeGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkHyperTreeGrid* vtkXMLPHyperTreeGridReader::GetOutput(int idx)
{
  return vtkHyperTreeGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
const char* vtkXMLPHyperTreeGridReader::GetDataSetName()
{
  return "PHyperTreeGrid";
}

//----------------------------------------------------------------------------
void vtkXMLPHyperTreeGridReader::GetOutputUpdateExtent(int& piece, int& numberOfPieces)
{
  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numberOfPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
}

//----------------------------------------------------------------------------
void vtkXMLPHyperTreeGridReader::SetupOutputTotals()
{
  this->TotalNumberOfPoints = 0;
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
  {
    if (this->PieceReaders[i])
    {
      this->TotalNumberOfPoints += this->PieceReaders[i]->GetNumberOfPoints();
    }
  }
}

//----------------------------------------------------------------------------
void vtkXMLPHyperTreeGridReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();
  this->GetCurrentOutput()->Initialize();
}

//----------------------------------------------------------------------------
int vtkXMLPHyperTreeGridReader::ReadPieceData(int index)
{
  this->Piece = index;

  // We need data, make sure the piece can be read.
  if (!this->CanReadPiece(this->Piece))
  {
    vtkErrorMacro("File for piece " << this->Piece << " cannot be read.");
    return 0;
  }

  // Actually read the data.
  this->PieceReaders[this->Piece]->SetAbortExecute(0);

  return this->ReadPieceData();
}

//----------------------------------------------------------------------------
int vtkXMLPHyperTreeGridReader::CanReadPiece(int index)
{
  // If necessary, test whether the piece can be read.
  vtkXMLHyperTreeGridReader* reader = this->PieceReaders[index];
  if (reader && !this->CanReadPieceFlag[index])
  {
    if (reader->CanReadFile(reader->GetFileName()))
    {
      // We can read the piece.  Save result to avoid later repeat of
      // test.
      this->CanReadPieceFlag[index] = 1;
    }
    else
    {
      // We cannot read the piece.  Destroy the reader to avoid later
      // repeat of test.
      this->PieceReaders[index] = nullptr;
      reader->Delete();
    }
  }

  return (this->PieceReaders[index] ? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkXMLPHyperTreeGridReader::PieceProgressCallback()
{
  float width = this->ProgressRange[1] - this->ProgressRange[0];
  float pieceProgress = this->PieceReaders[this->Piece]->GetProgress();
  float progress = this->ProgressRange[0] + pieceProgress * width;
  this->UpdateProgressDiscrete(progress);
  if (this->AbortExecute)
  {
    this->PieceReaders[this->Piece]->SetAbortExecute(1);
  }
}

//----------------------------------------------------------------------------
void vtkXMLPHyperTreeGridReader::SetupNextPiece() {}

namespace
{
//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void vtkXMLPHyperTreeGridAppendExecute(T* inPtr, T* outPtr, vtkIdType numTuple, vtkIdType numComp)
{
  for (vtkIdType nt = 0; nt < numTuple; nt++)
  {
    for (vtkIdType nc = 0; nc < numComp; nc++)
    {
      outPtr[nt * numComp + nc] = inPtr[nt * numComp + nc];
    }
  }
}
}

//----------------------------------------------------------------------------
int vtkXMLPHyperTreeGridReader::ReadPieceData()
{
  // Use the internal reader to read the piece.
  this->PieceReaders[this->Piece]->UpdatePiece(0, 1, 0);

  // Collect hypertree grid from the piece reader and the parallel output
  vtkHyperTreeGrid* input = this->GetPieceInputAsHyperTreeGrid(this->Piece);
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(this->GetCurrentOutput());

  if (!output)
  {
    vtkErrorMacro("Incorrect type of output: " << output->GetClassName());
    return 0;
  }

  // Set the attributes on hypertree grid using the header of the first Piece
  if (this->Piece == this->StartPiece)
  {
    // output->SetDimension(input->GetDimension());
    // output->SetOrientation(input->GetOrientation());
    output->SetBranchFactor(input->GetBranchFactor());
    output->SetTransposedRootIndexing(input->GetTransposedRootIndexing());
    output->SetDimensions(input->GetDimensions());

    output->SetXCoordinates(input->GetXCoordinates());
    output->SetYCoordinates(input->GetYCoordinates());
    output->SetZCoordinates(input->GetZCoordinates());
  }

  // Setup for iteration on input piece to copy to output grid
  vtkIdType inIndex;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator(it);
  vtkNew<vtkHyperTreeGridNonOrientedCursor> inCursor;
  vtkNew<vtkHyperTreeGridNonOrientedCursor> outCursor;

  // For this piece save the initial offset for copying in scalar data from input
  // Can do this because order is maintained building of structure
  int currentOffset = this->PieceStartIndex;

  // Iterate over every hypertree in the input piece
  // Transfer to output hypertree grid
  while (it.GetNextTree(inIndex))
  {
    // Initialize new grid cursor at root of current input tree
    input->InitializeNonOrientedCursor(inCursor, inIndex, true);
    vtkIdType numberOfVertices = inCursor->GetTree()->GetNumberOfVertices();

    // Get the global offset of this tree within the piece
    int globalOffset = inCursor->GetTree()->GetGlobalIndexFromLocal(0);

    // Initialize new cursor at root of current output tree
    output->InitializeNonOrientedCursor(outCursor, inIndex, true);
    outCursor->SetGlobalIndexStart(this->PieceStartIndex + globalOffset);
    currentOffset += numberOfVertices;

    // Process tree to recursively build
    this->RecursivelyProcessTree(inCursor, outCursor);
  }

  for (vtkIdType i = 0; i < input->GetPointData()->GetNumberOfArrays(); i++)
  {
    vtkAbstractArray* inArray = input->GetPointData()->GetAbstractArray(i);
    vtkAbstractArray* outArray = output->GetPointData()->GetAbstractArray(i);
    if (outArray == nullptr)
    {
      // Create output PointData array
      outArray = inArray->NewInstance();
      outArray->SetName(inArray->GetName());
      outArray->SetNumberOfComponents(inArray->GetNumberOfComponents());
      outArray->SetNumberOfTuples(this->TotalNumberOfPoints);
      output->GetPointData()->AddArray(outArray);
      outArray->Delete();
    }

    vtkIdType numComp = inArray->GetNumberOfComponents();
    vtkIdType numTuple = inArray->GetNumberOfTuples();

    if (numComp != outArray->GetNumberOfComponents())
    {
      vtkErrorMacro("Components of the inputs do not match");
      return 0;
    }

    // Input and output type must match
    if (inArray->GetDataType() != outArray->GetDataType())
    {
      vtkErrorMacro(<< "Execute: input" << this->Piece << " ScalarType (" << inArray->GetDataType()
                    << "), must match output ScalarType (" << outArray->GetDataType() << ")");
      return 0;
    }

    // Input and output name must match
    if (strcmp(inArray->GetName(), outArray->GetName()))
    {
      vtkErrorMacro(<< "Execute: input" << this->Piece << " Name (" << inArray->GetName()
                    << "), must match output Name (" << outArray->GetName() << ")");
      return 0;
    }

    // Copy all scalar data from input piece to the correct offset in outputpiece
    // All hypertrees from this piece were built in order so copy is in order
    void* inPtr = inArray->GetVoidPointer(0);
    void* outPtr = outArray->GetVoidPointer(this->PieceStartIndex);

    // Copies the data from piece to parallel output
    switch (inArray->GetDataType())
    {
      vtkTemplateMacro(vtkXMLPHyperTreeGridAppendExecute(
        static_cast<VTK_TT*>(inPtr), static_cast<VTK_TT*>(outPtr), numTuple, numComp));
      default:
        vtkErrorMacro(<< "Execute: Unknown ScalarType");
        return 0;
    }
  }

  this->PieceStartIndex = currentOffset;
  return 1;
}

//----------------------------------------------------------------------------
vtkXMLHyperTreeGridReader* vtkXMLPHyperTreeGridReader::CreatePieceReader()
{
  return vtkXMLHyperTreeGridReader::New();
}

//----------------------------------------------------------------------------
int vtkXMLPHyperTreeGridReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLPHyperTreeGridReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return this->Superclass::RequestInformation(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
vtkHyperTreeGrid* vtkXMLPHyperTreeGridReader::GetOutputAsHyperTreeGrid()
{
  return vtkHyperTreeGrid::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
vtkHyperTreeGrid* vtkXMLPHyperTreeGridReader::GetPieceInputAsHyperTreeGrid(int piece)
{
  vtkXMLHyperTreeGridReader* reader = this->PieceReaders[piece];
  if (!reader || reader->GetNumberOfOutputPorts() < 1)
  {
    return nullptr;
  }
  return static_cast<vtkHyperTreeGrid*>(reader->GetExecutive()->GetOutputData(0));
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPHyperTreeGridReader::GetNumberOfPoints()
{
  return this->TotalNumberOfPoints;
}

//----------------------------------------------------------------------------
void vtkXMLPHyperTreeGridReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

//----------------------------------------------------------------------------
void vtkXMLPHyperTreeGridReader::SetupOutputInformation(vtkInformation* vtkNotUsed(outInfo))
{
  if (this->InformationError)
  {
    vtkErrorMacro("Should not still be processing output information if have set InformationError");
    return;
  };
}

//----------------------------------------------------------------------------
void vtkXMLPHyperTreeGridReader::ReadXMLData()
{
  // Get the update request.
  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numberOfPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  vtkDebugMacro("Updating piece " << piece << " of " << numberOfPieces);

  // Setup the range of pieces that will be read and collect the number of points of scalar data
  this->SetupUpdateExtent(piece, numberOfPieces);

  // If there are no data to read, stop now.
  if (this->StartPiece == this->EndPiece)
  {
    return;
  }

  vtkDebugMacro(
    "Reading piece range [" << this->StartPiece << ", " << this->EndPiece << ") from file.");

  // Superclass ReadXMLData will call SetupOutputData which does the hypertree grid Initialize()
  this->Superclass::ReadXMLData();

  // Split current progress range based on fraction contributed by
  // each piece.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);

  // Calculate the cumulative fraction of data contributed by each
  // piece (for progress).
  std::vector<float> fractions(this->EndPiece - this->StartPiece + 1);
  fractions[0] = 0;
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
  {
    int index = i - this->StartPiece;
    fractions[index + 1] = (fractions[index] + this->GetNumberOfPointsInPiece(i));
  }
  if (fractions[this->EndPiece - this->StartPiece] == 0)
  {
    fractions[this->EndPiece - this->StartPiece] = 1;
  }
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
  {
    int index = i - this->StartPiece;
    fractions[index + 1] = fractions[index + 1] / fractions[this->EndPiece - this->StartPiece];
  }

  // Read the data needed from each piece.
  for (int i = this->StartPiece; (i < this->EndPiece && !this->AbortExecute && !this->DataError);
       ++i)
  {
    // Set the range of progress for this piece.
    this->SetProgressRange(progressRange, i - this->StartPiece, fractions.data());

    if (!this->ReadPieceData(i))
    {
      // An error occurred while reading the piece.
      this->DataError = 1;
    }
    this->SetupNextPiece();
  }
}

//----------------------------------------------------------------------------
int vtkXMLPHyperTreeGridReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{
  if (!this->Superclass::ReadPrimaryElement(ePrimary))
  {
    return 0;
  }

  // Read information about the pieces.
  int numNested = ePrimary->GetNumberOfNestedElements();
  int numPieces = 0;
  for (int i = 0; i < numNested; ++i)
  {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if (strcmp(eNested->GetName(), "Piece") == 0)
    {
      ++numPieces;
    }
  }
  this->SetupPieces(numPieces);
  int piece = 0;
  for (int i = 0; i < numNested; ++i)
  {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if (strcmp(eNested->GetName(), "Piece") == 0)
    {
      if (!this->ReadPiece(eNested, piece++))
      {
        return 0;
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLPHyperTreeGridReader::SetupUpdateExtent(int piece, int numberOfPieces)
{
  this->UpdatePiece = piece;
  this->UpdateNumberOfPieces = numberOfPieces;

  // If more pieces are requested than available, just return empty
  // pieces for the extra ones
  if (this->UpdateNumberOfPieces > this->NumberOfPieces)
  {
    this->UpdateNumberOfPieces = this->NumberOfPieces;
  }

  // Find the range of pieces to read.
  if (this->UpdatePiece < this->UpdateNumberOfPieces)
  {
    this->StartPiece = (this->UpdatePiece * this->NumberOfPieces) / this->UpdateNumberOfPieces;
    this->EndPiece = ((this->UpdatePiece + 1) * this->NumberOfPieces) / this->UpdateNumberOfPieces;
  }
  else
  {
    this->StartPiece = 0;
    this->EndPiece = 0;
  }

  // Update the information of the pieces we need.
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
  {
    if (this->CanReadPiece(i))
    {
      this->PieceReaders[i]->UpdateInformation();
      vtkXMLHyperTreeGridReader* pReader = this->PieceReaders[i];
      pReader->SetupUpdateExtent(0, 1);
    }
  }

  // Find the total size of the output
  this->SetupOutputTotals();
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPHyperTreeGridReader::GetNumberOfPointsInPiece(int piece)
{
  return this->PieceReaders[piece] ? this->PieceReaders[piece]->GetNumberOfPoints() : 0;
}

//----------------------------------------------------------------------------
void vtkXMLPHyperTreeGridReader::SetupPieces(int numPieces)
{
  this->Superclass::SetupPieces(numPieces);

  this->PieceReaders = new vtkXMLHyperTreeGridReader*[this->NumberOfPieces];

  for (int i = 0; i < this->NumberOfPieces; ++i)
  {
    this->PieceReaders[i] = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkXMLPHyperTreeGridReader::DestroyPieces()
{
  for (int i = 0; i < this->NumberOfPieces; ++i)
  {
    if (this->PieceReaders[i])
    {
      this->PieceReaders[i]->RemoveObserver(this->PieceProgressObserver);
      this->PieceReaders[i]->Delete();
    }
  }

  delete[] this->PieceReaders;
  this->PieceReaders = nullptr;

  this->Superclass::DestroyPieces();
}

//----------------------------------------------------------------------------
int vtkXMLPHyperTreeGridReader::ReadPiece(vtkXMLDataElement* ePiece)
{
  this->PieceElements[this->Piece] = ePiece;

  const char* fileName = ePiece->GetAttribute("Source");
  if (!fileName)
  {
    vtkErrorMacro("Piece " << this->Piece << " has no Source attribute.");
    return 0;
  }

  // The file name is relative to the summary file.  Convert it to
  // something we can use.
  char* pieceFileName = this->CreatePieceFileName(fileName);

  vtkXMLHyperTreeGridReader* reader = this->CreatePieceReader();
  this->PieceReaders[this->Piece] = reader;
  this->PieceReaders[this->Piece]->AddObserver(
    vtkCommand::ProgressEvent, this->PieceProgressObserver);
  reader->SetFileName(pieceFileName);

  delete[] pieceFileName;

  return 1;
}

//-----------------------------------------------------------------------------
void vtkXMLPHyperTreeGridReader::RecursivelyProcessTree(
  vtkHyperTreeGridNonOrientedCursor* inCursor, vtkHyperTreeGridNonOrientedCursor* outCursor)
{
  // Retrieve input grid
  vtkHyperTreeGrid* input = inCursor->GetGrid();

  // Descend further into input trees only if cursor is not at leaf
  if (!inCursor->IsLeaf())
  {
    // Cursor is not at leaf, subdivide output tree one level further
    // PKF outTree->SubdivideLeaf( outCursor );
    outCursor->SubdivideLeaf();

    // If input cursor is neither at leaf nor at maximum depth, recurse to all children
    int numChildren = input->GetNumberOfChildren();
    for (int child = 0; child < numChildren; ++child)
    {
      // Create child cursor from parent in input grid
      // PKF vtkHyperTreeGridCursor* childCursor = inCursor->Clone();
      vtkHyperTreeGridNonOrientedCursor* childCursor = inCursor->Clone();
      childCursor->ToChild(child);

      // Descend into child in output grid as well
      outCursor->ToChild(child);

      // Recurse and keep track of whether some children are kept
      this->RecursivelyProcessTree(childCursor, outCursor);

      // Return to parent in output grid
      outCursor->ToParent();

      // Clean up
      childCursor->Delete();
      childCursor = nullptr;
    }
  }
}
