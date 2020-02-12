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

#include "vtkXMLHyperTreeGridWriter.h"

#include "vtkAbstractArray.h"
#include "vtkBitArray.h"
#include "vtkDoubleArray.h"
#include "vtkErrorCode.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnsignedLongArray.h"

#define vtkXMLOffsetsManager_DoNotInclude
#include "vtkXMLOffsetsManager.h"
#undef vtkXMLOffsetsManager_DoNotInclude

#include <cassert>

vtkStandardNewMacro(vtkXMLHyperTreeGridWriter);

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
int vtkXMLHyperTreeGridWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLHyperTreeGridWriter::WriteData()
{
  // write XML header and VTK file header and file attributes
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

  if (this->GetDataSetMajorVersion() < 1 && !this->WriteTrees_0(indent.GetNextIndent()))
  {
    return 0;
  }
  else if (this->GetDataSetMajorVersion() >= 1 && !this->WriteTrees_1(indent.GetNextIndent()))
  {
    return 0;
  }

  this->WriteFieldData(indent.GetNextIndent());

  if (!this->FinishPrimaryElement(indent))
  {
    return 0;
  }

  // Write all appended data by tree using Helper function
  if (this->DataMode == vtkXMLWriter::Appended)
  {
    vtkHyperTreeGrid* input = this->GetInput();
    vtkPointData* pd = input->GetPointData();
    vtkIdType numberOfPointDataArrays = pd->GetNumberOfArrays();

    this->StartAppendedData();

    // Write the field data arrays.
    if (this->FieldDataOM->GetNumberOfElements())
    {
      vtkNew<vtkFieldData> fieldDataCopy;
      this->UpdateFieldData(fieldDataCopy);

      this->WriteFieldDataAppendedData(fieldDataCopy, this->CurrentTimeIndex, this->FieldDataOM);
      if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
      {
        return 0;
      }
    }

    // Write the Coordinates arrays
    if (this->CoordsOMG->GetNumberOfElements())
    {
      assert(this->CoordsOMG->GetNumberOfElements() == 3);
      this->WriteAppendedArrayDataHelper(input->GetXCoordinates(), this->CoordsOMG->GetElement(0));
      this->WriteAppendedArrayDataHelper(input->GetYCoordinates(), this->CoordsOMG->GetElement(1));
      this->WriteAppendedArrayDataHelper(input->GetZCoordinates(), this->CoordsOMG->GetElement(2));
    }

    // Write the data for each tree
    vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
    input->InitializeTreeIterator(it);
    vtkIdType inIndex;
    int treeIndx = 0;

    if (this->GetDataSetMajorVersion() < 1) // Major version < 1
    {
      while (it.GetNextTree(inIndex))
      {
        vtkHyperTreeGridNonOrientedCursor* inCursor = input->NewNonOrientedCursor(inIndex);
        vtkHyperTree* tree = inCursor->GetTree();
        vtkIdType numberOfVertices = tree->GetNumberOfVertices();

        // Tree Descriptor
        this->WriteAppendedArrayDataHelper(
          this->Descriptors[treeIndx], this->DescriptorOMG->GetElement(treeIndx));
        // Tree Mask
        this->WriteAppendedArrayDataHelper(
          this->Masks[treeIndx], this->MaskOMG->GetElement(treeIndx));
        // Point Data
        for (int i = 0; i < pd->GetNumberOfArrays(); ++i)
        {
          vtkAbstractArray* array = pd->GetAbstractArray(i);
          int pdIndx = treeIndx * numberOfPointDataArrays + i;
          this->WritePointDataAppendedArrayDataHelper(
            array, numberOfVertices, this->PointDataOMG->GetElement(pdIndx), tree);
        }
        ++treeIndx;
        inCursor->Delete();
      }
    }
    else // Major version >= 1
    {
      while (it.GetNextTree(inIndex))
      {
        // Tree Descriptor
        this->WriteAppendedArrayDataHelper(
          this->Descriptors[treeIndx], this->DescriptorOMG->GetElement(treeIndx));
        // Tree NbVerticesbyLevels
        this->WriteAppendedArrayDataHelper(
          this->NbVerticesbyLevels[treeIndx], this->NbVerticesbyLevelOMG->GetElement(treeIndx));
        // Tree Mask
        this->WriteAppendedArrayDataHelper(
          this->Masks[treeIndx], this->MaskOMG->GetElement(treeIndx));
        // Point Data
        vtkIdList* ids = this->Ids[treeIndx];
        vtkIdType numberOfVertices = ids->GetNumberOfIds();
        for (int i = 0; i < pd->GetNumberOfArrays(); ++i)
        {
          vtkAbstractArray* a = pd->GetAbstractArray(i);
          vtkAbstractArray* b = a->NewInstance();
          int numberOfComponents = a->GetNumberOfComponents();
          b->SetNumberOfTuples(numberOfVertices);
          b->SetNumberOfComponents(numberOfComponents);
          b->SetNumberOfValues(numberOfComponents * numberOfVertices);
          // BitArray processed
          vtkBitArray* aBit = vtkArrayDownCast<vtkBitArray>(a);
          if (aBit)
          {
            vtkBitArray* bBit = vtkArrayDownCast<vtkBitArray>(b);
            aBit->GetTuples(ids, bBit);
          } // DataArray processed
          else
          {
            a->GetTuples(ids, b);
          }
          // Write the data or XML description for appended data
          int pdIndx = treeIndx * numberOfPointDataArrays + i;
          this->WriteAppendedArrayDataHelper(b, this->PointDataOMG->GetElement(pdIndx));
          b->Delete();
        }
        ++treeIndx;
      }
    }
    this->EndAppendedData();
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
void vtkXMLHyperTreeGridWriter::WritePrimaryElementAttributes(ostream& os, vtkIndent indent)
{
  this->Superclass::WritePrimaryElementAttributes(os, indent);
  vtkHyperTreeGrid* input = this->GetInput();

  int extent[6];
  input->GetExtent(extent);

  if (this->GetDataSetMajorVersion() < 1) // Major version < 1
  {
    this->WriteScalarAttribute("Dimension", (int)input->GetDimension());
    this->WriteScalarAttribute("Orientation", (int)input->GetOrientation());
  }

  this->WriteScalarAttribute("BranchFactor", (int)input->GetBranchFactor());
  this->WriteScalarAttribute("TransposedRootIndexing", (bool)input->GetTransposedRootIndexing());
  this->WriteVectorAttribute(
    "Dimensions", 3, (int*)const_cast<unsigned int*>(input->GetDimensions()));
  if (input->GetHasInterface())
  {
    this->WriteStringAttribute("InterfaceNormalsName", input->GetInterfaceNormalsName());
  }
  if (input->GetHasInterface())
  {
    this->WriteStringAttribute("InterfaceInterceptsName", input->GetInterfaceInterceptsName());
  }

  if (this->GetDataSetMajorVersion() < 1)
  {
    this->WriteScalarAttribute("NumberOfVertices", input->GetNumberOfVertices());
  }
}

//----------------------------------------------------------------------------
int vtkXMLHyperTreeGridWriter::WriteGrid(vtkIndent indent)
{
  vtkHyperTreeGrid* input = this->GetInput();
  ostream& os = *(this->Stream);
  os << indent << "<Grid>\n";

  if (this->DataMode == Appended)
  {
    // Coordinates of the grid
    this->CoordsOMG->Allocate(3, this->NumberOfTimeSteps);
    this->WriteArrayAppended(input->GetXCoordinates(), indent.GetNextIndent(),
      this->CoordsOMG->GetElement(0), "XCoordinates",
      input->GetXCoordinates()->GetNumberOfTuples());
    this->WriteArrayAppended(input->GetYCoordinates(), indent.GetNextIndent(),
      this->CoordsOMG->GetElement(1), "YCoordinates",
      input->GetYCoordinates()->GetNumberOfTuples());
    this->WriteArrayAppended(input->GetZCoordinates(), indent.GetNextIndent(),
      this->CoordsOMG->GetElement(2), "ZCoordinates",
      input->GetZCoordinates()->GetNumberOfTuples());
  }
  else
  {
    // Coordinates of the grid
    this->WriteArrayInline(input->GetXCoordinates(), indent.GetNextIndent(), "XCoordinates",
      input->GetXCoordinates()->GetNumberOfValues());
    this->WriteArrayInline(input->GetYCoordinates(), indent.GetNextIndent(), "YCoordinates",
      input->GetYCoordinates()->GetNumberOfValues());
    this->WriteArrayInline(input->GetZCoordinates(), indent.GetNextIndent(), "ZCoordinates",
      input->GetZCoordinates()->GetNumberOfValues());
  }

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
vtkXMLHyperTreeGridWriter::vtkXMLHyperTreeGridWriter()
  : CoordsOMG(new OffsetsManagerGroup)
  , DescriptorOMG(new OffsetsManagerGroup)
  , NbVerticesbyLevelOMG(new OffsetsManagerGroup)
  , MaskOMG(new OffsetsManagerGroup)
  , PointDataOMG(new OffsetsManagerGroup)
{
}

//----------------------------------------------------------------------------
vtkXMLHyperTreeGridWriter::~vtkXMLHyperTreeGridWriter()
{
  delete this->CoordsOMG;
  delete this->DescriptorOMG;
  delete this->NbVerticesbyLevelOMG;
  delete this->MaskOMG;
  delete this->PointDataOMG;
  for (auto i = this->Descriptors.begin(); i != this->Descriptors.end(); ++i)
  {
    (*i)->Delete();
  }
  for (auto i = this->NbVerticesbyLevels.begin(); i != this->NbVerticesbyLevels.end(); ++i)
  {
    (*i)->Delete();
  }
  for (auto i = this->Masks.begin(); i != this->Masks.end(); ++i)
  {
    (*i)->Delete();
  }
  for (auto i = this->Ids.begin(); i != this->Ids.end(); ++i)
  {
    (*i)->Delete();
  }
}

//----------------------------------------------------------------------------
namespace
{
//
// Depth first recursion to walk the tree in child order
// Used to create the breadth first BitArray descriptor appending
// node and leaf indicator by level
//
void BuildDescriptor(vtkHyperTreeGridNonOrientedCursor* inCursor, int level,
  std::vector<std::string>& descriptor, std::vector<std::string>& mask)
{
  // Retrieve input grid
  vtkHyperTreeGrid* input = inCursor->GetGrid();

  // Append to mask string
  vtkIdType id = inCursor->GetGlobalNodeIndex();
  if (input->HasMask())
  {
    if (input->GetMask()->GetValue(id))
      mask[level] += '1';
    else
      mask[level] += '0';
  }

  // Append to descriptor string
  if (!inCursor->IsLeaf())
  {
    descriptor[level] += 'R';

    // If input cursor is not a leaf, recurse to all children
    int numChildren = input->GetNumberOfChildren();
    for (int child = 0; child < numChildren; ++child)
    {
      // Create child cursor from parent in input grid
      vtkHyperTreeGridNonOrientedCursor* childCursor = inCursor->Clone();
      childCursor->ToChild(child);

      // Recurse
      BuildDescriptor(childCursor, level + 1, descriptor, mask);

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
int vtkXMLHyperTreeGridWriter::WriteTrees_0(vtkIndent indent)
{
  vtkHyperTreeGrid* input = this->GetInput();
  vtkIdType maxLevels = input->GetNumberOfLevels();
  vtkPointData* pd = input->GetPointData();
  vtkIdType numberOfPointDataArrays = pd->GetNumberOfArrays();

  // Count the actual number of hypertrees represented in this hypertree grid
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator(it);
  this->NumberOfTrees = 0;
  vtkIdType inIndex;
  while (it.GetNextTree(inIndex))
  {
    ++this->NumberOfTrees;
  }

  // Allocate offsets managers for appended data
  if (this->DataMode == Appended && this->NumberOfTrees > 0)
  {
    this->DescriptorOMG->Allocate(this->NumberOfTrees, this->NumberOfTimeSteps);
    this->MaskOMG->Allocate(this->NumberOfTrees, this->NumberOfTimeSteps);
    this->PointDataOMG->Allocate(
      this->NumberOfTrees * numberOfPointDataArrays, this->NumberOfTimeSteps);
  }

  ostream& os = *(this->Stream);
  os << indent << "<Trees>\n";
  vtkIndent treeIndent = indent.GetNextIndent();

  // Collect description by processing depth first and writing breadth first
  input->InitializeTreeIterator(it);
  int treeIndx = 0;
  vtkIdType globalOffset = 0;
  while (it.GetNextTree(inIndex))
  {
    // Initialize new grid cursor at root of current input tree
    vtkHyperTreeGridNonOrientedCursor* inCursor = input->NewNonOrientedCursor(inIndex);
    vtkHyperTree* tree = inCursor->GetTree();
    vtkIdType numberOfVertices = tree->GetNumberOfVertices();

    os << treeIndent << "<Tree";
    this->WriteScalarAttribute("Index", inIndex);
    this->WriteScalarAttribute("GlobalOffset", globalOffset);
    this->WriteScalarAttribute("NumberOfVertices", numberOfVertices);
    os << ">\n";

    // Recursively compute descriptor for this tree, appending any
    // entries for each of the levels in descByLevel for output.
    // Collect the masked indicator at the same time
    std::vector<std::string> descByLevel(maxLevels);
    std::vector<std::string> maskByLevel(maxLevels);
    BuildDescriptor(inCursor, 0, descByLevel, maskByLevel);

    // Clean up
    inCursor->Delete();

    // Descriptor BitArray
    vtkBitArray* descriptor = vtkBitArray::New();
    std::string::const_iterator liter;
    for (int l = 0; l < maxLevels; ++l)
    {
      for (liter = descByLevel[l].begin(); liter != descByLevel[l].end(); ++liter)
      {
        switch (*liter)
        {
          case 'R': //  Refined cell
            descriptor->InsertNextValue(1);
            break;
          case '.': // Leaf cell
            descriptor->InsertNextValue(0);
            break;
          default:
            vtkErrorMacro(<< "Unrecognized character: " << *liter << " in string "
                          << descByLevel[l]);
            return 0;
        }
      }
    }
    descriptor->Squeeze();
    this->Descriptors.push_back(descriptor);

    // Mask BitAarray
    vtkBitArray* mask = vtkBitArray::New();
    if (input->GetMask())
    {
      for (int l = 0; l < maxLevels; ++l)
      {
        for (liter = maskByLevel[l].begin(); liter != maskByLevel[l].end(); ++liter)
        {
          switch (*liter)
          {
            case '0': // Not masked
              mask->InsertNextValue(0);
              break;
            case '1': // Masked
              mask->InsertNextValue(1);
              break;
            default:
              vtkErrorMacro(<< "Unrecognized character: " << *liter << " in string "
                            << maskByLevel[l]);
              return 0;
          }
        }
      }
      mask->Squeeze();
      this->Masks.push_back(mask);
    }

    vtkIndent infoIndent = treeIndent.GetNextIndent();

    // Write the descriptor and mask BitArrays
    if (this->DataMode == Appended)
    {
      this->WriteArrayAppended(descriptor, infoIndent, this->DescriptorOMG->GetElement(treeIndx),
        "Descriptor", descriptor->GetNumberOfValues());
      if (input->GetMask())
      {
        this->WriteArrayAppended(
          mask, infoIndent, this->MaskOMG->GetElement(treeIndx), "Mask", mask->GetNumberOfValues());
      }
    }
    else
    {
      this->WriteArrayInline(descriptor, infoIndent, "Descriptor", descriptor->GetNumberOfValues());
      if (input->GetMask())
      {
        this->WriteArrayInline(mask, infoIndent, "Mask", mask->GetNumberOfValues());
      }
    }

    // Write the point data
    os << infoIndent << "<PointData>\n";
    for (int i = 0; i < pd->GetNumberOfArrays(); ++i)
    {
      vtkAbstractArray* a = pd->GetAbstractArray(i);
      vtkAbstractArray* b = a->NewInstance();
      int numberOfComponents = a->GetNumberOfComponents();
      b->SetNumberOfTuples(numberOfVertices);
      b->SetNumberOfComponents(numberOfComponents);
      for (int e = 0; e < numberOfVertices; ++e)
      {
        // note - we unravel the array contents which may be interleaved in input array.
        // The reader expect that each grid's data will be contiguous and uses "GlobalOffset"
        // to assemble a big array on the other side.
        // The in memory order of elements then isn't necessarily the same but HTG handles that.
        int aDataOffset = tree->GetGlobalIndexFromLocal(e) * numberOfComponents;
        int bDataOffset = e * numberOfComponents;
        for (int c = 0; c < numberOfComponents; ++c)
        {
          b->SetVariantValue(bDataOffset + c, a->GetVariantValue(aDataOffset + c));
        }
      }

      // Write the data or XML description for appended data
      if (this->DataMode == Appended)
      {
        this->WriteArrayAppended(b, infoIndent.GetNextIndent(),
          this->PointDataOMG->GetElement(treeIndx * numberOfPointDataArrays + i), a->GetName(),
          numberOfVertices * numberOfComponents);
      }
      else
      {
        this->WriteArrayInline(
          b, infoIndent.GetNextIndent(), a->GetName(), numberOfVertices * numberOfComponents);
      }
      b->Delete();
    }
    ++treeIndx;

    // Increment to next tree with PointData
    os << infoIndent << "</PointData>\n";
    os << treeIndent << "</Tree>\n";
    globalOffset += numberOfVertices;
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
int vtkXMLHyperTreeGridWriter::WriteTrees_1(vtkIndent indent)
{
  vtkHyperTreeGrid* input = this->GetInput();
  vtkPointData* pd = input->GetPointData();
  vtkIdType numberOfPointDataArrays = pd->GetNumberOfArrays();

  // Count the actual number of hypertrees represented in this hypertree grid
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator(it);
  this->NumberOfTrees = 0;
  vtkIdType inIndex;
  while (it.GetNextTree(inIndex))
  {
    ++this->NumberOfTrees;
  }

  // Allocate offsets managers for appended data
  if (this->DataMode == Appended && this->NumberOfTrees > 0)
  {
    this->DescriptorOMG->Allocate(this->NumberOfTrees, this->NumberOfTimeSteps);
    this->NbVerticesbyLevelOMG->Allocate(this->NumberOfTrees, this->NumberOfTimeSteps);
    this->MaskOMG->Allocate(this->NumberOfTrees, this->NumberOfTimeSteps);
    this->PointDataOMG->Allocate(
      this->NumberOfTrees * numberOfPointDataArrays, this->NumberOfTimeSteps);
  }

  ostream& os = *(this->Stream);
  os << indent << "<Trees>\n";
  vtkIndent treeIndent = indent.GetNextIndent();

  // Collect description by processing depth first and writing breadth first
  input->InitializeTreeIterator(it);
  int treeIndx = 0;
  while (it.GetNextTree(inIndex))
  {
    os << treeIndent << "<Tree";
    this->WriteScalarAttribute("Index", inIndex);
    vtkHyperTree* tree = input->GetTree(inIndex);
    vtkIdType numberOfLevels = tree->GetNumberOfLevels();
    this->WriteScalarAttribute("NumberOfLevels", numberOfLevels);

    vtkUnsignedLongArray* nbVerticesbyLevel = vtkUnsignedLongArray::New();
    vtkBitArray* descriptor = vtkBitArray::New();
    vtkBitArray* mask = vtkBitArray::New();
    vtkIdList* ids = vtkIdList::New();
    tree->GetByLevelForWriter(input->GetMask(), nbVerticesbyLevel, descriptor, mask, ids);
    this->NbVerticesbyLevels.push_back(nbVerticesbyLevel);
    this->Descriptors.push_back(descriptor);
    this->Masks.push_back(mask);
    this->Ids.push_back(ids);

    vtkIndent infoIndent = treeIndent.GetNextIndent();

    vtkIdType numberOfVertices = ids->GetNumberOfIds();
    // Because non describe last False values...
    assert(numberOfVertices >= descriptor->GetNumberOfTuples());
    assert(numberOfVertices >= mask->GetNumberOfTuples());
    this->WriteScalarAttribute("NumberOfVertices", numberOfVertices);
    os << ">\n";

    // Write the descriptor and mask BitArrays
    if (this->DataMode == Appended)
    {
      this->WriteArrayAppended(descriptor, infoIndent, this->DescriptorOMG->GetElement(treeIndx),
        "Descriptor", descriptor->GetNumberOfValues());
      this->WriteArrayAppended(nbVerticesbyLevel, infoIndent,
        this->NbVerticesbyLevelOMG->GetElement(treeIndx), "NbVerticesByLevel",
        nbVerticesbyLevel->GetNumberOfValues());
      if (input->GetMask())
      {
        this->WriteArrayAppended(
          mask, infoIndent, this->MaskOMG->GetElement(treeIndx), "Mask", mask->GetNumberOfValues());
      }
    }
    else
    {
      this->WriteArrayInline(descriptor, infoIndent, "Descriptor", descriptor->GetNumberOfValues());
      this->WriteArrayInline(
        nbVerticesbyLevel, infoIndent, "NbVerticesByLevel", nbVerticesbyLevel->GetNumberOfValues());
      if (input->GetMask())
      {
        this->WriteArrayInline(mask, infoIndent, "Mask", mask->GetNumberOfValues());
      }
    }

    // Write the point data
    os << infoIndent << "<PointData>\n";

    for (int i = 0; i < pd->GetNumberOfArrays(); ++i)
    {
      vtkAbstractArray* a = pd->GetAbstractArray(i);

      // Write the data or XML description for appended data
      if (this->DataMode == Appended)
      {
        this->WriteArrayAppended(a, infoIndent.GetNextIndent(),
          this->PointDataOMG->GetElement(treeIndx * numberOfPointDataArrays + i), a->GetName(), 0);
        // Last parameter will eventually become the size of the stored arrays.
        // When this modification is done, 0 should be replaced by:
        // numberOfVertices * a->GetNumberOfComponents());
      }
      else
      {
        vtkAbstractArray* b = a->NewInstance();
        int numberOfComponents = a->GetNumberOfComponents();
        b->SetNumberOfTuples(numberOfVertices);
        b->SetNumberOfComponents(numberOfComponents);
        b->SetNumberOfValues(numberOfComponents * numberOfVertices);
        // BitArray processed
        vtkBitArray* aBit = vtkArrayDownCast<vtkBitArray>(a);
        if (aBit)
        {
          vtkBitArray* bBit = vtkArrayDownCast<vtkBitArray>(b);
          aBit->GetTuples(ids, bBit);
        } // DataArray processed
        else
        {
          a->GetTuples(ids, b);
        }
        this->WriteArrayInline(
          b, infoIndent.GetNextIndent(), a->GetName(), b->GetNumberOfTuples() * numberOfComponents);
        b->Delete();
      }
    }
    ++treeIndx;

    // Increment to next tree with PointData
    os << infoIndent << "</PointData>\n";
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

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridWriter::WriteAppendedArrayDataHelper(
  vtkAbstractArray* array, OffsetsManager& offsets)
{
  this->WriteArrayAppendedData(array, offsets.GetPosition(this->CurrentTimeIndex),
    offsets.GetOffsetValue(this->CurrentTimeIndex));

  vtkDataArray* dArray = vtkArrayDownCast<vtkDataArray>(array);
  if (dArray)
  {
    double* range = dArray->GetRange(-1);
    this->ForwardAppendedDataDouble(
      offsets.GetRangeMinPosition(this->CurrentTimeIndex), range[0], "RangeMin");
    this->ForwardAppendedDataDouble(
      offsets.GetRangeMaxPosition(this->CurrentTimeIndex), range[1], "RangeMax");
  }
}

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridWriter::WritePointDataAppendedArrayDataHelper(
  vtkAbstractArray* a, vtkIdType numberOfVertices, OffsetsManager& offsets, vtkHyperTree* tree)
{
  vtkAbstractArray* b = a->NewInstance();
  int numberOfComponents = a->GetNumberOfComponents();

  b->SetNumberOfComponents(numberOfComponents);
  b->SetNumberOfTuples(numberOfVertices);
  for (int e = 0; e < numberOfComponents * numberOfVertices; ++e)
  {
    b->SetVariantValue(e, a->GetVariantValue(tree->GetGlobalIndexFromLocal(e)));
  }

  this->WriteArrayAppendedData(
    b, offsets.GetPosition(this->CurrentTimeIndex), offsets.GetOffsetValue(this->CurrentTimeIndex));

  vtkDataArray* dArray = vtkArrayDownCast<vtkDataArray>(a);
  if (dArray)
  {
    double* range = dArray->GetRange(-1);
    this->ForwardAppendedDataDouble(
      offsets.GetRangeMinPosition(this->CurrentTimeIndex), range[0], "RangeMin");
    this->ForwardAppendedDataDouble(
      offsets.GetRangeMaxPosition(this->CurrentTimeIndex), range[1], "RangeMax");
  }
  b->Delete();
}
