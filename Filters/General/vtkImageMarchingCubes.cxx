/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMarchingCubes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageMarchingCubes.h"

#include "vtkCellArray.h"
#include "vtkCommand.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkInformationVector.h"
#include "vtkMarchingCubesTriangleCases.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include <math.h>

vtkStandardNewMacro(vtkImageMarchingCubes);

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
  this->InputMemoryLimit = 10240;  // 10 mega Bytes
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
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long contourValuesMTime=this->ContourValues->GetMTime();

  mTime = ( contourValuesMTime > mTime ? contourValuesMTime : mTime );

  return mTime;
}

//----------------------------------------------------------------------------
template <class T>
int vtkImageMarchingCubesGetTypeSize(T*)
{
  return sizeof(T);
}

//----------------------------------------------------------------------------
int vtkImageMarchingCubes::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkImageData *inData = vtkImageData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDemandDrivenPipeline* inputExec =
    vtkDemandDrivenPipeline::SafeDownCast(
      vtkExecutive::PRODUCER()->GetExecutive(inInfo));

  int numContours=this->ContourValues->GetNumberOfContours();
  double *values=this->ContourValues->GetValues();

  vtkDebugMacro("Starting Execute Method");

  // Gradients must be computed (but not saved) if Compute normals is on.
  this->NeedGradients = this->ComputeGradients || this->ComputeNormals;

  // Determine the number of slices per request from input memory limit.
  int minSlicesPerChunk, chunkOverlap;
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
  inputExec->UpdateInformation();
  // Each data type requires a different amount of memory.
  vtkIdType temp;
  switch (inData->GetScalarType())
    {
    vtkTemplateMacro(
      temp = vtkImageMarchingCubesGetTypeSize(static_cast<VTK_TT*>(0))
      );
    default:
      vtkErrorMacro(<< "Could not determine input scalar type.");
      return 1;
    }

  int extent[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
  // multiply by the area of each slice
  temp *= extent[1] - extent[0] + 1;
  temp *= extent[3] - extent[2] + 1;
  // temp holds memory per image. (+1 to avoid dividing by zero)
  this->NumberOfSlicesPerChunk =
    static_cast<int>(this->InputMemoryLimit * 1024 / (temp + 1));
  if (this->NumberOfSlicesPerChunk < minSlicesPerChunk)
    {
    vtkWarningMacro("Execute: Need "
      <<  minSlicesPerChunk*(temp/1024) << " KB to load "
      << minSlicesPerChunk << " slices.\n");
    this->NumberOfSlicesPerChunk = minSlicesPerChunk;
    }
  vtkDebugMacro("Execute: NumberOfSlicesPerChunk = "
                << this->NumberOfSlicesPerChunk);
  this->NumberOfSlicesPerChunk -= chunkOverlap;

  // Create the points, scalars, normals and Cell arrays for the output.
  // estimate the number of points from the volume dimensions
  vtkIdType estimatedSize = static_cast<vtkIdType>(extent[1] - extent[0] + 1);
  estimatedSize *= static_cast<vtkIdType>(extent[3] - extent[2] + 1);
  estimatedSize *= static_cast<vtkIdType>(extent[5] - extent[4] + 1);
  estimatedSize = static_cast<vtkIdType>(pow(1.0*estimatedSize, 0.75));
  estimatedSize = (estimatedSize / 1024) * 1024; //multiple of 1024
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
    this->Scalars = vtkFloatArray::New();
    this->Scalars->Allocate(estimatedSize,estimatedSize/2);
    }
  if (this->ComputeNormals)
    {
    this->Normals = vtkFloatArray::New();
    this->Normals->SetNumberOfComponents(3);
    this->Normals->Allocate(3*estimatedSize,3*estimatedSize/2);
    }
  if (this->ComputeGradients)
    {
    this->Gradients = vtkFloatArray::New();
    this->Gradients->SetNumberOfComponents(3);
    this->Gradients->Allocate(3*estimatedSize,3*estimatedSize/2);
    }

  // Initialize the internal point locator (edge table for oen image of cubes).
  this->InitializeLocator(extent[0], extent[1], extent[2], extent[3]);

  // Loop through the chunks running marching cubes on each one
  int zMin = extent[4];
  int zMax = extent[5];
  for(int chunkMin = zMin, chunkMax; chunkMin < zMax; chunkMin = chunkMax)
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
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                extent, 6);
    inputExec->Update();

    this->InvokeEvent(vtkCommand::StartEvent,NULL);
    this->March(inData, chunkMin, chunkMax, numContours, values);
    if ( !this->AbortExecute )
      {
      this->UpdateProgress(1.0);
      }
    this->InvokeEvent(vtkCommand::EndEvent,NULL);

    if (vtkDataObject::GetGlobalReleaseDataFlag() ||
        inInfo->Has(vtkStreamingDemandDrivenPipeline::RELEASE_DATA()))
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
    int idx = output->GetPointData()->AddArray(this->Scalars);
    output->GetPointData()->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
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

  return 1;
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
void vtkImageMarchingCubesComputePointGradient(T *ptr, double *g,
                                               int inc0, int inc1, int inc2,
                                               short b0, short b1, short b2)
{
  if (b0 < 0)
    {
    g[0] = (double)(ptr[inc0]) - (double)(*ptr);
    }
  else if (b0 > 0)
    {
    g[0] = (double)(*ptr) - (double)(ptr[-inc0]);
    }
  else
    {
    g[0] = (double)(ptr[inc0]) - (double)(ptr[-inc0]);
    }

  if (b1 < 0)
    {
    g[1] = (double)(ptr[inc1]) - (double)(*ptr);
    }
  else if (b1 > 0)
    {
    g[1] = (double)(*ptr) - (double)(ptr[-inc1]);
    }
  else
    {
    g[1] = (double)(ptr[inc1]) - (double)(ptr[-inc1]);
    }

  if (b2 < 0)
    {
    g[2] = (double)(ptr[inc2]) - (double)(*ptr);
    }
  else if (b2 > 0)
    {
    g[2] = (double)(*ptr) - (double)(ptr[-inc2]);
    }
  else
    {
    g[2] = (double)(ptr[inc2]) - (double)(ptr[-inc2]);
    }
}


//----------------------------------------------------------------------------
// This method interpolates vertices to make a new point.
template <class T>
int vtkImageMarchingCubesMakeNewPoint(vtkImageMarchingCubes *self,
                                      int idx0, int idx1, int idx2,
                                      int inc0, int inc1, int inc2,
                                      T *ptr, int edge,
                                      int *imageExtent,
                                      double *spacing, double *origin,
                                      double value)
{
  int edgeAxis = 0;
  T *ptrB = NULL;
  double temp, pt[3];

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
      pt[0] = origin[0] + spacing[0] * ((double)idx0 + temp);
      pt[1] = origin[1] + spacing[1] * ((double)idx1);
      pt[2] = origin[2] + spacing[2] * ((double)idx2);
      break;
    case 1:
      pt[0] = origin[0] + spacing[0] * ((double)idx0);
      pt[1] = origin[1] + spacing[1] * ((double)idx1 + temp);
      pt[2] = origin[2] + spacing[2] * ((double)idx2);
      break;
    case 2:
      pt[0] = origin[0] + spacing[0] * ((double)idx0);
      pt[1] = origin[1] + spacing[1] * ((double)idx1);
      pt[2] = origin[2] + spacing[2] * ((double)idx2 + temp);
      break;
    }

  // Save the scale if we are generating scalars
  if (self->ComputeScalars)
    {
    self->Scalars->InsertNextValue(value);
    }

  // Interpolate to find normal from vectors.
  if (self->NeedGradients)
    {
    short b0, b1, b2;
    double g[3], gB[3];
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
      self->Gradients->InsertNextTuple(g);
      }
    if (self->ComputeNormals)
      {
      temp = -1.0 / sqrt(g[0]*g[0] + g[1]*g[1] + g[2]*g[2]);
      g[0] *= temp;
      g[1] *= temp;
      g[2] *= temp;
      self->Normals->InsertNextTuple(g);
      }
    }

  return self->Points->InsertNextPoint(pt);
}

//----------------------------------------------------------------------------
// This method runs marching cubes on one cube.
template <class T>
void vtkImageMarchingCubesHandleCube(vtkImageMarchingCubes *self,
                                     int cellX, int cellY, int cellZ,
                                     vtkImageData *inData,
                                     T *ptr, int numContours, double *values)
{
  vtkIdType inc0, inc1, inc2;
  int valueIdx;
  double value;
  int cubeIndex, ii;
  vtkIdType pointIds[3];
  vtkMarchingCubesTriangleCases *triCase, *triCases;

  vtkInformation *inInfo = self->GetExecutive()->GetInputInformation(0, 0);

  triCases =  vtkMarchingCubesTriangleCases::GetCases();

  inData->GetIncrements(inc0, inc1, inc2);
  for (valueIdx = 0; valueIdx < numContours; ++valueIdx)
    {
    value = values[valueIdx];
    // compute the case index
    cubeIndex = 0;
    if ((double)(ptr[0]) > value)
      {
      cubeIndex += 1;
      }
    if ((double)(ptr[inc0]) > value)
      {
      cubeIndex += 2;
      }
    if ((double)(ptr[inc0 + inc1]) > value)
      {
      cubeIndex += 4;
      }
    if ((double)(ptr[inc1]) > value)
      {
      cubeIndex += 8;
      }
    if ((double)(ptr[inc2]) > value)
      {
      cubeIndex += 16;
      }
    if ((double)(ptr[inc0 + inc2]) > value)
      {
      cubeIndex += 32;
      }
    if ((double)(ptr[inc0 + inc1 + inc2]) > value)
      {
      cubeIndex += 64;
      }
    if ((double)(ptr[inc1 + inc2]) > value)
      {
      cubeIndex += 128;
      }
    // Make sure we have trianlges
    if (cubeIndex != 0 && cubeIndex != 255)
      {
      // Get edges.
      triCase = triCases + cubeIndex;
      EDGE_LIST *edge = triCase->edges;
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
            double *spacing = inData->GetSpacing();
            double *origin = inData->GetOrigin();
            int *extent =
              inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

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
void vtkImageMarchingCubesMarch(vtkImageMarchingCubes *self,
                                vtkImageData *inData, T *ptr,
                                int chunkMin, int chunkMax,
                                int numContours, double *values)
{
  int idx0, idx1, idx2;
  int min0, max0, min1, max1, min2, max2;
  vtkIdType inc0, inc1, inc2;
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
                                 int numContours, double *values)
{
  void *ptr = inData->GetScalarPointer();

  switch (inData->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageMarchingCubesMarch(this, inData, static_cast<VTK_TT*>(ptr),
                                 chunkMin, chunkMax, numContours, values)
      );
    default:
      vtkErrorMacro(<< "Unknown output ScalarType");
      return;
    }
}


//============================================================================
// These method act as the point locator so vertices will be shared.
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
  vtkIdType size = 5;
  size *= static_cast<vtkIdType>(this->LocatorDimX);
  size *= static_cast<vtkIdType>(this->LocatorDimY);
  this->LocatorPointIds = new vtkIdType[size];
  // Initialize the array
  for (vtkIdType idx = 0; idx < size; ++idx)
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
  vtkIdType *ptr = this->LocatorPointIds;
  for (int y = 0; y < this->LocatorDimY; ++y)
    {
    for (int x = 0; x < this->LocatorDimX; ++x)
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
                                            vtkIdType ptId)
{
  // Get the correct position in the array.
  vtkIdType *ptr = this->GetLocatorPointer(cellX, cellY, edge);
  *ptr = ptId;
}

//----------------------------------------------------------------------------
// This method gets a point from the locator.
vtkIdType vtkImageMarchingCubes::GetLocatorPoint(int cellX, int cellY, int edge)
{
  // Get the correct position in the array.
  vtkIdType *ptr = this->GetLocatorPointer(cellX, cellY, edge);
  return *ptr;
}

//----------------------------------------------------------------------------
// This method returns a pointer to an ID from a cube and an edge.
vtkIdType *vtkImageMarchingCubes::GetLocatorPointer(int cellX,int cellY,int edge)
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
  // must be compatible with LocatorIncrementZ.
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
    + (cellX + cellY * static_cast<vtkIdType>(this->LocatorDimX)) * 5;
}

//----------------------------------------------------------------------------
int vtkImageMarchingCubes::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkImageMarchingCubes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  this->ContourValues->PrintSelf(os,indent.GetNextIndent());

  os << indent << "ComputeScalars: " << this->ComputeScalars << "\n";
  os << indent << "ComputeNormals: " << this->ComputeNormals << "\n";
  os << indent << "ComputeGradients: " << this->ComputeGradients << "\n";

  os << indent << "InputMemoryLimit: " << this->InputMemoryLimit <<"K bytes\n";
}

