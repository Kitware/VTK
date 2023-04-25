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
#include "vtkHyperTreeGridGeometryInternal3D.h"

VTK_ABI_NAMESPACE_BEGIN

#include "vtkBitArray.h"
#include "vtkCellArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedVonNeumannSuperCursor.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkUnsignedCharArray.h"

#include <map>
#include <set>
#include <vector>

static constexpr unsigned int EdgeIndices[3][2][4] = { { { 3, 11, 7, 8 }, { 1, 10, 5, 9 } },
  { { 0, 9, 4, 8 }, { 2, 10, 6, 11 } }, { { 0, 1, 2, 3 }, { 4, 5, 6, 7 } } };

constexpr unsigned char FULL_WORK_FACES =
  std::numeric_limits<unsigned char>::max(); // apport de PPP

//------------------------------------------------------------------------------
vtkHyperTreeGridGeometry::vtkInternal3D::vtkInternal3D(std::string _trace, bool _merging_points,
  vtkHyperTreeGrid* _input, vtkPolyData* _output, vtkPoints* _outputPoints,
  vtkCellArray* _outputCells, vtkDataSetAttributes* _inputCellDataAttributes,
  vtkDataSetAttributes* _outputCellDataAttributes, bool _passThroughCellIds,
  const std::string& _originalCellIdArrayName)
  : vtkInternal(_trace, _merging_points, _input, _outputPoints, _outputCells,
      _inputCellDataAttributes, _outputCellDataAttributes, _passThroughCellIds,
      _originalCellIdArrayName)
{
  TRACE("vtkInternal3D")

  // Flag used to hide edges when needed
  this->m_edge_flags = vtkUnsignedCharArray::New();
  this->m_edge_flags->SetName("vtkEdgeFlags");
  this->m_edge_flags->SetNumberOfComponents(1);

  vtkPointData* outPointData = _output->GetPointData();
  outPointData->AddArray(this->m_edge_flags);
  outPointData->SetActiveAttribute(this->m_edge_flags->GetName(), vtkDataSetAttributes::EDGEFLAG);

  this->m_branch_factor = static_cast<int>(_input->GetBranchFactor());

  // Retrieve pure material mask
  // Lorsque puremask est a 0, alors il est inutile de traiter les cellules qui se trouvent a
  // l'interieur de la decomposition.
  //  FAUX Comme toutes les  cellules feuilles non masquee et non ghosted sont  puremask = 0,
  //  il n'est pas necssaire de differencier l'etat cellules feuilles.
  // on differencie donc le cas leaf, le cas coarse pure et le cas coarse non pure
  // L'utilisation de puremask permet de ne pas traiter toutes les cellules mais juste celle sur
  // un bord Cette methode de l'HyperTreeGrid construit un tableau de boolean en si 0 alors la
  // cellule n'est pas masquee, elle est pure
  // - si la cellule est masquee alors puremask = 1
  // - si la cellule n'est pas masquee mais contient une description d'interface (normale != 0)
  //   alors puremask = 1
  // - la notion de pure remonte a la cellule coarse suivant le critere suivant :
  //       - puremask = 1 si une cellule fille l'est
  //       - puremask = 0 si aucune des cellules filles, petites filles l'est
  this->m_inPureMaskArray = _input->GetPureMask();

  // initialize iterator on HypterTrees (HT) of an HyperTreeGrid (HTG)
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  this->m_input->InitializeTreeIterator(it);
  // index of current HT
  vtkIdType HT_index;
  // non oriented geometry cursor describe one cell on HT
  this->m_cursor = vtkHyperTreeGridNonOrientedVonNeumannSuperCursor::New();

  if (this->m_hasInterface)
  {
    TRACE("HASINTERFACE TRUE")
  }
  else
  {
    TRACE("HASINTERFACE FALSE")
  }

  // traversal on HTG for describe a current HT
  while (it.GetNextTree(HT_index))
  {
    TRACE("HT_index:" << HT_index)

    // initialize cursor on first cell (root)  of current HT
    this->m_input->InitializeNonOrientedVonNeumannSuperCursor(this->m_cursor, HT_index);

    this->m_number_of_children = this->m_cursor->GetNumberOfChildren();

    // traversal recursively
    this->recursivelyProcessTree(FULL_WORK_FACES);
  } // it
  TRACE("Finish")
  this->m_edge_flags->Delete();
  this->m_edge_flags = nullptr;
  this->m_cursor->Delete();
  this->m_cursor = nullptr;
  // finish trace
  this->finish();
}

//----------------------------------------------------------------------------------------------
vtkHyperTreeGridGeometry::vtkInternal3D::~vtkInternal3D() {}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::vtkInternal3D::recursivelyProcessTree(
  unsigned char _coarse_cell_faces_to_be_treat)
{
  // _that_work_face mask bit de ... pour octree et 27 tree
  vtkIdType inputCellIndex = this->m_cursor->GetGlobalNodeIndex();
  TRACE("recursivelyProcessTree #" << inputCellIndex)
  TRACE("recursivelyProcessTree IsLeaf " << this->m_cursor->IsLeaf())
  TRACE("recursivelyProcessTree isMaskedOrGhosted " << this->isMaskedOrGhosted(inputCellIndex))

  if (this->m_inputCellDataAttributes->GetArray("vtkCellId"))
  {
    double vtkCellId =
      this->m_inputCellDataAttributes->GetArray("vtkCellId")->GetTuple1(inputCellIndex);
    TRACE("vtkCellId#" << vtkCellId)
  }

  // FR Traitement specifique pour la maille fille centrale en raffinement 3
  // la construction d'une surface se faisant par la cellule de plus petit niveau , elle peut
  // être une cellule masked ou ghosted Create geometry output if cursor is at leaf
  if (this->m_cursor->IsLeaf() || this->isMaskedOrGhosted(inputCellIndex))
  {
    TRACE("recursivelyProcessTree isLeaf or/and isMaskedOrGhosted #" << inputCellIndex)
    // Cursor is at leaf, process it depending on its dimension
    // JBVTK9 ProcessLeaf3D2 prends en compte les interfaces... :)?

    // JB on pourrait prendre en compte ici _coarse_cell_faces_to_be_treat
    this->processLeafCell(_coarse_cell_faces_to_be_treat, inputCellIndex);
    return;
  }
  TRACE("recursivelyProcessTree m_inPureMaskArray: " << this->m_inPureMaskArray)
  // celle pure non masquee
  if (this->m_inPureMaskArray && this->m_inPureMaskArray->GetValue(inputCellIndex) == 0)
  {
    // toutes les filles sont dans le materiau, ontraite qu eles filles du bord
    TRACE("recursivelyProcessTree               crt: " << this->m_inPureMaskArray->GetValue(
            inputCellIndex))
    TRACE("recursivelyProcessTree pure (PureMask=false)")
    // c'est une optimisation
    // FR La maille courante est donc pure (inPureMaskArray == false car
    // GetPureMask()->GetValue(..) retourne false)
    std::set<int> childList;

    std::vector<unsigned char> _child_cell_faces_to_be_treat(this->m_number_of_children, 0);

    // FR Toutes les filles sont dans le materiau, on traite que les filles du bord
    for (unsigned int f = 0; f < 3; ++f) // dimension
    {
      for (unsigned int o = 0; o < 2; ++o) // gauche, centre, droite
      {
        int neighborIdx = (2 * o - 1) * (f + 1);
        if ((_coarse_cell_faces_to_be_treat & (1 << (3 + neighborIdx))))
        {
          bool isValidN = this->m_cursor->HasTree(3 + neighborIdx);
          vtkIdType _inputNeighboringCellIndex = 0;
          if (isValidN)
          {
            _inputNeighboringCellIndex = this->m_cursor->GetGlobalNodeIndex(3 + neighborIdx);
          }
          if (!isValidN || this->m_inPureMaskArray->GetValue(_inputNeighboringCellIndex))
          {
            // FR La maille voisine n'existe pas ou
            // si elle n'est pas pure (PureMask->GetValue(id)
            // retourne false) FR alors definition  Fille du bord
            int iMin = (f == 0 && o == 1) ? this->m_branch_factor - 1 : 0;
            int iMax = (f == 0 && o == 0) ? 1 : this->m_branch_factor;
            int jMin = (f == 1 && o == 1) ? this->m_branch_factor - 1 : 0;
            int jMax = (f == 1 && o == 0) ? 1 : this->m_branch_factor;
            int kMin = (f == 2 && o == 1) ? this->m_branch_factor - 1 : 0;
            int kMax = (f == 2 && o == 0) ? 1 : this->m_branch_factor;
            for (int i = iMin; i < iMax; ++i)
            {
              for (int j = jMin; j < jMax; ++j)
              {
                for (int k = kMin; k < kMax; ++k)
                {
                  unsigned int ichild = i + this->m_branch_factor * (j + this->m_branch_factor * k);

                  // FR Les mailles de coin peuvent etre sollicitees plusieurs fois suivant
                  // chacune des faces
                  childList.insert(ichild);
                  _child_cell_faces_to_be_treat[ichild] |= (1 << (3 + neighborIdx));
                } // k
              }   // j
            }     // i
          }       // if ...
        }
      } // o
    }   // f
    for (std::set<int>::iterator it = childList.begin(); it != childList.end(); ++it)
    {
      this->m_cursor->ToChild(*it);
      this->recursivelyProcessTree(_child_cell_faces_to_be_treat[*it]);
      this->m_cursor->ToParent();
    } // ichild
    return;
  }
  TRACE("recursivelyProcessTree not pure mask (PureMask=true)")
  // FR Il existe une filles qui n'est pas dans le mat, on la chercher partout
  // case coarse cell
  for (unsigned int ichild = 0; ichild < this->m_number_of_children; ++ichild)
  {
    TRACE("recursivelyProcessTree coarse #" << inputCellIndex << " #" << ichild)
    this->m_cursor->ToChild(ichild);
    this->recursivelyProcessTree(FULL_WORK_FACES);
    this->m_cursor->ToParent();
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::vtkInternal3D::processLeafCell(
  unsigned char _coarse_cell_faces_to_be_treat, vtkIdType _inputCellIndex)
{
  this->extractCellInterface(_inputCellIndex);

  // Cursor is at leaf, process it depending on its dimension
  // JBVTK9 ProcessLeaf3D2 prends en compte les interfaces... :)?
  // Cell at cursor center is a leaf, retrieve its global index, and mask
  unsigned level = this->m_cursor->GetLevel();
  bool masked = this->m_cursor->IsMasked();
  vtkHyperTree* tree = this->m_cursor->GetTree();
  const double* cell_origin = this->m_cursor->GetOrigin();
  const double* cell_size = this->m_cursor->GetSize();

  bool trace_htg_geometry_forced = false;
  if (__trace_htg_env_vtkcellid == -2)
  {
    if (getenv("vtkCellId"))
    {
      __trace_htg_env_vtkcellid = std::stod(getenv("vtkCellId"));
    }
    else
    {
      __trace_htg_env_vtkcellid = -1;
    }
  }
  if (__trace_htg_env_vtkcellid >= 0 && this->m_inputCellDataAttributes->GetArray("vtkCellId"))
  {
    double vtkCellId =
      this->m_inputCellDataAttributes->GetArray("vtkCellId")->GetTuple1(_inputCellIndex);
    if (__trace_htg_env_vtkcellid == vtkCellId)
    {
      if (!__trace_htg_geometry)
      {
        __trace_htg_geometry = true;
        trace_htg_geometry_forced = true;
      }
      TRACE("Finded vtkCellId#" << vtkCellId)
      TRACE("> Origin     [" << cell_origin[0] << " ; " << cell_origin[1] << " ; " << cell_origin[2]
                             << "]")
      TRACE(
        "> Size       [" << cell_size[0] << " ; " << cell_size[1] << " ; " << cell_size[2] << "]")
      if (this->hasInterfaceOnThisCell())
      {
        if (this->getInterfaceTypeOnThisCell() != 1.)
        {
          TRACE("> Distance A " << this->getInterfaceInterceptsA());
        }
        if (this->getInterfaceTypeOnThisCell() != -1.)
        {
          TRACE("> Distance B " << this->getInterfaceInterceptsB());
        }
        TRACE("> Type       " << this->getInterfaceTypeOnThisCell());
        TRACE("> Normal     [" << this->getInterfaceNormal()[0] << " ; "
                               << this->getInterfaceNormal()[1] << " ; "
                               << this->getInterfaceNormal()[2] << "]")
      }
      else
      {
        TRACE("> No interface on this cell!!!")
      }
      TRACE("")
    }
  }

  // Iterate over all cursors of Von Neumann neighborhood around center (offset=3)
  // - local offset of neighbor by VonNeumann cursor
  static constexpr unsigned int neighbor_cell_offset_list[] = { 0, 1, 2, 4, 5, 6 };
  // - Orientation describe for each face : 0 YZ, 1 XZ, 2 XY
  static constexpr unsigned int neighbor_cell_orientation[] = { 2, 1, 0, 0, 1, 2 };
  // - offsets ??
  static constexpr unsigned int neighbor_cell_front_plane_offset[] = { 0, 0, 0, 1, 1, 1 };

  TRACE("processLeafCell BEGIN")
  std::vector<Point> cell_points;
  cell_points.resize(8);

  std::vector<std::pair<Point, Point>> edge_points;
  edge_points.resize(12 + 8);

  std::map<unsigned int, std::pair<Point*, unsigned int>> internalFaceA;
  std::map<unsigned int, std::pair<Point*, unsigned int>> internalFaceB;

  for (unsigned int neighbor_offset_local = 0; neighbor_offset_local < 6; ++neighbor_offset_local)
  {
    TRACE("processLeafCell neighbor_offset_local#" << neighbor_offset_local)

    unsigned int offsetN = neighbor_cell_offset_list[neighbor_offset_local];
    unsigned int orientation = neighbor_cell_orientation[neighbor_offset_local];
    unsigned int orientation_plane_offset = neighbor_cell_front_plane_offset[neighbor_offset_local];

    // l'exploitation du masque qui etait jusqu'a maintenatn utilise que pour les coarses
    // permet une amelioration des perfs de 50% pour cette methode
    /*  if (!(_coarse_cell_faces_to_be_treat & (1 << offsetN)))
      {
        if (!this->m_hasInterfaceOnThisCell)
        {
          TRACE(">>>> NO WORK AND NOT INTERFACE IN THIS CELL")
          continue;
        }
        else
        {
          TRACE(">>>> WORK CAUSE INTERFACE IN THIS CELL")
        }
      } else {
        TRACE(">>>> WORK")
      }
      */

    // Retrieve cursor to neighbor across face
    // Retrieve tree, leaf flag, and mask of neighbor cursor
    bool leafN;
    vtkIdType _inputNeighboringCellIndex;
    unsigned int levelN;
    vtkHyperTree* treeN =
      this->m_cursor->GetInformation(offsetN, levelN, leafN, _inputNeighboringCellIndex);
    int maskedN = this->m_cursor->IsMasked(offsetN);
    bool hasInterfaceCellN = this->hasInterface(this->m_cursor->GetGlobalNodeIndex(offsetN));

    // In 3D masked and unmasked cells are handled differently:
    // . If cell is unmasked, and face neighbor is a masked leaf, or no such neighbor
    //   exists, then generate face.
    //   je rajoute ou si elle est interface
    TRACE("idxN# " << this->m_cursor->GetGlobalNodeIndex(offsetN))
    TRACE("J'ai une interface " << this->m_hasInterfaceOnThisCell)
    TRACE("Voisine a une interface " << hasInterfaceCellN)
    // . If cell is masked, and face neighbor exists and is an unmasked leaf, then
    //   generate face, breaking ties at same level. This ensures that faces between
    //   unmasked and masked cells will be generated once and only once.
    if ((!masked && (!treeN || maskedN || this->m_hasInterfaceOnThisCell || hasInterfaceCellN)) ||
      (masked && treeN && leafN && levelN <= level && !maskedN))
    {
      double boundsN[6], bounds[6];
      // If not using a flag on edges, faces that are neighbor to masked cells have unwanted
      // edges. That is because it is actually the neighbors of the coarser level that,
      // accumulate, construct this face. This flag intends to hide edges that are inside the
      // face.
      unsigned char edgeFlag = 15; // 1111 in binary, select all edge

      if (levelN != level && treeN && masked && tree)
      {
        this->m_cursor->GetBounds(bounds);
        this->m_cursor->GetBounds(offsetN, boundsN);

        edgeFlag = (static_cast<unsigned char>(vtkMathUtilities::NearlyEqual(
                     boundsN[((orientation + 1) % 3) * 2], bounds[((orientation + 1) % 3) * 2]))) |
          (static_cast<unsigned char>(vtkMathUtilities::NearlyEqual(
             boundsN[((orientation + 1) % 3) * 2 + 1], bounds[((orientation + 1) % 3) * 2 + 1]))
            << 1) |
          (static_cast<unsigned char>(vtkMathUtilities::NearlyEqual(
             boundsN[((orientation + 2) % 3) * 2], bounds[((orientation + 2) % 3) * 2]))
            << 2) |
          (static_cast<unsigned char>(vtkMathUtilities::NearlyEqual(
             boundsN[((orientation + 2) % 3) * 2 + 1], bounds[((orientation + 2) % 3) * 2 + 1]))
            << 3);
      }
      // Generate face with corresponding normal and offset
      if (!masked)
      { // pas de sens de parcours ?
        this->processLeafCellAddFace(cell_points, edge_points, neighbor_offset_local,
          _inputCellIndex, cell_origin, cell_size, orientation_plane_offset, orientation, edgeFlag,
          internalFaceA, internalFaceB);
      }
      else if (maskedN)
      { // pas d'inversion du sens de parcours ?
        this->processLeafCellAddFace(cell_points, edge_points, neighbor_offset_local,
          _inputNeighboringCellIndex, cell_origin, cell_size, orientation_plane_offset, orientation,
          edgeFlag, internalFaceA, internalFaceB);
      }
    }
  }

  TRACE("FACEA #" << internalFaceA.size())
  if (!internalFaceA.empty() && internalFaceA.size() >= 3)
  {
    this->stateInterfaceFace("A complete", internalFaceA);

    static const std::vector<std::pair<unsigned int, unsigned int>> idPtsEdge = { { 0, 1 } /*  0 */,
      { 0, 2 } /*  1 */, { 0, 4 } /* 2 */, { 1, 3 } /* 3 */, { 1, 5 } /* 4 */, { 2, 3 } /*  5 */,
      { 2, 6 } /*  6 */, { 3, 7 } /* 7 */, { 4, 5 } /* 8 */, { 4, 6 } /* 9 */, { 5, 7 } /* 10 */,
      { 6, 7 } /* 11 */ };

    TRACE("FACEA dedans")
    std::vector<vtkIdType> new_outputIndexPoints;
    unsigned int first_edge = internalFaceA.begin()->first; // get first key
    if (first_edge == VTK_DEFAULT_EDGE_INDEX)
    {
      ERROR(true, "internalFaceA first " << first_edge << "!")
    }
    else
    {
      Point* pt = internalFaceA[first_edge].first;
      TRACE("FIRST ID#" << pt->getId(this) << " first_edge:" << first_edge)
      new_outputIndexPoints.emplace_back(pt->getId(this));
      this->m_edge_flags->InsertNextValue(1);
      unsigned int next = internalFaceA[first_edge].second;
      while (next != first_edge && next != VTK_DEFAULT_EDGE_INDEX && next >= 0)
      {
        pt = internalFaceA[next].first;
        TRACE("NEXT ID#" << pt->getId(this) << "crt:" << next)
        new_outputIndexPoints.emplace_back(pt->getId(this));
        this->m_edge_flags->InsertNextValue(1);
        next = internalFaceA[next].second;
      }
      if (next == VTK_DEFAULT_EDGE_INDEX)
      {
        ERROR(true, "internalFaceA next#" << next << " is default!")
      }
      if (next < 0)
      {
        ERROR(true, "internalFaceA next#" << next << " is negatif! #####################")
      }
    }
    TRACE("FaceA add cell")
    vtkIdType outputCellIndex = this->m_outputCells->InsertNextCell(
      new_outputIndexPoints.size(), new_outputIndexPoints.data());
    // copy face data from that of the cell from which it comes
    this->m_outputCellDataAttributes->CopyData(
      this->m_inputCellDataAttributes, _inputCellIndex, outputCellIndex);
  }

  TRACE("FACEB #" << internalFaceB.size())
  if (!internalFaceB.empty() && internalFaceB.size() >= 3)
  {
    TRACE("FACEB dedans")
    std::vector<vtkIdType> new_outputIndexPoints;
    unsigned int first_edge = internalFaceB.begin()->first; // get first key
    if (first_edge == VTK_DEFAULT_EDGE_INDEX)
    {
      ERROR(true, "internalFaceB first " << first_edge << "!")
    }
    else
    {
      Point* pt = internalFaceB[first_edge].first;
      TRACE("FIRST ID#" << pt->getId(this) << " first_edge:" << first_edge)
      new_outputIndexPoints.emplace_back(pt->getId(this));
      this->m_edge_flags->InsertNextValue(1);
      unsigned int next = internalFaceB[first_edge].second;
      while (next != first_edge && next != VTK_DEFAULT_EDGE_INDEX && next >= 0)
      {
        pt = internalFaceB[next].first;
        TRACE("NEXT ID#" << pt->getId(this) << "crt:" << next)
        new_outputIndexPoints.emplace_back(pt->getId(this));
        this->m_edge_flags->InsertNextValue(1);
        next = internalFaceB[next].second;
      }
      if (next == VTK_DEFAULT_EDGE_INDEX)
      {
        ERROR(true, "internalFaceB next#" << next << " is default!")
      }
      if (next < 0)
      {
        ERROR(true, "internalFaceB next#" << next << " is negatif! #####################")
      }
    }
    TRACE("FaceB add cell")

    this->createNewCellAndCopyData(new_outputIndexPoints, _inputCellIndex);
  }
  TRACE("processLeafCell END")

  if (trace_htg_geometry_forced)
  {
    __trace_htg_geometry = false;
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::vtkInternal3D::processLeafCellAddFace(
  std::vector<Point>& _cell_points, std::vector<std::pair<Point, Point>>& _edge_points,
  unsigned int _neighbor_offset_local, vtkIdType _inputCellIndex, const double* _cell_origin,
  const double* _cell_size, unsigned int _front_plane_offset, unsigned int _orientation,
  unsigned char _hideEdge, std::map<unsigned int, std::pair<Point*, unsigned int>>& _internalFaceA,
  std::map<unsigned int, std::pair<Point*, unsigned int>>& _internalFaceB)
{
  // - offsets_des_points_de_chaque_face_dans_la_cellule
  static const std::vector<std::vector<unsigned int>> idPtFaceOnCell = { { 0, 1, 3, 2 },
    { 0, 4, 5, 1 }, { 0, 2, 6, 4 }, { 1, 3, 7, 5 }, { 2, 6, 7, 3 }, { 4, 5, 7, 6 } };

  TRACE("==============================================================")
  TRACE("processLeafCellAddFace _neighbor_offset_local#"
    << _neighbor_offset_local << " _front_plane_offset#" << _front_plane_offset << " _orientation#"
    << _orientation)

  // Compute Points
  double pt[] = { 0., 0., 0. };

  // On a deroule le code afin de ne faire que les actions utiles :
  // - soit le point a deja ete traite dans une aute cellule alors on n'a qu'a recuperer son
  // indice
  // - soit le point n'a pas ete calcule alors on en calcule les coordonnees pour
  // l'enregistrer sur une cellule de coin avec 3 faces exposées c'est 50% de gain pour cette
  // methode
  Point* crtPts = &_cell_points[idPtFaceOnCell[_neighbor_offset_local][0]];
  if (crtPts->isValid())
  {
    crtPts = &_cell_points[idPtFaceOnCell[_neighbor_offset_local][1]];
    if (crtPts->isValid())
    {
      crtPts = &_cell_points[idPtFaceOnCell[_neighbor_offset_local][2]];
      if (crtPts->isValid())
      {
        crtPts = &_cell_points[idPtFaceOnCell[_neighbor_offset_local][3]];
        if (!crtPts->isValid())
        {
          memcpy(pt, _cell_origin, 3 * sizeof(double));
          if (_front_plane_offset)
          {
            pt[_orientation] += _cell_size[_orientation];
          }
          unsigned int axis2 = (_orientation + 2) % 3;
          pt[axis2] += _cell_size[axis2];
          crtPts->setXYZ(this, pt);
        }
      }
      else
      {
        memcpy(pt, _cell_origin, 3 * sizeof(double));
        if (_front_plane_offset)
        {
          pt[_orientation] += _cell_size[_orientation];
        }
        unsigned int axis1 = (_orientation + 1) % 3;
        unsigned int axis2 = (_orientation + 2) % 3;
        pt[axis1] += _cell_size[axis1];
        pt[axis2] += _cell_size[axis2];
        crtPts->setXYZ(this, pt);
        crtPts = &_cell_points[idPtFaceOnCell[_neighbor_offset_local][3]];
        if (!crtPts->isValid())
        {
          pt[axis1] = _cell_origin[axis1];
          crtPts->setXYZ(this, pt);
        }
      }
    }
    else
    {
      memcpy(pt, _cell_origin, 3 * sizeof(double));
      if (_front_plane_offset)
      {
        // Offset point coordinate as needed
        pt[_orientation] += _cell_size[_orientation];
      }
      unsigned int axis1 = (_orientation + 1) % 3;
      pt[axis1] += _cell_size[axis1];
      crtPts->setXYZ(this, pt);
      crtPts = &_cell_points[idPtFaceOnCell[_neighbor_offset_local][2]];
      if (crtPts->isValid())
      {
        crtPts = &_cell_points[idPtFaceOnCell[_neighbor_offset_local][3]];
        if (!crtPts->isValid())
        {
          unsigned int axis2 = (_orientation + 2) % 3;
          pt[axis2] += _cell_size[axis2];
          crtPts->setXYZ(this, pt);
        }
      }
      else
      {
        unsigned int axis2 = (_orientation + 2) % 3;
        pt[axis2] += _cell_size[axis2];
        crtPts->setXYZ(this, pt);
        crtPts = &_cell_points[idPtFaceOnCell[_neighbor_offset_local][3]];
        if (!crtPts->isValid())
        {
          pt[axis1] = _cell_origin[axis1];
          crtPts->setXYZ(this, pt);
        }
      }
    }
  }
  else
  {
    memcpy(pt, _cell_origin, 3 * sizeof(double));
    if (_front_plane_offset)
    {
      // Offset point coordinate as needed
      pt[_orientation] += _cell_size[_orientation];
    }
    crtPts->setXYZ(this, pt);
    crtPts = &_cell_points[idPtFaceOnCell[_neighbor_offset_local][1]];
    if (crtPts->isValid())
    {
      crtPts = &_cell_points[idPtFaceOnCell[_neighbor_offset_local][2]];
      if (crtPts->isValid())
      {
        crtPts = &_cell_points[idPtFaceOnCell[_neighbor_offset_local][3]];
        if (!crtPts->isValid())
        {
          unsigned int axis2 = (_orientation + 2) % 3;
          pt[axis2] += _cell_size[axis2];
          crtPts->setXYZ(this, pt);
        }
      }
      else
      {
        unsigned int axis1 = (_orientation + 1) % 3;
        unsigned int axis2 = (_orientation + 2) % 3;
        pt[axis1] += _cell_size[axis1];
        pt[axis2] += _cell_size[axis2];
        crtPts->setXYZ(this, pt);
        crtPts = &_cell_points[idPtFaceOnCell[_neighbor_offset_local][3]];
        if (!crtPts->isValid())
        {
          pt[axis1] = _cell_origin[axis1];
          crtPts->setXYZ(this, pt);
        }
      }
    }
    else
    {
      unsigned int axis1 = (_orientation + 1) % 3;
      pt[axis1] += _cell_size[axis1];
      crtPts->setXYZ(this, pt);
      crtPts = &_cell_points[idPtFaceOnCell[_neighbor_offset_local][2]];
      if (crtPts->isValid())
      {
        crtPts = &_cell_points[idPtFaceOnCell[_neighbor_offset_local][3]];
        if (!crtPts->isValid())
        {
          unsigned int axis2 = (_orientation + 2) % 3;
          pt[axis2] += _cell_size[axis2];
          crtPts->setXYZ(this, pt);
        }
      }
      else
      {
        unsigned int axis2 = (_orientation + 2) % 3;
        pt[axis2] += _cell_size[axis2];
        crtPts->setXYZ(this, pt);
        crtPts = &_cell_points[idPtFaceOnCell[_neighbor_offset_local][3]];
        if (!crtPts->isValid())
        {
          pt[axis1] = _cell_origin[axis1];
          crtPts->setXYZ(this, pt);
        }
      }
    }
  }

  // Compute edges
  static const std::vector<std::vector<std::pair<unsigned int, unsigned int>>>
    idPtEdgeFaceOnCell = { { { 0, 1 }, { 1, 3 }, { 3, 2 }, { 2, 0 } },
      { { 0, 4 }, { 4, 5 }, { 5, 1 }, { 1, 0 } }, { { 0, 2 }, { 2, 6 }, { 6, 4 }, { 4, 0 } },
      { { 1, 3 }, { 3, 7 }, { 7, 5 }, { 5, 1 } }, { { 2, 6 }, { 6, 7 }, { 7, 3 }, { 3, 2 } },
      { { 4, 5 }, { 5, 7 }, { 7, 6 }, { 6, 4 } } };

  static const std::vector<std::pair<unsigned int, unsigned int>> idPtsEdge = { { 0, 1 } /*  0 */,
    { 0, 2 } /*  1 */, { 0, 4 } /* 2 */, { 1, 3 } /* 3 */, { 1, 5 } /* 4 */, { 2, 3 } /*  5 */,
    { 2, 6 } /*  6 */, { 3, 7 } /* 7 */, { 4, 5 } /* 8 */, { 4, 6 } /* 9 */, { 5, 7 } /* 10 */,
    { 6, 7 } /* 11 */ };

  // 0:X, 1:Y, 2:Z
  static const std::vector<unsigned int> axisEdge = { 0, 1, 2, 1, 2, 0, 2, 2, 0, 1, 1, 0 };

  // static const std::vector<std::vector<unsigned int>> idPtFaceOnCell = { { 0, 1, 3, 2 },
  //  { 0, 4, 5, 1 }, { 0, 2, 6, 4 }, { 1, 3, 7, 5 }, { 2, 6, 7, 3 }, { 4, 5, 7, 6 } };
  // static const std::vector<std::vector<unsigned int>> idEdgeFaceOnCell = { { 0, 3, 5, 1 },
  //   { 2, 8, 4, 0 }, { 1, 6, 9, 2 }, { 3, 7, 10, 4 }, { 6, 11, 7, 5 }, { 8, 10, 11, 9 } };
  // L'ordre de parcours des aretes est tres important... on doit d'abord definir les aretes
  // qui sont dans le bon sens (sens des points tels que défini dans idPtsEdge) puis
  // les deux aretes qui sont dans le sens oppose
  // ceci est exploite un peu plus loin par
  //        if (iEdge < 2)
  // qui permet d'identifier le parcours de l'arete ou le parcours inverse de l'arete
  static const std::vector<std::vector<unsigned int>> idEdgeFaceOnCell = { { 1, 5, 3, 0 },
    { 0, 4, 8, 2 }, { 2, 9, 6, 1 }, { 3, 7, 10, 4 }, { 6, 11, 7, 5 }, { 8, 10, 11, 9 } };

  // Storage for new face vertex IDs
  std::vector<vtkIdType> outputIndexPoints;

  unsigned int crtEdgePointA = VTK_DEFAULT_EDGE_INDEX;
  vtkIdType lastId = -1;
  unsigned int crtEdgePointB = VTK_DEFAULT_EDGE_INDEX;
  bool first = true;
  for (unsigned int iEdge = 0; iEdge < 4; ++iEdge)
  {
    TRACE("--------------------------------------------------------------")
    TRACE("Edge #" << iEdge)
    // pour test
    {
      unsigned int iEdgeCell = idEdgeFaceOnCell[_neighbor_offset_local][iEdge];
      std::pair<unsigned int, unsigned int> edge = idPtsEdge[iEdgeCell];
      TRACE("Edge #" << iEdge << " " << edge.first << " " << edge.second)
      // point dans la cellule ?
      bool checkFirst = false;
      bool checkSecond = false;
      for (unsigned int iPt = 0; iPt < 4; ++iPt)
      {
        TRACE("Point #" << iPt << " #" << idPtFaceOnCell[_neighbor_offset_local][iPt])
        if (edge.first == idPtFaceOnCell[_neighbor_offset_local][iPt])
        {
          TRACE("   Finded " << edge.first)
          checkFirst = true;
          if (checkSecond)
          {
            break;
          }
        }
        if (edge.second == idPtFaceOnCell[_neighbor_offset_local][iPt])
        {
          TRACE("   Finded " << edge.second)
          checkSecond = true;
          if (checkFirst)
          {
            break;
          }
        }
      }
      ERROR(!checkFirst,
        "processLeafCellAddFace BAD neigh#" << _neighbor_offset_local << " iEdge#" << iEdge
                                            << " first")
      ERROR(!checkSecond,
        "processLeafCellAddFace BAD neigh#" << _neighbor_offset_local << " iEdge#" << iEdge
                                            << " second")
    }
    //
    {
      unsigned int iEdgeCell = idEdgeFaceOnCell[_neighbor_offset_local][iEdge];
      TRACE("iEdgeCell#" << iEdgeCell)
      std::pair<unsigned int, unsigned int> iPts = idPtsEdge[iEdgeCell];
      TRACE("iPts#" << iPts.first << " " << iPts.second)
      TRACE("call computeEdge begin")
      _cell_points[iPts.first].computeEdge(this, _cell_points[iPts.second], _edge_points,
        axisEdge[iEdgeCell], iEdgeCell, _internalFaceA, _internalFaceB, crtEdgePointA,
        crtEdgePointB);
      TRACE("call computeEdge end")

      // la face (quad) de la cellule (cube ou pave) est defini de telle facon que
      // le sens de parcours est bon pour les deux premieres aretes et qu'il est
      // inverse pour les deux suivantes
      std::vector<Point*> points;
      if (iEdge < 2)
      {
        points.emplace_back(&_cell_points[iPts.first]);       // first vertex quad face
        points.emplace_back(&_edge_points[iEdgeCell].first);  // first point one edge
        points.emplace_back(&_edge_points[iEdgeCell].second); // second point one edge
      }
      else
      {
        points.emplace_back(&_cell_points[iPts.second]);      // second vertex quad face
        points.emplace_back(&_edge_points[iEdgeCell].second); // second point one edge
        points.emplace_back(&_edge_points[iEdgeCell].first);  // first point one edge
      }
      for (auto point : points)
      {
        if (point->isValid())
        {
          vtkIdType point_id = point->in(this);
          // si le point a un index defini c'est qu'il est digne d'interet
          // le lastId est la pour eviter les repetitions
          TRACE("lastId#" << lastId)
          if (point_id >= 0 && point_id != lastId)
          {
            TRACE(">>>>> NEW POINT ON SUB-FACE #"
              << _neighbor_offset_local << " isValid#1 idPt#" << point_id << " ["
              << point->getXYZ()[0] << " ; " << point->getXYZ()[1] << " ; " << point->getXYZ()[2]
              << "]")
            TRACE("")
            outputIndexPoints.emplace_back(point_id);
            lastId = point_id;
            if (!first)
            {
              this->m_edge_flags->InsertNextValue(1);
            }
            first = false;
          }
        }
      }
    }
  }
  // insert new face
  if (outputIndexPoints.size() > 2)
  {
    this->createNewCellAndCopyData(outputIndexPoints, _inputCellIndex);
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::vtkInternal3D::setInterfaceFace(
  std::vector<std::pair<vtkHyperTreeGridGeometry::vtkInternal3D::Point,
    vtkHyperTreeGridGeometry::vtkInternal3D::Point>>& _edge_points,
  unsigned int _iEdgeCell,
  std::map<unsigned int, std::pair<vtkHyperTreeGridGeometry::vtkInternal3D::Point*, unsigned int>>&
    _internalFace,
  vtkHyperTreeGridGeometry::vtkInternal3D::Point* _pt)
{
  this->stateInterfaceFace(" ( setInterfaceFace AVT)", _internalFace);

  TRACE("setInterfaceFace pointA valid#" << _pt->isValid() << " [" << _pt->getXYZ()[0] << " ; "
                                         << _pt->getXYZ()[1] << " ; " << _pt->getXYZ()[2] << "] id#"
                                         << _pt->getId(this))

  TRACE("_iEdgeCell#" << _iEdgeCell << " count#" << _internalFace.count(_iEdgeCell))

  if (_internalFace.count(_iEdgeCell) <= 0)
  {
    _internalFace[_iEdgeCell].first = _pt;
    _internalFace[_iEdgeCell].second = VTK_DEFAULT_EDGE_INDEX;
    TRACE("setInterfaceFace add internalFace " << _iEdgeCell)
  }
  else
  {
    ERROR(
      _internalFace[_iEdgeCell].first->getId(this) != _edge_points[_iEdgeCell].first.getId(this),
      "setInterfaceFace incoherence");
  }
  this->stateInterfaceFace(" ( setInterfaceFace APR)", _internalFace);
}

//------------------------------------------------------------------------------
// le point c'est juste pour acceder a des methodes
// il faut revoir cela
void vtkHyperTreeGridGeometry::vtkInternal3D::completeChainette(
  std::map<unsigned int, std::pair<vtkHyperTreeGridGeometry::vtkInternal3D::Point*, unsigned int>>&
    _internalFace,
  unsigned int _iEdgePoint1, unsigned int _iEdgePoint2)
{
  TRACE("completeChainette")
  if (_iEdgePoint1 == VTK_DEFAULT_EDGE_INDEX || _iEdgePoint2 == VTK_DEFAULT_EDGE_INDEX)
  {
    TRACE("completeChainette un des deux est EdgeDefault")
    return;
  }
  if (_iEdgePoint1 == _iEdgePoint2)
  {
    TRACE("completeChainette même arete")
    return;
  }

  // build link between _iEdgePoint1 et _iEdgePoint2
  unsigned int i1 = _internalFace[_iEdgePoint1].second;
  unsigned int i2 = _internalFace[_iEdgePoint2].second;
  TRACE("completeChainette 1 [" << _iEdgePoint1 << "] = " << i1)
  TRACE("completeChainette 2 [" << _iEdgePoint2 << "] = " << i2)
  if (i1 == VTK_DEFAULT_EDGE_INDEX)
  {
    if (i2 == VTK_DEFAULT_EDGE_INDEX)
    {
      // choix arbitraire d'un sens de chainage pour la chainette
      _internalFace[_iEdgePoint1].second = _iEdgePoint2;
      TRACE("completeChainette set [" << _iEdgePoint1 << "] = " << _iEdgePoint2)
    }
    else if (i2 == _iEdgePoint1)
    {
      // on a deja le chainage
      TRACE("completeChainette nothing")
    }
    else
    { // si i2 decrit deja une chainette differente
      _internalFace[_iEdgePoint1].second = _iEdgePoint2;
      TRACE("completeChainette set [" << _iEdgePoint1 << "] = " << _iEdgePoint2)
    }
  }
  else if (i1 == _iEdgePoint2)
  {
    // on a deja le chainage
    TRACE("completeChainette nothing")
  }
  else if (i2 == VTK_DEFAULT_EDGE_INDEX)
  {
    // choix arbitraire d'un sens de chainage pour la chainette
    _internalFace[_iEdgePoint2].second = _iEdgePoint1;
    TRACE("completeChainette set [" << _iEdgePoint2 << "] = " << _iEdgePoint1)
  }
  else if (i2 == _iEdgePoint1)
  {
    // on a deja le chainage
    TRACE("completeChainette nothing")
  }
  else
  {
    // si i1 decrit deja une chainette differente et que c'est aussi le cas de i2
    // il faut inverser une des deux chainettes et les relier
    std::vector<unsigned int> chainette;
    chainette.emplace_back(_iEdgePoint1);
    unsigned int next = _internalFace[_iEdgePoint1].second;
    while (next != VTK_DEFAULT_EDGE_INDEX)
    {
      chainette.emplace_back(next);
      next = _internalFace[next].second;
    }
    unsigned int crt = VTK_DEFAULT_EDGE_INDEX;
    for (std::vector<unsigned int>::reverse_iterator it = chainette.rbegin();
         it != chainette.rend(); ++it)
    {
      if (crt == VTK_DEFAULT_EDGE_INDEX)
      {
        crt = *it;
      }
      else
      {
        next = *it;
        _internalFace[crt].second = next;
        TRACE("completeChainette (inverse) set [" << crt << "] = " << next)
        crt = next;
      }
    }
    ERROR(crt != _iEdgePoint1, "Unexpected edge: " << crt << " instead of " << _iEdgePoint1);
    _internalFace[crt].second = _iEdgePoint2;
    TRACE("completeChainette set [" << crt << "] = " << _iEdgePoint2)
  }
}
//------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometry::vtkInternal3D::Point::computeEdgeInterface(
  vtkHyperTreeGridGeometry::vtkInternal3D* _parent, double _scalar, const Point& _endpoint,
  double _scalar_endpoint, std::vector<std::pair<Point, Point>>& _edge_points,
  unsigned int _axisEdge, unsigned int _iEdgeCell,
  std::map<unsigned int, std::pair<Point*, unsigned int>>& _internalFace, Point& _pointInter,
  unsigned int& _iEdgePoint)
{
  // local is _beginpoint on current edge
  static const std::vector<std::pair<unsigned int, unsigned int>> idPtsEdge = { { 0, 1 } /*  0 */,
    { 0, 2 } /*  1 */, { 0, 4 } /* 2 */, { 1, 3 } /* 3 */, { 1, 5 } /* 4 */, { 2, 3 } /*  5 */,
    { 2, 6 } /*  6 */, { 3, 7 } /* 7 */, { 4, 5 } /* 8 */, { 4, 6 } /* 9 */, { 5, 7 } /* 10 */,
    { 6, 7 } /* 11 */ };

  TRACE("::computeEdgeInterface iEdgeCell#" << _iEdgeCell << " (axisEdge#" << _axisEdge << ")")
  ERROR(!this->m_isValid, "FIRST is invalid Point.");
  TRACE("::computeEdgeInterface VERTEX FIRST [" << this->m_xyz[0] << " ; " << this->m_xyz[1]
                                                << " ; " << this->m_xyz[2] << "] id#" << this->m_id
                                                << " scalar#" << _scalar)
  ERROR(!_endpoint.m_isValid, "SECOND is invalid Point")
  TRACE("::computeEdgeInterface VERTEX SECOND ["
    << _endpoint.m_xyz[0] << " ; " << _endpoint.m_xyz[1] << " ; " << _endpoint.m_xyz[2] << "] id#"
    << _endpoint.m_id << " scalar#" << _scalar_endpoint)
  if (_scalar == 0.)
  {
    if (_scalar_endpoint == 0.)
    {
      TRACE("::computeEdgeInterface IS VERTEX FIRST TO SECOND")
      unsigned int iEdgePoint1 = idPtsEdge[_iEdgeCell].first + 12;
      TRACE("::computeEdgeInterface   SET EDGE#" << iEdgePoint1 << " VERTEX FIRST")
      _edge_points[iEdgePoint1].first.set(*this);
      _edge_points[iEdgePoint1].first.setIsCorner();
      _edge_points[iEdgePoint1].second.reset();
      _parent->setInterfaceFace(
        _edge_points, iEdgePoint1, _internalFace, &_edge_points[iEdgePoint1].first);
      unsigned int iEdgePoint2 = idPtsEdge[_iEdgeCell].second + 12;
      TRACE("::computeEdgeInterface   SET EDGE#" << iEdgePoint2 << " VERTEX SECOND")
      _edge_points[iEdgePoint2].first.set(_endpoint);
      _edge_points[iEdgePoint2].first.setIsCorner();
      _edge_points[iEdgePoint2].second.reset();
      _parent->setInterfaceFace(
        _edge_points, iEdgePoint2, _internalFace, &_edge_points[iEdgePoint2].first);
      TRACE("::computeEdgeInterface   BUILD between EDGE#" << iEdgePoint1 << "VERTEX and EDGE#"
                                                           << iEdgePoint2 << " VERTEX")
      _parent->completeChainette(_internalFace, iEdgePoint1, iEdgePoint2);
      return true;
    }
    TRACE("::computeEdgeInterface IS VERTEX FIRST")
    _pointInter.set(*this);
    _pointInter.setIsCorner();
    _iEdgePoint = idPtsEdge[_iEdgeCell].first + 12;
    TRACE("::computeEdgeInterface   SET EDGE#" << _iEdgePoint << " VERTEX FIRST")
  }
  else if (_scalar_endpoint == 0.)
  {
    TRACE("::computeEdgeInterface IS VERTEX SECOND")
    _pointInter.set(_endpoint);
    _pointInter.setIsCorner();
    _iEdgePoint = idPtsEdge[_iEdgeCell].second + 12;
    TRACE("::computeEdgeInterface   SET EDGE#" << _iEdgePoint << " VERTEX SECOND")
  }
  else if (_scalar * _scalar_endpoint < 0.)
  {
    TRACE("::computeEdgeInterface IS NEW POINT (axisEdge#" << _axisEdge << ")")
    double xyz[3];
    memcpy(xyz, this->m_xyz, 3 * sizeof(double));
    xyz[_axisEdge] =
      (_scalar_endpoint * this->m_xyz[_axisEdge] - _scalar * _endpoint.m_xyz[_axisEdge]) /
      (_scalar_endpoint - _scalar);
    TRACE("::computeEdgeInterface [" << xyz[0] << " ; " << xyz[1] << " ; " << xyz[2] << "]")
    _pointInter.setIntersectXYZ(_parent, xyz, true);
    ERROR(_pointInter.m_xyz[_axisEdge] == this->m_xyz[_axisEdge] ||
        _pointInter.m_xyz[_axisEdge] == _endpoint.m_xyz[_axisEdge],
      "NEW POINT can't be a Vertex.");
    _iEdgePoint = _iEdgeCell;
    TRACE("::computeEdgeInterface   SET EDGE#" << _iEdgeCell << " NEW POINT")
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::vtkInternal3D::Point::computeEdge(
  vtkHyperTreeGridGeometry::vtkInternal3D* _parent,
  const Point& _endpoint /* l'autre extremite de l'arete */,
  std::vector<std::pair<Point, Point>>& _edge_points, unsigned int _axisEdge,
  unsigned int _iEdgeCell, std::map<unsigned int, std::pair<Point*, unsigned int>>& _internalFaceA,
  std::map<unsigned int, std::pair<Point*, unsigned int>>& _internalFaceB,
  unsigned int& _crtEdgePointA, unsigned int& _crtEdgePointB)
{
  static const std::vector<std::pair<unsigned int, unsigned int>> idPtsEdge = { { 0, 1 } /*  0 */,
    { 0, 2 } /*  1 */, { 0, 4 } /* 2 */, { 1, 3 } /* 3 */, { 1, 5 } /* 4 */, { 2, 3 } /*  5 */,
    { 2, 6 } /*  6 */, { 3, 7 } /* 7 */, { 4, 5 } /* 8 */, { 4, 6 } /* 9 */, { 5, 7 } /* 10 */,
    { 6, 7 } /* 11 */ };

  TRACE("::computeEdge iEdgeCell#" << _iEdgeCell << " crtEdgePointA#" << _crtEdgePointA
                                   << " crtEdgePointB#" << _crtEdgePointB)
  Point pointA, pointB;
  unsigned int iEdgePointA = _iEdgeCell, iEdgePointB = _iEdgeCell;
  ERROR(this->m_xyz[_axisEdge] > _endpoint.m_xyz[_axisEdge],
    "According to axis Edge, the coordinate of the FIRST is less than the SECOND.");
  if (getenv("NEW"))
  {
    TRACE("NEW")
    TRACE("::computeEdge with interfaceA ? " << this->m_withInterfaceA << "?")
    if (existInterfaceA() &&
      this->computeEdgeInterface(_parent, scalarInterfaceA(), _endpoint,
        _endpoint.scalarInterfaceA(), _edge_points, _axisEdge, _iEdgeCell, _internalFaceA, pointA,
        iEdgePointA))
    {
      return;
    }
    TRACE("")
  }
  else
  {
    TRACE("computeEdge interfaceA ? " << this->m_withInterfaceA)
    if (existInterfaceA())
    {
      double scalar = _endpoint.scalarInterfaceA();
      TRACE("computeEdge pointA VERTEX FIRST valid#"
        << this->m_isValid << " [" << this->m_xyz[0] << " ; " << this->m_xyz[1] << " ; "
        << this->m_xyz[2] << "] id#" << this->m_id << " scalar#" << this->m_scalarInterfaceA)
      TRACE("computeEdge pointA VERTEX SECOND valid#"
        << _endpoint.m_isValid << " [" << _endpoint.m_xyz[0] << " ; " << _endpoint.m_xyz[1] << " ; "
        << _endpoint.m_xyz[2] << "] id#" << _endpoint.m_id << " scalar#" << scalar)
      if (this->m_scalarInterfaceA == 0.)
      {
        if (scalar == 0.)
        {
          TRACE("computeEdge pointA _axisEdge#" << _axisEdge)
          TRACE("computeEdge pointA IS VERTEX FIRST TO SECOND")

          // Le cas sera traite par ailleurs
          unsigned int iEdgePointA1 = idPtsEdge[_iEdgeCell].first + 12;
          _edge_points[iEdgePointA1].first.set(*this);
          _edge_points[iEdgePointA1].first.setIsCorner();
          _edge_points[iEdgePointA1].second.reset();
          _parent->setInterfaceFace(
            _edge_points, iEdgePointA1, _internalFaceA, &_edge_points[iEdgePointA1].first);

          pointA.set(*this);
          pointA.setIsCorner();
          TRACE("computeEdge set edge#" << iEdgePointA1 << " pointA valid#" << pointA.m_isValid
                                        << " [" << pointA.m_xyz[0] << " ; " << pointA.m_xyz[1]
                                        << " ; " << pointA.m_xyz[2] << "] id#" << pointA.m_id
                                        << " scalar#" << pointA.m_scalarInterfaceA)

          unsigned int iEdgePointA2 = idPtsEdge[_iEdgeCell].second + 12;
          _edge_points[iEdgePointA2].first.set(_endpoint);
          _edge_points[iEdgePointA2].first.setIsCorner();
          _edge_points[iEdgePointA2].second.reset();
          _parent->setInterfaceFace(
            _edge_points, iEdgePointA2, _internalFaceA, &_edge_points[iEdgePointA2].first);

          pointA.set(_endpoint);
          pointA.setIsCorner();
          TRACE("computeEdge set edge#" << iEdgePointA2 << " pointA valid#" << pointA.m_isValid
                                        << " [" << pointA.m_xyz[0] << " ; " << pointA.m_xyz[1]
                                        << " ; " << pointA.m_xyz[2] << "] id#" << pointA.m_id
                                        << " scalar#" << pointA.m_scalarInterfaceA)

          // build link between iEdgePointA1 et iEdgePointA2
          _parent->completeChainette(_internalFaceA, iEdgePointA1, iEdgePointA2);
          return;
        }
        TRACE("computeEdge pointA _axisEdge#" << _axisEdge)
        TRACE("computeEdge pointA IS VERTEX FIRST")
        pointA.set(*this);
        pointA.setIsCorner();
        iEdgePointA = idPtsEdge[_iEdgeCell].first + 12;
        TRACE("computeEdge iEdgePointA#" << iEdgePointA)

        TRACE("computeEdge pointA valid#" << pointA.m_isValid << " [" << pointA.m_xyz[0] << " ; "
                                          << pointA.m_xyz[1] << " ; " << pointA.m_xyz[2] << "] id#"
                                          << pointA.m_id << " scalar#" << pointA.m_scalarInterfaceA)
      }
      else if (scalar == 0.)
      {
        TRACE("computeEdge pointA _axisEdge#" << _axisEdge)
        TRACE("computeEdge pointA IS VERTEX SECOND")
        pointA.set(_endpoint);
        pointA.setIsCorner();
        iEdgePointA = idPtsEdge[_iEdgeCell].second + 12;
        TRACE("computeEdge iEdgePointA#" << iEdgePointA)

        TRACE("computeEdge pointA valid#" << pointA.m_isValid << " [" << pointA.m_xyz[0] << " ; "
                                          << pointA.m_xyz[1] << " ; " << pointA.m_xyz[2] << "] id#"
                                          << pointA.m_id << " scalar#" << pointA.m_scalarInterfaceA)
      }
      else if (this->m_scalarInterfaceA * scalar < 0.)
      {
        TRACE("computeEdge pointA _axisEdge#" << _axisEdge)
        double xyz[3];
        memcpy(xyz, this->m_xyz, 3 * sizeof(double));
        xyz[_axisEdge] = (scalar * this->m_xyz[_axisEdge] -
                           this->m_scalarInterfaceA * _endpoint.m_xyz[_axisEdge]) /
          (scalar - this->m_scalarInterfaceA);
        TRACE("computeEdge pointA [" << xyz[0] << " ; " << xyz[1] << " ; " << xyz[2] << "]")
        pointA.setIntersectXYZ(_parent, xyz, true);
        TRACE("computeEdge pointA [" << pointA.m_xyz[0] << " ; " << pointA.m_xyz[1] << " ; "
                                     << pointA.m_xyz[2] << "] id#" << pointA.m_id)
        if (pointA.m_xyz[_axisEdge] == this->m_xyz[_axisEdge] ||
          pointA.m_xyz[_axisEdge] == _endpoint.m_xyz[_axisEdge])
        {
          TRACE("computeEdge pointA same corner point -> isValid=false ########### IMPOSSIBLE "
                "############")
          pointA.m_isValid = false;
        }
        TRACE("computeEdge iEdgePointA#" << iEdgePointA)

        TRACE("computeEdge pointA valid#" << pointA.m_isValid << " [" << pointA.m_xyz[0] << " ; "
                                          << pointA.m_xyz[1] << " ; " << pointA.m_xyz[2] << "] id#"
                                          << pointA.m_id << " scalar#" << pointA.m_scalarInterfaceA)
      }
    }
  }
  ERROR(pointA.isValid() &&
      !(this->m_xyz[_axisEdge] <= pointA.m_xyz[_axisEdge] &&
        pointA.m_xyz[_axisEdge] <= _endpoint.m_xyz[_axisEdge]),
    "According to axis Edge, the coordinate of pointA is more or equal than the FIRST and less "
    "or equal than the SECOND.");
  if (getenv("NEW"))
  {
    TRACE("NEW")
    TRACE("computeEdge interfaceB?" << this->m_withInterfaceB)
    if (existInterfaceB() &&
      this->computeEdgeInterface(_parent, scalarInterfaceB(), _endpoint,
        _endpoint.scalarInterfaceB(), _edge_points, _axisEdge, _iEdgeCell, _internalFaceB, pointB,
        iEdgePointB))
    {
      return;
    }
    TRACE("")
  }
  else
  {
    if (existInterfaceB())
    {
      double scalar = _endpoint.scalarInterfaceB();
      TRACE("computeEdge pointB VERTEX FIRST [" << this->m_xyz[0] << " ; " << this->m_xyz[1]
                                                << " ; " << this->m_xyz[2] << "] id#" << this->m_id
                                                << " scalar#" << this->m_scalarInterfaceB)
      TRACE("computeEdge pointB VERTEX SECOND ["
        << _endpoint.m_xyz[0] << " ; " << _endpoint.m_xyz[1] << " ; " << _endpoint.m_xyz[2]
        << "] id#" << _endpoint.m_id << " scalar#" << scalar)
      if (this->m_scalarInterfaceB == 0.)
      {
        if (scalar == 0.)
        {
          TRACE("computeEdge pointB _axisEdge#" << _axisEdge)
          TRACE("computeEdge pointB IS VERTEX FIRST TO SECOND")

          // Le cas sera traite par ailleurs
          unsigned int iEdgePointB1 = idPtsEdge[_iEdgeCell].first + 12;
          _edge_points[iEdgePointB1].first.set(*this);
          _edge_points[iEdgePointB1].first.setIsCorner();
          _edge_points[iEdgePointB1].second.reset();
          _parent->setInterfaceFace(
            _edge_points, iEdgePointB1, _internalFaceB, &_edge_points[iEdgePointB1].first);

          pointB.set(*this);
          pointB.setIsCorner();
          TRACE("computeEdge set edge#" << iEdgePointB1 << " pointB valid#" << pointB.m_isValid
                                        << " [" << pointB.m_xyz[0] << " ; " << pointB.m_xyz[1]
                                        << " ; " << pointB.m_xyz[2] << "] id#" << pointB.m_id
                                        << " scalar#" << pointB.m_scalarInterfaceB)

          unsigned int iEdgePointB2 = idPtsEdge[_iEdgeCell].second + 12;
          _edge_points[iEdgePointB2].first.set(_endpoint);
          _edge_points[iEdgePointB2].first.setIsCorner();
          _edge_points[iEdgePointB2].second.reset();
          _parent->setInterfaceFace(
            _edge_points, iEdgePointB2, _internalFaceB, &_edge_points[iEdgePointB2].first);

          pointB.set(_endpoint);
          pointB.setIsCorner();
          TRACE("computeEdge set edge#" << iEdgePointB2 << " pointB valid#" << pointB.m_isValid
                                        << " [" << pointB.m_xyz[0] << " ; " << pointB.m_xyz[1]
                                        << " ; " << pointB.m_xyz[2] << "] id#" << pointB.m_id
                                        << " scalar#" << pointB.m_scalarInterfaceB)

          _parent->completeChainette(_internalFaceB, iEdgePointB1, iEdgePointB2);
          return;
        }
        TRACE("computeEdge pointB _axisEdge#" << _axisEdge)
        TRACE("computeEdge pointB IS VERTEX FIRST")
        pointB.set(*this);
        pointB.setIsCorner();
        iEdgePointB = idPtsEdge[_iEdgeCell].first + 12;
        TRACE("computeEdge iEdgePointB#" << iEdgePointB)

        TRACE("computeEdge pointB valid#" << pointB.m_isValid << " [" << pointB.m_xyz[0] << " ; "
                                          << pointB.m_xyz[1] << " ; " << pointB.m_xyz[2] << "] id#"
                                          << pointB.m_id << " scalar#" << pointB.m_scalarInterfaceB)
      }
      else if (scalar == 0.)
      {
        TRACE("computeEdge pointB _axisEdge#" << _axisEdge)
        TRACE("computeEdge pointB IS VERTEX SECOND")
        pointB.set(_endpoint);
        pointB.setIsCorner();
        iEdgePointB = idPtsEdge[_iEdgeCell].second + 12;
        TRACE("computeEdge iEdgePointB#" << iEdgePointB)

        TRACE("computeEdge pointB valid#" << pointB.m_isValid << " [" << pointB.m_xyz[0] << " ; "
                                          << pointB.m_xyz[1] << " ; " << pointB.m_xyz[2] << "] id#"
                                          << pointB.m_id << " scalar#" << pointB.m_scalarInterfaceB)
      }
      else if (this->m_scalarInterfaceB * scalar < 0)
      {
        TRACE("computeEdge pointB _axisEdge#" << _axisEdge)
        TRACE("computeEdge pointB VERTEX FIRST ["
          << this->m_xyz[0] << " ; " << this->m_xyz[1] << " ; " << this->m_xyz[2] << "] id#"
          << this->m_id << " scalar#" << this->m_scalarInterfaceB)
        TRACE("computeEdge pointB VERTEX SECOND ["
          << _endpoint.m_xyz[0] << " ; " << _endpoint.m_xyz[1] << " ; " << _endpoint.m_xyz[2]
          << "] id#" << _endpoint.m_id << " scalar#" << scalar)
        double xyz[3];
        memcpy(xyz, this->m_xyz, 3 * sizeof(double));
        xyz[_axisEdge] = (scalar * this->m_xyz[_axisEdge] -
                           this->m_scalarInterfaceB * _endpoint.m_xyz[_axisEdge]) /
          (scalar - this->m_scalarInterfaceB);
        TRACE("computeEdge pointB [" << xyz[0] << " ; " << xyz[1] << " ; " << xyz[2] << "]")
        pointB.setIntersectXYZ(_parent, xyz, false);
        TRACE("computeEdge pointB [" << pointB.m_xyz[0] << " ; " << pointB.m_xyz[1] << " ; "
                                     << pointB.m_xyz[2] << "]"
                                     << "] id#" << pointB.m_id)
        if (pointB.m_xyz[_axisEdge] == this->m_xyz[_axisEdge] ||
          pointB.m_xyz[_axisEdge] == _endpoint.m_xyz[_axisEdge])
        {
          TRACE("computeEdge pointB same corner point -> isValid=false IMPOSSIBLE")
          pointB.m_isValid = false;
        }

        TRACE("computeEdge iEdgePointB#" << iEdgePointB)

        TRACE("computeEdge pointB valid#" << pointB.m_isValid << " [" << pointB.m_xyz[0] << " ; "
                                          << pointB.m_xyz[1] << " ; " << pointB.m_xyz[2] << "] id#"
                                          << pointB.m_id << " scalar#" << pointB.m_scalarInterfaceA)
      }
    }
  }
  ERROR(pointB.isValid() &&
      !(this->m_xyz[_axisEdge] <= pointB.m_xyz[_axisEdge] &&
        pointB.m_xyz[_axisEdge] <= _endpoint.m_xyz[_axisEdge]),
    "According to axis Edge, the coordinate of pointB is more or equal than the FIRST and less "
    "or equal than the SECOND.")
  TRACE("")
  TRACE("computeEdge enregistrement")
  if (pointA.isValid())
  {
    if (pointB.isValid())
    {
      if (pointA.m_xyz[_axisEdge] < pointB.m_xyz[_axisEdge])
      {
        TRACE("computeEdge _edge_points pointA + pointB")
        if (_iEdgeCell == iEdgePointA && _iEdgeCell == iEdgePointB)
        {
          _edge_points[_iEdgeCell].first.set(pointA);
          _parent->setInterfaceFace(
            _edge_points, _iEdgeCell, _internalFaceA, &_edge_points[_iEdgeCell].first);

          _parent->completeChainette(_internalFaceA, _crtEdgePointA, _iEdgeCell);
          _crtEdgePointA = _iEdgeCell;

          _edge_points[_iEdgeCell].second.set(pointB);
          _parent->setInterfaceFace(
            _edge_points, _iEdgeCell, _internalFaceB, &_edge_points[_iEdgeCell].second);

          _parent->completeChainette(_internalFaceB, _crtEdgePointB, _iEdgeCell);
          _crtEdgePointB = _iEdgeCell;
        }
        else
        {
          _edge_points[iEdgePointA].first.set(pointA);
          _parent->setInterfaceFace(
            _edge_points, iEdgePointA, _internalFaceA, &_edge_points[iEdgePointA].first);

          _parent->completeChainette(_internalFaceA, _crtEdgePointA, iEdgePointA);
          _crtEdgePointA = iEdgePointA;

          _edge_points[iEdgePointB].second.set(pointB);
          _parent->setInterfaceFace(
            _edge_points, iEdgePointB, _internalFaceB, &_edge_points[iEdgePointB].second);

          _parent->completeChainette(_internalFaceB, _crtEdgePointB, iEdgePointB);
          _crtEdgePointB = iEdgePointB;
        }
      }
      if (pointA.m_xyz[_axisEdge] > pointB.m_xyz[_axisEdge])
      {
        TRACE("computeEdge _edge_points pointB + pointA")
        if (_iEdgeCell == iEdgePointA && _iEdgeCell == iEdgePointB)
        {
          _edge_points[_iEdgeCell].first.set(pointB);
          _parent->setInterfaceFace(
            _edge_points, _iEdgeCell, _internalFaceB, &_edge_points[_iEdgeCell].first);

          _parent->completeChainette(_internalFaceB, _crtEdgePointB, _iEdgeCell);
          _crtEdgePointB = _iEdgeCell;

          _edge_points[_iEdgeCell].second.set(pointA);
          _parent->setInterfaceFace(
            _edge_points, _iEdgeCell, _internalFaceA, &_edge_points[_iEdgeCell].second);

          _parent->completeChainette(_internalFaceA, _crtEdgePointA, _iEdgeCell);
          _crtEdgePointA = _iEdgeCell;
        }
        else
        {
          _edge_points[iEdgePointA].first.set(pointA);
          _edge_points[iEdgePointA].second.reset();
          _parent->setInterfaceFace(
            _edge_points, iEdgePointA, _internalFaceA, &_edge_points[iEdgePointA].first);

          _parent->completeChainette(_internalFaceA, _crtEdgePointA, iEdgePointA);
          _crtEdgePointA = iEdgePointA;

          _edge_points[iEdgePointB].second.set(pointB);
          _edge_points[iEdgePointB].second.reset();
          _parent->setInterfaceFace(
            _edge_points, iEdgePointB, _internalFaceB, &_edge_points[iEdgePointB].second);

          _parent->completeChainette(_internalFaceB, _crtEdgePointB, iEdgePointB);
          _crtEdgePointB = iEdgePointB;
        }
      }
    }
    else
    {
      TRACE("computeEdge _edge_points[" << iEdgePointA << "] pointA")
      _edge_points[iEdgePointA].first.set(pointA);
      _parent->setInterfaceFace(
        _edge_points, iEdgePointA, _internalFaceA, &_edge_points[iEdgePointA].first);

      _parent->completeChainette(_internalFaceA, _crtEdgePointA, iEdgePointA);
      _crtEdgePointA = iEdgePointA;
      TRACE("computeEdge set crtEdgePointA#" << _crtEdgePointA)
    }
  }
  else if (pointB.isValid())
  {
    TRACE("computeEdge _edge_points pointB")
    _edge_points[iEdgePointB].first.set(pointB);
    _parent->setInterfaceFace(
      _edge_points, iEdgePointB, _internalFaceB, &_edge_points[iEdgePointB].first);

    _parent->completeChainette(_internalFaceB, _crtEdgePointB, iEdgePointB);
    _crtEdgePointB = iEdgePointB;
  }
}

VTK_ABI_NAMESPACE_END
