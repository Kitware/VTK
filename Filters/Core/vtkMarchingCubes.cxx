/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarchingCubes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMarchingCubes.h"

#include "vtkCellArray.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkMarchingCubesTriangleCases.h"
#include "vtkMath.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkShortArray.h"
#include "vtkStructuredPoints.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkIncrementalPointLocator.h"

vtkStandardNewMacro(vtkMarchingCubes);

// Description:
// Construct object with initial range (0,1) and single contour value
// of 0.0. ComputeNormal is on, ComputeGradients is off and ComputeScalars is on.
vtkMarchingCubes::vtkMarchingCubes()
{
  this->ContourValues = vtkContourValues::New();
  this->ComputeNormals = 1;
  this->ComputeGradients = 0;
  this->ComputeScalars = 1;
  this->Locator = NULL;
}

vtkMarchingCubes::~vtkMarchingCubes()
{
  this->ContourValues->Delete();
  if ( this->Locator )
  {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
  }
}

// Description:
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
vtkMTimeType vtkMarchingCubes::GetMTime()
{
  vtkMTimeType mTime=this->Superclass::GetMTime();
  vtkMTimeType mTime2=this->ContourValues->GetMTime();

  mTime = ( mTime2 > mTime ? mTime2 : mTime );
  if (this->Locator)
  {
    mTime2=this->Locator->GetMTime();
    mTime = ( mTime2 > mTime ? mTime2 : mTime );
  }

  return mTime;
}

// Calculate the gradient using central difference.
// NOTE: We calculate the negative of the gradient for efficiency
template <class T>
void vtkMarchingCubesComputePointGradient(int i, int j, int k, T *s, int dims[3],
                                          vtkIdType sliceSize, double spacing[3], double n[3])
{
  double sp, sm;

  // x-direction
  if ( i == 0 )
  {
    sp = s[i+1 + j*dims[0] + k*sliceSize];
    sm = s[i + j*dims[0] + k*sliceSize];
    n[0] = (sm - sp) / spacing[0];
  }
  else if ( i == (dims[0]-1) )
  {
    sp = s[i + j*dims[0] + k*sliceSize];
    sm = s[i-1 + j*dims[0] + k*sliceSize];
    n[0] = (sm - sp) / spacing[0];
  }
  else
  {
    sp = s[i+1 + j*dims[0] + k*sliceSize];
    sm = s[i-1 + j*dims[0] + k*sliceSize];
    n[0] = 0.5 * (sm - sp) / spacing[0];
  }

  // y-direction
  if ( j == 0 )
  {
    sp = s[i + (j+1)*dims[0] + k*sliceSize];
    sm = s[i + j*dims[0] + k*sliceSize];
    n[1] = (sm - sp) / spacing[1];
  }
  else if ( j == (dims[1]-1) )
  {
    sp = s[i + j*dims[0] + k*sliceSize];
    sm = s[i + (j-1)*dims[0] + k*sliceSize];
    n[1] = (sm - sp) / spacing[1];
  }
  else
  {
    sp = s[i + (j+1)*dims[0] + k*sliceSize];
    sm = s[i + (j-1)*dims[0] + k*sliceSize];
    n[1] = 0.5 * (sm - sp) / spacing[1];
  }

  // z-direction
  if ( k == 0 )
  {
    sp = s[i + j*dims[0] + (k+1)*sliceSize];
    sm = s[i + j*dims[0] + k*sliceSize];
    n[2] = (sm - sp) / spacing[2];
  }
  else if ( k == (dims[2]-1) )
  {
    sp = s[i + j*dims[0] + k*sliceSize];
    sm = s[i + j*dims[0] + (k-1)*sliceSize];
    n[2] = (sm - sp) / spacing[2];
  }
  else
  {
    sp = s[i + j*dims[0] + (k+1)*sliceSize];
    sm = s[i + j*dims[0] + (k-1)*sliceSize];
    n[2] = 0.5 * (sm - sp) / spacing[2];
  }
}

//
// Contouring filter specialized for volumes and "short int" data values.
//
template <class T>
void vtkMarchingCubesComputeGradient(vtkMarchingCubes *self,T *scalars, int dims[3],
                                     double origin[3], double spacing[3],
                                     vtkIncrementalPointLocator *locator,
                                     vtkDataArray *newScalars,
                                     vtkDataArray *newGradients,
                                     vtkDataArray *newNormals,
                                     vtkCellArray *newPolys, double *values,
                                     int numValues)
{
  double s[8], value;
  int i, j, k;
  vtkIdType sliceSize;
  static int CASE_MASK[8] = {1,2,4,8,16,32,64,128};
  vtkMarchingCubesTriangleCases *triCase, *triCases;
  EDGE_LIST  *edge;
  int contNum, jOffset, ii, index, *vert;
  vtkIdType kOffset, idx;
  vtkIdType ptIds[3];
  int ComputeNormals = newNormals != NULL;
  int ComputeGradients = newGradients != NULL;
  int ComputeScalars = newScalars != NULL;
  int NeedGradients;
  int extent[6];
  double t, *x1, *x2, x[3], *n1, *n2, n[3], min, max;
  double pts[8][3], gradients[8][3], xp, yp, zp;
  static int edges[12][2] = { {0,1}, {1,2}, {3,2}, {0,3},
                              {4,5}, {5,6}, {7,6}, {4,7},
                              {0,4}, {1,5}, {3,7}, {2,6}};

  vtkInformation *inInfo = self->GetExecutive()->GetInputInformation(0, 0);
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),extent);

  triCases =  vtkMarchingCubesTriangleCases::GetCases();

//
// Get min/max contour values
//
  if ( numValues < 1 )
  {
    return;
  }
  for ( min=max=values[0], i=1; i < numValues; i++)
  {
    if ( values[i] < min )
    {
      min = values[i];
    }
    if ( values[i] > max )
    {
      max = values[i];
    }
  }
//
// Traverse all voxel cells, generating triangles and point gradients
// using marching cubes algorithm.
//
  sliceSize = dims[0] * dims[1];
  for ( k=0; k < (dims[2]-1); k++)
  {
    self->UpdateProgress (k / static_cast<double>(dims[2] - 1));
    if (self->GetAbortExecute())
    {
      break;
    }
    kOffset = k*sliceSize;
    pts[0][2] = origin[2] + (k+extent[4]) * spacing[2];
    zp = pts[0][2] + spacing[2];
    for ( j=0; j < (dims[1]-1); j++)
    {
      jOffset = j*dims[0];
      pts[0][1] = origin[1] + (j+extent[2]) * spacing[1];
      yp = pts[0][1] + spacing[1];
      for ( i=0; i < (dims[0]-1); i++)
      {
        //get scalar values
        idx = i + jOffset + kOffset;
        s[0] = scalars[idx];
        s[1] = scalars[idx+1];
        s[2] = scalars[idx+1 + dims[0]];
        s[3] = scalars[idx + dims[0]];
        s[4] = scalars[idx + sliceSize];
        s[5] = scalars[idx+1 + sliceSize];
        s[6] = scalars[idx+1 + dims[0] + sliceSize];
        s[7] = scalars[idx + dims[0] + sliceSize];

        if ( (s[0] < min && s[1] < min && s[2] < min && s[3] < min &&
        s[4] < min && s[5] < min && s[6] < min && s[7] < min) ||
        (s[0] > max && s[1] > max && s[2] > max && s[3] > max &&
        s[4] > max && s[5] > max && s[6] > max && s[7] > max) )
        {
          continue; // no contours possible
        }

        //create voxel points
        pts[0][0] = origin[0] + (i+extent[0]) * spacing[0];
        xp = pts[0][0] + spacing[0];

        pts[1][0] = xp;
        pts[1][1] = pts[0][1];
        pts[1][2] = pts[0][2];

        pts[2][0] = xp;
        pts[2][1] = yp;
        pts[2][2] = pts[0][2];

        pts[3][0] = pts[0][0];
        pts[3][1] = yp;
        pts[3][2] = pts[0][2];

        pts[4][0] = pts[0][0];
        pts[4][1] = pts[0][1];
        pts[4][2] = zp;

        pts[5][0] = xp;
        pts[5][1] = pts[0][1];
        pts[5][2] = zp;

        pts[6][0] = xp;
        pts[6][1] = yp;
        pts[6][2] = zp;

        pts[7][0] = pts[0][0];
        pts[7][1] = yp;
        pts[7][2] = zp;

        NeedGradients = ComputeGradients || ComputeNormals;

        //create gradients if needed
        if (NeedGradients)
        {
          vtkMarchingCubesComputePointGradient(i,j,k, scalars, dims, sliceSize, spacing, gradients[0]);
          vtkMarchingCubesComputePointGradient(i+1,j,k, scalars, dims, sliceSize, spacing, gradients[1]);
          vtkMarchingCubesComputePointGradient(i+1,j+1,k, scalars, dims, sliceSize, spacing, gradients[2]);
          vtkMarchingCubesComputePointGradient(i,j+1,k, scalars, dims, sliceSize, spacing, gradients[3]);
          vtkMarchingCubesComputePointGradient(i,j,k+1, scalars, dims, sliceSize, spacing, gradients[4]);
          vtkMarchingCubesComputePointGradient(i+1,j,k+1, scalars, dims, sliceSize, spacing, gradients[5]);
          vtkMarchingCubesComputePointGradient(i+1,j+1,k+1, scalars, dims, sliceSize, spacing, gradients[6]);
          vtkMarchingCubesComputePointGradient(i,j+1,k+1, scalars, dims, sliceSize, spacing, gradients[7]);
        }
        for (contNum=0; contNum < numValues; contNum++)
        {
          value = values[contNum];
          // Build the case table
          for ( ii=0, index = 0; ii < 8; ii++)
          {
            if ( s[ii] >= value )
            {
              index |= CASE_MASK[ii];
            }
          }
          if ( index == 0 || index == 255 ) //no surface
          {
            continue;
          }

          triCase = triCases+ index;
          edge = triCase->edges;

          for ( ; edge[0] > -1; edge += 3 )
          {
            for (ii=0; ii<3; ii++) //insert triangle
            {
              vert = edges[edge[ii]];
              t = (value - s[vert[0]]) / (s[vert[1]] - s[vert[0]]);
              x1 = pts[vert[0]];
              x2 = pts[vert[1]];
              x[0] = x1[0] + t * (x2[0] - x1[0]);
              x[1] = x1[1] + t * (x2[1] - x1[1]);
              x[2] = x1[2] + t * (x2[2] - x1[2]);

              // check for a new point
              if ( locator->InsertUniquePoint(x, ptIds[ii]) )
              {
                  if (NeedGradients)
                  {
                    n1 = gradients[vert[0]];
                    n2 = gradients[vert[1]];
                    n[0] = n1[0] + t * (n2[0] - n1[0]);
                    n[1] = n1[1] + t * (n2[1] - n1[1]);
                    n[2] = n1[2] + t * (n2[2] - n1[2]);
                  }
                  if (ComputeScalars)
                  {
                    newScalars->InsertTuple(ptIds[ii],&value);
                  }
                  if (ComputeGradients)
                  {
                    newGradients->InsertTuple(ptIds[ii],n);
                  }
                  if (ComputeNormals)
                  {
                    vtkMath::Normalize(n);
                    newNormals->InsertTuple(ptIds[ii],n);
                  }
              }
            }
            // check for degenerate triangle
            if ( ptIds[0] != ptIds[1] &&
                 ptIds[0] != ptIds[2] &&
                 ptIds[1] != ptIds[2] )
            {
                newPolys->InsertNextCell(3,ptIds);
            }
          }//for each triangle
        }//for all contours
      }//for i
    }//for j
  }//for k
}

//
// Contouring filter specialized for volumes and "short int" data values.
//
int vtkMarchingCubes::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkImageData *input = vtkImageData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints *newPts;
  vtkCellArray *newPolys;
  vtkFloatArray *newScalars;
  vtkFloatArray *newNormals;
  vtkFloatArray *newGradients;
  vtkPointData *pd;
  vtkDataArray *inScalars;
  int dims[3], extent[6];
  vtkIdType estimatedSize;
  double spacing[3], origin[3];
  double bounds[6];
  int numContours=this->ContourValues->GetNumberOfContours();
  double *values=this->ContourValues->GetValues();

  vtkDebugMacro(<< "Executing marching cubes");

//
// Initialize and check input
//
  pd=input->GetPointData();
  if (pd ==NULL)
  {
    vtkErrorMacro(<<"PointData is NULL");
    return 1;
  }
  inScalars=pd->GetScalars();
  if ( inScalars == NULL )
  {
    vtkErrorMacro(<<"Scalars must be defined for contouring");
    return 1;
  }

  if ( input->GetDataDimension() != 3 )
  {
    vtkErrorMacro(<<"Cannot contour data of dimension != 3");
    return 1;
  }
  input->GetDimensions(dims);
  input->GetOrigin(origin);
  input->GetSpacing(spacing);

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);

  // estimate the number of points from the volume dimensions
  estimatedSize = static_cast<vtkIdType>(
    pow(1.0*dims[0]*dims[1]*dims[2], 0.75));
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }
  vtkDebugMacro(<< "Estimated allocation size is " << estimatedSize);
  newPts = vtkPoints::New(); newPts->Allocate(estimatedSize,estimatedSize/2);
  // compute bounds for merging points
  for ( int i=0; i<3; i++)
  {
    bounds[2*i] = origin[i] + extent[2*i] * spacing[i];
    bounds[2*i+1] = origin[i] + extent[2*i+1] * spacing[i];
  }
  if ( this->Locator == NULL )
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion (newPts, bounds, estimatedSize);

  if (this->ComputeNormals)
  {
    newNormals = vtkFloatArray::New();
    newNormals->SetNumberOfComponents(3);
    newNormals->Allocate(3*estimatedSize,3*estimatedSize/2);
  }
  else
  {
    newNormals = NULL;
  }

  if (this->ComputeGradients)
  {
    newGradients = vtkFloatArray::New();
    newGradients->SetNumberOfComponents(3);
    newGradients->Allocate(3*estimatedSize,3*estimatedSize/2);
  }
  else
  {
    newGradients = NULL;
  }

  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(estimatedSize,3));

  if (this->ComputeScalars)
  {
    newScalars = vtkFloatArray::New();
    newScalars->Allocate(estimatedSize,estimatedSize/2);
  }
  else
  {
    newScalars = NULL;
  }

  if (inScalars->GetNumberOfComponents() == 1 )
  {
    void* scalars = inScalars->GetVoidPointer(0);
    switch (inScalars->GetDataType())
    {
      vtkTemplateMacro(
        vtkMarchingCubesComputeGradient(this, static_cast<VTK_TT*>(scalars),
                                        dims,origin,spacing,this->Locator,
                                        newScalars,newGradients,
                                        newNormals,newPolys,values,
                                        numContours)
        );
    } //switch
  }

  else //multiple components - have to convert
  {
    vtkIdType dataSize = static_cast<vtkIdType>(dims[0]) * dims[1] * dims[2];
    vtkDoubleArray *image=vtkDoubleArray::New();
    image->SetNumberOfComponents(inScalars->GetNumberOfComponents());
    image->SetNumberOfTuples(image->GetNumberOfComponents()*dataSize);
    inScalars->GetTuples(0,dataSize,image);

    double *scalars = image->GetPointer(0);
    vtkMarchingCubesComputeGradient(this,scalars,dims,origin,spacing,this->Locator,
                  newScalars,newGradients,
                  newNormals,newPolys,values,numContours);
    image->Delete();
  }

  vtkDebugMacro(<<"Created: "
               << newPts->GetNumberOfPoints() << " points, "
               << newPolys->GetNumberOfCells() << " triangles");
  //
  // Update ourselves.  Because we don't know up front how many triangles
  // we've created, take care to reclaim memory.
  //
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();

  if (newScalars)
  {
    int idx = output->GetPointData()->AddArray(newScalars);
    output->GetPointData()->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    newScalars->Delete();
  }
  if (newGradients)
  {
    output->GetPointData()->SetVectors(newGradients);
    newGradients->Delete();
  }
  if (newNormals)
  {
    output->GetPointData()->SetNormals(newNormals);
    newNormals->Delete();
  }
  output->Squeeze();
  if (this->Locator)
  {
    this->Locator->Initialize(); //free storage
  }

  return 1;
}

// Description:
// Specify a spatial locator for merging points. By default,
// an instance of vtkMergePoints is used.
void vtkMarchingCubes::SetLocator(vtkIncrementalPointLocator *locator)
{
  if ( this->Locator == locator )
  {
    return;
  }

  if ( this->Locator )
  {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
  }

  if (locator)
  {
    locator->Register(this);
  }

  this->Locator = locator;
  this->Modified();
}

void vtkMarchingCubes::CreateDefaultLocator()
{
  if ( this->Locator == NULL)
  {
    this->Locator = vtkMergePoints::New();
  }
}

int vtkMarchingCubes::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

void vtkMarchingCubes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  this->ContourValues->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Compute Normals: " << (this->ComputeNormals ? "On\n" : "Off\n");
  os << indent << "Compute Gradients: " << (this->ComputeGradients ? "On\n" : "Off\n");
  os << indent << "Compute Scalars: " << (this->ComputeScalars ? "On\n" : "Off\n");

  if ( this->Locator )
  {
    os << indent << "Locator:" << this->Locator << "\n";
    this->Locator->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }
}
