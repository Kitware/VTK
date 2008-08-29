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

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"

#include <vtkstd/vector>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkXMLPMultiBlockDataWriter);
vtkCxxRevisionMacro(vtkXMLPMultiBlockDataWriter, "1.2");

vtkCxxSetObjectMacro(vtkXMLPMultiBlockDataWriter, 
                     Controller,
                     vtkMultiProcessController);

class vtkXMLPMultiBlockDataWriter::vtkInternal
{
public:
  // For each piece in keeps the processes that have that piece. 
  // This is built only on the root node.
  vtkstd::vector<vtkstd::vector<int> > PieceProcessList;
};

//----------------------------------------------------------------------------
vtkXMLPMultiBlockDataWriter::vtkXMLPMultiBlockDataWriter()
{
  this->Internal = new vtkInternal();
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkXMLPMultiBlockDataWriter::~vtkXMLPMultiBlockDataWriter()
{
  this->SetController(0);
  delete this->Internal;
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
// Description:
// Overridden to do some extra workn the root node to handle the case where a
// leaf node in the multiblock hierarchy is non-null on more than 1 processes.
// In that case, we insert a MultiPiece node at that location with the
// non-null leaves as its children.
int vtkXMLPMultiBlockDataWriter::WriteNonCompositeData(vtkDataObject* dObj,
  vtkXMLDataElement* datasetXML, int &writerIdx)
{
  if (
    static_cast<int>(this->Internal->PieceProcessList.size()) <= writerIdx ||
    this->Internal->PieceProcessList[writerIdx].size() <= 1)
    {
    // The PieceProcessList is not built on non-root nodes.
    // On non-root nodes there's nothing special to do, since the datasetXML
    // generated on the non-root nodes is bogus anyways.
    return this->Superclass::WriteNonCompositeData(dObj, datasetXML, writerIdx);
    }


  // If the control reaches here, it means that this leaf node is non-null on
  // more than 1 process. We need to insert a <Piece /> node.
  vtkXMLDataElement* pieceXML = datasetXML;
  pieceXML->SetName("Piece");

  vtkstd::vector<int> &pieces = this->Internal->PieceProcessList[writerIdx];

  int myPiece = this->Piece;
  for (int cc=0; cc < static_cast<int>(pieces.size()); cc++)
    {
    this->Piece = pieces[cc];
    datasetXML = vtkXMLDataElement::New();
    datasetXML->SetName("DataSet");
    datasetXML->SetIntAttribute("index", cc);
    datasetXML->SetAttribute("file", this->CreatePieceFileName(writerIdx).c_str());
    pieceXML->AddNestedElement(datasetXML);
    datasetXML->Delete();
    }
  this->Piece = myPiece;
  return this->Superclass::WriteNonCompositeData(dObj, NULL, writerIdx);
}

//----------------------------------------------------------------------------
// Determine the data types for each of the leaf nodes.
// In parallel, all satellites send the datatypes it has at all the leaf nodes
// to the root. The root uses this information to identify leaf nodes
// that are non-null on more than 1 processes. Such a node is a conflicting
// node and writer writes out an XML meta-file with a multipiece inserted at
// locations for such conflicting nodes.
void vtkXMLPMultiBlockDataWriter::FillDataTypes(vtkCompositeDataSet* hdInput)
{
  this->Internal->PieceProcessList.clear();
  this->Superclass::FillDataTypes(hdInput);

  if (!this->Controller)
    {
    return;
    }

  int myid = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();

  unsigned int numBlocks = this->GetNumberOfDataTypes();
  int* myDataTypes = this->GetDataTypesPointer();

  if (numBlocks == 0 || numProcs <= 1)
    {
    return;
    }

  this->Internal->PieceProcessList.resize(numBlocks);

  unsigned int cc;
  if (myid == 0)
    {
    for (cc=0; cc < numBlocks; cc++)
      {
      if (myDataTypes[cc] >= 0)
        {
        this->Internal->PieceProcessList[cc].push_back(0);
        }
      }

    int* dataTypes = new int[numBlocks];
    for (int procId=1; procId<numProcs; procId++)
      {
      this->Controller->Receive(dataTypes, numBlocks, procId,
        vtkMultiProcessController::XML_WRITER_DATA_INFO);
      for (cc=0; cc < numBlocks; cc++)
        {
        if (dataTypes[cc] >= 0)
          {
          this->Internal->PieceProcessList[cc].push_back(procId);
          }
        }
      }
    delete[] dataTypes;
    }
  else
    {
    this->Controller->Send(myDataTypes, numBlocks, 0,
                           vtkMultiProcessController::XML_WRITER_DATA_INFO);
    }
}

