/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarchingCubes.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

    THIS CLASS IS PATENTED UNDER UNITED STATES PATENT NUMBER 4,710,876
    "System and Method for the Display of Surface Structures Contained
    Within the Interior Region of a Solid Body".
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
#include "vtkMarchingCubes.h"
#include "vtkMarchingCubesCases.h"
#include "vtkMergePoints.h"
#include "vtkStructuredPoints.h"
#include "vtkMath.h"
#include "vtkUnsignedCharScalars.h"
#include "vtkUnsignedShortScalars.h"
#include "vtkShortScalars.h"
#include "vtkFloatScalars.h"
#include "vtkIntScalars.h"

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
  this->SelfCreatedLocator = 0;
}

vtkMarchingCubes::~vtkMarchingCubes()
{
  this->ContourValues->Delete();
  if ( this->SelfCreatedLocator ) this->Locator->Delete();
  this->SelfCreatedLocator = 0;
}

// Description:
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
unsigned long vtkMarchingCubes::GetMTime()
{
  unsigned long mTime=this->vtkStructuredPointsToPolyDataFilter::GetMTime();
  unsigned long contourValuesMTime=this->ContourValues->GetMTime();

  mTime = ( contourValuesMTime > mTime ? contourValuesMTime : mTime );

  return mTime;
}

// Calculate the gradient using central difference.
// NOTE: We calculate the negative of the gradient for efficiency
template <class T>
static void ComputePointGradient(int i, int j, int k, T *s, int dims[3], 
                          int sliceSize, float Spacing[3], float n[3])
{
  float sp, sm;

  // x-direction
  if ( i == 0 )
    {
    sp = s[i+1 + j*dims[0] + k*sliceSize];
    sm = s[i + j*dims[0] + k*sliceSize];
    n[0] = (sm - sp) / Spacing[0];
    }
  else if ( i == (dims[0]-1) )
    {
    sp = s[i + j*dims[0] + k*sliceSize];
    sm = s[i-1 + j*dims[0] + k*sliceSize];
    n[0] = (sm - sp) / Spacing[0];
    }
  else
    {
    sp = s[i+1 + j*dims[0] + k*sliceSize];
    sm = s[i-1 + j*dims[0] + k*sliceSize];
    n[0] = 0.5 * (sm - sp) / Spacing[0];
    }

  // y-direction
  if ( j == 0 )
    {
    sp = s[i + (j+1)*dims[0] + k*sliceSize];
    sm = s[i + j*dims[0] + k*sliceSize];
    n[1] = (sm - sp) / Spacing[1];
    }
  else if ( j == (dims[1]-1) )
    {
    sp = s[i + j*dims[0] + k*sliceSize];
    sm = s[i + (j-1)*dims[0] + k*sliceSize];
    n[1] = (sm - sp) / Spacing[1];
    }
  else
    {
    sp = s[i + (j+1)*dims[0] + k*sliceSize];
    sm = s[i + (j-1)*dims[0] + k*sliceSize];
    n[1] = 0.5 * (sm - sp) / Spacing[1];
    }

  // z-direction
  if ( k == 0 )
    {
    sp = s[i + j*dims[0] + (k+1)*sliceSize];
    sm = s[i + j*dims[0] + k*sliceSize];
    n[2] = (sm - sp) / Spacing[2];
    }
  else if ( k == (dims[2]-1) )
    {
    sp = s[i + j*dims[0] + k*sliceSize];
    sm = s[i + j*dims[0] + (k-1)*sliceSize];
    n[2] = (sm - sp) / Spacing[2];
    }
  else
    {
    sp = s[i + j*dims[0] + (k+1)*sliceSize];
    sm = s[i + j*dims[0] + (k-1)*sliceSize];
    n[2] = 0.5 * (sm - sp) / Spacing[2];
    }
}

//
// Contouring filter specialized for volumes and "short int" data values.  
//
template <class T>
static void ContourVolume(T *scalars, int dims[3], float origin[3], float Spacing[3],
		   vtkPointLocator *locator, vtkScalars *newScalars, 
                   vtkFloatVectors *newGradients, vtkFloatNormals *newNormals, 
                   vtkCellArray *newPolys, float *values, int numValues)
{
  float s[8], value;
  int i, j, k, sliceSize;
  static int CASE_MASK[8] = {1,2,4,8,16,32,64,128};
  TRIANGLE_CASES *triCase;
  EDGE_LIST  *edge;
  int contNum, jOffset, kOffset, idx, ii, jj, index, *vert;
  int ptIds[3];
  int ComputeNormals = newNormals != NULL;
  int ComputeGradients = newGradients != NULL;
  int ComputeScalars = newScalars != NULL;
  int NeedGradients;
  float t, *x1, *x2, x[3], *n1, *n2, n[3], min, max;
  float pts[8][3], gradients[8][3], xp, yp, zp;
  static int edges[12][2] = { {0,1}, {1,2}, {3,2}, {0,3},
                              {4,5}, {5,6}, {7,6}, {4,7},
                              {0,4}, {1,5}, {3,7}, {2,6}};
//
// Get min/max contour values
//
  if ( numValues < 1 ) return;
  for ( min=max=values[0], i=1; i < numValues; i++)
    {
    if ( values[i] < min ) min = values[i];
    if ( values[i] > max ) max = values[i];
    }
//
// Traverse all voxel cells, generating triangles and point gradients
// using marching cubes algorithm.
//  
  sliceSize = dims[0] * dims[1];
  for ( k=0; k < (dims[2]-1); k++)
    {
    kOffset = k*sliceSize;
    pts[0][2] = origin[2] + k*Spacing[2];
    zp = origin[2] + (k+1)*Spacing[2];
    for ( j=0; j < (dims[1]-1); j++)
      {
      jOffset = j*dims[0];
      pts[0][1] = origin[1] + j*Spacing[1];
      yp = origin[1] + (j+1)*Spacing[1];
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
        pts[0][0] = origin[0] + i*Spacing[0];
        xp = origin[0] + (i+1)*Spacing[0];

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
	  ComputePointGradient(i,j,k, scalars, dims, sliceSize, Spacing, gradients[0]);
	  ComputePointGradient(i+1,j,k, scalars, dims, sliceSize, Spacing, gradients[1]);
	  ComputePointGradient(i+1,j+1,k, scalars, dims, sliceSize, Spacing, gradients[2]);
	  ComputePointGradient(i,j+1,k, scalars, dims, sliceSize, Spacing, gradients[3]);
	  ComputePointGradient(i,j,k+1, scalars, dims, sliceSize, Spacing, gradients[4]);
	  ComputePointGradient(i+1,j,k+1, scalars, dims, sliceSize, Spacing, gradients[5]);
	  ComputePointGradient(i+1,j+1,k+1, scalars, dims, sliceSize, Spacing, gradients[6]);
	  ComputePointGradient(i,j+1,k+1, scalars, dims, sliceSize, Spacing, gradients[7]);
	  }
        for (contNum=0; contNum < numValues; contNum++)
          {
          value = values[contNum];
          // Build the case table
          for ( ii=0, index = 0; ii < 8; ii++)
              if ( s[ii] >= value )
                  index |= CASE_MASK[ii];

          if ( index == 0 || index == 255 ) continue; //no surface

          triCase = triCases + index;
          edge = triCase->edges;

          for ( ; edge[0] > -1; edge += 3 )
            {
            for (ii=0; ii<3; ii++) //insert triangle
              {
              vert = edges[edge[ii]];
              t = (value - s[vert[0]]) / (s[vert[1]] - s[vert[0]]);
              x1 = pts[vert[0]];
              x2 = pts[vert[1]];
              for (jj=0; jj<3; jj++)
                {
                x[jj] = x1[jj] + t * (x2[jj] - x1[jj]);
                }
	      // check for a new point
	      if ( (ptIds[ii] = locator->IsInsertedPoint (x)) < 0)
                  {
                  ptIds[ii] = locator->InsertNextPoint(x);

	          if (NeedGradients)
    		    {
		    n1 = gradients[vert[0]];
		    n2 = gradients[vert[1]];
		    for (jj=0; jj<3; jj++)
		      {
		      n[jj] = n1[jj] + t * (n2[jj] - n1[jj]);
 		      }
		    }
                  if (ComputeScalars) newScalars->InsertScalar(ptIds[ii],value);
	          if (ComputeGradients) newGradients->InsertVector(ptIds[ii],n);
                  if (ComputeNormals)
		    {
 		    vtkMath::Normalize(n);
		    newNormals->InsertNormal(ptIds[ii],n);
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
void vtkMarchingCubes::Execute()
{
  vtkFloatPoints *newPts;
  vtkCellArray *newPolys;
  vtkScalars *newScalars;
  vtkFloatNormals *newNormals;
  vtkFloatVectors *newGradients;
  vtkStructuredPoints *input=(vtkStructuredPoints *)this->Input;
  vtkPointData *pd=input->GetPointData();
  vtkScalars *inScalars=pd->GetScalars();
  char *type;
  int dims[3];
  int estimatedSize;
  float Spacing[3], origin[3];
  float bounds[6];
  vtkPolyData *output = this->GetOutput();
  int numContours=this->ContourValues->GetNumberOfContours();
  float *values=this->ContourValues->GetValues();
  
  vtkDebugMacro(<< "Executing marching cubes");
//
// Initialize and check input
//
  if ( inScalars == NULL )
    {
    vtkErrorMacro(<<"Scalars must be defined for contouring");
    return;
    }

  if ( input->GetDataDimension() != 3 )
    {
    vtkErrorMacro(<<"Cannot contour data of dimension != 3");
    return;
    }
  input->GetDimensions(dims);
  input->GetOrigin(origin);
  input->GetSpacing(Spacing);

  // estimate the number of points from the volume dimensions
  estimatedSize = (int) pow ((double) (dims[0] * dims[1] * dims[2]), .75);
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024) estimatedSize = 1024;

  vtkDebugMacro(<< "Estimated allocation size is " << estimatedSize);
  newPts = vtkFloatPoints::New(); newPts->Allocate(estimatedSize,estimatedSize/2);
  // compute bounds for merging points
  for ( int i=0; i<3; i++)
    {
    bounds[2*i] = origin[i];
    bounds[2*i+1] = origin[i] + (dims[i]-1) * Spacing[i];
    }
  if ( this->Locator == NULL ) this->CreateDefaultLocator();
  this->Locator->InitPointInsertion (newPts, bounds);

  if (this->ComputeNormals)
    {
    newNormals = vtkFloatNormals::New();
    newNormals->Allocate(estimatedSize,estimatedSize/2);
    }
  else
    {
    newNormals = NULL;
    }

  if (this->ComputeGradients)
    {
    newGradients = vtkFloatVectors::New();
    newGradients->Allocate(estimatedSize,estimatedSize/2);
    }
  else
    {
    newGradients = NULL;
    }

  newPolys = vtkCellArray::New();
  newPolys->Allocate(estimatedSize,estimatedSize/2);

  type = inScalars->GetDataType();
  if ( !strcmp(type,"unsigned char") && 
  inScalars->GetNumberOfValuesPerScalar() == 1 )
    {
    unsigned char *scalars = ((vtkUnsignedCharScalars *)inScalars)->GetPointer(0);
    if (this->ComputeScalars)
      {
      newScalars = vtkUnsignedCharScalars::New();
      newScalars->Allocate(estimatedSize,estimatedSize/2);
      }
    else
      {
      newScalars = NULL;
      }
    ContourVolume(scalars,dims,origin,Spacing,this->Locator,newScalars,newGradients,
                  newNormals,newPolys,values,numContours);
    }

  else if ( !strcmp(type,"short") )
    {
    short *scalars = ((vtkShortScalars *)inScalars)->GetPointer(0);
    if (this->ComputeScalars)
      {
      newScalars = vtkShortScalars::New();
      newScalars->Allocate(estimatedSize,estimatedSize/2);
      }
    else
      {
      newScalars = NULL;
      }

    ContourVolume(scalars,dims,origin,Spacing,this->Locator,newScalars,newGradients,
                  newNormals,newPolys,values,numContours);
    }
  
  else if ( !strcmp(type,"unsigned short") )
    {
    unsigned short *scalars = ((vtkUnsignedShortScalars *)inScalars)->GetPointer(0);
    if (this->ComputeScalars)
      {
      newScalars = vtkUnsignedShortScalars::New();
      newScalars->Allocate(estimatedSize,estimatedSize/2);
      }
    else
      {
      newScalars = NULL;
      }

    ContourVolume(scalars,dims,origin,Spacing,this->Locator,newScalars,newGradients,
                  newNormals,newPolys,values,numContours);
    }
  
  else if ( !strcmp(type,"float") )
    {
    float *scalars = ((vtkFloatScalars *)inScalars)->GetPointer(0);
    if (this->ComputeScalars)
      {
      newScalars = vtkFloatScalars::New();
      newScalars->Allocate(estimatedSize,estimatedSize/2);
      }
    else
      {
      newScalars = NULL;
      }
    ContourVolume(scalars,dims,origin,Spacing,this->Locator,newScalars,newGradients,
                  newNormals,newPolys,values,numContours);
    }

  else if ( !strcmp(type,"int") )
    {
    int *scalars = ((vtkIntScalars *)inScalars)->GetPointer(0);
    if (this->ComputeScalars)
      {
      newScalars = vtkIntScalars::New();
      newScalars->Allocate(estimatedSize,estimatedSize/2);
      }
    else
      {
      newScalars = NULL;
      }
    ContourVolume(scalars,dims,origin,Spacing,this->Locator,newScalars,newGradients,
                  newNormals,newPolys,values,numContours);
    }

  else //use general method - temporarily copies image
    {
    int dataSize = dims[0] * dims[1] * dims[2];
    vtkFloatScalars *image=vtkFloatScalars::New(); image->Allocate(dataSize);
    inScalars->GetScalars(0,dataSize,*image);
    if (this->ComputeScalars)
      {
      newScalars = vtkFloatScalars::New();
      newScalars->Allocate(estimatedSize,estimatedSize/2);
      }
    else
      {
      newScalars = NULL;
      }
    float *scalars = image->GetPointer(0);
    ContourVolume(scalars,dims,origin,Spacing,this->Locator,newScalars,newGradients,
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
  if (this->Locator) this->Locator->Initialize(); //free storage
}

// Description:
// Specify a spatial locator for merging points. By default, 
// an instance of vtkMergePoints is used.
void vtkMarchingCubes::SetLocator(vtkPointLocator *locator)
{
  if ( this->Locator != locator ) 
    {
    if ( this->SelfCreatedLocator ) this->Locator->Delete();
    this->SelfCreatedLocator = 0;
    this->Locator = locator;
    this->Modified();
    }
}

void vtkMarchingCubes::CreateDefaultLocator()
{
  if ( this->SelfCreatedLocator ) this->Locator->Delete();
  this->Locator = vtkMergePoints::New();
  this->SelfCreatedLocator = 1;
}

void vtkMarchingCubes::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsToPolyDataFilter::PrintSelf(os,indent);

  this->ContourValues->PrintSelf(os,indent);

  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }
}









