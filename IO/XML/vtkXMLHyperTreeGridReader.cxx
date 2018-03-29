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

//TODO:
// Add support for timesteps
// Add streaming support.

#include "vtkXMLHyperTreeGridReader.h"

#include "vtkBitArray.h"
#include "vtkDataArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeCursor.h"
#include "vtkHyperTreeGrid.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"

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
int vtkXMLHyperTreeGridReader::FillOutputPortInformation(int,
                                                         vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLHyperTreeGridReader::GetNumberOfPoints()
{
  vtkIdType numPts = 0;
  vtkDataSet* output = vtkDataSet::SafeDownCast(this->GetCurrentOutput());
  if (output)
  {
    numPts = output->GetNumberOfPoints();
  }
  return numPts;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLHyperTreeGridReader::GetNumberOfCells()
{
  vtkIdType numCells = 0;
  vtkDataSet* output = vtkDataSet::SafeDownCast(this->GetCurrentOutput());
  if (output)
  {
    numCells = output->GetNumberOfCells();
  }
  return numCells;
}

//----------------------------------------------------------------------------
int vtkXMLHyperTreeGridReader::ReadArrayForPoints(vtkXMLDataElement* da,
                                                  vtkAbstractArray* outArray)
{
  vtkIdType components = outArray->GetNumberOfComponents();
  vtkIdType numberOfTuples = this->GetNumberOfPoints();
  outArray->SetNumberOfTuples(numberOfTuples);
  return this->ReadArrayValues(da, 0, outArray, 0, numberOfTuples*components);
}

//----------------------------------------------------------------------------
int vtkXMLHyperTreeGridReader::ReadArrayForCells(vtkXMLDataElement* da,
                                                 vtkAbstractArray* outArray)
{
  vtkIdType components = outArray->GetNumberOfComponents();
  vtkIdType numberOfTuples = this->GetNumberOfCells();
  outArray->SetNumberOfTuples(numberOfTuples);
  return this->ReadArrayValues(da, 0, outArray, 0, numberOfTuples*components);
}

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::ReadXMLData()
{
  this->Superclass::ReadXMLData();

  vtkXMLDataElement *ePrimary =
    this->XMLParser->GetRootElement()->GetNestedElement(0);

  int dimension;
  int branchFactor;
  int transposedRootIndexing;
  int gridSize[3];

  // Read the attributes of the hyper tree grid
  if (!ePrimary->GetScalarAttribute("Dimension", dimension))
  {
    dimension = 3;
  }

  if (!ePrimary->GetScalarAttribute("BranchFactor", branchFactor))
  {
    branchFactor = 2;
  }

  if (!ePrimary->GetScalarAttribute("TransposedRootIndexing",
                                    transposedRootIndexing))
  {
    transposedRootIndexing = 0;
  }

  if (ePrimary->GetVectorAttribute("GridSize", 3, gridSize) != 3)
  {
    gridSize[0] = 1;
    gridSize[1] = 1;
    gridSize[2] = 1;
  }

#if 0
  double gridOrigin[3];
  double gridScale[3];

  // If the origin and scale are uniform can use this instead of coordinates
  // Currently not implemented in vtkHyperTreeGrid but this is a place holder
  if (ePrimary->GetVectorAttribute("GridOrigin", 3, gridOrigin) != 3)
  {
    gridOrigin[0] = 0;
    gridOrigin[1] = 0;
    gridOrigin[2] = 0;
  }

  if (ePrimary->GetVectorAttribute("GridScale", 3, gridScale) != 3)
  {
    gridScale[0] = 1;
    gridScale[1] = 1;
    gridScale[2] = 1;
  }
#endif

  // Define the hypertree grid
  vtkHyperTreeGrid *output = vtkHyperTreeGrid::SafeDownCast(
      this->GetCurrentOutput());
  output->SetDimension(dimension);
  output->SetBranchFactor(branchFactor);
  output->SetTransposedRootIndexing((transposedRootIndexing!=0));
  output->SetGridSize((unsigned int) gridSize[0],
                      (unsigned int) gridSize[1],
                      (unsigned int) gridSize[2]);

  // Read geometry of hypertree grid expressed in coordinates
  // Read the topology of each hypertree
  vtkXMLDataElement* eNested = ePrimary->GetNestedElement(0);
  if (strcmp(eNested->GetName(), "Coordinates") == 0)
  {
    this->ReadCoordinates(eNested);
  }

  eNested = ePrimary->GetNestedElement(1);
  if (strcmp(eNested->GetName(), "Topology") == 0)
  {
    this->ReadTopology(eNested);
  }

  // Read the PointData attribute data with one piece per file
  this->ReadPieceData();
}

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::ReadCoordinates(vtkXMLDataElement *elem)
{
  vtkHyperTreeGrid *output = vtkHyperTreeGrid::SafeDownCast(
      this->GetCurrentOutput());

  vtkIdType numX, numY, numZ;

  vtkXMLDataElement* xc = elem->GetNestedElement(0);
  vtkXMLDataElement* yc = elem->GetNestedElement(1);
  vtkXMLDataElement* zc = elem->GetNestedElement(2);

  vtkAbstractArray* xa = this->CreateArray(xc);
  vtkAbstractArray* ya = this->CreateArray(yc);
  vtkAbstractArray* za = this->CreateArray(zc);

  vtkDataArray* x = vtkArrayDownCast<vtkDataArray>(xa);
  vtkDataArray* y = vtkArrayDownCast<vtkDataArray>(ya);
  vtkDataArray* z = vtkArrayDownCast<vtkDataArray>(za);

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
void vtkXMLHyperTreeGridReader::ReadTopology(vtkXMLDataElement *elem)
{
  // Topology consists of MaterialMaskIndex giving hypertree indices
  // and Descriptor to indicate the building of the recursive trees
  vtkHyperTreeGrid *output = vtkHyperTreeGrid::SafeDownCast(
      this->GetCurrentOutput());

  int numNested = elem->GetNumberOfNestedElements();
  if (numNested < 1 || numNested > 2)
  {
    vtkErrorMacro("Invalid topology in file. "
                  << "Expect at least descriptor array.");
    return;
  }

  int numberOfIds = 1;
  vtkIdTypeArray* id = nullptr;
  vtkAbstractArray* id_a = nullptr;
  if (numNested == 2)
  {
    // MaterialMaskIndex array gives existing hypertrees in file
    vtkXMLDataElement* id_e = elem->GetNestedElement(0);
    id_a = this->CreateArray(id_e);
    vtkDataArray* id_d = vtkArrayDownCast<vtkDataArray>(id_a);
    if (!id_d)
    {
      if (id_a)
      {
        vtkErrorMacro("Invalid material mask array. Not a numeric array. ");
        id_a->Delete();
      }
      return;
    }

    if (!id_e->GetScalarAttribute("NumberOfTuples", numberOfIds))
    {
      // No hyper trees defined in this file.
      // Maybe indicates an empty parallel partition on this processor?
      // That is is OK. Return an empty hypertreegrid.
      id = static_cast<vtkIdTypeArray*>(id_d);
      id->SetNumberOfTuples(0);
      output->Initialize();
      output->SetMaterialMaskIndex(id);
      output->GenerateTrees();
      id_d->Delete();
      return;
    }

    // Hypertrees exist in file so build topology
    id_d->SetNumberOfTuples(numberOfIds);
    if (numberOfIds > 0)
    {
      if (!this->ReadArrayValues(id_e, 0, id_d, 0, numberOfIds))
      {
        vtkErrorMacro("Invalid material mask array. Couldn't read values. ");
        id_d->Delete();
        return;
      }
    }

    // TODO: PROBLEM XMLWriter writes type as VTK_LONG not VTK_ID_TYPE so must
    // use static cast.
    id = static_cast<vtkIdTypeArray*>(id_d);
    if (!id)
    {
      vtkErrorMacro("Invalid material mask array. Type is "
                    << id_d->GetDataType() << " not vtkIdTypeArray.");
      id_d->Delete();
      return;
    }
  }

  // Descriptor describes the topology of existing hypertrees
  vtkXMLDataElement* desc_e = elem->GetNestedElement((numNested==2?1:0));
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
    vtkErrorMacro("Cannot convert vtkDataArray of type "
                  << desc_d->GetDataType()
                  << " to vtkBitArray.");
    desc_d->Delete();
    return;
  }

  // Parse descriptor storing the offset per level of hypertree
  vtkNew<vtkIdTypeArray> posByLevel;

  // Level 0 contains entries for each of the existing hypertrees
  posByLevel->InsertNextValue(0);
  vtkIdType nRefined = 0;
  vtkIdType nCurrentLevelCount = 0;
  vtkIdType descSize = desc->GetNumberOfTuples();
  int numberOfChildren = output->GetNumberOfChildren();
  if (numNested == 1)
  {
    //We don't have a materialmask telling us specific top level cells
    //to fill. Figure out how many there are here, and fill them out
    //sequentially below.
    unsigned int *dims = output->GetGridSize();
    for (unsigned int i = 0; i < output->GetDimension(); i++)
    {
      numberOfIds *= dims[i];
    }
  }
  vtkIdType nNextLevel = numberOfIds;

  // Iterate over every item in descriptor to gather level offset
  // information for recursion
  for (vtkIdType i = 0; i < descSize; ++i)
  {
    if (nCurrentLevelCount >= nNextLevel)
    {
      // reached the next level of data in the breadth first descriptor array
      nNextLevel = nRefined * numberOfChildren;
      nRefined = 0;
      nCurrentLevelCount = 0;
      posByLevel->InsertNextValue(i);
    }
    if (desc->GetValue(i) == 1)
    {
      nRefined++;
    }
    nCurrentLevelCount++;
  }

  // Initialize the hyper tree grid with empty hypertrees from file
  //output->Initialize();
  output->SetMaterialMaskIndex(id);
  output->GenerateTrees();

  // Iterate over all hypertrees belonging to this processor
  vtkIdType cellsOnProcessor = 0;
  for (int t = 0; t < numberOfIds; t++)
  {
    vtkIdType counter = 1;
    vtkIdType index = (id?id->GetValue(t):t); //fill sequentially without mask
    vtkHyperTreeCursor* treeCursor = output->NewCursor(index, true);
    treeCursor->ToRoot();
    vtkHyperTree* hyperTree = treeCursor->GetTree();

    this->SubdivideFromDescriptor(treeCursor, hyperTree, 0, numberOfChildren,
                                  desc, posByLevel, &counter);

    hyperTree->SetGlobalIndexStart(cellsOnProcessor);
    cellsOnProcessor += counter;

#if 0
    if (hyperTree->GetNumberOfLevels() > 1)
    {
      cerr << "Proc " << " Tree from Reader " << index
             << "  levels = " << hyperTree->GetNumberOfLevels()
             << "  nodes = " << hyperTree->GetNumberOfNodes()
             << "  LEAVES = " << hyperTree->GetNumberOfLeaves()
             << endl;
    }
#endif
    treeCursor->Delete();
  }

  // Material mask set because convert to unstructured grid wants to look at
  // it but I should not have to set it
  vtkNew<vtkBitArray> materialMask;
  materialMask->Allocate(cellsOnProcessor);
  for (int c = 0; c < cellsOnProcessor; c++)
  {
    materialMask->InsertNextValue(0);
  }
  output->SetMaterialMask(materialMask);

  if (id_a)
  {
    id_a->Delete();
  }
  desc_a->Delete();
}

//----------------------------------------------------------------------------
void vtkXMLHyperTreeGridReader::SubdivideFromDescriptor
(
 vtkHyperTreeCursor* treeCursor,
 vtkHyperTree* tree,
 unsigned int level,
 int numChildren,
 vtkBitArray* descriptor,
 vtkIdTypeArray* posByLevel,
 vtkIdType* cellsOnProcessor)
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
  tree->SubdivideLeaf(treeCursor);
  *cellsOnProcessor = *cellsOnProcessor + numChildren;

  for (int child = 0; child < numChildren; ++child)
  {
    treeCursor->ToChild(child);
    this->SubdivideFromDescriptor(treeCursor, tree, level + 1, numChildren,
                                  descriptor, posByLevel, cellsOnProcessor);
    treeCursor->ToParent();
  }
}
