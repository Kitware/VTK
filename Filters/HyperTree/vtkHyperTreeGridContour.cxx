// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHyperTreeGridContour.h"

#include "vtkArrayDispatch.h"
#include "vtkBitArray.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkCompositeArray.h"
#include "vtkContourHelper.h"
#include "vtkContourValues.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridNonOrientedMooreSuperCursor.h"
#include "vtkIdTypeArray.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkIndexedArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLine.h"
#include "vtkMathUtilities.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPixel.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyhedron.h"
#include "vtkPolyhedronUtilities.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVoxel.h"

#include <algorithm>
#include <memory>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
const unsigned int MooreCursors1D[2] = { 0, 2 };
const unsigned int MooreCursors2D[8] = { 0, 1, 2, 3, 5, 6, 7, 8 };
const unsigned int MooreCursors3D[26] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 16, 17,
  18, 19, 20, 21, 22, 23, 24, 25, 26 };
const unsigned int* MooreCursors[3] = {
  MooreCursors1D,
  MooreCursors2D,
  MooreCursors3D,
};

// Conversion table of canonical ids from voxel to polyhedron
constexpr vtkIdType CANONICAL_FACES[24] = { 2, 3, 1, 0, 1, 5, 4, 0, 4, 6, 2, 0, 3, 7, 5, 1, 2, 6, 7,
  3, 5, 7, 6, 4 };
constexpr vtkIdType POLY_FACES_NB = 6;
constexpr vtkIdType POLY_FACES_POINTS_NB = 4;
constexpr vtkIdType POLY_POINTS_NB = 8;

constexpr int MAX_NB_OF_CONTOURS = std::numeric_limits<unsigned char>::max() + 1; // 256

// Return true if all faces of the cell are planar.
// The cell is expected to be a vtkVoxel instance.
bool AreAllFacesPlanar(vtkCell* cell)
{
  bool allFacesArePlanar = true;

  std::array<std::array<double, 3>, POLY_FACES_POINTS_NB> facePoints{};

  // For each face
  for (int faceId = 0, canonicalId = 0; faceId < ::POLY_FACES_NB; faceId++)
  {
    // Retrieve face points
    for (int i = 0; i < ::POLY_FACES_POINTS_NB; i++, canonicalId++)
    {
      auto point = cell->GetPoints()->GetPoint(::CANONICAL_FACES[canonicalId]);
      facePoints[i][0] = point[0];
      facePoints[i][1] = point[1];
      facePoints[i][2] = point[2];
    }

    // Test if 3 vectors of the face are coplanar
    std::array<double, 3> v1{};
    v1[0] = facePoints[1][0] - facePoints[0][0];
    v1[1] = facePoints[1][1] - facePoints[0][1];
    v1[2] = facePoints[1][2] - facePoints[0][2];

    std::array<double, 3> v2{};
    v2[0] = facePoints[2][0] - facePoints[0][0];
    v2[1] = facePoints[2][1] - facePoints[0][1];
    v2[2] = facePoints[2][2] - facePoints[0][2];

    std::array<double, 3> v3{};
    v3[0] = facePoints[3][0] - facePoints[0][0];
    v3[1] = facePoints[3][1] - facePoints[0][1];
    v3[2] = facePoints[3][2] - facePoints[0][2];

    double cross[3] = { 0. };
    vtkMath::Cross(v1, v2, cross);

    if (!vtkMathUtilities::FuzzyCompare(vtkMath::Dot(cross, v3), 0.))
    {
      allFacesArePlanar = false;
      break;
    }
  }

  return allFacesArePlanar;
}

// Given contour values, find a "valid" epsilon value, allowing to discriminate values
// by fuzzy comparison. Returned espilon correspond to the min difference between contour
// values divided by 10.
double FindEpsilon(vtkDataArray* contourValues)
{
  vtkIdType numberOfContours = contourValues->GetNumberOfTuples();
  if (numberOfContours == 0)
  {
    vtkErrorWithObjectMacro(nullptr, "No contour values found");
    return 0;
  }

  // Sort contour values
  std::vector<double> sortedContourValues;
  sortedContourValues.reserve(numberOfContours);
  for (int contourId = 0; contourId < numberOfContours; contourId++)
  {
    sortedContourValues.emplace_back(contourValues->GetTuple1(contourId));
  }
  std::sort(sortedContourValues.begin(), sortedContourValues.end());

  // Find smallest difference between 2 values
  double epsilon = std::numeric_limits<double>::max();
  double contourValue1 = sortedContourValues[0];
  for (int contourId = 1; contourId < numberOfContours; contourId++)
  {
    double contourValue2 = sortedContourValues[contourId];
    double difference = contourValue2 - contourValue1;
    contourValue1 = contourValue2; // For next iteration

    // Avoid dupplicated contour values (compare using std::numeric_limits<double>::epsilon)
    if (vtkMathUtilities::FuzzyCompare(difference, 0.))
    {
      continue;
    }
    if (difference < epsilon)
    {
      epsilon = difference;
    }
  }

  // Ensure there is no overlap by dividing min diff by 10
  return epsilon * 0.1;
}

// Given the contour array and the contour values, generate handles by associating
// each value of contourArray to its corresponding index in contourValues
vtkSmartPointer<vtkUnsignedCharArray> GenerateHandles(
  vtkDataArray* contourArray, vtkDataArray* contourValues)
{
  vtkIdType nbOfPoints = contourArray->GetNumberOfTuples();
  vtkIdType numberOfContours = contourValues->GetNumberOfTuples();

  // Initialize handles
  vtkNew<vtkUnsignedCharArray> handles;
  handles->SetNumberOfComponents(1);
  handles->SetNumberOfTuples(nbOfPoints);
  // numberOfContours plays the role of id pointing to the default value
  // that will be used if no contourValue index is found
  handles->Fill(numberOfContours);

  double epsilon = ::FindEpsilon(contourValues);

  for (vtkIdType pointId = 0; pointId < nbOfPoints; pointId++)
  {
    int contourId = 0;
    for (; contourId < numberOfContours; contourId++)
    {
      if (vtkMathUtilities::FuzzyCompare(
            contourArray->GetTuple1(pointId), contourValues->GetTuple1(contourId), epsilon))
      {
        handles->SetValue(pointId, contourId);
        break;
      }
    }
    if (contourId == numberOfContours)
    {
      vtkErrorWithObjectMacro(nullptr,
        "Unable to retrieve contour value for point " << pointId << " with value "
                                                      << contourArray->GetTuple1(pointId));
    }
  }

  return handles;
}

// Given the contour array, the contour values and the output attributes,
// replace the contour array found in the attibutes by an implicit array.
struct ConvertToIndexedArrayWorker
{
  template <typename ArrayType>
  void operator()(ArrayType* contourArray, vtkContourValues* contourValues,
    vtkDataSetAttributes* outputAttributes) const
  {
    vtkIdType nbOfPoints = contourArray->GetNumberOfTuples();
    int numberOfContours = contourValues->GetNumberOfContours();

    using ValueType = vtk::GetAPIType<ArrayType>;

    // Fill values indexed by handles
    vtkNew<ArrayType> valuesArray;
    valuesArray->SetNumberOfComponents(1);
    valuesArray->SetNumberOfTuples(numberOfContours);
    for (int i = 0; i < numberOfContours; i++)
    {
      ValueType newVal = 0;
      vtkMath::RoundDoubleToIntegralIfNecessary(contourValues->GetValue(i), &newVal);
      valuesArray->SetValue(i, newVal);
    }

    // Fill handles
    vtkSmartPointer<vtkUnsignedCharArray> handles = ::GenerateHandles(contourArray, valuesArray);

    // Create array carrying the fallback default value
    vtkNew<ArrayType> defaultValueArray;
    defaultValueArray->SetNumberOfComponents(1);
    defaultValueArray->SetNumberOfTuples(1);
    if (defaultValueArray->GetDataType() == VTK_FLOAT ||
      defaultValueArray->GetDataType() == VTK_DOUBLE)
    {
      defaultValueArray->SetValue(0, vtkMath::Nan());
    }
    else
    {
      defaultValueArray->SetValue(0, 0);
    }

    // Create composite array (indexed values + default value)
    std::vector<vtkDataArray*> arrays({ valuesArray, defaultValueArray });

    vtkNew<vtkCompositeArray<ValueType>> compositeArr;
    compositeArr->SetBackend(std::make_shared<vtkCompositeImplicitBackend<ValueType>>(arrays));
    compositeArr->SetNumberOfComponents(1);
    // Allocate one more tuple to store the default value
    compositeArr->SetNumberOfTuples(valuesArray->GetNumberOfTuples() + 1);

    // Create indexed array from handles and composite array
    auto contourArrayName = contourArray->GetName();
    vtkNew<vtkIndexedArray<ValueType>> indexedArray;
    indexedArray->SetBackend(
      std::make_shared<vtkIndexedImplicitBackend<ValueType>>(handles, compositeArr));
    indexedArray->SetNumberOfComponents(1);
    indexedArray->SetNumberOfTuples(nbOfPoints);
    indexedArray->SetName(contourArrayName);

    // Replace the interpolated contour values by indexed ones
    outputAttributes->RemoveArray(contourArrayName);
    outputAttributes->AddArray(indexedArray);
  }
};

// Given the contour array name, the contour values and the output attributes,
// replace the contour array found in the attibutes by an indexed array.
// If there are less than 256 contour values:
// - store these values in a new array, removing duplicates
// - use a vtkUnsignedCharArray to index these values (handles)
// If there is strictly more than 256 contour values, this function will do nothing.
void ReplaceWithIndexedArray(const std::string& contourArrayName, vtkContourValues* contourValues,
  vtkDataSetAttributes* outputAttributes)
{
  int numberOfContours = contourValues->GetNumberOfContours();
  if (numberOfContours > MAX_NB_OF_CONTOURS) // 256
  {
    vtkDebugWithObjectMacro(nullptr,
      "There are more than " << MAX_NB_OF_CONTOURS << " values in contourValues. "
                             << "ReplaceWithIndexedArray will do nothing.");
    return;
  }

  if (!outputAttributes)
  {
    vtkErrorWithObjectMacro(nullptr, "Unable to retrieve output attributes");
    return;
  }

  auto contourArray =
    vtkDataArray::SafeDownCast(outputAttributes->GetAbstractArray(contourArrayName.c_str()));
  if (!contourArray)
  {
    vtkErrorWithObjectMacro(
      nullptr, "Unable to retrieve contour array " << contourArrayName << " from input attributes");
    return;
  }

  using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;
  ::ConvertToIndexedArrayWorker worker;

  // Dispatch
  if (!Dispatcher::Execute(contourArray, worker, contourValues, outputAttributes))
  {
    vtkErrorWithObjectMacro(nullptr, "Unable to dispatch the contour array " << contourArrayName);
  }
}
}

//------------------------------------------------------------------------------
struct vtkHyperTreeGridContour::vtkInternals
{
  // Temporary data structures related to USE_DECOMPOSED_POLYHEDRA strategy
  vtkNew<vtkCellArray> Faces;
  vtkNew<vtkPolyhedron> Polyhedron;
  vtkNew<vtkGenericCell> Tetra;
  vtkNew<vtkDoubleArray> TetraScalars;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkHyperTreeGridContour);

//------------------------------------------------------------------------------
vtkHyperTreeGridContour::vtkHyperTreeGridContour()
  : Internals(new vtkHyperTreeGridContour::vtkInternals())
{
  // Initialize storage for contour values
  this->ContourValues = vtkContourValues::New();

  // Initialize locator to null
  this->Locator = nullptr;

  // Initialize list of selected cells
  this->SelectedCells = nullptr;

  // Initialize per-cell quantities of interest
  this->CellSigns = nullptr;
  this->CellScalars = nullptr;

  // Initialize structures for isocontouring
  this->Helper = nullptr;
  this->Leaves = vtkIdList::New();
  this->Line = vtkLine::New();
  this->Pixel = vtkPixel::New();
  this->Voxel = vtkVoxel::New();

  // Output indices begin at 0
  this->CurrentId = 0;

  // Process active point scalars by default
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::SCALARS);

  // Input scalars point to null by default
  this->InScalars = nullptr;

  // Initialize temporal structures related to USE_DECOMPOSED_POLYHEDRA strategy
  this->Internals->Polyhedron->GetPointIds()->SetNumberOfIds(::POLY_POINTS_NB);
  this->Internals->Polyhedron->GetPoints()->SetNumberOfPoints(::POLY_POINTS_NB);
  this->Internals->Faces->AllocateExact(::POLY_FACES_NB, ::POLY_FACES_POINTS_NB * ::POLY_FACES_NB);
}

//------------------------------------------------------------------------------
vtkHyperTreeGridContour::~vtkHyperTreeGridContour()
{
  if (this->ContourValues)
  {
    this->ContourValues->Delete();
    this->ContourValues = nullptr;
  }

  if (this->Locator)
  {
    this->Locator->Delete();
    this->Locator = nullptr;
  }

  if (this->Line)
  {
    this->Line->Delete();
    this->Line = nullptr;
  }

  if (this->Pixel)
  {
    this->Pixel->Delete();
    this->Pixel = nullptr;
  }

  if (this->Voxel)
  {
    this->Voxel->Delete();
    this->Voxel = nullptr;
  }

  if (this->Leaves)
  {
    this->Leaves->Delete();
    this->Leaves = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridContour::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  this->ContourValues->PrintSelf(os, indent.GetNextIndent());

  os << indent << "CurrentId: " << this->CurrentId << endl;

  if (this->InScalars)
  {
    os << indent << "InScalars:\n";
    this->InScalars->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "InScalars: ( none )\n";
  }

  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }

  if (this->Line)
  {
    os << indent << ": " << this->Line << "\n";
  }
  else
  {
    os << indent << ": (none)\n";
  }

  if (this->Pixel)
  {
    os << indent << ": " << this->Pixel << "\n";
  }
  else
  {
    os << indent << ": (none)\n";
  }

  if (this->Voxel)
  {
    os << indent << ": " << this->Voxel << "\n";
  }
  else
  {
    os << indent << ": (none)\n";
  }

  if (this->Leaves)
  {
    os << indent << ": " << this->Leaves << "\n";
  }
  else
  {
    os << indent << ": (none)\n";
  }
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridContour::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridContour::SetLocator(vtkIncrementalPointLocator* locator)
{
  // Check if proposed locator is identical to existing one
  if (this->Locator == locator)
  {
    return;
  }

  // Clean up existing locator instance variable
  if (this->Locator)
  {
    this->Locator->Delete();
    this->Locator = nullptr;
  }

  // Register proposed locator and assign it
  if (locator)
  {
    locator->Register(this);
  }
  this->Locator = locator;

  // Modify time
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridContour::CreateDefaultLocator()
{
  // If no locator instance variable create a merge point one
  if (!this->Locator)
  {
    this->Locator = vtkMergePoints::New();
    this->Locator->Register(this);
    this->Locator->Delete();
  }
}

//------------------------------------------------------------------------------
vtkMTimeType vtkHyperTreeGridContour::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType time;

  if (this->ContourValues)
  {
    time = this->ContourValues->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  if (this->Locator)
  {
    time = this->Locator->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridContour::ProcessTrees(vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  // Downcast output data object to polygonal data set
  vtkPolyData* output = vtkPolyData::SafeDownCast(outputDO);
  if (!output)
  {
    vtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  // Retrieve scalar quantity of interest
  this->InScalars = this->GetInputArrayToProcess(0, input);
  if (!this->InScalars)
  {
    vtkWarningMacro(<< "No scalar data to contour");
    return 1;
  }

  // Initialize output point data
  this->InData = input->GetCellData();
  this->OutData = output->GetPointData();
  this->OutData->CopyAllocate(this->InData);

  // Output indices begin at 0
  this->CurrentId = 0;

  // Retrieve material mask
  this->InMask = input->HasMask() ? input->GetMask() : nullptr;

  // Retrieve ghost cells
  this->InGhostArray = input->GetGhostCells();

  // Estimate output size as a multiple of 1024
  vtkIdType numCells = input->GetNumberOfCells();
  vtkIdType numContours = this->ContourValues->GetNumberOfContours();
  vtkIdType estimatedSize = static_cast<vtkIdType>(pow(static_cast<double>(numCells), .75));
  estimatedSize *= numContours;
  estimatedSize = estimatedSize / 1024 * 1024;
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }

  // Create storage for output points
  vtkPoints* newPts = vtkPoints::New();
  newPts->Allocate(estimatedSize, estimatedSize);

  // Create storage for output vertices
  vtkNew<vtkCellArray> newVerts;
  newVerts->AllocateExact(estimatedSize, estimatedSize);

  // Create storage for output lines
  vtkNew<vtkCellArray> newLines;
  newLines->AllocateExact(estimatedSize, estimatedSize);

  // Create storage for output polygons
  vtkNew<vtkCellArray> newPolys;
  newPolys->AllocateExact(estimatedSize, estimatedSize);

  // Create storage for output scalar values
  this->CellScalars = this->InScalars->NewInstance();
  this->CellScalars->SetNumberOfComponents(this->InScalars->GetNumberOfComponents());
  this->CellScalars->Allocate(this->CellScalars->GetNumberOfComponents() * 8);

  // Initialize point locator
  if (!this->Locator)
  {
    // Create default locator if needed
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion(newPts, input->GetBounds(), estimatedSize);

  // Used to store the input cell data (hyper tree grid cells)
  // as point data (dual mesh point data), the two being equivalent.
  vtkNew<vtkPointData> dualPointData;
  dualPointData->PassData(input->GetCellData());

  // Instantiate a contour helper for convenience, with triangle generation on
  this->Helper = new vtkContourHelper(this->Locator, newVerts, newLines, newPolys, dualPointData,
    nullptr, output->GetPointData(), nullptr, estimatedSize, true);

  // Create storage to keep track of selected cells
  this->SelectedCells = vtkBitArray::New();
  this->SelectedCells->SetNumberOfTuples(numCells);

  // Initialize storage for signs and values
  // NOLINTNEXTLINE(bugprone-sizeof-expression)
  this->CellSigns = (vtkBitArray**)malloc(numContours * sizeof(*this->CellSigns));
  this->Signs.resize(numContours, true);
  for (int c = 0; c < numContours; ++c)
  {
    this->CellSigns[c] = vtkBitArray::New();
    this->CellSigns[c]->SetNumberOfTuples(numCells);
  }

  // First pass across tree roots to evince cells intersected by contours
  vtkIdType index;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator(it);
  vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor;
  while (it.GetNextTree(index))
  {
    if (this->CheckAbort())
    {
      break;
    }
    // Initialize new grid cursor at root of current input tree
    input->InitializeNonOrientedCursor(cursor, index);
    // Pre-process tree recursively
    this->RecursivelyPreProcessTree(cursor);
  } // it

  // Second pass across tree roots: now compute isocontours recursively
  input->InitializeTreeIterator(it);
  vtkNew<vtkHyperTreeGridNonOrientedMooreSuperCursor> supercursor;
  while (it.GetNextTree(index))
  {
    if (this->CheckAbort())
    {
      break;
    }
    // Initialize new Moore cursor at root of current tree
    input->InitializeNonOrientedMooreSuperCursor(supercursor, index);
    // Compute contours recursively
    this->RecursivelyProcessTree(supercursor, newVerts, newLines, newPolys, dualPointData);
  } // it

  // Set output
  output->SetPoints(newPts);
  if (newVerts->GetNumberOfCells())
  {
    output->SetVerts(newVerts);
  }
  if (newLines->GetNumberOfCells())
  {
    output->SetLines(newLines);
  }
  if (newPolys->GetNumberOfCells())
  {
    output->SetPolys(newPolys);
  }

  // Replace values from contour with implicit array if needed
  if (this->UseImplicitArrays && numContours <= 256)
  {
    const std::string contourValuesArrayName = this->InScalars->GetName();
    ::ReplaceWithIndexedArray(contourValuesArrayName, this->ContourValues, output->GetPointData());
  }

  // Clean up
  this->SelectedCells->Delete();
  for (vtkIdType c = 0; c < this->GetNumberOfContours(); ++c)
  {
    if (this->CellSigns[c])
    {
      this->CellSigns[c]->Delete();
    }
  } // c
  free(this->CellSigns);
  delete this->Helper;
  this->CellScalars->Delete();
  newPts->Delete();
  this->Locator->Initialize();

  // Squeeze output
  output->Squeeze();

  return 1;
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridContour::RecursivelyPreProcessTree(vtkHyperTreeGridNonOrientedCursor* cursor)
{
  // Retrieve global index of input cursor
  vtkIdType id = cursor->GetGlobalNodeIndex();

  if (this->InGhostArray && this->InGhostArray->GetTuple1(id))
  {
    return false;
  }

  // Retrieve number of contours
  vtkIdType numContours = this->ContourValues->GetNumberOfContours();

  // Descend further into input trees only if cursor is not a leaf
  bool selected = false;
  if (!cursor->IsLeaf() && !cursor->IsMasked())
  {
    // Cursor is not at leaf, recurse to all all children
    int numChildren = cursor->GetNumberOfChildren();
    for (int child = 0; child < numChildren; ++child)
    {
      if (this->CheckAbort())
      {
        break;
      }
      // Create storage for signs relative to contour values
      std::vector<bool> signs(numContours);

      cursor->ToChild(child);

      // Recurse and keep track of whether this branch is selected
      selected |= this->RecursivelyPreProcessTree(cursor);

      // Check if branch not completely selected
      if (!selected)
      {
        // If not, update contour values
        for (int c = 0; c < numContours; ++c)
        {
          // Retrieve global index of child
          vtkIdType childId = cursor->GetGlobalNodeIndex();

          // Compute and store selection flags for current contour
          if (!child)
          {
            // Initialize sign array with sign of first child
            signs[c] = (this->CellSigns[c]->GetTuple1(childId) != 0.0);
          } // if ( ! child )
          else
          {
            // For subsequent children compare their sign with stored value
            if (signs[c] != (this->CellSigns[c]->GetTuple1(childId) != 0.0))
            {
              // A change of sign occurred, therefore cell must selected
              selected = true;
            }
          } // else
        }   // c
      }     // if( ! selected )

      cursor->ToParent();
    } // child
  }
  else if (!this->InGhostArray || !this->InGhostArray->GetTuple1(id))
  {
    // Cursor is at leaf, retrieve its active scalar value
    double val = this->InScalars->GetTuple1(id);

    // Iterate over all contours
    double* values = this->ContourValues->GetValues();
    for (int c = 0; c < numContours; ++c)
    {
      this->Signs[c] = val > values[c];
    }
  } // else

  // Update list of selected cells
  this->SelectedCells->SetTuple1(id, selected);

  // Set signs for all contours
  for (int c = 0; c < numContours; ++c)
  {
    // Parent cell has that of one of its children
    this->CellSigns[c]->SetTuple1(id, this->Signs[c]);
  }

  // Return whether current node was fully selected
  return selected;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridContour::RecursivelyProcessTree(
  vtkHyperTreeGridNonOrientedMooreSuperCursor* supercursor, vtkCellArray* newVerts,
  vtkCellArray* newLines, vtkCellArray* newPolys, vtkPointData* inPd)
{
  // Retrieve global index of input cursor
  vtkIdType id = supercursor->GetGlobalNodeIndex();

  if (this->InGhostArray && this->InGhostArray->GetTuple1(id))
  {
    return;
  }
  // Retrieve dimensionality
  unsigned int dim = supercursor->GetDimension();

  // Descend further into input trees only if cursor is not a leaf
  if (!supercursor->IsLeaf())
  {
    // Selected cells are determined in RecursivelyPreProcessTree
    bool selected = (this->SelectedCells->GetTuple1(id) == 1.0);

    // Iterate over contours
    for (vtkIdType c = 0; c < this->ContourValues->GetNumberOfContours() && !selected; ++c)
    {
      // Retrieve sign with respect to contour value at current cursor
      bool sign = (this->CellSigns[c]->GetTuple1(id) != 0.0);

      // Iterate over all cursors of Moore neighborhood around center
      unsigned int nn = supercursor->GetNumberOfCursors() - 1;
      for (unsigned int neighbor = 0; neighbor < nn && !selected; ++neighbor)
      {
        // Retrieve global index of neighbor
        unsigned int icursorN = MooreCursors[dim - 1][neighbor];
        if (supercursor->HasTree(icursorN))
        {
          vtkIdType idN = supercursor->GetGlobalNodeIndex(icursorN);

          // Decide whether neighbor was selected or must be retained because of a sign change
          selected = this->SelectedCells->GetTuple1(idN) == 1 ||
            ((this->CellSigns[c]->GetTuple1(idN) != 0.0) != sign) ||
            (this->InGhostArray && this->InGhostArray->GetTuple1(idN));
        }
        else
        {
          selected = false;
        }
      } // neighbor
    }   // c
    if (selected && !supercursor->IsMasked())
    {
      // Node has at least one neighbor containing one contour, recurse to all children
      unsigned int numChildren = supercursor->GetNumberOfChildren();
      for (unsigned int child = 0; child < numChildren; ++child)
      {
        // Create child cursor from parent in input grid
        supercursor->ToChild(child);
        // Recurse
        this->RecursivelyProcessTree(supercursor, newVerts, newLines, newPolys, inPd);
        supercursor->ToParent();
      }
    }
  }
  else if ((!this->InMask || !this->InMask->GetTuple1(id)))
  {
    // Cell is not masked, iterate over its corners
    unsigned int numLeavesCorners = 1 << dim;
    for (unsigned int cornerIdx = 0; cornerIdx < numLeavesCorners; ++cornerIdx)
    {
      bool owner = true;
      this->Leaves->SetNumberOfIds(numLeavesCorners);

      // Iterate over every leaf touching the corner and check ownership
      for (unsigned int leafIdx = 0; leafIdx < numLeavesCorners && owner; ++leafIdx)
      {
        owner = supercursor->GetCornerCursors(cornerIdx, leafIdx, this->Leaves);
      } // leafIdx

      // If cell owns dual cell, compute contours thereof
      if (owner)
      {
        vtkIdType numContours = this->ContourValues->GetNumberOfContours();
        double* values = this->ContourValues->GetValues();

        /* Generate contour topology depending on dimensionality
         * XXX: please note that the generated dual vtkPixel / vtkVoxel do not meet the criteria
         * defined in their respective classes (orthogonal quadrilaterals / parallelepipeds) and
         * seems only used here for convenience (reasons needs to be determined explicitly).
         */
        vtkCell* cell = nullptr;
        switch (dim)
        {
          case 1:
            cell = this->Line;
            break;
          case 2:
            cell = this->Pixel;
            break;
          case 3:
            cell = this->Voxel;
            break;
          default:
            vtkErrorMacro("Unsupported cell dimension had been encountered (must be 1, 2 or 3).");
            return;
        } // switch ( dim )

        // Iterate over cell corners
        double x[3];
        supercursor->GetPoint(x);

        for (unsigned int _cornerIdx = 0; _cornerIdx < numLeavesCorners; ++_cornerIdx)
        {
          // Get cursor corresponding to this corner
          vtkIdType cursorId = this->Leaves->GetId(_cornerIdx);

          // Retrieve neighbor coordinates and store them
          supercursor->GetPoint(cursorId, x);
          cell->Points->SetPoint(_cornerIdx, x);

          // Retrieve neighbor index and add to list of cell vertices
          vtkIdType idN = supercursor->GetGlobalNodeIndex(cursorId);
          cell->PointIds->SetId(_cornerIdx, idN);

          // Assign scalar value attached to this contour item
          this->CellScalars->InsertTuple(_cornerIdx, this->InScalars->GetTuple(idN));
        } // cornerIdx

        /* If we are in 3D and the contour strategy is set to USE_DECOMPOSED_POLYHEDRA,
         * convert each voxel to polyhedron, decompose them and apply the contour on
         * resulting tetrahedrons to give better results in the concave case.
         * XXX: Here we assume that voxels are valid when converting them to polyhedrons.
         * Highly degenerated voxels (faces having duplicated points) will lead to degenerated
         * polyhedrons. However the computation of the contour after the decomposition seems to
         * be insensitive to this issue for now (edge cases are still possible and should be
         * reported if encountered).
         */
        if (this->Strategy3D == USE_DECOMPOSED_POLYHEDRA && dim == 3 && !::AreAllFacesPlanar(cell))
        {
          // Insert points and global point IDs
          for (int i = 0; i < ::POLY_POINTS_NB; ++i)
          {
            this->Internals->Polyhedron->GetPointIds()->SetId(i, cell->GetPointId(i));
            this->Internals->Polyhedron->GetPoints()->SetPoint(i, cell->GetPoints()->GetPoint(i));
          }

          // Construct faces from voxel point ids (global ids)
          this->Internals->Faces->Reset();
          for (int faceId = 0, canonicalId = 0; faceId < ::POLY_FACES_NB; faceId++)
          {
            this->Internals->Faces->InsertNextCell(::POLY_FACES_POINTS_NB);
            for (int i = 0; i < ::POLY_FACES_POINTS_NB; i++, canonicalId++)
            {
              this->Internals->Faces->InsertCellPoint(
                cell->GetPointId(::CANONICAL_FACES[canonicalId]));
            }
          }

          this->Internals->Polyhedron->SetCellFaces(this->Internals->Faces);
          this->Internals->Polyhedron->Initialize();

          // Decompose the this->Internals->Polyhedron
          auto resultUG = vtkPolyhedronUtilities::Decompose(
            this->Internals->Polyhedron, inPd, this->CurrentId, nullptr);

          auto outPointData = vtkPointData::SafeDownCast(this->OutData);
          if (!outPointData)
          {
            vtkErrorMacro("Unable to retrieve the output point data.");
            return;
          }

          /* Estimated size: estimated number of generated triangles (before merging them).
           * Only used in that case. Unused here because we choose to output triangles.
           */
          constexpr int estimatedSize = 0;

          /* Instantiate a new contour helper
           * Needed because we have to change the input point data (now indexed on resultUG point
           * ids)
           */
          vtkContourHelper helper(this->Locator, newVerts, newLines, newPolys,
            resultUG->GetPointData(), nullptr, outPointData, nullptr, estimatedSize, true);

          // Retrieve the contouring array in the resultUG
          auto contourScalars = resultUG->GetPointData()->GetArray(this->InScalars->GetName());
          if (!contourScalars)
          {
            vtkErrorMacro(
              "Unable to find the scalars used for contouring in decomposed dual cell.");
            return;
          }

          // Compute polyhedron isocontour for each isovalue
          for (int c = 0; c < numContours; ++c)
          {
            // Iterate on each tetrahedron of resultUG
            vtkSmartPointer<vtkCellIterator> iter;
            iter.TakeReference(resultUG->NewCellIterator());
            for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextCell())
            {
              iter->GetCell(this->Internals->Tetra);

              // Scalars used for contouring need to be indexed on tetrahedron local ids
              this->Internals->TetraScalars->Reset();
              this->Internals->TetraScalars->SetNumberOfComponents(
                contourScalars->GetNumberOfComponents());
              this->Internals->TetraScalars->SetNumberOfTuples(iter->GetNumberOfPoints());
              contourScalars->GetTuples(iter->GetPointIds(), this->Internals->TetraScalars);

              vtkIdType cellId = iter->GetCellId();
              helper.Contour(
                this->Internals->Tetra, values[c], this->Internals->TetraScalars, cellId);
            }
          }
        }
        else // USE_VOXELS || dim != 3
        {
          // Compute cell isocontour for each isovalue
          for (int c = 0; c < numContours; ++c)
          {
            this->Helper->Contour(cell, values[c], this->CellScalars, this->CurrentId);
          }
        }

        // Increment output cell counter
        ++this->CurrentId;
      } // if ( owner )
    }   // cornerIdx
  }     // else if ( ! this->InMask || this->InMask->GetTuple1( id ) )
}
VTK_ABI_NAMESPACE_END
