/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMarchingCubes.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

    THIS CLASS IS PATENTED UNDER UNITED STATES PATENT NUMBER 4,710,876
    "System and Method for the Display of Surface Structures Contained
    Within The Interior Region of a Solid body".
    Application of this software for commercial purposes requires 
    a license grant from GE. Contact:
        Mike Silver
        GE Medical Systems
        16705 West Lincoln Ave., 
        NB 900
        New Berlin, WI, 53151
        Phone:1-414-827-3400 
    for more information.

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
#include "vtkImageMarchingCubes.h"
#include "vtkImageRegion.h"

//----------------------------------------------------------------------------
// Description:
// Construct object with initial range (0,1) and single contour value
// of 0.0. ComputeNormal is on, ComputeGradients is off and ComputeScalars is on.
vtkImageMarchingCubes::vtkImageMarchingCubes()
{
  this->Input = NULL;
  this->ContourValues = vtkContourValues::New();
  this->ComputeNormals = 1;
  this->ComputeGradients = 0;
  this->ComputeScalars = 1;

  this->LocatorPointIds = NULL;
  this->InputMemoryLimit = 10000;  // 10 mega Bytes
}

vtkImageMarchingCubes::~vtkImageMarchingCubes()
{
  this->ContourValues->Delete();
}

// Description:
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
unsigned long vtkImageMarchingCubes::GetMTime()
{
  unsigned long mTime=this->vtkPolyDataSource::GetMTime();
  unsigned long contourValuesMTime=this->ContourValues->GetMTime();
 
  mTime = ( contourValuesMTime > mTime ? contourValuesMTime : mTime );

  return mTime;
}


//----------------------------------------------------------------------------
void vtkImageMarchingCubes::Execute()
{
  vtkImageRegion *inRegion;
  vtkPolyData *output = this->GetOutput();
  int extent[6], estimatedSize;
  int temp, zMin, zMax, chunkMin, chunkMax;
  int minSlicesPerChunk, chunkOverlap;
  int numContours=this->ContourValues->GetNumberOfContours();
  float *values=this->ContourValues->GetValues();
  
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "No Input");
    return;
    }
  
  // Gradients must be computed (but not saved) if Compute normals is on.
  this->NeedGradients = this->ComputeGradients || this->ComputeNormals;
  
  // Determine the number of slices per request from input memory limit.
  if (this->NeedGradients)
    {
    minSlicesPerChunk = 4;
    chunkOverlap = 3;
    }
  else
    {
    minSlicesPerChunk = 2;
    chunkOverlap = 1;
    }
  inRegion = vtkImageRegion::New();
  this->Input->UpdateImageInformation(inRegion);
  // Each data type requires a different amount of memory.
  switch (this->Input->GetScalarType())
    {
    case VTK_FLOAT:
      temp = sizeof(float);
      break;
    case VTK_INT:
      temp = sizeof(int);
      break;
    case VTK_SHORT:
      temp = sizeof(short);
      break;
    case VTK_UNSIGNED_SHORT:
      temp = sizeof(unsigned short);
      break;
    case VTK_UNSIGNED_CHAR:
      temp = sizeof(unsigned char);
      break;
    default:
      vtkErrorMacro(<< "Could not determine input scalar type.");
      return;
    }
  inRegion->GetImageExtent(3, extent);
  // multiply by the area of each slice
  temp *= extent[1] - extent[0] + 1;
  temp *= extent[3] - extent[2] + 1;
  temp = temp;
  // temp holds memory per image. (+1 to avoid dividing by zero)
  this->NumberOfSlicesPerChunk = this->InputMemoryLimit * 1000 / (temp + 1);
  if (this->NumberOfSlicesPerChunk < minSlicesPerChunk)
    {
    vtkWarningMacro("Execute: Need " << (temp/1000) << " KB to load " 
		    << minSlicesPerChunk << " minimum.\n");
    this->NumberOfSlicesPerChunk = minSlicesPerChunk;
    }
  vtkDebugMacro("Execute: NumberOfSlicesPerChunk = " 
		<< this->NumberOfSlicesPerChunk);
  this->NumberOfSlicesPerChunk -= chunkOverlap;
  
  // Create the points, scalars, normals and Cell arrays for the output.
  // estimate the number of points from the volume dimensions
  estimatedSize = (int) pow ((double) ((extent[1]-extent[0]+1) * 
				       (extent[3]-extent[2]+1) * 
				       (extent[5]-extent[4]+1)), 0.75);
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024) 
    {
    estimatedSize = 1024;
    }
  vtkDebugMacro(<< "Estimated number of points/triangles: " << estimatedSize);
  this->Points = vtkFloatPoints::New();
  this->Points->Allocate(estimatedSize,estimatedSize/2);
  this->Triangles = vtkCellArray::New();
  this->Triangles->Allocate(estimatedSize,estimatedSize/2);
  if (this->ComputeScalars)
    {
    this->Scalars = vtkFloatScalars::New();
    this->Scalars->Allocate(estimatedSize,estimatedSize/2);
    }
  if (this->ComputeNormals)
    {
    this->Normals = vtkFloatNormals::New();
    this->Normals->Allocate(estimatedSize,estimatedSize/2);
    }
  if (this->ComputeGradients)
    {
    this->Gradients = vtkFloatVectors::New();
    this->Gradients->Allocate(estimatedSize,estimatedSize/2);
    }

  // Initialize the internal point locator (edge table for oen image of cubes).
  this->InitializeLocator(extent[0], extent[1], extent[2], extent[3]);
  
  // Loop through the chunks running marching cubes on each one
  zMin = extent[4];
  zMax = extent[5];
  for(chunkMin = zMin; chunkMin < zMax; chunkMin = chunkMax)
    {
    // Get the chunk from the input
    chunkMax = chunkMin + this->NumberOfSlicesPerChunk;
    if (chunkMax > zMax)
      {
      chunkMax = zMax;
      }
    extent[4] = chunkMin;
    extent[5] = chunkMax;
    // Expand if computing gradients with central differences
    if (this->NeedGradients)
      {
      --extent[4];
      ++extent[5];
      }
    // Don't go over boundary of data.
    if (extent[4] < zMin)
      {
      extent[4] = zMin;
      }
    if (extent[5] > zMax)
      {
      extent[5] = zMax;
      }
    inRegion->SetExtent(3, extent);
    // Get the chunk from the input
    this->Input->Update(inRegion);
    
    this->March(inRegion, chunkMin, chunkMax, numContours, values);
    }
  
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
  output->Squeeze();
  
  // Clean up temporary images.
  inRegion->Delete();
  // release the locators memory
  this->DeleteLocator();
}

//----------------------------------------------------------------------------
// This method uses central differences to compute the gradient
// of a point. Note: This method assumes that max > min for all 3 axes!
// It does not consider the dataset spacing.
// b0 (b1, b2) indicates the boundary conditions for the three axes.: 
// b0 = -1 => pixel is on x axis minimum of region.
// b0 = 0 => no boundary conditions
// b0 = +1 => pixel is on x axis maximum of region.
template <class T>
static void vtkImageMarchingCubesComputePointGradient(T *ptr, float *g,
					     int inc0, int inc1, int inc2,
					     short b0, short b1, short b2)
{
  if (b0 < 0)
    {
    g[0] = ptr[inc0] - *ptr;
    }
  else if (b0 > 0)
    {
    g[0] = *ptr - ptr[-inc0];
    }
  else
    {
    g[0] = ptr[inc0] - ptr[-inc0];
    }

  if (b1 < 0)
    {
    g[1] = ptr[inc1] - *ptr;
    }
  else if (b1 > 0)
    {
    g[1] = *ptr - ptr[-inc1];
    }
  else
    {
    g[1] = ptr[inc1] - ptr[-inc1];
    }

  if (b2 < 0)
    {
    g[2] = ptr[inc2] - *ptr;
    }
  else if (b2 > 0)
    {
    g[2] = *ptr - ptr[-inc2];
    }
  else
    {
    g[2] = ptr[inc2] - ptr[-inc2];
    }
}


//----------------------------------------------------------------------------
// This method interpolates verticies to make a new point.
template <class T>
static int vtkImageMarchingCubesMakeNewPoint(vtkImageMarchingCubes *self,
					    int idx0, int idx1, int idx2,
					    int inc0, int inc1, int inc2,
					    T *ptr, int edge, 
					    int *imageExtent,
					    float *spacing, float *origin,
					    float value)
{
  int edgeAxis;
  T *ptrB;
  float temp, pt[3];

  // decode the edge into starting point and axis direction
  switch (edge)
    {
    case 0:  // 0,1
      ptrB = ptr + inc0;
      edgeAxis = 0;
      break;
    case 1:  // 1,2
      ++idx0;
      ptr += inc0;
      ptrB = ptr + inc1;
      edgeAxis = 1;
      break;
    case 2:  // 3,2
      ++idx1;
      ptr += inc1;
      ptrB = ptr + inc0;
      edgeAxis = 0;
      break;
    case 3:  // 0,3
      ptrB = ptr + inc1;
      edgeAxis = 1;
      break;
    case 4:  // 4,5
      ++idx2;
      ptr += inc2;
      ptrB = ptr + inc0;
      edgeAxis = 0;
      break;
    case 5:  // 5,6
      ++idx0; ++idx2;
      ptr += inc0 + inc2;
      ptrB = ptr + inc1;
      edgeAxis = 1;
      break;
    case 6:  // 7,6
      ++idx1; ++idx2;
      ptr += inc1 + inc2;
      ptrB = ptr + inc0;
      edgeAxis = 0;
      break;
    case 7: // 4,7
      ++idx2;
      ptr += inc2;
      ptrB = ptr + inc1;
      edgeAxis = 1;
      break;
    case 8: // 0,4
      ptrB = ptr + inc2;
      edgeAxis = 2;
      break;
    case 9: // 1,5
      ++idx0;
      ptr += inc0;
      ptrB = ptr + inc2;
      edgeAxis = 2;
      break;
    case 10: // 3,7
      ++idx1;
      ptr += inc1;
      ptrB = ptr + inc2;
      edgeAxis = 2;
      break;
    case 11: // 2,6
      ++idx0; ++idx1;
      ptr += inc0 + inc1;
      ptrB = ptr + inc2;
      edgeAxis = 2;
      break;
    }

  // interpolation factor
  temp = (value - *ptr) / (*ptrB - *ptr);
  
  // interpolate the point position
  switch (edgeAxis)
    {
    case 0:
      pt[0] = origin[0] + spacing[0] * ((float)idx0 + temp);
      pt[1] = origin[1] + spacing[1] * ((float)idx1);
      pt[2] = origin[2] + spacing[2] * ((float)idx2);
      break;
    case 1:
      pt[0] = origin[0] + spacing[0] * ((float)idx0);
      pt[1] = origin[1] + spacing[1] * ((float)idx1 + temp);
      pt[2] = origin[2] + spacing[2] * ((float)idx2);
      break;
    case 2:
      pt[0] = origin[0] + spacing[0] * ((float)idx0);
      pt[1] = origin[1] + spacing[1] * ((float)idx1);
      pt[2] = origin[2] + spacing[2] * ((float)idx2 + temp);
      break;
    }
  
  // Save the scale if we are generating scalars
  if (self->ComputeScalars)
    {
    self->Scalars->InsertNextScalar(value);
    }
  
  // Interpolate to find normal from vectors.
  if (self->NeedGradients)
    {
    short b0, b1, b2;
    float g[3], gB[3];
    // Find boundary conditions and compute gradient (first point)
    b0 = (idx0 == imageExtent[1]);
    if (idx0 == imageExtent[0]) b0 = -1;
    b1 = (idx1 == imageExtent[3]);
    if (idx1 == imageExtent[2]) b1 = -1;
    b2 = (idx2 == imageExtent[5]);
    if (idx2 == imageExtent[4]) b2 = -1;
    vtkImageMarchingCubesComputePointGradient(ptr, g, inc0, inc1, inc2, 
					     b0, b1, b2);
    // Find boundary conditions and compute gradient (second point)
    switch (edgeAxis)
      {
      case 0:  
	++idx0;
	b0 = (idx0 == imageExtent[1]);
	break;
      case 1:  
	++idx1;
	b1 = (idx1 == imageExtent[3]);
	break;
      case 2:  
	++idx2;
	b2 = (idx2 == imageExtent[5]);
	break;
      }
    vtkImageMarchingCubesComputePointGradient(ptrB, gB, inc0, inc1, inc2, 
					     b0, b1, b2);
    // Interpolate Gradient
    g[0] = (g[0] + temp * (gB[0] - g[0])) / spacing[0];
    g[1] = (g[1] + temp * (gB[1] - g[1])) / spacing[1];
    g[2] = (g[2] + temp * (gB[2] - g[2])) / spacing[2];
    if (self->ComputeGradients)
      {
      self->Gradients->InsertNextVector(g);
      }
    if (self->ComputeNormals)
      {
      temp = -1.0 / sqrt(g[0]*g[0] + g[1]*g[1] + g[2]*g[2]);
      g[0] *= temp;
      g[1] *= temp;
      g[2] *= temp;
      self->Normals->InsertNextNormal(g);
      }
    }
  
  return self->Points->InsertNextPoint(pt);
}

//----------------------------------------------------------------------------
// This method runs marching cubes on one cube.
template <class T>
static void vtkImageMarchingCubesHandleCube(vtkImageMarchingCubes *self,
					   int cellX, int cellY, int cellZ,
					   vtkImageRegion *inRegion,
					   T *ptr, int numContours, float *values)
{
  int inc0, inc1, inc2;
  int valueIdx;
  float value;
  int cubeIndex, ii, pointIds[3];
  TRIANGLE_CASES *triCase;
  EDGE_LIST  *edge;

  inRegion->GetIncrements(inc0, inc1, inc2);
  for (valueIdx = 0; valueIdx < numContours; ++valueIdx)
    {
    value = values[valueIdx];
    // compute the case index
    cubeIndex = 0;
    if ((float)(ptr[0]) > value) cubeIndex += 1;
    if ((float)(ptr[inc0]) > value) cubeIndex += 2;
    if ((float)(ptr[inc0 + inc1]) > value) cubeIndex += 4;
    if ((float)(ptr[inc1]) > value) cubeIndex += 8;
    if ((float)(ptr[inc2]) > value) cubeIndex += 16;
    if ((float)(ptr[inc0 + inc2]) > value) cubeIndex += 32;
    if ((float)(ptr[inc0 + inc1 + inc2]) > value) cubeIndex += 64;
    if ((float)(ptr[inc1 + inc2]) > value) cubeIndex += 128;
    // Make sure we have trianlges
    if (cubeIndex != 0 && cubeIndex != 255)
      {
      // Get edges.
      triCase = triCases + cubeIndex;
      edge = triCase->edges; 
      // loop over triangles  
      while(*edge > -1)
	{
	for (ii=0; ii<3; ++ii, ++edge) //insert triangle
	  {
	  // Get the index of the point
	  pointIds[ii] = self->GetLocatorPoint(cellX, cellY, *edge);
	  // If the point has not been created yet
	  if (pointIds[ii] == -1)
	    {
	    float *spacing = inRegion->GetSpacing();
	    float *origin = inRegion->GetOrigin();
	    int *extent = inRegion->GetImageExtent();
	    
	    pointIds[ii] = vtkImageMarchingCubesMakeNewPoint(self,
					      cellX, cellY, cellZ,
					      inc0, inc1, inc2,
					      ptr, *edge, extent,
					      spacing, origin, value);
	    self->AddLocatorPoint(cellX, cellY, *edge, pointIds[ii]);
	    }
	  }
	self->Triangles->InsertNextCell(3,pointIds);
	}//for each triangle
      }
    }
}

//----------------------------------------------------------------------------
// This method selectively applies marching cubes (iso surface = 0.0)
// to the derivative (second derivative because vector was first).
// the cube is ignored if the magnitude values are below the 
// MagnitudeThreshold.
template <class T>
static void vtkImageMarchingCubesMarch(vtkImageMarchingCubes *self,
				      vtkImageRegion *inRegion, T *ptr,
				      int chunkMin, int chunkMax,
                                      int numContours, float *values)
{
  int idx0, idx1, idx2;
  int min0, max0, min1, max1, min2, max2;
  int inc0, inc1, inc2;
  T *ptr0, *ptr1, *ptr2;

  // avoid warnings
  ptr = ptr;
  
  // Get information to loop through images.
  inRegion->GetExtent(min0, max0, min1, max1, min2, max2);
  ptr2 = (T *)(inRegion->GetScalarPointer(min0, min1, chunkMin));
  inRegion->GetIncrements(inc0, inc1, inc2);

  // Loop over all the cubes
  for (idx2 = chunkMin; idx2 < chunkMax; ++idx2)
    {
    ptr1 = ptr2;
    for (idx1 = min1; idx1 < max1; ++idx1)
      {
      ptr0 = ptr1;
      for (idx0 = min0; idx0 < max0; ++idx0)
	{
	// put magnitudes into the cube structure.
	vtkImageMarchingCubesHandleCube(self, idx0, idx1, idx2, inRegion, ptr0,
                                       numContours, values);

	ptr0 += inc0;
	}
      ptr1 += inc1;
      }
    ptr2 += inc2;
    self->IncrementLocatorZ();
    }
}



//----------------------------------------------------------------------------
// This method calls the proper templade function.
void vtkImageMarchingCubes::March(vtkImageRegion *inRegion, 
				 int chunkMin, int chunkMax,
                                 int numContours, float *values)
{
  void *ptr = inRegion->GetScalarPointer();
  
  switch (inRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageMarchingCubesMarch(this, inRegion, (float *)(ptr), 
				chunkMin, chunkMax, numContours, values);
      break;
    case VTK_INT:
      vtkImageMarchingCubesMarch(this, inRegion, (int *)(ptr), 
				chunkMin, chunkMax, numContours, values);
      break;
    case VTK_SHORT:
      vtkImageMarchingCubesMarch(this, inRegion, (short *)(ptr), 
				chunkMin, chunkMax, numContours, values);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageMarchingCubesMarch(this, inRegion, (unsigned short *)(ptr), 
				chunkMin, chunkMax, numContours, values);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageMarchingCubesMarch(this, inRegion, (unsigned char *)(ptr), 
				chunkMin, chunkMax, numContours, values);
      break;
    default:
      cerr << "March: Unknown output ScalarType";
      return;
    }
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
void vtkImageMarchingCubes::InitializeLocator(int min0, int max0, 
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
void vtkImageMarchingCubes::DeleteLocator()
{
  // Free old memory
  if (this->LocatorPointIds)
    {
    delete this->LocatorPointIds;
    }
}

//----------------------------------------------------------------------------
// This method moves the Z index of the locator up one slice.
void vtkImageMarchingCubes::IncrementLocatorZ()
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
void vtkImageMarchingCubes::AddLocatorPoint(int cellX, int cellY, int edge,
					       int ptId)
{
  int *ptr;
  
  // Get the correct position in the array.
  ptr = this->GetLocatorPointer(cellX, cellY, edge);
  *ptr = ptId;
}

//----------------------------------------------------------------------------
// This method gets a point from the locator.
int vtkImageMarchingCubes::GetLocatorPoint(int cellX, int cellY, int edge)
{
  int *ptr;
  
  // Get the correct position in the array.
  ptr = this->GetLocatorPointer(cellX, cellY, edge);
  return *ptr;
}

//----------------------------------------------------------------------------
// This method returns a pointer to an ID from a cube and an edge.
int *vtkImageMarchingCubes::GetLocatorPointer(int cellX,int cellY,int edge)
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

//----------------------------------------------------------------------------
void vtkImageMarchingCubes::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataSource::PrintSelf(os,indent);

  this->ContourValues->PrintSelf(os,indent);

  os << indent << "ComputeScalars: " << this->ComputeScalars << "\n";
  os << indent << "ComputeNormals: " << this->ComputeNormals << "\n";
  os << indent << "ComputeGradients: " << this->ComputeGradients << "\n";

  os << indent << "InputMemoryLimit: " << this->InputMemoryLimit <<"K bytes\n";
}

