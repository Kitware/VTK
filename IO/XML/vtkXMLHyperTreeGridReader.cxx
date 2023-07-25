// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkXMLHyperTreeGridReader.h"

#include "vtkArrayDispatch.h"
#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTypeInt64Array.h"
#include "vtkTypeUInt32Array.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"

#include <algorithm>
#include <limits>
#include <numeric>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLHyperTreeGridReader);

//------------------------------------------------------------------------------
vtkXMLHyperTreeGridReader::vtkXMLHyperTreeGridReader()
  : NumberOfPoints(0)
  , NumberOfPieces(0)
  , FixedLevel(std::numeric_limits<unsigned int>::max())
  , UpdatedPiece(0)
  , UpdateNumberOfPieces(0)
  , StartPiece(0)
  , EndPiece(0)
  , Piece(0)
{
  this->CoordinatesBoundingBox[0] = 1;
  this->CoordinatesBoundingBox[1] = -1;
  this->CoordinatesBoundingBox[2] = 1;
  this->CoordinatesBoundingBox[3] = -1;
  this->CoordinatesBoundingBox[4] = 1;
  this->CoordinatesBoundingBox[5] = -1;

  this->IndicesBoundingBox[0] = 0;
  this->IndicesBoundingBox[1] = std::numeric_limits<unsigned int>::max();
  this->IndicesBoundingBox[2] = 0;
  this->IndicesBoundingBox[3] = std::numeric_limits<unsigned int>::max();
  this->IndicesBoundingBox[4] = 0;
  this->IndicesBoundingBox[5] = std::numeric_limits<unsigned int>::max();
}

//------------------------------------------------------------------------------
vtkXMLHyperTreeGridReader::~vtkXMLHyperTreeGridReader() = default;

//------------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::SetCoordinatesBoundingBox(
  double xmin, double xmax, double ymin, double ymax, double zmin, double zmax)
{
  if (xmin == this->CoordinatesBoundingBox[0] && xmax == this->CoordinatesBoundingBox[1] &&
    ymin == this->CoordinatesBoundingBox[2] && ymax == this->CoordinatesBoundingBox[3] &&
    zmin == this->CoordinatesBoundingBox[4] && zmax == this->CoordinatesBoundingBox[5])
  {
    return;
  }
  this->FixedHTs = false;
  assert("pre: too_late" && !this->FixedHTs);
  this->SelectedHTs = COORDINATES_BOUNDING_BOX;
  this->CoordinatesBoundingBox[0] = xmin;
  this->CoordinatesBoundingBox[1] = xmax;
  this->CoordinatesBoundingBox[2] = ymin;
  this->CoordinatesBoundingBox[3] = ymax;
  this->CoordinatesBoundingBox[4] = zmin;
  this->CoordinatesBoundingBox[5] = zmax;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::SetIndicesBoundingBox(unsigned int imin, unsigned int imax,
  unsigned int jmin, unsigned int jmax, unsigned int kmin, unsigned int kmax)
{
  assert("pre: too_late" && !this->FixedHTs);
  this->SelectedHTs = INDICES_BOUNDING_BOX;
  this->IndicesBoundingBox[0] = imin;
  this->IndicesBoundingBox[1] = imax;
  this->IndicesBoundingBox[2] = jmin;
  this->IndicesBoundingBox[3] = jmax;
  this->IndicesBoundingBox[4] = kmin;
  this->IndicesBoundingBox[5] = kmax;
}

//------------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::ClearAndAddSelectedHT(unsigned int idg, unsigned int fixedLevel)
{
  assert("pre: too_late" && !this->FixedHTs);
  this->SelectedHTs = IDS_SELECTED;
  this->IdsSelected.clear();
  this->IdsSelected[idg] = fixedLevel;
}

//------------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::AddSelectedHT(unsigned int idg, unsigned int fixedLevel)
{
  assert("pre: too_late" && !this->FixedHTs);
  assert("pre: not_clear_and_add_selected " && this->SelectedHTs == IDS_SELECTED);
  this->IdsSelected[idg] = fixedLevel;
}

//------------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::CalculateHTs(const vtkHyperTreeGrid* grid)
{
  assert("pre: already_done" && !this->FixedHTs);
  if (this->SelectedHTs == COORDINATES_BOUNDING_BOX)
  {
    this->SelectedHTs = INDICES_BOUNDING_BOX;
    // if coord_min{x,y,z} < HTG->coord_min{x,y,z} then ht_idx{x,y,z} = 0
    double* htg_bbox = const_cast<vtkHyperTreeGrid*>(grid)->GetBounds();
    this->IndicesBoundingBox[0] = this->CoordinatesBoundingBox[0] <= htg_bbox[0]
      ? 0
      : grid->FindDichotomicX(this->CoordinatesBoundingBox[0]);
    this->IndicesBoundingBox[1] = grid->FindDichotomicX(this->CoordinatesBoundingBox[1]);
    this->IndicesBoundingBox[2] = this->CoordinatesBoundingBox[2] <= htg_bbox[2]
      ? 0
      : grid->FindDichotomicY(this->CoordinatesBoundingBox[2]);
    this->IndicesBoundingBox[3] = grid->FindDichotomicY(this->CoordinatesBoundingBox[3]);
    this->IndicesBoundingBox[4] = this->CoordinatesBoundingBox[4] <= htg_bbox[4]
      ? 0
      : grid->FindDichotomicZ(this->CoordinatesBoundingBox[4]);
    this->IndicesBoundingBox[5] = grid->FindDichotomicZ(this->CoordinatesBoundingBox[5]);
  }
  this->FixedHTs = true;
}

//------------------------------------------------------------------------------
bool vtkXMLHyperTreeGridReader::IsSelectedHT(const vtkHyperTreeGrid* grid, vtkIdType treeIndx) const
{
  assert("pre: not_calculateHTs" && this->FixedHTs);
  switch (this->SelectedHTs)
  {
    case vtkXMLHyperTreeGridReader::ALL:
      return true;
    case vtkXMLHyperTreeGridReader::INDICES_BOUNDING_BOX:
      unsigned int i, j, k;
      grid->GetLevelZeroCoordinatesFromIndex(treeIndx, i, j, k);
      return this->IndicesBoundingBox[0] <= i && i <= this->IndicesBoundingBox[1] &&
        this->IndicesBoundingBox[2] <= j && j <= this->IndicesBoundingBox[3] &&
        this->IndicesBoundingBox[4] <= k && k <= this->IndicesBoundingBox[5];
    case vtkXMLHyperTreeGridReader::IDS_SELECTED:
      if (this->Verbose)
      {
        std::cerr << "treeIndx:" << treeIndx << " "
                  << (this->IdsSelected.find(treeIndx) != this->IdsSelected.end()) << std::endl;
      }
      return this->IdsSelected.find(treeIndx) != this->IdsSelected.end();
    case vtkXMLHyperTreeGridReader::COORDINATES_BOUNDING_BOX:
      // Replace by INDICES_BOUNDING_BOX in CalculateHTs
      assert(this->SelectedHTs == COORDINATES_BOUNDING_BOX);
      break;
    default:
      assert("pre: error_value" && true);
      break;
  }
  return false;
}

//------------------------------------------------------------------------------
unsigned int vtkXMLHyperTreeGridReader::GetFixedLevelOfThisHT(
  unsigned int numberOfLevels, vtkIdType treeIndx) const
{
  unsigned int fixedLevel = this->FixedLevel;
  if (this->IdsSelected.find(treeIndx) != this->IdsSelected.end())
  {
    unsigned int htFixedLevel = this->IdsSelected.at(treeIndx);
    if (htFixedLevel != std::numeric_limits<unsigned int>::max())
    {
      fixedLevel = htFixedLevel;
    }
  }
  return std::min(numberOfLevels, fixedLevel);
}

//------------------------------------------------------------------------------
vtkHyperTreeGrid* vtkXMLHyperTreeGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//------------------------------------------------------------------------------
vtkHyperTreeGrid* vtkXMLHyperTreeGridReader::GetOutput(int idx)
{
  return vtkHyperTreeGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//------------------------------------------------------------------------------
const char* vtkXMLHyperTreeGridReader::GetDataSetName()
{
  return "HyperTreeGrid";
}

//------------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

//------------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::GetOutputUpdateExtent(int& piece, int& numberOfPieces)
{
  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numberOfPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
}

//------------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::SetupOutputTotals() {}

//------------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::SetupNextPiece() {}

//------------------------------------------------------------------------------
int vtkXMLHyperTreeGridReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid");
  return 1;
}

//------------------------------------------------------------------------------
vtkIdType vtkXMLHyperTreeGridReader::GetNumberOfPoints() const
{
  return this->NumberOfPoints;
}

//------------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::SetupUpdateExtent(int piece, int numberOfPieces)
{
  this->UpdatedPiece = piece;
  this->UpdateNumberOfPieces = numberOfPieces;

  // If more pieces are requested than available, just return empty
  // pieces for the extra ones.
  if (this->UpdateNumberOfPieces > this->NumberOfPieces)
  {
    this->UpdateNumberOfPieces = this->NumberOfPieces;
  }

  // Find the range of pieces to read.
  if (this->UpdatedPiece < this->UpdateNumberOfPieces)
  {
    this->StartPiece = ((this->UpdatedPiece * this->NumberOfPieces) / this->UpdateNumberOfPieces);
    this->EndPiece =
      (((this->UpdatedPiece + 1) * this->NumberOfPieces) / this->UpdateNumberOfPieces);
  }
  else
  {
    this->StartPiece = 0;
    this->EndPiece = 0;
  }

  // Find the total size of the output.
  this->SetupOutputTotals();
}

//------------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::SetupPieces(int numPieces)
{
  if (this->NumberOfPieces)
  {
    this->DestroyPieces();
  }
  this->NumberOfPieces = numPieces;
}

//------------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::DestroyPieces()
{
  this->NumberOfPieces = 0;
}

//------------------------------------------------------------------------------
vtkIdType vtkXMLHyperTreeGridReader::GetNumberOfPieces() const
{
  return this->NumberOfPieces;
}

//------------------------------------------------------------------------------
// Note that any changes (add or removing information) made to this method
// should be replicated in CopyOutputInformation
void vtkXMLHyperTreeGridReader::SetupOutputInformation(vtkInformation* outInfo)
{
  this->Superclass::SetupOutputInformation(outInfo);

  if (this->NumberOfPieces > 1)
  {
    outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  }
}

//------------------------------------------------------------------------------
int vtkXMLHyperTreeGridReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{
  if (!this->Superclass::ReadPrimaryElement(ePrimary))
  {
    return 0;
  }

  // Minimum for parallel reader is to know the number of points over all pieces
  if (!ePrimary->GetScalarAttribute("NumberOfVertices", this->NumberOfPoints))
  {
    this->NumberOfPoints = 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::CopyOutputInformation(vtkInformation* outInfo, int port)
{
  this->Superclass::CopyOutputInformation(outInfo, port);
}

//------------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();
}

//------------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::ReadXMLData()
{
  // Initializes the output structure
  this->Superclass::ReadXMLData();

  vtkXMLDataElement* ePrimary =
    this->XMLParser->GetRootElement()->LookupElementWithName("HyperTreeGrid");

  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(this->GetCurrentOutput());
  int branchFactor;
  int transposedRootIndexing;
  int dimensions[3];

  // Read the attributes of the hyper tree grid
  // Whether or not there is a file description in the XML file,
  // the Dimension and Orientation scalar attributes are no longer exploited.
  if (!ePrimary->GetScalarAttribute("BranchFactor", branchFactor))
  {
    branchFactor = 2;
  }
  if (!ePrimary->GetScalarAttribute("TransposedRootIndexing", transposedRootIndexing))
  {
    transposedRootIndexing = 0;
  }
  if (ePrimary->GetVectorAttribute("Dimensions", 3, dimensions) != 3)
  {
    dimensions[0] = 1;
    dimensions[1] = 1;
    dimensions[2] = 1;
  }
  const char* normalsName = ePrimary->GetAttribute("InterfaceNormalsName");
  if (normalsName)
  {
    output->SetInterfaceNormalsName(normalsName);
  }
  const char* interceptsName = ePrimary->GetAttribute("InterfaceInterceptsName");
  if (interceptsName)
  {
    output->SetInterfaceInterceptsName(interceptsName);
  }
  if (normalsName && interceptsName)
  {
    output->SetHasInterface(true);
  }
  if (!ePrimary->GetScalarAttribute("NumberOfVertices", this->NumberOfPoints))
  {
    this->NumberOfPoints = 0;
  }

  // Define the hypertree grid
  output->SetBranchFactor(branchFactor);
  output->SetTransposedRootIndexing((transposedRootIndexing != 0));
  output->SetDimensions(dimensions);

  // Read geometry of hypertree grid expressed in coordinates
  vtkXMLDataElement* eNested = ePrimary->LookupElementWithName("Grid");
  if (eNested)
  {
    this->ReadGrid(eNested);
  }

  // The output is defined, fixed selected HTs
  this->CalculateHTs(output);

  // Read the topology and data of each hypertree
  eNested = ePrimary->LookupElementWithName("Trees");
  if (eNested)
  {
    if (this->GetFileMajorVersion() == 0)
    {
      this->ReadTrees_0(eNested);
    }
    else if (this->GetFileMajorVersion() == 1)
    {
      this->ReadTrees_1(eNested);
    }
    else if (this->GetFileMajorVersion() == 2)
    {
      this->ReadTrees_2(ePrimary);
    }
  }
  this->IdsSelected.clear();
  this->FixedHTs = false;
}

//------------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::ReadGrid(vtkXMLDataElement* elem)
{
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(this->GetCurrentOutput());

  // Read the coordinates arrays
  vtkXMLDataElement* xc =
    elem->FindNestedElementWithNameAndAttribute("DataArray", "Name", "XCoordinates");
  vtkXMLDataElement* yc =
    elem->FindNestedElementWithNameAndAttribute("DataArray", "Name", "YCoordinates");
  vtkXMLDataElement* zc =
    elem->FindNestedElementWithNameAndAttribute("DataArray", "Name", "ZCoordinates");

  vtkAbstractArray* xa = this->CreateArray(xc);
  vtkAbstractArray* ya = this->CreateArray(yc);
  vtkAbstractArray* za = this->CreateArray(zc);

  auto x = vtkArrayDownCast<vtkDataArray>(xa);
  auto y = vtkArrayDownCast<vtkDataArray>(ya);
  auto z = vtkArrayDownCast<vtkDataArray>(za);

  vtkIdType numX, numY, numZ;
  xc->GetScalarAttribute("NumberOfTuples", numX);
  yc->GetScalarAttribute("NumberOfTuples", numY);
  zc->GetScalarAttribute("NumberOfTuples", numZ);

  if (x && y && z)
  {
    x->SetNumberOfTuples(numX);
    y->SetNumberOfTuples(numY);
    z->SetNumberOfTuples(numZ);

    this->ReadArrayValues(xc, 0, x, 0, numX);
    this->ReadArrayValues(yc, 0, y, 0, numY);
    this->ReadArrayValues(zc, 0, z, 0, numZ);

    output->SetXCoordinates(x);
    output->SetYCoordinates(y);
    output->SetZCoordinates(z);

    x->Delete();
    y->Delete();
    z->Delete();
  }
  else
  {
    if (xa)
    {
      xa->Delete();
    }
    if (ya)
    {
      ya->Delete();
    }
    if (za)
    {
      za->Delete();
    }
    this->DataError = 1;
  }
}

//------------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::ReadTrees_0(vtkXMLDataElement* elem)
{
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(this->GetCurrentOutput());
  vtkNew<vtkHyperTreeGridNonOrientedCursor> treeCursor;

  // Number of trees in this hypertree grid file
  int numberOfTrees = elem->GetNumberOfNestedElements();

  // Hypertree grid mask collected while processing hypertrees
  vtkNew<vtkBitArray> htgMask;
  htgMask->SetNumberOfTuples(this->NumberOfPoints);
  bool hasMaskData = false;

  for (int treeIndx = 0; treeIndx < numberOfTrees; treeIndx++)
  {
    // Nested elements within Trees is Tree
    vtkXMLDataElement* eTree = elem->GetNestedElement(treeIndx);
    vtkIdType treeId;
    vtkIdType globalOffset;
    vtkIdType numberOfVertices;
    eTree->GetScalarAttribute("Index", treeId);
    eTree->GetScalarAttribute("GlobalOffset", globalOffset);
    eTree->GetScalarAttribute("NumberOfVertices", numberOfVertices);

    // Descriptor for hypertree
    vtkXMLDataElement* desc_e =
      eTree->FindNestedElementWithNameAndAttribute("DataArray", "Name", "Descriptor");
    vtkAbstractArray* desc_a = this->CreateArray(desc_e);
    auto desc_d = vtkArrayDownCast<vtkDataArray>(desc_a);
    if (!desc_d)
    {
      if (desc_a)
      {
        desc_a->Delete();
      }
      return;
    }
    vtkIdType numberOfNodes = 0;
    if (!desc_e->GetScalarAttribute("NumberOfTuples", numberOfNodes))
    {
      desc_d->Delete();
      return;
    }
    desc_d->SetNumberOfTuples(numberOfNodes);
    if (!this->ReadArrayValues(desc_e, 0, desc_d, 0, numberOfNodes))
    {
      desc_d->Delete();
      return;
    }
    auto desc = vtkArrayDownCast<vtkBitArray>(desc_d);
    if (!desc)
    {
      vtkErrorMacro(
        "Cannot convert vtkDataArray of type " << desc_d->GetDataType() << " to vtkBitArray.");
      desc_d->Delete();
      return;
    }

    // Parse descriptor storing the global index per level of hypertree
    vtkNew<vtkIdTypeArray> posByLevel;
    output->InitializeNonOrientedCursor(treeCursor, treeId, true);
    treeCursor->SetGlobalIndexStart(globalOffset);

    // Level 0 contains root of hypertree
    posByLevel->InsertNextValue(0);
    vtkIdType nRefined = 0;
    vtkIdType nCurrentLevel = 0;
    vtkIdType nNextLevel = 1;
    vtkIdType descSize = desc->GetNumberOfTuples();
    unsigned int numberOfChildren = output->GetNumberOfChildren();

    // Determine position of the start of each level within descriptor
    for (vtkIdType i = 0; i < descSize; ++i)
    {
      if (nCurrentLevel >= nNextLevel)
      {
        // reached the next level of data in the breadth first descriptor array
        nNextLevel = nRefined * numberOfChildren;
        nRefined = 0;
        nCurrentLevel = 0;
        posByLevel->InsertNextValue(i);
      }
      if (desc->GetValue(i) == 1)
      {
        nRefined++;
      }
      nCurrentLevel++;
    }

    // Recursively subdivide tree
    this->SubdivideFromDescriptor_0(treeCursor, 0, numberOfChildren, desc, posByLevel);

    // Mask is stored in XML element
    vtkXMLDataElement* mask_e =
      eTree->FindNestedElementWithNameAndAttribute("DataArray", "Name", "Mask");
    if (mask_e)
    {
      vtkAbstractArray* mask_a = this->CreateArray(mask_e);
      auto mask_d = vtkArrayDownCast<vtkDataArray>(mask_a);
      numberOfNodes = 0;
      mask_e->GetScalarAttribute("NumberOfTuples", numberOfNodes);
      mask_d->SetNumberOfTuples(numberOfNodes);
      auto mask = vtkArrayDownCast<vtkBitArray>(mask_d);

      if (mask_e)
      {
        this->ReadArrayValues(mask_e, 0, mask_d, 0, numberOfNodes);
      }

      if (numberOfNodes == numberOfVertices)
      {
        for (int i = 0; i < numberOfNodes; i++)
        {
          htgMask->SetValue(globalOffset + i, mask->GetValue(i));
        }
        hasMaskData = true;
      }
      mask_a->Delete();
    }

    // CellData belonging to hypertree immediately follows descriptor
    vtkCellData* pointData = output->GetCellData();
    vtkXMLDataElement* eCellData = eTree->LookupElementWithName("CellData");
    if (!eCellData)
    {
      eCellData = eTree->LookupElementWithName("PointData");
    }
    if (eCellData)
    {
      for (int j = 0; j < eCellData->GetNumberOfNestedElements(); j++)
      {
        vtkXMLDataElement* eNested = eCellData->GetNestedElement(j);
        const char* ename = eNested->GetAttribute("Name");
        vtkAbstractArray* outArray = pointData->GetArray(ename);
        int numberOfComponents = 1;
        const char* eNC = eNested->GetAttribute("NumberOfComponents");
        if (eNC)
        {
          numberOfComponents = atoi(eNC);
        }

        // Create the output CellData array when processing first tree
        if (outArray == nullptr)
        {
          outArray = this->CreateArray(eNested);
          outArray->SetNumberOfComponents(numberOfComponents);
          outArray->SetNumberOfTuples(this->NumberOfPoints);
          pointData->AddArray(outArray);
          outArray->Delete();
        }
        // Read data into the global offset which is
        // number of vertices in the tree * number of components in the data
        this->ReadArrayValues(eNested, globalOffset * numberOfComponents, outArray, 0,
          numberOfVertices * numberOfComponents, POINT_DATA);
      }
    }
    desc_a->Delete();
  }
  if (hasMaskData)
  {
    output->SetMask(htgMask);
  }
}

//------------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::SubdivideFromDescriptor_0(
  vtkHyperTreeGridNonOrientedCursor* treeCursor, unsigned int level, unsigned int numChildren,
  vtkBitArray* descriptor, vtkIdTypeArray* posByLevel)
{
  vtkIdType curOffset = posByLevel->GetValue(level);
  // Current offset within descriptor is advanced
  // for if/when we get back to this level on next tree
  posByLevel->SetValue(level, curOffset + 1);

  if (descriptor->GetValue(curOffset) == 0)
  {
    return;
  }

  // Subdivide hyper tree grid leaf and traverse to children
  treeCursor->SubdivideLeaf();

  for (unsigned int child = 0; child < numChildren; ++child)
  {
    treeCursor->ToChild(child);
    this->SubdivideFromDescriptor_0(treeCursor, level + 1, numChildren, descriptor, posByLevel);
    treeCursor->ToParent();
  }
}

//------------------------------------------------------------------------------
// Functor used to accumulate in the native array type with dispatch
struct AccImpl
{
  explicit AccImpl(unsigned int limitedLevel)
    : LimitedLevel(limitedLevel)
  {
  }
  // Fixed input
  const unsigned int LimitedLevel{ 0 };
  // Output
  vtkTypeInt64 FixedNbVertices{ 0 };
  vtkTypeInt64 LimitedLevelElement{ 0 };

  template <typename ArrayType>
  void operator()(ArrayType* array)
  {
    auto range = vtk::DataArrayValueRange<1>(array);
    this->FixedNbVertices = std::accumulate(range.begin(), range.begin() + this->LimitedLevel, 0);
    this->LimitedLevelElement = range[this->LimitedLevel - 1];
  }
};

//------------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::ReadTrees_1(vtkXMLDataElement* elem)
{
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(this->GetCurrentOutput());
  vtkNew<vtkHyperTreeGridNonOrientedCursor> treeCursor;

  // Number of trees in this hypertree grid file
  int numberOfTrees = elem->GetNumberOfNestedElements();

  vtkIdType globalOffset = 0;
  for (int treeIndxInFile = 0; treeIndxInFile < numberOfTrees; ++treeIndxInFile)
  {
    // Nested elements within Trees is Tree
    vtkXMLDataElement* eTree = elem->GetNestedElement(treeIndxInFile);
    vtkIdType treeIndxInHTG;
    vtkIdType numberOfVertices;
    int numberOfLevels;
    eTree->GetScalarAttribute("Index", treeIndxInHTG);

    // Functionality not available on older versions
    if (!this->IsSelectedHT(output, treeIndxInHTG))
    {
      continue;
    }

    eTree->GetScalarAttribute("NumberOfLevels", numberOfLevels);
    eTree->GetScalarAttribute("NumberOfVertices", numberOfVertices);

    // Descriptor for hypertree
    vtkXMLDataElement* desc_e =
      eTree->FindNestedElementWithNameAndAttribute("DataArray", "Name", "Descriptor");
    vtkSmartPointer<vtkAbstractArray> desc_a =
      vtkSmartPointer<vtkAbstractArray>::Take(this->CreateArray(desc_e));
    auto desc_d = vtkArrayDownCast<vtkDataArray>(desc_a);
    if (!desc_d)
    {
      return;
    }
    vtkIdType descSize = 0;
    vtkBitArray* desc = nullptr;
    desc_e->GetScalarAttribute("NumberOfTuples", descSize);
    if (descSize)
    {
      desc_d->SetNumberOfTuples(descSize);
      if (!this->ReadArrayValues(desc_e, 0, desc_d, 0, descSize))
      {
        return;
      }
      desc = vtkArrayDownCast<vtkBitArray>(desc_d);
      if (!desc)
      {
        vtkErrorMacro(
          "Cannot convert vtkDataArray of type " << desc_d->GetDataType() << " to vtkBitArray.");
        return;
      }
    }

    // Parse descriptor storing the global index per level of hypertree
    output->InitializeNonOrientedCursor(treeCursor, treeIndxInHTG, true);

    treeCursor->SetGlobalIndexStart(globalOffset);
    vtkHyperTree* tree = treeCursor->GetTree();

    vtkXMLDataElement* maskElement =
      eTree->FindNestedElementWithNameAndAttribute("DataArray", "Name", "Mask");
    vtkSmartPointer<vtkBitArray> maskArray = nullptr;
    if (maskElement)
    {
      maskArray = vtkSmartPointer<vtkBitArray>::Take(
        vtkArrayDownCast<vtkBitArray>(this->CreateArray(maskElement)));
      vtkIdType numberOfNodes = 0;
      maskElement->GetScalarAttribute("NumberOfTuples", numberOfNodes);
      maskArray->SetNumberOfTuples(numberOfNodes);
      this->ReadArrayValues(maskElement, 0, maskArray, 0, numberOfNodes);

      if (!output->GetMask())
      {
        vtkNew<vtkBitArray> mask;
        output->SetMask(mask);
      }
    }

    vtkXMLDataElement* nbVerticesByLevelElement =
      eTree->FindNestedElementWithNameAndAttribute("DataArray", "Name", "NbVerticesByLevel");
    vtkSmartPointer<vtkDataArray> nbVerticesByLevelArray = nullptr;
    if (nbVerticesByLevelElement)
    {
      nbVerticesByLevelArray = vtkSmartPointer<vtkDataArray>::Take(
        vtkArrayDownCast<vtkDataArray>(this->CreateArray(nbVerticesByLevelElement)));
      vtkIdType numberOfNodes = 0;
      nbVerticesByLevelElement->GetScalarAttribute("NumberOfTuples", numberOfNodes);
      nbVerticesByLevelArray->SetNumberOfTuples(numberOfNodes);
      this->ReadArrayValues(nbVerticesByLevelElement, 0, nbVerticesByLevelArray, 0, numberOfNodes);
    }
    AccImpl accFunctor(this->GetFixedLevelOfThisHT(numberOfLevels, treeIndxInHTG));
    if (!vtkArrayDispatch::Dispatch::Execute(nbVerticesByLevelArray, accFunctor))
    {
      cerr << "Should not happen: could not dispatch nbVerticesByLevel array" << endl;
      cerr << "Falling back to vtkDataArray, can pose problems on windows" << endl;
      accFunctor(nbVerticesByLevelArray.GetPointer());
    }
    tree->InitializeForReader(accFunctor.LimitedLevel, accFunctor.FixedNbVertices,
      accFunctor.LimitedLevelElement, desc, maskArray, output->GetMask());

    // CellData belonging to hypertree immediately follows descriptor
    vtkCellData* pointData = output->GetCellData();
    vtkXMLDataElement* eCellData = eTree->LookupElementWithName("CellData");
    // Legacy support : cell data used to be point data.
    if (!eCellData)
    {
      eCellData = eTree->LookupElementWithName("PointData");
    }
    if (eCellData)
    {
      for (int j = 0; j < eCellData->GetNumberOfNestedElements(); ++j)
      {
        vtkXMLDataElement* eNested = eCellData->GetNestedElement(j);
        const char* ename = eNested->GetAttribute("Name");
        vtkAbstractArray* outArray = pointData->GetArray(ename);
        int numberOfComponents = 1;
        const char* eNC = eNested->GetAttribute("NumberOfComponents");
        if (eNC)
        {
          numberOfComponents = atoi(eNC);
        }

        // Create the output CellData array when processing first tree
        if (outArray == nullptr)
        {
          outArray = this->CreateArray(eNested);
          outArray->SetNumberOfComponents(numberOfComponents);
          outArray->SetNumberOfTuples(0);
          pointData->AddArray(outArray);
          pointData->SetActiveScalars(ename);
          outArray->Delete();
        }

        // Doing Resize() is not enough !
        // outArray->Resize(outArray->GetNumberOfTuples()+accFunctor.FixedNbVertices);
        // Tip: insert copy of an existing table data in position 0 to
        // the last position of the same table
        outArray->InsertTuple(
          outArray->GetNumberOfTuples() + accFunctor.FixedNbVertices - 1, 0, outArray);

        // Read data into the global offset which is
        // number of vertices in the tree * number of components in the data
        this->ReadArrayValues(eNested, globalOffset * numberOfComponents, outArray, 0,
          accFunctor.FixedNbVertices * numberOfComponents, POINT_DATA);
      }
    }
    // Calculating the first offset of the next HyperTree
    globalOffset += tree->GetNumberOfVertices();
  }
}

//------------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::ReadTrees_2(vtkXMLDataElement* element)
{
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(this->GetCurrentOutput());
  vtkNew<vtkHyperTreeGridNonOrientedCursor> treeCursor;

  vtkXMLDataElement* treesElement = element->LookupElementWithName("Trees");

  vtkXMLDataElement* treeIdsElement =
    treesElement->FindNestedElementWithNameAndAttribute("DataArray", "Name", "TreeIds");

  if (!treeIdsElement->GetAttribute("NumberOfTuples"))
  {
    // Empty htg
    return;
  }

  auto treeIds = vtkSmartPointer<vtkTypeInt64Array>::Take(
    vtkArrayDownCast<vtkTypeInt64Array>(this->CreateArray(treeIdsElement)));
  vtkIdType treeIdsSize;

  treeIdsElement->GetScalarAttribute("NumberOfTuples", treeIdsSize);
  treeIds->SetNumberOfValues(treeIdsSize);
  if (!this->ReadArrayValues(treeIdsElement, 0, treeIds, 0, treeIdsSize))
  {
    vtkErrorMacro("Failed to parse " << this->GetFileName()
                                     << ". Missing / flawed array TreeIds, in XML element Trees");
    return;
  }

  vtkXMLDataElement* depthPerTreeElement =
    treesElement->FindNestedElementWithNameAndAttribute("DataArray", "Name", "DepthPerTree");
  auto depthPerTree = vtkSmartPointer<vtkTypeUInt32Array>::Take(
    vtkArrayDownCast<vtkTypeUInt32Array>(this->CreateArray(depthPerTreeElement)));
  vtkIdType depthPerTreeSize;
  depthPerTreeElement->GetScalarAttribute("NumberOfTuples", depthPerTreeSize);
  depthPerTree->SetNumberOfValues(depthPerTreeSize);
  if (!this->ReadArrayValues(depthPerTreeElement, 0, depthPerTree, 0, depthPerTreeSize))
  {
    vtkErrorMacro("Failed to parse "
      << this->GetFileName() << ". Missing /flawed array DepthPerTree, in XML element Trees"
      << ", which is required in HyperTreeGrid XML format of version 2. Aborting");
    return;
  }

  vtkXMLDataElement* numberOfVerticesPerDepthElement =
    treesElement->FindNestedElementWithNameAndAttribute(
      "DataArray", "Name", "NumberOfVerticesPerDepth");
  auto numberOfVerticesPerDepth = vtkSmartPointer<vtkTypeInt64Array>::Take(
    vtkArrayDownCast<vtkTypeInt64Array>(this->CreateArray(numberOfVerticesPerDepthElement)));

  vtkIdType numberOfVerticesPerDepthSize;
  numberOfVerticesPerDepthElement->GetScalarAttribute(
    "NumberOfTuples", numberOfVerticesPerDepthSize);
  numberOfVerticesPerDepth->SetNumberOfValues(numberOfVerticesPerDepthSize);
  if (!this->ReadArrayValues(numberOfVerticesPerDepthElement, 0, numberOfVerticesPerDepth, 0,
        numberOfVerticesPerDepthSize))
  {
    vtkErrorMacro("Failed to parse "
      << this->GetFileName()
      << ". Missing / flawed array NumberOfVerticesPerDepth, in XML element Trees"
      << ", which is required in HyperTreeGrid XML format of version 2. Aborting");
    return;
  }

  vtkXMLDataElement* descriptorsElement =
    treesElement->FindNestedElementWithNameAndAttribute("DataArray", "Name", "Descriptors");

  vtkIdType totalNumberOfVertices = 0;
  auto numberOfVerticesPerDepthRange = vtk::DataArrayValueRange<1>(numberOfVerticesPerDepth);
  auto numberOfVerticesPerDepthIterator = numberOfVerticesPerDepthRange.cbegin();

  // Computing the total size of scalar fields / mask
  for (vtkIdType treeId = 0; treeId < treeIdsSize; ++treeId)
  {
    unsigned int depth = 0;
    for (; depth < std::min<unsigned int>(depthPerTree->GetValue(treeId), this->FixedLevel);
         ++depth, ++numberOfVerticesPerDepthIterator)
    {
      totalNumberOfVertices += *numberOfVerticesPerDepthIterator;
    }
    while (depth < depthPerTree->GetValue(treeId) && ++numberOfVerticesPerDepthIterator)
    {
      ++depth;
    }
  }

  vtkXMLDataElement* maskElement =
    treesElement->FindNestedElementWithNameAndAttribute("DataArray", "Name", "Mask");
  vtkSmartPointer<vtkBitArray> mask = nullptr;
  if (maskElement)
  {
    mask = vtkSmartPointer<vtkBitArray>::Take(
      vtkArrayDownCast<vtkBitArray>(this->CreateArray(maskElement)));
    mask->SetNumberOfValues(totalNumberOfVertices);
    output->SetMask(mask);
  }

  vtkCellData* cellData = output->GetCellData();
  vtkXMLDataElement* cellDataElement = element->FindNestedElementWithName("CellData");
  std::vector<vtkXMLDataElement*> arrayElements;
  std::vector<vtkSmartPointer<vtkAbstractArray>> arrays;
  if (cellDataElement)
  {
    for (vtkIdType id = 0; id < cellDataElement->GetNumberOfNestedElements(); ++id)
    {
      arrayElements.emplace_back(cellDataElement->GetNestedElement(id));
      int numberOfComponents = 1;
      const char* numberOfComponentsChar = arrayElements.back()->GetAttribute("NumberOfComponents");
      if (numberOfComponentsChar)
      {
        numberOfComponents = atoi(numberOfComponentsChar);
      }

      arrays.emplace_back(
        vtkSmartPointer<vtkAbstractArray>::Take(this->CreateArray(arrayElements.back())));
      arrays.back()->SetNumberOfComponents(numberOfComponents);
      arrays.back()->SetNumberOfTuples(totalNumberOfVertices);
      cellData->AddArray(arrays.back());
    }
  }

  vtkIdType descriptorOffset = 0;
  vtkIdType inputOffset = 0;
  vtkIdType outputOffset = 0;
  numberOfVerticesPerDepthIterator = numberOfVerticesPerDepthRange.cbegin();

  for (vtkIdType treeId = 0; treeId < treeIdsSize; ++treeId)
  {
    vtkIdType treeSize = 0;
    vtkIdType readableTreeSize = 0;
    vtkIdType lastDepthSize = 0;
    vtkIdType lastReadableDepthSize = 0;
    for (unsigned int depth = 0; depth < depthPerTree->GetValue(treeId);
         ++depth, ++numberOfVerticesPerDepthIterator)
    {
      lastDepthSize = *numberOfVerticesPerDepthIterator;
      if (depth < this->FixedLevel)
      {
        lastReadableDepthSize = lastDepthSize;
        readableTreeSize += lastDepthSize;
      }
      treeSize += lastDepthSize;
    }

    vtkIdType descriptorSize = treeSize - lastDepthSize;
    vtkIdType readableDescriptorSize = readableTreeSize - lastReadableDepthSize;
    auto descriptor = vtkSmartPointer<vtkBitArray>::Take(
      vtkArrayDownCast<vtkBitArray>(this->CreateArray(descriptorsElement)));
    descriptor->SetNumberOfValues(readableDescriptorSize);
    if (!descriptor)
    {
      vtkErrorMacro(<< "Missing Descriptor. Aborting");
      return;
    }
    if (readableDescriptorSize &&
      !this->ReadArrayValues(
        descriptorsElement, 0, descriptor, descriptorOffset, readableDescriptorSize))
    {
      vtkErrorMacro(<< "Failed reading descriptor at tree " << treeIds->GetValue(treeId)
                    << ". Aborting.");
      return;
    }

    // Parse descriptor storing the global index per level of hypertree
    output->InitializeNonOrientedCursor(treeCursor, treeIds->GetValue(treeId), true);

    treeCursor->SetGlobalIndexStart(outputOffset);
    vtkHyperTree* tree = treeCursor->GetTree();

    if (maskElement)
    {
      if (!this->ReadArrayValues(maskElement, outputOffset, mask, inputOffset, readableTreeSize))
      {
        vtkErrorMacro(<< "Failed reading mask at tree " << treeIds->GetValue(treeId)
                      << ". Aborting.");
        return;
      }
    }

    for (std::size_t arrayId = 0; arrayId < arrays.size(); ++arrayId)
    {
      if (!this->ReadArrayTuples(
            arrayElements[arrayId], outputOffset, arrays[arrayId], inputOffset, readableTreeSize))
      {
        vtkErrorMacro(<< "Failed reading array " << arrayId << ". Aborting.");
        return;
      }
    }

    tree->BuildFromBreadthFirstOrderDescriptor(descriptor, readableDescriptorSize, 0);
    descriptorOffset += descriptorSize;
    outputOffset += readableTreeSize;
    inputOffset += treeSize;
  }
}
VTK_ABI_NAMESPACE_END
