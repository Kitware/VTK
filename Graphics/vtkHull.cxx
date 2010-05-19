/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHull.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHull.h"

#include "vtkCellArray.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPlanes.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkHull);

// Construct an the hull object with no planes
vtkHull::vtkHull()
{
  this->Planes             = NULL;
  this->PlanesStorageSize  = 0;
  this->NumberOfPlanes     = 0;
}

// Destructor for a hull object - remove the planes if necessary
vtkHull::~vtkHull()
{
  if ( this->Planes ) 
    {
    delete [] this->Planes;
    this->Planes = NULL;
    }
}

// Remove all planes.
void vtkHull::RemoveAllPlanes()
{
  if ( this->Planes )
    {
    delete [] this->Planes;
    this->Planes = NULL;
    }

  this->PlanesStorageSize  = 0;
  this->NumberOfPlanes     = 0;
  this->Modified();
}

// Add a plane. The vector (A,B,C) is the plane normal and is from the
// plane equation Ax + By + Cz + D = 0. The normal should point outwards
// away from the center of the hull.
int vtkHull::AddPlane( double A, double B, double C )
{
  double     *tmpPointer;
  int       i;
  double     norm, dotproduct;

  // Normalize the direction,
  // and make sure the vector has a length.
  norm = sqrt( A*A + B*B + C*C );
  if ( norm == 0.0 )
    {
    vtkErrorMacro( << "Zero length vector not allowed for plane normal!" );
    return -VTK_LARGE_INTEGER;
    }
  A /= norm;
  B /= norm;
  C /= norm;

  // Check that it is at least somewhat different from the other
  // planes we have so far - can't have a normalized dot product of 
  // nearly 1.
  for ( i = 0; i < this->NumberOfPlanes; i++ )
    {
    dotproduct = 
      A * this->Planes[i*4 + 0] +
      B * this->Planes[i*4 + 1] +
      C * this->Planes[i*4 + 2];

    //If planes are parallel, we already have the plane.
    //Indicate this with the appropriate return value.
    if ( dotproduct > 0.99999 && dotproduct < 1.00001 )
      {
      return -(i+1);
      }
    }

  // If adding this plane would put us over the amount of space we've
  // allocated for planes, then we'll have to allocated some more space
  if ( (this->NumberOfPlanes+1) >= this->PlanesStorageSize )
    {
    // Hang onto the previous set of planes
    tmpPointer = this->Planes;

    // Increase our storage 
    if ( this->PlanesStorageSize <= 0 )
      {
      this->PlanesStorageSize = 100;
      }
    else
      {
      this->PlanesStorageSize *= 2;
      }
    this->Planes = new double [this->PlanesStorageSize * 4];

    if ( !this->Planes )
      {
      vtkErrorMacro( << "Unable to allocate space for planes" );
      this->Planes = tmpPointer;
      return -VTK_LARGE_INTEGER;
      }

    // Copy the planes and delete the old storage space
    for ( i = 0; i < this->NumberOfPlanes*4; i++ )
      {
      this->Planes[i] = tmpPointer[i];
      }
    if ( tmpPointer )
      {
      delete [] tmpPointer;
      }
    }

  // Add the plane at the end of the array. 
  // The fourth element doesn't actually need to be set, but it 
  // eliminates a purify uninitialized memory copy error if it is set
  i = this->NumberOfPlanes;
  this->Planes[i*4 + 0] = A;
  this->Planes[i*4 + 1] = B;
  this->Planes[i*4 + 2] = C;
  this->Planes[i*4 + 3] = 0.0;
  this->NumberOfPlanes++;

  this->Modified();

  // Return the index to this plane so that it can be set later
  return i;
}

// Add a plane, passing the plane normal vector as a double array instead
// of three doubles.
int vtkHull::AddPlane( double plane[3] )
{
  return this->AddPlane( plane[0], plane[1], plane[2] );
}

// Set a specific plane - this plane should already have been added with
// AddPlane, and the return value then used to modifiy the plane normal
// with this method.
void vtkHull::SetPlane( int i, double A, double B, double C )
{
  double norm;

  // Make sure this is a plane that was already added
  if ( i < 0 || i >= this->NumberOfPlanes )
    {
    vtkErrorMacro( << "Invalid index in SetPlane" );
    return;
    }

  double *plane = this->Planes + i*4;
  if ( A == plane[0] && B == plane[1] && C == plane[2] )
    {
    return; //no modified
    }

  // Set plane that has index i. Normalize the direction,
  // and make sure the vector has a length.
  norm = sqrt( A*A + B*B + C*C );
  if ( norm == 0.0 )
    {
    vtkErrorMacro( << "Zero length vector not allowed for plane normal!" );
    return;
    }
  this->Planes[i*4 + 0] = A/norm;
  this->Planes[i*4 + 1] = B/norm;
  this->Planes[i*4 + 2] = C/norm;

  this->Modified();
}

// Set a specific plane (that has already been added) - passing the plane
// normal as a double array
void vtkHull::SetPlane( int i, double plane[3] )
{
  this->SetPlane( i, plane[0], plane[1], plane[2] );
}

int vtkHull::AddPlane( double A, double B, double C, double D )
{
  int i, j;

  if ( (i=this->AddPlane(A,B,C)) >= 0 )
    {
    this->Planes[4*i + 3] = D;
    }
  else if ( i >= -this->NumberOfPlanes )
    {//pick the D that minimizes the convex set
    j = -i - 1;
    this->Planes[4*j + 3] = (D > this->Planes[4*j + 3] ? 
                             D : this->Planes[4*j + 3]);
    }
  return i;
}

int vtkHull::AddPlane( double plane[3], double D )
{
  int i, j;

  if ( (i=this->AddPlane(plane[0],plane[1],plane[2])) >= 0 )
    {
    this->Planes[4*i + 3] = D;
    }
  else if ( i >= -this->NumberOfPlanes )
    {//pick the D that minimizes the convex set
    j = -i - 1;
    this->Planes[4*j + 3] = (D > this->Planes[4*j + 3] ? 
                             D : this->Planes[4*j + 3]);
    }
  return i;
}

void vtkHull::SetPlane( int i, double A, double B, double C, double D )
{
  if ( i >= 0 && i < this->NumberOfPlanes )
    {
    double *plane = this->Planes + 4*i;
    if ( plane[0] != A || plane[1] != B || plane[2] != C ||
         plane[3] != D )
      {
      this->SetPlane(i, A,B,C);
      plane[3] = D;
      this->Modified();
      }
    }
}

void vtkHull::SetPlane( int i, double plane[3], double D )
{
  this->SetPlane(i, plane[0], plane[1], plane[2], D);
}

void  vtkHull::SetPlanes( vtkPlanes *planes )
{
  this->RemoveAllPlanes();

  // Add the planes to the hull
  if ( planes )
    {
    int i, idx;
    vtkPoints *points = planes->GetPoints();
    vtkDataArray *normals = planes->GetNormals();
    if ( points && normals )
      {
      for (i=0; i<planes->GetNumberOfPlanes(); i++)
        {
        double point[3];
        points->GetPoint(i, point);
        if ( (idx=this->AddPlane(normals->GetTuple(i))) >= 0)
          { 
          idx *= 4;
          this->Planes[idx + 3] = -(this->Planes[idx]*point[0] +
                                    this->Planes[idx+1]*point[1] +
                                    this->Planes[idx+2]*point[2]);
          }

        else if ( idx >= -this->NumberOfPlanes )
          { //planes are parallel, take the one that minimizes the convex set
          idx = -4*(idx+1);
          double D = -(this->Planes[idx]*point[0] +
                      this->Planes[idx+1]*point[1] +
                      this->Planes[idx+2]*point[2]);
          this->Planes[idx + 3] = (D > this->Planes[idx + 3] ? 
                                   D : this->Planes[idx + 3]);

          }//special parallel planes case
        }//for all planes
      }//if points and normals
    }//if planes defined

  return;
}

// Add the six planes that represent the faces on a cube
void vtkHull::AddCubeFacePlanes()
{
  this->AddPlane(  1.0,  0.0,  0.0 );
  this->AddPlane( -1.0,  0.0,  0.0 );
  this->AddPlane(  0.0,  1.0,  0.0 );
  this->AddPlane(  0.0, -1.0,  0.0 );
  this->AddPlane(  0.0,  0.0,  1.0 );
  this->AddPlane(  0.0,  0.0, -1.0 );
}

// Add the twelve planes that represent the edges on a cube - halfway between
// the two adjacent face planes
void vtkHull::AddCubeEdgePlanes()
{
  this->AddPlane(  1.0,  1.0,  0.0 );
  this->AddPlane(  1.0, -1.0,  0.0 );
  this->AddPlane( -1.0,  1.0,  0.0 );
  this->AddPlane( -1.0, -1.0,  0.0 );
  this->AddPlane(  1.0,  0.0,  1.0 );
  this->AddPlane(  1.0,  0.0, -1.0 );
  this->AddPlane( -1.0,  0.0,  1.0 );
  this->AddPlane( -1.0,  0.0, -1.0 );
  this->AddPlane(  0.0,  1.0,  1.0 );
  this->AddPlane(  0.0,  1.0, -1.0 );
  this->AddPlane(  0.0, -1.0,  1.0 );
  this->AddPlane(  0.0, -1.0, -1.0 );
}

// Add the eight planes that represent the vertices on a cube - partway 
// between the three adjacent face planes.
void vtkHull::AddCubeVertexPlanes()
{
  this->AddPlane(  1.0,  1.0,  1.0 );
  this->AddPlane(  1.0,  1.0, -1.0 );
  this->AddPlane(  1.0, -1.0,  1.0 );
  this->AddPlane(  1.0, -1.0, -1.0 );
  this->AddPlane( -1.0,  1.0,  1.0 );
  this->AddPlane( -1.0,  1.0, -1.0 );
  this->AddPlane( -1.0, -1.0,  1.0 );
  this->AddPlane( -1.0, -1.0, -1.0 );
}

// Add the planes that represent the normals of the vertices of a
// polygonal sphere formed by recursively subdividing the triangles in
// an octahedron.  Each triangle is subdivided by connecting the
// midpoints of the edges thus forming 4 smaller triangles. The level
// indicates how many subdivisions to do with a level of 0 used to add
// the 6 planes from the original octahedron, level 1 will add 18
// planes, and so on.
void vtkHull::AddRecursiveSpherePlanes( int level )
{
  int   numTriangles;
  double *points;
  int   *triangles;
  int   *validPoint;
  int   triCount, pointCount;
  int   i, j, k, loop, limit;
  double midpoint[3][3];
  double midindex[3];
  int   A, B, C;
  
  if ( level < 0 ) 
    {
    vtkErrorMacro( << "Cannot have a level less than 0!" );
    return;
    }

  if ( level > 10 ) 
    {
    vtkErrorMacro( << "Cannot have a level greater than 10!" );
    return;
    }

  numTriangles = static_cast<int>(8.0*pow(4.0,static_cast<double>(level)));

  // Create room for the triangles and points
  // We will also need to keep track of which points are
  // duplicates so keep a validPoint array for this
  points = new double[3*numTriangles];
  triangles = new int[3*numTriangles];
  validPoint = new int[3*numTriangles];


  // Add the initial points
  i = 0;
  points[i++] =  0.0;   points[i++] =  1.0;   points[i++] =  0.0;
  points[i++] = -1.0;   points[i++] =  0.0;   points[i++] =  0.0;
  points[i++] =  0.0;   points[i++] =  0.0;   points[i++] = -1.0;
  points[i++] =  1.0;   points[i++] =  0.0;   points[i++] =  0.0;
  points[i++] =  0.0;   points[i++] =  0.0;   points[i++] =  1.0;
  points[i++] =  0.0;   points[i++] = -1.0;   points[i++] =  0.0;
  pointCount = i / 3;

  // Add the initial triangles
  i = 0;
  triangles[i++] = 0;   triangles[i++] = 1;   triangles[i++] = 2;
  triangles[i++] = 0;   triangles[i++] = 2;   triangles[i++] = 3;
  triangles[i++] = 0;   triangles[i++] = 3;   triangles[i++] = 4;
  triangles[i++] = 0;   triangles[i++] = 4;   triangles[i++] = 1;
  triangles[i++] = 5;   triangles[i++] = 1;   triangles[i++] = 2;
  triangles[i++] = 5;   triangles[i++] = 2;   triangles[i++] = 3;
  triangles[i++] = 5;   triangles[i++] = 3;   triangles[i++] = 4;
  triangles[i++] = 5;   triangles[i++] = 4;   triangles[i++] = 1;
  triCount = i / 3;

  // loop over the levels adding points and triangles
  for ( loop = 0; loop < level; loop++ )
    {
    limit = triCount;
    for ( i = 0; i < limit; i++ )
      {
      for ( j = 0; j < 3; j++ )
        {
        A = j;
        B = (j+1) % 3;
        for ( k = 0; k < 3; k++ )
          {
          midpoint[j][k] = ( points[3*triangles[i*3 + A] + k] +
                             points[3*triangles[i*3 + B] + k]  ) * 0.5;
          points[pointCount*3 + k] = midpoint[j][k];
          }     
        midindex[j] = pointCount;
        pointCount++;
        }
      A = triangles[i*3 + 0];
      B = triangles[i*3 + 1];
      C = triangles[i*3 + 2];

      // Add the middle triangle in place of the one we just processed
      triangles[i*3 + 0] = static_cast<int>(midindex[0]);
      triangles[i*3 + 1] = static_cast<int>(midindex[1]);
      triangles[i*3 + 2] = static_cast<int>(midindex[2]);

      // Now add the 3 outer triangles at the end of the triangle list
      triangles[triCount*3 + 0] = static_cast<int>(midindex[0]);
      triangles[triCount*3 + 1] = B;
      triangles[triCount*3 + 2] = static_cast<int>(midindex[1]);
      triCount++;

      triangles[triCount*3 + 0] = static_cast<int>(midindex[1]);
      triangles[triCount*3 + 1] = C;
      triangles[triCount*3 + 2] = static_cast<int>(midindex[2]);
      triCount++;

      triangles[triCount*3 + 0] = static_cast<int>(midindex[2]);
      triangles[triCount*3 + 1] = A;
      triangles[triCount*3 + 2] = static_cast<int>(midindex[0]);
      triCount++;
      }
    }

  // Mark duplicate points as invalid so that we don't try to add the
  // same plane twice
  for ( i = 0; i < pointCount; i++ )
    {
    validPoint[i] = 1;
    for ( j = 0; j < i; j++ )
      {
      if ( fabs(points[i*3 + 0] - points[j*3 + 0]) < 0.001 &&
           fabs(points[i*3 + 1] - points[j*3 + 1]) < 0.001 &&
           fabs(points[i*3 + 2] - points[j*3 + 2]) < 0.001 )
        {
        validPoint[i] = 0;
        break;
        }
      }
    }
  for ( i = 0; i < pointCount; i++ )
    {
    if ( validPoint[i] ) 
      {
      this->AddPlane( points[i*3 + 0], points[i*3 + 1], points[i*3 + 2] );
      }
    }

  delete [] points;
  delete [] triangles;
  delete [] validPoint;

}

// Create the n-sided convex hull from the input geometry according to the
// set of planes.
int vtkHull::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType      numPoints;
  vtkPoints      *outPoints;
  vtkCellArray   *outPolys;
  double          *bounds      = input->GetBounds();

  // Get the number of points in the input data
  numPoints = input->GetNumberOfPoints();

  // There should be at least three points for this to work.
  if ( numPoints < 3 )
    {
    vtkErrorMacro( << "There must be >= 3 points in the input data!!!\n" );
    return 1;
    }

  // There should be at least four planes for this to work. There will need
  // to be more planes than four if any of them are parallel.
  if ( this->NumberOfPlanes < 4 )
    {
    vtkErrorMacro( << "There must be >= 4 planes!!!\n" );
    return 1;
    }

  // Create a new set of points and polygons into which the results will
  // be stored
  outPoints = vtkPoints::New();
  outPolys  = vtkCellArray::New();

  // Compute the D value for each plane according to the vertices in the
  // geometry
  this->ComputePlaneDistances(input);
  this->UpdateProgress(0.25);

  // Create a large polygon representing each plane, and clip that polygon
  // against all other planes to form the polygons of the hull.
  this->ClipPolygonsFromPlanes( outPoints, outPolys, bounds );
  this->UpdateProgress(0.80);

  // Set the output vertices and polygons
  output->SetPoints( outPoints );
  output->SetPolys( outPolys );

  // Delete the temporary storage
  outPoints->Delete();
  outPolys->Delete();

  return 1;
}

// Compute the D value for each plane. This is the largest D value obtained 
// by passing a plane with the specified normal through each vertex in the
// geometry. This plane will have a normal pointing in towards the center of
// the hull.
void vtkHull::ComputePlaneDistances(vtkPolyData *input)
{
  vtkIdType      i;
  int            j;
  double          coord[3];
  double         v;

  // Initialize all planes to the first vertex value
  input->GetPoint( 0, coord );
  for ( j = 0; j < this->NumberOfPlanes; j++ )
    {
    this->Planes[j*4 + 3] = -( this->Planes[j*4 + 0] * coord[0] +
                               this->Planes[j*4 + 1] * coord[1] +
                               this->Planes[j*4 + 2] * coord[2] );
    }
  // For all other vertices in the geometry, check if it produces a larger
  // D value for each of the planes.
  for ( i = 1; i < input->GetNumberOfPoints(); i++ )
    {
    input->GetPoint( i, coord );
    for ( j = 0; j < this->NumberOfPlanes; j++ )
      {
      v =  -( this->Planes[j*4 + 0] * coord[0] +
              this->Planes[j*4 + 1] * coord[1] +
              this->Planes[j*4 + 2] * coord[2] );
      // negative means further in + direction of plane
      if ( v < this->Planes[j*4 + 3] )
        {
        this->Planes[j*4 + 3] = v;
        }
      }
    }
}

// Given the set of planes, create a large polygon for each, then use all the
// other planes to clip this polygon.
void vtkHull::ClipPolygonsFromPlanes( vtkPoints *outPoints,
                                      vtkCellArray *outPolys,
                                      double *bounds)
{
  int            i, j, k, q;
  double         previousD, d, crosspoint;
  double         *verts, *newVerts, *tmpVerts;
  int            vertCount, newVertCount;
  vtkIdType      *pnts;

  // Use two arrays to store the vertices of the polygon
  verts = new double[3*(this->NumberOfPlanes+1)];
  newVerts = new double[3*(this->NumberOfPlanes+1)];

  // We need an array to store the indices for the polygon
  pnts = new vtkIdType[this->NumberOfPlanes-1];

  // We have no vertices yet
  //vertCount = 0;

  // For each plane, create a polygon (if it gets completely clipped there
  // won't be a polygon)
  for ( i = 0; i < this->NumberOfPlanes; i++ )
    {
    // Create the initial polygon - this is a large square around the
    // projected center of the object (projected onto this plane). We
    // now have four vertices.
    this->CreateInitialPolygon( verts, i, bounds );
    vertCount = 4;

    // Clip this polygon by each plane.
    for ( j = 0; j < this->NumberOfPlanes; j++ )
      {
      // Stop if we have removed too many vertices and no longer have
      // a polygon
      if ( vertCount <= 2 ) 
        {
        break;
        }
      // Otherwise, if this is not the plane we are working on, clip
      // it by this plane.
      if ( i != j )
        {
        // Test each pair of vertices to make sure this edge
        // isn't clipped. If the d values are different, it is
        // clipped - find the crossing point and add that as
        // a new vertex. If the second vertex's d is greater than 0, 
        // then keep this vertex.
        newVertCount = 0;
        previousD = 
            this->Planes[j*4 + 0] * verts[(vertCount-1)*3 + 0] +
            this->Planes[j*4 + 1] * verts[(vertCount-1)*3 + 1] +
            this->Planes[j*4 + 2] * verts[(vertCount-1)*3 + 2] +
            this->Planes[j*4 + 3];

        for ( k = 0; k < vertCount; k++ )
          {
          d = 
            this->Planes[j*4 + 0] * verts[k*3 + 0] +
            this->Planes[j*4 + 1] * verts[k*3 + 1] +
            this->Planes[j*4 + 2] * verts[k*3 + 2] +
            this->Planes[j*4 + 3];

          if ( (previousD < 0.0) != (d < 0.0) )
            {
            if ( k > 0 ) 
              {
              q = k - 1;
              }
            else
              {
              q = vertCount - 1;
              }

            crosspoint = -previousD / (d - previousD);
            newVerts[newVertCount*3 + 0] =
              verts[q*3+0] + crosspoint*(verts[k*3+0] - verts[q*3+0]);
            newVerts[newVertCount*3 + 1] =
              verts[q*3+1] + crosspoint*(verts[k*3+1] - verts[q*3+1]);
            newVerts[newVertCount*3 + 2] =
              verts[q*3+2] + crosspoint*(verts[k*3+2] - verts[q*3+2]);
            newVertCount++;
            }

          if ( d < 0.0 )
            {
            newVerts[newVertCount*3 + 0] = verts[k*3 + 0];
            newVerts[newVertCount*3 + 1] = verts[k*3 + 1];
            newVerts[newVertCount*3 + 2] = verts[k*3 + 2];
            newVertCount++;
            }

          previousD = d;
          } //for all vertices of this plane
        tmpVerts = newVerts;
        newVerts = verts;
        verts = tmpVerts;
        vertCount = newVertCount;
        }
      } //for each potentially intersecting plane

    if ( vertCount > 0 )
      {
      for ( j = 0; j < vertCount; j++ )
        {
        pnts[j] = outPoints->InsertNextPoint( (verts + j*3) );
        }
      outPolys->InsertNextCell( vertCount, pnts );
      }
    } //for each plane

  delete [] verts;
  delete [] newVerts;
  delete [] pnts;
}

void vtkHull::CreateInitialPolygon( double *verts, int i, double *bounds)
{
  double         center[3], d, planeCenter[3];
  double         v1[3], v2[3], norm, dotProduct;
  int            j;

  center[0] = ( bounds[0] + bounds[1] ) * 0.5;
  center[1] = ( bounds[2] + bounds[3] ) * 0.5;
  center[2] = ( bounds[4] + bounds[5] ) * 0.5;

  d = 
    this->Planes[i*4 + 0] * center[0] +
    this->Planes[i*4 + 1] * center[1] +
    this->Planes[i*4 + 2] * center[2] +
    this->Planes[i*4 + 3];

  planeCenter[0] = center[0] - d * this->Planes[i*4 + 0];
  planeCenter[1] = center[1] - d * this->Planes[i*4 + 1];
  planeCenter[2] = center[2] - d * this->Planes[i*4 + 2];

  dotProduct = 1.0;
  j = i;
  while (dotProduct > 0.99999 || dotProduct < -0.99999)
    {
    j++;
    if ( j >= this->NumberOfPlanes )
      {
      j = 0;
      }
    dotProduct = 
      this->Planes[i*4 + 0] * this->Planes[j*4 + 0] +
      this->Planes[i*4 + 1] * this->Planes[j*4 + 1] +
      this->Planes[i*4 + 2] * this->Planes[j*4 + 2];
    }

  v1[0] = 
    this->Planes[j*4 + 1] * this->Planes[i*4 + 2] -
    this->Planes[j*4 + 2] * this->Planes[i*4 + 1];
  v1[1] = 
    this->Planes[j*4 + 2] * this->Planes[i*4 + 0] -
    this->Planes[j*4 + 0] * this->Planes[i*4 + 2];
  v1[2] = 
    this->Planes[j*4 + 0] * this->Planes[i*4 + 1] -
    this->Planes[j*4 + 1] * this->Planes[i*4 + 0];

  norm = sqrt( (v1[0]*v1[0] + v1[1]*v1[1] + v1[2]*v1[2]) );
  v1[0] /= norm;
  v1[1] /= norm;
  v1[2] /= norm;

  v2[0] = 
    v1[1] * this->Planes[i*4 + 2] -
    v1[2] * this->Planes[i*4 + 1];
  v2[1] = 
    v1[2] * this->Planes[i*4 + 0] -
    v1[0] * this->Planes[i*4 + 2];
  v2[2] = 
    v1[0] * this->Planes[i*4 + 1] -
    v1[1] * this->Planes[i*4 + 0];

  norm = sqrt(v2[0]*v2[0] + v2[1]*v2[1] + v2[2]*v2[2]);
  v2[0] /= norm;
  v2[1] /= norm;
  v2[2] /= norm;

  d = 
    (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]);

  verts[0*3 + 0] = planeCenter[0] - d * v1[0] - d * v2[0];
  verts[0*3 + 1] = planeCenter[1] - d * v1[1] - d * v2[1];
  verts[0*3 + 2] = planeCenter[2] - d * v1[2] - d * v2[2];

  verts[1*3 + 0] = planeCenter[0] - d * v1[0] + d * v2[0];
  verts[1*3 + 1] = planeCenter[1] - d * v1[1] + d * v2[1];
  verts[1*3 + 2] = planeCenter[2] - d * v1[2] + d * v2[2];

  verts[2*3 + 0] = planeCenter[0] + d * v1[0] + d * v2[0];
  verts[2*3 + 1] = planeCenter[1] + d * v1[1] + d * v2[1];
  verts[2*3 + 2] = planeCenter[2] + d * v1[2] + d * v2[2];

  verts[3*3 + 0] = planeCenter[0] + d * v1[0] - d * v2[0];
  verts[3*3 + 1] = planeCenter[1] + d * v1[1] - d * v2[1];
  verts[3*3 + 2] = planeCenter[2] + d * v1[2] - d * v2[2];

}

void vtkHull::GenerateHull(vtkPolyData *pd, double xmin, double xmax,
                           double ymin, double ymax, double zmin, double zmax)
{
  double bounds[6];
  bounds[0] = xmin; bounds[1] = xmax;
  bounds[2] = ymin; bounds[3] = ymax;
  bounds[4] = zmin; bounds[5] = zmax;

  this->GenerateHull(pd, bounds);
}

void vtkHull::GenerateHull(vtkPolyData *pd, double *bounds)
{
  vtkPoints      *newPoints;
  vtkCellArray   *newPolys;

  // There should be at least four planes for this to work. There will need
  // to be more planes than four if any of them are parallel.
  if ( this->NumberOfPlanes < 4 )
    {
    vtkErrorMacro( << "There must be >= 4 planes!!!" );
    return;
    }

  // Create a new set of points and polygons into which the results will
  // be stored
  newPoints = vtkPoints::New();
  newPoints->Allocate(this->NumberOfPlanes*3);
  newPolys  = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(this->NumberOfPlanes,3));

  this->ClipPolygonsFromPlanes( newPoints, newPolys, bounds );

  pd->SetPoints(newPoints);
  pd->SetPolys(newPolys);
  newPoints->Delete();
  newPolys->Delete();

  pd->Squeeze();
}

// Print the object
void vtkHull::PrintSelf(ostream& os, vtkIndent indent)
{
  int i;

  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number Of Planes: " << this->NumberOfPlanes << endl;

  for ( i = 0; i < this->NumberOfPlanes; i++ )
    {
    os << indent << "Plane " << i << ":  " 
       << this->Planes[i*4] << " " 
       << this->Planes[i*4+1] << " " 
       << this->Planes[i*4+2] << " " 
       << this->Planes[i*4+3] << endl;
    }
}
