/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredGrid.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellLinks.h"
#include "vtkConvexPointSet.h"
#include "vtkCubicLine.h"
#include "vtkEmptyCell.h"
#include "vtkGenericCell.h"
#include "vtkHexahedron.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLine.h"
#include "vtkObjectFactory.h"
#include "vtkPixel.h"
#include "vtkPointData.h"
#include "vtkPolyLine.h"
#include "vtkPolyVertex.h"
#include "vtkPolygon.h"
#include "vtkPolyhedron.h"
#include "vtkPyramid.h"
#include "vtkPentagonalPrism.h"
#include "vtkHexagonalPrism.h"
#include "vtkQuad.h"
#include "vtkQuadraticEdge.h"
#include "vtkQuadraticHexahedron.h"
#include "vtkQuadraticWedge.h"
#include "vtkQuadraticPyramid.h"
#include "vtkQuadraticQuad.h"
#include "vtkQuadraticTetra.h"
#include "vtkQuadraticTriangle.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkTriangleStrip.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVertex.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"
#include "vtkTriQuadraticHexahedron.h"
#include "vtkQuadraticLinearWedge.h"
#include "vtkQuadraticLinearQuad.h"
#include "vtkBiQuadraticQuad.h"
#include "vtkBiQuadraticQuadraticWedge.h"
#include "vtkBiQuadraticQuadraticHexahedron.h"
#include "vtkBiQuadraticTriangle.h"

#include <set>

vtkStandardNewMacro(vtkUnstructuredGrid);

vtkUnstructuredGrid::vtkUnstructuredGrid ()
{
  this->Vertex = NULL;
  this->PolyVertex = NULL;
  this->Line = NULL;
  this->PolyLine = NULL;
  this->Triangle = NULL;
  this->TriangleStrip = NULL;
  this->Pixel = NULL;
  this->Quad = NULL;
  this->Polygon = NULL;
  this->Tetra = NULL;
  this->Voxel = NULL;
  this->Hexahedron = NULL;
  this->Wedge = NULL;
  this->Pyramid = NULL;
  this->PentagonalPrism = NULL;
  this->HexagonalPrism = NULL;
  this->QuadraticEdge = NULL;
  this->QuadraticTriangle =NULL;
  this->QuadraticQuad = NULL;
  this->QuadraticTetra = NULL;
  this->QuadraticHexahedron = NULL;
  this->QuadraticWedge = NULL;
  this->QuadraticPyramid = NULL;
  this->QuadraticLinearQuad = NULL;
  this->BiQuadraticQuad = NULL;
  this->TriQuadraticHexahedron = NULL;
  this->QuadraticLinearWedge = NULL;
  this->BiQuadraticQuadraticWedge = NULL;
  this->BiQuadraticQuadraticHexahedron = NULL;
  this->BiQuadraticTriangle = NULL;
  this->CubicLine = NULL;

  this->ConvexPointSet = NULL;
  this->Polyhedron = NULL;
  this->EmptyCell = NULL;

  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_PIECES_EXTENT);
  this->Information->Set(vtkDataObject::DATA_PIECE_NUMBER(), -1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 0);

  this->Connectivity = NULL;
  this->Links = NULL;
  this->Types = NULL;
  this->Locations = NULL;

  this->Faces = NULL;
  this->FaceLocations = NULL;

  this->Allocate(1000,1000);
}

//----------------------------------------------------------------------------
// Allocate memory space for data insertion. Execute this method before
// inserting any cells into object.
void vtkUnstructuredGrid::Allocate (vtkIdType numCells, int extSize)
{
  if ( numCells < 1 )
    {
    numCells = 1000;
    }
  if ( extSize < 1 )
    {
    extSize = 1000;
    }

  if ( this->Connectivity )
    {
    this->Connectivity->UnRegister(this);
    }
  this->Connectivity = vtkCellArray::New();
  this->Connectivity->Allocate(numCells,4*extSize);
  this->Connectivity->Register(this);
  this->Connectivity->Delete();

  if ( this->Types )
    {
    this->Types->UnRegister(this);
    }
  this->Types = vtkUnsignedCharArray::New();
  this->Types->Allocate(numCells,extSize);
  this->Types->Register(this);
  this->Types->Delete();

  if ( this->Locations )
    {
    this->Locations->UnRegister(this);
    }
  this->Locations = vtkIdTypeArray::New();
  this->Locations->Allocate(numCells,extSize);
  this->Locations->Register(this);
  this->Locations->Delete();
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid::~vtkUnstructuredGrid()
{
  this->Cleanup();

  if(this->Vertex)
    {
    this->Vertex->Delete();
    }
  if(this->PolyVertex)
    {
    this->PolyVertex->Delete();
    }
  if(this->Line)
    {
    this->Line->Delete();
    }
  if(this->PolyLine)
    {
    this->PolyLine->Delete();
    }
  if(this->Triangle)
    {
    this->Triangle->Delete();
    }
  if(this->TriangleStrip)
    {
    this->TriangleStrip->Delete();
    }
  if(this->Pixel)
    {
    this->Pixel->Delete();
    }
  if(this->Quad)
    {
    this->Quad->Delete();
    }
  if(this->Polygon)
    {
    this->Polygon->Delete();
    }
  if(this->Tetra)
    {
    this->Tetra->Delete();
    }
  if(this->Voxel)
    {
    this->Voxel->Delete();
    }
  if(this->Hexahedron)
    {
    this->Hexahedron->Delete();
    }
  if(this->Wedge)
    {
    this->Wedge->Delete();
    }
  if(this->Pyramid)
    {
    this->Pyramid->Delete();
    }
  if(this->PentagonalPrism)
    {
    this->PentagonalPrism->Delete();
    }
  if(this->HexagonalPrism)
    {
    this->HexagonalPrism->Delete();
    }
  if(this->QuadraticEdge)
    {
    this->QuadraticEdge->Delete();
    }
  if(this->QuadraticTriangle)
    {
    this->QuadraticTriangle->Delete();
    }
  if(this->QuadraticQuad)
    {
    this->QuadraticQuad->Delete();
    }
  if(this->QuadraticTetra)
    {
    this->QuadraticTetra->Delete();
    }
  if(this->QuadraticHexahedron)
    {
    this->QuadraticHexahedron->Delete();
    }
  if(this->QuadraticWedge)
    {
    this->QuadraticWedge->Delete();
    }
  if(this->QuadraticPyramid)
    {
    this->QuadraticPyramid->Delete();
    }
  if(this->QuadraticLinearQuad)
    {
    this->QuadraticLinearQuad->Delete ();
    }
  if(this->BiQuadraticQuad)
    {
    this->BiQuadraticQuad->Delete ();
    }
  if(this->TriQuadraticHexahedron)
    {
    this->TriQuadraticHexahedron->Delete ();
    }
  if(this->QuadraticLinearWedge)
    {
    this->QuadraticLinearWedge->Delete ();
    }
  if(this->BiQuadraticQuadraticWedge)
    {
    this->BiQuadraticQuadraticWedge->Delete ();
    }
  if(this->BiQuadraticQuadraticHexahedron)
    {
    this->BiQuadraticQuadraticHexahedron->Delete ();
    }
  if(this->BiQuadraticTriangle)
    {
    this->BiQuadraticTriangle->Delete ();
    }
  if(this->CubicLine)
    {
    this->CubicLine->Delete ();
    }

  if(this->ConvexPointSet)
    {
    this->ConvexPointSet->Delete();
    }
  if(this->Polyhedron)
    {
    this->Polyhedron->Delete();
    }
  if(this->EmptyCell)
    {
    this->EmptyCell->Delete();
    }
}

//----------------------------------------------------------------------------
int vtkUnstructuredGrid::GetPiece()
{
  return this->Information->Get(vtkDataObject::DATA_PIECE_NUMBER());
}

//----------------------------------------------------------------------------
int vtkUnstructuredGrid::GetNumberOfPieces()
{
  return this->Information->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());
}

//----------------------------------------------------------------------------
int vtkUnstructuredGrid::GetGhostLevel()
{
  return this->Information->Get(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS());
}

//----------------------------------------------------------------------------
// Copy the geometric and topological structure of an input unstructured grid.
void vtkUnstructuredGrid::CopyStructure(vtkDataSet *ds)
{
  vtkUnstructuredGrid *ug=static_cast<vtkUnstructuredGrid *>(ds);
  vtkPointSet::CopyStructure(ds);

  if (this->Connectivity != ug->Connectivity)
    {
    if ( this->Connectivity )
      {
      this->Connectivity->UnRegister(this);
      }
    this->Connectivity = ug->Connectivity;
    if (this->Connectivity)
      {
      this->Connectivity->Register(this);
      }
    }

  if (this->Links != ug->Links)
    {
    if ( this->Links )
      {
      this->Links->UnRegister(this);
      }
    this->Links = ug->Links;
    if (this->Links)
      {
      this->Links->Register(this);
      }
    }

  if (this->Types != ug->Types)
    {
    if ( this->Types )
      {
      this->Types->UnRegister(this);
      }
    this->Types = ug->Types;
    if (this->Types)
      {
      this->Types->Register(this);
      }
    }

  if (this->Locations != ug->Locations)
    {
    if ( this->Locations )
      {
      this->Locations->UnRegister(this);
      }
    this->Locations = ug->Locations;
    if (this->Locations)
      {
      this->Locations->Register(this);
      }
    }

  if (this->Faces != ug->Faces)
    {
    if ( this->Faces )
      {
      this->Faces->UnRegister(this);
      }
    this->Faces = ug->Faces;
    if (this->Faces)
      {
      this->Faces->Register(this);
      }
    }

  if (this->FaceLocations != ug->FaceLocations)
    {
    if ( this->FaceLocations )
      {
      this->FaceLocations->UnRegister(this);
      }
    this->FaceLocations = ug->FaceLocations;
    if (this->FaceLocations)
      {
      this->FaceLocations->Register(this);
      }
    }
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::Cleanup()
{
  if ( this->Connectivity )
    {
    this->Connectivity->UnRegister(this);
    this->Connectivity = NULL;
    }

  if ( this->Links )
    {
    this->Links->UnRegister(this);
    this->Links = NULL;
    }

  if ( this->Types )
    {
    this->Types->UnRegister(this);
    this->Types = NULL;
    }

  if ( this->Locations )
    {
    this->Locations->UnRegister(this);
    this->Locations = NULL;
    }

  if ( this->Faces )
    {
    this->Faces->UnRegister(this);
    this->Faces = NULL;
    }

  if ( this->FaceLocations )
    {
    this->FaceLocations->UnRegister(this);
    this->FaceLocations = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::Initialize()
{
  vtkPointSet::Initialize();

  this->Cleanup();

  if(this->Information)
    {
    this->Information->Set(vtkDataObject::DATA_PIECE_NUMBER(), -1);
    this->Information->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 0);
    this->Information->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 0);
    }
}

//----------------------------------------------------------------------------
int vtkUnstructuredGrid::GetCellType(vtkIdType cellId)
{

  vtkDebugMacro(<< "Returning cell type " << static_cast<int>(this->Types->GetValue(cellId)));
  return static_cast<int>(this->Types->GetValue(cellId));
}

//----------------------------------------------------------------------------
vtkCell *vtkUnstructuredGrid::GetCell(vtkIdType cellId)
{
  vtkIdType i;
  vtkIdType loc;
  vtkCell *cell = NULL;
  vtkIdType *pts, numPts;

  loc = this->Locations->GetValue(cellId);
  vtkDebugMacro(<< "location = " <<  loc);
  this->Connectivity->GetCell(loc,numPts,pts);

  int cellType = static_cast<int>(this->Types->GetValue(cellId));
  switch (cellType)
    {
    case VTK_VERTEX:
      if(!this->Vertex)
        {
        this->Vertex = vtkVertex::New();
        }
      cell = this->Vertex;
      break;

    case VTK_POLY_VERTEX:
      if(!this->PolyVertex)
        {
        this->PolyVertex = vtkPolyVertex::New();
        }
      cell = this->PolyVertex;
      break;

    case VTK_LINE:
      if(!this->Line)
        {
        this->Line = vtkLine::New();
        }
      cell = this->Line;
      break;

    case VTK_POLY_LINE:
      if(!this->PolyLine)
        {
        this->PolyLine = vtkPolyLine::New();
        }
      cell = this->PolyLine;
      break;

    case VTK_TRIANGLE:
      if(!this->Triangle)
        {
        this->Triangle = vtkTriangle::New();
        }
      cell = this->Triangle;
      break;

    case VTK_TRIANGLE_STRIP:
      if(!this->TriangleStrip)
        {
        this->TriangleStrip = vtkTriangleStrip::New();
        }
      cell = this->TriangleStrip;
      break;

    case VTK_PIXEL:
      if(!this->Pixel)
        {
        this->Pixel = vtkPixel::New();
        }
      cell = this->Pixel;
      break;

    case VTK_QUAD:
      if(!this->Quad)
        {
        this->Quad = vtkQuad::New();
        }
      cell = this->Quad;
      break;

    case VTK_POLYGON:
      if(!this->Polygon)
        {
        this->Polygon = vtkPolygon::New();
        }
      cell = this->Polygon;
      break;

    case VTK_TETRA:
      if(!this->Tetra)
        {
        this->Tetra = vtkTetra::New();
        }
      cell = this->Tetra;
      break;

    case VTK_VOXEL:
      if(!this->Voxel)
        {
        this->Voxel = vtkVoxel::New();
        }
      cell = this->Voxel;
      break;

    case VTK_HEXAHEDRON:
      if(!this->Hexahedron)
        {
        this->Hexahedron = vtkHexahedron::New();
        }
      cell = this->Hexahedron;
      break;

    case VTK_WEDGE:
      if(!this->Wedge)
        {
        this->Wedge = vtkWedge::New();
        }
      cell = this->Wedge;
      break;

    case VTK_PYRAMID:
      if(!this->Pyramid)
        {
        this->Pyramid = vtkPyramid::New();
        }
      cell = this->Pyramid;
      break;

    case VTK_PENTAGONAL_PRISM:
      if(!this->PentagonalPrism)
        {
        this->PentagonalPrism = vtkPentagonalPrism::New();
        }
      cell = this->PentagonalPrism;
      break;

    case VTK_HEXAGONAL_PRISM:
      if(!this->HexagonalPrism)
        {
        this->HexagonalPrism = vtkHexagonalPrism::New();
        }
      cell = this->HexagonalPrism;
      break;

    case VTK_QUADRATIC_EDGE:
      if(!this->QuadraticEdge)
        {
        this->QuadraticEdge = vtkQuadraticEdge::New();
        }
      cell = this->QuadraticEdge;
      break;

    case VTK_QUADRATIC_TRIANGLE:
      if(!this->QuadraticTriangle)
        {
        this->QuadraticTriangle = vtkQuadraticTriangle::New();
        }
      cell = this->QuadraticTriangle;
      break;

    case VTK_QUADRATIC_QUAD:
      if(!this->QuadraticQuad)
        {
        this->QuadraticQuad = vtkQuadraticQuad::New();
        }
      cell = this->QuadraticQuad;
      break;

    case VTK_QUADRATIC_TETRA:
      if(!this->QuadraticTetra)
        {
        this->QuadraticTetra = vtkQuadraticTetra::New();
        }
      cell = this->QuadraticTetra;
      break;

    case VTK_QUADRATIC_HEXAHEDRON:
      if(!this->QuadraticHexahedron)
        {
        this->QuadraticHexahedron = vtkQuadraticHexahedron::New();
        }
      cell = this->QuadraticHexahedron;
      break;

    case VTK_QUADRATIC_WEDGE:
      if(!this->QuadraticWedge)
        {
        this->QuadraticWedge = vtkQuadraticWedge::New();
        }
      cell = this->QuadraticWedge;
      break;

    case VTK_QUADRATIC_PYRAMID:
      if(!this->QuadraticPyramid)
        {
        this->QuadraticPyramid = vtkQuadraticPyramid::New();
        }
      cell = this->QuadraticPyramid;
      break;

    case VTK_QUADRATIC_LINEAR_QUAD:
      if(!this->QuadraticLinearQuad)
        {
        this->QuadraticLinearQuad = vtkQuadraticLinearQuad::New();
        }
      cell = this->QuadraticLinearQuad;
      break;

    case VTK_BIQUADRATIC_QUAD:
      if(!this->BiQuadraticQuad)
        {
        this ->BiQuadraticQuad = vtkBiQuadraticQuad::New();
        }
      cell = this->BiQuadraticQuad;
      break;

    case VTK_TRIQUADRATIC_HEXAHEDRON:
      if(!this->TriQuadraticHexahedron)
        {
        this->TriQuadraticHexahedron = vtkTriQuadraticHexahedron::New();
        }
      cell = this->TriQuadraticHexahedron;
      break;

    case VTK_QUADRATIC_LINEAR_WEDGE:
      if(!this->QuadraticLinearWedge)
        {
        this->QuadraticLinearWedge = vtkQuadraticLinearWedge::New();
        }
      cell = this->QuadraticLinearWedge;
      break;

    case VTK_BIQUADRATIC_QUADRATIC_WEDGE:
      if(!this->BiQuadraticQuadraticWedge)
        {
        this->BiQuadraticQuadraticWedge = vtkBiQuadraticQuadraticWedge::New();
        }
      cell = this->BiQuadraticQuadraticWedge;
      break;

    case VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
      if(!this->BiQuadraticQuadraticHexahedron)
        {
        this->BiQuadraticQuadraticHexahedron = vtkBiQuadraticQuadraticHexahedron::New();
        }
      cell = this->BiQuadraticQuadraticHexahedron;
      break;
    case VTK_BIQUADRATIC_TRIANGLE:
      if(!this->BiQuadraticTriangle)
        {
        this->BiQuadraticTriangle = vtkBiQuadraticTriangle::New();
        }
      cell = this->BiQuadraticTriangle;
      break;
    case VTK_CUBIC_LINE:
      if(!this->CubicLine)
        {
        this->CubicLine = vtkCubicLine::New();
        }
      cell = this->CubicLine;
      break;

    case VTK_CONVEX_POINT_SET:
      if(!this->ConvexPointSet)
        {
        this->ConvexPointSet = vtkConvexPointSet::New();
        }
      cell = this->ConvexPointSet;
      break;

    case VTK_POLYHEDRON:
      if(!this->Polyhedron)
        {
        this->Polyhedron = vtkPolyhedron::New();
        }
      this->Polyhedron->SetFaces(this->GetFaces(cellId));
      cell = this->Polyhedron;
      break;

    case VTK_EMPTY_CELL:
      if(!this->EmptyCell)
        {
        this->EmptyCell = vtkEmptyCell::New();
        }
      cell = this->EmptyCell;
      break;
    }

  if( !cell )
    {
    return NULL;
    }

  // Copy the points over to the cell.
  cell->PointIds->SetNumberOfIds(numPts);
  cell->Points->SetNumberOfPoints(numPts);
  for (i=0; i<numPts; i++)
    {
    cell->PointIds->SetId(i,pts[i]);
    cell->Points->SetPoint(i,this->Points->GetPoint(pts[i]));
    }

  // Some cells require special initialization to build data structures
  // and such.
  if ( cell->RequiresInitialization() )
    {
    cell->Initialize();
    }

  return cell;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::GetCell(vtkIdType cellId, vtkGenericCell *cell)
{
  vtkIdType i;
  vtkIdType    loc;
  double  x[3];
  vtkIdType *pts, numPts;

  int cellType = static_cast<int>(this->Types->GetValue(cellId));
  cell->SetCellType(cellType);

  loc = this->Locations->GetValue(cellId);
  this->Connectivity->GetCell(loc,numPts,pts);

  cell->PointIds->SetNumberOfIds(numPts);
  cell->Points->SetNumberOfPoints(numPts);

  for (i=0; i<numPts; i++)
    {
    cell->PointIds->SetId(i,pts[i]);
    this->Points->GetPoint(pts[i], x);
    cell->Points->SetPoint(i, x);
    }

  // Explicit face representation
  if ( cell->RequiresExplicitFaceRepresentation() )
    {
    cell->SetFaces(this->GetFaces(cellId));
    }

  // Some cells require special initialization to build data structures
  // and such.
  if ( cell->RequiresInitialization() )
    {
    cell->Initialize();
    }
}

//----------------------------------------------------------------------------
// Fast implementation of GetCellBounds().  Bounds are calculated without
// constructing a cell.
void vtkUnstructuredGrid::GetCellBounds(vtkIdType cellId, double bounds[6])
{
  vtkIdType i;
  vtkIdType loc;
  double x[3];
  vtkIdType *pts, numPts;

  loc = this->Locations->GetValue(cellId);
  this->Connectivity->GetCell(loc,numPts,pts);

  // carefully compute the bounds
  if (numPts)
    {
    this->Points->GetPoint( pts[0], x );
    bounds[0] = x[0];
    bounds[2] = x[1];
    bounds[4] = x[2];
    bounds[1] = x[0];
    bounds[3] = x[1];
    bounds[5] = x[2];
    for (i=1; i < numPts; i++)
      {
      this->Points->GetPoint( pts[i], x );
      bounds[0] = (x[0] < bounds[0] ? x[0] : bounds[0]);
      bounds[1] = (x[0] > bounds[1] ? x[0] : bounds[1]);
      bounds[2] = (x[1] < bounds[2] ? x[1] : bounds[2]);
      bounds[3] = (x[1] > bounds[3] ? x[1] : bounds[3]);
      bounds[4] = (x[2] < bounds[4] ? x[2] : bounds[4]);
      bounds[5] = (x[2] > bounds[5] ? x[2] : bounds[5]);
      }
    }
  else
    {
    vtkMath::UninitializeBounds(bounds);
    }

}

//----------------------------------------------------------------------------
int vtkUnstructuredGrid::GetMaxCellSize()
{
  if (this->Connectivity)
    {
    return this->Connectivity->GetMaxCellSize();
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkUnstructuredGrid::GetNumberOfCells()
{
  vtkDebugMacro(<< "NUMBER OF CELLS = " <<
    (this->Connectivity ? this->Connectivity->GetNumberOfCells() : 0));
  return (this->Connectivity ? this->Connectivity->GetNumberOfCells() : 0);
}

//----------------------------------------------------------------------------
// Insert/create cell in object by type and list of point ids defining
// cell topology. Using a special input format, this function also support
// polyhedron cells.
vtkIdType vtkUnstructuredGrid::InsertNextCell(int type, vtkIdList *ptIds)
{
  if (type == VTK_POLYHEDRON)
    {
    // For polyhedron cell, input ptIds is of format:
    // (numCellFaces, numFace0Pts, id1, id2, id3, numFace1Pts,id1, id2, id3, ...)
    vtkIdType* dataPtr = ptIds->GetPointer(0);
    return this->InsertNextCell(type, dataPtr[0], dataPtr+1);
    }

  vtkIdType npts = ptIds->GetNumberOfIds();
  // insert connectivity
  this->Connectivity->InsertNextCell(ptIds);
  // insert type and storage information
  vtkDebugMacro(<< "insert location "
                << this->Connectivity->GetInsertLocation(npts));
  this->Locations->InsertNextValue(this->Connectivity->GetInsertLocation(npts));

  // If faces have been created, we need to pad them (we are not creating
  // a polyhedral cell in this method)
  if ( this->FaceLocations )
    {
    this->FaceLocations->InsertNextValue(-1);
    }

  // insert cell type
  return this->Types->InsertNextValue(static_cast<unsigned char>(type));
}

//----------------------------------------------------------------------------
// Insert/create cell in object by type and list of point ids defining
// cell topology. Using a special input format, this function also support
// polyhedron cells.
vtkIdType vtkUnstructuredGrid::InsertNextCell(int type, vtkIdType npts,
                                              vtkIdType *ptIds)
{
  if (type != VTK_POLYHEDRON)
    {
    // insert connectivity
    this->Connectivity->InsertNextCell(npts,ptIds);
    // insert type and storage information
    vtkDebugMacro(<< "insert location "
                  << this->Connectivity->GetInsertLocation(npts));
    this->Locations->InsertNextValue(
      this->Connectivity->GetInsertLocation(npts));

    // If faces have been created, we need to pad them (we are not creating
    // a polyhedral cell in this method)
    if ( this->FaceLocations )
      {
      this->FaceLocations->InsertNextValue(-1);
      }
    }
  else
    {
    // For polyhedron, npts is actually number of faces, ptIds is of format:
    // (numFace0Pts, id1, id2, id3, numFace1Pts,id1, id2, id3, ...)
    vtkIdType realnpts;

    // We defer allocation for the faces because they are not commonly used and
    // we only want to allocate when necessary.
    if ( ! this->Faces )
      {
      this->Faces = vtkIdTypeArray::New();
      this->Faces->Allocate(this->Types->GetSize());
      this->FaceLocations = vtkIdTypeArray::New();
      this->FaceLocations->Allocate(this->Types->GetSize());
      // FaceLocations must be padded until the current position
      for(vtkIdType i = 0; i <= this->Types->GetMaxId(); i++)
        {
        this->FaceLocations->InsertNextValue(-1);
        }
      }

    // insert cell location
    this->Locations->InsertNextValue(this->Connectivity->GetData()->GetMaxId()+1);
    // insert face location
    this->FaceLocations->InsertNextValue(this->Faces->GetMaxId()+1);
    // insert cell connectivity and faces stream
    vtkUnstructuredGrid::DecomposeAPolyhedronCell(
        npts, ptIds, realnpts, this->Connectivity, this->Faces);
    }

  return this->Types->InsertNextValue(static_cast<unsigned char>(type));
}

//----------------------------------------------------------------------------
// Insert/create cell in object by type and list of point and face ids
// defining cell topology. This method is meant for face-explicit cells (e.g.
// polyhedron).
vtkIdType vtkUnstructuredGrid::
InsertNextCell(int type, vtkIdType npts, vtkIdType *pts,
               vtkIdType nfaces, vtkIdType *faces)
{
  // Insert connectivity (points that make up polyhedron)
  this->Connectivity->InsertNextCell(npts,pts);

  // Insert location of cell in connectivity array
  this->Locations->InsertNextValue(
    this->Connectivity->GetInsertLocation(npts));

  // Now insert faces; allocate storage if necessary.
  // We defer allocation for the faces because they are not commonly used and
  // we only want to allocate when necessary.
  if ( ! this->Faces )
    {
    this->Faces = vtkIdTypeArray::New();
    this->Faces->Allocate(this->Types->GetSize());
    this->FaceLocations = vtkIdTypeArray::New();
    this->FaceLocations->Allocate(this->Types->GetSize());
    // FaceLocations must be padded until the current position
    for(vtkIdType i = 0; i <= this->Types->GetMaxId(); i++)
      {
      this->FaceLocations->InsertNextValue(-1);
      }
    }

  // Okay the faces go in
  this->FaceLocations->InsertNextValue(
    this->Faces->GetMaxId() + 1);
  this->Faces->InsertNextValue(nfaces);
  vtkIdType i, *face=faces;
  for (int faceNum=0; faceNum < nfaces; ++faceNum)
    {
    npts = face[0];
    this->Faces->InsertNextValue(npts);
    for (i=1; i <= npts; ++i)
      {
      this->Faces->InsertNextValue(face[i]);
      }
    face += npts + 1;
    } //for all faces

  return this->Types->InsertNextValue(static_cast<unsigned char>(type));
}


//----------------------------------------------------------------------------
int vtkUnstructuredGrid::InitializeFacesRepresentation(vtkIdType numPrevCells)
{
  if (this->Faces || this->FaceLocations)
    {
    vtkErrorMacro("Face information already exist for this unstuructured grid. "
                  "InitializeFacesRepresentation returned without execution.");
    return 0;
    }

  this->Faces = vtkIdTypeArray::New();
  this->Faces->Allocate(this->Types->GetSize());

  this->FaceLocations = vtkIdTypeArray::New();
  this->FaceLocations->Allocate(this->Types->GetSize());
  // FaceLocations must be padded until the current position
  for(vtkIdType i = 0; i < numPrevCells; i++)
    {
    this->FaceLocations->InsertNextValue(-1);
    }

  return 1;
}

//----------------------------------------------------------------------------
// Return faces for a polyhedral cell (or face-explicit cell).
vtkIdType *vtkUnstructuredGrid::GetFaces(vtkIdType cellId)
{
  // Get the locations of the face
  vtkIdType loc;
  if ( !this->Faces ||
       cellId < 0 || cellId > this->FaceLocations->GetMaxId() ||
       (loc=this->FaceLocations->GetValue(cellId)) == -1 )
    {
    return NULL;
    }

  return this->Faces->GetPointer(loc);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::SetCells(int type, vtkCellArray *cells)
{
  int *types = new int [cells->GetNumberOfCells()];
  for (vtkIdType i = 0; i < cells->GetNumberOfCells(); i++)
    {
    types[i] = type;
    }

  this->SetCells(types, cells);

  delete [] types;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::SetCells(int *types, vtkCellArray *cells)
{
  // check if cells contain any polyhedron cell
  vtkIdType ncells = cells->GetNumberOfCells();
  bool containPolyhedron = false;
  vtkIdType i;
  for (i = 0; i < ncells; i++)
    {
    if (types[i] == VTK_POLYHEDRON)
      {
      containPolyhedron = true;
      }
    }

  vtkIdType npts, nfaces, realnpts, *pts;

  vtkIdTypeArray *cellLocations = vtkIdTypeArray::New();
  cellLocations->Allocate(ncells);
  vtkUnsignedCharArray *cellTypes = vtkUnsignedCharArray::New();
  cellTypes->Allocate(ncells);

  if (!containPolyhedron)
    {
    // only need to build types and locations
    for (i=0, cells->InitTraversal(); cells->GetNextCell(npts,pts); i++)
      {
      cellTypes->InsertNextValue(static_cast<unsigned char>(types[i]));
      cellLocations->InsertNextValue(cells->GetTraversalLocation(npts));
      }

    this->SetCells(cellTypes, cellLocations, cells, NULL, NULL);

    cellTypes->Delete();
    cellLocations->Delete();
    return;
    }

  // If a polyhedron cell exists, its input cellArray is of special format.
  // [nCell0Faces, nFace0Pts, i, j, k, nFace1Pts, i, j, k, ...]
  // We need to convert it into new cell connectivities of standard format,
  // update cellLocations as well as create faces and facelocations.
  vtkCellArray   *newCells = vtkCellArray::New();
  newCells->Allocate(cells->GetActualMemorySize());
  vtkIdTypeArray *faces = vtkIdTypeArray::New();
  faces->Allocate(cells->GetActualMemorySize());
  vtkIdTypeArray *faceLocations = vtkIdTypeArray::New();
  faceLocations->Allocate(ncells);

  for (i=0, cells->InitTraversal(); cells->GetNextCell(npts,pts); i++)
    {
    cellTypes->InsertNextValue(static_cast<unsigned char>(types[i]));
    cellLocations->InsertNextValue(newCells->GetData()->GetMaxId()+1);
    if (types[i] != VTK_POLYHEDRON)
      {
      newCells->InsertNextCell(npts, pts);
      faceLocations->InsertNextValue(-1);
      }
    else
      {
      faceLocations->InsertNextValue(faces->GetMaxId()+1);
      vtkUnstructuredGrid::DecomposeAPolyhedronCell(
        pts, realnpts, nfaces, newCells, faces);
      }
    }

  this->SetCells(cellTypes, cellLocations, newCells, faceLocations, faces);

  cellTypes->Delete();
  cellLocations->Delete();
  newCells->Delete();
  faces->Delete();
  faceLocations->Delete();
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::SetCells(vtkUnsignedCharArray *cellTypes,
                                   vtkIdTypeArray *cellLocations,
                                   vtkCellArray *cells)
{
  // check if cells contain any polyhedron cell
  vtkIdType ncells = cells->GetNumberOfCells();
  bool containPolyhedron = false;
  vtkIdType i;
  for (i = 0; i < ncells; i++)
    {
    if (cellTypes->GetValue(i) == VTK_POLYHEDRON)
      {
      containPolyhedron = true;
      }
    }

  // directly set connectivity and location if there is no polyhedron
  if (!containPolyhedron)
    {
    this->SetCells(cellTypes, cellLocations, cells, NULL, NULL);
    return;
    }

  // If a polyhedron cell exists, its input cellArray is of special format.
  // [nCell0Faces, nFace0Pts, i, j, k, nFace1Pts, i, j, k, ...]
  // We need to convert it into new cell connectivities of standard format,
  // update cellLocations as well as create faces and facelocations.
  vtkCellArray   *newCells = vtkCellArray::New();
  newCells->Allocate(cells->GetActualMemorySize());
  vtkIdTypeArray *newCellLocations = vtkIdTypeArray::New();
  newCellLocations->Allocate(ncells);
  vtkIdTypeArray *faces = vtkIdTypeArray::New();
  faces->Allocate(cells->GetActualMemorySize());
  vtkIdTypeArray *faceLocations = vtkIdTypeArray::New();
  faceLocations->Allocate(ncells);

  vtkIdType npts, nfaces, realnpts, *pts;
  for (i=0, cells->InitTraversal(); cells->GetNextCell(npts,pts); i++)
    {
    newCellLocations->InsertNextValue(newCells->GetData()->GetMaxId()+1);
    if (cellTypes->GetValue(i) != VTK_POLYHEDRON)
      {
      newCells->InsertNextCell(npts, pts);
      faceLocations->InsertNextValue(-1);
      }
    else
      {
      faceLocations->InsertNextValue(faces->GetMaxId()+1);
      vtkUnstructuredGrid::DecomposeAPolyhedronCell(
        pts, realnpts, nfaces, newCells, faces);
      }
    }

  // set the new cells
  this->SetCells(cellTypes, newCellLocations, newCells, faceLocations, faces);

  newCells->Delete();
  newCellLocations->Delete();
  faces->Delete();
  faceLocations->Delete();
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::SetCells(vtkUnsignedCharArray *cellTypes,
                                   vtkIdTypeArray *cellLocations,
                                   vtkCellArray *cells,
                                   vtkIdTypeArray *faceLocations,
                                   vtkIdTypeArray *faces)
{
  if ( this->Connectivity )
    {
    this->Connectivity->UnRegister(this);
    }
  this->Connectivity = cells;
  if ( this->Connectivity )
    {
    this->Connectivity->Register(this);
    }

  if ( this->Types )
    {
    this->Types->UnRegister(this);
    }
  this->Types = cellTypes;
  if ( this->Types )
    {
    this->Types->Register(this);
    }

  if ( this->Locations )
    {
    this->Locations->UnRegister(this);
    }
  this->Locations = cellLocations;
  if ( this->Locations )
    {
    this->Locations->Register(this);
    }

  if ( this->Faces )
    {
    this->Faces->UnRegister(this);
    }
  this->Faces = faces;
  if ( this->Faces )
    {
    this->Faces->Register(this);
    }

  if ( this->FaceLocations )
    {
    this->FaceLocations->UnRegister(this);
    }
  this->FaceLocations = faceLocations;
  if ( this->FaceLocations )
    {
    this->FaceLocations->Register(this);
    }
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::BuildLinks()
{
  // Remove the old links if they are already built
  if (this->Links)
    {
    this->Links->UnRegister(this);
    }

  this->Links = vtkCellLinks::New();
  this->Links->Allocate(this->GetNumberOfPoints());
  this->Links->Register(this);
  this->Links->BuildLinks(this, this->Connectivity);
  this->Links->Delete();
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::GetCellPoints(vtkIdType cellId, vtkIdList *ptIds)
{
  vtkIdType i, loc;
  vtkIdType *pts, numPts;

  loc = this->Locations->GetValue(cellId);
  this->Connectivity->GetCell(loc,numPts,pts);
  ptIds->SetNumberOfIds(numPts);
  for (i=0; i<numPts; i++)
    {
    ptIds->SetId(i,pts[i]);
    }

}

//----------------------------------------------------------------------------
// Return a pointer to a list of point ids defining cell. (More efficient than alternative
// method.)
void vtkUnstructuredGrid::GetCellPoints(vtkIdType cellId, vtkIdType& npts,
                                        vtkIdType* &pts)
{
  vtkIdType loc;

  loc = this->Locations->GetValue(cellId);

  this->Connectivity->GetCell(loc,npts,pts);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::GetFaceStream(vtkIdType cellId, vtkIdList *ptIds)
{
  if (this->GetCellType(cellId) != VTK_POLYHEDRON)
    {
    this->GetCellPoints(cellId, ptIds);
    return;
    }

  if (!this->Faces || !this->FaceLocations)
    {
    return;
    }

  ptIds->Reset();

  vtkIdType loc = this->FaceLocations->GetValue(cellId);
  vtkIdType* facePtr = this->Faces->GetPointer(loc);

  vtkIdType nfaces = *facePtr++;
  ptIds->InsertNextId(nfaces);
  for (vtkIdType i = 0; i < nfaces; i++)
    {
    vtkIdType npts = *facePtr++;
    ptIds->InsertNextId(npts);
    for (vtkIdType j = 0; j < npts; j++)
      {
      ptIds->InsertNextId(*facePtr++);
      }
    }
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::GetFaceStream(vtkIdType cellId, vtkIdType& nfaces,
                                        vtkIdType* &ptIds)
{
  if (this->GetCellType(cellId) != VTK_POLYHEDRON)
    {
    this->GetCellPoints(cellId, nfaces, ptIds);
    return;
    }

  if (!this->Faces || !this->FaceLocations)
    {
    return;
    }

  vtkIdType loc = this->FaceLocations->GetValue(cellId);
  vtkIdType* facePtr = this->Faces->GetPointer(loc);

  nfaces = *facePtr;
  ptIds = facePtr+1;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::GetPointCells(vtkIdType ptId, vtkIdList *cellIds)
{
  vtkIdType *cells;
  int numCells;
  int i;

  if ( ! this->Links )
    {
    this->BuildLinks();
    }
  cellIds->Reset();

  numCells = this->Links->GetNcells(ptId);
  cells = this->Links->GetCells(ptId);

  cellIds->SetNumberOfIds(numCells);
  for (i=0; i < numCells; i++)
    {
    cellIds->SetId(i,cells[i]);
    }
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::Reset()
{
  if ( this->Connectivity )
    {
    this->Connectivity->Reset();
    }
  if ( this->Links )
    {
    this->Links->Reset();
    }
  if ( this->Types )
    {
    this->Types->Reset();
    }
  if ( this->Locations )
    {
    this->Locations->Reset();
    }
  if ( this->Faces )
    {
    this->Faces->Reset();
    }
  if ( this->FaceLocations )
    {
    this->FaceLocations->Reset();
    }
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::Squeeze()
{
  if ( this->Connectivity )
    {
    this->Connectivity->Squeeze();
    }
  if ( this->Links )
    {
    this->Links->Squeeze();
    }
  if ( this->Types )
    {
    this->Types->Squeeze();
    }
  if ( this->Locations )
    {
    this->Locations->Squeeze();
    }
  if ( this->Faces )
    {
    this->Faces->Squeeze();
    }
  if ( this->FaceLocations )
    {
    this->FaceLocations->Squeeze();
    }

  vtkPointSet::Squeeze();
}

//----------------------------------------------------------------------------
// Remove a reference to a cell in a particular point's link list. You may
// also consider using RemoveCellReference() to remove the references from
// all the cell's points to the cell. This operator does not reallocate
// memory; use the operator ResizeCellList() to do this if necessary.
void vtkUnstructuredGrid::RemoveReferenceToCell(vtkIdType ptId,
                                                vtkIdType cellId)
{
  this->Links->RemoveCellReference(cellId, ptId);
}

//----------------------------------------------------------------------------
// Add a reference to a cell in a particular point's link list. (You may also
// consider using AddCellReference() to add the references from all the
// cell's points to the cell.) This operator does not realloc memory; use the
// operator ResizeCellList() to do this if necessary.
void vtkUnstructuredGrid::AddReferenceToCell(vtkIdType ptId, vtkIdType cellId)
{
  this->Links->AddCellReference(cellId, ptId);
}

//----------------------------------------------------------------------------
// Resize the list of cells using a particular point. (This operator assumes
// that BuildLinks() has been called.)
void vtkUnstructuredGrid::ResizeCellList(vtkIdType ptId, int size)
{
  this->Links->ResizeCellList(ptId,size);
}

//----------------------------------------------------------------------------
// Replace the points defining cell "cellId" with a new set of points. This
// operator is (typically) used when links from points to cells have not been
// built (i.e., BuildLinks() has not been executed). Use the operator
// ReplaceLinkedCell() to replace a cell when cell structure has been built.
void vtkUnstructuredGrid::ReplaceCell(vtkIdType cellId, int npts,
                                      vtkIdType *pts)
{
  vtkIdType loc;

  loc = this->Locations->GetValue(cellId);
  this->Connectivity->ReplaceCell(loc,npts,pts);
}

//----------------------------------------------------------------------------
// Add a new cell to the cell data structure (after cell links have been
// built). This method adds the cell and then updates the links from the points
// to the cells. (Memory is allocated as necessary.)
vtkIdType vtkUnstructuredGrid::InsertNextLinkedCell(int type, int npts,
                                                    vtkIdType *pts)
{
  vtkIdType i, id;

  id = this->InsertNextCell(type,npts,pts);

  for (i=0; i<npts; i++)
    {
    this->Links->ResizeCellList(pts[i],1);
    this->Links->AddCellReference(id,pts[i]);
    }

  return id;
}

//----------------------------------------------------------------------------
unsigned long vtkUnstructuredGrid::GetActualMemorySize()
{
  unsigned long size=this->vtkPointSet::GetActualMemorySize();
  if ( this->Connectivity )
    {
    size += this->Connectivity->GetActualMemorySize();
    }

  if ( this->Links )
    {
    size += this->Links->GetActualMemorySize();
    }

  if ( this->Types )
    {
    size += this->Types->GetActualMemorySize();
    }

  if ( this->Locations )
    {
    size += this->Locations->GetActualMemorySize();
    }

  if ( this->Faces )
    {
    size += this->Faces->GetActualMemorySize();
    }

  if ( this->FaceLocations )
    {
    size += this->FaceLocations->GetActualMemorySize();
    }

  return size;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::ShallowCopy(vtkDataObject *dataObject)
{
  vtkUnstructuredGrid *grid = vtkUnstructuredGrid::SafeDownCast(dataObject);

  if ( grid != NULL )
    {
    // I do not know if this is correct but.

    if (this->Connectivity)
      {
      this->Connectivity->UnRegister(this);
      }
    this->Connectivity = grid->Connectivity;
    if (this->Connectivity)
      {
      this->Connectivity->Register(this);
      }

    if (this->Links)
      {
      this->Links->Delete();
      }
    this->Links = grid->Links;
    if (this->Links)
      {
      this->Links->Register(this);
      }

    if (this->Types)
      {
      this->Types->UnRegister(this);
      }
    this->Types = grid->Types;
    if (this->Types)
      {
      this->Types->Register(this);
      }

    if (this->Locations)
      {
      this->Locations->UnRegister(this);
      }
    this->Locations = grid->Locations;
    if (this->Locations)
      {
      this->Locations->Register(this);
      }

    if (this->Faces)
      {
      this->Faces->UnRegister(this);
      }
    this->Faces = grid->Faces;
    if (this->Faces)
      {
      this->Faces->Register(this);
      }

    if (this->FaceLocations)
      {
      this->FaceLocations->UnRegister(this);
      }
    this->FaceLocations = grid->FaceLocations;
    if (this->FaceLocations)
      {
      this->FaceLocations->Register(this);
      }

    }

  // Do superclass
  this->vtkPointSet::ShallowCopy(dataObject);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::DeepCopy(vtkDataObject *dataObject)
{
  vtkUnstructuredGrid *grid = vtkUnstructuredGrid::SafeDownCast(dataObject);

  if ( grid != NULL )
    {
    if ( this->Connectivity )
      {
      this->Connectivity->UnRegister(this);
      this->Connectivity = NULL;
      }
    if (grid->Connectivity)
      {
      this->Connectivity = vtkCellArray::New();
      this->Connectivity->DeepCopy(grid->Connectivity);
      this->Connectivity->Register(this);
      this->Connectivity->Delete();
      }

    if ( this->Links )
      {
      this->Links->UnRegister(this);
      this->Links = NULL;
      }
    if ( this->Types )
      {
      this->Types->UnRegister(this);
      this->Types = NULL;
      }
    if (grid->Types)
      {
      this->Types = vtkUnsignedCharArray::New();
      this->Types->DeepCopy(grid->Types);
      this->Types->Register(this);
      this->Types->Delete();
      }

    if ( this->Locations )
      {
      this->Locations->UnRegister(this);
      this->Locations = NULL;
      }
    if (grid->Locations)
      {
      this->Locations = vtkIdTypeArray::New();
      this->Locations->DeepCopy(grid->Locations);
      this->Locations->Register(this);
      this->Locations->Delete();
      }

    if ( this->Faces )
      {
      this->Faces->UnRegister(this);
      this->Faces = NULL;
      }
    if (grid->Faces)
      {
      this->Faces = vtkIdTypeArray::New();
      this->Faces->DeepCopy(grid->Faces);
      this->Faces->Register(this);
      this->Faces->Delete();
      }

    if ( this->FaceLocations )
      {
      this->FaceLocations->UnRegister(this);
      this->FaceLocations = NULL;
      }
    if (grid->FaceLocations)
      {
      this->FaceLocations = vtkIdTypeArray::New();
      this->FaceLocations->DeepCopy(grid->FaceLocations);
      this->FaceLocations->Register(this);
      this->FaceLocations->Delete();
      }
    }

  // Do superclass
  this->vtkPointSet::DeepCopy(dataObject);

  // Finally Build Links if we need to
  if (grid && grid->Links)
    {
    this->BuildLinks();
    }
}


//----------------------------------------------------------------------------
void vtkUnstructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number Of Pieces: " << this->GetNumberOfPieces() << endl;
  os << indent << "Piece: " << this->GetPiece() << endl;
  os << indent << "Ghost Level: " << this->GetGhostLevel() << endl;
}

//----------------------------------------------------------------------------
// Determine neighbors as follows. Find the (shortest) list of cells that
// uses one of the points in ptIds. For each cell, in the list, see whether
// it contains the other points in the ptIds list. If so, it's a neighbor.
//
void vtkUnstructuredGrid::GetCellNeighbors(vtkIdType cellId, vtkIdList *ptIds,
                                           vtkIdList *cellIds)
{
  vtkIdType i, j, k;
  vtkIdType numPts, minNumCells, numCells;
  vtkIdType *pts, ptId, *cellPts, *cells;
  vtkIdType *minCells = NULL;
  vtkIdType match;
  vtkIdType minPtId = 0, npts;

  if ( ! this->Links )
    {
    this->BuildLinks();
    }

  cellIds->Reset();

  //Find the point used by the fewest number of cells
  //
  numPts = ptIds->GetNumberOfIds();
  pts = ptIds->GetPointer(0);
  for (minNumCells=VTK_LARGE_INTEGER,i=0; i<numPts; i++)
    {
    ptId = pts[i];
    numCells = this->Links->GetNcells(ptId);
    cells = this->Links->GetCells(ptId);
    if ( numCells < minNumCells )
      {
      minNumCells = numCells;
      minCells = cells;
      minPtId = ptId;
      }
    }

  if (minNumCells == VTK_LARGE_INTEGER && numPts == 0) {
    vtkErrorMacro("input point ids empty.");
    minNumCells = 0;
  }
  //Now for each cell, see if it contains all the points
  //in the ptIds list.
  for (i=0; i<minNumCells; i++)
    {
    if ( minCells[i] != cellId ) //don't include current cell
      {
      this->GetCellPoints(minCells[i],npts,cellPts);
      for (match=1, j=0; j<numPts && match; j++) //for all pts in input cell
        {
        if ( pts[j] != minPtId ) //of course minPtId is contained by cell
          {
          for (match=k=0; k<npts; k++) //for all points in candidate cell
            {
            if ( pts[j] == cellPts[k] )
              {
              match = 1; //a match was found
              break;
              }
            }//for all points in current cell
          }//if not guaranteed match
        }//for all points in input cell
      if ( match )
        {
        cellIds->InsertNextId(minCells[i]);
        }
      }//if not the reference cell
    }//for all candidate cells attached to point
}


//----------------------------------------------------------------------------
int vtkUnstructuredGrid::IsHomogeneous()
{
  unsigned char type;
  if (this->Types && this->Types->GetMaxId() >= 0)
    {
    type = Types->GetValue(0);
    for (int cellId = 0; cellId < this->GetNumberOfCells(); cellId++)
      {
      if (this->Types->GetValue(cellId) != type)
        {
        return 0;
        }
      }
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
// Fill container with indices of cells which match given type.
void vtkUnstructuredGrid::GetIdsOfCellsOfType(int type, vtkIdTypeArray *array)
{
  for (int cellId = 0; cellId < this->GetNumberOfCells(); cellId++)
    {
    if (static_cast<int>(Types->GetValue(cellId)) == type)
      {
      array->InsertNextValue(cellId);
      }
    }
}


//----------------------------------------------------------------------------
void vtkUnstructuredGrid::RemoveGhostCells(int level)
{
  vtkUnstructuredGrid* newGrid = vtkUnstructuredGrid::New();
  vtkDataArray* temp;
  unsigned char* cellGhostLevels;

  vtkIdType cellId, newCellId;
  vtkIdList *cellPts, *pointMap;
  vtkIdList *newCellPts;
  vtkCell *cell;
  vtkPoints *newPoints;
  vtkIdType i, ptId, newId, numPts;
  vtkIdType numCellPts;
  double *x;
  vtkPointData*   pd    = this->GetPointData();
  vtkPointData*   outPD = newGrid->GetPointData();
  vtkCellData*    cd    = this->GetCellData();
  vtkCellData*    outCD = newGrid->GetCellData();


  // Get a pointer to the cell ghost level array.
  temp = this->CellData->GetArray("vtkGhostLevels");
  if (temp == NULL)
    {
    vtkDebugMacro("Could not find cell ghost level array.");
    newGrid->Delete();
    return;
    }
  if ( (temp->GetDataType() != VTK_UNSIGNED_CHAR)
       || (temp->GetNumberOfComponents() != 1)
       || (temp->GetNumberOfTuples() < this->GetNumberOfCells()))
    {
    vtkErrorMacro("Poorly formed ghost level array.");
    newGrid->Delete();
    return;
    }
  cellGhostLevels =(static_cast<vtkUnsignedCharArray*>(temp))->GetPointer(0);


  // Now threshold based on the cell ghost level array.
  outPD->CopyAllocate(pd);
  outCD->CopyAllocate(cd);

  numPts = this->GetNumberOfPoints();
  newGrid->Allocate(this->GetNumberOfCells());
  newPoints = vtkPoints::New();
  newPoints->Allocate(numPts);

  pointMap = vtkIdList::New(); //maps old point ids into new
  pointMap->SetNumberOfIds(numPts);
  for (i=0; i < numPts; i++)
    {
    pointMap->SetId(i,-1);
    }


  newCellPts = vtkIdList::New();

  // Check that the scalars of each cell satisfy the threshold criterion
  for (cellId=0; cellId < this->GetNumberOfCells(); cellId++)
    {
    cell = this->GetCell(cellId);
    cellPts = cell->GetPointIds();
    numCellPts = cell->GetNumberOfPoints();

    if ( cellGhostLevels[cellId] < level ) // Keep the cell.
      {
      for (i=0; i < numCellPts; i++)
        {
        ptId = cellPts->GetId(i);
        if ( (newId = pointMap->GetId(ptId)) < 0 )
          {
          x = this->GetPoint(ptId);
          newId = newPoints->InsertNextPoint(x);
          pointMap->SetId(ptId,newId);
          outPD->CopyData(pd,ptId,newId);
          }
        newCellPts->InsertId(i,newId);
        }
      newCellId = newGrid->InsertNextCell(cell->GetCellType(),newCellPts);
      outCD->CopyData(cd,cellId,newCellId);
      newCellPts->Reset();
      } // satisfied thresholding
    } // for all cells

  // now clean up / update ourselves
  pointMap->Delete();
  newCellPts->Delete();

  newGrid->SetPoints(newPoints);
  newPoints->Delete();

  this->CopyStructure(newGrid);
  this->GetPointData()->ShallowCopy(newGrid->GetPointData());
  this->GetCellData()->ShallowCopy(newGrid->GetCellData());
  newGrid->Delete();
  newGrid = NULL;

  this->Squeeze();
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::DecomposeAPolyhedronCell(vtkCellArray * polyhedronCell,
       vtkIdType & numCellPts, vtkIdType & nCellfaces,
       vtkCellArray * cellArray, vtkIdTypeArray * faces)
{
  vtkIdType *cellStream = 0;
  vtkIdType cellLength = 0;

  polyhedronCell->InitTraversal();
  polyhedronCell->GetNextCell(cellLength, cellStream);

  vtkUnstructuredGrid::DecomposeAPolyhedronCell(
    cellStream, numCellPts, nCellfaces, cellArray, faces);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::DecomposeAPolyhedronCell(vtkIdType *cellStream,
       vtkIdType & numCellPts, vtkIdType & nCellFaces,
       vtkCellArray * cellArray, vtkIdTypeArray * faces)
{
  nCellFaces = cellStream[0];
  if (nCellFaces <= 0)
    {
    return;
    }

  vtkUnstructuredGrid::DecomposeAPolyhedronCell(
    nCellFaces, cellStream+1, numCellPts, cellArray, faces);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::DecomposeAPolyhedronCell(vtkIdType nCellFaces,
       vtkIdType * cellStream, vtkIdType & numCellPts,
       vtkCellArray * cellArray, vtkIdTypeArray * faces)
{
  std::set<vtkIdType>  cellPointSet;
  std::set<vtkIdType>::iterator  it;

  // insert number of faces into the face array
  faces->InsertNextValue(nCellFaces);

  // for each face
  for (vtkIdType fid = 0; fid < nCellFaces; fid++)
    {
    // extract all points on the same face, store them into a set
    vtkIdType npts = *cellStream++;
    faces->InsertNextValue(npts);
    for (vtkIdType i = 0; i < npts; i++)
      {
      vtkIdType pid = *cellStream++;
      faces->InsertNextValue(pid);
      cellPointSet.insert(pid);
      }
    }

  // standard cell connectivity array that stores the number of points plus
  // a list of point ids.
  cellArray->InsertNextCell(static_cast<int>(cellPointSet.size()));
  for (it = cellPointSet.begin(); it != cellPointSet.end(); ++it)
    {
    cellArray->InsertCellPoint(*it);
    }

  // the real number of points in the polyhedron cell.
  numCellPts = static_cast<vtkIdType>(cellPointSet.size());
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::ConvertFaceStreamPointIds(vtkIdList * faceStream,
                                                    vtkIdType * idMap)
{
  vtkIdType* idPtr = faceStream->GetPointer(0);
  vtkIdType nfaces = *idPtr++;
  for (vtkIdType i = 0; i < nfaces; i++)
    {
    vtkIdType npts = *idPtr++;
    for (vtkIdType j = 0; j < npts; j++)
      {
      *idPtr = idMap[*idPtr];
      idPtr++;
      }
    }
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::ConvertFaceStreamPointIds(vtkIdType nfaces,
                                                    vtkIdType * faceStream,
                                                    vtkIdType * idMap)
{
  vtkIdType* idPtr = faceStream;
  for (vtkIdType i = 0; i < nfaces; i++)
    {
    vtkIdType npts = *idPtr++;
    for (vtkIdType j = 0; j < npts; j++)
      {
      *idPtr = idMap[*idPtr];
      idPtr++;
      }
    }
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkUnstructuredGrid::GetData(vtkInformation* info)
{
  return info? vtkUnstructuredGrid::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkUnstructuredGrid::GetData(vtkInformationVector* v,
                                                  int i)
{
  return vtkUnstructuredGrid::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
void vtkUnstructuredGrid::GetCellNeighbors(vtkIdType cellId, vtkIdList& ptIds, vtkIdList& cellIds)
{
  this->GetCellNeighbors(cellId, &ptIds, &cellIds);
}
#endif
