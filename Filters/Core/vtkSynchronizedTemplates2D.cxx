/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSynchronizedTemplates2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSynchronizedTemplates2D.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkShortArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"

#include <cmath>

vtkStandardNewMacro(vtkSynchronizedTemplates2D);

//----------------------------------------------------------------------------
// Description:
// Construct object with initial scalar range (0,1) and single contour value
// of 0.0. The ImageRange are set to extract the first k-plane.
vtkSynchronizedTemplates2D::vtkSynchronizedTemplates2D()
{
  this->ContourValues = vtkContourValues::New();
  this->ComputeScalars = 1;
  this->ArrayComponent = 0;

  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);
}

vtkSynchronizedTemplates2D::~vtkSynchronizedTemplates2D()
{
  this->ContourValues->Delete();
}

//----------------------------------------------------------------------------
// Description:
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
vtkMTimeType vtkSynchronizedTemplates2D::GetMTime()
{
  vtkMTimeType mTime=this->Superclass::GetMTime();
  vtkMTimeType mTime2=this->ContourValues->GetMTime();

  mTime = ( mTime2 > mTime ? mTime2 : mTime );
  return mTime;
}

//----------------------------------------------------------------------------
//
// Contouring filter specialized for images
//
template <class T>
void vtkContourImage(vtkSynchronizedTemplates2D *self,
                     T *scalars, vtkPoints *newPts,
                     vtkDataArray *newScalars, vtkCellArray *lines,
                     vtkImageData *input, int *updateExt)
{
  double *values = self->GetValues();
  int numContours = self->GetNumberOfContours();
  T *inPtr, *rowPtr;
  double x[3];
  double *origin = input->GetOrigin();
  double *spacing = input->GetSpacing();
  double y, t;
  int *isect1Ptr, *isect2Ptr;
  vtkIdType ptIds[2];
  int *tablePtr;
  int v0, v1 = 0, v2;
  int idx, vidx;
  double s0, s1, s2, value;
  int i, j;
  int lineCases[64];

  // The update extent may be different than the extent of the image.
  // The only problem with using the update extent is that one or two
  // sources enlarge the update extent.  This behavior is slated to be
  // eliminated.
  vtkIdType *incs = input->GetIncrements();
  int *ext = input->GetExtent();
  int axis0, axis1;
  int min0, max0, dim0;
  int min1, max1;
  int inc0, inc1;

  // Figure out which plane the image lies in.
  if (updateExt[4] == updateExt[5])
  { // z collapsed
    axis0 = 0;
    min0 = updateExt[0];
    max0 = updateExt[1];
    inc0 = incs[0];
    axis1 = 1;
    min1 = updateExt[2];
    max1 = updateExt[3];
    inc1 = incs[1];
    x[2] = origin[2] + (updateExt[4]*spacing[2]);
  }
  else if (updateExt[2] == updateExt[3])
  { // y collapsed
    axis0 = 0;
    min0 = updateExt[0];
    max0 = updateExt[1];
    inc0 = incs[0];
    axis1 = 2;
    min1 = updateExt[4];
    max1 = updateExt[5];
    inc1 = incs[2];
    x[1] = origin[1] + (updateExt[2]*spacing[1]);
  }
  else if (updateExt[0] == updateExt[1])
  { // x collapsed
    axis0 = 1;
    min0 = updateExt[2];
    max0 = updateExt[3];
    inc0 = incs[1];
    axis1 = 2;
    min1 = updateExt[4];
    max1 = updateExt[5];
    inc1 = incs[2];
    x[0] = origin[0] + (updateExt[0]*spacing[0]);
  }
  else
  {
    vtkGenericWarningMacro("Expecting 2D data.");
    return;
  }
  dim0 = max0-min0+1;


  // setup the table entries
  for (i = 0; i < 64; i++)
  {
    lineCases[i] = -1;
  }

  lineCases[12] = 3;
  lineCases[13] = dim0*2;

  lineCases[20] = 1;
  lineCases[21] = dim0*2;

  lineCases[24] = 1;
  lineCases[25] = 3;

  lineCases[36] = 0;
  lineCases[37] = dim0*2;

  lineCases[40] = 0;
  lineCases[41] = 3;

  lineCases[48] = 0;
  lineCases[49] = 1;

  lineCases[60] = 0;
  lineCases[61] = 1;
  lineCases[62] = 3;
  lineCases[63] = dim0*2;

  // allocate storage arrays
  int *isect1 = new int [dim0*4];
  isect1[dim0*2-2] = -1;
  isect1[dim0*2-1] = -1;
  isect1[dim0*4-2] = -1;
  isect1[dim0*4-1] = -1;


  // Compute the staring location.  We may be operating
  // on a part of the image.
  scalars += incs[0]*(updateExt[0]-ext[0])
    + incs[1]*(updateExt[2]-ext[2])
    + incs[2]*(updateExt[4]-ext[4]) + self->GetArrayComponent();

  // for each contour
  for (vidx = 0; vidx < numContours; vidx++)
  {
    rowPtr = scalars;

    lineCases[13] = dim0*2;
    lineCases[21] = dim0*2;
    lineCases[37] = dim0*2;
    lineCases[63] = dim0*2;

    value = values[vidx];

    // Traverse all pixel cells, generating line segements using templates
    for (j = min1; j <= max1; j++)
    {
      inPtr = rowPtr;
      rowPtr += inc1;

      // set the y coordinate
      y = origin[axis1] + j*spacing[axis1];
      // first compute the intersections
      s1 = *inPtr;

      // swap the buffers
      if (j%2)
      {
        lineCases[13] = dim0*2;
        lineCases[21] = dim0*2;
        lineCases[37] = dim0*2;
        lineCases[63] = dim0*2;
        isect1Ptr = isect1;
        isect2Ptr = isect1 + dim0*2;
      }
      else
      {
        lineCases[13] = -dim0*2;
        lineCases[21] = -dim0*2;
        lineCases[37] = -dim0*2;
        lineCases[63] = -dim0*2;
        isect1Ptr = isect1 + dim0*2;
        isect2Ptr = isect1;
      }

      for (i = min0; i < max0; i++)
      {
        s0 = s1;
        s1 = *(inPtr + inc0);
        // compute in/out for verts
        v0 = (s0 < value ? 0 : 1);
        v1 = (s1 < value ? 0 : 1);
        // if we have an intersection
        *isect2Ptr = -1;
        *(isect2Ptr + 1) = -1;
        if (v0 ^ v1)
        {
          // watch for degenerate points
          if (s0 == value)
          {
            if (i > min0 && *(isect2Ptr-2) > -1)
            {
              *isect2Ptr = *(isect2Ptr-2);
            }
            else if (j > min1 && *(isect1Ptr+1) > -1)
            {
              *isect2Ptr = *(isect1Ptr+1);
            }
          }
          else if (s1 == value && j > min1 && *(isect1Ptr+3) > -1)
          {
            *isect2Ptr = *(isect1Ptr+3);
          }
          // if the edge has not been set yet then it is a new point
          if (*isect2Ptr == -1)
          {
            t = (value - s0) / (s1 - s0);
            x[axis0] = origin[axis0] + spacing[axis0]*(i+t);
            x[axis1] = y;
            *isect2Ptr = newPts->InsertNextPoint(x);
            if (newScalars)
            {
              newScalars->InsertNextTuple(&value);
            }
          }
        }
        if (j < max1)
        {
          s2 = *(inPtr + inc1);
          v2 = (s2 < value ? 0 : 1);
          if (v0 ^ v2)
          {
            if (s0 == value)
            {
              if (*isect2Ptr > -1)
              {
                *(isect2Ptr + 1) = *isect2Ptr;
              }
              else if (j > min1 && *(isect1Ptr+1) > -1)
              {
                *(isect2Ptr + 1) = *(isect1Ptr+1);
              }
              else if (i > min0 && *(isect2Ptr-2) > -1)
              {
                *(isect2Ptr + 1) = *(isect2Ptr-2);
              }
            }
            // if the edge has not been set yet then it is a new point
            if (*(isect2Ptr + 1) == -1)
            {
              t = (value - s0) / (s2 - s0);
              x[axis0] = origin[axis0] + spacing[axis0]*i;
              x[axis1] = y + spacing[axis1]*t;
              *(isect2Ptr + 1) = newPts->InsertNextPoint(x);
              if (newScalars)
              {
                newScalars->InsertNextTuple(&value);
              }
            }
          }
        }

        if (j > min1)
        {
          // now add any lines that need to be added
          // basically look at the isect values,
          // form an index and lookup the lines
          idx = (*isect1Ptr > -1 ? 8 : 0);
          idx = idx + (*(isect1Ptr +1) > -1 ? 4 : 0);
          idx = idx + (*(isect1Ptr +3) > -1 ? 2 : 0);
          idx = idx + (*isect2Ptr > -1 ? 1 : 0);
          tablePtr = lineCases + idx*4;

          if (*tablePtr != -1)
          {
            ptIds[0] = *(isect1Ptr + *tablePtr);
            tablePtr++;
            ptIds[1] = *(isect1Ptr + *tablePtr);
            if (ptIds[0] != ptIds[1])
            {
              // insert non-degenerate lines
              lines->InsertNextCell(2,ptIds);
            }
            tablePtr++;
            if (*tablePtr != -1)
            {
              ptIds[0] = *(isect1Ptr + *tablePtr);
              tablePtr++;
              ptIds[1] = *(isect1Ptr + *tablePtr);
              if (ptIds[0] != ptIds[1])
              {
                lines->InsertNextCell(2,ptIds);
              }
            }
          }
        }
        inPtr += inc0;
        isect2Ptr += 2;
        isect1Ptr += 2;
      }
      // now compute the last column, use s2 since it is around
      if (j < max1)
      {
        s2 = *(inPtr + dim0);
        v2 = (s2 < value ? 0 : 1);
        *(isect2Ptr + 1) = -1;
        if (v1 ^ v2)
        {
          if (s1 == value && *(isect2Ptr-2) > -1)
          {
            *(isect2Ptr + 1) = *(isect2Ptr-2);
          }
          else if (s1 == value && *(isect1Ptr+1) > -1)
          {
            *(isect2Ptr + 1) = *(isect1Ptr+1);
          }
          else
          {
            t = (value - s1) / (s2 - s1);
            x[axis0] = origin[axis0] + spacing[axis0]*max0;
            x[axis1] = y + spacing[axis1]*t;
            *(isect2Ptr + 1) = newPts->InsertNextPoint(x);
            if (newScalars)
            {
              newScalars->InsertNextTuple(&value);
            }
          }
        }
      }
    }
  }

  delete [] isect1;
}

//----------------------------------------------------------------------------
//
// Contouring filter specialized for images (or slices from images)
//
int vtkSynchronizedTemplates2D::RequestData(
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

  vtkPoints     *newPts;
  vtkCellArray  *newLines;
  vtkDataArray  *inScalars;
  vtkDataArray  *newScalars = NULL;
  int           *ext;
  int           dims[3];
  int           dataSize, estimatedSize;


  vtkDebugMacro(<< "Executing 2D structured contour");

  ext =
    inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  inScalars = this->GetInputArrayToProcess(0,inputVector);
  if ( inScalars == NULL )
  {
    vtkErrorMacro(<<"Scalars must be defined for contouring");
    return 1;
  }

  int numComps = inScalars->GetNumberOfComponents();
  if (this->ArrayComponent >= numComps)
  {
    vtkErrorMacro("Scalars have " << numComps << " components. "
                  "ArrayComponent must be smaller than " << numComps);
    return 1;
  }

  // We have to compute the dimenisons from the update extent because
  // the extent may be larger.
  dims[0] = ext[1]-ext[0]+1;
  dims[1] = ext[3]-ext[2]+1;
  dims[2] = ext[5]-ext[4]+1;

  //
  // Check dimensionality of data and get appropriate form
  //
  dataSize = dims[0] * dims[1] * dims[2];

  //
  // Allocate necessary objects
  //
  estimatedSize = (int) (sqrt((double)(dataSize)));
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }
  newPts = vtkPoints::New();
  newPts->Allocate(estimatedSize,estimatedSize);
  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(estimatedSize,2));

  //
  // Check data type and execute appropriate function
  //

  void *scalars = inScalars->GetVoidPointer(0);
  if (this->ComputeScalars)
  {
    newScalars = inScalars->NewInstance();
    newScalars->SetNumberOfComponents(inScalars->GetNumberOfComponents());
    newScalars->SetName(inScalars->GetName());
    newScalars->Allocate(5000,25000);
  }
  switch (inScalars->GetDataType())
  {
    vtkTemplateMacro(
      vtkContourImage(this,(VTK_TT *)scalars, newPts,
                      newScalars, newLines, input, ext));
  }//switch

  // Lets set the name of the scalars here.
  if (newScalars)
  {
    newScalars->SetName(inScalars->GetName());
  }

  vtkDebugMacro(<<"Created: "
               << newPts->GetNumberOfPoints() << " points, "
               << newLines->GetNumberOfCells() << " lines");
  //
  // Update ourselves.  Because we don't know up front how many lines
  // we've created, take care to reclaim memory.
  //
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  if (newScalars)
  {
    int idx = output->GetPointData()->AddArray(newScalars);
    output->GetPointData()->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    newScalars->Delete();
  }

  output->Squeeze();
  return 1;
}

//----------------------------------------------------------------------------
int vtkSynchronizedTemplates2D::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkSynchronizedTemplates2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  this->ContourValues->PrintSelf(os,indent.GetNextIndent());
  if (this->ComputeScalars)
  {
    os << indent << "ComputeScalarsOn\n";
  }
  else
  {
    os << indent << "ComputeScalarsOff\n";
  }
  os << indent << "ArrayComponent: " << this->ArrayComponent << endl;
}
