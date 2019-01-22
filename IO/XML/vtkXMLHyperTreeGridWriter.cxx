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

#include "vtkAbstractArray.h"
#include "vtkBitArray.h"
#include "vtkDoubleArray.h"
#include "vtkErrorCode.h"
#include "vtkHyperTree.h"
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
  if (!this->WriteGrid(indent.GetNextIndent()))
  {
    return 0;
  }

  if (!this->WriteTrees(indent.GetNextIndent()))
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

  int extent[6];
  input->GetExtent(extent);

  this->WriteScalarAttribute("Dimension", (int) input->GetDimension());
  this->WriteScalarAttribute("Orientation",
                             (unsigned char) input->GetOrientation());
  this->WriteScalarAttribute("BranchFactor", (int) input->GetBranchFactor());
  this->WriteScalarAttribute("TransposedRootIndexing",
                             (bool)input->GetTransposedRootIndexing());
  this->WriteVectorAttribute("Extent", 6, (int*) input->GetExtent());
  this->WriteVectorAttribute("Dimensions", 3, (int*) input->GetDimensions());
  this->WriteScalarAttribute("NumberOfVertices", input->GetNumberOfVertices());
}

//----------------------------------------------------------------------------
int vtkXMLHyperTreeGridWriter::WriteGrid(vtkIndent indent)
{
  vtkHyperTreeGrid* input = this->GetInput();
  ostream& os = *(this->Stream);
  os << indent << "<Grid>\n";

  // Coordinates of the grid
  this->WriteArrayInline(input->GetXCoordinates(), indent.GetNextIndent(),
                         "XCoordinates",
                         input->GetXCoordinates()->GetNumberOfValues());
  this->WriteArrayInline(input->GetYCoordinates(), indent.GetNextIndent(),
                         "YCoordinates",
                         input->GetYCoordinates()->GetNumberOfValues());
  this->WriteArrayInline(input->GetZCoordinates(), indent.GetNextIndent(),
                         "ZCoordinates",
                         input->GetZCoordinates()->GetNumberOfValues());

  // Write mask on grid
  this->WriteArrayInline(input->GetMask(), indent.GetNextIndent(),
                         "Mask",
                         input->GetMask()->GetNumberOfValues());

  os << indent << "</Grid>\n";
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
    // Retrieve input grid
    vtkHyperTreeGrid* input = inCursor->GetGrid();

    if ( ! inCursor->IsLeaf() )
    {
      descriptor[level] += 'R';

      // If input cursor is not a leaf, recurse to all children
      int numChildren = input->GetNumberOfChildren();
      for ( int child = 0; child < numChildren; ++ child )
      {
        // Create child cursor from parent in input grid
        vtkHyperTreeGridNonOrientedCursor* childCursor = inCursor->Clone();
        childCursor->ToChild( child );

        // Recurse
        BuildDescriptor( childCursor, level+1, descriptor );

        // Clean up
        childCursor->Delete();
      } // child
    }
    else
    {
      descriptor[level] += '.';
    }
  }
}

//----------------------------------------------------------------------------
int vtkXMLHyperTreeGridWriter::WriteTrees(vtkIndent indent)
{
  vtkHyperTreeGrid* input = this->GetInput();
  vtkIdType maxLevels = input->GetNumberOfLevels();

  ostream& os = *(this->Stream);
  os << indent << "<Trees>\n";
  vtkIndent treeIndent = indent.GetNextIndent();

  // Collect description by processing depth first and writing breadth first
  vtkIdType inIndex;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator( it );

  while ( it.GetNextTree( inIndex ) )
  {
    // Initialize new grid cursor at root of current input tree
    vtkHyperTreeGridNonOrientedCursor* inCursor = input->NewNonOrientedCursor( inIndex );
    vtkHyperTree* tree = inCursor->GetTree();
    vtkIdType count = tree->GetNumberOfVertices();
    vtkIdType offset = tree->GetGlobalIndexFromLocal(0);;

    os << treeIndent << "<Tree";
    this->WriteScalarAttribute("Index", inIndex);
    this->WriteScalarAttribute("Offset", offset);
    this->WriteScalarAttribute("Count", count);
    os << ">\n";

    // Recursively compute descriptor for this tree, appending any
    // entries for each of the levels in descByLevel.
    std::vector<std::string> descByLevel(maxLevels);
    BuildDescriptor(inCursor, 0, descByLevel );

    // Clean up
    inCursor->Delete();

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
    vtkIndent descIndent = treeIndent.GetNextIndent();
    this->WriteArrayInline(descriptor, descIndent, "Descriptor", descriptor->GetNumberOfValues());

    // Write the point data and cell data arrays for tree.
    os << descIndent << "<PointData>\n";
    vtkPointData* pd = input->GetPointData();

    for (int i = 0; i < pd->GetNumberOfArrays(); ++i)
    {
      vtkAbstractArray* a = pd->GetAbstractArray(i);
      vtkAbstractArray* b = a->NewInstance();

      // BitArray processed
      vtkBitArray *aBit = vtkArrayDownCast<vtkBitArray>(a);
      if (aBit)
      {
        vtkBitArray *bBit = vtkArrayDownCast<vtkBitArray>(b);
        bBit->SetNumberOfTuples(count / bBit->GetNumberOfComponents());

        // Assuming count is a number of values, not a number of tuples:
        for (vtkIdType j = offset; j < offset + count; ++j)
        {
          bBit->SetValue(j, aBit->GetValue(j + offset));
        }
      }
      // DataArray processed
      else
      {
        void* data = a->GetVoidPointer(offset);
        b->SetVoidArray(data, count, 0);
      }
      this->WriteArrayInline(b, descIndent.GetNextIndent(), a->GetName());
    }

    // Increment to next tree with PointData
    os << descIndent << "</PointData>\n";
    os << treeIndent << "</Tree>\n";
  }
  os << indent << "</Trees>\n";

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
