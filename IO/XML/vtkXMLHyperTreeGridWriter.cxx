/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLHyperTreeGridWriter.cxx

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

#include "vtkXMLHyperTreeGridWriter.h"

#include "vtkBitArray.h"
#include "vtkDoubleArray.h"
#include "vtkErrorCode.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkXMLHyperTreeGridWriter);

//----------------------------------------------------------------------------
vtkXMLHyperTreeGridWriter::~vtkXMLHyperTreeGridWriter() = default;

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkHyperTreeGrid* vtkXMLHyperTreeGridWriter::GetInput()
{
  return static_cast<vtkHyperTreeGrid*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
const char* vtkXMLHyperTreeGridWriter::GetDefaultFileExtension()
{
  return "htg";
}

//----------------------------------------------------------------------------
const char* vtkXMLHyperTreeGridWriter::GetDataSetName()
{
  return "HyperTreeGrid";
}

//----------------------------------------------------------------------------
int vtkXMLHyperTreeGridWriter::FillInputPortInformation(
  int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLHyperTreeGridWriter::WriteData()
{
  //write XML header and VTK file header and file attributes
  if (!this->StartFile())
  {
    return 0;
  }

  vtkIndent indent = vtkIndent().GetNextIndent();

  // Header attributes
  if (!this->StartPrimaryElement(indent))
  {
    return 0;
  }

  // Coordinates for grid (can be replaced by origin and scale)
  if (!this->WriteGridCoordinates(indent.GetNextIndent()))
  {
    return 0;
  }

  if (!this->WriteDescriptor(indent.GetNextIndent()))
  {
    return 0;
  }

  if (!this->WriteAttributeData(indent.GetNextIndent()))
  {
    return 0;
  }

  this->WriteFieldData(indent.GetNextIndent());

  if (!this->FinishPrimaryElement(indent))
  {
    return 0;
  }

  if (!this->EndFile())
  {
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLHyperTreeGridWriter::StartPrimaryElement(vtkIndent indent)
{
  ostream& os = *(this->Stream);

  return (!this->WritePrimaryElement(os, indent)) ? 0 : 1;
}

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridWriter::WritePrimaryElementAttributes
  (ostream &os, vtkIndent indent)
{
  this->Superclass::WritePrimaryElementAttributes(os, indent);
  vtkHyperTreeGrid* input = this->GetInput();

  this->WriteScalarAttribute("Dimension", (int) input->GetDimension());
  this->WriteScalarAttribute("BranchFactor", (int) input->GetBranchFactor());
  this->WriteScalarAttribute("TransposedRootIndexing",
                             (bool)input->GetTransposedRootIndexing());
  this->WriteVectorAttribute("GridSize", 3, (int*) input->GetGridSize());

  // vtkHyperTreeGrid does not yet store origin and scale but
  // calculate as place holder
  vtkDataArray* xcoord = input->GetXCoordinates();
  vtkDataArray* ycoord = input->GetYCoordinates();
  vtkDataArray* zcoord = input->GetZCoordinates();

  double gridOrigin[3] = {xcoord->GetTuple1(0),
                          ycoord->GetTuple1(0),
                          zcoord->GetTuple1(0)};

  double gridScale[3] = {xcoord->GetTuple1(1)-xcoord->GetTuple1(0),
                         ycoord->GetTuple1(1)-ycoord->GetTuple1(0),
                         zcoord->GetTuple1(1)-zcoord->GetTuple1(0)};

  this->WriteVectorAttribute("GridOrigin", 3, gridOrigin);
  this->WriteVectorAttribute("GridScale", 3, gridScale);
}

//----------------------------------------------------------------------------
int vtkXMLHyperTreeGridWriter::WriteGridCoordinates(vtkIndent indent)
{
  vtkHyperTreeGrid* input = this->GetInput();
  ostream& os = *(this->Stream);
  os << indent << "<Coordinates>\n";

  this->WriteArrayInline(input->GetXCoordinates(), indent.GetNextIndent(),
                         "XCoordinates",
                         input->GetXCoordinates()->GetNumberOfValues());
  this->WriteArrayInline(input->GetYCoordinates(), indent.GetNextIndent(),
                         "YCoordinates",
                         input->GetYCoordinates()->GetNumberOfValues());
  this->WriteArrayInline(input->GetZCoordinates(), indent.GetNextIndent(),
                         "ZCoordinates",
                         input->GetZCoordinates()->GetNumberOfValues());

  os << indent << "</Coordinates>\n";
  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
vtkXMLHyperTreeGridWriter::vtkXMLHyperTreeGridWriter() = default;

//----------------------------------------------------------------------------
namespace {
  void BuildDescriptor
  ( vtkHyperTreeGridNonOrientedCursor* inCursor,
    int level,
    std::vector<std::string> &descriptor)
  {
    if ( ! inCursor->IsLeaf() )
    {
      descriptor[level] += 'R';
      // If input cursor is not a leaf, recurse to all children
      int numChildren = inCursor->GetNumberOfChildren();
      for ( int child = 0; child < numChildren; ++ child )
      {
        inCursor->ToChild( child );
        // Recurse
        BuildDescriptor( inCursor, level+1, descriptor );
        inCursor->ToParent();
      } // child
    }
    else
    {
      descriptor[level] += '.';
    }
  }
}

//----------------------------------------------------------------------------
int vtkXMLHyperTreeGridWriter::WriteDescriptor(vtkIndent indent)
{
  vtkHyperTreeGrid* input = this->GetInput();
  vtkIdType maxLevels = input->GetNumberOfLevels();

  ostream& os = *(this->Stream);
  os << indent << "<Topology>\n";

  // Collect description by processing depth first and writing breadth first
  std::vector<std::string> descByLevel(maxLevels);
  vtkIdType inIndex;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator( it );
  vtkNew<vtkHyperTreeGridNonOrientedCursor> inCursor;
  while ( it.GetNextTree( inIndex ) )
  {
    // CEA Warning TODO Write inIndex in XML file
    // Initialize new grid cursor at root of current input tree
    input->InitializeNonOrientedCursor( inCursor, inIndex );
    // Recursively compute descriptor for this tree, appending any
    // entries for each of the levels in descByLevel.
    BuildDescriptor(inCursor, 0, descByLevel );
  }

  // Build the BitArray from the level descriptors
  vtkNew<vtkBitArray> descriptor;

  std::string::const_iterator dit;
  for (int l = 0; l < maxLevels; l++)
  {
    for (dit = descByLevel[l].begin(); dit != descByLevel[l].end(); ++dit)
    {
      switch (*dit)
      {
        case 'R':    //  Refined cell
          descriptor->InsertNextValue(1);
          break;

        case '.':    // Leaf cell
          descriptor->InsertNextValue(0);
          break;

        default:
          vtkErrorMacro(<< "Unrecognized character: "
                        << *dit
                        << " in string "
                        << descByLevel[l]);
          return 0;
      }
    }
  }
  descriptor->Squeeze();
  this->WriteArrayInline(descriptor, indent.GetNextIndent(), "Descriptor",
                         descriptor->GetNumberOfValues());

  os << indent << "</Topology>\n";

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLHyperTreeGridWriter::WriteAttributeData(vtkIndent indent)
{
  // Write the point data and cell data arrays.
  vtkDataSet* input = this->GetInputAsDataSet();

  // Split progress between point data and cell data arrays.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);
  int pdArrays = input->GetPointData()->GetNumberOfArrays();
  int total = (pdArrays)? (pdArrays):1;
  float fractions[3] = { 0.f, static_cast<float>(pdArrays) / total, 1.f };

  // Set the range of progress for the point data arrays.
  this->SetProgressRange(progressRange, 0, fractions);

  this->WritePointDataInline(input->GetPointData(), indent);

  ostream& os = *(this->Stream);
  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLHyperTreeGridWriter::FinishPrimaryElement(vtkIndent indent)
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
