/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGridSynchronizedTemplates3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSynchronizedTemplates3D.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkExtentTranslator.h"
#include "vtkFloatArray.h"
#include "vtkGridSynchronizedTemplates3D.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkShortArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkPolygonBuilder.h"
#include "vtkSmartPointer.h"
#include <math.h>

vtkStandardNewMacro(vtkGridSynchronizedTemplates3D);

//----------------------------------------------------------------------------
// Description:
// Construct object with initial scalar range (0,1) and single contour value
// of 0.0. The ImageRange are set to extract the first k-plane.
vtkGridSynchronizedTemplates3D::vtkGridSynchronizedTemplates3D()
{
  this->ContourValues = vtkContourValues::New();
  this->ComputeNormals = 1;
  this->ComputeGradients = 0;
  this->ComputeScalars = 1;
  this->GenerateTriangles = 1;

  this->ExecuteExtent[0] = this->ExecuteExtent[1]
    = this->ExecuteExtent[2] = this->ExecuteExtent[3]
    = this->ExecuteExtent[4] = this->ExecuteExtent[5] = 0;

  this->MinimumPieceSize[0] = 10;
  this->MinimumPieceSize[1] = 10;
  this->MinimumPieceSize[2] = 10;

  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
vtkGridSynchronizedTemplates3D::~vtkGridSynchronizedTemplates3D()
{
  this->ContourValues->Delete();
}

//----------------------------------------------------------------------------
void vtkGridSynchronizedTemplates3D::SetInputMemoryLimit(
  long vtkNotUsed(limit))
{
  vtkErrorMacro( << "This filter no longer supports a memory limit." );
  vtkErrorMacro( << "This filter no longer initiates streaming." );
  vtkErrorMacro( << "Please use a .... after this filter to achieve similar functionality." );
}

//----------------------------------------------------------------------------
// Description:
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
unsigned long vtkGridSynchronizedTemplates3D::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long mTime2=this->ContourValues->GetMTime();

  mTime = ( mTime2 > mTime ? mTime2 : mTime );
  return mTime;
}

//----------------------------------------------------------------------------
void vtkGridSynchronizedTemplates3DInitializeOutput(int *ext,
                                                    vtkStructuredGrid *input,
                                                    vtkPolyData *o,
                                                    vtkFloatArray *scalars,
                                                    vtkFloatArray *normals,
                                                    vtkFloatArray *gradients,
                                                    vtkDataArray *inScalars)
{
  vtkPoints *newPts;
  vtkCellArray *newPolys;
  long estimatedSize;

  estimatedSize = static_cast<int>(pow(static_cast<double>(
                 (ext[1]-ext[0]+1)*(ext[3]-ext[2]+1)*(ext[5]-ext[4]+1)), .75));
  if (estimatedSize < 1024)
    {
    estimatedSize = 1024;
    }

  newPts = vtkPoints::New();
  newPts->Allocate(estimatedSize,estimatedSize);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(estimatedSize,3));
  o->SetPoints(newPts);
  newPts->Delete();
  o->SetPolys(newPolys);
  newPolys->Delete();

  o->GetPointData()->CopyAllOn();

  // It is more efficient to just create the scalar array
  // rather than redundantly interpolate the scalars.
  if (input->GetPointData()->GetScalars() == inScalars)
    {
    o->GetPointData()->CopyScalarsOff();
    }
  else
    {
    o->GetPointData()->CopyFieldOff(inScalars->GetName());
    }

  if (normals)
    {
    normals->SetNumberOfComponents(3);
    normals->Allocate(3*estimatedSize,3*estimatedSize/2);
    normals->SetName("Normals");
    }
  if (gradients)
    {
    gradients->SetNumberOfComponents(3);
    gradients->Allocate(3*estimatedSize,3*estimatedSize/2);
    gradients->SetName("Gradients");
    }
  if (scalars)
    {
    scalars->Allocate(estimatedSize,estimatedSize/2);
    scalars->SetName("Scalars");
    }


  // It is more efficient to just create the scalar array
  o->GetPointData()->InterpolateAllocate(input->GetPointData(),
                                         estimatedSize,estimatedSize/2);
  o->GetCellData()->CopyAllocate(input->GetCellData(),
                                 estimatedSize,estimatedSize/2);
}

//----------------------------------------------------------------------------
// Close to central differences for a grid as I could get.
// Given a linear gradient assumption find gradient that minimizes
// error squared for + and - (*3) neighbors).
template <class T, class PointsType>
void ComputeGridPointGradient(int i, int j, int k, int inExt[6],
                              int incY, int incZ, T *sc, PointsType* pt,
                              double g[3])
{
  double N[6][3];
  double NtN[3][3], NtNi[3][3];
  double *NtN2[3], *NtNi2[3];
  double tmpDoubleArray[3];
  int tmpIntArray[3];
  double s[6], Nts[3], sum;
  int count = 0;
  T *s2;
  PointsType *p2;

  if (i == 2 && k == 2)
    {
    count = 0;
    }

  // x-direction
  if (i > inExt[0])
    {
    p2 = pt - 3;
    s2 = sc - 1;
    N[count][0] = p2[0] - pt[0];
    N[count][1] = p2[1] - pt[1];
    N[count][2] = p2[2] - pt[2];
    s[count] = static_cast<double>(*s2) - static_cast<double>(*sc);
    ++count;
    }
  if (i < inExt[1])
    {
    p2 = pt + 3;
    s2 = sc + 1;
    N[count][0] = p2[0] - pt[0];
    N[count][1] = p2[1] - pt[1];
    N[count][2] = p2[2] - pt[2];
    s[count] = static_cast<double>(*s2) - static_cast<double>(*sc);
    ++count;
    }

  // y-direction
  if (j > inExt[2])
    {
    p2 = pt - 3*incY;
    s2 = sc - incY;
    N[count][0] = p2[0] - pt[0];
    N[count][1] = p2[1] - pt[1];
    N[count][2] = p2[2] - pt[2];
    s[count] = static_cast<double>(*s2) - static_cast<double>(*sc);
    ++count;
    }
  if (j < inExt[3])
    {
    p2 = pt + 3*incY;
    s2 = sc + incY;
    N[count][0] = p2[0] - pt[0];
    N[count][1] = p2[1] - pt[1];
    N[count][2] = p2[2] - pt[2];
    s[count] = static_cast<double>(*s2) - static_cast<double>(*sc);
    ++count;
    }

  // z-direction
  if (k > inExt[4])
    {
    p2 = pt - 3*incZ;
    s2 = sc - incZ;
    N[count][0] = p2[0] - pt[0];
    N[count][1] = p2[1] - pt[1];
    N[count][2] = p2[2] - pt[2];
    s[count] = static_cast<double>(*s2) - static_cast<double>(*sc);
    ++count;
    }
  if (k < inExt[5])
    {
    p2 = pt + 3*incZ;
    s2 = sc + incZ;
    N[count][0] = p2[0] - pt[0];
    N[count][1] = p2[1] - pt[1];
    N[count][2] = p2[2] - pt[2];
    s[count] = static_cast<double>(*s2) - static_cast<double>(*sc);
    ++count;
    }

  // compute transpose(N)N.
  // since this will be a symetric matrix, we could make the
  // computation a little more efficient.
  for (i = 0; i < 3; ++i)
    {
    for (j = 0; j < 3; ++j)
      {
      sum = 0.0;
      for (k = 0; k < count; ++k)
        {
        sum += N[k][i] * N[k][j];
        }
      NtN[i][j] = sum;
      }
    }
  // compute the inverse of NtN
  // We have to setup a double** for the invert matrix call (@#$%!&%$!)
  NtN2[0] = &(NtN[0][0]);
  NtN2[1] = &(NtN[1][0]);
  NtN2[2] = &(NtN[2][0]);
  NtNi2[0] = &(NtNi[0][0]);
  NtNi2[1] = &(NtNi[1][0]);
  NtNi2[2] = &(NtNi[2][0]);
  if (vtkMath::InvertMatrix(NtN2, NtNi2, 3, tmpIntArray, tmpDoubleArray) == 0)
    {
    vtkGenericWarningMacro("Cannot compute gradient of grid");
    return;
    }

  // compute transpose(N)s.
  for (i = 0; i < 3; ++i)
    {
    sum = 0.0;
    for (j = 0; j < count; ++j)
      {
      sum += N[j][i] * s[j];
      }
    Nts[i] = sum;
    }

  // now compute gradient
  for (i = 0; i < 3; ++i)
    {
    sum = 0.0;
    for (j = 0; j < 3; ++j)
      {
      sum += NtNi[j][i] * Nts[j];
      }
    g[i] = sum;
    }
}

//----------------------------------------------------------------------------
#define VTK_CSP3PA(i2,j2,k2,s,p, grad, norm) \
if (NeedGradients) \
  { \
  if (!g0) \
    { \
    ComputeGridPointGradient(i, j, k, inExt, incY, incZ, s0, p0, n0); \
    g0 = 1; \
    } \
  ComputeGridPointGradient(i2, j2, k2, inExt, incY, incZ, s, p, n1); \
  for (jj=0; jj<3; jj++) \
    { \
    grad[jj] = n0[jj] + t * (n1[jj] - n0[jj]); \
    } \
  if (ComputeGradients) \
    { \
    newGradients->InsertNextTuple(grad); \
    } \
  if (ComputeNormals) \
    { \
    norm[0] = -grad[0];  norm[1] = -grad[1];  norm[2] = -grad[2]; \
    vtkMath::Normalize(norm); \
    newNormals->InsertNextTuple(norm); \
    }   \
} \
if (ComputeScalars) \
{ \
  newScalars->InsertNextTuple(&value); \
}

//----------------------------------------------------------------------------
// Contouring filter specialized for structured grids
template <class T, class PointsType>
void ContourGrid(vtkGridSynchronizedTemplates3D *self,
                 int *exExt, T *scalars,
                 vtkStructuredGrid *input, vtkPolyData *output, PointsType*, vtkDataArray *inScalars, bool outputTriangles)
{
  int *inExt = input->GetExtent();
  int xdim = exExt[1] - exExt[0] + 1;
  int ydim = exExt[3] - exExt[2] + 1;
  double n0[3], n1[3];  // used in gradient macro
  double *values = self->GetValues();
  int numContours = self->GetNumberOfContours();
  PointsType *inPtPtrX, *inPtPtrY, *inPtPtrZ;
  PointsType *p0, *p1, *p2, *p3;
  T *inPtrX, *inPtrY, *inPtrZ;
  T *s0, *s1, *s2, *s3;
  int XMin, XMax, YMin, YMax, ZMin, ZMax;
  int incY, incZ;
  PointsType* points =
    static_cast<PointsType*>(input->GetPoints()->GetData()->GetVoidPointer(0));
  double t;
  int *isect1Ptr, *isect2Ptr;
  vtkIdType ptIds[3];
  int *tablePtr;
  int v0, v1, v2, v3;
  int idx, vidx;
  double value;
  int i, j, k;
  int zstep, yisectstep;
  int offsets[12];
  int ComputeNormals = self->GetComputeNormals();
  int ComputeGradients = self->GetComputeGradients();
  int ComputeScalars = self->GetComputeScalars();
  int NeedGradients = ComputeGradients || ComputeNormals;
  int jj, g0;
  // We need to know the edgePointId's for interpolating attributes.
  vtkIdType edgePtId, inCellId, outCellId;
  vtkPointData *inPD = input->GetPointData();
  vtkCellData *inCD = input->GetCellData();
  vtkPointData *outPD = output->GetPointData();
  vtkCellData *outCD = output->GetCellData();
  // Temporary point data.
  double x[3];
  double grad[3];
  double norm[3];
  // Used to be passed in as parameteters.
  vtkCellArray *newPolys;
  vtkPoints *newPts;
  vtkFloatArray *newScalars = NULL;
  vtkFloatArray *newNormals = NULL;
  vtkFloatArray *newGradients = NULL;
  vtkPolygonBuilder polyBuilder;
  vtkSmartPointer<vtkIdList> poly = vtkSmartPointer<vtkIdList>::New();

  if (ComputeScalars)
    {
    newScalars = vtkFloatArray::New();
    }
  if (ComputeNormals)
    {
    newNormals = vtkFloatArray::New();
    }
  if (ComputeGradients)
    {
    newGradients = vtkFloatArray::New();
    }
  vtkGridSynchronizedTemplates3DInitializeOutput(exExt, input, output,
                                                 newScalars, newNormals, newGradients, inScalars);
  newPts = output->GetPoints();
  newPolys = output->GetPolys();

  // this is an exploded execute extent.
  XMin = exExt[0];
  XMax = exExt[1];
  YMin = exExt[2];
  YMax = exExt[3];
  ZMin = exExt[4];
  ZMax = exExt[5];
  // to skip over an x row of the input.
  incY = inExt[1]-inExt[0]+1;
  // to skip over an xy slice of the input.
  incZ = (inExt[3]-inExt[2]+1)*incY;

  // Kens increments, probably to do with edge array
  zstep = xdim*ydim;
  yisectstep = xdim*3;
  // compute offsets probably how to get to the edges in the edge array.
  offsets[0] = -xdim*3;
  offsets[1] = -xdim*3 + 1;
  offsets[2] = -xdim*3 + 2;
  offsets[3] = -xdim*3 + 4;
  offsets[4] = -xdim*3 + 5;
  offsets[5] = 0;
  offsets[6] = 2;
  offsets[7] = 5;
  offsets[8] = (zstep - xdim)*3;
  offsets[9] = (zstep - xdim)*3 + 1;
  offsets[10] = (zstep - xdim)*3 + 4;
  offsets[11] = zstep*3;

  // allocate storage array
  int *isect1 = new int [xdim*ydim*3*2];
  // set impossible edges to -1
  for (i = 0; i < ydim; i++)
    {
    isect1[(i+1)*xdim*3-3] = -1;
    isect1[(i+1)*xdim*3*2-3] = -1;
    }
  for (i = 0; i < xdim; i++)
    {
    isect1[((ydim-1)*xdim + i)*3 + 1] = -1;
    isect1[((ydim-1)*xdim + i)*3*2 + 1] = -1;
    }


  //fprintf(stderr, "%d: -------- Extent %d, %d, %d, %d, %d, %d\n", threadId,
  //      exExt[0], exExt[1], exExt[2], exExt[3], exExt[4], exExt[5]);

  // for each contour
  for (vidx = 0; vidx < numContours; vidx++)
    {
    value = values[vidx];
    //  skip any slices which are overlap for computing gradients.
    inPtPtrZ = points + 3*((ZMin - inExt[4]) * incZ +
                           (YMin - inExt[2]) * incY +
                           (XMin - inExt[0]));
    inPtrZ = scalars + ((ZMin - inExt[4]) * incZ +
                        (YMin - inExt[2]) * incY +
                        (XMin - inExt[0]));
    s2 = inPtrZ;
    v2 = (*s2 < value ? 0 : 1);

    //==================================================================
    for (k = ZMin; k <= ZMax; k++)
      {
      // swap the buffers
      if (k%2)
        {
        offsets[8] = (zstep - xdim)*3;
        offsets[9] = (zstep - xdim)*3 + 1;
        offsets[10] = (zstep - xdim)*3 + 4;
        offsets[11] = zstep*3;
        isect1Ptr = isect1;
        isect2Ptr = isect1 + xdim*ydim*3;
        }
      else
        {
        offsets[8] = (-zstep - xdim)*3;
        offsets[9] = (-zstep - xdim)*3 + 1;
        offsets[10] = (-zstep - xdim)*3 + 4;
        offsets[11] = -zstep*3;
        isect1Ptr = isect1 + xdim*ydim*3;
        isect2Ptr = isect1;
        }

      inPtPtrY = inPtPtrZ;
      inPtrY = inPtrZ;
      for (j = YMin; j <= YMax; j++)
        {
        // Should not impact performance here/
        edgePtId = (j-inExt[2])*incY + (k-inExt[4])*incZ;
        // Increments are different for cells.
        // Since the cells are not contoured until the second row of templates,
        // subtract 1 from i,j,and k.  Note: first cube is formed when i=0, j=1, and k=1.
        inCellId = (XMin-inExt[0]) + (inExt[1]-inExt[0])*( (j-inExt[2]-1) + (k-inExt[4]-1)*(inExt[3]-inExt[2]) );

        p1 = inPtPtrY;
        s1 = inPtrY;
        v1 = (*s1 < value ? 0 : 1);
        inPtPtrX = inPtPtrY;
        inPtrX = inPtrY;
        // inCellId is ised to keep track of ids for copying cell attributes.
        for (i = XMin; i <= XMax; i++, inCellId++)
          {
          p0 = p1;
          s0 = s1;
          v0 = v1;
          // this flag keeps up from computing gradient for grid point 0 twice.
          g0 = 0;
          *isect2Ptr = -1;
          *(isect2Ptr + 1) = -1;
          *(isect2Ptr + 2) = -1;
          if (i < XMax)
            {
            p1 = (inPtPtrX + 3);
            s1 = (inPtrX + 1);
            v1 = (*s1 < value ? 0 : 1);
            if (v0 ^ v1)
              {
              // watch for degenerate points
              if (*s0 == value)
                {
                if (i > XMin && *(isect2Ptr-3) > -1)
                  {
                  *isect2Ptr = *(isect2Ptr-3);
                  }
                else if (j > XMin && *(isect2Ptr - yisectstep + 1) > -1)
                  {
                  *isect2Ptr = *(isect2Ptr - yisectstep + 1);
                  }
                else if (k > ZMin && *(isect1Ptr+2) > -1)
                  {
                  *isect2Ptr = *(isect1Ptr+2);
                  }
                }
              else if (*s1 == value)
                {
                if (j > YMin && *(isect2Ptr - yisectstep +4) > -1)
                  {
                  *isect2Ptr = *(isect2Ptr - yisectstep + 4);
                  }
                else if (k > ZMin && i < XMax && *(isect1Ptr + 5) > -1)
                  {
                  *isect2Ptr = *(isect1Ptr + 5);
                  }
                }
              // if the edge has not been set yet then it is a new point
              if (*isect2Ptr == -1)
                {
                t = (value - static_cast<double>(*s0)) / (static_cast<double>(*s1) - static_cast<double>(*s0));
                x[0] = p0[0] + t*(p1[0] - p0[0]);
                x[1] = p0[1] + t*(p1[1] - p0[1]);
                x[2] = p0[2] + t*(p1[2] - p0[2]);
                *isect2Ptr = newPts->InsertNextPoint(x);
                VTK_CSP3PA(i+1,j,k,s1,p1,grad,norm);
                outPD->InterpolateEdge(inPD, *isect2Ptr, edgePtId, edgePtId+1, t);
                }
              }
            }
          if (j < YMax)
            {
            p2 = (inPtPtrX + incY*3);
            s2 = (inPtrX + incY);
            v2 = (*s2 < value ? 0 : 1);
            if (v0 ^ v2)
              {
              // watch for degen points
              if (*s0 == value)
                {
                if (*isect2Ptr > -1)
                  {
                  *(isect2Ptr + 1) = *isect2Ptr;
                  }
                else if (i > XMin && *(isect2Ptr-3) > -1)
                  {
                  *(isect2Ptr + 1) = *(isect2Ptr-3);
                  }
                else if (j > YMin && *(isect2Ptr - yisectstep + 1) > -1)
                  {
                  *(isect2Ptr + 1) = *(isect2Ptr - yisectstep + 1);
                  }
                else if (k > ZMin && *(isect1Ptr+2) > -1)
                  {
                  *(isect2Ptr + 1) = *(isect1Ptr+2);
                  }
                }
              else if (*s2 == value && k > ZMin && *(isect1Ptr + yisectstep + 2) > -1)
                {
                *(isect2Ptr+1) = *(isect1Ptr + yisectstep + 2);
                }
              // if the edge has not been set yet then it is a new point
              if (*(isect2Ptr + 1) == -1)
                {
                t = (value - static_cast<double>(*s0)) / (static_cast<double>(*s2) - static_cast<double>(*s0));
                x[0] = p0[0] + t*(p2[0] - p0[0]);
                x[1] = p0[1] + t*(p2[1] - p0[1]);
                x[2] = p0[2] + t*(p2[2] - p0[2]);
                *(isect2Ptr + 1) = newPts->InsertNextPoint(x);
                VTK_CSP3PA(i,j+1,k,s2,p2,grad,norm);
                outPD->InterpolateEdge(inPD, *(isect2Ptr+1), edgePtId,
                                       edgePtId+incY, t);
                }
              }
            }
          if (k < ZMax)
            {
            p3 = (inPtPtrX + incZ*3);
            s3 = (inPtrX + incZ);
            v3 = (*s3 < value ? 0 : 1);
            if (v0 ^ v3)
              {
              // watch for degen points
              if (*s0 == value)
                {
                if (*isect2Ptr > -1)
                  {
                  *(isect2Ptr + 2) = *isect2Ptr;
                  }
                else if (*(isect2Ptr+1) > -1)
                  {
                  *(isect2Ptr + 2) = *(isect2Ptr+1);
                  }
                else if (i > XMin && *(isect2Ptr-3) > -1)
                  {
                  *(isect2Ptr + 2) = *(isect2Ptr-3);
                  }
                else if (j > YMin && *(isect2Ptr - yisectstep + 1) > -1)
                  {
                  *(isect2Ptr + 2) = *(isect2Ptr - yisectstep + 1);
                  }
                else if (k > ZMin && *(isect1Ptr+2) > -1)
                  {
                  *(isect2Ptr + 2) = *(isect1Ptr+2);
                  }
                }
              if (*(isect2Ptr + 2) == -1)
                {
                t = (value - static_cast<double>(*s0)) / (static_cast<double>(*s3) - static_cast<double>(*s0));
                x[0] = p0[0] + t*(p3[0] - p0[0]);
                x[1] = p0[1] + t*(p3[1] - p0[1]);
                x[2] = p0[2] + t*(p3[2] - p0[2]);
                *(isect2Ptr + 2) = newPts->InsertNextPoint(x);
                VTK_CSP3PA(i,j,k+1,s3,p3,grad,norm);
                outPD->InterpolateEdge(inPD, *(isect2Ptr+2),
                                       edgePtId, edgePtId+incZ, t);
                }
              }
            }

          // To keep track of ids for interpolating attributes.
          ++edgePtId;

          // now add any polys that need to be added
          // basically look at the isect values,
          // form an index and lookup the polys
          if (j > YMin && i < XMax && k > ZMin)
            {
            idx = (v0 ? 4096 : 0);
            idx = idx + (*(isect1Ptr - yisectstep) > -1 ? 2048 : 0);
            idx = idx + (*(isect1Ptr -yisectstep +1) > -1 ? 1024 : 0);
            idx = idx + (*(isect1Ptr -yisectstep +2) > -1 ? 512 : 0);
            idx = idx + (*(isect1Ptr -yisectstep +4) > -1 ? 256 : 0);
            idx = idx + (*(isect1Ptr -yisectstep +5) > -1 ? 128 : 0);
            idx = idx + (*(isect1Ptr) > -1 ? 64 : 0);
            idx = idx + (*(isect1Ptr + 2) > -1 ? 32 : 0);
            idx = idx + (*(isect1Ptr + 5) > -1 ? 16 : 0);
            idx = idx + (*(isect2Ptr -yisectstep) > -1 ? 8 : 0);
            idx = idx + (*(isect2Ptr -yisectstep +1) > -1 ? 4 : 0);
            idx = idx + (*(isect2Ptr -yisectstep +4) > -1 ? 2 : 0);
            idx = idx + (*(isect2Ptr) > -1 ? 1 : 0);

            tablePtr = VTK_SYNCHRONIZED_TEMPLATES_3D_TABLE_2
              + VTK_SYNCHRONIZED_TEMPLATES_3D_TABLE_1[idx];
            // to protect data against multiple threads
            if (  input->IsCellVisible(inCellId) )
              {
              if (!outputTriangles)
                {
                polyBuilder.Reset();
                }
              while (*tablePtr != -1)
                {
                ptIds[0] = *(isect1Ptr + offsets[*tablePtr]);
                tablePtr++;
                ptIds[1] = *(isect1Ptr + offsets[*tablePtr]);
                tablePtr++;
                ptIds[2] = *(isect1Ptr + offsets[*tablePtr]);
                tablePtr++;
                if (ptIds[0] != ptIds[1] &&
                    ptIds[0] != ptIds[2] &&
                    ptIds[1] != ptIds[2])
                  {
                  if(outputTriangles)
                    {
                    outCellId = newPolys->InsertNextCell(3,ptIds);
                    outCD->CopyData(inCD, inCellId, outCellId);
                    }
                  else
                    {
                    polyBuilder.InsertTriangle(ptIds);
                    }
                  }
                }
              if(!outputTriangles)
                {
                polyBuilder.GetPolygon(poly);
                if(poly->GetNumberOfIds()>0)
                  {
                  outCellId = newPolys->InsertNextCell(poly);
                  outCD->CopyData(inCD, inCellId, outCellId);
                  }
                }
              }
            }
          inPtPtrX += 3;
          ++inPtrX;
          isect2Ptr += 3;
          isect1Ptr += 3;
          }
        inPtPtrY += 3*incY;
        inPtrY += incY;
        }
      inPtPtrZ += 3*incZ;
      inPtrZ += incZ;
      }
    }

  if (newScalars)
    {
    newScalars->SetName(inScalars->GetName());
    idx = output->GetPointData()->AddArray(newScalars);
    output->GetPointData()->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    newScalars->Delete();
    newScalars = NULL;
    }
  if (newGradients)
    {
    output->GetPointData()->SetVectors(newGradients);
    newGradients->Delete();
    newGradients = NULL;
    }
  if (newNormals)
    {
    output->GetPointData()->SetNormals(newNormals);
    newNormals->Delete();
    newNormals = NULL;
    }

  delete [] isect1;
}

template <class T>
void ContourGrid(vtkGridSynchronizedTemplates3D *self,
                 int *exExt, T *scalars, vtkStructuredGrid *input,
                 vtkPolyData *output, vtkDataArray *inScalars, bool outputTriangles)
{
  switch(input->GetPoints()->GetData()->GetDataType())
    {
    vtkTemplateMacro(
      ContourGrid(self, exExt, scalars, input, output,static_cast<VTK_TT *>(0), inScalars, outputTriangles));
    }
}

//----------------------------------------------------------------------------
// Contouring filter specialized for images (or slices from images)
void vtkGridSynchronizedTemplates3D::ThreadedExecute(int *exExt, int ,
                                                     vtkStructuredGrid *input,
                                                     vtkInformationVector **inputVector,
                                                     vtkInformation *outInfo)
{
  vtkDataArray *inScalars = this->GetInputArrayToProcess(0,inputVector);
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  long dataSize;

  vtkDebugMacro(<< "Executing 3D structured contour");

  if ( inScalars == NULL )
    {
    vtkErrorMacro(<<"Scalars must be defined for contouring");
    return;
    }

  if ( input->GetDataDimension() != 3 )
    {
    vtkErrorMacro(<<"3D structured contours requires 3D data");
    return;
    }

  //
  // Check dimensionality of data and get appropriate form
  //
  dataSize = (exExt[1]-exExt[0]+1) * (exExt[3]-exExt[2]+1)
                * (exExt[5]-exExt[4]+1);

  //
  // Check data type and execute appropriate function
  //
  if (inScalars->GetNumberOfComponents() == 1 )
    {
    void *scalars = inScalars->GetVoidPointer(0);
    switch (inScalars->GetDataType())
      {
      vtkTemplateMacro(
        ContourGrid(this, exExt, static_cast<VTK_TT *>(scalars), input, output, inScalars, this->GenerateTriangles!=0));
      }//switch
    }
  else //multiple components - have to convert
    {
    vtkDoubleArray *image = vtkDoubleArray::New();
    image->SetNumberOfComponents(inScalars->GetNumberOfComponents());
    image->Allocate(dataSize*image->GetNumberOfComponents());
    inScalars->GetTuples(0,dataSize,image);
    double *scalars = image->GetPointer(0);
    ContourGrid(this, exExt, scalars, input, output, inScalars, this->GenerateTriangles!=0);
    image->Delete();
    }

  // Some useful debugging information
  vtkDebugMacro(<<"Produced: " << output->GetNumberOfPoints() << " points, "
                << output->GetNumberOfCells() << " cells");

  // Lets set the name of the scalars here.
  if (this->ComputeScalars)
    {
    vtkDataArray *outScalars = output->GetPointData()->GetScalars();
    outScalars->SetName(inScalars->GetName());
    }
}

//----------------------------------------------------------------------------
int vtkGridSynchronizedTemplates3D::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int piece, numPieces;
  int *wholeExt;
  int ext[6];

  vtkExtentTranslator *translator = vtkExtentTranslator::SafeDownCast(
    inInfo->Get(vtkStreamingDemandDrivenPipeline::EXTENT_TRANSLATOR()));
  wholeExt = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

  // Get request from output
  piece =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  // Start with the whole grid.
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext);

  // get the extent associated with the piece.
  if (translator == NULL)
    {
    // Default behavior
    if (piece != 0)
      {
      ext[0] = ext[2] = ext[4] = 0;
      ext[1] = ext[3] = ext[5] = -1;
      }
    }
  else
    {
    translator->PieceToExtentThreadSafe(piece, numPieces, 0, wholeExt, ext,
                                        translator->GetSplitMode(),0);
    }

  // As a side product of this call, ExecuteExtent is set.
  // This is the region that we are really updating, although
  // we may require a larger input region in order to generate
  // it if normals / gradients are being computed

  this->ExecuteExtent[0] = ext[0];
  this->ExecuteExtent[1] = ext[1];
  this->ExecuteExtent[2] = ext[2];
  this->ExecuteExtent[3] = ext[3];
  this->ExecuteExtent[4] = ext[4];
  this->ExecuteExtent[5] = ext[5];

  // expand if we need to compute gradients
  if (this->ComputeGradients || this->ComputeNormals)
    {
    ext[0] -= 1;
    if (ext[0] < wholeExt[0])
      {
      ext[0] = wholeExt[0];
      }
    ext[1] += 1;
    if (ext[1] > wholeExt[1])
      {
      ext[1] = wholeExt[1];
      }

    ext[2] -= 1;
    if (ext[2] < wholeExt[2])
      {
      ext[2] = wholeExt[2];
      }
    ext[3] += 1;
    if (ext[3] > wholeExt[3])
      {
      ext[3] = wholeExt[3];
      }

    ext[4] -= 1;
    if (ext[4] < wholeExt[4])
      {
      ext[4] = wholeExt[4];
      }
    ext[5] += 1;
    if (ext[5] > wholeExt[5])
      {
      ext[5] = wholeExt[5];
      }
    }

  // Set the update extent of the input.
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext, 6);
  return 1;
}

//----------------------------------------------------------------------------
int vtkGridSynchronizedTemplates3D::FillInputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkStructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
void vtkGridSynchronizedTemplates3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  this->ContourValues->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Compute Normals: " << (this->ComputeNormals ? "On\n" : "Off\n");
  os << indent << "Compute Gradients: " << (this->ComputeGradients ? "On\n" : "Off\n");
  os << indent << "Compute Scalars: " << (this->ComputeScalars ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
int vtkGridSynchronizedTemplates3D::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkStructuredGrid *input = vtkStructuredGrid::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Make sure the attributes match the geometry.
  if (input->CheckAttributes())
    {
    return 1;
    }

  if (input->GetNumberOfPoints() == 0)
    {
    return 1;
    }

  // just call the threaded execute directly.
  this->ThreadedExecute(this->GetExecuteExtent(), 0, input, inputVector, outInfo);

  output->Squeeze();

  return 1;
}
