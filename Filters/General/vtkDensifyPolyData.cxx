/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDensifyPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDensifyPolyData.h"

#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkMath.h"
#include <vector>

//----------------------------------------------------------------------------
class vtkDensifyPolyDataInternals
{
public:

  // Constructor with params :
  //  - vertices of the triangle to be decimated,
  //  - number of vertices
  //  - ptIds of these vertices
  //  - current running count of the max pointIds, so the new points added
  //    can be assigned ptIds starting from this value.
  //  - Number of subdivisions
  vtkDensifyPolyDataInternals( double    * verts,
                                    vtkIdType   numVerts,
                                    vtkIdType * vertIds,
                                    vtkIdType  & numPoints,
                                    unsigned int nSubdivisions)
  {
    this->NumPoints = this->StartPointId = this->CurrentPointId = numPoints;
    Polygon p( verts, numVerts, vertIds );
    this->Polygons.push_back( p );

    // The actual work: subdivision of the supplied polygon is done here.
    for (unsigned int i = 0; i < nSubdivisions; i++)
    {
      this->Polygons = this->Subdivide( this->Polygons );
    }

    this->PolygonsIterator = this->Polygons.begin();
    numPoints = this->NumPoints;
  }

  // Internal class to represent an nSided polygon.
  class Polygon
  {
  public:
    // Construct a polygon.
    //  nPts:        number of vertices in the polygon
    //  p:           Array of vertices, organized as p0[0], p0[1], p0[2],
    //               p1[0], ..
    //  ptIds:       The pointids of the vertices.
    //  parentPtIds: PointIds of the parent polygon (if one exists), ie the
    //               polygon this is intended to represent a subdivision of.
    //
    Polygon( double *p, vtkIdType nPts, vtkIdType *ptIds,
             vtkIdType nParentPoints = 0, vtkIdType *parentPtIds = NULL)
    {
      this->Verts   = new double[3*nPts];
      this->VertIds = new vtkIdType[nPts];
      this->NumVerts = nPts;
      for (vtkIdType i = 0; i < nPts; i++ )
      {
        this->Verts[3*i]       = p[3*i];
        this->Verts[3*i + 1]   = p[3*i + 1];
        this->Verts[3*i + 2]   = p[3*i + 2];
        this->VertIds[i]       = ptIds[i];
      }

      if (nParentPoints && parentPtIds)
      {
        this->ParentVertIds = new vtkIdType[nParentPoints];
        for (vtkIdType i = 0; i < nPts; i++ )
        {
          this->ParentVertIds[i] = parentPtIds[i];
        }
        this->NumParentVerts = nParentPoints;
      }
      else
      {
        this->NumParentVerts = 0;
        this->ParentVertIds  = NULL;
      }
    }

    // Default constructor
    Polygon()
    {
      this->NumVerts = this->NumParentVerts = 0;
      this->VertIds = this->ParentVertIds = NULL;
      this->Verts = NULL;
    }

    ~Polygon()
    {
      this->Clear();
    }

    void Clear()
    {
      delete [] this->Verts;
      this->Verts=0;
      delete [] this->VertIds;
      this->VertIds=0;
      delete [] this->ParentVertIds;
      this->ParentVertIds=0;
    }

    void DeepCopy( const Polygon & p )
    {
      // Copy the vertices.
      this->NumVerts = p.NumVerts;
      if (p.Verts)
      {
        this->Verts = new double [3 * p.NumVerts];
        for (vtkIdType i = 0; i < this->NumVerts; i++ )
        {
          this->Verts[3*i]       = p.Verts[3*i];
          this->Verts[3*i + 1]   = p.Verts[3*i + 1];
          this->Verts[3*i + 2]   = p.Verts[3*i + 2];
        }
      }
      else
      {
          this->Verts=0; // can happen if called from the copy constructor.
      }

      // Copy the vertex ids.
      if (p.VertIds)
      {
        this->VertIds = new vtkIdType [p.NumVerts];
        for (vtkIdType i = 0; i < this->NumVerts; i++ )
        {
          this->VertIds[i] = p.VertIds[i];
        }
      }
      else
      {
          this->VertIds=0; // can happen if called from the copy constructor.
      }

      // Copy the parent vertex ids, if any.
      this->NumParentVerts = p.NumParentVerts;
      if (p.ParentVertIds)
      {
        this->ParentVertIds = new vtkIdType [p.NumParentVerts];
        for (vtkIdType i = 0; i < this->NumParentVerts; i++ )
        {
          this->ParentVertIds[i] = p.ParentVertIds[i];
        }
      }
      else
      {
        this->ParentVertIds = NULL;
        this->NumParentVerts = 0;
      }
    }

    // Overload method to deep-copy internal data instead of just copying over
    // the pointers.
    Polygon (const Polygon & p)
    {
      this->DeepCopy(p);
    }

    // Overload method to deep-copy internal data instead of just copying over
    // the pointers.
    Polygon &operator=(const Polygon &p)
    {
      // start afresh
      this->Clear();

      this->DeepCopy(p);
      return *this;
    }

    void GetCentroid( double centroid[3] )
    {
      centroid[0] = centroid[1] = centroid[2] = 0.0;
      for (vtkIdType i = 0; i < this->NumVerts; i++)
      {
        centroid[0] += this->Verts[3*i];
        centroid[1] += this->Verts[3*i+1];
        centroid[2] += this->Verts[3*i+2];
      }
      centroid[0] /= static_cast<double>(this->NumVerts);
      centroid[1] /= static_cast<double>(this->NumVerts);
      centroid[2] /= static_cast<double>(this->NumVerts);
    }

    // Get a point with a specific pointId. Returns false if not found
    bool GetPointWithId( vtkIdType id, double p[3] )
    {
      for (vtkIdType i=0; i < this->NumVerts; i++)
      {
        if (this->VertIds[i] == id)
        {
          p[0] = this->Verts[3*i];
          p[1] = this->Verts[3*i+1];
          p[2] = this->Verts[3*i+2];
          return true;
        }
      }
      return false;
    }

    double          * Verts;
    vtkIdType       * VertIds;
    vtkIdType         NumVerts;
    vtkIdType       * ParentVertIds;
    vtkIdType         NumParentVerts;
  };

  // A container of polygons.
  typedef std::vector< Polygon > PolygonsType;

  // After subdivision, use this method to get the next point.
  // Returns the pointId of the point. Returns -1 if no more points.
  vtkIdType GetNextPoint( double p[3], vtkIdList * parentPointIds = NULL )
  {
    vtkIdType id = -1;
    if (this->CurrentPointId < this->NumPoints)
    {
      for (PolygonsType::iterator it = this->Polygons.begin();
           it != this->Polygons.end(); ++it )
      {
        if ((*it).GetPointWithId( this->CurrentPointId, p ) )
        {
          id = this->CurrentPointId;
          if (parentPointIds)
          {
            parentPointIds->Reset();
            for (vtkIdType i = 0; i < it->NumParentVerts; i++)
            {
              parentPointIds->InsertNextId( it->ParentVertIds[i] );
            }
          }
        }
      }
    }
    this->CurrentPointId++;
    return id;
  }

  // After subdivision, methods to get the next cell (polygon) point Ids.
  // Returns false if no more cells.
  vtkIdType * GetNextCell(
      vtkIdType & numVerts /* number of verts in the returned ids */ )
  {
    vtkIdType *vertIds = NULL;
    numVerts = 0;
    if (this->PolygonsIterator != this->Polygons.end())
    {
      vertIds = this->PolygonsIterator->VertIds;
      numVerts = this->PolygonsIterator->NumVerts;
      ++this->PolygonsIterator;
    }
    return vertIds;
  }

  // Subdivide a triangle. Returns a container containing 3 new triangles.
  PolygonsType Subdivide( Polygon & t )
  {
    PolygonsType polygons;

    // Can't subdivide a polygon with less than 3 vertices ! It will be passed
    // through to the output.
    if (t.NumVerts < 3)
    {
      polygons.push_back(t);
      return polygons;
    }

    // Subdivide the polygon by fanning out triangles from the centroid of the
    // polygon over to each of the vertices of the polygon.
    double centroid[3];
    t.GetCentroid( centroid );

    for (vtkIdType i = 0; i < t.NumVerts; i++)
    {
      vtkIdType id1 = i, id2 = (i+1)%(t.NumVerts), id3 = this->NumPoints;

      // verts for the new triangle.
      double verts[9] =
        {  t.Verts[3*id1], t.Verts[3*id1+1], t.Verts[3*id1+2],
           t.Verts[3*id2], t.Verts[3*id2+1], t.Verts[3*id2+2],
           centroid[0],    centroid[1],      centroid[2]       };
      vtkIdType vertIds[3] = { t.VertIds[id1], t.VertIds[id2], id3 };
      polygons.push_back(
          Polygon( verts, 3, vertIds, t.NumVerts, t.VertIds ) );
    }

    this->NumPoints++;
    return polygons;
  }

  // Subdivide each polygon in a container of polygons once.
  PolygonsType Subdivide( PolygonsType & polygons )
  {
    PolygonsType newPolygons;
    for (PolygonsType::iterator it = polygons.begin();
         it != polygons.end(); ++it)
    {
      PolygonsType output = this->Subdivide( *it );
      for (PolygonsType::iterator it2 = output.begin();
           it2 != output.end(); ++it2 )
      {
        newPolygons.push_back( *it2 );
      }
    }
    return newPolygons;
  }

private:
  PolygonsType            Polygons;
  vtkIdType               NumPoints;
  vtkIdType               StartPointId;
  vtkIdType               CurrentPointId;
  PolygonsType::iterator  PolygonsIterator;
};


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkDensifyPolyData);

//----------------------------------------------------------------------------
vtkDensifyPolyData::vtkDensifyPolyData()
{
  this->NumberOfSubdivisions = 1;
  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
vtkDensifyPolyData::~vtkDensifyPolyData()
{
}

//----------------------------------------------------------------------------
int vtkDensifyPolyData::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkCellArray *inputPolys = input->GetPolys();
  vtkPoints * inputPoints = input->GetPoints();

  if (!inputPolys || !inputPoints)
  {
    vtkWarningMacro(
      "vtkDensifyPolyData has no points/cells to linearly interpolate.");
    return 0;
  }

  vtkIdType npts = 0, *ptIds = 0;

  input->BuildLinks();

  const vtkIdType inputNumCells = input->GetNumberOfCells();
  const vtkIdType inputNumPoints = input->GetNumberOfPoints();
  vtkCellArray *outputPolys = vtkCellArray::New();

  // Deep copy the input points. We will then add more points this during
  // subdivision.
  vtkPoints * outputPoints = vtkPoints::New();
  outputPoints->DeepCopy( inputPoints );

  // Will be at least that big.. in reality much larger..
  outputPolys->Allocate( outputPolys->EstimateSize( inputNumCells, 3 ) );

  // Copy pointdata structure from input. There will be at least as many
  // points as in the input.
  vtkPointData * inputPD  = input->GetPointData();
  vtkCellData  * inputCD  = input->GetCellData();
  vtkPointData * outputPD = output->GetPointData();
  vtkCellData  * outputCD = output->GetCellData();
  outputPD->DeepCopy(inputPD);

  // Copy celldata structure from input. There will be at least as many cells
  // in the output as in the input.
  outputCD->CopyStructure(inputCD);
  outputCD->CopyAllocate(outputCD, inputNumCells);

  double q[3];
  vtkIdType outputNumPoints = inputNumPoints, cellId = 0, ptId;

  vtkIdList * parentPointIds = vtkIdList::New();

  // var containing number of cells in the output
  vtkIdType outputNumCells = 0;

  for (inputPolys->InitTraversal();
       inputPolys->GetNextCell(npts, ptIds); cellId++)
  { // for every cell

    // Make sure that the polygon is a planar polygon.
    int cellType = input->GetCellType(cellId);
    if ( cellType != VTK_POLYGON &&
         cellType != VTK_QUAD &&
         cellType != VTK_TRIANGLE )
    {
      // Only triangles are subdivided, the others are simply passed through
      // to the output.
      vtkIdType newCellId = outputPolys->InsertNextCell( npts, ptIds );
      ++outputNumCells;
      outputCD->CopyAllocate( outputCD, outputNumCells );
      outputCD->CopyData( inputCD, cellId, newCellId );
      continue;
    }

    double triangleOrQuadPoints[4*3]; // points of the cell (triangle or quad)
    double *p;

    if(cellType==VTK_POLYGON)
    {
      p=new double[npts*3];
    }
    else
    {
      p=triangleOrQuadPoints;
    }

    for (vtkIdType j=0; j < npts; j++)
    {
      inputPoints->GetPoint(ptIds[j], p+(3*j));
    }

    // Check constraints..
    unsigned int nSubdivisions = VTK_UNSIGNED_INT_MAX;

    if (this->NumberOfSubdivisions > 0)
    {
      if(nSubdivisions > this->NumberOfSubdivisions)
      {
        nSubdivisions=this->NumberOfSubdivisions;
      }
    }

    if (nSubdivisions == 0 || nSubdivisions == VTK_UNSIGNED_INT_MAX)
    {
      // No need to subdivide.. just keep the same cell..
      vtkIdType newCellId = outputPolys->InsertNextCell( npts, ptIds );
      ++outputNumCells;
      outputCD->CopyAllocate( outputCD, outputNumCells );
      outputCD->CopyData( inputCD, cellId, newCellId );
    }
    else
    {
      // Subdivide the triangle.
      // The number of new cells formed due to the subdivision of this triangle
      //  = nPts * pow(3, nSubdivisions-1)
      outputNumCells += (npts * static_cast< vtkIdType >(
                           pow(3.0,static_cast<int>(nSubdivisions-1))));

      // Ensure that we have enough space to hold the new cell data. (This does
      // not actually resize the array at every step of the iteration.) It will
      // end up resizing when
      // outputNumCells = { 2*inputNumCells, 4*inputNumCells, 8*inputNumCells,
      //                    16*inputNumCells, .... }.
      outputCD->CopyAllocate( outputCD, outputNumCells );

      vtkDensifyPolyDataInternals
        polygons( p, npts, ptIds, outputNumPoints, nSubdivisions );

      // Insert points and cells generated by subdividing this polygon
      // nSubdivisions times. Generate the point data and the cell data
      // for the new cell points and cells.

      outputPD->CopyAllocate( outputPD, outputNumPoints);
      while ((ptId = polygons.GetNextPoint(q, parentPointIds)) != -1)
      {

        // Interpolation weights for interpolating point data at the
        // subdivided polygon
        vtkIdType nParentVerts = parentPointIds->GetNumberOfIds();
        double *interpolationWeights = new double [nParentVerts];
        double weight = 1.0 / static_cast<double>(nParentVerts);
        for (vtkIdType i = 0; i < nParentVerts; i++)
        {
          interpolationWeights[i] = weight;
        }

        outputPoints->InsertNextPoint( q );
        outputPD->InterpolatePoint( inputPD, ptId,
                                    parentPointIds, interpolationWeights );

        delete [] interpolationWeights;
      }

      vtkIdType numNewCellVerts;
      while (vtkIdType * newCellVertIds = polygons.GetNextCell(numNewCellVerts))
      {
        vtkIdType newCellId = outputPolys->
          InsertNextCell( numNewCellVerts, newCellVertIds );
        outputCD->CopyData( inputCD, cellId, newCellId );
      }
    } // else
    if(cellType==VTK_POLYGON)
    {
      delete[] p;
    }
  } // for every cell


  output->SetPoints( outputPoints );
  output->SetPolys( outputPolys );

  outputPolys->Delete();
  outputPoints->Delete();
  parentPointIds->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkDensifyPolyData::FillInputPortInformation(
                       int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  }
  else
  {
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkDensifyPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number of Subdivisions: "
     << this->NumberOfSubdivisions << endl;
}

