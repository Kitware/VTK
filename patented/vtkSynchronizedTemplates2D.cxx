/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSynchronizedTemplates2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$



Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

    THIS CLASS IS PATENT PENDING.

    Application of this software for commercial purposes requires 
    a license grant from Kitware. Contact:
        Ken Martin
        Kitware
        469 Clifton Corporate Parkway,
        Clifton Park, NY 12065
        Phone:1-518-371-3971 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
#include "vtkSynchronizedTemplates2D.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkSynchronizedTemplates2D* vtkSynchronizedTemplates2D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSynchronizedTemplates2D");
  if(ret)
    {
    return (vtkSynchronizedTemplates2D*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkSynchronizedTemplates2D;
}




// Description:
// Construct object with initial scalar range (0,1) and single contour value
// of 0.0. The ImageRange are set to extract the first k-plane.
vtkSynchronizedTemplates2D::vtkSynchronizedTemplates2D()
{
  this->ContourValues = vtkContourValues::New();
}

vtkSynchronizedTemplates2D::~vtkSynchronizedTemplates2D()
{
  this->ContourValues->Delete();
}

// Description:
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
unsigned long vtkSynchronizedTemplates2D::GetMTime()
{
  unsigned long mTime=this->vtkStructuredPointsToPolyDataFilter::GetMTime();
  unsigned long mTime2=this->ContourValues->GetMTime();

  mTime = ( mTime2 > mTime ? mTime2 : mTime );
  return mTime;
}


//
// Contouring filter specialized for images
//
template <class T>
static void ContourImage(vtkSynchronizedTemplates2D *self,
			 T *scalars, vtkPoints *newPts,
			 vtkScalars *newScalars, vtkCellArray *lines)
{
  int xdim = self->GetInput()->GetDimensions()[0];
  int ydim = self->GetInput()->GetDimensions()[1];
  float *values = self->GetValues();
  int numContours = self->GetNumberOfContours();
  T *inPtr;
  int XMax, YMax;
  float x[3];
  float *origin = self->GetInput()->GetOrigin();
  float *spacing = self->GetInput()->GetSpacing();
  float y, t;
  int *isect1Ptr, *isect2Ptr;
  int ptIds[2];
  int *tablePtr;
  int v0, v1, v2;
  int idx, vidx;
  float s0, s1, s2, value;
  int i, j;
  
  int lineCases[64];
  
  // setup the table entries
  for (i = 0; i < 64; i++)
    {
    lineCases[i] = -1;
    }
  
  lineCases[12] = 3;
  lineCases[13] = xdim*2;

  lineCases[20] = 1;
  lineCases[21] = xdim*2;

  lineCases[24] = 1;
  lineCases[25] = 3;

  lineCases[36] = 0;
  lineCases[37] = xdim*2;

  lineCases[40] = 0;
  lineCases[41] = 3;

  lineCases[48] = 0;
  lineCases[49] = 1;

  lineCases[60] = 0;
  lineCases[61] = 1;
  lineCases[62] = 3;
  lineCases[63] = xdim*2;
    
  XMax = xdim - 1;
  YMax = ydim - 1;
  
  // allocate storage arrays
  int *isect1 = new int [xdim*4];
  isect1[xdim*2-2] = -1;
  isect1[xdim*2-1] = -1;
  isect1[xdim*4-2] = -1;
  isect1[xdim*4-1] = -1;

  // find the default z value
  x[2] = origin[2];
  
  // for each contour
  for (vidx = 0; vidx < numContours; vidx++)
    {
    lineCases[13] = xdim*2;
    lineCases[21] = xdim*2;
    lineCases[37] = xdim*2;
    lineCases[63] = xdim*2;
    
    value = values[vidx];
    inPtr = scalars;
    
    // Traverse all pixel cells, generating line segements using templates
    for (j = 0; j < YMax; j++)
      {
      // set the y coordinate
      y = origin[1] + j*spacing[1];
      // first compute the intersections
      s1 = *inPtr;
      
      // swap the buffers
      if (j%2)
	{
	lineCases[13] = xdim*2;
	lineCases[21] = xdim*2;
	lineCases[37] = xdim*2;
	lineCases[63] = xdim*2;
	isect1Ptr = isect1;
	isect2Ptr = isect1 + xdim*2;
	}
      else
	{
	lineCases[13] = -xdim*2;
	lineCases[21] = -xdim*2;
	lineCases[37] = -xdim*2;
	lineCases[63] = -xdim*2;
	isect1Ptr = isect1 + xdim*2;
	isect2Ptr = isect1;
	}
      
      for (i = 0; i < XMax; i++)
	{
	s0 = s1;
	s1 = *(inPtr + 1);
	// compute in/out for verts
	v0 = (s0 < value ? 0 : 1);
	v1 = (s1 < value ? 0 : 1);
	if (v0 ^ v1)
	  {
	  t = (value - s0) / (s1 - s0);
	  x[0] = origin[0] + spacing[0]*(i+t);
	  x[1] = y;
	  *isect2Ptr = newPts->InsertNextPoint(x);
	  newScalars->InsertNextScalar(value);
	  }
	else
	  {
	  *isect2Ptr = -1;
	  }
	if (j < YMax)
	  {
	  s2 = *(inPtr + xdim);
	  v2 = (s2 < value ? 0 : 1);
	  if (v0 ^ v2)
	    {
	    t = (value - s0) / (s2 - s0);
	    x[0] = origin[0] + spacing[0]*i;
	    x[1] = y + spacing[1]*t;
	    *(isect2Ptr + 1) = newPts->InsertNextPoint(x);
	    newScalars->InsertNextScalar(value);
	    }
	  else
	    {
	    *(isect2Ptr + 1) = -1;
	    }
	  }
	
	if (j > 0)
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
	    lines->InsertNextCell(2,ptIds);
	  tablePtr++;
	    }
	  else
	    {
	    tablePtr += 2;
	    }
	  if (*tablePtr != -1)
	    {
	    ptIds[0] = *(isect1Ptr + *tablePtr);
	    tablePtr++;
	    ptIds[1] = *(isect1Ptr + *tablePtr);
	    lines->InsertNextCell(2,ptIds);
	    }
	  }
	inPtr++;
	isect2Ptr += 2;
	isect1Ptr += 2;
	}
      // now compute the last column, use s2 since it is around
      if (j < YMax)
	{
	s2 = *(inPtr + xdim);
	v2 = (s2 < value ? 0 : 1);
	if (v1 ^ v2)
	  {
	  t = (value - s1) / (s2 - s1);
	  x[0] = origin[0] + spacing[0]*XMax;
	  x[1] = y + spacing[1]*t;
	  *(isect2Ptr + 1) = newPts->InsertNextPoint(x);
	  newScalars->InsertNextScalar(value);
	  }
	else
	  {
	  *(isect2Ptr + 1) = -1;
	  }
	}
      inPtr++;
      }
    }

  delete [] isect1;
}

//
// Contouring filter specialized for images (or slices from images)
//
void vtkSynchronizedTemplates2D::Execute()
{
  vtkStructuredPoints *input= this->GetInput();
  vtkPointData *pd = input->GetPointData();
  vtkPoints *newPts;
  vtkCellArray *newLines;
  vtkScalars *inScalars = pd->GetScalars(), *newScalars;
  vtkPolyData *output = this->GetOutput();
  int *dims = this->GetInput()->GetDimensions();
  int dataSize, estimatedSize;
  

  vtkDebugMacro(<< "Executing 2D structured contour");
  
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

  if ( input->GetDataDimension() != 2 )
    {
    vtkErrorMacro(<<"2D structured contours requires 2D data");
    return;
    }

  //
  // Allocate necessary objects
  //
  estimatedSize = (int) (sqrt((double)dims[0]*dims[1]));
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
  if (inScalars->GetNumberOfComponents() == 1 )
    {
    switch (inScalars->GetDataType())
      {
      case VTK_CHAR:
	{
	char *scalars = ((vtkCharArray *)inScalars->GetData())->GetPointer(0);
	newScalars = vtkScalars::New(VTK_CHAR,1);
	newScalars->Allocate(5000,25000);
	ContourImage(this, scalars, newPts, newScalars, newLines);
	}
      break;
      case VTK_UNSIGNED_CHAR:
	{
	unsigned char *scalars = ((vtkUnsignedCharArray *)inScalars->GetData())->GetPointer(0);
	newScalars = vtkScalars::New(VTK_UNSIGNED_CHAR,1);
	newScalars->Allocate(5000,25000);
	ContourImage(this, scalars, newPts, newScalars, newLines);
	}
      break;
      case VTK_SHORT:
	{
	short *scalars = ((vtkShortArray *)inScalars->GetData())->GetPointer(0);
	newScalars = vtkScalars::New(VTK_SHORT,1);
	newScalars->Allocate(5000,25000);
	ContourImage(this, scalars, newPts, newScalars, newLines);
	}
      break;
      case VTK_UNSIGNED_SHORT:
	{
	unsigned short *scalars = ((vtkUnsignedShortArray *)inScalars->GetData())->GetPointer(0);
	newScalars = vtkScalars::New(VTK_UNSIGNED_SHORT,1);
	newScalars->Allocate(5000,25000);
	ContourImage(this, scalars, newPts, newScalars, newLines);
	}
      break;
      case VTK_INT:
	{
	int *scalars = ((vtkIntArray *)inScalars->GetData())->GetPointer(0);
	newScalars = vtkScalars::New(VTK_INT,1);
	newScalars->Allocate(5000,25000);
	ContourImage(this, scalars, newPts, newScalars, newLines);
	}
      break;
      case VTK_UNSIGNED_INT:
	{
	unsigned int *scalars = ((vtkUnsignedIntArray *)inScalars->GetData())->GetPointer(0);
	newScalars = vtkScalars::New(VTK_UNSIGNED_INT,1);
	newScalars->Allocate(5000,25000);
	ContourImage(this, scalars, newPts, newScalars, newLines);
	}
      break;
      case VTK_LONG:
	{
	long *scalars = ((vtkLongArray *)inScalars->GetData())->GetPointer(0);
	newScalars = vtkScalars::New(VTK_LONG,1);
	newScalars->Allocate(5000,25000);
	ContourImage(this, scalars, newPts, newScalars, newLines);
	}
      break;
      case VTK_UNSIGNED_LONG:
	{
	unsigned long *scalars = ((vtkUnsignedLongArray *)inScalars->GetData())->GetPointer(0);
	newScalars = vtkScalars::New(VTK_UNSIGNED_LONG,1);
	newScalars->Allocate(5000,25000);
	ContourImage(this, scalars, newPts, newScalars, newLines);
	}
      break;
      case VTK_FLOAT:
	{
	float *scalars = ((vtkFloatArray *)inScalars->GetData())->GetPointer(0);
	newScalars = vtkScalars::New(VTK_FLOAT,1);
	newScalars->Allocate(5000,25000);
	ContourImage(this, scalars, newPts, newScalars, newLines);
	}
      break;
      case VTK_DOUBLE:
	{
	double *scalars = ((vtkDoubleArray *)inScalars->GetData())->GetPointer(0);
	newScalars = vtkScalars::New(VTK_DOUBLE,1);
	newScalars->Allocate(5000,25000);
	ContourImage(this, scalars, newPts, newScalars, newLines);
	}
      break;
      }//switch
    }

  else //multiple components - have to convert
    {
    vtkScalars *image = vtkScalars::New();
    image->Allocate(dataSize);
    inScalars->GetScalars(0,dataSize,image);
    newScalars = vtkScalars::New(VTK_FLOAT);
    newScalars->Allocate(5000,25000);
    float *scalars = ((vtkFloatArray *)image->GetData())->GetPointer(0);
    ContourImage(this, scalars, newPts, newScalars, newLines);
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

  output->Squeeze();
}

void vtkSynchronizedTemplates2D::PrintSelf(vtkOstream& os, vtkIndent indent)
{
  vtkStructuredPointsToPolyDataFilter::PrintSelf(os,indent);

  this->ContourValues->PrintSelf(os,indent);
}


