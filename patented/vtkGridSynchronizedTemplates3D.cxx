/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGridSynchronizedTemplates3D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$



Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

    THIS CLASS IS PATENT PENDING.

    Application of this software for commercial purposes requires 
    a license grant from Kitware. Contact:
        Ken Martin
        Kitware
        469 Clifton Corporate Parkway,
        Clifton Park, NY 12065
        Phone:1-518-371-3971 
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
#include "vtkStructuredPoints.h"
#include "vtkCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkShortArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkLongArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkSynchronizedTemplates3D.h"
#include "vtkGridSynchronizedTemplates3D.h"
#include "vtkMath.h"



// Description:
// Construct object with initial scalar range (0,1) and single contour value
// of 0.0. The ImageRange are set to extract the first k-plane.
vtkGridSynchronizedTemplates3D::vtkGridSynchronizedTemplates3D()
{
  this->ContourValues = vtkContourValues::New();
  this->ComputeNormals = 1;
  this->ComputeGradients = 0;
  this->ComputeScalars = 1;
}

vtkGridSynchronizedTemplates3D::~vtkGridSynchronizedTemplates3D()
{
  this->ContourValues->Delete();
}

// Description:
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
unsigned long vtkGridSynchronizedTemplates3D::GetMTime()
{
  unsigned long mTime=this->vtkStructuredGridToPolyDataFilter::GetMTime();
  unsigned long mTime2=this->ContourValues->GetMTime();

  mTime = ( mTime2 > mTime ? mTime2 : mTime );
  return mTime;
}

// Close to central differences for a grid as I could get.
// Given a linear gradient assumption find gradient that minimizes
// error squared for + and - (*3) neighbors).
template <class T>
static void ComputeGridPointGradient(int i, int j, int k, int dims[3], 
                                int sliceSize, T *sc, float *pt, float g[3])
{
  float N[6][3];
  double NtN[3][3], NtNi[3][3];
  double *NtN2[3], *NtNi2[3];
  float s[6], Nts[3], sum;
  int count = 0;
  T *s2;
  float *p2;

  if (i == 2 && k == 2)
    {
    count = 0;
    }

  // x-direction
  if (i > 0)
    {
    p2 = pt - 3;
    s2 = sc - 1;
    N[count][0] = p2[0] - pt[0];
    N[count][1] = p2[1] - pt[1];
    N[count][2] = p2[2] - pt[2];
    s[count] = (float)(*s2) - (float)(*sc);
    ++count;
    }
  if (i < (dims[0]-1))
    {
    p2 = pt + 3;
    s2 = sc + 1;
    N[count][0] = p2[0] - pt[0];
    N[count][1] = p2[1] - pt[1];
    N[count][2] = p2[2] - pt[2];
    s[count] = (float)(*s2) - (float)(*sc);
    ++count;
    }

  // y-direction
  if (j > 0)
    {
    p2 = pt - 3*dims[0];
    s2 = sc - dims[0];
    N[count][0] = p2[0] - pt[0];
    N[count][1] = p2[1] - pt[1];
    N[count][2] = p2[2] - pt[2];
    s[count] = (float)(*s2) - (float)(*sc);
    ++count;
    }
  if (j < (dims[1]-1))
    {
    p2 = pt + 3*dims[0];
    s2 = sc + dims[0];
    N[count][0] = p2[0] - pt[0];
    N[count][1] = p2[1] - pt[1];
    N[count][2] = p2[2] - pt[2];
    s[count] = (float)(*s2) - (float)(*sc);
    ++count;
    }

  // z-direction
  if (k > 0)
    {
    p2 = pt - 3*sliceSize;
    s2 = sc - sliceSize;
    N[count][0] = p2[0] - pt[0];
    N[count][1] = p2[1] - pt[1];
    N[count][2] = p2[2] - pt[2];
    s[count] = (float)(*s2) - (float)(*sc);
    ++count;
    }
  if (k < (dims[2]-1))
    {
    p2 = pt + 3*sliceSize;
    s2 = sc + sliceSize;
    N[count][0] = p2[0] - pt[0];
    N[count][1] = p2[1] - pt[1];
    N[count][2] = p2[2] - pt[2];
    s[count] = (float)(*s2) - (float)(*sc);
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
  if (vtkMath::InvertMatrix(NtN2, NtNi2, 3) == 0)
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
      sum += NtNi[i][j] * Nts[j];
      }
    g[i] = sum;
    }
} 


   



#define VTK_CSP3PA(i2,j2,k2,s,p) \
if (NeedGradients) \
  { \
  if (!g0) \
    { \
    ComputeGridPointGradient(i, j, k, dims, zstep, \
		       s0, p0, n0); \
    g0 = 1; \
    } \
  ComputeGridPointGradient(i2, j2, k2, dims, zstep, \
		       s, p, n1); \
  for (jj=0; jj<3; jj++) \
    { \
    n[jj] = n0[jj] + t * (n1[jj] - n0[jj]); \
    } \
  if (ComputeGradients) \
    { \
    newGradients->InsertNextVector(n); \
    } \
  if (ComputeNormals) \
    { \
    vtkMath::Normalize(n); \
    n[0] = -n[0];  n[1] = -n[1];  n[2] = -n[2]; \
    newNormals->InsertNextNormal(n); \
    }   \
  } \
if (ComputeScalars) \
  { \
  newScalars->InsertNextScalar(value); \
  }


//
// Contouring filter specialized for images
//
template <class T>
static void ContourGrid(vtkGridSynchronizedTemplates3D *self,
			 T *scalars, vtkPoints *newPts,
			 vtkScalars *newScalars, vtkCellArray *polys,
			 vtkNormals *newNormals, vtkVectors *newGradients)
{
  int *dims = self->GetInput()->GetDimensions();
  int xdim = dims[0];
  int ydim = dims[1];
  int zdim = dims[2];
  float *values = self->GetValues();
  int numContours = self->GetNumberOfContours();
  float *inPtPtr, *p0, *p1, *p2, *p3;
  T *inPtr;
  int XMax, YMax, ZMax;
  float x[3];
  vtkPoints *inPts = self->GetInput()->GetPoints();
  float y, t;
  int *itmp, *isect1Ptr, *isect2Ptr;
  int ptIds[3];
  int *tablePtr;
  int v0, v1, v2, v3;
  int idx, vidx;
  T *s0, *s1, *s2, *s3;
  float value;
  int i, j, k;
  int zstep, yisectstep;
  int offsets[12];
  int ComputeNormals = self->GetComputeNormals();
  int ComputeGradients = self->GetComputeGradients();
  int ComputeScalars = self->GetComputeScalars();
  int NeedGradients = ComputeGradients || ComputeNormals;
  float n[3], n0[3], n1[3], n2[3], n3[3];
  int jj, g0;
  
  XMax = xdim - 1;
  YMax = ydim - 1;
  ZMax = zdim - 1;
  zstep = xdim*ydim;
  yisectstep = xdim*3;

  // compute offsets
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
    isect1[(YMax*xdim + i)*3 + 1] = -1;
    isect1[(YMax*xdim + i)*3*2 + 1] = -1;
    }

  // for each contour
  for (vidx = 0; vidx < numContours; vidx++)
    {
    value = values[vidx];
    inPtPtr = inPts->GetPoint(0);
    p2 = inPtPtr;
    inPtr = scalars;
    s2 = inPtr;
    v2 = (*s2 < value ? 0 : 1);

    //==================================================================
    for (k = 0; k < zdim; k++)
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

      for (j = 0; j < ydim; j++)
        {
        p1 = inPtPtr;
        s1 = inPtr;
        v1 = (*s1 < value ? 0 : 1);
        for (i = 0; i < xdim; i++)
	        {
          p0 = p1;
	        s0 = s1;
	        v0 = v1;
	        g0 = 0;
	        if (i < XMax)
	          {
	          p1 = (inPtPtr + 3);
	          s1 = (inPtr + 1);
	          v1 = (*s1 < value ? 0 : 1);
	          if (v0 ^ v1)
	            {
	            t = (value - (float)(*s0)) / ((float)(*s1) - (float)(*s0));
              x[0] = p0[0] + t*(p1[0] - p0[0]);
              x[1] = p0[1] + t*(p1[1] - p0[1]);
              x[2] = p0[2] + t*(p1[2] - p0[2]);
              *isect2Ptr = newPts->InsertNextPoint(x);
              VTK_CSP3PA(i+1,j,k,s1,p1);
              }
            else
              {
              *isect2Ptr = -1;
              }
            }
          if (j < YMax)
	          {
	          p2 = (inPtPtr + xdim*3);
	          s2 = (inPtr + xdim);
	          v2 = (*s2 < value ? 0 : 1);
	          if (v0 ^ v2)
	            {
	            t = (value - (float)(*s0)) / ((float)(*s2) - (float)(*s0));
              x[0] = p0[0] + t*(p2[0] - p0[0]);
              x[1] = p0[1] + t*(p2[1] - p0[1]);
              x[2] = p0[2] + t*(p2[2] - p0[2]);
	            *(isect2Ptr + 1) = newPts->InsertNextPoint(x);
	            VTK_CSP3PA(i,j+1,k,s2,p2);
	            }
	          else
	            {
	            *(isect2Ptr + 1) = -1;
	            }
	          }
	        if (k < ZMax)
	          {
	          p3 = (inPtPtr + zstep*3);
	          s3 = (inPtr + zstep);
	          v3 = (*s3 < value ? 0 : 1);
	          if (v0 ^ v3)
	            {
	            t = (value - (float)(*s0)) / ((float)(*s3) - (float)(*s0));
              x[0] = p0[0] + t*(p3[0] - p0[0]);
              x[1] = p0[1] + t*(p3[1] - p0[1]);
              x[2] = p0[2] + t*(p3[2] - p0[2]);
	            *(isect2Ptr + 2) = newPts->InsertNextPoint(x);
	            VTK_CSP3PA(i,j,k+1,s3,p3);
	            }
	          else
	            {
	            *(isect2Ptr + 2) = -1;
	            }
	          }
	  
	        // now add any polys that need to be added
	        // basically look at the isect values, 
	        // form an index and lookup the polys
	        if (j > 0 && i < XMax && k > 0)
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

	          tablePtr = VTK_SYNCHONIZED_TEMPLATES_3D_TABLE_2 
                        + VTK_SYNCHONIZED_TEMPLATES_3D_TABLE_1[idx];
	          while (*tablePtr != -1)
	            {
	            ptIds[0] = *(isect1Ptr + offsets[*tablePtr]);
	            tablePtr++;
	            ptIds[1] = *(isect1Ptr + offsets[*tablePtr]);
	            tablePtr++;
	            ptIds[2] = *(isect1Ptr + offsets[*tablePtr]);
	            tablePtr++;
	            polys->InsertNextCell(3,ptIds);
	            }
	          }
	        inPtPtr += 3;
	        inPtr++;
	        isect2Ptr += 3;
	        isect1Ptr += 3;
	        }
	      }
      }
    }
  delete [] isect1;
}



//
// Contouring filter specialized for images (or slices from images)
//
void vtkGridSynchronizedTemplates3D::Execute()
{
  vtkStructuredGrid *input= this->GetInput();
  vtkPointData *pd = input->GetPointData();
  vtkPoints *newPts;
  vtkCellArray *newPolys;
  vtkScalars *inScalars = pd->GetScalars();
  vtkPolyData *output = this->GetOutput();
  int *dims = this->GetInput()->GetDimensions();
  int dataSize, estimatedSize;
  vtkScalars *newScalars;
  vtkNormals *newNormals;
  vtkVectors *newGradients;
  

  vtkDebugMacro(<< "Executing 3D structured contour");
  
  if ( inScalars == NULL )
    {
    vtkErrorMacro(<<"Scalars must be defined for contouring");
    return;
    }

  //
  // Check dimensionality of data and get appropriate form
  //
  input->GetDimensions(dims);
  dataSize = dims[0] * dims[1] * dims[2];

  if ( input->GetDataDimension() != 3 )
    {
    vtkErrorMacro(<<"3D structured contours requires 3D data");
    return;
    }

  //
  // Allocate necessary objects
  //
  estimatedSize = (int) pow ((double) (dims[0] * dims[1] * dims[2]), .75);
  if (estimatedSize < 1024)
    {
    estimatedSize = 1024;
    }
  newPts = vtkPoints::New();
  newPts->Allocate(estimatedSize,estimatedSize);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(estimatedSize,3));

  if (this->ComputeNormals)
    {
    newNormals = vtkNormals::New();
    newNormals->Allocate(estimatedSize,estimatedSize/2);
    }
  else
    {
    newNormals = NULL;
    }
  
  if (this->ComputeGradients)
    {
    newGradients = vtkVectors::New();
    newGradients->Allocate(estimatedSize,estimatedSize/2);
    }
  else
    {
    newGradients = NULL;
    }

  if (this->ComputeScalars)
    {
    newScalars = vtkScalars::New();
    newScalars->Allocate(estimatedSize,estimatedSize/2);
    }
  else
    {
    newScalars = NULL;
    }

  //
  // Check data type and execute appropriate function
  //
  if (inScalars->GetNumberOfComponents() == 1 )
    {
    switch (inScalars->GetDataType())
      {
      case VTK_CHAR:
	      {
	      char *scalars = ((vtkCharArray *)inScalars->GetData())->GetPointer(0);
	      ContourGrid(this, scalars, newPts, newScalars, newPolys, 
		           newNormals, newGradients);
	      }
      break;
      case VTK_UNSIGNED_CHAR:
	      {
	      unsigned char *scalars = 
	        ((vtkUnsignedCharArray *)inScalars->GetData())->GetPointer(0);
	      ContourGrid(this, scalars, newPts, newScalars, newPolys,
		           newNormals, newGradients);
	      }
      break;
      case VTK_SHORT:
	      {
	      short *scalars = 
	        ((vtkShortArray *)inScalars->GetData())->GetPointer(0);
	      ContourGrid(this, scalars, newPts, newScalars, newPolys,
		           newNormals, newGradients);
	      }
      break;
      case VTK_UNSIGNED_SHORT:
	      {
	      unsigned short *scalars = 
	        ((vtkUnsignedShortArray *)inScalars->GetData())->GetPointer(0);
	      ContourGrid(this, scalars, newPts, newScalars, newPolys,
		           newNormals, newGradients);
	      }
      break;
      case VTK_INT:
	      {
	      int *scalars = ((vtkIntArray *)inScalars->GetData())->GetPointer(0);
	      ContourGrid(this, scalars, newPts, newScalars, newPolys,
		           newNormals, newGradients);
	      }
      break;
      case VTK_UNSIGNED_INT:
	      {
	      unsigned int *scalars = 
	        ((vtkUnsignedIntArray *)inScalars->GetData())->GetPointer(0);
	      ContourGrid(this, scalars, newPts, newScalars, newPolys,
		           newNormals, newGradients);
	      }
      break;
      case VTK_LONG:
	      {
	      long *scalars = ((vtkLongArray *)inScalars->GetData())->GetPointer(0);
	      ContourGrid(this, scalars, newPts, newScalars, newPolys,
		           newNormals, newGradients);
	      }
      break;
      case VTK_UNSIGNED_LONG:
	      {
	      unsigned long *scalars = 
	        ((vtkUnsignedLongArray *)inScalars->GetData())->GetPointer(0);
	      ContourGrid(this, scalars, newPts, newScalars, newPolys,
		           newNormals, newGradients);
	      }
      break;
      case VTK_FLOAT:
	      {
	      float *scalars =((vtkFloatArray *)inScalars->GetData())->GetPointer(0);
	      ContourGrid(this, scalars, newPts, newScalars, newPolys,
		           newNormals, newGradients);
	      }
      break;
      case VTK_DOUBLE:
	      {
	      double *scalars = 
	        ((vtkDoubleArray *)inScalars->GetData())->GetPointer(0);
	      ContourGrid(this, scalars, newPts, newScalars, newPolys,
		           newNormals, newGradients);
	      }
      break;
      }//switch
    }
  else //multiple components - have to convert
    {
    vtkScalars *image = vtkScalars::New();
    image->Allocate(dataSize);
    inScalars->GetScalars(0,dataSize,image);
    float *scalars = ((vtkFloatArray *)image->GetData())->GetPointer(0);
    ContourGrid(this, scalars, newPts, newScalars, newPolys,
		 newNormals, newGradients);
    image->Delete();
    }

  vtkDebugMacro(<<"Created: " 
               << newPts->GetNumberOfPoints() << " points, " 
               << newPolys->GetNumberOfCells() << " polygons");
  //
  // Update ourselves.  Because we don't know up front how many polys
  // we've created, take care to reclaim memory. 
  //
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();

  if (newScalars)
    {
    output->GetPointData()->SetScalars(newScalars);
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
}

void vtkGridSynchronizedTemplates3D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredGridToPolyDataFilter::PrintSelf(os,indent);

  this->ContourValues->PrintSelf(os,indent);

  os << indent << "Compute Normals: " << (this->ComputeNormals ? "On\n" : "Off\n");
  os << indent << "Compute Gradients: " << (this->ComputeGradients ? "On\n" : "Off\n");
  os << indent << "Compute Scalars: " << (this->ComputeScalars ? "On\n" : "Off\n");
}


