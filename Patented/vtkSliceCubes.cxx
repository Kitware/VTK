/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSliceCubes.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

    THIS CLASS IS PATENTED UNDER UNITED STATES PATENT NUMBER 4,710,876
    "System and Method for the Display of Surface Structures Contained
    Within The Interior Region of a Solid body".
    Application of this software for commercial purposes requires 
    a license grant from GE. Contact:
        Jerald Roehling
        GE Licensing
        One Independence Way
        PO Box 2023
        Princeton, N.J. 08540
        phone 609-734-9823
        e-mail:Roehlinj@gerlmo.ge.com
    for more information.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkSliceCubes.h"
#include <stdio.h>
#include "vtkMarchingCubesCases.h"
#include "vtkMath.h"
#include "vtkByteSwap.h"
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
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkSliceCubes* vtkSliceCubes::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSliceCubes");
  if(ret)
    {
    return (vtkSliceCubes*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkSliceCubes;
}




// Description:
// Construct with NULL reader, output FileName specification, and limits 
// FileName.
vtkSliceCubes::vtkSliceCubes()
{
  this->Reader = NULL;
  this->FileName = NULL;
  this->LimitsFileName = NULL;
  this->Value = 0.0;
}

vtkSliceCubes::~vtkSliceCubes()
{
  if (this->FileName)
    {
    delete [] this->FileName;
    }
  if (this->LimitsFileName)
    {
    delete [] this->LimitsFileName;
    }
  this->SetReader(NULL);
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
static void ComputePointGradient(int i, int j, int k, int dims[3], 
                          float Spacing[3], float n[3],
                          T *s0, T *s1, T *s2)
{
  float sp, sm;

  // x-direction
  if ( i == 0 )
    {
    sp = s1[i+1 + j*dims[0]];
    sm = s1[i + j*dims[0]];
    n[0] = (sm - sp) / Spacing[0];
    }
  else if ( i == (dims[0]-1) )
    {
    sp = s1[i + j*dims[0]];
    sm = s1[i-1 + j*dims[0]];
    n[0] = (sm - sp) / Spacing[0];
    }
  else
    {
    sp = s1[i+1 + j*dims[0]];
    sm = s1[i-1 + j*dims[0]];
    n[0] = 0.5 * (sm - sp) / Spacing[0];
    }

  // y-direction
  if ( j == 0 )
    {
    sp = s1[i + (j+1)*dims[0]];
    sm = s1[i + j*dims[0]];
    n[1] = (sm - sp) / Spacing[1];
    }
  else if ( j == (dims[1]-1) )
    {
    sp = s1[i + j*dims[0]];
    sm = s1[i + (j-1)*dims[0]];
    n[1] = (sm - sp) / Spacing[1];
    }
  else

    {
    sp = s1[i + (j+1)*dims[0]];
    sm = s1[i + (j-1)*dims[0]];
    n[1] = 0.5 * (sm - sp) / Spacing[1];
    }

  // z-direction
  // z-direction
  if ( k == 0 )
    {
    sp = s2[i + j*dims[0]];
    sm = s1[i + j*dims[0]];
    n[2] = (sm - sp) / Spacing[2];
    }
  else if ( k == (dims[2]-1) )
    {
    sp = s1[i + j*dims[0]];
    sm = s0[i + j*dims[0]];
    n[2] = (sm - sp) / Spacing[2];
    }
  else
    {
    sp = s2[i + j*dims[0]];
    sm = s0[i + j*dims[0]];
    n[2] = 0.5 * (sm - sp) / Spacing[2];
    }
}

template <class T, class S>
static int vtkSliceCubesContour(T *slice, S *scalars, int imageRange[2], int dims[3], float origin[3],
            float Spacing[3], float value, float xmin[3], float xmax[3],
            FILE *outFP, vtkVolumeReader *reader, unsigned char debug)
{
  S *slice0scalars=NULL, *slice1scalars;
  S *slice2scalars, *slice3scalars;
  T *slice0, *slice1, *slice2, *slice3;
  vtkStructuredPoints *sp;
  vtkFloatArray *floatScalars=NULL;
  int numTriangles=0, numComp = 0;
  float s[8];
  int i, j, k, idx, jOffset, ii, index, *vert, jj, sliceSize=0;
  static int CASE_MASK[8] = {1,2,4,8,16,32,64,128};
  VTK_TRIANGLE_CASES *triCase;
  EDGE_LIST  *edge;
  float pts[8][3], grad[8][3];
  float t, *x1, *x2, *n1, *n2;
  float xp, yp, zp;
  float point[6];
  static int edges[12][2] = { {0,1}, {1,2}, {3,2}, {0,3},
                              {4,5}, {5,6}, {7,6}, {4,7},
                              {0,4}, {1,5}, {3,7}, {2,6}};

  if ( slice == NULL ) //have to do conversion to float slice-by-slice
    {
    sliceSize = dims[0] * dims[1];
    floatScalars = vtkFloatArray::New();
    floatScalars->Allocate(sliceSize);
    }

  slice1scalars = NULL;
  slice2scalars = scalars;
  slice2scalars->Register(NULL);

  if (debug)  vtkGenericWarningMacro(<< "  Slice# " << imageRange[0]);

  if ( slice2scalars == NULL )
    {
    return 0;
    }
  if ( slice != NULL )
    {
    slice1 = slice2 = slice2scalars->GetPointer(0);
    }
  else
    {//get as float
    numComp = scalars->GetNumberOfComponents();
    slice2scalars->GetData(0,sliceSize-1,0,numComp-1,floatScalars);
    slice1 = slice2 = (T *) floatScalars->GetPointer(0);
    }
    
  sp = reader->GetImage(imageRange[0]+1);
  slice3scalars = (S *) sp->GetPointData()->GetScalars()->GetData();
  slice3scalars->Register(NULL);
  sp->Delete();

  if (debug)  vtkGenericWarningMacro(<< "  Slice# " << imageRange[0]+1 );

  if ( slice != NULL )
    {
    slice3 = slice3scalars->GetPointer(0);
    }
  else
    {//get as float: cast is ok because this code is only executed for float type
    slice3scalars->GetData(0,sliceSize-1,0,numComp-1,floatScalars);
    slice3 = (T *) floatScalars->GetPointer(0);
    }

  if ( !slice2 || !slice3 )
    {
    vtkGenericWarningMacro(<< "Cannot allocate data!");
    return 0;
    }

  // Generate triangles and normals from slices
  for (k=0; k < (dims[2]-1); k++)
    {

    // swap things around
    if ( slice0scalars != NULL )
      {
      slice0scalars->Delete();
      }
    slice0scalars = slice1scalars;
    slice0 = slice1;
    slice1scalars = slice2scalars;
    slice1 = slice2;
    slice2scalars = slice3scalars;
    slice2 = slice3;
    if ( k < (dims[2]-2) )
      {
      if (debug)  vtkGenericWarningMacro(<< "  Slice# " << imageRange[0]+k+2);
      sp = reader->GetImage(imageRange[0]+k+2);
      slice3scalars = (S *) sp->GetPointData()->GetScalars()->GetData();
      if ( slice3scalars == NULL )
        {
        vtkGenericWarningMacro(<< "Can't read all the requested slices");
        goto PREMATURE_TERMINATION;
        }
      slice3scalars->Register(NULL);
      sp->Delete();
      if ( slice != NULL )
        {
        slice3 = slice3scalars->GetPointer(0);
        }
      else
        {//get as float
        slice3scalars->GetData(0,sliceSize-1,0,numComp-1,floatScalars);
        slice3 = (T *) floatScalars->GetPointer(0);
        }
      }

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
	  {
          if ( s[ii] >= value )
	    {
            index |= CASE_MASK[ii];
	    }
	  }

        if ( index == 0 || index == 255 ) // no surface
	  {
	  continue;
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

        //create gradients
        ComputePointGradient(i,j, k, dims, Spacing, grad[0],
                             slice0, slice1, slice2);
        ComputePointGradient(i+1,j, k, dims, Spacing, grad[1],
                             slice0, slice1, slice2);
        ComputePointGradient(i+1,j+1, k, dims, Spacing, grad[2],
                             slice0, slice1, slice2);
        ComputePointGradient(i,j+1, k, dims, Spacing, grad[3],
                             slice0, slice1, slice2);
        ComputePointGradient(i,j, k+1, dims, Spacing, grad[4],
                             slice1, slice2, slice3);
        ComputePointGradient(i+1,j, k+1, dims, Spacing, grad[5],
                             slice1, slice2, slice3);
        ComputePointGradient(i+1,j+1, k+1, dims, Spacing, grad[6],
                             slice1, slice2, slice3);
        ComputePointGradient(i,j+1, k+1, dims, Spacing, grad[7],
                             slice1, slice2, slice3);

	triCase = VTK_TRIANGLE_CASES::GetCases() + index;
        edge = triCase->edges;

        for ( ; edge[0] > -1; edge += 3 )
          {
          for (ii=0; ii<3; ii++) //insert triangle
            {
            vert = edges[edge[ii]];
            t = (value - s[vert[0]]) / (s[vert[1]] - s[vert[0]]);
            x1 = pts[vert[0]];
            x2 = pts[vert[1]];
            n1 = grad[vert[0]];
            n2 = grad[vert[1]];
            for (jj=0; jj<3; jj++)
              {
              point[jj] = x1[jj] + t * (x2[jj] - x1[jj]);
              point[jj+3] = n1[jj] + t * (n2[jj] - n1[jj]);
              if (point[jj] < xmin[jj] )
		{
		xmin[jj] = point[jj];
		}
              if (point[jj] > xmax[jj] )
		{
		xmax[jj] = point[jj];
		}
              }
            vtkMath::Normalize(point+3);
	    // swap bytes if necessary
	    vtkByteSwap::SwapWrite4BERange(point,6,outFP);
            }
          numTriangles++;
          }//for each triangle
        }//for i
      }//for j
    }//for k

  // Close things down
  PREMATURE_TERMINATION:

  fclose(outFP);
  if ( slice == NULL )
    {
    floatScalars->Delete();
    }
  if (slice0scalars && slice0scalars != slice1scalars)
    {
    slice0scalars->Delete();
    }
  if (slice3scalars && slice3scalars != slice2scalars)
    {
    slice3scalars->Delete();
    }
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
  float origin[3], Spacing[3];
  int numTriangles = 0;

  // check input/initalize
  vtkDebugMacro(<< "Executing slice cubes");
  if ( this->Reader == NULL )
   {
   vtkErrorMacro(<<"No reader specified...can't generate isosurface");
   return;
   }

  if ( this->FileName == NULL )
   {
   vtkErrorMacro(<<"No FileName specified...can't output isosurface");
   return;
   }

  if ( (outFP = fopen(this->FileName, "wb")) == NULL )
   {
   vtkErrorMacro(<<"Cannot open specified output file...");
   return;
   }

  // get image dimensions from the readers first slice
  this->Reader->GetImageRange(imageRange);
  tempStructPts = this->Reader->GetImage(imageRange[0]);
  tempStructPts->GetDimensions(dims);
  tempStructPts->GetOrigin(origin);
  tempStructPts->GetSpacing(Spacing);
  
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
    tempStructPts->Delete();
    return;
    }
  inScalars->Register(this);
  tempStructPts->Delete();

  if (inScalars->GetNumberOfComponents() == 1 )
    {
    switch (inScalars->GetDataType())
      {
      case VTK_CHAR:
	{
	vtkCharArray *scalars = (vtkCharArray *)inScalars->GetData();
	char *s = scalars->GetPointer(0);
	numTriangles = vtkSliceCubesContour(s,scalars,imageRange,dims,origin,
			       Spacing,this->Value,
			       xmin,xmax,outFP,this->Reader,this->Debug);
	}
      break;
      case VTK_UNSIGNED_CHAR:
	{
	vtkUnsignedCharArray *scalars = (vtkUnsignedCharArray *)inScalars->GetData();
	unsigned char *s = scalars->GetPointer(0);
	numTriangles = vtkSliceCubesContour(s,scalars,imageRange,dims,origin,
			       Spacing,this->Value,
			       xmin,xmax,outFP,this->Reader,this->Debug);
	}
      break;
      case VTK_SHORT:
	{
	vtkShortArray *scalars = (vtkShortArray *)inScalars->GetData();
	short *s = scalars->GetPointer(0);
	numTriangles = vtkSliceCubesContour(s,scalars,imageRange,dims,origin,
			       Spacing,this->Value,
			       xmin,xmax,outFP,this->Reader,this->Debug);
	}
      break;
      case VTK_UNSIGNED_SHORT:
	{
	vtkUnsignedShortArray *scalars = (vtkUnsignedShortArray *)inScalars->GetData();
	unsigned short *s = scalars->GetPointer(0);
	numTriangles = vtkSliceCubesContour(s,scalars,imageRange,dims,origin,
			       Spacing,this->Value,
			       xmin,xmax,outFP,this->Reader,this->Debug);
	}
      break;
      case VTK_INT:
	{
	vtkIntArray *scalars = (vtkIntArray *)inScalars->GetData();
	int *s = scalars->GetPointer(0);
	numTriangles = vtkSliceCubesContour(s,scalars,imageRange,dims,origin,
			       Spacing,this->Value,
			       xmin,xmax,outFP,this->Reader,this->Debug);
	}
      break;
      case VTK_UNSIGNED_INT:
	{
	vtkUnsignedIntArray *scalars = (vtkUnsignedIntArray *)inScalars->GetData();
	unsigned int *s = scalars->GetPointer(0);
	numTriangles = vtkSliceCubesContour(s,scalars,imageRange,dims,origin,
			       Spacing,this->Value,
			       xmin,xmax,outFP,this->Reader,this->Debug);
	}
      break;
      case VTK_LONG:
	{
	vtkLongArray *scalars = (vtkLongArray *)inScalars->GetData();
	long *s = scalars->GetPointer(0);
	numTriangles = vtkSliceCubesContour(s,scalars,imageRange,dims,origin,
			       Spacing,this->Value,
			       xmin,xmax,outFP,this->Reader,this->Debug);
	}
      break;
      case VTK_UNSIGNED_LONG:
	{
	vtkUnsignedLongArray *scalars = (vtkUnsignedLongArray *)inScalars->GetData();
	unsigned long *s = scalars->GetPointer(0);
	numTriangles = vtkSliceCubesContour(s,scalars,imageRange,dims,origin,
			       Spacing,this->Value,
			       xmin,xmax,outFP,this->Reader,this->Debug);
	}
      break;
      case VTK_FLOAT:
	{
	vtkFloatArray *scalars = (vtkFloatArray *)inScalars->GetData();
	float *s = scalars->GetPointer(0);
	numTriangles = vtkSliceCubesContour(s,scalars,imageRange,dims,origin,
			       Spacing,this->Value,
			       xmin,xmax,outFP,this->Reader,this->Debug);
	}
      break;
      case VTK_DOUBLE:
	{
	vtkDoubleArray *scalars = (vtkDoubleArray *)inScalars->GetData();
	double *s = scalars->GetPointer(0);
	numTriangles = vtkSliceCubesContour(s,scalars,imageRange,dims,origin,
			       Spacing,this->Value,
			       xmin,xmax,outFP,this->Reader,this->Debug);
	}
      break;
      }//switch
    }

  else //multiple components have to convert
    {
    vtkFloatArray *scalars = (vtkFloatArray *)inScalars->GetData();
    float *s = NULL; //clue to convert data to float
    numTriangles = vtkSliceCubesContour(s,scalars,imageRange,dims,origin,
			   Spacing,this->Value,
			   xmin,xmax,outFP,this->Reader,this->Debug);
    }

  inScalars->UnRegister(this);
  vtkDebugMacro(<<"Created: " << 3*numTriangles << " points, " 
                << numTriangles << " triangles");

  if ( this->LimitsFileName )
    {
    int i;
    float t;

    if ( (outFP = fopen(this->LimitsFileName, "wb")) == NULL )
      {
      vtkWarningMacro(<<"Sorry, couldn't write limits file...");
      }
    else
      {
      for (i=0; i<3; i++)
        {
        t = origin[i] + (dims[i] - 1)*Spacing[i];
	vtkByteSwap::SwapWrite4BERange(origin+i,1,outFP);
	// swap if neccessary
	vtkByteSwap::SwapWrite4BERange(&t,1,outFP);
        }
      for (i=0; i<3; i++)
        {
	vtkByteSwap::SwapWrite4BERange(xmin+i,1,outFP);
	vtkByteSwap::SwapWrite4BERange(xmax+i,1,outFP);
        }
      }
     fclose(outFP);
    }
}

void vtkSliceCubes::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Iso Value: " << this->Value << "\n";

  if ( this->Reader )
    {
    os << indent << "Reader:\n";
    this->Reader->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Reader: (none)\n";
    }

  os << indent << "File Name: " 
     << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "Limits File Name: " 
     << (this->LimitsFileName ? this->LimitsFileName : "(none)") << "\n";
}
