/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSliceCubes.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkSliceCubes.h"
#include <stdio.h>
#include "vtkMarchingCubesCases.h"
#include "vtkMath.h"
#include "vtkUnsignedCharScalars.h"
#include "vtkShortScalars.h"
#include "vtkIntScalars.h"
#include "vtkFloatScalars.h"
#include "vtkByteSwap.h"

// Description:
// Construct with NULL reader, output filename specification, and limits 
// filename.
vtkSliceCubes::vtkSliceCubes()
{
  this->Reader = NULL;
  this->Filename = NULL;
  this->LimitsFilename = NULL;
}

// Description:
// Method causes object to read slices and generate isosurface.
void vtkSliceCubes::Update()
{
  this->Execute();
}

// Calculate the gradient using central difference.
// NOTE: We calculate the negative of the gradient for efficiency
template <class T>
void ComputePointGradient(int i, int j, int k, int dims[3], 
                          float aspectRatio[3], float n[3],
                          T *s0, T *s1, T *s2)
{
  float sp, sm;

  // x-direction
  if ( i == 0 )
    {
    sp = s1[i+1 + j*dims[0]];
    sm = s1[i + j*dims[0]];
    n[0] = (sm - sp) / aspectRatio[0];
    }
  else if ( i == (dims[0]-1) )
    {
    sp = s1[i + j*dims[0]];
    sm = s1[i-1 + j*dims[0]];
    n[0] = (sm - sp) / aspectRatio[0];
    }
  else
    {
    sp = s1[i+1 + j*dims[0]];
    sm = s1[i-1 + j*dims[0]];
    n[0] = 0.5 * (sm - sp) / aspectRatio[0];
    }

  // y-direction
  if ( j == 0 )
    {
    sp = s1[i + (j+1)*dims[0]];
    sm = s1[i + j*dims[0]];
    n[1] = (sm - sp) / aspectRatio[1];
    }
  else if ( j == (dims[1]-1) )
    {
    sp = s1[i + j*dims[0]];
    sm = s1[i + (j-1)*dims[0]];
    n[1] = (sm - sp) / aspectRatio[1];
    }
  else

    {
    sp = s1[i + (j+1)*dims[0]];
    sm = s1[i + (j-1)*dims[0]];
    n[1] = 0.5 * (sm - sp) / aspectRatio[1];
    }

  // z-direction
  // z-direction
  if ( k == 0 )
    {
    sp = s2[i + j*dims[0]];
    sm = s1[i + j*dims[0]];
    n[2] = (sm - sp) / aspectRatio[2];
    }
  else if ( k == (dims[2]-1) )
    {
    sp = s1[i + j*dims[0]];
    sm = s0[i + j*dims[0]];
    n[2] = (sm - sp) / aspectRatio[2];
    }
  else
    {
    sp = s2[i + j*dims[0]];
    sm = s0[i + j*dims[0]];
    n[2] = 0.5 * (sm - sp) / aspectRatio[2];
    }
}

template <class T, class S>
int Contour(T *slice, S *scalars, int imageRange[2], int dims[3], float origin[3],
            float aspectRatio[3], float value, float xmin[3], float xmax[3],
            FILE *outFP, vtkVolumeReader *reader, int debug)
{
  S *slice0scalars=NULL, *slice1scalars;
  S *slice2scalars, *slice3scalars;
  T *slice0, *slice1, *slice2, *slice3;
  vtkFloatScalars *floatScalars=NULL;
  int numTriangles=0;
  float s[8];
  int i, j, k, idx, jOffset, ii, index, *vert, jj, sliceSize=0;
  static int CASE_MASK[8] = {1,2,4,8,16,32,64,128};
  TRIANGLE_CASES *triCase;
  EDGE_LIST  *edge;
  float pts[8][3], grad[8][3];
  float t, *x1, *x2, *n1, *n2;
  float point[6];
  vtkByteSwap swap;
  static int edges[12][2] = { {0,1}, {1,2}, {3,2}, {0,3},
                              {4,5}, {5,6}, {7,6}, {4,7},
                              {0,4}, {1,5}, {3,7}, {2,6}};

  if ( slice == NULL ) //have to do conversion to float slice-by-slice
    {
    sliceSize = dims[0] * dims[1];
    floatScalars = new vtkFloatScalars(sliceSize);
    }

  slice1scalars = NULL;
  slice2scalars = scalars;
  if (debug)  cerr << "  Slice# " << imageRange[0] << "\n";

  if ( slice2scalars == NULL ) return 0;
  if ( slice != NULL )
    {
    slice1 = slice2 = slice2scalars->GetPtr(0);
    }
  else
    {//get as float
    slice2scalars->GetScalars(0,sliceSize-1,*floatScalars);
    slice1 = slice2 = (T *) floatScalars->GetPtr(0);
    }
  slice3scalars = (S *) reader->GetImage(imageRange[0]+1)->GetPointData()->GetScalars();
  if (debug)  cerr << "  Slice# " << imageRange[0]+1 << "\n";

  if ( slice != NULL )
    {
    slice3 = slice3scalars->GetPtr(0);
    }
  else
    {//get as float: cast is ok because this code is only executed for float type
    slice3scalars->GetScalars(0,sliceSize-1,*floatScalars);
    slice3 = (T *) floatScalars->GetPtr(0);
    }

  if ( !slice2 || !slice3 )
    {
    cerr << "Cannot allocate data!";
    return 0;
    }

  // Generate triangles and normals from slices
  for (k=0; k < (dims[2]-1); k++)
    {

    // swap things around
    if ( slice0scalars != NULL ) slice0scalars->Delete();
    slice0scalars = slice1scalars;
    slice0 = slice1;
    slice1scalars = slice2scalars;
    slice1 = slice2;
    slice2scalars = slice3scalars;
    slice2 = slice3;
    if ( k < (dims[2]-2) )
      {
      if (debug)  cerr << "  Slice# " << imageRange[0]+k+2 << "\n";
      slice3scalars = (S *)
        reader->GetImage(imageRange[0]+k+2)->GetPointData()->GetScalars();
      if ( slice3scalars == NULL )
        {
        cerr << "Can't read all the requested slices\n";
        goto PREMATURE_TERMINATION;
        }
      if ( slice != NULL )
        {
        slice3 = slice3scalars->GetPtr(0);
        }
      else
        {//get as float
        slice3scalars->GetScalars(0,sliceSize-1,*floatScalars);
        slice3 = (T *) floatScalars->GetPtr(0);
        }
      }

    pts[0][2] = origin[2] + k*aspectRatio[2];
    for ( j=0; j < (dims[1]-1); j++)
      {
      jOffset = j*dims[0];
      pts[0][1] = origin[1] + j*aspectRatio[1];
      for ( i=0; i < (dims[0]-1); i++)
        {
        //get scalar values
        idx = i + jOffset;
        s[0] = slice1[idx];
        s[1] = slice1[idx+1];
        s[2] = slice1[idx+1 + dims[0]];
        s[3] = slice1[idx + dims[0]];
        s[4] = slice2[idx];
        s[5] = slice2[idx+1];
        s[6] = slice2[idx+1 + dims[0]];
        s[7] = slice2[idx + dims[0]];

        // Build the case table
        for ( ii=0, index = 0; ii < 8; ii++)
            if ( s[ii] >= value )
                index |= CASE_MASK[ii];

        if ( index == 0 || index == 255 ) continue; //no surface

        //create voxel points
        pts[0][0] = origin[0] + i*aspectRatio[0];

        pts[1][0] = pts[0][0] + aspectRatio[0];  
        pts[1][1] = pts[0][1];
        pts[1][2] = pts[0][2];

        pts[2][0] = pts[0][0] + aspectRatio[0];  
        pts[2][1] = pts[0][1] + aspectRatio[1];
        pts[2][2] = pts[0][2];

        pts[3][0] = pts[0][0];
        pts[3][1] = pts[0][1] + aspectRatio[1];
        pts[3][2] = pts[0][2];

        pts[4][0] = pts[0][0];
        pts[4][1] = pts[0][1];
        pts[4][2] = pts[0][2] + aspectRatio[2];

        pts[5][0] = pts[0][0] + aspectRatio[0];  
        pts[5][1] = pts[0][1];
        pts[5][2] = pts[0][2] + aspectRatio[2];

        pts[6][0] = pts[0][0] + aspectRatio[0];  
        pts[6][1] = pts[0][1] + aspectRatio[1];
        pts[6][2] = pts[0][2] + aspectRatio[2];

        pts[7][0] = pts[0][0];
        pts[7][1] = pts[0][1] + aspectRatio[1];
        pts[7][2] = pts[0][2] + aspectRatio[2];

        //create gradients
        ComputePointGradient(i,j, k, dims, aspectRatio, grad[0],
                             slice0, slice1, slice2);
        ComputePointGradient(i+1,j, k, dims, aspectRatio, grad[1],
                             slice0, slice1, slice2);
        ComputePointGradient(i+1,j+1, k, dims, aspectRatio, grad[2],
                             slice0, slice1, slice2);
        ComputePointGradient(i,j+1, k, dims, aspectRatio, grad[3],
                             slice0, slice1, slice2);
        ComputePointGradient(i,j, k+1, dims, aspectRatio, grad[4],
                             slice1, slice2, slice3);
        ComputePointGradient(i+1,j, k+1, dims, aspectRatio, grad[5],
                             slice1, slice2, slice3);
        ComputePointGradient(i+1,j+1, k+1, dims, aspectRatio, grad[6],
                             slice1, slice2, slice3);
        ComputePointGradient(i,j+1, k+1, dims, aspectRatio, grad[7],
                             slice1, slice2, slice3);

        triCase = triCases + index;
        edge = triCase->edges;

        for ( ; edge[0] > -1; edge += 3 )
          {
          for (ii=0; ii<3; ii++) //insert triangle
            {
            vert = edges[edge[ii]];
            t = (float)(value - s[vert[0]]) / (s[vert[1]] - s[vert[0]]);
            x1 = pts[vert[0]];
            x2 = pts[vert[1]];
            n1 = grad[vert[0]];
            n2 = grad[vert[1]];
            for (jj=0; jj<3; jj++)
              {
              point[jj] = x1[jj] + t * (x2[jj] - x1[jj]);
              point[jj+3] = n1[jj] + t * (n2[jj] - n1[jj]);
              if (point[jj] < xmin[jj] ) xmin[jj] = point[jj];
              if (point[jj] > xmax[jj] ) xmax[jj] = point[jj];
              }
            vtkMath::Normalize(point+3);
	    // swap bytes if necessary
	    swap.SwapWrite4BERange(point,6,outFP);
            }
          numTriangles++;
          }//for each triangle
        }//for i
      }//for j
    }//for k

  // Close things down
  PREMATURE_TERMINATION:

  fclose(outFP);
  if ( slice == NULL ) floatScalars->Delete();
  if (slice0scalars && slice0scalars != slice1scalars) slice0scalars->Delete();
  if (slice3scalars && slice3scalars != slice2scalars) slice3scalars->Delete();
  slice1scalars->Delete();
  slice2scalars->Delete();

  return numTriangles;
}

void vtkSliceCubes::Execute() 
{
  FILE *outFP;
  vtkStructuredPoints *tempStructPts;
  vtkScalars *inScalars;
  int dims[3], imageRange[2];
  float xmin[3], xmax[3];
  char *type;
  float origin[3], aspectRatio[3];
  int numTriangles;

  // check input/initialize
  vtkDebugMacro(<< "Executing slice cubes");
  if ( this->Reader == NULL )
   {
   vtkErrorMacro(<<"No reader specified...can't generate isosurface");
   return;
   }

  if ( this->Filename == NULL )
   {
   vtkErrorMacro(<<"No filename specified...can't output isosurface");
   return;
   }

  if ( (outFP = fopen(this->Filename, "w")) == NULL )
   {
   vtkErrorMacro(<<"Cannot open specified output file...");
   return;
   }

  // get image dimensions from the readers first slice
  this->Reader->GetImageRange(imageRange);
  tempStructPts = this->Reader->GetImage(imageRange[0]);
  tempStructPts->GetDimensions(dims);
  tempStructPts->GetOrigin(origin);
  tempStructPts->GetAspectRatio(aspectRatio);
  
  dims[2] = (imageRange[1] - imageRange[0] + 1);

  if ( (dims[0]*dims[1]*dims[2]) <= 1 || dims[2] < 2 )
    {
    vtkErrorMacro(<<"Bad dimensions...slice must be 3D volume");
    return;
    }

  xmin[0]=xmin[1]=xmin[2] = VTK_LARGE_FLOAT;
  xmax[0]=xmax[1]=xmax[2] = -VTK_LARGE_FLOAT;

  inScalars = tempStructPts->GetPointData()->GetScalars();
  if ( inScalars == NULL )
    {
    vtkErrorMacro(<<"Must have scalars to generate isosurface");
    return;
    }

  type = inScalars->GetDataType();
  if ( !strcmp(type,"unsigned char") && inScalars->GetNumberOfValuesPerScalar() == 1 )
    {
    vtkUnsignedCharScalars *scalars = (vtkUnsignedCharScalars *)inScalars;
    unsigned char *s = scalars->GetPtr(0);
    numTriangles = Contour(s,scalars,imageRange,dims,origin,aspectRatio,this->Value,
                  xmin,xmax,outFP,this->Reader,this->Debug);
    }
  else if ( !strcmp(type,"short") )
    {
    vtkShortScalars *scalars = (vtkShortScalars *)inScalars;
    short *s = scalars->GetPtr(0);
    numTriangles = Contour(s,scalars,imageRange,dims,origin,aspectRatio,this->Value,
                  xmin,xmax,outFP,this->Reader,this->Debug);
    }
  else if ( !strcmp(type,"float") )
    {
    vtkFloatScalars *scalars = (vtkFloatScalars *)inScalars;
    float *s = scalars->GetPtr(0);
    numTriangles = Contour(s,scalars,imageRange,dims,origin,aspectRatio,this->Value,
                  xmin,xmax,outFP,this->Reader,this->Debug);
    }
  else if ( !strcmp(type,"int") )
    {
    vtkIntScalars *scalars = (vtkIntScalars *)inScalars;
    int *s = scalars->GetPtr(0);
    numTriangles = Contour(s,scalars,imageRange,dims,origin,aspectRatio,this->Value,
                  xmin,xmax,outFP,this->Reader,this->Debug);
    }
  else //use general method
    {
    vtkFloatScalars *scalars = (vtkFloatScalars *)inScalars;
    float *s = NULL; //clue to use general method
    numTriangles = Contour(s,scalars,imageRange,dims,origin,aspectRatio,this->Value,
                  xmin,xmax,outFP,this->Reader,this->Debug);
    }

  vtkDebugMacro(<<"Created: " << 3*numTriangles << " points, " 
                << numTriangles << " triangles");

  if ( this->LimitsFilename )
    {
    vtkByteSwap swap;
    int i;
    float t;

    if ( (outFP = fopen(this->LimitsFilename, "w")) == NULL )
      {
      vtkWarningMacro(<<"Sorry, couldn't write limits file...");
      }
    else
      {
      for (i=0; i<3; i++)
        {
        t = origin[i] + (dims[i] - 1)*aspectRatio[i];
	swap.SwapWrite4BERange(origin+i,1,outFP);
	// swap if neccessary
	swap.SwapWrite4BERange(&t,1,outFP);
        }
      for (i=0; i<3; i++)
        {
	swap.SwapWrite4BERange(xmin+i,1,outFP);
	swap.SwapWrite4BERange(xmax+i,1,outFP);
        }
      }
     fclose(outFP);
    }
}

void vtkSliceCubes::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  if ( this->Reader )
    {
    os << indent << "Reader:\n";
    this->Reader->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Reader: (none)\n";
    }

  os << indent << "Filename: " 
     << (this->Filename ? this->Filename : "(none)") << "\n";
  os << indent << "Limits Filename: " 
     << (this->LimitsFilename ? this->LimitsFilename : "(none)") << "\n";
}
