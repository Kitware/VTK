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

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkXMLDataElement.h"
#include "vtkInformation.h"

#include <vtksys/ios/sstream>
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
      this->PieceProcessList = 0;
    }
  ~vtkInternal()
    {
      if(this->PieceProcessList)
        {
        delete []this->PieceProcessList;
        this->PieceProcessList = 0;
        }
    }
  void Allocate(int numPieces, int numProcs)
    {
      this->NumberOfPieces = numPieces;
      this->NumberOfProcesses = numProcs;
      if(this->PieceProcessList)
        {
        delete []this->PieceProcessList;
        }
      this->PieceProcessList = new int[numPieces*numProcs];
    }

  void GetPieceProcessList(int piece, int* processList)
    {
      if(!this->PieceProcessList || piece >= this->NumberOfPieces ||
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
  int* PieceProcessList;
  int NumberOfPieces;
  int NumberOfProcesses;
};

//----------------------------------------------------------------------------
vtkXMLPMultiBlockDataWriter::vtkXMLPMultiBlockDataWriter()
{
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
  this->Controller->Gather(myDataTypes, this->Internal->PieceProcessList,
                           numBlocks, 0);
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
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(compositeData->NewIterator());
  iter->VisitOnlyLeavesOff();
  iter->TraverseSubTreeOff();
  iter->SkipEmptyNodesOff();

  int retVal = 0;
  int indexCounter = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); 
       iter->GoToNextItem(), indexCounter++)
    {
    vtkDataObject* curDO = iter->GetCurrentDataObject();
    if (curDO && curDO->IsA("vtkCompositeDataSet"))
      {
      // if node is a supported composite dataset
      // note in structure file and recurse.
      vtkXMLDataElement* tag = vtkXMLDataElement::New();

      const char *name =
        compositeData->GetMetaData(iter)->Get(vtkCompositeDataSet::NAME());
      
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
      if (this->ParallelWriteNonCompositeData( 
            curDO, datasetXML, currentFileIndex) )
        {
        retVal = 1;
        }
      parentXML->AddNestedElement(datasetXML);
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
  if(dObj)
    {
    vtkStdString fName = this->CreatePieceFileName(
      currentFileIndex, myProcId, this->GetDataTypesPointer()[currentFileIndex]);
    return this->Superclass::WriteNonCompositeData(
      dObj, NULL, currentFileIndex, fName.c_str());
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkStdString vtkXMLPMultiBlockDataWriter::CreatePieceFileName(
  int currentFileIndex, int procId, int dataSetType)
{
  std::string fname;
  std::string extension;
  
  switch (dataSetType)
    {
    case VTK_POLY_DATA:
    {
    extension = "vtp";
    break;
    }
    case VTK_STRUCTURED_POINTS:
    case VTK_IMAGE_DATA:
    case VTK_UNIFORM_GRID:
    {
    extension = "vti";
    break;
    }
    case VTK_UNSTRUCTURED_GRID:
    {
    extension = "vtu";
    break;
    }
    case VTK_STRUCTURED_GRID:
    {
    extension = "vts";
    break;
    }
    case VTK_RECTILINEAR_GRID:
    {
    extension = "vtr";
    break;
    }
    default:
    {
    vtkErrorMacro(<<this->Controller->GetLocalProcessId() << " Unknown data set type.");
    return fname;
    }
    }

  vtksys_ios::ostringstream fn_with_warning_C4701;
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
