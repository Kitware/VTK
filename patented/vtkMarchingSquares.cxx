/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarchingSquares.cxx
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
#include "vtkMarchingSquares.h"
#include "vtkMarchingSquaresCases.h"
#include "vtkStructuredPoints.h"
#include "vtkMergePoints.h"
#include "vtkCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkShortArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkLongArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkDoubleArray.h"

// Description:
// Construct object with initial scalar range (0,1) and single contour value
// of 0.0. The ImageRange are set to extract the first k-plane.
vtkMarchingSquares::vtkMarchingSquares()
{
  this->ContourValues = vtkContourValues::New();

  this->ImageRange[0] = 0; this->ImageRange[1] = VTK_LARGE_INTEGER;
  this->ImageRange[2] = 0; this->ImageRange[3] = VTK_LARGE_INTEGER;
  this->ImageRange[4] = 0; this->ImageRange[5] = 0;

  this->Locator = NULL;
}

vtkMarchingSquares::~vtkMarchingSquares()
{
  this->ContourValues->Delete();
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
}

void vtkMarchingSquares::SetImageRange(int imin, int imax, int jmin, int jmax, 
                                       int kmin, int kmax)
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
unsigned long vtkMarchingSquares::GetMTime()
{
  unsigned long mTime=this->vtkStructuredPointsToPolyDataFilter::GetMTime();
  unsigned long mTime2=this->ContourValues->GetMTime();

  mTime = ( mTime2 > mTime ? mTime2 : mTime );
  if (this->Locator)
    {
    mTime2=this->Locator->GetMTime();
    mTime = ( mTime2 > mTime ? mTime2 : mTime );
    }

  return mTime;
}

//
// Contouring filter specialized for images
//
template <class T>
static void ContourImage(T *scalars, vtkScalars *newScalars, int roi[6], int dir[3],
                  int start[2], int end[2], int offset[3], float ar[3], 
                  float origin[3], float *values, int numValues, 
                  vtkPointLocator *p, vtkCellArray *lines)
{
  int i, j;
  int ptIds[2];
  float t, *x1, *x2, x[3], xp, yp;
  float pts[4][3], min, max;
  int contNum, jOffset, idx, ii, jj, index, *vert;
  static int CASE_MASK[4] = {1,2,8,4};  LINE_CASES *lineCase;
  static int edges[4][2] = { {0,1}, {1,3}, {3,2}, {2,0} };
  EDGE_LIST  *edge;
  float value, s[4];

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
  x[dir[2]] = origin[dir[2]] + roi[dir[2]*2]*ar[dir[2]];

  // Traverse all pixel cells, generating line segements using marching squares.
  for ( j=roi[start[1]]; j < roi[end[1]]; j++ )
    {

    jOffset = j * offset[1];
    pts[0][dir[1]] = origin[dir[1]] + j*ar[dir[1]];
    yp = origin[dir[1]] + (j+1)*ar[dir[1]];

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
      pts[0][dir[0]] = origin[dir[0]] + i*ar[dir[0]];
      xp = origin[dir[0]] + (i+1)*ar[dir[0]];

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
        if ( index == 0 || index == 15 ) continue; //no lines

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
            if ( (ptIds[ii] = p->IsInsertedPoint(x)) < 0 )
              {
              ptIds[ii] = p->InsertNextPoint(x);
              newScalars->InsertScalar(ptIds[ii],value);
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

//
// Contouring filter specialized for images (or slices from images)
//
void vtkMarchingSquares::Execute()
{
  vtkStructuredPoints *input=(vtkStructuredPoints *)this->Input;
  vtkPointData *pd=input->GetPointData();
  vtkPoints *newPts;
  vtkCellArray *newLines;
  vtkScalars *inScalars=pd->GetScalars(), *newScalars;
  int i, dims[3], roi[6], dataSize, dim, plane=0;
  float origin[3], ar[3];
  vtkPolyData *output = this->GetOutput();
  int start[2], end[2], offset[3], dir[3], estimatedSize;
  int numContours=this->ContourValues->GetNumberOfContours();
  float *values=this->ContourValues->GetValues();

  vtkDebugMacro(<< "Executing marching squares");
//
// Initialize and check input
//
  if ( inScalars == NULL )
    {
    vtkErrorMacro(<<"Scalars must be defined for contouring");
    return;
    }
//
// Check dimensionality of data and get appropriate form
//
  input->GetDimensions(dims);
  input->GetOrigin(origin);
  input->GetSpacing(ar);
  dataSize = dims[0] * dims[1] * dims[2];

  if ( input->GetDataDimension() != 2 )
    {
    for (i=0; i < 6; i++)
      {
      roi[i] = this->ImageRange[i];
      }
    }
  else
    {
    roi[0] = roi[2] = roi[4] = 0;
    for (i=0; i < 3; i++)
      {
      roi[2*i+1] = dims[i] - 1;
      }
    }

  // check the final region of interest to make sure its acceptable
  for ( dim=0, i=0; i < 3; i++ )
    {
    if ( roi[2*i+1] >= dims[i] )
      {
      roi[2*i+1] = dims[i] - 1;
      }
    else if ( roi[2*i+1] < 0 )
      {
      roi[2*i+1] = 0;
      }

    if ( roi[2*i] > roi[2*i+1] )
      {
      roi[2*i] = roi[2*i+1];
      }
    else if ( roi[2*i] < 0 )
      {
      roi[2*i] = 0;
      }

    if ( (roi[2*i+1]-roi[2*i]) > 0 )
      {
      dim++;
      }
    else
      {
      plane = i;
      }
    }

  if ( dim != 2 )
    {
    vtkErrorMacro(<<"Marching squares requires 2D data");
    return;
    }
//
// Setup indices and offsets (since can have x-y or z-plane)
//
  if ( plane == 0 ) //x-plane
    {
    start[0] = 2; end[0] = 3;
    start[1] = 4; end[1] = 5;
    offset[0] = dims[0];
    offset[1] = dims[0]*dims[1];
    offset[2] = roi[0];
    dir[0] = 1; dir[1] = 2; dir[2] = 0;
    }
  else if ( plane == 1 ) //y-plane
    {
    start[0] = 0; end[0] = 1;
    start[1] = 4; end[1] = 5;
    offset[0] = 1;
    offset[1] = dims[0]*dims[1];
    offset[2] = roi[2]*dims[0];
    dir[0] = 0; dir[1] = 2; dir[2] = 1;
    }
  else //z-plane
    {
    start[0] = 0; end[0] = 1;
    start[1] = 2; end[1] = 3;
    offset[0] = 1;
    offset[1] = dims[0];
    offset[2] = roi[4]*dims[0]*dims[1];
    dir[0] = 0; dir[1] = 1; dir[2] = 2;
    }
//
// Allocate necessary objects
//
  estimatedSize = (int) (numContours * sqrt((double)dims[0]*dims[1]));
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024)
    {
    estimatedSize = 1024;
    }

  newPts = vtkPoints::New();
  newPts->Allocate(estimatedSize,estimatedSize);
  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(estimatedSize,2));

  // locator used to merge potentially duplicate points
  if ( this->Locator == NULL )
    {
    this->CreateDefaultLocator();
    }
  this->Locator->InitPointInsertion (newPts, ((vtkDataSet *)this->Input)->GetBounds());
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
	newScalars = vtkScalars::New(VTK_CHAR,1);
	newScalars->Allocate(5000,25000);
	ContourImage(scalars,newScalars,roi,dir,start,end,offset,ar,origin,
		     values,numContours,this->Locator,newLines);
	}
      break;
      case VTK_UNSIGNED_CHAR:
	{
	unsigned char *scalars = ((vtkUnsignedCharArray *)inScalars->GetData())->GetPointer(0);
	newScalars = vtkScalars::New(VTK_UNSIGNED_CHAR,1);
	newScalars->Allocate(5000,25000);
	ContourImage(scalars,newScalars,roi,dir,start,end,offset,ar,origin,
		     values,numContours,this->Locator,newLines);
	}
      break;
      case VTK_SHORT:
	{
	short *scalars = ((vtkShortArray *)inScalars->GetData())->GetPointer(0);
	newScalars = vtkScalars::New(VTK_SHORT,1);
	newScalars->Allocate(5000,25000);
	ContourImage(scalars,newScalars,roi,dir,start,end,offset,ar,origin,
		     values,numContours,this->Locator,newLines);
	}
      break;
      case VTK_UNSIGNED_SHORT:
	{
	unsigned short *scalars = ((vtkUnsignedShortArray *)inScalars->GetData())->GetPointer(0);
	newScalars = vtkScalars::New(VTK_UNSIGNED_SHORT,1);
	newScalars->Allocate(5000,25000);
	ContourImage(scalars,newScalars,roi,dir,start,end,offset,ar,origin,
		     values,numContours,this->Locator,newLines);
	}
      break;
      case VTK_INT:
	{
	int *scalars = ((vtkIntArray *)inScalars->GetData())->GetPointer(0);
	newScalars = vtkScalars::New(VTK_INT,1);
	newScalars->Allocate(5000,25000);
	ContourImage(scalars,newScalars,roi,dir,start,end,offset,ar,origin,
		     values,numContours,this->Locator,newLines);
	}
      break;
      case VTK_UNSIGNED_INT:
	{
	unsigned int *scalars = ((vtkUnsignedIntArray *)inScalars->GetData())->GetPointer(0);
	newScalars = vtkScalars::New(VTK_UNSIGNED_INT,1);
	newScalars->Allocate(5000,25000);
	ContourImage(scalars,newScalars,roi,dir,start,end,offset,ar,origin,
		     values,numContours,this->Locator,newLines);
	}
      break;
      case VTK_LONG:
	{
	long *scalars = ((vtkLongArray *)inScalars->GetData())->GetPointer(0);
	newScalars = vtkScalars::New(VTK_LONG,1);
	newScalars->Allocate(5000,25000);
	ContourImage(scalars,newScalars,roi,dir,start,end,offset,ar,origin,
		     values,numContours,this->Locator,newLines);
	}
      break;
      case VTK_UNSIGNED_LONG:
	{
	unsigned long *scalars = ((vtkUnsignedLongArray *)inScalars->GetData())->GetPointer(0);
	newScalars = vtkScalars::New(VTK_UNSIGNED_LONG,1);
	newScalars->Allocate(5000,25000);
	ContourImage(scalars,newScalars,roi,dir,start,end,offset,ar,origin,
		     values,numContours,this->Locator,newLines);
	}
      break;
      case VTK_FLOAT:
	{
	float *scalars = ((vtkFloatArray *)inScalars->GetData())->GetPointer(0);
	newScalars = vtkScalars::New(VTK_FLOAT,1);
	newScalars->Allocate(5000,25000);
	ContourImage(scalars,newScalars,roi,dir,start,end,offset,ar,origin,
		     values,numContours,this->Locator,newLines);
	}
      break;
      case VTK_DOUBLE:
	{
	double *scalars = ((vtkDoubleArray *)inScalars->GetData())->GetPointer(0);
	newScalars = vtkScalars::New(VTK_DOUBLE,1);
	newScalars->Allocate(5000,25000);
	ContourImage(scalars,newScalars,roi,dir,start,end,offset,ar,origin,
		     values,numContours,this->Locator,newLines);
	}
      break;
      }//switch
    }

  else //multiple components - have to convert
    {
    vtkScalars *image = vtkScalars::New();
    image->Allocate(dataSize);
    inScalars->GetScalars(0,dataSize,*image);
    newScalars = vtkScalars::New(VTK_FLOAT);
    newScalars->Allocate(5000,25000);
    float *scalars = ((vtkFloatArray *)image->GetData())->GetPointer(0);
    ContourImage(scalars,newScalars,roi,dir,start,end,offset,ar,origin,
		 values,numContours,this->Locator,newLines);
    image->Delete();
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

  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();

  this->Locator->Initialize();
  output->Squeeze();
}

// Description:
// Specify a spatial locator for merging points. By default, 
// an instance of vtkMergePoints is used.
void vtkMarchingSquares::SetLocator(vtkPointLocator *locator)
{
  if ( this->Locator == locator)
    {
    return;
    }

  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
  
  if ( locator )
    {
    locator->Register(this);
    }
  
  this->Locator = locator;
  this->Modified();
}

void vtkMarchingSquares::CreateDefaultLocator()
{
  if ( this->Locator == NULL)
    {
    this->Locator = vtkMergePoints::New();
    }
}

void vtkMarchingSquares::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsToPolyDataFilter::PrintSelf(os,indent);

  this->ContourValues->PrintSelf(os,indent);

  os << indent << "Image Range: ( " 
     << this->ImageRange[0] << ", "
     << this->ImageRange[1] << ", "
     << this->ImageRange[2] << ", "
     << this->ImageRange[3] << ", "
     << this->ImageRange[4] << ", "
     << this->ImageRange[5] << " )\n";

  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }
}


