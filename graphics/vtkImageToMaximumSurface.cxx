/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToMaximumSurface.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <math.h>
#include "vtkMarchingCubesCases.h"
#include "vtkImageToMaximumSurface.h"


//----------------------------------------------------------------------------
vtkImageToMaximumSurface::vtkImageToMaximumSurface()
{
  this->Input = NULL;
  this->LocatorPointIds = NULL;
  this->Threshold = 1.0;
}

//----------------------------------------------------------------------------
void vtkImageToMaximumSurface::Execute()
{
  vtkImageRegion *vectors, *magnitudes;
  vtkImageRegion *derivatives;
  vtkPolyData *output = this->GetOutput();
  int *extent, estimatedSize;
  
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "No Input");
    return;
    }
  
  // Get the vector image from the input
  vectors = new vtkImageRegion;
  vectors->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS, VTK_IMAGE_Z_AXIS, 
		   VTK_IMAGE_COMPONENT_AXIS);
  vectors->SetScalarType(VTK_FLOAT);
  this->Input->UpdateImageInformation(vectors);
  vectors->SetExtent(VTK_IMAGE_DIMENSIONS, vectors->GetImageExtent());
  this->Input->UpdateRegion(vectors);
  
  // Get the magnitude image.
  magnitudes = new vtkImageRegion;
  this->ComputeMagnitudes(vectors, magnitudes);

  // Compute the derivative information
  derivatives = new vtkImageRegion;
  this->ComputeDerivatives(vectors, magnitudes, derivatives);

  // Create the points, scalars, normals and Cell arrays for the output.
  // estimate the number of points from the volume dimensions
  extent = magnitudes->GetExtent();
  estimatedSize = (int) pow ((double) ((extent[1]-extent[0]+1) * 
				       (extent[3]-extent[2]+1) * 
				       (extent[5]-extent[4]+1)), 0.75);
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024) estimatedSize = 1024;
  this->Points = new vtkFloatPoints(estimatedSize,estimatedSize/2);
  this->Triangles = new vtkCellArray(estimatedSize,estimatedSize/2);
  if (this->ComputeScalars)
    {
    this->Scalars = new vtkFloatScalars(estimatedSize,estimatedSize/2);
    }
  if (this->ComputeNormals)
    {
    this->Normals = new vtkFloatNormals(estimatedSize,estimatedSize/2);
    }
  
  // Loop over all cells running marching cubes selectively.
  this->March(derivatives, magnitudes, vectors, vectors->GetOrigin(),
	      vectors->GetAspectRatio());
  
  // Put results in our output
  vtkDebugMacro(<<"Created: " 
               << this->Points->GetNumberOfPoints() << " points, " 
               << this->Triangles->GetNumberOfCells() << " triangles");
  output->SetPoints(this->Points);
  this->Points->Delete();
  this->Points = NULL;
  output->SetPolys(this->Triangles);
  this->Triangles->Delete();
  this->Triangles = NULL;
  if (this->ComputeScalars)
    {
    output->GetPointData()->SetScalars(this->Scalars);
    this->Scalars->Delete();
    this->Scalars = NULL;
    }
  if (this->ComputeNormals)
    {
    output->GetPointData()->SetNormals(this->Normals);
    this->Normals->Delete();
    this->Normals = NULL;
    }
  
  // Recover extra space.
  //output->Squeeze();
  
  // Clean up temporary images.
  vectors->Delete();
  derivatives->Delete();
  magnitudes->Delete();
}





//----------------------------------------------------------------------------
// This method computes magnitude of the vectors internally.
void vtkImageToMaximumSurface::ComputeMagnitudes(vtkImageRegion *vectors,
						 vtkImageRegion *magnitudes)
{
  int idx0, idx1, idx2;
  int min0, max0, min1, max1, min2, max2, min3, max3;
  int vInc0, vInc1, vInc2, vInc3, mInc0, mInc1, mInc2;
  float *vPtr0, *vPtr1, *vPtr2, *vPtr3, *mPtr0, *mPtr1, *mPtr2;
  float sum;
  
  // Set up the derivative image
  magnitudes->SetScalarType(VTK_FLOAT);
  magnitudes->SetExtent(3, vectors->GetExtent());
  vectors->GetExtent(min0, max0, min1, max1, min2, max2, min3, max3);
  if (min3 != 0 || max3 != 2)
    {
    vtkErrorMacro(<< "Expecting 3 component vectors, not " << max3 - min3 + 1);
    return;
    }
  // Get information to loop through images.
  vPtr2 = (float *)(vectors->GetScalarPointer());
  mPtr2 = (float *)(magnitudes->GetScalarPointer());
  vectors->GetIncrements(vInc0, vInc1, vInc2, vInc3);
  magnitudes->GetIncrements(mInc0, mInc1, mInc2);
  
  for (idx2 = min2; idx2 <= max2; ++idx2)
    {
    vPtr1 = vPtr2;
    mPtr1 = mPtr2;
    for (idx1 = min1; idx1 <= max1; ++idx1)
      {
      vPtr0 = vPtr1;
      mPtr0 = mPtr1;
      for (idx0 = min0; idx0 <= max0; ++idx0)
	{
	vPtr3 = vPtr0;
	sum = *vPtr3 * *vPtr3;
	vPtr3 += vInc3;
	sum += *vPtr3 * *vPtr3;
	vPtr3 += vInc3;
	sum += *vPtr3 * *vPtr3;
	
	*mPtr0 = sqrt(sum);
	vPtr0 += vInc0;
	mPtr0 += mInc0;
	}
      vPtr1 += vInc1;
      mPtr1 += mInc1;
      }
    vPtr2 += vInc2;
    mPtr2 += mInc2;
    }
}

//----------------------------------------------------------------------------
// This method computes the scalar derivative from vector and magnitude 
// images.  Derivitive is computed by taking the dot product of the vector 
// with the magnitude gradient.
void vtkImageToMaximumSurface::ComputeDerivatives(vtkImageRegion *vectors,
						  vtkImageRegion *magnitudes,
						  vtkImageRegion *derivatives)
{
  int idx0, idx1, idx2;
  int min0, max0, min1, max1, min2, max2, min3, max3;
  int vInc0, vInc1, vInc2, vInc3, mInc0, mInc1, mInc2, dInc0, dInc1, dInc2;
  float *vPtr0, *vPtr1, *vPtr2, *vPtr3;
  float *mPtr0, *mPtr1, *mPtr2, *dPtr0, *dPtr1, *dPtr2;
  float valLeft, valRight;
  float dot;
  
  // Set up the derivative image
  derivatives->SetScalarType(VTK_FLOAT);
  derivatives->SetExtent(3, vectors->GetExtent());
  vectors->GetExtent(min0, max0, min1, max1, min2, max2, min3, max3);
  if (min3 != 0 || max3 != 2)
    {
    vtkErrorMacro(<< "Expecting 3 component vectors, not " << max3 - min3 + 1);
    return;
    }
  // Get information to loop through images.
  vPtr2 = (float *)(vectors->GetScalarPointer());
  mPtr2 = (float *)(magnitudes->GetScalarPointer());
  dPtr2 = (float *)(derivatives->GetScalarPointer());
  vectors->GetIncrements(vInc0, vInc1, vInc2, vInc3);
  magnitudes->GetIncrements(mInc0, mInc1, mInc2);
  derivatives->GetIncrements(dInc0, dInc1, dInc2);
  
  for (idx2 = min2; idx2 <= max2; ++idx2)
    {
    vPtr1 = vPtr2;
    mPtr1 = mPtr2;
    dPtr1 = dPtr2;
    for (idx1 = min1; idx1 <= max1; ++idx1)
      {
      vPtr0 = vPtr1;
      mPtr0 = mPtr1;
      dPtr0 = dPtr1;
      for (idx0 = min0; idx0 <= max0; ++idx0)
	{
	vPtr3 = vPtr0;
	// Central differences and dot product
	valLeft = (idx0 == min0) ? *mPtr0 : mPtr0[-mInc0];
	valRight = (idx0 == max0) ? *mPtr0 : mPtr0[mInc0];
	dot = (valRight - valLeft) * *vPtr3;
	vPtr3 += vInc3;
	valLeft = (idx1 == min1) ? *mPtr0 : mPtr0[-mInc1];
	valRight = (idx1 == max1) ? *mPtr0 : mPtr0[mInc1];
	dot += (valRight - valLeft) * *vPtr3;
	vPtr3 += vInc3;
	valLeft = (idx2 == min2) ? *mPtr0 : mPtr0[-mInc2];
	valRight = (idx2 == max2) ? *mPtr0 : mPtr0[mInc2];
	dot += (valRight - valLeft) * *vPtr3;
	// Save dot in derivative
	*dPtr0 = dot;

	vPtr0 += vInc0;
	mPtr0 += mInc0;
	dPtr0 += dInc0;
	}
      vPtr1 += vInc1;
      mPtr1 += mInc1;
      dPtr1 += dInc1;
      }
    vPtr2 += vInc2;
    mPtr2 += mInc2;
    dPtr2 += dInc2;
    }
}


//----------------------------------------------------------------------------
// This method selectively applies marching cubes (iso surface = 0.0)
// to the derivative (second derivative because vector was first).
// the cube is ignored if the magnitude values are below the 
// MagnitudeThreshold.
void vtkImageToMaximumSurface::March(vtkImageRegion *derivatives,
				     vtkImageRegion *magnitudes,
				     vtkImageRegion *vectors,
				     float *origin, float *ratio)
{
  int idx0, idx1, idx2;
  int min0, max0, min1, max1, min2, max2;
  int mInc0, mInc1, mInc2, dInc0, dInc1, dInc2;
  int vInc0, vInc1, vInc2, vInc3;
  float *mPtr0, *mPtr1, *mPtr2, *dPtr0, *dPtr1, *dPtr2;
  float *vPtr0, *vPtr1, *vPtr2;
  float t = this->Threshold;
  float cubeMags[8], cubeDers[8], *cubeVects[8];
  
  // Get information to loop through images.
  magnitudes->GetExtent(min0, max0, min1, max1, min2, max2);
  vPtr2 = (float *)(vectors->GetScalarPointer());
  mPtr2 = (float *)(magnitudes->GetScalarPointer());
  dPtr2 = (float *)(derivatives->GetScalarPointer());
  vectors->GetIncrements(vInc0, vInc1, vInc2, vInc3);
  magnitudes->GetIncrements(mInc0, mInc1, mInc2);
  derivatives->GetIncrements(dInc0, dInc1, dInc2);

  // Initialize the internal locator
  this->InitializeLocator(min0, max0, min1, max1);

  // Loop over all the cubes
  for (idx2 = min2; idx2 < max2; ++idx2)
    {
    vPtr1 = vPtr2;
    mPtr1 = mPtr2;
    dPtr1 = dPtr2;
    for (idx1 = min1; idx1 < max1; ++idx1)
      {
      vPtr0 = vPtr1;
      mPtr0 = mPtr1;
      dPtr0 = dPtr1;
      for (idx0 = min0; idx0 < max0; ++idx0)
	{
	// put magnitudes into the cube structure.
	cubeMags[0] = mPtr0[0];
	cubeMags[1] = mPtr0[mInc0];
	cubeMags[3] = mPtr0[mInc1];
	cubeMags[4] = mPtr0[mInc2];
	cubeMags[2] = mPtr0[mInc0 + mInc1];
	cubeMags[5] = mPtr0[mInc0 + mInc2];
	cubeMags[7] = mPtr0[mInc1 + mInc2];
	cubeMags[6] = mPtr0[mInc0 + mInc1 + mInc2];

	// check magnitudes to make sure they are ALL above threshold
	// Maybe we should interploate magnitudes?
	if (cubeMags[0] > t || cubeMags[1] > t || cubeMags[2] > t ||
	    cubeMags[3] > t || cubeMags[4] > t || cubeMags[5] > t || 
	    cubeMags[6] > t || cubeMags[7] > t)
	  {
	  // put derivatives into the cube structure.
	  cubeDers[0] = dPtr0[0];
	  cubeDers[1] = dPtr0[dInc0];
	  cubeDers[3] = dPtr0[dInc1];
	  cubeDers[4] = dPtr0[dInc2];
	  cubeDers[2] = dPtr0[dInc0 + dInc1];
	  cubeDers[5] = dPtr0[dInc0 + dInc2];
	  cubeDers[7] = dPtr0[dInc1 + dInc2];
	  cubeDers[6] = dPtr0[dInc0 + dInc1 + dInc2];
	  // put vectors into the cube structure.
	  if (this->ComputeNormals)
	    {
	    cubeVects[0] = vPtr0;
	    cubeVects[1] = vPtr0 + vInc0;
	    cubeVects[3] = vPtr0 + vInc1;
	    cubeVects[4] = vPtr0 + vInc2;
	    cubeVects[2] = vPtr0 + vInc0 + vInc1;
	    cubeVects[5] = vPtr0 + vInc0 + vInc2;
	    cubeVects[7] = vPtr0 + vInc1 + vInc2;
	    cubeVects[6] = vPtr0 + vInc0 + vInc1 + vInc2;
	    }
	  
	  this->HandleCube(idx0, idx1, idx2, origin, ratio, cubeDers, cubeMags,
			   cubeVects, vInc3);
	  }
	vPtr0 += vInc0;
	mPtr0 += mInc0;
	dPtr0 += dInc0;
	}
      vPtr1 += vInc1;
      mPtr1 += mInc1;
      dPtr1 += dInc1;
      }
    vPtr2 += vInc2;
    mPtr2 += mInc2;
    dPtr2 += dInc2;
    this->IncrementLocatorZ();
    }  

  // release the locators memory
  this->DeleteLocator();
}





//----------------------------------------------------------------------------
// This method runs marching cubes on one cube.
// Iso surface 0.0;
void vtkImageToMaximumSurface::HandleCube(int cellX, int cellY, int cellZ,
					  float *origin, float *ratio,
					  float *derivatives,
					  float *magnitudes, float **vectors,
					  int vInc3)
{
  int index, ii, pointIds[3];
  float *pf;
  TRIANGLE_CASES *triCase;
  EDGE_LIST  *edge;
  
  // compute the case index
  index = 0;
  pf = derivatives;
  if (*pf++ > 0.0) index += 1;
  if (*pf++ > 0.0) index += 2;
  if (*pf++ > 0.0) index += 4;
  if (*pf++ > 0.0) index += 8;
  if (*pf++ > 0.0) index += 16;
  if (*pf++ > 0.0) index += 32;
  if (*pf++ > 0.0) index += 64;
  if (*pf > 0.0) index += 128;
  // no triangles ?
  if (index == 0 || index == 255)
    {
    return;
    }
  // Get edges.
  triCase = triCases + index;
  edge = triCase->edges; 
  // loop over triangles  
  while(*edge > -1)
    {
    for (ii=0; ii<3; ++ii, ++edge) //insert triangle
      {
      // Get the index of the point
      pointIds[ii] = this->GetLocatorPoint(cellX, cellY, *edge);
      // If the point has not been created yet
      if (pointIds[ii] == -1)
	{
	pointIds[ii] = this->MakeNewPoint(cellX, cellY, cellZ, origin, ratio,
					  derivatives, magnitudes, vectors, 
					  vInc3, *edge);
	this->AddLocatorPoint(cellX, cellY, *edge, pointIds[ii]);
	}
      }
    this->Triangles->InsertNextCell(3,pointIds);
    }//for each triangle
}





//----------------------------------------------------------------------------
// This method interpolates verticies to make a new point.
int vtkImageToMaximumSurface::MakeNewPoint(int cellX, int cellY, int cellZ,
					   float *origin, float *ratio,
					   float *derivatives,
					   float *magnitudes, float **vectors,
					   int vInc3, int edge)
{
  float temp[3];
  float interpolationFactor;
  int vert0, vert1;
  static int edges[12][2] = { {0,1}, {1,2}, {3,2}, {0,3},
                              {4,5}, {5,6}, {7,6}, {4,7},
                              {0,4}, {1,5}, {3,7}, {2,6}};
  
  // Compute where the iso surface intersects the edge
  vert0 = edges[edge][0];
  vert1 = edges[edge][1];
  interpolationFactor = derivatives[vert0] 
    / (derivatives[vert0] - derivatives[vert1]);
  
  // Interpolate to find magnitude
  if (this->ComputeScalars)
    {
    this->Scalars->InsertNextScalar(magnitudes[vert0] 
	    + interpolationFactor * (magnitudes[vert1] - magnitudes[vert0]));
    }
    
  // Interpolate to find normal from vectors.
  if (this->ComputeNormals)
    {
    float *v0 = vectors[vert0];
    float *v1 = vectors[vert1];
    float sum = 0.0;
    int idx;
    // Interpolate
    for (idx = 0; idx < 3; ++idx)
      {
      temp[idx] = (*v0 + interpolationFactor * (*v1 - *v0));
      v0 += vInc3;
      v1 += vInc3;
      sum += temp[idx] * temp[idx];
      }
    // normalize
    sum = 1.0 / sqrt(sum);
    temp[0] *= sum;
    temp[1] *= sum;
    temp[2] *= sum;
    this->Normals->InsertNextNormal(temp);
    }
  
  // Find the location of the point.
  switch (edge)
    {
    case 1:  edge = 3; ++cellX;          break;
    case 7:  edge = 3; ++cellZ;          break;
    case 5:  edge = 3; ++cellX; ++cellZ; break;
    case 2:  edge = 0; ++cellY;          break;
    case 4:  edge = 0; ++cellZ;          break;
    case 6:  edge = 0; ++cellY; ++cellZ; break;
    case 9:  edge = 8; ++cellX;          break;
    case 10: edge = 8; ++cellY;          break;
    case 11: edge = 8; ++cellX; ++cellY; break;
    }

  switch (edge)
    {
    case 0:
      // interpolate X axis
      temp[0] = *origin++ + ((float)(cellX) + interpolationFactor) * *ratio++;
      temp[1] = *origin++ + (float)(cellY) * *ratio++;
      temp[2] = *origin + (float)(cellZ) * *ratio;
      break;
    case 3:
      // interpolate Y axis
      temp[0] = *origin++ + (float)(cellX) * *ratio++;
      temp[1] = *origin++ + ((float)(cellY) + interpolationFactor) * *ratio++;
      temp[2] = *origin + (float)(cellZ) * *ratio;
      break;
    case 8:
      // interpolate Z axis
      temp[0] = *origin++ + (float)(cellX) * *ratio++;
      temp[1] = *origin++ + (float)(cellY) * *ratio++;
      temp[2] = *origin + ((float)(cellZ) + interpolationFactor) * *ratio;
      break;
    }
  return this->Points->InsertNextPoint(temp);
}



//============================================================================
// These method act as the point locator so verticies will be shared.
// One 2d array of cubes is stored. (z dimension is ignored).
// Points are indexed by their cube and edge.
// Shared edges are only represented once.  Cubes are responsible for
// edges on their min faces.  Their is an extra row and column of cubes
// to store the max edges of the last row/column of cubes,


//----------------------------------------------------------------------------
// This method allocates and initializes the point array.
// One 2d array of cubes is stored. (z dimension is ignored).
void vtkImageToMaximumSurface::InitializeLocator(int min0, int max0, 
						 int min1, int max1)
{
  int idx;
  int size;

  // Free old memory
  if (this->LocatorPointIds)
    {
    delete this->LocatorPointIds;
    }
  // Extra row and column
  this->LocatorDimX = (max0 - min0 + 2);
  this->LocatorDimY = (max1 - min1 + 2);
  this->LocatorMinX = min0;
  this->LocatorMinY = min1;
  // 5 non shared edges.
  size = (this->LocatorDimX)*(this->LocatorDimY)*5;
  this->LocatorPointIds = new int[size];
  // Initialize the array
  for (idx = 0; idx < size; ++idx)
    {
    this->LocatorPointIds[idx] = -1;
    }
}

//----------------------------------------------------------------------------
// This method frees the locators memory.
void vtkImageToMaximumSurface::DeleteLocator()
{
  // Free old memory
  if (this->LocatorPointIds)
    {
    delete this->LocatorPointIds;
    }
}

//----------------------------------------------------------------------------
// This method moves the Z index of the locator up one value.
void vtkImageToMaximumSurface::IncrementLocatorZ()
{
  int x, y;
  int *ptr;

  ptr = this->LocatorPointIds;
  for (y = 0; y < this->LocatorDimY; ++y)
    {
    for (x = 0; x < this->LocatorDimX; ++x)
      {
      ptr[0] = ptr[4];
      ptr[3] = ptr[1];
      ptr[1] = ptr[2] = ptr[4] = -1;
      ptr += 5;
      }
    }
}

//----------------------------------------------------------------------------
// This method adds a point to the array.  Cube is the X/Y cube,
// segment is the index of the segment (same as marching cubes).(XYZ)
// (0,0,0)->(1,0,0): 0,  (1,0,0)->(1,1,0): 1,
// (1,1,0)->(0,1,0): 2,  (0,1,0)->(0,0,0): 3,
// (0,0,1)->(1,0,1): 4,  (1,0,1)->(1,1,1): 5,
// (1,1,1)->(0,1,1): 6,  (0,1,1)->(0,0,1): 7,
// (0,0,0)->(0,0,1): 8,  (1,0,0)->(1,0,1): 9,
// (0,1,0)->(0,1,1): 10, (1,1,0)->(1,1,1): 11.
// Shared edges are computed internaly. (no error checking)
void vtkImageToMaximumSurface::AddLocatorPoint(int cellX, int cellY, int edge,
					       int ptId)
{
  int *ptr;
  
  // Get the correct position in the array.
  ptr = this->GetLocatorPointer(cellX, cellY, edge);
  *ptr = ptId;
}

//----------------------------------------------------------------------------
// This method gets a point from the locator.
int vtkImageToMaximumSurface::GetLocatorPoint(int cellX, int cellY, int edge)
{
  int *ptr;
  
  // Get the correct position in the array.
  ptr = this->GetLocatorPointer(cellX, cellY, edge);
  return *ptr;
}

//----------------------------------------------------------------------------
// This method returns a pointer to an ID from a cube and an edge.
int *vtkImageToMaximumSurface::GetLocatorPointer(int cellX,int cellY,int edge)
{
  // Remove redundant edges (shared by more than one cube).
  // Take care of shared edges
  switch (edge)
    {
    case 9:  ++cellX;          edge = 8; break;
    case 10: ++cellY;          edge = 8; break;
    case 11: ++cellX; ++cellY; edge = 8; break;
    case 5:  ++cellX;          edge = 7; break;
    case 6:  ++cellY;          edge = 4; break;
    case 1:  ++cellX;          edge = 3; break;
    case 2:  ++cellY;          edge = 0; break;
    }
  
  // relative to min and max.
  cellX -= this->LocatorMinX;
  cellY -= this->LocatorMinY;

  // compute new indexes for edges (0 to 4) 
  // must be compatable with LocatorIncrementZ.
  if (edge == 7)
    {
    edge = 1;
    }
  if (edge == 8)
    {
    edge = 2;
    }

  // return correct pointer
  return this->LocatorPointIds + edge 
    + (cellX + cellY * (this->LocatorDimX)) * 5;
}

  
  





