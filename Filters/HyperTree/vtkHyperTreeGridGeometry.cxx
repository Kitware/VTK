/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridGeometry.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridGeometry.h"

#include "vtkBitArray.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkExtentTranslator.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridNonOrientedVonNeumannSuperCursor.h"
#include "vtkHyperTreeGridOrientedGeometryCursor.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"

#include <limits>
#include <set>
#include <vector>

static constexpr unsigned int VonNeumannCursors3D[] = { 0, 1, 2, 4, 5, 6 };
static constexpr unsigned int VonNeumannOrientations3D[] = { 2, 1, 0, 0, 1, 2 };
static constexpr unsigned int VonNeumannOffsets3D[] = { 0, 0, 0, 1, 1, 1 };

static constexpr unsigned int EdgeIndices[3][2][4] = { { { 3, 11, 7, 8 }, { 1, 10, 5, 9 } },
  { { 0, 9, 4, 8 }, { 2, 10, 6, 11 } }, { { 0, 1, 2, 3 }, { 4, 5, 6, 7 } } };

constexpr unsigned char FULL_WORK_FACES = std::numeric_limits<unsigned char>::max();

vtkStandardNewMacro(vtkHyperTreeGridGeometry);

//-----------------------------------------------------------------------------
vtkHyperTreeGridGeometry::vtkHyperTreeGridGeometry()
{
  // Create storage for corners of leaf cells
  this->Points = vtkPoints::New();

  // Create storage for untructured leaf cells
  this->Cells = vtkCellArray::New();

  // Default dimension is 0
  this->Dimension = 0;

  // Default orientation is 0
  this->Orientation = 0;

  // Default orientation is 0
  this->BranchFactor = 0;

  // Default Locator is 0
  this->Merging = false;
  this->Locator = nullptr;

  // Default interface values
  this->HasInterface = false;
  this->Normals = nullptr;
  this->Intercepts = nullptr;
  this->FaceIDs = vtkIdList::New();
  this->FacePoints = vtkPoints::New();
  this->FacePoints->SetNumberOfPoints(4);
  this->FacesA = vtkIdTypeArray::New();
  this->FacesA->SetNumberOfComponents(2);
  this->FacesB = vtkIdTypeArray::New();
  this->FacesB->SetNumberOfComponents(2);
  this->FaceScalarsA = vtkDoubleArray::New();
  this->FaceScalarsA->SetNumberOfTuples(4);
  this->FaceScalarsB = vtkDoubleArray::New();
  this->FaceScalarsB->SetNumberOfTuples(4);

  this->EdgeFlags = nullptr;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridGeometry::~vtkHyperTreeGridGeometry()
{
  if (this->Points)
  {
    this->Points->Delete();
    this->Points = nullptr;
  }

  if (this->Cells)
  {
    this->Cells->Delete();
    this->Cells = nullptr;
  }

  if (this->FacePoints)
  {
    this->FacePoints->Delete();
    this->FacePoints = nullptr;
  }

  if (this->FaceIDs)
  {
    this->FaceIDs->Delete();
    this->FaceIDs = nullptr;
  }

  if (this->FacesA)
  {
    this->FacesA->Delete();
    this->FacesA = nullptr;
  }

  if (this->FacesB)
  {
    this->FacesB->Delete();
    this->FacesB = nullptr;
  }

  if (this->FaceScalarsA)
  {
    this->FaceScalarsA->Delete();
    this->FaceScalarsA = nullptr;
  }

  if (this->FaceScalarsB)
  {
    this->FaceScalarsB->Delete();
    this->FaceScalarsB = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Points)
  {
    os << indent << "Points:\n";
    this->Points->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Points: ( none )\n";
  }

  if (this->Cells)
  {
    os << indent << "Cells:\n";
    this->Cells->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Cells: ( none )\n";
  }

  os << indent << "Dimension: " << this->Dimension << endl;
  os << indent << "Orientation: " << this->Orientation << endl;
  os << indent << "Merging: " << this->Merging << endl;
  os << indent << "HasInterface: " << this->HasInterface << endl;
  if (this->Normals)
  {
    os << indent << ":\n";
    this->Normals->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Normals: ( none )\n";
  }
  if (this->Intercepts)
  {
    os << indent << ":\n";
    this->Intercepts->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Intercepts: ( none )\n";
  }
  if (this->FacePoints)
  {
    os << indent << ":\n";
    this->FacePoints->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "FacePoints: ( none )\n";
  }
  if (this->FaceIDs)
  {
    os << indent << ":\n";
    this->FaceIDs->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "FaceIDs: ( none )\n";
  }
  os << indent << "EdgesA:";
  for (unsigned int i = 0; i < 12; ++i)
  {
    os << " " << EdgesA[i];
  }
  os << endl;
  os << indent << "EdgesB:";
  for (unsigned int i = 0; i < 12; ++i)
  {
    os << " " << EdgesB[i];
  }
  os << endl;
  if (this->FacesA)
  {
    os << indent << ":\n";
    this->FacesA->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "FacesA: ( none )\n";
  }
  if (this->FacesB)
  {
    os << indent << ":\n";
    this->FacesB->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "FacesB: ( none )\n";
  }
  if (this->FaceScalarsA)
  {
    os << indent << ":\n";
    this->FaceScalarsA->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "FaceScalarsA: ( none )\n";
  }
  if (this->FaceScalarsB)
  {
    os << indent << ":\n";
    this->FaceScalarsB->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "FaceScalarsB: ( none )\n";
  }
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridGeometry::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridGeometry::ProcessTrees(vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  // Downcast output data object to polygonal data set
  vtkPolyData* output = vtkPolyData::SafeDownCast(outputDO);
  if (!output)
  {
    vtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  // Retrieve useful grid parameters for speed of access
  this->Dimension = input->GetDimension();
  this->Orientation = input->GetOrientation();

  this->BranchFactor = static_cast<int>(input->GetBranchFactor());

  // Initialize output cell data
  this->InData = input->GetPointData();
  this->OutData = output->GetCellData();
  this->OutData->CopyAllocate(this->InData);

  // Retrieve material mask
  this->Mask = input->HasMask() ? input->GetMask() : nullptr;

  // Retrieve pure material mask
  this->PureMask = input->GetPureMask();

  // Retrieve interface data when relevant
  this->HasInterface = input->GetHasInterface();
  if (this->HasInterface)
  {
    this->Normals =
      vtkDoubleArray::SafeDownCast(this->InData->GetArray(input->GetInterfaceNormalsName()));
    this->Intercepts =
      vtkDoubleArray::SafeDownCast(this->InData->GetArray(input->GetInterfaceInterceptsName()));
  } // this->HasInterface

  // Create storage for corners of leaf cells
  if (this->Points)
  {
    this->Points->Delete();
  }
  this->Points = vtkPoints::New();

  // Create storage for untructured leaf cells
  if (this->Cells)
  {
    this->Cells->Delete();
  }
  this->Cells = vtkCellArray::New();

  // JB Initialize a Locator
  if (this->Merging)
  {
    if (this->Locator)
    {
      this->Locator->Delete();
    }
    this->Locator = vtkMergePoints::New();
    this->Locator->InitPointInsertion(this->Points, input->GetBounds());
  }

  // Iterate over all hyper trees
  vtkIdType index;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  input->InitializeTreeIterator(it);
  if (this->Dimension == 3)
  {
    // Flag used to hide edges when needed
    this->EdgeFlags = vtkUnsignedCharArray::New();
    this->EdgeFlags->SetName("vtkEdgeFlags");
    this->EdgeFlags->SetNumberOfComponents(1);

    vtkPointData* outPointData = output->GetPointData();
    outPointData->AddArray(this->EdgeFlags);
    outPointData->SetActiveAttribute(this->EdgeFlags->GetName(), vtkDataSetAttributes::EDGEFLAG);

    vtkNew<vtkHyperTreeGridNonOrientedVonNeumannSuperCursor> cursor;
    while (it.GetNextTree(index))
    {
      // Initialize new cursor at root of current tree
      // In 3 dimensions, von Neumann neighborhood information is needed
      input->InitializeNonOrientedVonNeumannSuperCursor(cursor, index);
      // Build geometry recursively
      this->RecursivelyProcessTree3D(cursor, FULL_WORK_FACES);
    } // it
  }
  else
  {
    vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursor;
    while (it.GetNextTree(index))
    {
      // Initialize new cursor at root of current tree
      // Otherwise, geometric properties of the cells suffice
      input->InitializeNonOrientedGeometryCursor(cursor, index);
      // Build geometry recursively
      this->RecursivelyProcessTreeNot3D(cursor);
    } // it
  }   // else

  // Set output geometry and topology
  output->SetPoints(this->Points);
  if (this->Dimension == 1)
  {
    output->SetLines(this->Cells);
  }
  else
  {
    output->SetPolys(this->Cells);
  }

  if (this->EdgeFlags)
  {
    this->EdgeFlags->Delete();
    this->EdgeFlags = nullptr;
  }

  if (this->Points)
  {
    this->Points->Delete();
    this->Points = nullptr;
  }
  if (this->Cells)
  {
    this->Cells->Delete();
    this->Cells = nullptr;
  }

  if (this->Locator)
  {
    this->Locator->Delete();
    this->Locator = nullptr;
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::RecursivelyProcessTreeNot3D(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  if (this->Mask ? this->Mask->GetValue(cursor->GetGlobalNodeIndex()) : false)
  {
    return;
  }

  // Create geometry output if cursor is at leaf
  if (cursor->IsLeaf())
  {
    // Cursor is at leaf, process it depending on its dimension
    switch (this->Dimension)
    {
      case 1:
        this->ProcessLeaf1D(cursor);
        break;
      case 2:
        this->ProcessLeaf2D(cursor);
        break;
      default:
        break;
    } // switch ( this->Dimension )
    return;
  } // if ( cursor->IsLeaf() )

  // FR Il existe une filles qui n'est pas dans le mat, on la chercher partout
  unsigned int numChildren = cursor->GetNumberOfChildren();
  for (unsigned int ichild = 0; ichild < numChildren; ++ichild)
  {
    cursor->ToChild(ichild);
    // Recurse
    this->RecursivelyProcessTreeNot3D(cursor);
    cursor->ToParent();
  } // ichild
}

//----------------------------------------------------------------------------
// JB Meme code que vtkAdaptativeDataSetSurfaceFiltre ??
void vtkHyperTreeGridGeometry::ProcessLeaf1D(vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  // Cell at cursor center is a leaf, retrieve its global index
  vtkIdType inId = cursor->GetGlobalNodeIndex();
  if (inId < 0)
  {
    return;
  }

  // In 1D the geometry is composed of edges, create storage for endpoint IDs
  vtkIdType ids[2];

  // First endpoint is at origin of cursor
  const double* origin = cursor->GetOrigin();

  // Second endpoint is at origin of cursor plus its length
  double pt[3];
  memcpy(pt, origin, 3 * sizeof(double));
  pt[this->Orientation] += cursor->GetSize()[this->Orientation];

  if (this->Locator)
  {
    this->Locator->InsertUniquePoint(origin, ids[0]);
    this->Locator->InsertUniquePoint(pt, ids[1]);
  }
  else
  {
    ids[0] = this->Points->InsertNextPoint(origin);
    ids[1] = this->Points->InsertNextPoint(pt);
  }

  // Insert edge into 1D geometry
  vtkIdType outId = this->Cells->InsertNextCell(2, ids);

  // Copy edge data from that of the cell from which it comes
  this->OutData->CopyData(this->InData, inId, outId);
}

//----------------------------------------------------------------------------
// JB Meme code que vtkAdaptativeDataSetSurfaceFiltre ??
void vtkHyperTreeGridGeometry::ProcessLeaf2D(vtkHyperTreeGridNonOrientedGeometryCursor* cursor)

{
  // Cell at cursor center is a leaf, retrieve its global index
  vtkIdType inId = cursor->GetGlobalNodeIndex();
  assert(inId >= 0);

  // Reset interface variables if needed
  if (this->HasInterface)
  {
    size_t int12sz = 12 * sizeof(vtkIdType);
    memset(this->EdgesA, -1, int12sz);
    memset(this->EdgesB, -1, int12sz);
    this->FacesA->Reset();
    this->FacesB->Reset();
  } // if ( this->HasInterface )

  // Insert face into 2D geometry depending on orientation
  // this->AddFace( inId, cursor->GetOrigin(), cursor->GetSize(), 0, this->Orientation );
  this->AddFace2(inId, inId, cursor->GetOrigin(), cursor->GetSize(), 0, this->Orientation);
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::RecursivelyProcessTree3D(
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursor* cursor, unsigned char crtWorkFaces)
{
  // FR Traitement specifique pour la maille fille centrale en raffinement 3
  // Create geometry output if cursor is at leaf
  if (cursor->IsLeaf() || cursor->IsMasked())
  {
    // Cursor is at leaf, process it depending on its dimension
    this->ProcessLeaf3D(cursor); // JBVTK9 ProcessLeaf3D2 prends en compte les interfaces... :)?
    return;
  } // if ( cursor->IsLeaf() )

  // FR Parce que le curseur est un super curseur
  bool pureMask = false;
  if (this->Mask != nullptr)
  {
    // JB Question : que fait le PureMaterialMask quand un masque est mis sur un coarse et pas
    // toutes les filles
    pureMask = this->PureMask->GetValue(cursor->GetGlobalNodeIndex()) != 0;
  }
  if (!pureMask)
  {
    // FR La maille courante est donc pure (pureMask == false car GetPureMask()->GetValue(..)
    // retourne false)
    std::set<int> childList;

    const unsigned int numChildren = cursor->GetNumberOfChildren();
    std::vector<unsigned char> workFaces(numChildren, 0);

    // FR Toutes les filles sont dans le materiau, on traite que les filles du bord
    for (unsigned int f = 0; f < 3; ++f) // dimension
    {
      for (unsigned int o = 0; o < 2; ++o) // gauche, centre, droite
      {
        int neighborIdx = (2 * o - 1) * (f + 1);
        if ((crtWorkFaces & (1 << (this->Dimension + neighborIdx))))
        {
          bool isValidN = cursor->HasTree(this->Dimension + neighborIdx);
          vtkIdType idN = 0;
          if (isValidN)
          {
            idN = cursor->GetGlobalNodeIndex(this->Dimension + neighborIdx);
          }
          if (!isValidN || (this->Mask && this->PureMask->GetValue(idN)))
          {
            // FR La maille voisine n'existe pas ou n'est pas pure (PureMask->GetValue(id) retourne
            // false) FR Fille du bord
            int iMin = (f == 0 && o == 1) ? this->BranchFactor - 1 : 0;
            int iMax = (f == 0 && o == 0) ? 1 : this->BranchFactor;
            int jMin = (f == 1 && o == 1) ? this->BranchFactor - 1 : 0;
            int jMax = (f == 1 && o == 0) ? 1 : this->BranchFactor;
            int kMin = (f == 2 && o == 1) ? this->BranchFactor - 1 : 0;
            int kMax = (f == 2 && o == 0) ? 1 : this->BranchFactor;
            for (int i = iMin; i < iMax; ++i)
            {
              for (int j = jMin; j < jMax; ++j)
              {
                for (int k = kMin; k < kMax; ++k)
                {
                  unsigned int ichild = i + this->BranchFactor * (j + this->BranchFactor * k);

                  // FR Les mailles de coin peuvent etre sollicitees plusieurs fois suivant chacune
                  // des faces
                  childList.insert(ichild);
                  workFaces[ichild] |= (1 << (this->Dimension + neighborIdx));
                } // k
              }   // j
            }     // i
          }       // if ...
        }
      } // o
    }   // f
    for (std::set<int>::iterator it = childList.begin(); it != childList.end(); ++it)
    {
      cursor->ToChild(*it);
      this->RecursivelyProcessTree3D(cursor, workFaces[*it]);
      cursor->ToParent();
    } // ichild
    return;
  }
  // FR Il existe une filles qui n'est pas dans le mat, on la chercher partout
  unsigned int numChildren = cursor->GetNumberOfChildren();
  for (unsigned int ichild = 0; ichild < numChildren; ++ichild)
  {
    cursor->ToChild(ichild);
    this->RecursivelyProcessTree3D(cursor, FULL_WORK_FACES);
    cursor->ToParent();
  } // ichild
}

//----------------------------------------------------------------------------
// JB Meme code que vtkAdaptativeDataSetSurfaceFiltre ??
void vtkHyperTreeGridGeometry::ProcessLeaf3D(
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursor* superCursor)
{
  // Cell at cursor center is a leaf, retrieve its global index, and mask
  vtkIdType inId = superCursor->GetGlobalNodeIndex();
  if (inId < 0)
  {
    return;
  }
  unsigned level = superCursor->GetLevel();

  // Reset interface variables if needed
  if (this->HasInterface)
  {
    size_t int12sz = 12 * sizeof(vtkIdType);
    memset(this->EdgesA, -1, int12sz);
    memset(this->EdgesB, -1, int12sz);
    this->FacesA->Reset();
    this->FacesB->Reset();

    // Retrieve intercept type
    this->Intercepts->GetComponent(inId, 2);
  } // if ( this->HasInterface )

  // Iterate over all cursors of Von Neumann neighborhood around center
  unsigned int nc = superCursor->GetNumberOfCursors() - 1;
  for (unsigned int c = 0; c < nc; ++c)
  {
    // Retrieve cursor to neighbor across face
    // Retrieve tree, leaf flag, and mask of neighbor cursor
    bool leafN;
    vtkIdType idN;
    unsigned int levelN;
    vtkHyperTree* treeN = superCursor->GetInformation(VonNeumannCursors3D[c], levelN, leafN, idN);
    int maskedN = superCursor->IsMasked(VonNeumannCursors3D[c]);

    // In 3D masked and unmasked cells are handled differently:
    // . If cell is unmasked, and face neighbor is a masked leaf, or no such neighbor
    //   exists, then generate face.
    // . If cell is masked, and face neighbor exists and is an unmasked leaf, then
    //   generate face, breaking ties at same level. This ensures that faces between
    //   unmasked and masked cells will be generated once and only once.
    if ((!superCursor->IsMasked() && (!treeN || maskedN)) ||
      (superCursor->IsMasked() && treeN && leafN && levelN <= level && !maskedN))
    {
      double boundsN[6], bounds[6];
      vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursorN =
        superCursor->GetOrientedGeometryCursor(VonNeumannCursors3D[c]);

      // If not using a flag on edges, faces that are neighbor to masked cells have unwanted edges.
      // That is because it is actually the neighbors of the coarser level that, accumulate,
      // construct this face. This flag intends to hide edges that are inside the face.
      unsigned char edgeFlag;

      if (cursorN->GetTree() && superCursor->GetTree())
      {
        superCursor->GetBounds(bounds);
        cursorN->GetBounds(boundsN);

        edgeFlag = (static_cast<unsigned char>(vtkMathUtilities::NearlyEqual(
                     boundsN[((VonNeumannOrientations3D[c] + 1) % 3) * 2],
                     bounds[((VonNeumannOrientations3D[c] + 1) % 3) * 2]))) |
          (static_cast<unsigned char>(
             vtkMathUtilities::NearlyEqual(boundsN[((VonNeumannOrientations3D[c] + 1) % 3) * 2 + 1],
               bounds[((VonNeumannOrientations3D[c] + 1) % 3) * 2 + 1]))
            << 1) |
          (static_cast<unsigned char>(
             vtkMathUtilities::NearlyEqual(boundsN[((VonNeumannOrientations3D[c] + 2) % 3) * 2],
               bounds[((VonNeumannOrientations3D[c] + 2) % 3) * 2]))
            << 2) |
          (static_cast<unsigned char>(
             vtkMathUtilities::NearlyEqual(boundsN[((VonNeumannOrientations3D[c] + 2) % 3) * 2 + 1],
               bounds[((VonNeumannOrientations3D[c] + 2) % 3) * 2 + 1]))
            << 3);
      }
      else
      {
        edgeFlag = 15;
      }
      if (levelN == level || !treeN || !superCursor->IsMasked())
      {
        edgeFlag = 15; // 1111 in binary
      }

      // Generate face with corresponding normal and offset
      this->AddFace(superCursor->IsMasked() ? idN : inId, superCursor->GetOrigin(),
        superCursor->GetSize(), VonNeumannOffsets3D[c], VonNeumannOrientations3D[c], edgeFlag);
    }
    /*
        // In 3D masked and unmasked cells are handled differently
        if ( superCursor->IsMasked() )
        {
          // If cell is masked, and face neighbor exists and is an unmasked leaf,
          // generate face, breaking ties at same level. This ensures that faces
          // between unmasked and masked cells will be generated once and only once.
          if ( treeN && leafN && levelN < level && ! maskedN )
          {
            // Generate face with corresponding normal and offset
            this->AddFace2( inId, ?, superCursor->GetOrigin(), superCursor->GetSize(),
                           VonNeumannOffsets3D[c], VonNeumannOrientations3D[c] );
          }
        } // if ( superCursor->IsMasked() )
        else
        {
          // If cell is unmasked, and face neighbor is a masked leaf, or no
          // such neighbor exists, generate face except in some interface cases.
          bool addFace = ! treeN || ( leafN && maskedN );
          bool create = true;
          if ( ! addFace && type < 2. )
          {
            // Mixed cells must be handled but not created
            create  = false;
            addFace = true;
          }
          if ( ( ! addFace || ! create ) && this->HasInterface && leafN )
          {
            // Face must be created if neighbor is a mixed cell
            create  = this->Intercepts->GetComponent( idN, 2 ) < 2.;
            addFace |= create;
          }
          if ( addFace )
          {
            // Generate or handle face with corresponding normal and offset
            this->AddFace2( inId, ?, superCursor->GetOrigin(), superCursor->GetSize(),
                           VonNeumannOffsets3D[c], VonNeumannOrientations3D[c],
                           create );
          } // if ( addFace )
        } // else
    */
  } // c

  // Handle interfaces separately
  if (this->HasInterface)
  {
    // Create face A when its edges are present
    vtkIdType nA = this->FacesA->GetNumberOfTuples();
    if (nA > 0)
    {
      this->FaceIDs->Reset();
      vtkIdType i0 = 0;
      vtkIdType edge0[2];
      this->FacesA->GetTypedTuple(i0, edge0);
      this->FaceIDs->InsertNextId(this->EdgesA[edge0[1]]);
      while (edge0[0] != edge0[1])
      {
        // Iterate over edges of face A
        for (vtkIdType i = 0; i < nA; ++i)
        {
          // Seek next edge then break out from loop
          vtkIdType edge[2];
          this->FacesA->GetTypedTuple(i, edge);
          if (i0 != i)
          {
            if (edge[0] == edge0[1])
            {
              edge0[1] = edge[1];
              i0 = i;
              break;
            }
            if (edge[1] == edge0[1])
            {
              edge0[1] = edge[0];
              i0 = i;
              break;
            }
          } // if ( i != i0 )
        }   // nA
        this->FaceIDs->InsertNextId(this->EdgesA[edge0[1]]);
      } // while ( edge0[0] != edge0[1] )

      // Create new face
      vtkIdType outId = this->Cells->InsertNextCell(this->FaceIDs);

      // Copy face data from that of the cell from which it comes
      this->OutData->CopyData(this->InData, inId, outId);
    } // if ( nA > 0 )

    // Create face B when its vertices are present
    vtkIdType nB = this->FacesB->GetNumberOfTuples();
    if (nB > 0)
    {
      this->FaceIDs->Reset();
      int i0 = 0;
      vtkIdType edge0[2];
      this->FacesB->GetTypedTuple(i0, edge0);
      this->FaceIDs->InsertNextId(this->EdgesB[edge0[1]]);
      while (edge0[0] != edge0[1])
      {
        // Iterate over faces B
        for (vtkIdType i = 0; i < nB; ++i)
        {
          // Seek next edge then break out from loop
          vtkIdType edge[2];
          this->FacesB->GetTypedTuple(i, edge);
          if (i0 != i)
          {
            if (edge[0] == edge0[1])
            {
              edge0[1] = edge[1];
              i0 = i;
              break;
            }
            if (edge[1] == edge0[1])
            {
              edge0[1] = edge[0];
              i0 = i;
              break;
            }
          } // if ( i0 != i )
        }   // nB
        this->FaceIDs->InsertNextId(this->EdgesB[edge0[1]]);
      } // while ( edge0[0] != edge0[1] )

      // Create new face
      vtkIdType outId = this->Cells->InsertNextCell(this->FaceIDs);

      // Copy face data from that of the cell from which it comes
      this->OutData->CopyData(this->InData, inId, outId);
    } // if ( nB > 0 )
  }   // if ( this->HasInterface )
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::AddFace(vtkIdType useId, const double* origin, const double* size,
  unsigned int offset, unsigned int orientation, unsigned char hideEdge)
{
  // Reading edge flag encoded in binary, each bit corresponding to an edge of the constructed face.
  this->EdgeFlags->InsertNextValue((hideEdge & 4) != 0);
  this->EdgeFlags->InsertNextValue((hideEdge & 2) != 0);
  this->EdgeFlags->InsertNextValue((hideEdge & 8) != 0);
  this->EdgeFlags->InsertNextValue((hideEdge & 1) != 0);

  double pt[] = { 0., 0., 0. };

  // Storage for face vertex IDs
  vtkIdType ids[4];

  // First cell vertex is always at origin of cursor
  memcpy(pt, origin, 3 * sizeof(double));

  if (this->Locator)
  {
    if (offset)
    {
      // Offset point coordinate as needed
      pt[orientation] += size[orientation];
    }
    this->Locator->InsertUniquePoint(pt, ids[0]);
    // Create other face vertices depending on orientation
    unsigned int axis1 = orientation ? 0 : 1;
    unsigned int axis2 = orientation == 2 ? 1 : 2;
    pt[axis1] += size[axis1];
    this->Locator->InsertUniquePoint(pt, ids[1]);
    pt[axis2] += size[axis2];
    this->Locator->InsertUniquePoint(pt, ids[2]);
    pt[axis1] = origin[axis1];
    this->Locator->InsertUniquePoint(pt, ids[3]);
  }
  else
  {
    if (offset)
    {
      // Offset point coordinate as needed
      pt[orientation] += size[orientation];
    }
    ids[0] = this->Points->InsertNextPoint(pt);
#ifdef TRACE
    cerr << "Point #" << ids[0] << " : ";
    for (unsigned int ipt = 0; ipt < 3; ++ipt)
    {
      cerr << pt[ipt] << " ";
    }
    cerr << std::endl;
#endif
    // Create other face vertices depending on orientation
    unsigned int axis1 = (orientation + 1) % 3;
    unsigned int axis2 = (orientation + 2) % 3;
    pt[axis1] += size[axis1];
    ids[1] = this->Points->InsertNextPoint(pt);
#ifdef TRACE
    cerr << "Point #" << ids[1] << " : ";
    for (unsigned int ipt = 0; ipt < 3; ++ipt)
    {
      cerr << pt[ipt] << " ";
    }
    cerr << std::endl;
#endif
    pt[axis2] += size[axis2];
    ids[2] = this->Points->InsertNextPoint(pt);
#ifdef TRACE
    cerr << "Point #" << ids[2] << " : ";
    for (unsigned int ipt = 0; ipt < 3; ++ipt)
    {
      cerr << pt[ipt] << " ";
    }
    cerr << std::endl;
#endif
    pt[axis1] = origin[axis1];
    ids[3] = this->Points->InsertNextPoint(pt);
#ifdef TRACE
    cerr << "Point #" << ids[3] << " : ";
    for (unsigned int ipt = 0; ipt < 3; ++ipt)
    {
      cerr << pt[ipt] << " ";
    }
    cerr << std::endl;
    cerr << "Face #" << this->Cells->GetNumberOfCells() << " : ";
    for (unsigned int ipt = 0; ipt < 3; ++ipt)
    {
      cerr << ids[ipt] << " ";
    }
    cerr << std::endl;
#endif
  }

  // Insert next face
  vtkIdType outId = this->Cells->InsertNextCell(4, ids);

  // Copy face data from that of the cell from which it comes
  this->OutData->CopyData(this->InData, useId, outId);
}
//----------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::AddFace2(vtkIdType inId, vtkIdType useId, const double* origin,
  const double* size, unsigned int offset, unsigned int orientation, bool create)
{
  // First cell vertex is always at origin of cursor
  double pt[3];
  memcpy(pt, origin, 3 * sizeof(double));
  if (offset)
  {
    // Offset point coordinate as needed
    pt[orientation] += size[orientation];
  }

  // Storage for face vertex IDs
  vtkIdType ids[4];
  unsigned int nPts = 4;

  // Keep track of face axes
  unsigned int axis1 = orientation ? 0 : 1;
  unsigned int axis2 = orientation == 2 ? 1 : 2;

  if (this->HasInterface)
  {
    // Retrieve intercept tuple and type
    double* inter = this->Intercepts->GetTuple(inId);
    double type = inter[2];

    // Distinguish cases depending on intercept type
    if (type < 2)
    {
      // Create interface intersection points
      this->FacePoints->SetPoint(0, pt);
      pt[axis1] += size[axis1];
      this->FacePoints->SetPoint(1, pt);
      pt[axis2] += size[axis2];
      this->FacePoints->SetPoint(2, pt);
      pt[axis1] = origin[axis1];
      this->FacePoints->SetPoint(3, pt);

      // Create interface intersection faces
      double coordsA[3];
      double* normal = this->Normals->GetTuple(inId);
      for (vtkIdType pId = 0; pId < 4; ++pId)
      {
        // Retrieve vertex coordinates
        this->FacePoints->GetPoint(pId, coordsA);

        // Set face scalars
        if (type != 1.)
        {
          double val =
            inter[0] + normal[0] * coordsA[0] + normal[1] * coordsA[1] + normal[2] * coordsA[2];
          this->FaceScalarsA->SetTuple1(pId, val);
        } // if ( type != 1. )
        if (type != -1.)
        {
          double val =
            inter[1] + normal[0] * coordsA[0] + normal[1] * coordsA[1] + normal[2] * coordsA[2];
          this->FaceScalarsB->SetTuple1(pId, val);
        } // if ( type != -1. )
      }   // p

      // Storage for points
      double coordsB[3];
      double coordsC[3];
      nPts = 0;

      // Distinguish between relevant types
      if (type == 1)
      {
        // Take negative values in B
        vtkIdType pair[2];
        unsigned int indPair = 0;

        // Loop over face vertices
        for (int p = 0; p < 4; ++p)
        {
          // Retrieve vertex coordinates
          this->FacePoints->GetPoint(p, coordsA);

          // Retrieve vertex scalars
          double A = this->FaceScalarsB->GetTuple1(p);
          double B = this->FaceScalarsB->GetTuple1((p + 1) % 4);

          // Add point when necessary
          if (create && A <= 0.)
          {
            ids[nPts] = this->Points->InsertNextPoint(coordsA);
            ++nPts;
          }
          if (A * B < 0)
          {
            unsigned int i = EdgeIndices[orientation][offset][p];
            if (this->EdgesB[i] == -1)
            {
              // Compute barycenter of A and B
              this->FacePoints->GetPoint((p + 1) % 4, coordsB);
              for (int j = 0; j < 3; ++j)
              {
                coordsC[j] = (B * coordsA[j] - A * coordsB[j]) / (B - A);
              }
              this->EdgesB[i] = this->Points->InsertNextPoint(coordsC);
            } // if ( this->EdgesB[i] == -1 )

            // Update points
            ids[nPts] = this->EdgesB[i];
            ++nPts;
            if (indPair)
            {
              pair[1] = i;
            }
            else
            {
              pair[0] = i;
            };
            ++indPair;
          } // if ( A * B < 0 )
        }   // p

        // Insert pair only if it makes sense
        if (indPair == 2)
        {
          this->FacesB->InsertNextTypedTuple(pair);
        }
      } // if (type = 1 )

      else if (!type)
      {
        // Take positive values in A
        vtkIdType pairA[2];
        unsigned int indPairA = 0;

        // Take negative values in B
        vtkIdType pairB[2];
        unsigned int indPairB = 0;

        // Loop over face vertices
        for (int p = 0; p < 4; ++p)
        {
          // Retrieve vertex coordinates
          this->FacePoints->GetPoint(p, coordsA);

          // Retrieve vertex scalars
          double A1 = this->FaceScalarsA->GetTuple1(p);
          double B1 = this->FaceScalarsA->GetTuple1((p + 1) % 4);
          double A2 = this->FaceScalarsB->GetTuple1(p);
          double B2 = this->FaceScalarsB->GetTuple1((p + 1) % 4);

          // Add point when necessary
          if (create && A1 >= 0. && A2 <= 0.)
          {
            ids[nPts] = this->Points->InsertNextPoint(coordsA);
            ++nPts;
          }
          if (A1 < 0. && A1 * B1 < 0.)
          {
            unsigned int i = EdgeIndices[orientation][offset][p];
            if (this->EdgesA[i] == -1)
            {
              // Compute barycenter of A and B
              this->FacePoints->GetPoint((p + 1) % 4, coordsB);
              for (int j = 0; j < 3; ++j)
              {
                coordsC[j] = (B1 * coordsA[j] - A1 * coordsB[j]) / (B1 - A1);
              }
              this->EdgesA[i] = this->Points->InsertNextPoint(coordsC);
            } // if ( this->EdgesB[i] == -1 )

            // Update points
            ids[nPts] = this->EdgesA[i];
            ++nPts;
            if (indPairA)
            {
              pairA[1] = i;
            }
            else
            {
              pairA[0] = i;
            };
            ++indPairA;
          } // if ( A1 < 0. && A1 * B1 < 0. )
          if (A2 * B2 < 0.)
          {
            unsigned int i = EdgeIndices[orientation][offset][p];
            if (this->EdgesA[i] == -1)
            {
              // Compute barycenter of A and B
              this->FacePoints->GetPoint((p + 1) % 4, coordsB);
              for (int j = 0; j < 3; ++j)
              {
                coordsC[j] = (B2 * coordsA[j] - A2 * coordsB[j]) / (B2 - A2);
              }
              this->EdgesB[i] = this->Points->InsertNextPoint(coordsC);
            } // if ( this->EdgesA[i] == -1 )

            // Update points
            ids[nPts] = this->EdgesB[i];
            ++nPts;
            if (indPairB)
            {
              pairB[1] = i;
            }
            else
            {
              pairB[0] = i;
            };
            ++indPairB;
          } // if ( A2 * B2 < 0. )
          if (A1 > 0. && A1 * B1 < 0.)
          {
            unsigned int i = EdgeIndices[orientation][offset][p];
            if (this->EdgesA[i] == -1)
            {
              // Compute barycenter of A and B
              this->FacePoints->GetPoint((p + 1) % 4, coordsB);
              for (int j = 0; j < 3; ++j)
              {
                coordsC[j] = (B1 * coordsA[j] - A1 * coordsB[j]) / (B1 - A1);
              }
              this->EdgesA[i] = this->Points->InsertNextPoint(coordsC);
            } // if ( this->EdgesA[i] == -1 )

            // Update points
            ids[nPts] = this->EdgesA[i];
            ++nPts;
            if (indPairA)
            {
              pairA[1] = i;
            }
            else
            {
              pairA[0] = i;
            };
            ++indPairA;
          } // if ( A1 > 0. && A1 * B1 < 0. )
        }   // p

        // Insert pairs only if it makes sense
        if (indPairA == 2)
        {
          this->FacesA->InsertNextTypedTuple(pairA);
        }
        if (indPairB == 2)
        {
          this->FacesB->InsertNextTypedTuple(pairB);
        }
      } // else if( ! type )
      else if (type == -1.)
      {
        // Take positive values in A
        vtkIdType pair[] = { -1, -1 };
        unsigned int indPair = 0;

        // Loop over face vertices
        for (int p = 0; p < 4; ++p)
        {
          // Retrieve vertex coordinates
          this->FacePoints->GetPoint(p, coordsA);

          // Retrieve vertex scalars
          double A = this->FaceScalarsA->GetTuple1(p);
          double B = this->FaceScalarsA->GetTuple1((p + 1) % 4);

          // Add point when necessary
          if (create && A >= 0.)
          {
            ids[nPts] = this->Points->InsertNextPoint(coordsA);
            ++nPts;
          }
          if (A * B < 0.)
          {
            unsigned int i = EdgeIndices[orientation][offset][p];
            if (this->EdgesA[i] == -1)
            {
              // Compute barycenter of A and B
              this->FacePoints->GetPoint((p + 1) % 4, coordsB);
              for (int j = 0; j < 3; ++j)
              {
                coordsC[j] = (B * coordsA[j] - A * coordsB[j]) / (B - A);
              }
              this->EdgesA[i] = this->Points->InsertNextPoint(coordsC);
            } // if ( this->EdgesB[i] == -1 )

            // Update points
            ids[nPts] = this->EdgesA[i];
            ++nPts;
            if (indPair)
            {
              pair[1] = i;
            }
            else
            {
              pair[0] = i;
            };
            ++indPair;
          } // if ( A * B < 0. )
        }   // p

        // Insert pair only if it makes sense
        if (indPair == 2)
        {
          this->FacesA->InsertNextTypedTuple(pair);
        }
      } // else if ( type == -1. )
    }   // if ( type < 2 )
    else
    {
      // Create quadrangle vertices depending on orientation
      ids[0] = this->Points->InsertNextPoint(pt);
      pt[axis1] += size[axis1];
      ids[1] = this->Points->InsertNextPoint(pt);
      pt[axis2] += size[axis2];
      ids[2] = this->Points->InsertNextPoint(pt);
      pt[axis1] = origin[axis1];
      ids[3] = this->Points->InsertNextPoint(pt);
    } // else
  }   // if ( this->HasInterface )
  else
  {
    // Create quadrangle vertices depending on orientation
    ids[0] = this->Points->InsertNextPoint(pt);
    pt[axis1] += size[axis1];
    ids[1] = this->Points->InsertNextPoint(pt);
    pt[axis2] += size[axis2];
    ids[2] = this->Points->InsertNextPoint(pt);
    pt[axis1] = origin[axis1];
    ids[3] = this->Points->InsertNextPoint(pt);
  } // else

  // Insert next face if needed
  if (create)
  {
    // Create cell and corresponding ID
    vtkIdType outId = this->Cells->InsertNextCell(nPts, ids);

    // Copy face data from that of the cell from which it comes
    this->OutData->CopyData(this->InData, useId, outId);
  } // if ( create )
}
