/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLHyperOctreeWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

//TODO:
// Add support for timesteps
// Add streaming support.

#include "vtkXMLHyperOctreeWriter.h"

#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkHyperOctree.h"
#include "vtkHyperOctreeCursor.h"
#include "vtkIntArray.h"
#include "vtkErrorCode.h"

#define vtkXMLOffsetsManager_DoNotInclude
#include "vtkXMLOffsetsManager.h"
#undef  vtkXMLOffsetsManager_DoNotInclude


vtkStandardNewMacro(vtkXMLHyperOctreeWriter);

//----------------------------------------------------------------------------
vtkXMLHyperOctreeWriter::vtkXMLHyperOctreeWriter()
{
  this->TopologyArray = NULL;
  this->TopologyOM = new OffsetsManagerGroup;
  this->PointDataOM = new OffsetsManagerGroup;
  this->CellDataOM = new OffsetsManagerGroup;
  this->TopologyOM->Allocate(1, 1);
}

//----------------------------------------------------------------------------
vtkXMLHyperOctreeWriter::~vtkXMLHyperOctreeWriter()
{
  if (this->TopologyArray)
    {
    this->TopologyArray->Delete();
    }
  delete this->TopologyOM;
  delete this->PointDataOM;
  delete this->CellDataOM;
}

//----------------------------------------------------------------------------
void vtkXMLHyperOctreeWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkHyperOctree* vtkXMLHyperOctreeWriter::GetInput()
{
  return static_cast<vtkHyperOctree*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
const char* vtkXMLHyperOctreeWriter::GetDefaultFileExtension()
{
  return "vto";
}

//----------------------------------------------------------------------------
const char* vtkXMLHyperOctreeWriter::GetDataSetName()
{
  return "HyperOctree";
}

//----------------------------------------------------------------------------
int vtkXMLHyperOctreeWriter::FillInputPortInformation(
  int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperOctree");
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLHyperOctreeWriter::WriteData()
{
  //write XML header and VTK file header and file attributes
  if (!this->StartFile())
    {
    return 0;
    }

  vtkIndent indent = vtkIndent().GetNextIndent();

  if (!this->StartPrimElement(indent))
    {
    return 0;
    }

  if (!this->WriteTopology(indent.GetNextIndent()))
    {
    return 0;
    }

  if (!this->WriteAttributeData(indent.GetNextIndent()))
    {
    return 0;
    }

  this->WriteFieldData(indent.GetNextIndent());

  if (!this->FinishPrimElement(indent))
    {
    return 0;
    }

  if (this->GetDataMode() == vtkXMLWriter::Appended)
    {

    float progressRange[2] = { 0.f, 0.f };
    this->GetProgressRange(progressRange);
    //Part spent serializing and writing assumed to be roughly equal.
    float fractions[5] = { 0.f, 0.25f, 0.5f, 0.75f, 1.f };
    this->SetProgressRange(progressRange, 0, fractions);

    this->StartAppendedData();

    //write out the data arrays in the appended data block while going back
    //and filling in empty offset space in previously written entries

    this->WriteArrayAppendedData
      (this->TopologyArray,
       this->TopologyOM->GetElement(0).GetPosition(0),
       this->TopologyOM->GetElement(0).GetOffsetValue(0));
    double *range = this->TopologyArray->GetRange(-1);
    this->ForwardAppendedDataDouble
      (this->TopologyOM->GetElement(0).GetRangeMinPosition(0),
       range[0],"RangeMin" );
    this->ForwardAppendedDataDouble
      (this->TopologyOM->GetElement(0).GetRangeMaxPosition(0),
       range[1],"RangeMax" );
    this->SetProgressRange(progressRange, 1, fractions);

    this->WritePointDataAppendedData(this->GetInput()->GetPointData(), 0, this->PointDataOM);

    this->SetProgressRange(progressRange, 2, fractions);

    this->WriteCellDataAppendedData(this->GetInput()->GetCellData(), 0, this->CellDataOM);

    this->SetProgressRange(progressRange, 3, fractions);

    this->WriteFieldDataAppendedData(this->GetInput()->GetFieldData(), 0, this->FieldDataOM);

    this->EndAppendedData();
    }

  this->TopologyArray->Delete();
  this->TopologyArray = NULL;

  if (!this->EndFile())
    {
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLHyperOctreeWriter::StartPrimElement(vtkIndent indent)
{
  ostream& os = *(this->Stream);

  return (!this->WritePrimaryElement(os, indent)) ? 0 : 1;
}

//----------------------------------------------------------------------------
void vtkXMLHyperOctreeWriter::WritePrimaryElementAttributes(ostream &os, vtkIndent indent)
{
  this->Superclass::WritePrimaryElementAttributes(os, indent);
  vtkHyperOctree* input = this->GetInput();
  this->WriteScalarAttribute("Dimension", input->GetDimension());
  this->WriteVectorAttribute("Size", 3, input->GetSize());
  this->WriteVectorAttribute("Origin", 3, input->GetOrigin());
}

//----------------------------------------------------------------------------
int vtkXMLHyperOctreeWriter::WriteTopology(vtkIndent indent)
{
  if (this->TopologyArray)
    {
    this->TopologyArray->Delete();
    }
  this->TopologyArray = vtkIntArray::New();
  this->TopologyArray->SetNumberOfComponents(1);

  vtkHyperOctree* input = this->GetInput();
  vtkHyperOctreeCursor *cursor=input->NewCellCursor();
  cursor->ToRoot();

  //record structure into an array

  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);
  //Part spent serializing and writing assumed to be roughly equal.
  float fractions[3] = { 0.f, 0.5f, 1.f };
  this->SetProgressRange(progressRange, 0, fractions);

  this->SerializeTopology(cursor, cursor->GetNumberOfChildren());

  //write out the array
  this->SetProgressRange(progressRange, 1, fractions);
  ostream& os = *(this->Stream);
  os << indent << "<" << "Topology" << ">\n";
  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    return 0;
    }
  //...,1) to save length of array for easy recovery by reader

  if (this->GetDataMode() == vtkXMLWriter::Appended)
    {
    this->WriteArrayAppended(this->TopologyArray,
      indent.GetNextIndent(),
      this->TopologyOM->GetElement(0),
      "Topology", 1, 0);
    }
  else
    {
    this->WriteArrayInline(this->TopologyArray,
      indent.GetNextIndent(), "Topology", 1);
    }

  os << indent << "</" << "Topology" << ">\n";
  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    return 0;
    }

  cursor->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLHyperOctreeWriter::SerializeTopology(
  vtkHyperOctreeCursor *cursor, int nchildren)
{
  if (cursor->CurrentIsLeaf())
    {
    //this node is a leaf, we must stop now
    this->TopologyArray->InsertNextValue(1);
    }
  /*
  else if (cursor->CurrentIsTerminalNode())
    {
    //this node has 'nchildren' children,
    //all of which are leaves, so we can stop now too
    this->TopologyArray->InsertNextValue(2);
    }
  */
  else
    {
    //this node has 'nchildren' children,
    //some of which are internal nodes, so we must continue down
    this->TopologyArray->InsertNextValue(0);

    int i = 0;
    while (i < nchildren)
      {
      cursor->ToChild(i);
      this->SerializeTopology(cursor, nchildren);
      cursor->ToParent();
      ++i;
      }
    }
}

//----------------------------------------------------------------------------
int vtkXMLHyperOctreeWriter::WriteAttributeData(vtkIndent indent)
{
  // Write the point data and cell data arrays.
  vtkDataSet* input = this->GetInputAsDataSet();

  // Split progress between point data and cell data arrays.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);
  int pdArrays = input->GetPointData()->GetNumberOfArrays();
  int cdArrays = input->GetCellData()->GetNumberOfArrays();
  int total = (pdArrays+cdArrays)? (pdArrays+cdArrays):1;
  float fractions[3] = { 0.f, static_cast<float>(pdArrays) / total, 1.f };

  // Set the range of progress for the point data arrays.
  this->SetProgressRange(progressRange, 0, fractions);

  if (this->GetDataMode() == vtkXMLWriter::Appended)
    {
    this->WritePointDataAppended(
      input->GetPointData(), indent, this->PointDataOM);
    }
  else
    {
    this->WritePointDataInline(input->GetPointData(), indent);
    }

  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
    return 0;
    }

  // Set the range of progress for the cell data arrays.
  this->SetProgressRange(progressRange, 1, fractions);
  if (this->GetDataMode() == vtkXMLWriter::Appended)
    {
    this->WriteCellDataAppended(
      input->GetCellData(), indent, this->CellDataOM);
    }
  else
    {
    this->WriteCellDataInline(input->GetCellData(), indent);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLHyperOctreeWriter::FinishPrimElement(vtkIndent indent)
{
  ostream& os = *(this->Stream);

  // End the primary element.
  os << indent << "</" << this->GetDataSetName() << ">\n";
  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    return 0;
    }
  return 1;
}
