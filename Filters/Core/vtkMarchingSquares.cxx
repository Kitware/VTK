/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarchingSquares.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMarchingSquares.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCharArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkImageTransform.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkMarchingSquaresLineCases.h"
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

#include <cmath>

vtkStandardNewMacro(vtkMarchingSquares);

// Description:
// Construct object with initial scalar range (0,1) and single contour value
// of 0.0. The ImageRange are set to extract the first k-plane.
vtkMarchingSquares::vtkMarchingSquares()
{
  this->ContourValues = vtkContourValues::New();

  this->ImageRange[0] = 0;
  this->ImageRange[1] = VTK_INT_MAX;
  this->ImageRange[2] = 0;
  this->ImageRange[3] = VTK_INT_MAX;
  this->ImageRange[4] = 0;
  this->ImageRange[5] = 0;

  this->Locator = nullptr;
}

vtkMarchingSquares::~vtkMarchingSquares()
{
  this->ContourValues->Delete();
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
}

void vtkMarchingSquares::SetImageRange(int imin, int imax, int jmin, int jmax, int kmin, int kmax)
{
  int dim[6];

  dim[0] = imin;
  dim[1] = imax;
  dim[2] = jmin;
  dim[3] = jmax;
  dim[4] = kmin;
  dim[5] = kmax;

  this->SetImageRange(dim);
}

// Description:
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
vtkMTimeType vtkMarchingSquares::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType mTime2 = this->ContourValues->GetMTime();

  mTime = (mTime2 > mTime ? mTime2 : mTime);
  if (this->Locator)
  {
    mTime2 = this->Locator->GetMTime();
    mTime = (mTime2 > mTime ? mTime2 : mTime);
  }

  return mTime;
}

namespace {

//
// Contouring filter specialized for images
//
struct ContourImageWorker
{
  template <typename ScalarArrayT>
  void operator()(ScalarArrayT *inScalars,
                  vtkDataArray *newScalars,
                  int roi[6],
                  int dir[3],
                  int start[2],
                  int end[2],
                  int offset[3],
                  double *values,
                  vtkIdType numValues,
                  vtkIncrementalPointLocator *p,
                  vtkCellArray *lines)
  {
    const auto scalars = vtk::DataArrayValueRange<1>(inScalars);

    int i, j, pts[4][3], xp, yp, *x1, *x2;
    vtkIdType ptIds[2];
    double t, x[3];
    double min, max;
    int contNum, jOffset, idx, ii, jj, index, *vert;
    static const int CASE_MASK[4] = {1,2,8,4};
    vtkMarchingSquaresLineCases *lineCase, *lineCases;
    static int edges[4][2] = { {0,1}, {1,3}, {2,3}, {0,2} };
    EDGE_LIST  *edge;
    double value, s[4];

    lineCases = vtkMarchingSquaresLineCases::GetCases();
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

    //assign coordinate value to non-varying coordinate direction
    x[dir[2]] = roi[dir[2]*2];

    // Traverse all pixel cells, generating line segments using marching squares.
    for ( j=roi[start[1]]; j < roi[end[1]]; j++ )
    {

      jOffset = j * offset[1];
      pts[0][dir[1]] = j;
      yp = j+1;

      for ( i=roi[start[0]]; i < roi[end[0]]; i++)
      {
        //get scalar values
        idx = i * offset[0] + jOffset + offset[2];
        s[0] = scalars[idx];
        s[1] = scalars[idx+offset[0]];
        s[2] = scalars[idx + offset[1]];
        s[3] = scalars[idx+offset[0] + offset[1]];

        if ( (s[0] < min && s[1] < min && s[2] < min && s[3] < min) ||
             (s[0] > max && s[1] > max && s[2] > max && s[3] > max) )
        {
          continue; // no contours possible
        }

        //create pixel points
        pts[0][dir[0]] = i;
        xp = i+1;

        pts[1][dir[0]] = xp;
        pts[1][dir[1]] = pts[0][dir[1]];

        pts[2][dir[0]] = pts[0][dir[0]];
        pts[2][dir[1]] = yp;

        pts[3][dir[0]] = xp;
        pts[3][dir[1]] = yp;

        // Loop over contours in this pixel
        for (contNum=0; contNum < numValues; contNum++)
        {
          value = values[contNum];

          // Build the case table
          for ( ii=0, index = 0; ii < 4; ii++)
          {
            if ( s[ii] >= value )
            {
              index |= CASE_MASK[ii];
            }
          }
          if ( index == 0 || index == 15 )
          {
            continue; //no lines
          }

          lineCase = lineCases + index;
          edge = lineCase->edges;

          for ( ; edge[0] > -1; edge += 2 )
          {
            for (ii=0; ii<2; ii++) //insert line
            {
              vert = edges[edge[ii]];
              t = (value - s[vert[0]]) / (s[vert[1]] - s[vert[0]]);
              x1 = pts[vert[0]];
              x2 = pts[vert[1]];
              for (jj=0; jj<2; jj++) //only need to interpolate two values
              {
                x[dir[jj]] = x1[dir[jj]] + t * (x2[dir[jj]] - x1[dir[jj]]);
              }
              if ( p->InsertUniquePoint(x, ptIds[ii]) )
              {
                newScalars->InsertComponent(ptIds[ii],0,value);
              }
            }

            if ( ptIds[0] != ptIds[1] ) //check for degenerate line
            {
              lines->InsertNextCell(2,ptIds);
            }

          }//for each line
        }//for all contours
      }//for i
    }//for j
  }
};

} // end anon namespace

//
// Contouring filter specialized for images (or slices from images)
//
int vtkMarchingSquares::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkImageData *input = vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPointData *pd;
  vtkPoints *newPts;
  vtkCellArray *newLines;
  vtkDataArray *inScalars;
  vtkDataArray *newScalars = nullptr;
  int i, dims[3], roi[6], dim, plane=0;
  int *ext;
  int start[2], end[2], offset[3], dir[3], estimatedSize;
  vtkIdType numContours = this->ContourValues->GetNumberOfContours();
  double* values = this->ContourValues->GetValues();

  vtkDebugMacro(<< "Executing marching squares");
  //
  // Initialize and check input
  //
  pd = input->GetPointData();
  if (pd == nullptr)
  {
    vtkErrorMacro(<< "PointData is nullptr");
    return 1;
  }
  vtkInformationVector* inArrayVec = this->Information->Get(INPUT_ARRAYS_TO_PROCESS());
  if (inArrayVec)
  { // we have been passed an input array
    inScalars = this->GetInputArrayToProcess(0, inputVector);
  }
  else
  {
    inScalars = pd->GetScalars();
  }
  if (inScalars == nullptr)
  {
    vtkErrorMacro(<< "Scalars must be defined for contouring");
    return 1;
  }

  if (inScalars->GetNumberOfComponents() != 1)
  {
    vtkErrorMacro("Scalar array must only have a single component.");
    return 1;
  }

//
// Check dimensionality of data and get appropriate form
//
  input->GetDimensions(dims);
  ext = input->GetExtent();

  if (input->GetDataDimension() != 2)
  {
    for (i = 0; i < 6; i++)
    {
      roi[i] = this->ImageRange[i];
    }
  }
  else
  {
    input->GetExtent(roi);
  }

  // check the final region of interest to make sure its acceptable
  for (dim = 0, i = 0; i < 3; i++)
  {
    if (roi[2 * i + 1] > ext[2 * i + 1])
    {
      roi[2 * i + 1] = ext[2 * i + 1];
    }
    else if (roi[2 * i + 1] < ext[2 * i])
    {
      roi[2 * i + 1] = ext[2 * i];
    }

    if (roi[2 * i] > roi[2 * i + 1])
    {
      roi[2 * i] = roi[2 * i + 1];
    }
    else if (roi[2 * i] < ext[2 * i])
    {
      roi[2 * i] = ext[2 * i];
    }

    if ((roi[2 * i + 1] - roi[2 * i]) > 0)
    {
      dim++;
    }
    else
    {
      plane = i;
    }
  }

  if (dim != 2)
  {
    vtkErrorMacro(<< "Marching squares requires 2D data");
    return 1;
  }
  //
  // Setup indices and offsets (since can have x-y or z-plane)
  //
  if (plane == 0) // x-plane
  {
    start[0] = 2;
    end[0] = 3;
    start[1] = 4;
    end[1] = 5;
    offset[0] = dims[0];
    offset[1] = dims[0] * dims[1];
    offset[2] = (roi[0] - ext[0]);
    dir[0] = 1;
    dir[1] = 2;
    dir[2] = 0;
  }
  else if (plane == 1) // y-plane
  {
    start[0] = 0;
    end[0] = 1;
    start[1] = 4;
    end[1] = 5;
    offset[0] = 1;
    offset[1] = dims[0] * dims[1];
    offset[2] = (roi[2] - ext[2]) * dims[0];
    dir[0] = 0;
    dir[1] = 2;
    dir[2] = 1;
  }
  else // z-plane
  {
    start[0] = 0;
    end[0] = 1;
    start[1] = 2;
    end[1] = 3;
    offset[0] = 1;
    offset[1] = dims[0];
    offset[2] = (roi[4] - ext[4]) * dims[0] * dims[1];
    dir[0] = 0;
    dir[1] = 1;
    dir[2] = 2;
  }
  //
  // Allocate necessary objects
  //
  estimatedSize = (int)(numContours * sqrt((double)dims[0] * dims[1]));
  estimatedSize = estimatedSize / 1024 * 1024; // multiple of 1024
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }

  newPts = vtkPoints::New();
  newPts->Allocate(estimatedSize, estimatedSize);
  newLines = vtkCellArray::New();
  newLines->AllocateEstimate(estimatedSize, 2);

  // locator used to merge potentially duplicate points
  if (this->Locator == nullptr)
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion (newPts, input->GetBounds());

  newScalars = inScalars->NewInstance();
  newScalars->Allocate(5000, 25000);

  ContourImageWorker worker;
  using Dispatcher = vtkArrayDispatch::Dispatch;
  if (!Dispatcher::Execute(inScalars, worker, newScalars, roi, dir, start, end,
                           offset, values, numContours, this->Locator, newLines))
  { // Fallback to slow path for unknown arrays:
    worker(inScalars, newScalars, roi, dir, start, end, offset, values,
           numContours, this->Locator, newLines);
  }

  vtkDebugMacro(<< "Created: " << newPts->GetNumberOfPoints() << " points, "
                << newLines->GetNumberOfCells() << " lines");
  //
  // Update ourselves.  Because we don't know up front how many lines
  // we've created, take care to reclaim memory.
  //
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  int idx = output->GetPointData()->AddArray(newScalars);
  output->GetPointData()->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
  newScalars->Delete();

  this->Locator->Initialize();
  output->Squeeze();

  vtkImageTransform::TransformPointSet(input, output);

  return 1;
}

// Description:
// Specify a spatial locator for merging points. By default,
// an instance of vtkMergePoints is used.
void vtkMarchingSquares::SetLocator(vtkIncrementalPointLocator* locator)
{
  if (this->Locator == locator)
  {
    return;
  }

  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }

  if (locator)
  {
    locator->Register(this);
  }

  this->Locator = locator;
  this->Modified();
}

void vtkMarchingSquares::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = vtkMergePoints::New();
  }
}

int vtkMarchingSquares::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

void vtkMarchingSquares::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  this->ContourValues->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Image Range: ( " << this->ImageRange[0] << ", " << this->ImageRange[1] << ", "
     << this->ImageRange[2] << ", " << this->ImageRange[3] << ", " << this->ImageRange[4] << ", "
     << this->ImageRange[5] << " )\n";

  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }
}
