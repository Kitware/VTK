/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearSynchronizedTemplates.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRectilinearSynchronizedTemplates.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkShortArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredPoints.h"
#include "vtkSynchronizedTemplates3D.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkPolygonBuilder.h"
#include "vtkSmartPointer.h"

#include <math.h>

vtkStandardNewMacro(vtkRectilinearSynchronizedTemplates);

//----------------------------------------------------------------------------
// Description:
// Construct object with initial scalar range (0,1) and single contour value
// of 0.0. The ImageRange are set to extract the first k-plane.
vtkRectilinearSynchronizedTemplates::vtkRectilinearSynchronizedTemplates()
{
  this->ContourValues = vtkContourValues::New();
  this->ComputeNormals = 1;
  this->ComputeGradients = 0;
  this->ComputeScalars = 1;
  this->GenerateTriangles = 1;

  this->ArrayComponent = 0;

  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
vtkRectilinearSynchronizedTemplates::~vtkRectilinearSynchronizedTemplates()
{
  this->ContourValues->Delete();
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
unsigned long vtkRectilinearSynchronizedTemplates::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long mTime2=this->ContourValues->GetMTime();

  mTime = ( mTime2 > mTime ? mTime2 : mTime );
  return mTime;
}

//----------------------------------------------------------------------------
static void vtkRectilinearSynchronizedTemplatesInitializeOutput(
  int *ext, vtkRectilinearGrid *input, vtkPolyData *o, vtkFloatArray *scalars,
  vtkFloatArray *normals, vtkFloatArray *gradients, vtkDataArray *inScalars)
{
  vtkPoints *newPts;
  vtkCellArray *newPolys;
  long estimatedSize;

  estimatedSize = (int) pow ((double)
      ((ext[1]-ext[0]+1)*(ext[3]-ext[2]+1)*(ext[5]-ext[4]+1)), .75);
  if (estimatedSize < 1024)
    {
    estimatedSize = 1024;
    }
  newPts = vtkPoints::New();
  newPts->Allocate(estimatedSize,estimatedSize);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(estimatedSize,3));

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
    // A temporary name.
    scalars->SetName("Scalars");
    }

  o->GetPointData()->InterpolateAllocate(input->GetPointData(),
                                         estimatedSize,estimatedSize/2);
  o->GetCellData()->CopyAllocate(input->GetCellData(),
                                 estimatedSize,estimatedSize/2);

  o->SetPoints(newPts);
  newPts->Delete();

  o->SetPolys(newPolys);
  newPolys->Delete();
}

//----------------------------------------------------------------------------
// Calculate the gradient using central difference.
template <class T>
void vtkRSTComputePointGradient(int i, int j, int k, T *s, int *inExt,
                               int xInc, int yInc, int zInc,
                               double *spacing, double n[3])
{
  double sp, sm;

  // x-direction
  if ( i == inExt[0] )
    {
    sp = *(s+xInc);
    sm = *s;
    n[0] = (sp - sm) / spacing[1];
    }
  else if ( i == inExt[1] )
    {
    sp = *s;
    sm = *(s-xInc);
    n[0] = (sp - sm) / spacing[0];
    }
  else
    {
    sp = *(s+xInc);
    sm = *(s-xInc);
    n[0] = (sp - sm) / (spacing[0]+spacing[1]);
    }

  // y-direction
  if ( j == inExt[2] )
    {
    sp = *(s+yInc);
    sm = *s;
    n[1] = (sp - sm) / spacing[3];
    }
  else if ( j == inExt[3] )
    {
    sp = *s;
    sm = *(s-yInc);
    n[1] = (sp - sm) / spacing[2];
    }
  else
    {
    sp = *(s+yInc);
    sm = *(s-yInc);
    n[1] = (sp - sm) / (spacing[2]+spacing[3]);
    }

  // z-direction
  if ( k == inExt[4] )
    {
    sp = *(s+zInc);
    sm = *s;
    n[2] = (sp - sm) / spacing[5];
    }
  else if ( k == inExt[5] )
    {
    sp = *s;
    sm = *(s-zInc);
    n[2] = (sp - sm) / spacing[4];
    }
  else
    {
    sp = *(s+zInc);
    sm = *(s-zInc);
    n[2] = (sp - sm) / (spacing[4]+spacing[5]);
    }
}

//----------------------------------------------------------------------------
#define VTK_RECT_CSP3PA(i2,j2,k2,s) \
if (NeedGradients) \
{ \
  if (!g0) \
    { \
    self->ComputeSpacing(data, i, j, k, exExt, spacing); \
    vtkRSTComputePointGradient(i, j, k, s0, inExt, xInc, yInc, zInc, spacing, n0); \
    g0 = 1; \
    } \
  self->ComputeSpacing(data, i2, j2, k2, exExt, spacing); \
  vtkRSTComputePointGradient(i2, j2, k2, s, inExt, xInc, yInc, zInc, spacing, n1); \
  for (jj=0; jj<3; jj++) \
    { \
    n[jj] = n0[jj] + t * (n1[jj] - n0[jj]); \
    } \
  if (ComputeGradients) \
    { \
    newGradients->InsertNextTuple(n); \
    } \
  if (ComputeNormals) \
    { \
    vtkMath::Normalize(n); \
    n[0] = -n[0]; n[1] = -n[1]; n[2] = -n[2]; \
    newNormals->InsertNextTuple(n); \
    }   \
} \
if (ComputeScalars) \
{ \
  newScalars->InsertNextTuple(&value); \
}

//----------------------------------------------------------------------------
//
// Contouring filter specialized for images
//
template <class T>
void ContourRectilinearGrid(vtkRectilinearSynchronizedTemplates *self, int *exExt,
                            vtkRectilinearGrid *data, vtkPolyData *output, T *ptr,
                            vtkDataArray *inScalars, bool outputTriangles)
{
  int *inExt = data->GetExtent();
  int xdim = exExt[1] - exExt[0] + 1;
  int ydim = exExt[3] - exExt[2] + 1;
  double *values = self->GetValues();
  int numContours = self->GetNumberOfContours();
  T *inPtrX, *inPtrY, *inPtrZ;
  T *s0, *s1, *s2, *s3;
  int xMin, xMax, yMin, yMax, zMin, zMax;
  int xInc, yInc, zInc;
  int *isect1Ptr, *isect2Ptr;
  double y, z, t;
  int i, j, k;
  int zstep, yisectstep;
  int offsets[12];
  int ComputeNormals = self->GetComputeNormals();
  int ComputeGradients = self->GetComputeGradients();
  int ComputeScalars = self->GetComputeScalars();
  int NeedGradients = ComputeGradients || ComputeNormals;
  double n[3], n0[3], n1[3];
  int jj, g0;
  int *tablePtr;
  int idx, vidx;
  double x[3], xz[3];
  int v0, v1, v2, v3;
  vtkIdType ptIds[3];
  double value;
  // We need to know the edgePointId's for interpolating attributes.
  int edgePtId, inCellId, outCellId;
  vtkPointData *inPD = data->GetPointData();
  vtkCellData *inCD = data->GetCellData();
  vtkPointData *outPD = output->GetPointData();
  vtkCellData *outCD = output->GetCellData();
  // Use to be arguments
  vtkFloatArray *newScalars = NULL;
  vtkFloatArray *newNormals = NULL;
  vtkFloatArray *newGradients = NULL;
  vtkPoints *newPts;
  vtkCellArray *newPolys;
  ptr += self->GetArrayComponent();
  vtkDataArray *xCoords = data->GetXCoordinates();
  vtkDataArray *yCoords = data->GetYCoordinates();
  vtkDataArray *zCoords = data->GetZCoordinates();
  double x1, x2, y2, z2;
  double spacing[6];
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
  vtkRectilinearSynchronizedTemplatesInitializeOutput(exExt, data, output,
                                         newScalars, newNormals, newGradients, inScalars);
  newPts = output->GetPoints();
  newPolys = output->GetPolys();

  // this is an exploded execute extent.
  xMin = exExt[0];
  xMax = exExt[1];
  yMin = exExt[2];
  yMax = exExt[3];
  zMin = exExt[4];
  zMax = exExt[5];

  // increments to move through scalars Compute these ourself because
  // we may be contouring an array other than scalars.

  xInc = inScalars->GetNumberOfComponents();
  yInc = xInc*(inExt[1]-inExt[0]+1);
  zInc = yInc*(inExt[3]-inExt[2]+1);

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

  // for each contour
  for (vidx = 0; vidx < numContours; vidx++)
    {
    value = values[vidx];
    inPtrZ = ptr;
    s2 = inPtrZ;

    //==================================================================
    for (k = zMin; k <= zMax; k++)
      {
      self->UpdateProgress((double)vidx/numContours +
                           (k-zMin)/((zMax - zMin+1.0)*numContours));

      z = zCoords->GetComponent(k-inExt[4], 0);
      x[2] = z;

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

      inPtrY = inPtrZ;
      for (j = yMin; j <= yMax; j++)
        {
        // Should not impact performance here/
        edgePtId = (j-inExt[2])*yInc + (k-inExt[4])*zInc;
        // Increments are different for cells.
        // Since the cells are not contoured until the second row of templates,
        // subtract 1 from i,j,and k.  Note: first cube is formed when i=0, j=1, and k=1.
        inCellId = (xMin-inExt[0]) + (inExt[1]-inExt[0])*( (j-inExt[2]-1) + (k-inExt[4]-1)*(inExt[3]-inExt[2]) );

        y = yCoords->GetComponent(j-inExt[2], 0);
        xz[1] = y;

        s1 = inPtrY;
        v1 = (*s1 < value ? 0 : 1);

        inPtrX = inPtrY;
        for (i = xMin; i <= xMax; i++)
          {
          s0 = s1;
          v0 = v1;
          // this flag keeps up from computing gradient for grid point 0 twice.
          g0 = 0;
          *isect2Ptr = -1;
          *(isect2Ptr + 1) = -1;
          *(isect2Ptr + 2) = -1;
          if (i < xMax)
            {
            s1 = (inPtrX + xInc);
            v1 = (*s1 < value ? 0 : 1);
            if (v0 ^ v1)
              {
              // watch for degenerate points
              if (*s0 == value)
                {
                if (i > xMin && *(isect2Ptr-3) > -1)
                  {
                  *isect2Ptr = *(isect2Ptr-3);
                  }
                else if (j > yMin && *(isect2Ptr - yisectstep + 1) > -1)
                  {
                  *isect2Ptr = *(isect2Ptr - yisectstep + 1);
                  }
                else if (k > zMin && *(isect1Ptr+2) > -1)
                  {
                  *isect2Ptr = *(isect1Ptr+2);
                  }
                }
              else if (*s1 == value)
                {
                if (j > yMin && *(isect2Ptr - yisectstep +4) > -1)
                  {
                  *isect2Ptr = *(isect2Ptr - yisectstep + 4);
                  }
                else if (k > zMin && i < xMax && *(isect1Ptr + 5) > -1)
                  {
                  *isect2Ptr = *(isect1Ptr + 5);
                  }
                }
              // if the edge has not been set yet then it is a new point
              if (*isect2Ptr == -1)
                {
                t = (value - (double)(*s0)) / ((double)(*s1) - (double)(*s0));
                x1 = xCoords->GetComponent(i-inExt[0], 0);
                x2 = xCoords->GetComponent(i-inExt[0]+1, 0);
                x[0] = x1 + t*(x2-x1);
                x[1] = y;

                *isect2Ptr = newPts->InsertNextPoint(x);
                VTK_RECT_CSP3PA(i+1,j,k,s1);
                outPD->InterpolateEdge(inPD, *isect2Ptr, edgePtId, edgePtId+1, t);
                }
              }
            }
          if (j < yMax)
            {
            s2 = (inPtrX + yInc);
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
                else if (i > xMin && *(isect2Ptr-3) > -1)
                  {
                  *(isect2Ptr + 1) = *(isect2Ptr-3);
                  }
                else if (j > yMin && *(isect2Ptr - yisectstep + 1) > -1)
                  {
                  *(isect2Ptr + 1) = *(isect2Ptr - yisectstep + 1);
                  }
                else if (k > zMin && *(isect1Ptr+2) > -1)
                  {
                  *(isect2Ptr + 1) = *(isect1Ptr+2);
                  }
                }
              else if (*s2 == value && k > zMin && *(isect1Ptr + yisectstep + 2) > -1)
                {
                *(isect2Ptr+1) = *(isect1Ptr + yisectstep + 2);
                }
              // if the edge has not been set yet then it is a new point
              if (*(isect2Ptr + 1) == -1)
                {
                t = (value - (double)(*s0)) / ((double)(*s2) - (double)(*s0));
                x[0] = xCoords->GetComponent(i-inExt[0], 0);

                y2 = yCoords->GetComponent(j-inExt[2]+1, 0);
                x[1] = y + t*(y2-y);

                *(isect2Ptr + 1) = newPts->InsertNextPoint(x);
                VTK_RECT_CSP3PA(i,j+1,k,s2);
                outPD->InterpolateEdge(inPD, *(isect2Ptr+1), edgePtId, edgePtId+yInc, t);
                }
              }
            }
          if (k < zMax)
            {
            s3 = (inPtrX + zInc);
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
                else if (i > xMin && *(isect2Ptr-3) > -1)
                  {
                  *(isect2Ptr + 2) = *(isect2Ptr-3);
                  }
                else if (j > yMin && *(isect2Ptr - yisectstep + 1) > -1)
                  {
                  *(isect2Ptr + 2) = *(isect2Ptr - yisectstep + 1);
                  }
                else if (k > zMin && *(isect1Ptr+2) > -1)
                  {
                  *(isect2Ptr + 2) = *(isect1Ptr+2);
                  }
                }
              if (*(isect2Ptr + 2) == -1)
                {
                t = (value - (double)(*s0)) / ((double)(*s3) - (double)(*s0));
                xz[0] = xCoords->GetComponent(i-inExt[0], 0);

                z2 = zCoords->GetComponent(k-inExt[4]+1, 0);
                xz[2] = z + t*(z2-z);

                *(isect2Ptr + 2) = newPts->InsertNextPoint(xz);
                VTK_RECT_CSP3PA(i,j,k+1,s3);
                outPD->InterpolateEdge(inPD, *(isect2Ptr+2), edgePtId, edgePtId+zInc, t);
                }
              }
            }
          // To keep track of ids for interpolating attributes.
          ++edgePtId;

          // now add any polys that need to be added
          // basically look at the isect values,
          // form an index and lookup the polys
          if (j > yMin && i < xMax && k > zMin)
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
          inPtrX += xInc;
          isect2Ptr += 3;
          isect1Ptr += 3;
          // To keep track of ids for copying cell attributes..
          ++inCellId;
          }
        inPtrY += yInc;
        }
      inPtrZ += zInc;
      }
    }
  delete [] isect1;

  if (newScalars)
    {
    // Lets set the name of the scalars here.
    if (inScalars)
      {
      newScalars->SetName(inScalars->GetName());
      }
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
}

//----------------------------------------------------------------------------
//
// Contouring filter specialized for images (or slices from images)
//
int vtkRectilinearSynchronizedTemplates::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkRectilinearGrid *data = vtkRectilinearGrid::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  void *ptr;
  vtkDataArray *inScalars;

  vtkDebugMacro(<< "Executing 3D structured contour");

  //
  // Check data type and execute appropriate function
  //
  inScalars = this->GetInputArrayToProcess(0,inputVector);
  if (inScalars == NULL)
    {
    vtkErrorMacro("No scalars for contouring.");
    return 1;
    }
  int numComps = inScalars->GetNumberOfComponents();

  if (this->ArrayComponent >= numComps)
    {
    vtkErrorMacro("Scalars have " << numComps << " components. "
                  "ArrayComponent must be smaller than " << numComps);
    return 1;
    }

  int* inExt = data->GetExtent();
  ptr = this->GetScalarsForExtent(inScalars, inExt, data);

  int exExt[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), exExt);
  for (int i=0; i<3; i++)
    {
    if (inExt[2*i] > exExt[2*i])
      {
      exExt[2*i] = inExt[2*i];
      }
    if (inExt[2*i+1] < exExt[2*i+1])
      {
      exExt[2*i+1] = inExt[2*i+1];
      }
    }

  switch (inScalars->GetDataType())
    {
    vtkTemplateMacro(
      ContourRectilinearGrid(this, exExt, data,
                             output, (VTK_TT *)ptr, inScalars,this->GenerateTriangles!=0));
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkRectilinearSynchronizedTemplates::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // These require extra ghost levels
  if (this->ComputeGradients || this->ComputeNormals)
    {
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    vtkInformation *outInfo = outputVector->GetInformationObject(0);

    int ghostLevels;
    ghostLevels =
      outInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                ghostLevels + 1);
    }

  return 1;
}

//----------------------------------------------------------------------------
void* vtkRectilinearSynchronizedTemplates::GetScalarsForExtent(
  vtkDataArray *array, int extent[6], vtkRectilinearGrid *input)
{
  if ( ! array )
    {
    return NULL;
    }

  int increments[3], iExt[6], idx;

  input->GetExtent(iExt);

  for (idx = 0; idx < 3; idx++)
    {
    if (extent[idx*2] < iExt[idx*2] ||
        extent[idx*2] > iExt[idx*2+1])
      {
      vtkErrorMacro("requested extent not in input's extent");
      return NULL;
      }
    }

  increments[0] = array->GetNumberOfComponents();
  increments[1] = increments[0] * (iExt[1]-iExt[0]+1);
  increments[2] = increments[1] * (iExt[3]-iExt[2]+1);

  idx = (extent[0] - iExt[0]) * increments[0] +
    (extent[2] - iExt[2]) * increments[1] +
    (extent[4] - iExt[4]) * increments[2];

  if (idx < 0 || idx > array->GetMaxId())
    {
    vtkErrorMacro("computed coordinate outside of array bounds");
    return NULL;
    }

  return array->GetVoidPointer(idx);
}

//----------------------------------------------------------------------------
void vtkRectilinearSynchronizedTemplates::ComputeSpacing(
  vtkRectilinearGrid *data, int i, int j, int k, int extent[6],
  double spacing[6])
{
  vtkDataArray *xCoords = data->GetXCoordinates();
  vtkDataArray *yCoords = data->GetYCoordinates();
  vtkDataArray *zCoords = data->GetZCoordinates();

  spacing[0] = 0;
  spacing[1] = 0;
  spacing[2] = 0;
  spacing[3] = 0;
  spacing[4] = 0;
  spacing[5] = 0;

  if (i > extent[0])
    {
    spacing[0] = xCoords->GetComponent(i-extent[0], 0) -
      xCoords->GetComponent(i-extent[0]-1, 0);
    }
  if (i < extent[1])
    {
    spacing[1] = xCoords->GetComponent(i-extent[0]+1, 0) -
      xCoords->GetComponent(i-extent[0], 0);
    }
  if (j > extent[2])
    {
    spacing[2] = yCoords->GetComponent(j-extent[2], 0) -
      yCoords->GetComponent(j-extent[2]-1, 0);
    }
  if (j < extent[3])
    {
    spacing[3] = yCoords->GetComponent(j-extent[2]+1, 0) -
      yCoords->GetComponent(j-extent[2], 0);
    }
  if (k > extent[4])
    {
    spacing[4] = zCoords->GetComponent(k-extent[4], 0) -
      zCoords->GetComponent(k-extent[4]-1, 0);
    }
  if (k < extent[5])
    {
    spacing[5] = zCoords->GetComponent(k-extent[4]+1, 0) -
      zCoords->GetComponent(k-extent[4], 0);
    }
}

//----------------------------------------------------------------------------
int vtkRectilinearSynchronizedTemplates::FillInputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  return 1;
}

//----------------------------------------------------------------------------
void vtkRectilinearSynchronizedTemplates::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  this->ContourValues->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Compute Normals: " << (this->ComputeNormals ? "On\n" : "Off\n");
  os << indent << "Compute Gradients: " << (this->ComputeGradients ? "On\n" : "Off\n");
  os << indent << "Compute Scalars: " << (this->ComputeScalars ? "On\n" : "Off\n");
  os << indent << "ArrayComponent: " << this->ArrayComponent << endl;
}

