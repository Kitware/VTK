/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPMultiBlockDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPMultiBlockDataWriter.h"

#include "vtkDataObjectTreeIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkXMLDataElement.h"
#include "vtkInformation.h"

#include <sstream>
#include <vector>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkXMLPMultiBlockDataWriter);

vtkCxxSetObjectMacro(vtkXMLPMultiBlockDataWriter,
                     Controller,
                     vtkMultiProcessController);

class vtkXMLPMultiBlockDataWriter::vtkInternal
{
public:
  vtkInternal()
  {
  }
  ~vtkInternal()
  {
  }
  void Allocate(int numPieces, int numProcs)
  {
      this->NumberOfPieces = numPieces;
      this->NumberOfProcesses = numProcs;
      this->PieceProcessList.resize(numPieces*numProcs);
  }

  void GetPieceProcessList(int piece, int* processList)
  {
      if(this->PieceProcessList.empty() || piece >= this->NumberOfPieces ||
         piece < 0)
      {
        return;
      }
      for(int i=0;i<this->NumberOfProcesses;i++)
      {
        processList[i] =
          this->PieceProcessList[piece+i*this->NumberOfPieces];
      }
  }

  // For each piece it keeps the processes that have that piece.
  // This is built and used only on the root node.
  // PieceProcessList[piece+NumPieces*process] = dataset type (-1 for NULL)
  // This NumberOfPieces is based on the number of blocks in the multiblock
  // which is different than the vtkXMLPMultiBlockDataWriter::NumberOfPieces
  // which is usually the number of parallel processes.
  std::vector<int> PieceProcessList;
  int NumberOfPieces;
  int NumberOfProcesses;
};

//----------------------------------------------------------------------------
vtkXMLPMultiBlockDataWriter::vtkXMLPMultiBlockDataWriter()
{
  this->StartPiece = 0;
  this->NumberOfPieces = 1;
  this->Internal = new vtkInternal();
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->SetWriteMetaFile(1);
}

//----------------------------------------------------------------------------
vtkXMLPMultiBlockDataWriter::~vtkXMLPMultiBlockDataWriter()
{
  this->SetController(0);
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkXMLPMultiBlockDataWriter::SetWriteMetaFile(int flag)
{
  this->Modified();
  if(this->Controller == NULL || this->Controller->GetLocalProcessId() == 0)
  {
    if(this->WriteMetaFile != flag)
    {
      this->WriteMetaFile = flag;
    }
  }
  else
  {
    this->WriteMetaFile = 0;
  }
}

//----------------------------------------------------------------------------
void vtkXMLPMultiBlockDataWriter::PrintSelf(ostream& os, vtkIndent indent)
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

//----------------------------------------------------------------------------
int vtkXMLPMultiBlockDataWriter::ProcessRequest(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      this->NumberOfPieces);
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), this->StartPiece);
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      this->GhostLevel);
    return 1;
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkXMLPMultiBlockDataWriter::FillDataTypes(vtkCompositeDataSet* hdInput)
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

  this->Internal->Allocate(numBlocks, this->Controller->GetNumberOfProcesses());

  // gather on to root node.
  if(numBlocks)
  {
    this->Controller->Gather(myDataTypes, &this->Internal->PieceProcessList[0],
                             numBlocks, 0);
  }
}

//----------------------------------------------------------------------------
int vtkXMLPMultiBlockDataWriter::WriteComposite(
  vtkCompositeDataSet* compositeData, vtkXMLDataElement* parentXML,
  int &currentFileIndex)
{
  if (! (compositeData->IsA("vtkMultiBlockDataSet")
        ||compositeData->IsA("vtkMultiPieceDataSet")) )
  {
    vtkErrorMacro("Unsupported composite dataset type: "
                  << compositeData->GetClassName() << ".");
    return 0;
  }

  // Write each input.
  vtkSmartPointer<vtkDataObjectTreeIterator> iter;
  iter.TakeReference(
    vtkDataObjectTreeIterator::SafeDownCast(compositeData->NewIterator()));
  iter->VisitOnlyLeavesOff();
  iter->TraverseSubTreeOff();
  iter->SkipEmptyNodesOff();

  int retVal = 0;
  int indexCounter = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
       iter->GoToNextItem(), indexCounter++)
  {
    vtkDataObject* curDO = iter->GetCurrentDataObject();
    const char *name = NULL;
    if(iter->HasCurrentMetaData())
    {
      name = iter->GetCurrentMetaData()->Get(vtkCompositeDataSet::NAME());
    }

    if (curDO && curDO->IsA("vtkCompositeDataSet"))
    {
      // if node is a supported composite dataset
      // note in structure file and recurse.
      vtkXMLDataElement* tag = vtkXMLDataElement::New();

      if (curDO->IsA("vtkMultiPieceDataSet"))
      {
        tag->SetName("Piece");
        tag->SetIntAttribute("index", indexCounter);

        if(name)
        {
          tag->SetAttribute("name", name);
        }
      }
      else if (curDO->IsA("vtkMultiBlockDataSet"))
      {
        tag->SetName("Block");
        tag->SetIntAttribute("index", indexCounter);

        if(name)
        {
          tag->SetAttribute("name", name);
        }
      }
      vtkCompositeDataSet* curCD
        = vtkCompositeDataSet::SafeDownCast(curDO);
      if (this->WriteComposite(curCD, tag, currentFileIndex))
      {
        parentXML->AddNestedElement(tag);
        retVal = 1;
      }
      tag->Delete();
    }
    else
    {
      // this node is not a composite data set.
      vtkXMLDataElement* datasetXML = vtkXMLDataElement::New();
      // datasetXML::Name may get overwritten in ParallelWriteNonCompositeData
      // if this piece is on different processes.
      datasetXML->SetName("DataSet");
      datasetXML->SetIntAttribute("index", indexCounter);
      if (name)
      {
        datasetXML->SetAttribute("name", name);
      }
      if (this->ParallelWriteNonCompositeData(
            curDO, datasetXML, currentFileIndex) )
      {
        retVal = 1;
        parentXML->AddNestedElement(datasetXML);
      }
      currentFileIndex++;
      datasetXML->Delete();
    }
  }

  return retVal;
}

//----------------------------------------------------------------------------
int vtkXMLPMultiBlockDataWriter::ParallelWriteNonCompositeData(
  vtkDataObject* dObj, vtkXMLDataElement* parentXML, int currentFileIndex)
{
  int myProcId = this->Controller->GetLocalProcessId();
  if (myProcId == 0)
  {
    // pieceProcessList is a list where index is the process number and value is
    // the data-type for the current leaf on that process.
    int numberOfProcesses = this->Controller->GetNumberOfProcesses();
    std::vector<int> pieceProcessList(numberOfProcesses);
    this->Internal->GetPieceProcessList(currentFileIndex, &pieceProcessList[0]);

    int numPieces = 0;
    for (int procId=0; procId < numberOfProcesses; procId++)
    {
      if(pieceProcessList[procId] >= 0)
      {
        numPieces++;
      }
    }
    if(numPieces > 1)
    {
      // intentionally overwrite parentXML::Name from "DataSet" to
      //"Piece" as the calling function did not know this had multiple
      // pieces.  It will still have the index that was set before.
      parentXML->SetName("Piece");
    }

    int indexCounter = 0;
    for (int procId=0; procId < numberOfProcesses; procId++)
    {
      if(pieceProcessList[procId] >= 0)
      {
        vtkXMLDataElement* datasetXML = parentXML;
        if(numPieces > 1)
        {
          // a hacky way to make sure that the pieces are nested into
          // parentXML
          datasetXML = vtkXMLDataElement::New();
          datasetXML->SetName("DataSet");
          datasetXML->SetIntAttribute("index", indexCounter);
          parentXML->AddNestedElement(datasetXML);
          datasetXML->Delete();
          indexCounter++;
        }
        vtkStdString fName = this->CreatePieceFileName(
          currentFileIndex, procId, pieceProcessList[procId]);
        datasetXML->SetAttribute("file", fName.c_str());
      }
    }
  }

  const int* datatypes_ptr = this->GetDataTypesPointer();
  if(dObj && datatypes_ptr[currentFileIndex] != -1)
  {
    vtkStdString fName = this->CreatePieceFileName(
      currentFileIndex, myProcId, datatypes_ptr[currentFileIndex]);
    return this->Superclass::WriteNonCompositeData(
      dObj, NULL, currentFileIndex, fName.c_str());
  }
  return 1;
}

//----------------------------------------------------------------------------
vtkStdString vtkXMLPMultiBlockDataWriter::CreatePieceFileName(
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
    vtkErrorMacro(<<this->Controller->GetLocalProcessId() << " Unknown data set type.");
    return fname;
  }

  std::ostringstream fn_with_warning_C4701;
  fn_with_warning_C4701
    << this->GetFilePrefix() << "/"
    << this->GetFilePrefix() << "_" << currentFileIndex
    << "_" << procId << "." << extension;
  fname = fn_with_warning_C4701.str();
  return fname;
}

//----------------------------------------------------------------------------
void vtkXMLPMultiBlockDataWriter::RemoveWrittenFiles(const char* SubDirectory)
{
  if(this->Controller->GetLocalProcessId() == 0)
  {
    // only proc 0 deletes the files
    this->Superclass::RemoveWrittenFiles(SubDirectory);
  }
}
