/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelopipedRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParallelopipedRepresentation.h"

#include "vtkSmartPointer.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkInteractorObserver.h"
#include "vtkEvent.h"
#include "vtkSphereHandleRepresentation.h"
#include "vtkLine.h"
#include "vtkClosedSurfacePointPlacer.h"
#include "vtkPlaneCollection.h"
#include "vtkPlane.h"
#include <vector>
#include <set>
#include <algorithm>

//----------------------------------------------------------------------------
// This class manages topological information for a parallelopiped with a
// chair etched out at any node.
// README : Uncomment the line that reads "PrintTopology(cout) to
//          understand what the class does. The goal of the class is succintly
//          described in that one line.
class vtkParallelopipedTopology
{
public:
  typedef struct Line { vtkIdType Id[2];
          Line(vtkIdType a, vtkIdType b) { Id[0]=a; Id[1]=b; } }  LineType;
  typedef std::vector< vtkIdType >                             CellType;
  typedef std::vector< CellType  >                             CliqueType;

  // Diametric opposite of Corner 0 = 6, 1 = 7, 2 = 4, 3 = 5.
  // Mathematically, if a diametric corner is represented by a 3 bit value:
  // abc, its diametric opposite = a'b'c.
  static int GetDiametricOppositeOfCorner( int i )
  {
    return ((~i) & 0x6) | (i & 0x1);
  }

  // Get the corners connected to corner 'i'. There will be three such corners
  void GetNeighbors( int c, vtkIdType neighborPtIds[3], int configuration = 0 ) const
  {
    std::set< vtkIdType > neighbors;
    const CliqueType & clique = m_Topology[configuration];
    for (CliqueType::const_iterator clit = clique.begin();
         clit != clique.end(); ++clit)
    {
      if (std::find(clit->begin(), clit->end(), c) != clit->end())
      {
        const CellType cell = RotateCell( *clit, c );
        neighbors.insert(cell[0]);
        neighbors.insert(cell[cell.size()-2]);
      }
    }
    int i = 0;
    for (std::set< vtkIdType >::const_iterator it = neighbors.begin();
         it != neighbors.end(); neighborPtIds[i++] = *it, ++it)
    {
      ;
    }
  }

  void GetNeighbors( vtkIdType node, vtkIdType neighborPtIds[3],
      vtkCellArray *neighborCells, std::vector< LineType > & lines )
  {
    GetNeighbors( 8 + GetDiametricOppositeOfCorner(node), neighborPtIds, node+1 );
    vtkIdType opposingNeighborPtIds[3],
              opposite = GetDiametricOppositeOfCorner(node);
    GetNeighbors( opposite, opposingNeighborPtIds );

    std::vector< vtkIdType > nodes(2);
    for (int i = 0; i < 3; i++)
    {
      nodes[0] = neighborPtIds[i];
      for (int j = 0; j < 3; j++)
      {
        nodes[1] = opposingNeighborPtIds[j];
        const CliqueType cells =
          FindCellsContainingNodes( m_Topology[node+1], nodes );
        if (cells.size())
        {
          PopulateTopology( cells, neighborCells );
          lines.push_back( LineType(opposite, nodes[1]) );
        }
      }
    }
  }

  void FindCellsContainingNodes( int configuration, vtkCellArray *cellArray,
                        const std::vector< vtkIdType > & nodes ) const
  {
    vtkParallelopipedTopology::PopulateTopology(
      FindCellsContainingNodes( m_Topology[configuration], nodes), cellArray );
  }

  std::vector< CellType > FindCellsContainingNodes(
       int configuration, const std::vector< vtkIdType > & nodes )
    { return FindCellsContainingNodes( m_Topology[configuration], nodes); }

  vtkParallelopipedTopology()
  {

    // The topology of a parallelopiped.
    CliqueType clique;
    AddCellToClique(clique, 3,0,4,7);
    AddCellToClique(clique, 1,2,6,5);
    AddCellToClique(clique, 0,1,5,4);
    AddCellToClique(clique, 2,3,7,6);
    AddCellToClique(clique, 0,3,2,1);
    AddCellToClique(clique, 4,5,6,7);
    m_Topology.push_back(clique);

    for ( vtkIdType i = 0; i < 8;
          m_Topology.push_back( GetChairClique( i++, clique ) ) )
    {
      ;
    }

    // README : The goal of the class is succintly described by the line below
    // PrintTopology( cout );
  }

  // Populate topoplogy into a vtkCellArray.
  // If configuration is 0, the topoology populated is that of a parallelopiped.
  // If configuration > 0, the topology populated is that of a parallelopiped
  // with a chair at node = (configuration - 1).
  void PopulateTopology( int configuration, vtkCellArray * cellArray ) const
    { vtkParallelopipedTopology::PopulateTopology(
                          m_Topology[configuration], cellArray ); }

  void PrintTopology(ostream &os) const
  {
    os << "Connectivity of Point Ids in a parallelopiped: " << endl;
    PrintClique(m_Topology[0], os);
    for (int i = 0; i < 8; i++)
    {
      os << "Connectivity of Point Ids in a parallelopiped "
         << "with chair carved out at node: " << i << endl;
      PrintClique(m_Topology[i+1], os);
    }
  }

private:

  void AddCellToClique( CliqueType & clique,
      vtkIdType a, vtkIdType b, vtkIdType c, vtkIdType d)
  {
    CellType v(4);
    v[0] = a; v[1] = b; v[2] = c; v[3] = d;
    clique.push_back(v);
  }

  static CellType RotateCell( const CellType & cell, vtkIdType endval )
  {
    CellType outputCell;
    for (CellType::const_iterator cit = cell.begin(); cit != cell.end(); ++cit)
    {
      outputCell.push_back(*cit);
      if (*cit == endval) break;
    }
    for (CellType::const_reverse_iterator cit = cell.rbegin(); cit != cell.rend(); ++cit)
    {
      if (*cit == endval) break;
      outputCell.insert(outputCell.begin(), *cit);
    }
    return outputCell;
  }

  static CellType ReverseCell( const CellType & cell )
  {
    CellType outputCell;
    for (CellType::const_reverse_iterator cit = cell.rbegin(); cit != cell.rend(); ++cit)
      outputCell.push_back(*cit);
    return outputCell;
  }

  static CellType ChairCell( const CellType & cell )
  {
    CellType outputCell = ReverseCell(cell);
    for (CellType::iterator cit = outputCell.begin(); cit != outputCell.end(); ++cit)
      *cit += 8;
    return outputCell;
  }

  static CellType ChairCell( vtkIdType c, const CellType & cell )
  {
    CellType tmpCell = RotateCell(cell, c);
    CellType::iterator it = tmpCell.end();
    tmpCell.erase(--it);
    CellType outputCell = tmpCell;
    for (CellType::reverse_iterator cit = tmpCell.rbegin(); cit != tmpCell.rend(); ++cit)
      outputCell.push_back(*cit + 8);
    return outputCell;
  }

  static CliqueType GetChairClique( vtkIdType c, const CliqueType & clique )
  {
    CliqueType outputClique;
    for (CliqueType::const_iterator clit = clique.begin();
         clit != clique.end(); ++clit)
    {
      if (std::find(clit->begin(), clit->end(), c) == clit->end())
      {
        outputClique.insert( outputClique.begin(), *clit );
        outputClique.push_back( ChairCell(*clit) );
      }
      else
      {
        outputClique.insert( outputClique.begin(), ChairCell(c, *clit) );
      }
    }
    return outputClique;
  }

  static void PopulateTopology( const CliqueType & clique, vtkCellArray * cellArray )
  {
    for (CliqueType::const_iterator clit = clique.begin();
         clit != clique.end(); ++clit)
    {
      vtkIdType *ids = new vtkIdType[clit->size()];
      int i = 0;
      for (CellType::const_iterator cit = clit->begin();
           cit != clit->end(); ids[i++] = *cit, ++cit )
      {
        ;
      }
      cellArray->InsertNextCell( static_cast<vtkIdType>(clit->size()), ids );
      delete [] ids;
    }
  }

  // Find all the cells in a given a configuration (specified by the clique)
  // that contain the nodes. (specified by nodes). Each cell returned must
  // contain all the nodes specified.
  static std::vector< CellType > FindCellsContainingNodes(
       const CliqueType & clique, const std::vector< vtkIdType > & nodes )
  {
    std::vector< CellType > cells;
    for (CliqueType::const_iterator clit = clique.begin();
         clit != clique.end(); ++clit)
    {
      bool found = true;
      for (std::vector< vtkIdType >::const_iterator nit = nodes.begin();
            nit != nodes.end(); ++nit)
        found &= (std::find(clit->begin(), clit->end(), *nit) != clit->end());
      if (found) cells.push_back(*clit);
    }
    return cells;
  }

  static void PrintCell( const CellType & cell, ostream &os )
  {
    for (CellType::const_iterator cit = cell.begin();
         cit != cell.end(); os << *cit << " ", ++cit )
    {
      ;
    }
  }

  static void PrintClique( const CliqueType & clique, ostream &os )
  {
    os << "  Clique has " << clique.size() << " cells." << endl;
    for (CliqueType::const_iterator clit = clique.begin();
         clit != clique.end(); ++clit)
    {
      os << "  Cell PtIds: ";
      PrintCell( *clit, os );
      os << endl;
    }
  }

  std::vector< CliqueType > m_Topology;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkParallelopipedRepresentation);

vtkCxxSetObjectMacro(vtkParallelopipedRepresentation,
                     HandleProperty, vtkProperty);
vtkCxxSetObjectMacro(vtkParallelopipedRepresentation,
                     SelectedHandleProperty, vtkProperty);
vtkCxxSetObjectMacro(vtkParallelopipedRepresentation,
                     HoveredHandleProperty, vtkProperty);

//----------------------------------------------------------------------------
vtkParallelopipedRepresentation::vtkParallelopipedRepresentation()
{
  // This contains all the connectivity information.
  this->Topology = new vtkParallelopipedTopology;

  this->LastEventPosition[0] = this->LastEventPosition[1] = 0.0;

  // Construct the poly data representing the hex
  this->HexPolyData       = vtkPolyData::New();
  this->HexMapper         = vtkPolyDataMapper::New();
  this->HexActor          = vtkActor::New();
  this->HexMapper->SetInputData(HexPolyData);
  this->HexActor->SetMapper(this->HexMapper);

  // 16 points from the parallelopiped and the chair (also modelled as a
  // parallelopiped).
  this->Points            = vtkPoints::New(VTK_DOUBLE);
  this->Points->SetNumberOfPoints(16);
  this->HexPolyData->SetPoints(this->Points);

  vtkCellArray *cellArray = vtkCellArray::New();
  this->Topology->PopulateTopology( 0, cellArray );
  this->HexPolyData->SetPolys(cellArray);
  this->HexPolyData->BuildCells();
  cellArray->Delete();

  // The face of the polyhedron
  vtkIdType pts[4] = { 4, 5, 6, 7 };
  vtkCellArray * cells = vtkCellArray::New();
  cells->Allocate(cells->EstimateSize(1,4));
  cells->InsertNextCell(4,pts); //temporary, replaced later
  this->HexFacePolyData = vtkPolyData::New();
  this->HexFaceMapper   = vtkPolyDataMapper::New();
  this->HexFaceActor    = vtkActor::New();
  this->HexFacePolyData->SetPoints(this->Points);
  this->HexFacePolyData->SetPolys(cells);
  this->HexFaceMapper->SetInputData(HexFacePolyData);
  this->HexFaceActor->SetMapper(this->HexFaceMapper);
  cells->Delete();

  // Set some default properties.
  // Handle properties
  this->HandleProperty          = vtkProperty::New();
  this->SelectedHandleProperty  = vtkProperty::New();
  this->HoveredHandleProperty   = vtkProperty::New();
  this->HandleProperty        ->SetColor(1.0,1.0,0.7);
  this->SelectedHandleProperty->SetColor(1.0,0.2,0.1);
  this->HoveredHandleProperty ->SetColor(1.0,0.7,0.5);

  // Face properties
  this->FaceProperty            = vtkProperty::New();
  this->SelectedFaceProperty    = vtkProperty::New();
  this->FaceProperty        ->SetColor(1,1,1);
  this->SelectedFaceProperty->SetColor(0,0,1);
  this->FaceProperty->SetOpacity(0.0);
  this->SelectedFaceProperty->SetOpacity(0.25);

  // Outline properties (for the hex and the chair)
  this->OutlineProperty = vtkProperty::New();
  this->OutlineProperty->SetRepresentationToWireframe();
  this->OutlineProperty->SetAmbient(1.0);
  this->OutlineProperty->SetAmbientColor(1.0,1.0,1.0);
  this->OutlineProperty->SetLineWidth(2.0);
  this->SelectedOutlineProperty = vtkProperty::New();
  this->SelectedOutlineProperty->SetRepresentationToWireframe();
  this->SelectedOutlineProperty->SetAmbient(1.0);
  this->SelectedOutlineProperty->SetAmbientColor(0.0,0.0,1.0);
  this->SelectedOutlineProperty->SetLineWidth(2.0);
  this->HexActor->SetProperty(this->OutlineProperty);
  this->HexFaceActor->SetProperty(this->FaceProperty);

  // Handle looks like a sphere.
  this->HandleRepresentation  = NULL;
  this->HandleRepresentations = NULL;
  vtkSphereHandleRepresentation * hRep = vtkSphereHandleRepresentation::New();
  this->SetHandleRepresentation(hRep);
  hRep->Delete();

  this->CurrentHandleIdx  = -1;
  this->LastResizeAxisIdx = -1;
  this->ChairHandleIdx    = -1;

  // Point placer to dictate placement of the chair point inside the
  // parallelopiped.
  this->ChairPointPlacer  = vtkClosedSurfacePointPlacer::New();

  this->InitialChairDepth = 0.25;
  this->MinimumThickness  = 0.05;
  this->AbsoluteMinimumThickness  = 0.05;
  this->PlaceFactor       = 1.0;

  // Define the point coordinates and initial placement of the widget
  double bounds[6] = { -0.5, 0.5, -0.5, 0.5, -0.5, 0.5 };
  this->PlaceWidget(bounds);
}

//----------------------------------------------------------------------------
vtkParallelopipedRepresentation::~vtkParallelopipedRepresentation()
{
  this->HexActor->Delete();
  this->HexMapper->Delete();
  this->HexPolyData->Delete();
  this->Points->Delete();
  this->HexFaceActor->Delete();
  this->HexFaceMapper->Delete();
  this->HexFacePolyData->Delete();

  this->SetHandleRepresentation(NULL);

  this->FaceProperty->Delete();
  this->SelectedFaceProperty->Delete();
  this->OutlineProperty->Delete();
  this->SelectedOutlineProperty->Delete();

  this->SetHandleProperty         ( NULL );
  this->SetSelectedHandleProperty ( NULL );
  this->SetHoveredHandleProperty  ( NULL );
  this->ChairPointPlacer->Delete();
  delete this->Topology;
}

//----------------------------------------------------------------------------
vtkHandleRepresentation* vtkParallelopipedRepresentation
::GetHandleRepresentation( int handleIndex )
{
  return (handleIndex > 7) ? NULL : this->HandleRepresentations[handleIndex];
}

//----------------------------------------------------------------------
// You can swap the handle representation to one that you like.
void vtkParallelopipedRepresentation
::SetHandleRepresentation(vtkHandleRepresentation *handle)
{
  if ( handle == this->HandleRepresentation )
  {
    return;
  }

  vtkSetObjectBodyMacro( HandleRepresentation,
                      vtkHandleRepresentation, handle );

  if (this->HandleRepresentation)
  {
    // Allocate the 8 handles if they haven't been allocated.
    if (!this->HandleRepresentations)
    {
      this->HandleRepresentations = new vtkHandleRepresentation* [8];
      for (int i=0; i<8; this->HandleRepresentations[i++] = NULL)
      {
        ;
      }
    }
  }
  else
  {
    // Free the 8 handles if they haven't been freed.
    if (this->HandleRepresentations)
    {
      for (int i=0; i<8; this->HandleRepresentations[i++]->Delete())
      {
        ;
      }
      delete [] this->HandleRepresentations;
      this->HandleRepresentations = NULL;
    }
  }

  for (int i=0; i<8; i++)
  {

    // We will remove the old handle, in anticipation of the new user-
    // provided handle type that we are going to set a few lines later.
    if (this->HandleRepresentations && this->HandleRepresentations[i])
    {
      this->HandleRepresentations[i]->Delete();
      this->HandleRepresentations[i] = NULL;
    }

    // Copy the new user-provided handle.
    if (this->HandleRepresentation)
    {
      this->HandleRepresentations[i] = this->HandleRepresentation->NewInstance();
      this->HandleRepresentations[i]->ShallowCopy(this->HandleRepresentation);
    }
  }
}

//----------------------------------------------------------------------
// Remove any existing chairs in the parallelopiped.
void vtkParallelopipedRepresentation::RemoveExistingChairs()
{
  // If we have a chair. A chair has 9 faces as opposed to a parallelopiped
  // which has 6 faces.
  if (this->HexPolyData->GetPolys()->GetNumberOfCells() == 9)
  {

    // Go back to the topology of a parallelopiped.
    vtkCellArray *parallelopipedcells = vtkCellArray::New();
    this->Topology->PopulateTopology( 0, parallelopipedcells );
    this->HexPolyData->SetPolys(parallelopipedcells);
    this->HexPolyData->BuildCells();
    parallelopipedcells->Delete();


    // Bring the node that had the chair back to the 4th corner of the
    // parallelopiped. We will use vector addition by finding the 4th point of
    // a parallelogram from the other 3 points.
    vtkIdType neighborPtIds[3], npts = 0, *cellPtIds = NULL;
    this->Topology->GetNeighbors( this->ChairHandleIdx, neighborPtIds );

    // First find 4 points that form a parallelogram and contain the chaired
    // handle. The pointIds shall be stored in "nodes"
    vtkParallelopipedTopology::CellType nodes(3);
    nodes[0] = this->ChairHandleIdx;
    nodes[1] = neighborPtIds[0];
    nodes[2] = neighborPtIds[1];

    vtkSmartPointer< vtkCellArray > cells = vtkSmartPointer<vtkCellArray>::New();
    this->Topology->FindCellsContainingNodes( 0, cells, nodes );

    cells->InitTraversal();
    cells->GetNextCell(npts, cellPtIds);

    // Find the 4th pointId.
    int j = 0;
    while (cellPtIds[j] == nodes[0]
        || cellPtIds[j] == nodes[1]
        || cellPtIds[j] == nodes[2]) ++j;
    nodes.push_back(cellPtIds[j]);

    // Now go about finding the 4th point (Index 0) in the parallelogram..
    //     0 ------ 1
    //     |        |
    //     2 ------ 3
    //
    double p[4][3];  // for 4 points.. 3 we know, 4th to find..
    this->Points->GetPoint( nodes[3], p[0] );
    this->Points->GetPoint( nodes[1], p[1] );
    this->Points->GetPoint( nodes[2], p[2] );
    p[3][0] = p[1][0] + p[2][0] - p[0][0];
    p[3][1] = p[1][1] + p[2][1] - p[0][1];
    p[3][2] = p[1][2] + p[2][2] - p[0][2];
    this->Points->SetPoint( nodes[0], p[3] );

    this->ChairHandleIdx = -1;
  }
}

//----------------------------------------------------------------------
// Node can be an integer within [0,7]. This will create a chair one one of
// the handle corners. The '0 < scale < 1' value dicates the starting
// depth of the cavity.
void vtkParallelopipedRepresentation::UpdateChairAtNode( int node )
{
  vtkIdType npts = 0, *cellPtIds = NULL;

  // If we have a chair somewhere else, remove it. We can have only one
  // chair at a time.
  if (this->CurrentHandleIdx != this->ChairHandleIdx &&
      this->HexPolyData->GetPolys()->GetNumberOfCells() == 9)
  {
    this->RemoveExistingChairs();
  }

  this->ChairHandleIdx = node;

  // If we already don't have a chair, create one. (a chair has 6 faces,
  // unlike a parallelopiped).
  if (this->HexPolyData->GetPolys()->GetNumberOfCells() != 9)
  {
    // chair has 14 points, but we will model this with 2 parallelopipeds.
    // Hence 16 points. Look at vtkParallelopipedTopology for details.

    // Scale points with respect to the node.
    double origin[3], d[3];
    this->Points->GetPoint( node, origin );

    for (int i = 0; i < 8 ; i++)
    {
      this->Points->GetPoint(i, d);
      d[0] = (d[0] - origin[0]) * this->InitialChairDepth + origin[0];
      d[1] = (d[1] - origin[1]) * this->InitialChairDepth + origin[1];
      d[2] = (d[2] - origin[2]) * this->InitialChairDepth + origin[2];
      this->Points->SetPoint(i+8, d);
    }

    this->Points->SetPoint( node, this->Points->GetPoint(
        vtkParallelopipedTopology::GetDiametricOppositeOfCorner(node) + 8));

    vtkSmartPointer< vtkCellArray > cells = vtkSmartPointer<vtkCellArray>::New();
    this->Topology->PopulateTopology( node + 1, cells );
    this->HexPolyData->SetPolys(cells);
    this->HexPolyData->BuildCells();

    // Synchronize the handle representations with our recently updated
    // "Points" data-structure.
    this->PositionHandles();
  }
  else
  {
    // We do have a chair. Update the points in the chair by taking the
    // projection of the chaired node onto the axes of the parallelopiped.

    // These three PtIds are those that lie on the chair and are connected via
    // a line to the "Chair node" in question. It is the position of these 3
    // points that we seek to find in the next few lines.
    vtkIdType neighborPtIds[3];

    // This will contain the 3 faces that lie on the parallelopiped and have
    // a chair carved out in them. As you know, we are about to compute the
    // points at the carved out locations.
    vtkSmartPointer< vtkCellArray > neighborCells = vtkSmartPointer<vtkCellArray>::New();

    // Handle PointID is the diametric opposite of the chair corner on the
    // higher order parallelopiped (the chair parallelopiped).
    const vtkIdType chairHandleId = 8 + vtkParallelopipedTopology::
                  GetDiametricOppositeOfCorner(this->CurrentHandleIdx);

    // Get the world position of the chair handle.
    double chairPoint[3];
    this->Points->GetPoint( chairHandleId, chairPoint );

    std::vector< vtkParallelopipedTopology::LineType > lines;
    this->Topology->GetNeighbors( node, neighborPtIds, neighborCells, lines );

    neighborCells->InitTraversal();

    for (int i = 0; i < 3; i++)
    {
      double lineEndPt[2][3];
      this->Points->GetPoint( lines[i].Id[0], lineEndPt[0] );
      this->Points->GetPoint( lines[i].Id[1], lineEndPt[1] );

      double t, neighborPt[3]; // "x" is the point that we are trying to find.

      neighborCells->GetNextCell(npts, cellPtIds);

      vtkIdType planePtIds[3];

      // For each point in the cell
      for (int j = 0, idx = 0; j < npts && idx < 3; j++)
      {
        // Avoid the points that are on the chair as these are the ones we seek
        // to find.
        if ( cellPtIds[j] < 8 )
        {
          planePtIds[idx++] = cellPtIds[j];
        }
      }

      // Construct a plane from the cell.
      vtkPlane *plane = vtkPlane::New();
      this->DefinePlane(plane, planePtIds[0], planePtIds[1], planePtIds[2]);

      double endPoint[3] = { chairPoint[0] + lineEndPt[1][0] - lineEndPt[0][0],
                             chairPoint[1] + lineEndPt[1][1] - lineEndPt[0][1],
                             chairPoint[2] + lineEndPt[1][2] - lineEndPt[0][2] };

      vtkPlane::IntersectWithLine( chairPoint, endPoint,
          plane->GetNormal(), plane->GetOrigin(), t, neighborPt );
      plane->Delete();

      vtkDebugMacro( << "ChairPoint: (" << chairPoint[0] << "," << chairPoint[1]
        << "," << chairPoint[2] << ") lineEndPts [" << lines[i].Id[0] << "("
        << lineEndPt[0][0] << "," << lineEndPt[0][1] << "," << lineEndPt[0][2]
        << ")-" << lines[i].Id[1] << "(" << lineEndPt[1][0] << ","
        << lineEndPt[1][1] << "," << lineEndPt[1][2] << ")]"
        << " Intersection at: (" << neighborPt[0] << "," << neighborPt[1]
        << "," << neighborPt[2] << ")" );

      this->Points->SetPoint( neighborPtIds[i], neighborPt );
    }

    // Now that we have found the 3 neighbors, we need to compute the other
    // points in the chair. Note that we have 4 so far (3 neighbors + the
    // chair node). There are 2 more to be found. Given that they will
    // have to satisfy a parallelogram relationship, we can easily use
    // vector addition to evaluate them.

    for (int i = 0; i < 3; i++)
    {
      std::vector< vtkIdType > nodes(3);
      vtkSmartPointer< vtkCellArray > cells
          = vtkSmartPointer<vtkCellArray>::New();
      nodes[0] = 8 + vtkParallelopipedTopology::
        GetDiametricOppositeOfCorner(this->CurrentHandleIdx);
      nodes[1] = neighborPtIds[i];
      nodes[2] = neighborPtIds[(i+1)%3];
      vtkDebugMacro( << "Looking for cells containing nodes: " << nodes[0]
        << "," << nodes[1] << "," << nodes[2] << " in topology "
        << (this->CurrentHandleIdx+1) );
      this->Topology->FindCellsContainingNodes(
          this->CurrentHandleIdx + 1, cells, nodes );

      npts = 0; cellPtIds = NULL;
      cells->InitTraversal();
      cells->GetNextCell(npts, cellPtIds);

      // Find the 4th pointId. The pointIds shall be stored in "nodes"
      int j = 0;
      while (cellPtIds[j] == nodes[0]
          || cellPtIds[j] == nodes[1]
          || cellPtIds[j] == nodes[2]) ++j;
      nodes.push_back(cellPtIds[j]);

      // Now go about finding the 4th point (Index 3) in the parallelogram..
      //     0 ------ 1
      //     |        |
      //     2 ------ 3
      //
      double p[4][3];  // for 4 points.. 3 we know, 4th to find..
      this->Points->GetPoint( nodes[0], p[0] );
      this->Points->GetPoint( nodes[1], p[1] );
      this->Points->GetPoint( nodes[2], p[2] );
      p[3][0] = p[1][0] + p[2][0] - p[0][0];
      p[3][1] = p[1][1] + p[2][1] - p[0][1];
      p[3][2] = p[1][2] + p[2][2] - p[0][2];

      vtkDebugMacro( << "Parallelogram built from (nodes and points): \n"
       << "(" << nodes[0] << ") = [" << p[0][0] << "," << p[0][1] << "," << p[0][2] << "]\n"
       << "(" << nodes[1] << ") = [" << p[1][0] << "," << p[1][1] << "," << p[1][2] << "]\n"
       << "(" << nodes[2] << ") = [" << p[2][0] << "," << p[2][1] << "," << p[2][2] << "]\n"
       << "(" << cellPtIds[j] << ") = [" << p[3][0] << "," << p[3][1] << "," << p[3][2] << "]\n");

      this->Points->SetPoint( nodes[3], p[3] );
    }

    this->Points->SetPoint( 8 + vtkParallelopipedTopology::
        GetDiametricOppositeOfCorner(this->CurrentHandleIdx),
        this->Points->GetPoint(this->CurrentHandleIdx));
  }
}

//----------------------------------------------------------------------
// This is where the bulk of the work is done.
int vtkParallelopipedRepresentation
::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  int oldInteractionState = this->InteractionState;

  // (A) -----------------------------------------------------------
  // Handle the request methods. These are mere requests and will not cause
  // any change in the position of the handles or the shape of the
  // parallelopiped. The representation will, within this IF block change its
  // state from a request to a concrete state.

  if ( this->InteractionState == vtkParallelopipedRepresentation::RequestResizeParallelopiped
    || this->InteractionState == vtkParallelopipedRepresentation::RequestResizeParallelopipedAlongAnAxis
    || this->InteractionState == vtkParallelopipedRepresentation::RequestChairMode )
  {
    this->CurrentHandleIdx = -1;

    // We are trying to perform user interaction that might potentially
    // select a handle. Check if we are really near a handle, so it
    // can be selected.

    // Loop over all the handles and check if one of them is selected
    for(int i = 0; i< 8; i++)
    {
      this->HandleRepresentations[i]->ComputeInteractionState(X, Y, 0);

       if (this->HandleRepresentations[i]->GetInteractionState() ==
                                 vtkHandleRepresentation::Selecting)
       {
        // The selected handle.
        this->CurrentHandleIdx = i;

        // The shift modifier determines if the handles are going to be
        // translated along an axes of the parallelopiped.
        switch (this->InteractionState)
        {
          case vtkParallelopipedRepresentation::RequestResizeParallelopiped:
            this->InteractionState = (this->CurrentHandleIdx == this->ChairHandleIdx)
               ? ChairMode : ResizingParallelopiped;
            break;
          case vtkParallelopipedRepresentation::RequestResizeParallelopipedAlongAnAxis:
            this->InteractionState = (this->CurrentHandleIdx == this->ChairHandleIdx)
               ? ChairMode : ResizingParallelopipedAlongAnAxis;
            break;
          case vtkParallelopipedRepresentation::RequestChairMode:
          {

            // Toggle chair mode if we already have a chair here.. We are
            // trying to toggle of course.. In this case remove all chairs,
            if (this->CurrentHandleIdx == this->ChairHandleIdx &&
                this->HexPolyData->GetPolys()->GetNumberOfCells() == 9)
            {
              this->RemoveExistingChairs();
              this->LastEventPosition[0] = X;
              this->LastEventPosition[1] = Y;
              this->InteractionState = vtkParallelopipedRepresentation::Inside;

              // Synchronize the handle representations with our recently updated
              // "Points" data-structure.
              this->PositionHandles();
              return this->InteractionState;
            }

            // We aren't trying to toggle. Create one
            // Create a chair with a default cavity depth of 0.1
            this->UpdateChairAtNode( this->CurrentHandleIdx );

            // We are in chair mode. Use the placer to dictate where the
            // "chaired" handle can move. (It can only move within the
            // parallelopiped). First set some parameters on the placer.

            vtkPlaneCollection *pc = vtkPlaneCollection::New();
            this->GetParallelopipedBoundingPlanes( pc );
            this->ChairPointPlacer->SetBoundingPlanes( pc );
            pc->Delete();

            this->InteractionState = ChairMode;
            break;
          }
        }

        // Highlight the selected handle and unhighlight all others.
        this->SetHandleHighlight(-1, this->HandleProperty);
        this->SetHandleHighlight(
            this->CurrentHandleIdx, this->SelectedHandleProperty);

        break;
       }
    }

    if (this->CurrentHandleIdx == -1)
    {
      // We are near none of the handles.

      // Now check if we are within the parallelopiped or outside the
      // parallelopiped. We will use the pointplacer to evaluate this.
      vtkPlaneCollection *pc = vtkPlaneCollection::New();
      this->GetParallelopipedBoundingPlanes( pc );
      this->ChairPointPlacer->SetBoundingPlanes( pc );
      pc->Delete();

      // Use any random handle as a reference for the point placer.
      double eventDisplayPos[3] = {static_cast<double>(X),
                                   static_cast<double>(Y),
                                   0.0};
      double dummy[4], worldOrient[9], handleWorldPos[4];
      this->HandleRepresentations[0]->GetWorldPosition(handleWorldPos);

      this->InteractionState = (this->ChairPointPlacer->ComputeWorldPosition(
        this->Renderer, eventDisplayPos, handleWorldPos, dummy, worldOrient )
            ? vtkParallelopipedRepresentation::Inside
            : vtkParallelopipedRepresentation::Outside);
    }

    if (this->InteractionState == vtkParallelopipedRepresentation::Inside &&
        oldInteractionState == vtkParallelopipedRepresentation::
                                RequestResizeParallelopipedAlongAnAxis)
    {
      this->HighlightAllFaces();
    }
    else
    {
      // UnHighlight all faces
      this->UnHighlightAllFaces();
    }

    // Reset any cached "resize along that axis" stuff.
    this->LastResizeAxisIdx = -1;
  }


  // (B) -----------------------------------------------------------
  // Handle the resizing operations (along the axis or arbitrarily).

  else if (this->InteractionState ==
        vtkParallelopipedRepresentation::ResizingParallelopipedAlongAnAxis ||
      this->InteractionState ==
        vtkParallelopipedRepresentation::ResizingParallelopiped)
  {
    // Ensure that a handle has been picked.
    if (this->CurrentHandleIdx != -1)
    {
      // Compute world positions corresponding to the current event position
      // (X,Y) and the last event positions such that they lie at the same
      // depth that the handle lies on.

      double axis[3][3], eventWorldPos[4], handleWorldPos[4],
        handleDisplayPos[4], neighborWorldPos[3][4], neighborDisplayPos[3][4];

      this->HandleRepresentations[this->CurrentHandleIdx]
                                 ->GetWorldPosition(handleWorldPos);

      vtkInteractorObserver::ComputeWorldToDisplay( this->Renderer,
        handleWorldPos[0], handleWorldPos[1], handleWorldPos[2],
        handleDisplayPos);

      // Now find and get the display positions of the three neighbors of the
      // current handle. We have to rescale along one of the three edges.

      vtkIdType neighborIndices[3];
      this->Topology->GetNeighbors( this->CurrentHandleIdx, neighborIndices,
              (this->ChairHandleIdx == -1) ? 0 : this->ChairHandleIdx + 1 );

      // The motion vector in display coords
      const double motionVector[3] = { X - this->LastEventPosition[0],
                                       Y - this->LastEventPosition[1],
                                       0.0                            };

      double maxConfidence = VTK_DOUBLE_MIN;

      // The next few lines attempt to find the axis should we scale along.
      // The axis is the axis of the parallelopiped that is most aligned with
      // the direction of mouse motion.

      int axisIdx = this->LastResizeAxisIdx; // To be found out ..

      // loop over the 3 neighbors of the current handle
      for (int i = 0; i < 3; i++)
      {

        // Compute display position of this neighbor
        this->Points->GetPoint(neighborIndices[i], neighborWorldPos[i]);
        vtkInteractorObserver::ComputeWorldToDisplay( this->Renderer,
            neighborWorldPos[i][0], neighborWorldPos[i][1],
            neighborWorldPos[i][2], neighborDisplayPos[i]);

        // Dot product of the motion vector (in display coords) with each
        // of the three edges (in display coords). The maximum of the three
        // will determine which axis of the parallelopiped we will rescale along

        axis[i][0] = neighborDisplayPos[i][0] - handleDisplayPos[0];
        axis[i][1] = neighborDisplayPos[i][1] - handleDisplayPos[1];
        axis[i][2] = 0.0;
        vtkMath::Normalize2D(axis[i]);

        // If we did not compute the resize axis Idx already the last time,
        // we were in this method, compute it now, by checking which axis
        // the motion vector is most aligned with.
        if (this->LastResizeAxisIdx == -1 ||
            this->InteractionState ==
                vtkParallelopipedRepresentation::ResizingParallelopiped)
        {
          const double confidence
            = fabs(vtkMath::Dot2D( axis[i], motionVector ));
          if (confidence > maxConfidence)
          {
            axisIdx = i;
            maxConfidence = confidence;
          }
        }
      }


      // Now that we know the axis to translate along, find the amount we should
      // translate by. The new handle position must lie somewhere along the
      // line joining the currently selected handle and the neighbor that lies
      // along the rescale axis. We will evaluate 't E [-inf, 1.0]', the
      // parametric position along the line. This point will simply be the
      // point on the aforementioned line that the current event position is
      // closest to.

      double directionOfProjection[3], closestPt1[3], closestPt2[3], t1, t;

      this->Renderer->GetActiveCamera()->
            GetDirectionOfProjection(directionOfProjection);
      vtkInteractorObserver::ComputeDisplayToWorld( this->Renderer,
                            X, Y, handleDisplayPos[2], eventWorldPos);

      double l0[3] = {eventWorldPos[0] - directionOfProjection[0],
                      eventWorldPos[1] - directionOfProjection[1],
                      eventWorldPos[2] - directionOfProjection[2] };
      double l1[3] = {eventWorldPos[0] + directionOfProjection[0],
                      eventWorldPos[1] + directionOfProjection[1],
                      eventWorldPos[2] + directionOfProjection[2] };

      vtkLine::DistanceBetweenLines( handleWorldPos, neighborWorldPos[axisIdx],
                                                 l0, l1,
                                         closestPt1, closestPt2,
                                                  t, t1 );
      t = (t > 1.0 ? 1.0 : t); // clamp 't'

      vtkDebugMacro( << "Currently selected handle is at : (" <<
        handleWorldPos[0] << "," << handleWorldPos[1] << "," << handleWorldPos[2] <<
        ")\n Pt2 (the selected handle will be moved along the axis represented by"
        << " itself and Pt2) is at: (" << neighborWorldPos[axisIdx][0]
        << "," << neighborWorldPos[axisIdx][1] << "," << neighborWorldPos[axisIdx][2]
        << ")\n The selected handle will be moved to parametric location t = " << t
        << "with the line being specified by the above 2 points.");


      // This is the amount by which the face will move towards
      // (or away from if t < 0.0) the other face. We know that the face has
      // the following PointIds.
      //   1) CurrentHandleIdx
      //   2) Neighbor 1 of currentHandleIdx
      //   3) Neighbor 2 of CurrentHandleIdx
      // It will be our job in the next few lines to find the other points in
      // the face and translate the face.

      // "nodes" contains the 3 pointIds that we know are present on the face.
      std::vector< vtkIdType > nodes(3);
      nodes[0] = this->CurrentHandleIdx;

      for (int i = 0, j = 1; i < 3; i++)
      {
        if (i != axisIdx)
        {
          nodes[j++] = neighborIndices[i];
        }
      }

      // "cells" below contains the face to be translated.
      vtkSmartPointer< vtkCellArray > cells = vtkSmartPointer<vtkCellArray>::New();
      this->Topology->FindCellsContainingNodes(
        (this->ChairHandleIdx == -1) ? 0 :
          this->ChairHandleIdx + 1, cells, nodes );

      vtkIdType npts = 0, *cellPtIds = NULL;
      cells->InitTraversal();
      cells->GetNextCell(npts, cellPtIds);

      // The translation vector
      double handleTranslation[3] =
        { t * neighborWorldPos[axisIdx][0] - t * handleWorldPos[0],
          t * neighborWorldPos[axisIdx][1] - t * handleWorldPos[1],
          t * neighborWorldPos[axisIdx][2] - t * handleWorldPos[2]  };

      double newHandleWorldPos[3] = { handleWorldPos[0] + handleTranslation[0],
                                      handleWorldPos[1] + handleTranslation[1],
                                      handleWorldPos[2] + handleTranslation[2]};

      if (   t > 0.0  // We are moving towards the other handle
          && (vtkMath::Distance2BetweenPoints(
              neighborWorldPos[axisIdx], newHandleWorldPos)) < (
              this->AbsoluteMinimumThickness * this->AbsoluteMinimumThickness))
      {
        // Too close. We don't want the parallelopiped collapsing, do we ?
        vtkDebugMacro( << "AbsoluteMaximumThickness = "
          << this->AbsoluteMinimumThickness << " This move will bring us "
          << sqrt(vtkMath::Distance2BetweenPoints(
              neighborWorldPos[axisIdx], newHandleWorldPos)) << " far away to ("
          << newHandleWorldPos[0] << "," << newHandleWorldPos[1] << ","
          << newHandleWorldPos[2] << "). We can\'t do that." );

        // Revise 't' so as to maintain minimum thickness. The bottom line is
        // that although 't E [-inf, 1.0]', 't' will never hit 1.0 unless
        // AbsoluteMinimumThickness is 0.0.
        t = 1.0 - this->AbsoluteMinimumThickness / sqrt(
            vtkMath::Distance2BetweenPoints(
              neighborWorldPos[axisIdx], handleWorldPos));

        // Recompute these 2 positions with our revised 't' value.
        handleTranslation[0] = t * neighborWorldPos[axisIdx][0] - t * handleWorldPos[0];
        handleTranslation[1] = t * neighborWorldPos[axisIdx][1] - t * handleWorldPos[1];
        handleTranslation[2] = t * neighborWorldPos[axisIdx][2] - t * handleWorldPos[2];

        newHandleWorldPos[0] = handleWorldPos[0] + handleTranslation[0];
        newHandleWorldPos[1] = handleWorldPos[1] + handleTranslation[1];
        newHandleWorldPos[2] = handleWorldPos[2] + handleTranslation[2];

        if (t < 0.0)
        {
          // Sanity check. We should never get here in the first place.
          this->LastEventPosition[0] = X;
          this->LastEventPosition[1] = Y;
          return this->InteractionState;
        }

        vtkDebugMacro( "So we are revising the value of t to " << t
          << " and newHandleWorldPos to (" << newHandleWorldPos[0] << ","
          << newHandleWorldPos[1] << "," << newHandleWorldPos[2] << ")" );
      }

      // If we have a chair, prevent the handle from being translated beyond
      // the plane of the chair, otherwise it will cause the chair to turn
      // inside out. So we will do some dot-product stuff and revise the
      // "neighborWorldPos", if we have a chair.
      if (this->ChairHandleIdx != -1)
      {
        std::vector< vtkIdType > nodes2(1);
        nodes2[0] = vtkParallelopipedTopology::GetDiametricOppositeOfCorner(this->ChairHandleIdx)+8;
        const vtkParallelopipedTopology::CliqueType cells2 = this->
          Topology->FindCellsContainingNodes( this->ChairHandleIdx+1, nodes2 );

        for (vtkParallelopipedTopology::CliqueType::const_iterator clit = cells2.begin();
             clit != cells2.end(); ++clit)
        {
          vtkSmartPointer< vtkPlane > plane = vtkSmartPointer< vtkPlane >::New();
          this->DefinePlane(plane, (*clit)[0], (*clit)[1], (*clit)[2] );
          double distance = plane->EvaluateFunction(newHandleWorldPos);

          // Ensure that the handle is on the right side of the chair's plane,
          // and that it is at least 'MinimumThickness' away from any of the
          // planes of the chair.
          if (fabs(distance) < this->MinimumThickness ||
               (distance * (std::find(clit->begin(), clit->end(),
                  this->CurrentHandleIdx+8) != clit->end() ? -1 : 1) > 0))
          {
            this->LastEventPosition[0] = X;
            this->LastEventPosition[1] = Y;
            return this->InteractionState;
          }
        }
      }

      // Highlight this face...
      this->SetFaceHighlight( cells, this->SelectedFaceProperty );

      // Translate this face...
      for (vtkIdType i = 0; i < npts;
           this->TranslatePoint( cellPtIds[i++], handleTranslation ))
      {
        ;
      }

      // Cache the axis along which we resized the previous time, so we don't
      // have to recompute it.
      this->LastResizeAxisIdx = axisIdx;

      // Update the bounding planes.
      vtkPlaneCollection *pc = vtkPlaneCollection::New();
      this->GetParallelopipedBoundingPlanes( pc );
      this->ChairPointPlacer->SetBoundingPlanes( pc );
      pc->Delete();

    }
    else
    {
      // In theory, we should never get there.
      this->InteractionState = vtkParallelopipedRepresentation::Outside;
    }
  }


  // (C) -----------------------------------------------------------
  // Default method for all other states.

  else if (this->InteractionState == vtkParallelopipedRepresentation::ChairMode)
  {
    // Ensure that a handle has been picked.
    if (this->CurrentHandleIdx != -1)
    {
      double handleWorldPos[4];

      this->HandleRepresentations[this->CurrentHandleIdx]
                                 ->GetWorldPosition(handleWorldPos);

      // The new handle poistion, will lie on a plane that passes through the
      // current world position and is parallel to the focal plane.
      // To compute this, we will use the help of the focal plane point placer,
      // and supply it with the offset of the handle's distance from the
      // focal plane.

      double eventDisplayPos[3] = {static_cast<double>(X),
                                   static_cast<double>(Y),
                                   0.0};
      double newHandlePos[4], worldOrient[9];

      if (this->ChairPointPlacer->ComputeWorldPosition(
            this->Renderer, eventDisplayPos, handleWorldPos,
            newHandlePos, worldOrient ))
      {
        const double handleTranslation[3] =
              { newHandlePos[0] - handleWorldPos[0],
                newHandlePos[1] - handleWorldPos[1],
                newHandlePos[2] - handleWorldPos[2] };
        this->TranslatePoint( this->CurrentHandleIdx, handleTranslation);
      }

      this->UpdateChairAtNode( this->CurrentHandleIdx );

    }
    else
    {
      // In theory, we should never get there.
      this->InteractionState = vtkParallelopipedRepresentation::Outside;
    }
  }


  // (D) -----------------------------------------------------------
  // Default method for all other states.

  else
  {
    this->InteractionState = vtkParallelopipedRepresentation::Outside;

    // Loop over all the handles and check if we are near one of them.
    for(int i = 0; i< 8; i++)
    {
      this->HandleRepresentations[i]->ComputeInteractionState(X, Y, 0);
      if (this->HandleRepresentations[i]->GetInteractionState() ==
                                    vtkHandleRepresentation::Selecting)
      {
        this->SetHandleHighlight( i, this->HoveredHandleProperty );
        this->InteractionState = vtkParallelopipedRepresentation::Inside;
        break;
      }
    }

    if (this->InteractionState == vtkParallelopipedRepresentation::Outside)
    {
      // Unhighlight all handles and faces.
      this->SetHandleHighlight( -1, this->HandleProperty );
      this->UnHighlightAllFaces();
    }
  }

  // Cache the last event position.
  this->LastEventPosition[0] = X;
  this->LastEventPosition[1] = Y;
  return this->InteractionState;
}

//----------------------------------------------------------------------------
void vtkParallelopipedRepresentation
::TranslatePoint( int id, const double translation[3] )
{
  double p[3];
  this->Points->GetPoint(id, p);
  p[0] += translation[0];
  p[1] += translation[1];
  p[2] += translation[2];
  this->Points->SetPoint(id, p);
  if (id < 8)
  {
    this->HandleRepresentations[id]->SetWorldPosition(p);
  }

  // Update our records.
  this->PositionHandles();
}

//----------------------------------------------------------------------------
// Get the bounding planes of the object. The first 6 planes will
// be bounding planes of the parallelopiped. If in chair mode, three
// additional planes will be present. The last three planes will be those
// of the chair. The Normals of all the planes will point into the object.
//
void vtkParallelopipedRepresentation::GetBoundingPlanes( vtkPlaneCollection *pc )
{
  vtkSmartPointer< vtkCellArray > cellArray = vtkSmartPointer<vtkCellArray>::New();
  this->Topology->PopulateTopology( this->ChairHandleIdx + 1, cellArray );

  vtkIdType npts = 0, *ptIds = NULL;

  // For each planar cell in our object, we need to find the plane it lies on
  for (cellArray->InitTraversal(); cellArray->GetNextCell(npts, ptIds); )
  {
    vtkIdType planePtIds[3];

    // For each cell, get the point ids that comprise the planar cell.
    for (int i = 0, idx = 0; i < npts && idx < 3; i++)
    {
      if (this->CurrentHandleIdx != ptIds[i])
      {
        planePtIds[idx++] = ptIds[i];
      }
    }

    // Construct a plane from the cell.
    vtkPlane *plane = vtkPlane::New();
    this->DefinePlane(plane, planePtIds[0], planePtIds[1], planePtIds[2]);
    pc->AddItem(plane);
    plane->Delete();
  }
}

//----------------------------------------------------------------------------
// Convenience method to get just the planes that define the parallelopiped.
// If we aren't in chair mode, this will be the same as GetBoundingPlanes().
// If we are in chair mode, this will be the first 6 planes from amongst
// those returned by "GetBoundingPlanes".
// All planes have their normals pointing inwards.
//
void vtkParallelopipedRepresentation
::GetParallelopipedBoundingPlanes( vtkPlaneCollection * pc )
{
  vtkPlaneCollection * pc2 = vtkPlaneCollection::New();
  this->GetBoundingPlanes( pc2 );
  vtkPlane *p;
  int i = 0;
  for (pc2->InitTraversal(); ((p = pc2->GetNextItem()) && i < 6); ++i )
  {
    pc->AddItem(p);
  }
  pc2->Delete();
}

//----------------------------------------------------------------------------
// Convenience method to populate a plane from 3 pointIds
void vtkParallelopipedRepresentation::DefinePlane( vtkPlane *plane,
      vtkIdType id1, vtkIdType id2, vtkIdType id3)
{
  double p[3][3];
  this->Points->GetPoint(id1, p[0]);
  this->Points->GetPoint(id2, p[1]);
  this->Points->GetPoint(id3, p[2]);
  this->DefinePlane(plane, p);
}

//----------------------------------------------------------------------------
// Convenience method to populate a plane from 3 points.
void vtkParallelopipedRepresentation::DefinePlane( vtkPlane *plane, double p[3][3])
{
  plane->SetOrigin( p[0] );
  double v1[3] = { p[1][0] - p[0][0], p[1][1] - p[0][1], p[1][2] - p[0][2] };
  double v2[3] = { p[2][0] - p[0][0], p[2][1] - p[0][1], p[2][2] - p[0][2] };
  double normal[3];
  vtkMath::Cross( v1, v2, normal );
  vtkMath::Normalize(normal);
  plane->SetNormal( normal );
}

//----------------------------------------------------------------------
void vtkParallelopipedRepresentation::GetActors(vtkPropCollection *pc)
{
  for (int i=0; i<8; i++)
  {
    this->HandleRepresentations[i]->GetActors(pc);
  }
  this->HexActor->GetActors(pc);
  this->HexFaceActor->GetActors(pc);
}

//----------------------------------------------------------------------
void vtkParallelopipedRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
  this->HexActor->ReleaseGraphicsResources(w);
  this->HexFaceActor->ReleaseGraphicsResources(w);
  for (int i=0; i<8; i++)
  {
    this->HandleRepresentations[i]->ReleaseGraphicsResources(w);
  }
}

//----------------------------------------------------------------------
int vtkParallelopipedRepresentation::RenderOverlay(vtkViewport *v)
{
  int count = 0;
  count+=this->HexActor->RenderOverlay(v);
  count+=this->HexFaceActor->RenderOverlay(v);
  for (int i=0; i<8; i++)
  {
    count+=this->HandleRepresentations[i]->RenderOverlay(v);
  }
  return count;
}

//----------------------------------------------------------------------------
int vtkParallelopipedRepresentation::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int count = 0;
  this->BuildRepresentation();
  count+=this->HexActor->RenderOpaqueGeometry(viewport);
  count+=this->HexFaceActor->RenderOpaqueGeometry(viewport);
  for (int i=0; i<8; i++)
  {
    count += this->HandleRepresentations[i]->RenderOpaqueGeometry(viewport);
  }
  return count;
}

//----------------------------------------------------------------------------
void vtkParallelopipedRepresentation::PositionHandles()
{
  for (int i = 0; i < 8; ++i)
  {
    this->HandleRepresentations[i]->SetWorldPosition(this->Points->GetPoint(i));
  }

  this->Points->GetData()->Modified();
  this->HexFacePolyData->Modified();
  this->HexPolyData->Modified();
}

//----------------------------------------------------------------------------
void vtkParallelopipedRepresentation::HandlesOn()
{
  for (int i=0; i<8; this->HandleRepresentations[i++]->SetVisibility(1))
  {
    ;
  }
}

//----------------------------------------------------------------------------
void vtkParallelopipedRepresentation::HandlesOff()
{
  for (int i=0; i<8; this->HandleRepresentations[i++]->SetVisibility(0))
  {
    ;
  }
}

//----------------------------------------------------------------------------
void vtkParallelopipedRepresentation::SetHandleHighlight(
                      int handleIdx, vtkProperty *property )
{
  if ( handleIdx == -1)
  {
    // Do for all handles
    for (int i = 0; i < 8; i++)
    {
      static_cast< vtkSphereHandleRepresentation * >(
          this->HandleRepresentations[i])->SetProperty(property);
      static_cast< vtkSphereHandleRepresentation * >(
          this->HandleRepresentations[i])->SetSelectedProperty(property);
    }
  }
  else
  {
    static_cast< vtkSphereHandleRepresentation * >(
        this->HandleRepresentations[handleIdx])->SetProperty(property);
    static_cast< vtkSphereHandleRepresentation * >(
        this->HandleRepresentations[handleIdx])->SetSelectedProperty(property);
  }
}

//----------------------------------------------------------------------------
void vtkParallelopipedRepresentation
::SetFaceHighlight( vtkCellArray * face, vtkProperty *p )
{
  if (face)
  {
    this->HexFacePolyData->SetPolys(face);
  }
  this->HexFaceActor->SetProperty( p );
}

//----------------------------------------------------------------------------
void vtkParallelopipedRepresentation::HighlightAllFaces()
{
  vtkSmartPointer< vtkCellArray > cells = vtkSmartPointer<vtkCellArray>::New();
  this->Topology->PopulateTopology( this->ChairHandleIdx + 1, cells );
  this->SetFaceHighlight( cells, this->SelectedFaceProperty );
}

//----------------------------------------------------------------------------
void vtkParallelopipedRepresentation::UnHighlightAllFaces()
{
  this->SetFaceHighlight( NULL, this->FaceProperty );
}

//----------------------------------------------------------------------------
// Translate by a vector to be computed from the last Pick position and the
// supplied event position
void vtkParallelopipedRepresentation::Translate( int X, int Y )
{
  double eventPos[2] = { static_cast<double>(X),
                         static_cast<double>(Y)};
  double lastEventPos[2] =
    { this->LastEventPosition[0], this->LastEventPosition[1] };

  // First compute the centroid. Its only use is to determine a reference
  // plane, on which we will assume lastEventPos and eventPos lie.
  double *pts =
         static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
  double center[3] = {0.0, 0.0, 0.0};
  for (int i=0; i<8; i++)
  {
    center[0] += *pts++;
    center[1] += *pts++;
    center[2] += *pts++;
  }
  center[0] /= 8.0;
  center[1] /= 8.0;
  center[2] /= 8.0;

  // Now convert the event positions to world positions as if they lay at the
  // same plane as the center.

  double fp[4], lastEventWorldPos[4], eventWorldPos[4];

  vtkInteractorObserver::ComputeWorldToDisplay( this->Renderer,
    center[0], center[1], center[2], fp );

  vtkInteractorObserver::ComputeDisplayToWorld( this->Renderer,
      lastEventPos[0], lastEventPos[1], fp[2], lastEventWorldPos);
  vtkInteractorObserver::ComputeDisplayToWorld( this->Renderer,
      eventPos[0], eventPos[1], fp[2], eventWorldPos);

  // Compute the offset from the last event position and translate.
  double translation[3] = { eventWorldPos[0] - lastEventWorldPos[0],
                            eventWorldPos[1] - lastEventWorldPos[1],
                            eventWorldPos[2] - lastEventWorldPos[2] };
  this->Translate( translation );

  // Update our records
  this->LastEventPosition[0] = X;
  this->LastEventPosition[1] = Y;
}

//----------------------------------------------------------------------------
// Loop through all points and translate them
void vtkParallelopipedRepresentation::Translate(double translation[3])
{
  double *pts =
         static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
  for (int i=0; i<16; i++)
  {
    *pts++ += translation[0];
    *pts++ += translation[1];
    *pts++ += translation[2];
  }

  // Synchronize the handle representations with our recently updated
  // "Points" data-structure.
  this->PositionHandles();
}

//----------------------------------------------------------------------------
void vtkParallelopipedRepresentation::Scale( int vtkNotUsed(X), int Y )
{
  double *pts =
          static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
  double *center
    = static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(3*14);
  double sf = ( Y > this->LastEventPosition[1] ? 1.03 : 0.97 );

  for (int i=0; i<16; i++, pts+=3)
  {
    pts[0] = sf * (pts[0] - center[0]) + center[0];
    pts[1] = sf * (pts[1] - center[1]) + center[1];
    pts[2] = sf * (pts[2] - center[2]) + center[2];
  }

  // Synchronize the handle representations with our recently updated
  // "Points" data-structure.
  this->PositionHandles();
}

//----------------------------------------------------------------------------
void vtkParallelopipedRepresentation::PlaceWidget(double bounds[6])
{
  double corners[8][3] =
    { { bounds[0], bounds[2], bounds[4] },
      { bounds[1], bounds[2], bounds[4] },
      { bounds[1], bounds[3], bounds[4] },
      { bounds[0], bounds[3], bounds[4] },
      { bounds[0], bounds[2], bounds[5] },
      { bounds[1], bounds[2], bounds[5] },
      { bounds[1], bounds[3], bounds[5] },
      { bounds[0], bounds[3], bounds[5] } };

  this->PlaceWidget(corners);
}

//----------------------------------------------------------------------------
void vtkParallelopipedRepresentation::PlaceWidget(double corners[8][3])
{
  // Scale the corners of parallelopiped according to the place factor.
  // Note that the default place factor is 0.5. So if your corners
  // appear half way in, don't be surprised.
  //
  double center[3] = {0.0, 0.0, 0.0}, newCorners[8][3];
  for (int j = 0; j < 3; j++)
  {
    for (int i = 0; i < 8; center[j] += corners[i][j], i++)
    {
      ;
    }
    center[j] /= 8.0;

    for (int i = 0; i < 8; i++)
    {
      newCorners[i][j] = center[j] +
        this->PlaceFactor*(corners[i][j]-center[j]);
    }
  }

  for (int i = 0; i < 8; i++)
  {
    this->Points->SetPoint(i, newCorners[i]);
  }
  this->AbsoluteMinimumThickness =
    this->HexPolyData->GetLength()*this->MinimumThickness;

  this->ChairPointPlacer->SetMinimumDistance( 0.5 * this->AbsoluteMinimumThickness );

  // Initialize the chair points too
  for (int i = 8; i < 16; i++)
  {
    this->Points->SetPoint(i, newCorners[0]);
  }

  this->PositionHandles();
}

//----------------------------------------------------------------------------
void vtkParallelopipedRepresentation::GetPolyData(vtkPolyData *pd)
{
  pd->SetPoints(this->HexPolyData->GetPoints());
  pd->SetPolys(this->HexPolyData->GetPolys());
}

//----------------------------------------------------------------------------
double *vtkParallelopipedRepresentation::GetBounds()
{
  return this->Points->GetBounds();
}

//----------------------------------------------------------------------------
void vtkParallelopipedRepresentation::BuildRepresentation()
{
  this->Points->Modified();
}

//----------------------------------------------------------------------------
void vtkParallelopipedRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Minimum Thickness: " << this->MinimumThickness << "\n";

  if ( this->HandleProperty )
  {
    os << indent << "Handle Property: " << this->HandleProperty << "\n";
  }
  else
  {
    os << indent << "Handle Property: (none)\n";
  }

  if ( this->HoveredHandleProperty )
  {
    os << indent << "Hovered Handle Property: " << this->HoveredHandleProperty << "\n";
  }
  else
  {
    os << indent << "Hovered Handle Property: (none)\n";
  }

  if ( this->FaceProperty )
  {
    os << indent << "Face Property: " << this->FaceProperty << "\n";
  }
  else
  {
    os << indent << "Face Property: (none)\n";
  }

  if ( this->OutlineProperty )
  {
    os << indent << "Outline Property: " << this->OutlineProperty << "\n";
  }
  else
  {
    os << indent << "Outline Property: (none)\n";
  }

  if ( this->SelectedHandleProperty )
  {
    os << indent << "Selected Handle Property: " << this->SelectedHandleProperty << "\n";
  }
  else
  {
    os << indent << "Selected Handle Property: (none)\n";
  }

  if ( this->SelectedFaceProperty )
  {
    os << indent << "Selected Face Property: " << this->SelectedFaceProperty << "\n";
  }
  else
  {
    os << indent << "Selected Face Property: (none)\n";
  }

  if ( this->SelectedOutlineProperty )
  {
    os << indent << "Selected Outline Property: " << this->SelectedOutlineProperty << "\n";
  }
  else
  {
    os << indent << "Selected Outline Property: (none)\n";
  }

  // this->InteractionState is printed in superclass
  // this is commented to avoid PrintSelf errors
}
