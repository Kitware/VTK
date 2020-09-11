/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPPartitionedDataSetWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPPartitionedDataSetWriter.h"

#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkXMLDataElement.h"

#include <sstream>
#include <vector>

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkXMLPPartitionedDataSetWriter);

vtkCxxSetObjectMacro(vtkXMLPPartitionedDataSetWriter, Controller, vtkMultiProcessController);

class vtkXMLPPartitionedDataSetWriter::vtkInternal
{
public:
  vtkInternal() = default;
  ~vtkInternal() = default;
  void Allocate(int numPieces, int numProcs)
  {
    this->NumberOfPieces = numPieces;
    this->NumberOfProcesses = numProcs;
    this->PieceProcessList.resize(numPieces * numProcs);
  }

  void GetPieceProcessList(int piece, int* processList)
  {
    if (this->PieceProcessList.empty() || piece >= this->NumberOfPieces || piece < 0)
    {
      return;
    }
    for (int i = 0; i < this->NumberOfProcesses; i++)
    {
      processList[i] = this->PieceProcessList[piece + i * this->NumberOfPieces];
    }
  }

  // For each piece it keeps the processes that have that piece.
  // This is built and used only on the root node.
  // PieceProcessList[piece+NumPieces*process] = dataset type (-1 for nullptr)
  // This NumberOfPieces is based on the number of blocks in the multiblock
  // which is different than the vtkXMLPPartitionedDataSetWriter::NumberOfPieces
  // which is usually the number of parallel processes.
  std::vector<int> PieceProcessList;
  int NumberOfPieces;
  int NumberOfProcesses;
};

//------------------------------------------------------------------------------
vtkXMLPPartitionedDataSetWriter::vtkXMLPPartitionedDataSetWriter()
{
  this->StartPiece = 0;
  this->NumberOfPieces = 1;
  this->XMLPPartitionedDataSetWriterInternal = new vtkInternal();
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->SetWriteMetaFile(1);
}

//------------------------------------------------------------------------------
vtkXMLPPartitionedDataSetWriter::~vtkXMLPPartitionedDataSetWriter()
{
  this->SetController(nullptr);
  delete this->XMLPPartitionedDataSetWriterInternal;
}

//------------------------------------------------------------------------------
void vtkXMLPPartitionedDataSetWriter::SetWriteMetaFile(int flag)
{
  this->Modified();
  if (this->Controller == nullptr || this->Controller->GetLocalProcessId() == 0)
  {
    if (this->WriteMetaFile != flag)
    {
      this->WriteMetaFile = flag;
    }
  }
  else
  {
    this->WriteMetaFile = 0;
  }
}

//------------------------------------------------------------------------------
void vtkXMLPPartitionedDataSetWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Controller: ";
  if (this->Controller)
  {
    this->Controller->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << "\n";
  os << indent << "StartPiece: " << this->StartPiece << "\n";
}

//------------------------------------------------------------------------------
vtkTypeBool vtkXMLPPartitionedDataSetWriter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), this->NumberOfPieces);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), this->StartPiece);
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), this->GhostLevel);
    return 1;
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
void vtkXMLPPartitionedDataSetWriter::FillDataTypes(vtkCompositeDataSet* hdInput)
{
  // FillDataTypes is called before the actual data writing begins.
  // Every process fills up an array with the data types for all the leaf nodes.
  // (Since the composite data structure is same on all the processes, the
  // number of leaf nodes is same on all processes as well).
  // Then we gather this list on to the root node, since the root node is the
  // one that is writing out the vtmb file.
  this->Superclass::FillDataTypes(hdInput);
  if (!this->Controller)
  {
    return;
  }

  unsigned int numBlocks = this->GetNumberOfDataTypes();
  int* myDataTypes = this->GetDataTypesPointer();

  this->XMLPPartitionedDataSetWriterInternal->Allocate(
    numBlocks, this->Controller->GetNumberOfProcesses());

  // gather on to root node.
  if (numBlocks)
  {
    this->Controller->Gather(
      myDataTypes, &this->XMLPPartitionedDataSetWriterInternal->PieceProcessList[0], numBlocks, 0);
  }
}

//------------------------------------------------------------------------------
int vtkXMLPPartitionedDataSetWriter::WriteComposite(
  vtkCompositeDataSet* compositeData, vtkXMLDataElement* parent, int& writerIdx)
{
  if (!compositeData->IsA("vtkPartitionedDataSet"))
  {
    vtkErrorMacro("Unsupported composite dataset type: " << compositeData->GetClassName() << ".");
    return 0;
  }

  // Write each input.
  vtkSmartPointer<vtkDataObjectTreeIterator> iter;
  iter.TakeReference(vtkDataObjectTree::SafeDownCast(compositeData)->NewTreeIterator());
  iter->VisitOnlyLeavesOff();
  iter->TraverseSubTreeOff();
  iter->SkipEmptyNodesOff();
  int toBeWritten = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    toBeWritten++;
  }

  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);

  int index = 0;
  int RetVal = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem(), index++)
  {
    vtkDataObject* curDO = iter->GetCurrentDataObject();

    vtkXMLDataElement* datasetXML = vtkXMLDataElement::New();
    datasetXML->SetName("DataSet");
    datasetXML->SetIntAttribute("index", index);
    vtkStdString fileName = this->CreatePieceFileName(writerIdx);

    this->SetProgressRange(progressRange, writerIdx, toBeWritten);
    if (this->ParallelWriteNonCompositeData(curDO, parent, writerIdx))
    {
      //     parent->AddNestedElement(datasetXML);
      RetVal = 1;
    }
    datasetXML->Delete();
  }
  return RetVal;
}

//------------------------------------------------------------------------------
int vtkXMLPPartitionedDataSetWriter::ParallelWriteNonCompositeData(
  vtkDataObject* dObj, vtkXMLDataElement* parentXML, int currentFileIndex)
{
  int myProcId = this->Controller->GetLocalProcessId();
  if (myProcId == 0)
  {
    // pieceProcessList is a list where index is the process number and value is
    // the data-type for the current leaf on that process.
    int numberOfProcesses = this->Controller->GetNumberOfProcesses();
    std::vector<int> pieceProcessList(numberOfProcesses);
    this->XMLPPartitionedDataSetWriterInternal->GetPieceProcessList(
      currentFileIndex, &pieceProcessList[0]);

    int indexCounter = 0;
    for (int procId = 0; procId < numberOfProcesses; procId++)
    {
      if (pieceProcessList[procId] >= 0)
      {
        vtkXMLDataElement* datasetXML = parentXML;
        datasetXML = vtkXMLDataElement::New();
        datasetXML->SetName("DataSet");
        datasetXML->SetIntAttribute("index", indexCounter);
        parentXML->AddNestedElement(datasetXML);
        datasetXML->Delete();
        indexCounter++;
        vtkStdString fName =
          this->CreatePieceFileName(currentFileIndex, procId, pieceProcessList[procId]);
        datasetXML->SetAttribute("file", fName.c_str());
      }
    }
  }

  const int* datatypes_ptr = this->GetDataTypesPointer();
  if (dObj && datatypes_ptr[currentFileIndex] != -1)
  {
    vtkStdString fName =
      this->CreatePieceFileName(currentFileIndex, myProcId, datatypes_ptr[currentFileIndex]);
    return this->Superclass::WriteNonCompositeData(dObj, nullptr, currentFileIndex, fName.c_str());
  }
  return 1;
}

//------------------------------------------------------------------------------
vtkStdString vtkXMLPPartitionedDataSetWriter::CreatePieceFileName(
  int currentFileIndex, int procId, int dataSetType)
{
  std::string fname;
  std::string extension;

  if (const char* cext = this->GetDefaultFileExtensionForDataSet(dataSetType))
  {
    extension = cext;
  }
  else
  {
    vtkErrorMacro(<< this->Controller->GetLocalProcessId() << " Unknown data set type.");
    return fname;
  }

  std::ostringstream fn_with_warning_C4701;
  fn_with_warning_C4701 << this->GetFilePrefix() << "/" << this->GetFilePrefix() << "_"
                        << currentFileIndex << "_" << procId << "." << extension;
  fname = fn_with_warning_C4701.str();
  return fname;
}

//------------------------------------------------------------------------------
void vtkXMLPPartitionedDataSetWriter::RemoveWrittenFiles(const char* SubDirectory)
{
  if (this->Controller->GetLocalProcessId() == 0)
  {
    // only proc 0 deletes the files
    this->Superclass::RemoveWrittenFiles(SubDirectory);
  }
}
