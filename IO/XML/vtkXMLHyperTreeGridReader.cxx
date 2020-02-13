/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLHyperTreeGridReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkXMLHyperTreeGridReader.h"

#include "vtkBitArray.h"
#include "vtkDataArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedLongArray.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"

#include <algorithm>

vtkStandardNewMacro(vtkXMLHyperTreeGridReader);

//----------------------------------------------------------------------------
vtkXMLHyperTreeGridReader::vtkXMLHyperTreeGridReader() = default;

//----------------------------------------------------------------------------
vtkXMLHyperTreeGridReader::~vtkXMLHyperTreeGridReader() = default;

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::SetCoordinatesBoundingBox(
  double xmin, double xmax, double ymin, double ymax, double zmin, double zmax)
{
  assert("pre: too_late" && !this->FixedHTs);
  this->SelectedHTs = COORDINATES_BOUNDING_BOX;
  this->CoordinatesBoundingBox[0] = xmin;
  this->CoordinatesBoundingBox[1] = xmax;
  this->CoordinatesBoundingBox[2] = ymin;
  this->CoordinatesBoundingBox[3] = ymax;
  this->CoordinatesBoundingBox[4] = zmin;
  this->CoordinatesBoundingBox[5] = zmax;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::ClearAndAddSelectedHT(unsigned int idg, unsigned int fixedLevel)
{
  assert("pre: too_late" && !this->FixedHTs);
  this->SelectedHTs = IDS_SELECTED;
  this->IdsSelected.clear();
  this->IdsSelected[idg] = fixedLevel;
}

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::AddSelectedHT(unsigned int idg, unsigned int fixedLevel)
{
  assert("pre: too_late" && !this->FixedHTs);
  assert("pre: not_clear_and_add_selected " && this->SelectedHTs == IDS_SELECTED);
  this->IdsSelected[idg] = fixedLevel;
}

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::CalculateHTs(const vtkHyperTreeGrid* grid)
{
  assert("pre: already_done" && !this->FixedHTs);
  if (this->SelectedHTs == COORDINATES_BOUNDING_BOX)
  {
    this->SelectedHTs = INDICES_BOUNDING_BOX;
    this->IndicesBoundingBox[0] = grid->FindDichotomicX(this->CoordinatesBoundingBox[0]);
    this->IndicesBoundingBox[1] = grid->FindDichotomicX(this->CoordinatesBoundingBox[1]);
    this->IndicesBoundingBox[2] = grid->FindDichotomicY(this->CoordinatesBoundingBox[2]);
    this->IndicesBoundingBox[3] = grid->FindDichotomicY(this->CoordinatesBoundingBox[3]);
    this->IndicesBoundingBox[4] = grid->FindDichotomicZ(this->CoordinatesBoundingBox[4]);
    this->IndicesBoundingBox[5] = grid->FindDichotomicZ(this->CoordinatesBoundingBox[5]);
  }
  this->FixedHTs = true;
}

//----------------------------------------------------------------------------
bool vtkXMLHyperTreeGridReader::IsSelectedHT(
  const vtkHyperTreeGrid* grid, unsigned int treeIndx) const
{
  assert("pre: not_calculateHTs" && this->FixedHTs);
  switch (this->SelectedHTs)
  {
    case vtkXMLHyperTreeGridReader::ALL:
      return true;
    case vtkXMLHyperTreeGridReader::INDICES_BOUNDING_BOX:
      unsigned int i, j, k;
      grid->GetLevelZeroCoordinatesFromIndex(treeIndx, i, j, k);
      return this->IndicesBoundingBox[0] >= i && i <= this->IndicesBoundingBox[1] &&
        this->IndicesBoundingBox[2] >= j && j <= this->IndicesBoundingBox[3] &&
        this->IndicesBoundingBox[4] >= k && k <= this->IndicesBoundingBox[5];
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

//----------------------------------------------------------------------------
vtkIdType vtkXMLHyperTreeGridReader::GetFixedLevelOfThisHT(
  vtkIdType numberOfLevels, unsigned int treeIndx) const
{
  unsigned int fixedLevel = this->FixedLevel;
  if (this->IdsSelected.find(treeIndx) != this->IdsSelected.end())
  {
    unsigned int htFixedLevel = this->IdsSelected.at(treeIndx);
    if (htFixedLevel != UINT_MAX)
    {
      fixedLevel = htFixedLevel;
    }
  }
  return std::min(numberOfLevels, vtkIdType(fixedLevel));
}

//----------------------------------------------------------------------------
vtkHyperTreeGrid* vtkXMLHyperTreeGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkHyperTreeGrid* vtkXMLHyperTreeGridReader::GetOutput(int idx)
{
  return vtkHyperTreeGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
const char* vtkXMLHyperTreeGridReader::GetDataSetName()
{
  return "HyperTreeGrid";
}

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::GetOutputUpdateExtent(int& piece, int& numberOfPieces)
{
  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numberOfPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
}

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::SetupOutputTotals() {}

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::SetupNextPiece() {}

//----------------------------------------------------------------------------
int vtkXMLHyperTreeGridReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLHyperTreeGridReader::GetNumberOfPoints()
{
  return this->NumberOfPoints;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::SetupPieces(int numPieces)
{
  if (this->NumberOfPieces)
  {
    this->DestroyPieces();
  }
  this->NumberOfPieces = numPieces;
}

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::DestroyPieces()
{
  this->NumberOfPieces = 0;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLHyperTreeGridReader::GetNumberOfPieces()
{
  return this->NumberOfPieces;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::CopyOutputInformation(vtkInformation* outInfo, int port)
{
  this->Superclass::CopyOutputInformation(outInfo, port);
}

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();
}

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::ReadXMLData()
{
  // Initializes the output structure
  this->Superclass::ReadXMLData();

  vtkXMLDataElement* ePrimary = this->XMLParser->GetRootElement()->GetNestedElement(0);

  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(this->GetCurrentOutput());
  int branchFactor;
  int transposedRootIndexing;
  int dimensions[3];
  const char* name;

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
  if ((name = ePrimary->GetAttribute("InterfaceNormalsName")))
  {
    output->SetInterfaceNormalsName(name);
  }
  if ((name = ePrimary->GetAttribute("InterfaceInterceptsName")))
  {
    output->SetInterfaceInterceptsName(name);
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
  vtkXMLDataElement* eNested = ePrimary->GetNestedElement(0);
  if (strcmp(eNested->GetName(), "Grid") == 0)
  {
    this->ReadGrid(eNested);
  }

  // The output is defined, fixed selected HTs
  this->CalculateHTs(output);

  // Read the topology and data of each hypertree
  eNested = ePrimary->GetNestedElement(1);
  if (strcmp(eNested->GetName(), "Trees") == 0)
  {
    if (this->GetFileMajorVersion() < 1)
    {
      this->ReadTrees_0(eNested);
    }
    else
    {
      this->ReadTrees_1(eNested);
    }
  }
}

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::ReadGrid(vtkXMLDataElement* elem)
{
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(this->GetCurrentOutput());

  // Read the coordinates arrays
  vtkXMLDataElement* xc = elem->GetNestedElement(0);
  vtkXMLDataElement* yc = elem->GetNestedElement(1);
  vtkXMLDataElement* zc = elem->GetNestedElement(2);

  vtkAbstractArray* xa = this->CreateArray(xc);
  vtkAbstractArray* ya = this->CreateArray(yc);
  vtkAbstractArray* za = this->CreateArray(zc);

  vtkDataArray* x = vtkArrayDownCast<vtkDataArray>(xa);
  vtkDataArray* y = vtkArrayDownCast<vtkDataArray>(ya);
  vtkDataArray* z = vtkArrayDownCast<vtkDataArray>(za);

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

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::ReadTrees_0(vtkXMLDataElement* elem)
{
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(this->GetCurrentOutput());
  vtkNew<vtkHyperTreeGridNonOrientedCursor> treeCursor;

  // Number of trees in this hypertree grid file
  int numberOfTrees = elem->GetNumberOfNestedElements();
  elem->GetScalarAttribute("NumberOfTrees", numberOfTrees);

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
    vtkXMLDataElement* desc_e = eTree->GetNestedElement(0);
    vtkAbstractArray* desc_a = this->CreateArray(desc_e);
    vtkDataArray* desc_d = vtkArrayDownCast<vtkDataArray>(desc_a);
    if (!desc_d)
    {
      if (desc_a)
      {
        desc_a->Delete();
      }
      return;
    }
    int numberOfNodes = 0;
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
    vtkBitArray* desc = vtkArrayDownCast<vtkBitArray>(desc_d);
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
    int numberOfChildren = output->GetNumberOfChildren();

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
    vtkXMLDataElement* mask_e = eTree->GetNestedElement(1);
    vtkAbstractArray* mask_a = this->CreateArray(mask_e);
    vtkDataArray* mask_d = vtkArrayDownCast<vtkDataArray>(mask_a);
    numberOfNodes = 0;
    mask_e->GetScalarAttribute("NumberOfTuples", numberOfNodes);
    mask_d->SetNumberOfTuples(numberOfNodes);
    vtkBitArray* mask = vtkArrayDownCast<vtkBitArray>(mask_d);

    this->ReadArrayValues(mask_e, 0, mask_d, 0, numberOfNodes);

    if (numberOfNodes == numberOfVertices)
    {
      for (int i = 0; i < numberOfNodes; i++)
      {
        htgMask->SetValue(globalOffset + i, mask->GetValue(i));
      }
      hasMaskData = true;
    }

    // PointData belonging to hypertree immediately follows descriptor
    vtkPointData* pointData = output->GetPointData();
    vtkXMLDataElement* ePointData = eTree->GetNestedElement(2);
    if (ePointData)
    {
      for (int j = 0; j < ePointData->GetNumberOfNestedElements(); j++)
      {
        vtkXMLDataElement* eNested = ePointData->GetNestedElement(j);
        const char* ename = eNested->GetAttribute("Name");
        vtkAbstractArray* outArray = pointData->GetArray(ename);
        int numberOfComponents = 1;
        const char* eNC = eNested->GetAttribute("NumberOfComponents");
        if (eNC)
        {
          numberOfComponents = atoi(eNC);
        }

        // Create the output PointData array when processing first tree
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
    mask_a->Delete();
  }
  if (hasMaskData)
  {
    output->SetMask(htgMask);
  }
}

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::SubdivideFromDescriptor_0(
  vtkHyperTreeGridNonOrientedCursor* treeCursor, unsigned int level, int numChildren,
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

  for (int child = 0; child < numChildren; ++child)
  {
    treeCursor->ToChild(child);
    this->SubdivideFromDescriptor_0(treeCursor, level + 1, numChildren, descriptor, posByLevel);
    treeCursor->ToParent();
  }
}

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::ReadTrees_1(vtkXMLDataElement* elem)
{
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(this->GetCurrentOutput());
  vtkNew<vtkHyperTreeGridNonOrientedCursor> treeCursor;

  // Number of trees in this hypertree grid file
  int numberOfTrees = elem->GetNumberOfNestedElements();
  elem->GetScalarAttribute("NumberOfTrees", numberOfTrees);

  // Hypertree grid mask collected while processing hypertrees
  vtkNew<vtkBitArray> htgMask;
  htgMask->SetNumberOfTuples(this->NumberOfPoints);

  vtkIdType globalOffset = 0;
  for (int treeIndxInFile = 0; treeIndxInFile < numberOfTrees; ++treeIndxInFile)
  {
    // Nested elements within Trees is Tree
    vtkXMLDataElement* eTree = elem->GetNestedElement(treeIndxInFile);
    vtkIdType treeIndxInHTG;
    vtkIdType numberOfVertices;
    vtkIdType numberOfLevels;
    eTree->GetScalarAttribute("Index", treeIndxInHTG);

    // Functionnality not available on older versions
    if (!this->IsSelectedHT(output, treeIndxInHTG))
    {
      continue;
    }

    eTree->GetScalarAttribute("NumberOfLevels", numberOfLevels);
    eTree->GetScalarAttribute("NumberOfVertices", numberOfVertices);

    // Descriptor for hypertree
    vtkXMLDataElement* desc_e = eTree->GetNestedElement(0);
    vtkAbstractArray* desc_a = this->CreateArray(desc_e);
    vtkDataArray* desc_d = vtkArrayDownCast<vtkDataArray>(desc_a);
    if (!desc_d)
    {
      if (desc_a)
      {
        desc_a->Delete();
      }
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
        desc_d->Delete();
        return;
      }
      desc = vtkArrayDownCast<vtkBitArray>(desc_d);
      if (!desc)
      {
        vtkErrorMacro(
          "Cannot convert vtkDataArray of type " << desc_d->GetDataType() << " to vtkBitArray.");
        desc_d->Delete();
        return;
      }
    }

    // Parse descriptor storing the global index per level of hypertree
    vtkNew<vtkIdTypeArray> posByLevel;
    output->InitializeNonOrientedCursor(treeCursor, treeIndxInHTG, true);

    treeCursor->SetGlobalIndexStart(globalOffset);

    // Level 0 contains root of hypertree
    posByLevel->InsertNextValue(0);
    vtkIdType nRefined = 0;
    vtkIdType nCurrentLevel = 0;
    vtkIdType nNextLevel = 1;
    int numberOfChildren = output->GetNumberOfChildren();

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

    // NbVerticesByLevel is stored in XML element
    vtkXMLDataElement* nbByLvl_e = eTree->GetNestedElement(1);
    vtkAbstractArray* nbByLvl_a = this->CreateArray(nbByLvl_e);
    vtkDataArray* nbByLvl_d = vtkArrayDownCast<vtkDataArray>(nbByLvl_a);
    int numberOfNodes = 0;
    nbByLvl_e->GetScalarAttribute("NumberOfTuples", numberOfNodes);
    nbByLvl_d->SetNumberOfTuples(numberOfNodes);
    vtkUnsignedLongArray* nbByLvl = vtkArrayDownCast<vtkUnsignedLongArray>(nbByLvl_d);

    this->ReadArrayValues(nbByLvl_e, 0, nbByLvl_d, 0, numberOfNodes);

    // Mask is stored in XML element
    vtkXMLDataElement* mask_e = eTree->GetNestedElement(2);
    vtkAbstractArray* mask_a = this->CreateArray(mask_e);
    vtkDataArray* mask_d = vtkArrayDownCast<vtkDataArray>(mask_a);
    numberOfNodes = 0;
    mask_e->GetScalarAttribute("NumberOfTuples", numberOfNodes);
    mask_d->SetNumberOfTuples(numberOfNodes);
    vtkBitArray* mask = vtkArrayDownCast<vtkBitArray>(mask_d);

    this->ReadArrayValues(mask_e, 0, mask_d, 0, numberOfNodes);

    vtkIdType limitedLevel = this->GetFixedLevelOfThisHT(numberOfLevels, treeIndxInHTG);
    vtkIdType fixedNbVertices = 0;
    for (vtkIdType ilevel = 0; ilevel < limitedLevel; ++ilevel)
    {
      fixedNbVertices += nbByLvl->GetValue(ilevel);
    }
    treeCursor->GetTree()->InitializeForReader(limitedLevel, fixedNbVertices,
      nbByLvl->GetValue(limitedLevel - 1), desc, mask, output->GetMask());
    nbByLvl_a->Delete();
    desc_a->Delete();
    mask_a->Delete();
    // PointData belonging to hypertree immediately follows descriptor
    vtkPointData* pointData = output->GetPointData();
    vtkXMLDataElement* ePointData = eTree->GetNestedElement(3);
    if (ePointData)
    {

      for (int j = 0; j < ePointData->GetNumberOfNestedElements(); ++j)
      {
        vtkXMLDataElement* eNested = ePointData->GetNestedElement(j);
        const char* ename = eNested->GetAttribute("Name");
        vtkAbstractArray* outArray = pointData->GetArray(ename);
        int numberOfComponents = 1;
        const char* eNC = eNested->GetAttribute("NumberOfComponents");
        if (eNC)
        {
          numberOfComponents = atoi(eNC);
        }

        // Create the output PointData array when processing first tree
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
        // outArray->Resize(outArray->GetNumberOfTuples()+fixedNbVertices);
        // Tip: insert copy of an existing table data in position 0 to
        // the last position of the same table
        outArray->InsertTuple(outArray->GetNumberOfTuples() + fixedNbVertices - 1, 0, outArray);

        // Read data into the global offset which is
        // number of vertices in the tree * number of components in the data
        this->ReadArrayValues(eNested, globalOffset * numberOfComponents, outArray, 0,
          fixedNbVertices * numberOfComponents, POINT_DATA);
      }
    }
    // Calculating the first offset of the next HyperTree
    globalOffset += treeCursor->GetTree()->GetNumberOfVertices();
  }
}
