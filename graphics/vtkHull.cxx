/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHull.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkHull.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkHull* vtkHull::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkHull");
  if(ret)
    {
    return (vtkHull*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkHull;
}




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
    }
}


// Remove all planes.
void vtkHull::RemoveAllPlanes()
{
  if ( this->Planes )
    {
    delete [] this->Planes;
    }

  this->PlanesStorageSize  = 0;
  this->NumberOfPlanes     = 0;
  this->Modified();
}

// Add a plane. The vector (A,B,C) is the plane normal and is from the
// plane equation Ax + By + Cz + D = 0. The normal should point inward
// towards the center of the hull.
int vtkHull::AddPlane( float A, float B, float C )
{
  float     *tmp_pointer;
  int       i;
  float     norm;
  float     dotproduct;

  // Normalize the direction,
  // and make sure the vector has a length.
  norm = sqrt( (double) A*A + B*B + C*C );
  if ( norm == 0.0 )
    {
    vtkErrorMacro( << "Zero length vector not allowed for plane normal!" );
    return -1;
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

    if ( dotproduct > 0.999 && dotproduct < 1.001 )
      {
	vtkErrorMacro( << "Redundant directions:\n" <<
	"    (" << A << ", " << B << ", " << C << ") and \n" <<
	"    (" << this->Planes[i*4 + 0] << ", " << this->Planes[i*4 + 1] <<
	", " << this->Planes[i*4 + 2] << ")" );
      return -1;
      }
    }

  // If adding this plane would put us over the amount of space we've
  // allocated for planes, then we'll have to allocated some more space
  if ( this->NumberOfPlanes >= this->PlanesStorageSize )
    {
    // Hang onto the previous set of planes
    tmp_pointer = this->Planes;

    // Increase our storage by 20 (space for 20 more planes)
    this->PlanesStorageSize += 20;
    this->Planes = new float [this->PlanesStorageSize * 4];

    if ( !this->Planes )
      {
      vtkErrorMacro( << "Unable to allocate space for planes" );
      this->Planes = tmp_pointer;
      return -1;
      }

    // Copy the planes and delete the old storage space
    for ( i = 0; i < this->NumberOfPlanes*4; i++ )
      {
      this->Planes[i] = tmp_pointer[i];
      }
    if ( tmp_pointer )
      {
      delete [] tmp_pointer;
      }
    }

  // Add the plane at the end of the array. 
  // The fourth element doesn't actually need to be set, but it 
  // eliminates a purify uninitialized memory copy error if it is set
  i = this->NumberOfPlanes;
  this->Planes[i*4 + 0] = A;
  this->Planes[i*4 + 1] = B;
  this->Planes[i*4 + 2] = C;
  this->Planes[i*4 + 3] = 0;
  this->NumberOfPlanes++;

  this->Modified();

  // Return the index to this plane so that it can be set later
  return i;
}

// Add a plane, passing the plane normal vector as a float array instead
// of three floats.
int vtkHull::AddPlane( float plane[3] )
{
  return this->AddPlane( plane[0], plane[1], plane[2] );
}

// Set a specific plane - this plane should already have been added with
// AddPlane, and the return value then used to modifiy the plane normal
// with this method.
void vtkHull::SetPlane( int i, float A, float B, float C )
{
  float norm;

  // Make sure this is a plane that was already added
  if ( i >= this->NumberOfPlanes )
    {
    vtkErrorMacro( << "Invalid index in SetPlane" );
    return;
    }

  // Add the plane at the end of the array. Normalize the direction,
  // and make sure the vector has a length.
  norm = sqrt( (double) A*A + B*B + C*C );
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
// normal as a float array
void vtkHull::SetPlane( int i, float plane[3] )
{
  this->SetPlane( i, plane[0], plane[1], plane[2] );
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

// Add the planes that represent the normals of the vertices of a polygonal
// sphere formed by recursively subdividing the triangles in an octahedron.
// Each triangle is subdivided by connecting the midpoints of the edges thus
// forming 4 smaller triangles. The level indicates how many subdivisions to do
// with a level of 0 used to add the 6 planes from the original octahedron, level
// 1 will add 18 planes, and so on.
void vtkHull::AddRecursiveSpherePlanes( int level )
{
  int   numTriangles;
  float *points;
  int   *triangles;
  int   *validPoint;
  int   triCount, pointCount;
  int   i, j, k, loop, limit;
  float midpoint[3][3];
  float midindex[3];
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

  numTriangles = (int)(8*pow( 4.0, (double)level ));

  // Create room for the triangles and points
  // We will also need to keep track of which points are
  // duplicates so keep a validPoint array for this
  points = new float[3*numTriangles];
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
      triangles[i*3 + 0] = (int)(midindex[0]);
      triangles[i*3 + 1] = (int)(midindex[1]);
      triangles[i*3 + 2] = (int)(midindex[2]);

      // Now add the 3 outer triangles at the end of the triangle list
      triangles[triCount*3 + 0] = (int)(midindex[0]);
      triangles[triCount*3 + 1] = B;
      triangles[triCount*3 + 2] = (int)(midindex[1]);
      triCount++;

      triangles[triCount*3 + 0] = (int)(midindex[1]);
      triangles[triCount*3 + 1] = C;
      triangles[triCount*3 + 2] = (int)(midindex[2]);
      triCount++;

      triangles[triCount*3 + 0] = (int)(midindex[2]);
      triangles[triCount*3 + 1] = A;
      triangles[triCount*3 + 2] = (int)(midindex[0]);
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
      if ( fabs((double)(points[i*3 + 0] - points[j*3 + 0])) < 0.001 &&
	   fabs((double)(points[i*3 + 1] - points[j*3 + 1])) < 0.001 &&
	   fabs((double)(points[i*3 + 2] - points[j*3 + 2])) < 0.001 )
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
void vtkHull::Execute()
{
  vtkPolyData    *input       = this->GetInput();
  vtkPolyData    *output      = this->GetOutput();
  int            num_points;
  vtkPoints      *out_points;
  vtkCellArray   *out_polys;

  // Get the number of points in the input data
  num_points = input->GetNumberOfPoints();

  // There should be at least three points for this to work.
  if ( num_points < 3 )
    {
    vtkErrorMacro( << "There must be >= 3 points in the input data!!!\n" );
    return;
    }

  // There should be at least four planes for this to work. There will need
  // to be more planes than four if any of them are parallel.
  if ( this->NumberOfPlanes < 4 )
    {
    vtkErrorMacro( << "There must be >= 4 planes!!!\n" );
    return;
    }

  // Create a new set of points and polygons into which the results will
  // be stored
  out_points = vtkPoints::New();
  out_polys  = vtkCellArray::New();

  // Compute the D value for each plane according to the vertices in the
  // geometry
  this->ComputePlaneDistances();

  // Create a large polygon representing each plane, and clip that polygon
  // against all other planes to form the polygons of the hull.
  this->ClipPolygonsFromPlanes( out_points, out_polys );

  // Set the output vertices and polygons
  output->SetPoints( out_points );
  output->SetPolys( out_polys );

  // Delete the temporary storage
  out_points->Delete();
  out_polys->Delete();

}

// Compute the D value for each plane. This is the largest D value obtained 
// by passing a plane with the specified normal through each vertex in the
// geometry. This plane will have a normal pointing in towards the center of
// the hull.
void vtkHull::ComputePlaneDistances()
{
  vtkPolyData    *input       = this->GetInput();
  int            i, j;
  float          coord[3], v;

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
      if ( v > this->Planes[j*4 + 3] )
	{
	this->Planes[j*4 + 3] = v;
	}
      }
    }
  
}

// Given the set of planes, create a large polygon for each, then use all the
// other planes to clip this polygon.
void vtkHull::ClipPolygonsFromPlanes( vtkPoints *out_points,
				      vtkCellArray *out_polys )
{
  int            i, j, k, q;
  float          previous_d, d, crosspoint;
  float          *verts, *new_verts, *tmp_verts;
  int            vert_count, new_vert_count;
  int            *pnts;

  // Use two arrays to store the vertices of the polygon
  verts = new float[3*(this->NumberOfPlanes+1)];
  new_verts = new float[3*(this->NumberOfPlanes+1)];

  // We need an array to store the indices for the polygon
  pnts = new int[this->NumberOfPlanes-1];

  // We have no vertices yet
  vert_count = 0;

  // For each plane, create a polygon (if it gets completely clipped there
  // won't be a polygon
  for ( i = 0; i < this->NumberOfPlanes; i++ )
    {
    // Create the initial polygon - this is a large square around the
    // projected center of the object (projected onto this plane). We
    // now have four vertices.
    this->CreateInitialPolygon( verts, i );
    vert_count = 4;

    // Clip this polygon by each plane.
    for ( j = 0; j < this->NumberOfPlanes; j++ )
      {
      // Stop if we have removed too many vertices and no longer have
      // a polygon
      if ( vert_count <= 2 ) 
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
	new_vert_count = 0;
	previous_d = 
	    this->Planes[j*4 + 0] * verts[(vert_count-1)*3 + 0] +
	    this->Planes[j*4 + 1] * verts[(vert_count-1)*3 + 1] +
	    this->Planes[j*4 + 2] * verts[(vert_count-1)*3 + 2] +
	    this->Planes[j*4 + 3];

	for ( k = 0; k < vert_count; k++ )
	  {
	  d = 
	    this->Planes[j*4 + 0] * verts[k*3 + 0] +
	    this->Planes[j*4 + 1] * verts[k*3 + 1] +
	    this->Planes[j*4 + 2] * verts[k*3 + 2] +
	    this->Planes[j*4 + 3];

	  if ( (previous_d > 0) != (d > 0) )
	    {
	    if ( k > 0 ) 
	      {
	      q = k - 1;
	      }
	    else
	      {
	      q = vert_count - 1;
	      }

	    crosspoint = -previous_d / (d - previous_d);
	    new_verts[new_vert_count*3 + 0] =
	      verts[q*3+0] + crosspoint*(verts[k*3+0] - verts[q*3+0]);
	    new_verts[new_vert_count*3 + 1] =
	      verts[q*3+1] + crosspoint*(verts[k*3+1] - verts[q*3+1]);
	    new_verts[new_vert_count*3 + 2] =
	      verts[q*3+2] + crosspoint*(verts[k*3+2] - verts[q*3+2]);
	    new_vert_count++;
	    }

	  if ( d > 0 )
	    {
	    new_verts[new_vert_count*3 + 0] = verts[k*3 + 0];
	    new_verts[new_vert_count*3 + 1] = verts[k*3 + 1];
	    new_verts[new_vert_count*3 + 2] = verts[k*3 + 2];
	    new_vert_count++;
	    }

	  previous_d = d;
	  }
	tmp_verts = new_verts;
	new_verts = verts;
	verts = tmp_verts;
        vert_count = new_vert_count;
	}
      }

    if ( vert_count > 0 )
      {
      for ( j = 0; j < vert_count; j++ )
	{
	pnts[j] = out_points->InsertNextPoint( (verts + j*3) );
	}
      out_polys->InsertNextCell( vert_count, pnts );
      }
    }

  delete [] verts;
  delete [] new_verts;
  delete [] pnts;
}

void vtkHull::CreateInitialPolygon( float *verts, int i )
{
  vtkPolyData    *input       = this->GetInput();
  float          *bounds, center[3], d, plane_center[3];
  float          v1[3], v2[3], norm, dot_product;
  int            j;

  bounds = input->GetBounds();

  center[0] = ( bounds[0] + bounds[1] ) * 0.5;
  center[1] = ( bounds[2] + bounds[3] ) * 0.5;
  center[2] = ( bounds[4] + bounds[5] ) * 0.5;

  d = 
    this->Planes[i*4 + 0] * center[0] +
    this->Planes[i*4 + 1] * center[1] +
    this->Planes[i*4 + 2] * center[2] +
    this->Planes[i*4 + 3];

  plane_center[0] = center[0] - d * this->Planes[i*4 + 0];
  plane_center[1] = center[1] - d * this->Planes[i*4 + 1];
  plane_center[2] = center[2] - d * this->Planes[i*4 + 2];

  dot_product = 1.0;
  j = i;
  while (dot_product > 0.9 || dot_product < -0.9)
    {
    j++;
    if ( j >= this->NumberOfPlanes )
      {
      j = 0;
      }
    dot_product = 
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

  norm = sqrt( (double) (v1[0]*v1[0] + v1[1]*v1[1] + v1[2]*v1[2]) );
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

  norm = sqrt( (double) (v2[0]*v2[0] + v2[1]*v2[1] + v2[2]*v2[2]) );
  v2[0] /= norm;
  v2[1] /= norm;
  v2[2] /= norm;

  d = 
    (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]);

  verts[0*3 + 0] = plane_center[0] - d * v1[0] - d * v2[0];
  verts[0*3 + 1] = plane_center[1] - d * v1[1] - d * v2[1];
  verts[0*3 + 2] = plane_center[2] - d * v1[2] - d * v2[2];

  verts[1*3 + 0] = plane_center[0] + d * v1[0] - d * v2[0];
  verts[1*3 + 1] = plane_center[1] + d * v1[1] - d * v2[1];
  verts[1*3 + 2] = plane_center[2] + d * v1[2] - d * v2[2];

  verts[2*3 + 0] = plane_center[0] + d * v1[0] + d * v2[0];
  verts[2*3 + 1] = plane_center[1] + d * v1[1] + d * v2[1];
  verts[2*3 + 2] = plane_center[2] + d * v1[2] + d * v2[2];

  verts[3*3 + 0] = plane_center[0] - d * v1[0] + d * v2[0];
  verts[3*3 + 1] = plane_center[1] - d * v1[1] + d * v2[1];
  verts[3*3 + 2] = plane_center[2] - d * v1[2] + d * v2[2];

}

// Print the object
void vtkHull::PrintSelf(vtkOstream& os, vtkIndent indent)
{
  int i;

  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Number Of Planes: " << this->NumberOfPlanes << vtkEndl;

  for ( i = 0; i < this->NumberOfPlanes; i++ )
    {
    os << indent << "Plane " << i << ":  " << this->Planes[i*4] << " " <<
      this->Planes[i*4+1] << " " << this->Planes[i*4+2] << 
      " " << this->Planes[i*4+3] << vtkEndl;
    }
}
