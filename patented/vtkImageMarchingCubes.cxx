/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMarchingCubes.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

    THIS CLASS IS PATENTED UNDER UNITED STATES PATENT NUMBER 4,710,876
    "System and Method for the Display of Surface Structures Contained
    Within The Interior Region of a Solid body".
    Application of this software for commercial purposes requires 
    a license grant from GE. Contact:
        Jerald Roehling
        GE Licensing
        One Independence Way
        PO Box 2023
        Princeton, N.J. 08540
        phone 609-734-9823
        e-mail:Roehlinj@gerlmo.ge.com
    for more information.

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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <math.h>
#include "vtkMarchingCubesCases.h"
#include "vtkImageMarchingCubes.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"


//------------------------------------------------------------------------------
vtkImageMarchingCubes* vtkImageMarchingCubes::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageMarchingCubes");
  if(ret)
    {
    return (vtkImageMarchingCubes*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageMarchingCubes;
}




//----------------------------------------------------------------------------
// Description:
// Construct object with initial range (0,1) and single contour value
// of 0.0. ComputeNormal is on, ComputeGradients is off and ComputeScalars is on.
vtkImageMarchingCubes::vtkImageMarchingCubes()
{
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

void vtkImageMarchingCubes::Update()
{
  if ( ! this->GetInput())
    {
    vtkErrorMacro("No Input");
    return;
    }
  
  if (this->GetOutput())
    {
    this->GetOutput()->Initialize(); //clear output
    }
  this->AbortExecute = 0;
  this->Progress = 0.0;
  this->Execute();
}

//----------------------------------------------------------------------------
void vtkImageMarchingCubes::Execute()
{
  vtkImageData *inData = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  int extent[8], estimatedSize;
  int temp, zMin, zMax, chunkMin, chunkMax;
  int minSlicesPerChunk, chunkOverlap;
  int numContours=this->ContourValues->GetNumberOfContours();
  float *values=this->ContourValues->GetValues();
  
  vtkDebugMacro("Starting Execute Method");
  if ( ! inData)
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
  inData->UpdateInformation();
  // Each data type requires a different amount of memory.
  switch (inData->GetScalarType())
    {
    case VTK_FLOAT:
      temp = sizeof(float);
      break;
    case VTK_DOUBLE:
      temp = sizeof(double);
      break;
    case VTK_INT:
      temp = sizeof(int);
      break;
    case VTK_UNSIGNED_INT:
      temp = sizeof(unsigned int);
      break;
    case VTK_LONG:
      temp = sizeof(long);
      break;
    case VTK_UNSIGNED_LONG:
      temp = sizeof(long);
      break;
    case VTK_SHORT:
      temp = sizeof(short);
      break;
    case VTK_UNSIGNED_SHORT:
      temp = sizeof(unsigned short);
      break;
    case VTK_CHAR:
      temp = sizeof(char);
      break;
    case VTK_UNSIGNED_CHAR:
      temp = sizeof(unsigned char);
      break;
    default:
      vtkErrorMacro(<< "Could not determine input scalar type.");
      return;
    }
  inData->GetWholeExtent(extent);
  // multiply by the area of each slice
  temp *= extent[1] - extent[0] + 1;
  temp *= extent[3] - extent[2] + 1;
  temp = temp;
  // temp holds memory per image. (+1 to avoid dividing by zero)
  this->NumberOfSlicesPerChunk = this->InputMemoryLimit * 1000 / (temp + 1);
  if (this->NumberOfSlicesPerChunk < minSlicesPerChunk)
    {
    vtkWarningMacro("Execute: Need " 
      <<  minSlicesPerChunk*(temp/1000) << " KB to load " 
      << minSlicesPerChunk << " slices.\n");
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
  this->Points = vtkPoints::New();
  this->Points->Allocate(estimatedSize,estimatedSize/2);
  this->Triangles = vtkCellArray::New();
  this->Triangles->Allocate(estimatedSize,estimatedSize/2);
  if (this->ComputeScalars)
    {
    this->Scalars = vtkScalars::New();
    this->Scalars->Allocate(estimatedSize,estimatedSize/2);
    }
  if (this->ComputeNormals)
    {
    this->Normals = vtkNormals::New();
    this->Normals->Allocate(estimatedSize,estimatedSize/2);
    }
  if (this->ComputeGradients)
    {
    this->Gradients = vtkVectors::New();
    this->Gradients->Allocate(estimatedSize,estimatedSize/2);
    }

  // Initialize the internal point locator (edge table for oen image of cubes).
  this->InitializeLocator(extent[0], extent[1], extent[2], extent[3]);
  
  // Loop through the chunks running marching cubes on each one
  zMin = extent[4];
  zMax = extent[5];
  // to avoid warnings
  chunkMax = zMin + this->NumberOfSlicesPerChunk;
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
    // Get the chunk from the input
    inData->SetUpdateExtent(extent);
    inData->Update();
    
    this->InvokeEvent(vtkCommand::StartEvent,NULL);
    this->March(inData, chunkMin, chunkMax, numContours, values);
    if ( !this->AbortExecute )
      {
      this->UpdateProgress(1.0);
      }
    this->InvokeEvent(vtkCommand::EndEvent,NULL);

    if (inData->ShouldIReleaseData())
      {
      inData->ReleaseData();
      }
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
    g[0] = (float)(ptr[inc0]) - (float)(*ptr);
    }
  else if (b0 > 0)
    {
    g[0] = (float)(*ptr) - (float)(ptr[-inc0]);
    }
  else
    {
    g[0] = (float)(ptr[inc0]) - (float)(ptr[-inc0]);
    }

  if (b1 < 0)
    {
    g[1] = (float)(ptr[inc1]) - (float)(*ptr);
    }
  else if (b1 > 0)
    {
    g[1] = (float)(*ptr) - (float)(ptr[-inc1]);
    }
  else
    {
    g[1] = (float)(ptr[inc1]) - (float)(ptr[-inc1]);
    }

  if (b2 < 0)
    {
    g[2] = (float)(ptr[inc2]) - (float)(*ptr);
    }
  else if (b2 > 0)
    {
    g[2] = (float)(*ptr) - (float)(ptr[-inc2]);
    }
  else
    {
    g[2] = (float)(ptr[inc2]) - (float)(ptr[-inc2]);
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
  int edgeAxis = 0;
  T *ptrB = NULL;
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
    if (idx0 == imageExtent[0])
      {
      b0 = -1;
      }
    b1 = (idx1 == imageExtent[3]);
    if (idx1 == imageExtent[2])
      {
      b1 = -1;
      }
    b2 = (idx2 == imageExtent[5]);
    if (idx2 == imageExtent[4])
      {
      b2 = -1;
      }
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
					   vtkImageData *inData,
					   T *ptr, int numContours, float *values)
{
  int inc0, inc1, inc2;
  int valueIdx;
  float value;
  int cubeIndex, ii;
  vtkIdType pointIds[3];
  TRIANGLE_CASES *triCase;
  EDGE_LIST  *edge;

  inData->GetIncrements(inc0, inc1, inc2);
  for (valueIdx = 0; valueIdx < numContours; ++valueIdx)
    {
    value = values[valueIdx];
    // compute the case index
    cubeIndex = 0;
    if ((float)(ptr[0]) > value)
      {
      cubeIndex += 1;
      }
    if ((float)(ptr[inc0]) > value)
      {
      cubeIndex += 2;
      }
    if ((float)(ptr[inc0 + inc1]) > value)
      {
      cubeIndex += 4;
      }
    if ((float)(ptr[inc1]) > value)
      {
      cubeIndex += 8;
      }
    if ((float)(ptr[inc2]) > value)
      {
      cubeIndex += 16;
      }
    if ((float)(ptr[inc0 + inc2]) > value)
      {
      cubeIndex += 32;
      }
    if ((float)(ptr[inc0 + inc1 + inc2]) > value)
      {
      cubeIndex += 64;
      }
    if ((float)(ptr[inc1 + inc2]) > value)
      {
      cubeIndex += 128;
      }
    // Make sure we have trianlges
    if (cubeIndex != 0 && cubeIndex != 255)
      {
      // Get edges.
      triCase = VTK_MARCHING_CUBES_TRICASES + cubeIndex;
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
	    float *spacing = inData->GetSpacing();
	    float *origin = inData->GetOrigin();
	    int *extent = self->GetInput()->GetWholeExtent();
	    
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
template <class T>
static void vtkImageMarchingCubesMarch(vtkImageMarchingCubes *self,
				      vtkImageData *inData, T *ptr,
				      int chunkMin, int chunkMax,
                                      int numContours, float *values)
{
  int idx0, idx1, idx2;
  int min0, max0, min1, max1, min2, max2;
  int inc0, inc1, inc2;
  T *ptr0, *ptr1, *ptr2;
  unsigned long target, count;
  
  // avoid warnings
  ptr = ptr;
  
  // Get information to loop through images.
  inData->GetExtent(min0, max0, min1, max1, min2, max2);
  ptr2 = (T *)(inData->GetScalarPointer(min0, min1, chunkMin));
  inData->GetIncrements(inc0, inc1, inc2);

  // Setup the progress reporting
  target = (unsigned long)((max0-min0+1) * (max1-min1+1) / 50.0);
  ++target;
  count = 0;
  
  // Loop over all the cubes
  for (idx2 = chunkMin; idx2 < chunkMax; ++idx2)
    {
    ptr1 = ptr2;
    for (idx1 = min1; idx1 < max1; ++idx1)
      {
      // update progress if necessary
      if (!(count%target))
	{
	self->UpdateProgress(count/(50.0*target));
        if (self->GetAbortExecute())
          {
          return;
          }
	}
      count++;
      // continue with last loop
      ptr0 = ptr1;
      for (idx0 = min0; idx0 < max0; ++idx0)
	{
	// put magnitudes into the cube structure.
	vtkImageMarchingCubesHandleCube(self, idx0, idx1, idx2, inData, ptr0,
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
void vtkImageMarchingCubes::March(vtkImageData *inData, 
				 int chunkMin, int chunkMax,
                                 int numContours, float *values)
{
  void *ptr = inData->GetScalarPointer();
  
  switch (inData->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageMarchingCubesMarch(this, inData, (float *)(ptr), 
				chunkMin, chunkMax, numContours, values);
      break;
    case VTK_DOUBLE:
      vtkImageMarchingCubesMarch(this, inData, (double *)(ptr), 
				chunkMin, chunkMax, numContours, values);
      break;
    case VTK_INT:
      vtkImageMarchingCubesMarch(this, inData, (int *)(ptr), 
				chunkMin, chunkMax, numContours, values);
      break;
    case VTK_UNSIGNED_INT:
      vtkImageMarchingCubesMarch(this, inData, (unsigned int *)(ptr), 
				chunkMin, chunkMax, numContours, values);
      break;
    case VTK_SHORT:
      vtkImageMarchingCubesMarch(this, inData, (short *)(ptr), 
				chunkMin, chunkMax, numContours, values);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageMarchingCubesMarch(this, inData, (unsigned short *)(ptr), 
				chunkMin, chunkMax, numContours, values);
      break;
    case VTK_CHAR:
      vtkImageMarchingCubesMarch(this, inData, (char *)(ptr), 
				chunkMin, chunkMax, numContours, values);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageMarchingCubesMarch(this, inData, (unsigned char *)(ptr), 
				chunkMin, chunkMax, numContours, values);
      break;
    case VTK_LONG:
      vtkImageMarchingCubesMarch(this, inData, (long *)(ptr), 
				chunkMin, chunkMax, numContours, values);
      break;
    case VTK_UNSIGNED_LONG:
      vtkImageMarchingCubesMarch(this, inData, (unsigned long *)(ptr), 
				chunkMin, chunkMax, numContours, values);
      break;
    default:
      vtkErrorMacro(<< "Unknown output ScalarType");
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
    delete [] this->LocatorPointIds;
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
    delete [] this->LocatorPointIds;
    this->LocatorPointIds = NULL;
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
void vtkImageMarchingCubes::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageMarchingCubes::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Inputs[0]);
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

