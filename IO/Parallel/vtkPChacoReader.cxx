/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPChacoReader.cxx

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
#include "vtkPChacoReader.h"
#include "vtkDataSetWriter.h"
#include "vtkDataSetReader.h"
#include "vtkExtractCells.h"
#include "vtkPKdTree.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDoubleArray.h"
#include "vtkCharArray.h"
#include "vtkIntArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkMultiProcessController.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkProcessGroup.h"

vtkStandardNewMacro(vtkPChacoReader);

//----------------------------------------------------------------------------
vtkPChacoReader::vtkPChacoReader()
{
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkPChacoReader::~vtkPChacoReader()
{
  this->SetController(NULL);
}
//----------------------------------------------------------------------------
void vtkPChacoReader::SetController(vtkMultiProcessController *c)
{
  if ((c == NULL) || (c->GetNumberOfProcesses() == 0))
    {
    this->NumProcesses = 1;
    this->MyId = 0;
    }

  if (this->Controller == c)
    {
    return;
    }

  this->Modified();

  if (this->Controller != NULL)
    {
    this->Controller->UnRegister(this);
    this->Controller = NULL;
    }

  if (c == NULL)
    {
    return;
    }

  this->Controller = c;

  c->Register(this);
  this->NumProcesses = c->GetNumberOfProcesses();
  this->MyId    = c->GetLocalProcessId();
}

//----------------------------------------------------------------------------
int vtkPChacoReader::RequestInformation(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  if (!this->BaseName)
    {
    vtkErrorMacro(<< "No BaseName specified");
    return 0;
    }

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  int retVal = 1;

  if (this->MyId == 0)
    {
    retVal =
      this->Superclass::RequestInformation(request, inputVector, outputVector);
    }

  if (this->NumProcesses == 1)
    {
    return retVal;
    }

  unsigned long metadata[8];

  if (this->MyId == 0)
    {
    metadata[0] = static_cast<unsigned long>(retVal);

    if (retVal)
      {
      metadata[1] = static_cast<unsigned long>(this->RemakeDataCacheFlag);
      if (this->RemakeDataCacheFlag)
        {
        metadata[2] = static_cast<unsigned long>(this->Dimensionality);
        metadata[3] = static_cast<unsigned long>(this->NumberOfVertices);
        metadata[4] = static_cast<unsigned long>(this->NumberOfEdges);
        metadata[5] = static_cast<unsigned long>(this->NumberOfVertexWeights);
        metadata[6] = static_cast<unsigned long>(this->NumberOfEdgeWeights);
        metadata[7] = static_cast<unsigned long>(this->GraphFileHasVertexNumbers);
        }
      }
    }

  this->Controller->Broadcast(metadata, 8, 0);

  if (this->MyId > 0)
    {
    retVal = metadata[0];
    if (retVal)
      {
      this->RemakeDataCacheFlag       = static_cast<int>(metadata[1]);
      if (this->RemakeDataCacheFlag)
        {
        this->Dimensionality            = static_cast<int>(metadata[2]);
        this->NumberOfVertices          = static_cast<vtkIdType>(metadata[3]);
        this->NumberOfEdges             = static_cast<vtkIdType>(metadata[4]);
        this->NumberOfVertexWeights     = static_cast<int>(metadata[5]);
        this->NumberOfEdgeWeights       = static_cast<int>(metadata[6]);
        this->GraphFileHasVertexNumbers = static_cast<int>(metadata[7]);

        this->MakeWeightArrayNames(this->NumberOfVertexWeights,
                                   this->NumberOfEdgeWeights);

        this->SetCurrentBaseName(this->BaseName);
        }
      }
    }
  return retVal;
}


//----------------------------------------------------------------------------
int vtkPChacoReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  if (!this->BaseName)
    {
    vtkErrorMacro(<< "No BaseName specified");
    return 0;
    }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int piece =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  int pieceZeroProc = 0;

  vtkMultiProcessController *contr = this->Controller;

  int i=0;

  int oops = ((piece != this->MyId) || (numPieces != this->NumProcesses));
  int sum = 0;

  contr->Reduce(&oops, &sum, 1, vtkCommunicator::SUM_OP, 0);
  contr->Broadcast(&sum, 1, 0);

  if (sum > 0)
    {
    // I don't know if this situation can occur, but we'll try to handle it.

    int *myPiece = new int [this->NumProcesses];

    contr->AllGather(&piece, myPiece, 1);

    vtkMultiProcessController *subController;
    vtkProcessGroup *group = vtkProcessGroup::New();

    group->Initialize(contr);

    int nparticipants = 0;

    for (i=0; i<this->NumProcesses; i++)
      {
      if ((myPiece[i] >= 0) && (myPiece[i] < numPieces))
        {
        group->AddProcessId(i);
        if (myPiece[i] == 0)
          {
          pieceZeroProc = nparticipants;
          }
        nparticipants++;
        }
      }

    delete [] myPiece;

    if (nparticipants < numPieces) // Can this happen?
      {
      group->Delete();
      output->Initialize();
      vtkErrorMacro("<<vtkPChacoReader can't produce less than entire file");
      return 0;
      }

    subController = contr->CreateSubController(group);
    group->Delete();

    if (subController)
      {
      contr = subController;
      }
    else
      {
      contr = NULL;
      }
    }

  if ( !contr)
    {
    // This process doesn't participate (can this happen?)

    this->SetUpEmptyGrid(output);
    return 1;
    }

  int retVal = 1;

  if (piece == 0)
    {
    // "piece" 0 reads in the entire mesh

    retVal = this->BuildOutputGrid(output);
    }

  if (numPieces > 1)
    {
    contr->Broadcast(&retVal, 1, pieceZeroProc);

    if (retVal == 1)
      {
      retVal = this->DivideCells(contr, output, pieceZeroProc);
      }
    }

  if (contr != this->Controller)
    {
    contr->Delete();
    }

  return retVal;
}
//----------------------------------------------------------------------------
void vtkPChacoReader::SetUpEmptyGrid(vtkUnstructuredGrid *output)
{
  int i;
  // Note: The cell and point arrays should be added in the same order in
  // which they are added in vtkChacoReader::BuildOutputGrid.  See "Note" in
  // vtkChacoReader.cxx.

  output->Initialize();

  if (this->GetGenerateVertexWeightArrays())
    {
    for (i=0; i < this->NumberOfVertexWeights; i++)
      {
      vtkDoubleArray *da = vtkDoubleArray::New();
      da->SetNumberOfTuples(0);
      da->SetNumberOfComponents(1);
      da->SetName(this->GetVertexWeightArrayName(i+1));
      output->GetPointData()->AddArray(da);
      da->Delete();
      }

    this->NumberOfPointWeightArrays = this->NumberOfVertexWeights;
    }

  if (this->GetGenerateEdgeWeightArrays())
    {
    for (i=0; i < this->NumberOfEdgeWeights; i++)
      {
      vtkDoubleArray *da = vtkDoubleArray::New();
      da->SetNumberOfTuples(0);
      da->SetNumberOfComponents(1);
      da->SetName(this->GetEdgeWeightArrayName(i+1));
      output->GetCellData()->AddArray(da);
      da->Delete();
      }

    this->NumberOfCellWeightArrays = this->NumberOfEdgeWeights;
    }

  if (this->GetGenerateGlobalElementIdArray())
    {
    vtkIntArray *ia = vtkIntArray::New();
    ia->SetNumberOfTuples(0);
    ia->SetNumberOfComponents(1);
    ia->SetName(this->GetGlobalElementIdArrayName());
    output->GetCellData()->AddArray(ia);
    }

  if (this->GetGenerateGlobalNodeIdArray())
    {
    vtkIntArray *ia = vtkIntArray::New();
    ia->SetNumberOfTuples(0);
    ia->SetNumberOfComponents(1);
    ia->SetName(this->GetGlobalNodeIdArrayName());
    output->GetPointData()->AddArray(ia);
    }
}
//----------------------------------------------------------------------------
int vtkPChacoReader::DivideCells(vtkMultiProcessController *contr,
                                  vtkUnstructuredGrid *output, int source)
{
  int retVal = 1;

  int i=0;

  int nprocs = contr->GetNumberOfProcesses();
  int myrank = contr->GetLocalProcessId();

  vtkUnstructuredGrid *mygrid = NULL;

  if (source == myrank)
    {
    vtkIdType ntotalcells = output->GetNumberOfCells();
    vtkIdType nshare = ntotalcells / nprocs;
    vtkIdType leftover = ntotalcells - (nprocs * nshare);

    vtkIdType startId = 0;

    for (i=0; i < nprocs; i++)
      {
      if (!retVal && (i != myrank))
        {
        this->SendGrid(contr, i, NULL);  // we failed
        continue;
        }

      vtkIdType ncells = ((i < leftover) ? nshare + 1 : nshare);

      vtkIdType endId = startId + ncells - 1;

      vtkUnstructuredGrid *ug = this->SubGrid(output, startId, endId);

      if (i != myrank)
        {
        retVal = this->SendGrid(contr, i, ug);
        ug->Delete();
        }
      else
        {
        mygrid = ug;
        }
      startId += ncells;
      }
    }
  else
    {
    mygrid = this->GetGrid(contr, source);
    if (mygrid == NULL)
      {
      retVal = 0;
      }
    }

  int vote = 0;
  contr->Reduce(&retVal, &vote, 1, vtkCommunicator::SUM_OP, 0);
  contr->Broadcast(&vote, 1, 0);

  if (vote < nprocs)
    {
    retVal = 0;
    }

  output->Initialize();

  if (retVal)
    {
    output->ShallowCopy(mygrid);
    }
  else if (mygrid)
    {
    mygrid->Delete();
    }

  return retVal;
}
int vtkPChacoReader::SendGrid(vtkMultiProcessController *contr,
                              int to, vtkUnstructuredGrid *grid)
{
  int retVal = 1;

  int bufsize=0, ack = 0;

  if (!grid)
    {
    // sending notice of a failure

    contr->Send(&bufsize, 1, to, 0x11);
    return retVal;
    }

  char *buf = this->MarshallDataSet(grid, bufsize);

  contr->Send(&bufsize, 1, to, 0x11);

  contr->Receive(&ack, 1, to, 0x12);

  if (!ack)
    {
    retVal = 0;
    }
  else
    {
    contr->Send(buf, bufsize, to, 0x13);
    }

  delete [] buf;
  return retVal;
}
vtkUnstructuredGrid *vtkPChacoReader::GetGrid(vtkMultiProcessController *contr,
                                              int from)
{
  vtkUnstructuredGrid *grid = NULL;

  int bufsize=0, ack=1;

  contr->Receive(&bufsize, 1, from, 0x11);

  if (bufsize == 0)
    {
    // Node zero is reporting an error
    return NULL;
    }

  char *buf = new char [bufsize];

  if (buf)
    {
    contr->Send(&ack, 1, from, 0x12);
    contr->Receive(buf, bufsize, from, 0x13);
    grid = this->UnMarshallDataSet(buf, bufsize);
    delete [] buf;
    }
  else
    {
    ack = 0;
    contr->Send(&ack, 1, 0, 0x12);
    }
  return grid;
}

vtkUnstructuredGrid  *
  vtkPChacoReader::SubGrid(vtkUnstructuredGrid *ug, vtkIdType from, vtkIdType to)
{
  vtkUnstructuredGrid *tmp = vtkUnstructuredGrid::New();

  if (from > to)
    {
    this->SetUpEmptyGrid(tmp);
    }
  else
    {
    tmp->ShallowCopy(ug);

    vtkExtractCells *ec = vtkExtractCells::New();
    ec->AddCellRange(from, to);
    ec->SetInputData(tmp);
    ec->Update();

    tmp->Initialize();
    tmp->ShallowCopy(ec->GetOutput());
    ec->Delete();
    }

  return tmp;
}
char *vtkPChacoReader::MarshallDataSet(vtkUnstructuredGrid *extractedGrid, int &len)
{
  // taken from vtkCommunicator::WriteDataSet

  vtkUnstructuredGrid *copy;
  vtkDataSetWriter *writer = vtkDataSetWriter::New();

  copy = extractedGrid->NewInstance();
  copy->ShallowCopy(extractedGrid);

  // There is a problem with binary files with no data.
  if (copy->GetNumberOfCells() > 0)
    {
    writer->SetFileTypeToBinary();
    }
  writer->WriteToOutputStringOn();
  writer->SetInputData(copy);

  writer->Write();

  len = writer->GetOutputStringLength();

  char *packedFormat = writer->RegisterAndGetOutputString();

  writer->Delete();

  copy->Delete();

  return packedFormat;
}
vtkUnstructuredGrid *vtkPChacoReader::UnMarshallDataSet(char *buf, int size)
{
  // taken from vtkCommunicator::ReadDataSet

  vtkDataSetReader *reader = vtkDataSetReader::New();

  reader->ReadFromInputStringOn();

  vtkCharArray* mystring = vtkCharArray::New();

  mystring->SetArray(buf, size, 1);

  reader->SetInputArray(mystring);
  mystring->Delete();

  vtkDataSet *output = reader->GetOutput();
  reader->Update();

  vtkUnstructuredGrid *newGrid = vtkUnstructuredGrid::New();

  newGrid->ShallowCopy(output);

  reader->Delete();

  return newGrid;
}
//----------------------------------------------------------------------------
void vtkPChacoReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkChacoReader::PrintSelf(os,indent);
  os << indent << "MyId: " << this->MyId << endl;
  os << indent << "NumProcesses: " << this->NumProcesses << endl;
  os << indent << "Controller: " << this->Controller << endl;
}

