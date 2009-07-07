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

#include <vtksys/ios/sstream>
#include <vtkstd/vector>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkXMLPMultiBlockDataWriter);
vtkCxxRevisionMacro(vtkXMLPMultiBlockDataWriter, "1.4");

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

  void GetPieceProcessList(int Piece, int* ProcessList)
    {
      if(!this->PieceProcessList || Piece >= this->NumberOfPieces ||
         Piece < 0)
        {
        return;
        }
      for(int i=0;i<this->NumberOfProcesses;i++)
        {
        ProcessList[i] = 
          this->PieceProcessList[Piece+i*this->NumberOfPieces];
        }
    }
  // For each piece it keeps the processes that have that piece. 
  // This is built and used on all processes.
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
  this->vtkXMLPMultiBlockDataWriter::SetWriteMetaFile(1);
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
  if(this->Controller->GetLocalProcessId() == 0)
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
  this->Superclass::FillDataTypes(hdInput);

  if (!this->Controller)
    {
    return;
    }

  unsigned int numBlocks = this->GetNumberOfDataTypes();
  int* myDataTypes = this->GetDataTypesPointer();

  this->Internal->Allocate(numBlocks, this->Controller->GetNumberOfProcesses());
  this->Controller->AllGather(myDataTypes, this->Internal->PieceProcessList, numBlocks);
}

//----------------------------------------------------------------------------
int vtkXMLPMultiBlockDataWriter::WriteComposite(
  vtkCompositeDataSet* compositeData, vtkXMLDataElement* ParentXML, 
  int &CurrentFileIndex)
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

  int RetVal = 0;
  int IndexCounter = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); 
       iter->GoToNextItem(), IndexCounter++)
    {
    vtkDataObject* curDO = iter->GetCurrentDataObject();
    if (curDO && curDO->IsA("vtkCompositeDataSet"))
      {
      // if node is a supported composite dataset
      // note in structure file and recurse.
      vtkXMLDataElement* tag = vtkXMLDataElement::New();
      
      if (curDO->IsA("vtkMultiPieceDataSet"))
        {
        tag->SetName("Piece");
        tag->SetIntAttribute("index", IndexCounter);
        }
      else if (curDO->IsA("vtkMultiBlockDataSet"))
        {
        tag->SetName("Block");
        tag->SetIntAttribute("index", IndexCounter);
        }
      vtkCompositeDataSet* curCD
        = vtkCompositeDataSet::SafeDownCast(curDO);
      if (this->WriteComposite(curCD, tag, CurrentFileIndex))
        {
        ParentXML->AddNestedElement(tag);
        RetVal = 1;
        }
      tag->Delete();
      }
    else
      {
      // this node is not a composite data set.
      vtkXMLDataElement* datasetXML = vtkXMLDataElement::New();
      // datasetXML::Name may get overwritten in WriteNonCompositeData
      // if this piece is on different processes.
      datasetXML->SetName("DataSet");
      datasetXML->SetIntAttribute("index", IndexCounter);
      if (this->WriteNonCompositeData( curDO, datasetXML, CurrentFileIndex) )
        {
        RetVal = 1;
        }
      ParentXML->AddNestedElement(datasetXML);
      CurrentFileIndex++;
      datasetXML->Delete();
      }
    }

  return RetVal;
}

//----------------------------------------------------------------------------
int vtkXMLPMultiBlockDataWriter::WriteNonCompositeData(
  vtkDataObject* dObj, vtkXMLDataElement* ParentXML, int CurrentFileIndex)
{
  int NumberOfProcesses = this->Controller->GetNumberOfProcesses();
  vtkstd::vector<int> PieceProcessList(NumberOfProcesses);
  this->Internal->GetPieceProcessList(CurrentFileIndex, &PieceProcessList[0]);
  int MyProcId = this->Controller->GetLocalProcessId();
 
  if(MyProcId == 0)
    {
    int NumPieces = 0;
    for (int ProcId=0; ProcId < NumberOfProcesses; ProcId++)
      {
      if(PieceProcessList[ProcId] >= 0)
        {
        NumPieces++;
        }
      }
    if(NumPieces > 1)
      {
      // intentionally overwrite ParentXML::Name from "DataSet" to 
      //"Piece" as the calling function did not know this had multiple
      // pieces.  It will still have the index that was set before.
      ParentXML->SetName("Piece");
      }
    int IndexCounter = 0;
    for (int ProcId=0; ProcId < NumberOfProcesses; ProcId++)
      {
      if(PieceProcessList[ProcId] >= 0)
        {
        vtkXMLDataElement* datasetXML = ParentXML;
        if(NumPieces > 1)
          {
          // a hacky way to make sure that the pieces are nested into
          // ParentXML
          datasetXML = vtkXMLDataElement::New();
          datasetXML->SetName("DataSet");
          datasetXML->SetIntAttribute("index", IndexCounter);
          ParentXML->AddNestedElement(datasetXML);
          datasetXML->Delete();          
          IndexCounter++;
          }
        vtkStdString fName = this->CreatePieceFileName(
          CurrentFileIndex, ProcId, PieceProcessList[ProcId]);
        datasetXML->SetAttribute("file", fName.c_str());
        }
      }
    }
  if(dObj)
    {
    vtkStdString FileName = this->CreatePieceFileName(
      CurrentFileIndex, MyProcId, PieceProcessList[MyProcId]);
    return this->Superclass::WriteNonCompositeData(
      dObj, NULL, CurrentFileIndex, FileName.c_str());
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkStdString vtkXMLPMultiBlockDataWriter::CreatePieceFileName(
  int CurrentFileIndex, int ProcId, int DataSetType)
{
  vtkstd::string fname;
  vtkstd::string extension;
  
  switch (DataSetType)
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
    << this->GetFilePrefix() << "_" << CurrentFileIndex
    << "_" << ProcId << "." << extension;
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
