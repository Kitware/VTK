/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChairDisplay.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

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
#include <math.h>
#include "vtkChairDisplay.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkChairDisplay* vtkChairDisplay::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkChairDisplay");
  if(ret)
    {
    return (vtkChairDisplay*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkChairDisplay;
}




// Description:
// Construct object with initial range (0,1) and single contour value of
// 0.0. ComputeNormal is on, ComputeGradients is off and ComputeScalars is
// on.
vtkChairDisplay::vtkChairDisplay()
{
  this->Input = NULL;
  this->XNotchSize = 1;
  this->YNotchSize = 1;
  this->ZNotchSize = 1;
  this->TextureOutput = vtkStructuredPoints::New();
  this->TextureOutput->SetSource(this);
  this->Scalars = NULL;
}

vtkChairDisplay::~vtkChairDisplay()
{
  if (this->Scalars)
    {
    this->Scalars->Delete();
    this->Scalars = NULL;
    }
  if (this->Input)
    {
    this->Input->UnRegister(this);
    this->Input = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkChairDisplay::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkChairDisplay::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Inputs[0]);
}

void vtkChairDisplay::Update()
{
  if ( ! this->Input)
    {
    vtkErrorMacro("No Input");
    return;
    }
  
  // Make sure virtual getMTime method is called since subclasses will overload
  if ( this->GetMTime() > this->ExecuteTime ||
       this->Input->GetPipelineMTime() > this->ExecuteTime)
    {
    if (this->Output)
      {
      this->Output->Initialize();
      }
    if (this->TextureOutput)
      {
      this->TextureOutput->Initialize();
      }
    this->AbortExecute = 0;
    this->Progress = 0.0;
    
    // if the input changed then we have to completely recompute
    // the texturemap.
    if (this->Input->GetPipelineMTime() > this->ExecuteTime)
      {
      this->Execute(1);
      }
    else
      {
      this->Execute(0);
      }
    this->ExecuteTime.Modified();
    }
}

void vtkChairDisplay::GeneratePolyData(int *dimensions, float *origin, 
				       float *spacing, int p2x, int p2y,
				       vtkCellArray *polys, vtkPoints *points, 
				       vtkTCoords *tcoords)
{
  float TCXStarts[3];
  float TCYStarts[3];
  float TCXNotch[3];
  float TCXMids[3];
  float TCYMids[3];
  float TCYMids2[3];
  float TCXEnds[3];
  float TCYEnds[3];
  float TCYEnds2[3];

  float PointsX[17];
  float PointsY[17];
  float PointsZ[17];
  int i;
  
  for (i = 0; i < 7; i++)
    {
    PointsX[i] = origin[0] + (i%2)*spacing[0]*(dimensions[0]-1);
    PointsY[i] = origin[1] + ((i/2)%2)*spacing[1]*(dimensions[1]-1);
    PointsZ[i] = origin[2] + ((i/4)%2)*spacing[2]*(dimensions[2]-1);
    }

  for (i = 7; i < 14; i++)
    {
    PointsX[i] = origin[0] + 
      spacing[0]*(dimensions[0] - (i%2)*this->XNotchSize - 1);
    PointsY[i] = origin[1] + 
      spacing[1]*(dimensions[1] - (((i-5)/2)%2)*this->YNotchSize - 1);
    PointsZ[i] = origin[2] + 
      spacing[2]*(dimensions[2] - (((i-3)/4)%2)*this->ZNotchSize - 1);
    }

  PointsX[14] = origin[0] + 
    spacing[0]*(dimensions[0] - this->XNotchSize - 1);
  PointsY[14] = origin[1];
  PointsZ[14] = origin[2] + spacing[2]*(dimensions[2]-1);

  PointsX[15] = origin[0] + 
    spacing[0]*(dimensions[0] - this->XNotchSize - 1);
  PointsY[15] = origin[1] + spacing[1]*(dimensions[1]-1);
  PointsZ[15] = origin[2];

  PointsX[16] = origin[0] + spacing[0]*(dimensions[0] - 1);
  PointsY[16] = origin[1] + 
    spacing[1]*(dimensions[1] - this->YNotchSize - 1);
  PointsZ[16] = origin[2];
    
  //insert the twelve polygons
  for (i = 0; i < 12; i++)
    {
    polys->InsertNextCell(4);
    polys->InsertCellPoint(i*4);
    polys->InsertCellPoint(i*4+1);
    polys->InsertCellPoint(i*4+2);
    polys->InsertCellPoint(i*4+3);
    }
  
  // precalculate some useful values
  TCXStarts[0] = 0.0;
  TCXStarts[1] = dimensions[0]/(float)p2x;
  TCXStarts[2] = (dimensions[0] + dimensions[1])/(float)p2x;
  TCXMids[0] = (dimensions[0]-this->XNotchSize-1)/(float)p2x;
  TCXMids[1] = (dimensions[0] + dimensions[1]-this->YNotchSize-1)/(float)p2x;
  TCXMids[2] = (2.0*dimensions[0] + dimensions[1]-this->XNotchSize-1)/
    (float)p2x;
  TCXEnds[0] = (dimensions[0]-1)/(float)p2x;
  TCXEnds[1] = (dimensions[0] + dimensions[1]-1)/(float)p2x;
  TCXEnds[2] = (2.0*dimensions[0] + dimensions[1]-1)/(float)p2x;
  TCXNotch[0] = this->XNotchSize/(float)p2x;
  TCXNotch[1] = (dimensions[0] + this->YNotchSize)/(float)p2x;
  TCXNotch[2] = (dimensions[0] + dimensions[1]+this->XNotchSize)/(float)p2x;

  TCYStarts[0] = 0.0;
  TCYStarts[1] = this->MaxYZSize/(float)p2y;
  TCYStarts[2] = 2.0*this->MaxYZSize/(float)p2y;
  TCYMids[0] = (dimensions[2] - this->ZNotchSize - 1)/(float)p2y;
  TCYMids[1] = (dimensions[2] + this->MaxYZSize - this->ZNotchSize - 1)/
    (float)p2y;
  TCYMids[2] = (dimensions[2] + 2.0*this->MaxYZSize - this->ZNotchSize-1)/
    (float)p2y;
  TCYEnds[0] = (dimensions[2] - 1)/(float)p2y;
  TCYEnds[1] = (dimensions[2] + this->MaxYZSize - 1)/(float)p2y;
  TCYEnds[2] = (this->ZNotchSize + 2.0*this->MaxYZSize-1)/(float)p2y;

  TCYMids2[0] = (dimensions[1] - this->YNotchSize - 1)/(float)p2y;
  TCYMids2[1] = (dimensions[1] + this->MaxYZSize - this->YNotchSize - 1)/
    (float)p2y;
  TCYMids2[2] = (dimensions[1] + 2.0*this->MaxYZSize - this->YNotchSize -1)/
    (float)p2y;
  TCYEnds2[0] = (dimensions[1] - 1)/(float)p2y;
  TCYEnds2[1] = (dimensions[1] + this->MaxYZSize - 1)/(float)p2y;
  TCYEnds2[2] = (this->YNotchSize + 2.0*this->MaxYZSize-1)/(float)p2y;

  // XY plane
  points->InsertNextPoint(PointsX[0],PointsY[0],PointsZ[0]);
  points->InsertNextPoint(PointsX[2],PointsY[2],PointsZ[2]);
  points->InsertNextPoint(PointsX[3],PointsY[3],PointsZ[3]);
  points->InsertNextPoint(PointsX[1],PointsY[1],PointsZ[1]);
  tcoords->InsertNextTCoord(TCXStarts[2],TCYStarts[0],0);
  tcoords->InsertNextTCoord(TCXStarts[2],TCYEnds2[0],0);
  tcoords->InsertNextTCoord(TCXEnds[2],TCYEnds2[0],0);
  tcoords->InsertNextTCoord(TCXEnds[2],TCYStarts[0],0);

  // XZ Plane
  points->InsertNextPoint(PointsX[0],PointsY[0],PointsZ[0]);
  points->InsertNextPoint(PointsX[1],PointsY[1],PointsZ[1]);
  points->InsertNextPoint(PointsX[5],PointsY[5],PointsZ[5]);
  points->InsertNextPoint(PointsX[4],PointsY[4],PointsZ[4]);
  tcoords->InsertNextTCoord(TCXStarts[0],TCYStarts[0],0);
  tcoords->InsertNextTCoord(TCXEnds[0],TCYStarts[0],0);
  tcoords->InsertNextTCoord(TCXEnds[0],TCYEnds[0],0);
  tcoords->InsertNextTCoord(TCXStarts[0],TCYEnds[0],0);

  // YZ Plane
  points->InsertNextPoint(PointsX[0],PointsY[0],PointsZ[0]); 
  points->InsertNextPoint(PointsX[4],PointsY[4],PointsZ[4]);
  points->InsertNextPoint(PointsX[6],PointsY[6],PointsZ[6]);
  points->InsertNextPoint(PointsX[2],PointsY[2],PointsZ[2]);
  tcoords->InsertNextTCoord(TCXStarts[1],TCYStarts[0],0);
  tcoords->InsertNextTCoord(TCXStarts[1],TCYEnds[0],0);
  tcoords->InsertNextTCoord(TCXEnds[1],TCYEnds[0],0);
  tcoords->InsertNextTCoord(TCXEnds[1],TCYStarts[0],0);

  // XY2 plane
  points->InsertNextPoint(PointsX[4],PointsY[4],PointsZ[4]); 
  points->InsertNextPoint(PointsX[14],PointsY[14],PointsZ[14]);
  points->InsertNextPoint(PointsX[13],PointsY[13],PointsZ[13]);
  points->InsertNextPoint(PointsX[6],PointsY[6],PointsZ[6]);
  tcoords->InsertNextTCoord(TCXStarts[2],TCYStarts[1],0);
  tcoords->InsertNextTCoord(TCXMids[2],TCYStarts[1],0);
  tcoords->InsertNextTCoord(TCXMids[2],TCYEnds2[1],0);
  tcoords->InsertNextTCoord(TCXStarts[2],TCYEnds2[1],0);

  // XZ2 Plane
  points->InsertNextPoint(PointsX[2],PointsY[2],PointsZ[2]); 
  points->InsertNextPoint(PointsX[6],PointsY[6],PointsZ[6]);
  points->InsertNextPoint(PointsX[13],PointsY[13],PointsZ[13]);
  points->InsertNextPoint(PointsX[15],PointsY[15],PointsZ[15]);
  tcoords->InsertNextTCoord(TCXStarts[0],TCYStarts[1],0);
  tcoords->InsertNextTCoord(TCXStarts[0],TCYEnds[1],0);
  tcoords->InsertNextTCoord(TCXMids[0],TCYEnds[1],0);
  tcoords->InsertNextTCoord(TCXMids[0],TCYStarts[1],0);

  // YZ2 Plane
  points->InsertNextPoint(PointsX[1],PointsY[1],PointsZ[1]); 
  points->InsertNextPoint(PointsX[16],PointsY[16],PointsZ[16]);
  points->InsertNextPoint(PointsX[12],PointsY[12],PointsZ[12]);
  points->InsertNextPoint(PointsX[5],PointsY[5],PointsZ[5]);
  tcoords->InsertNextTCoord(TCXStarts[1],TCYStarts[1],0);
  tcoords->InsertNextTCoord(TCXMids[1],TCYStarts[1],0);
  tcoords->InsertNextTCoord(TCXMids[1],TCYEnds[1],0);
  tcoords->InsertNextTCoord(TCXStarts[1],TCYEnds[1],0);

  // XY3 plane
  points->InsertNextPoint(PointsX[14],PointsY[14],PointsZ[14]); 
  points->InsertNextPoint(PointsX[5],PointsY[5],PointsZ[5]);
  points->InsertNextPoint(PointsX[12],PointsY[12],PointsZ[12]);
  points->InsertNextPoint(PointsX[11],PointsY[11],PointsZ[11]);
  tcoords->InsertNextTCoord(TCXMids[2],TCYStarts[1],0);
  tcoords->InsertNextTCoord(TCXEnds[2],TCYStarts[1],0);
  tcoords->InsertNextTCoord(TCXEnds[2],TCYMids2[1],0);
  tcoords->InsertNextTCoord(TCXMids[2],TCYMids2[1],0);

  // XZ3 Plane
  points->InsertNextPoint(PointsX[15],PointsY[15],PointsZ[15]); 
  points->InsertNextPoint(PointsX[9],PointsY[9],PointsZ[9]);
  points->InsertNextPoint(PointsX[10],PointsY[10],PointsZ[10]);
  points->InsertNextPoint(PointsX[3],PointsY[3],PointsZ[3]);
  tcoords->InsertNextTCoord(TCXMids[0],TCYStarts[1],0);
  tcoords->InsertNextTCoord(TCXMids[0],TCYMids[1],0);
  tcoords->InsertNextTCoord(TCXEnds[0],TCYMids[1],0);
  tcoords->InsertNextTCoord(TCXEnds[0],TCYStarts[1],0);

  // YZ3 Plane
  points->InsertNextPoint(PointsX[16],PointsY[16],PointsZ[16]); 
  points->InsertNextPoint(PointsX[3],PointsY[3],PointsZ[3]);
  points->InsertNextPoint(PointsX[10],PointsY[10],PointsZ[10]);
  points->InsertNextPoint(PointsX[8],PointsY[8],PointsZ[8]);
  tcoords->InsertNextTCoord(TCXMids[1],TCYStarts[1],0);
  tcoords->InsertNextTCoord(TCXEnds[1],TCYStarts[1],0);
  tcoords->InsertNextTCoord(TCXEnds[1],TCYMids[1],0);
  tcoords->InsertNextTCoord(TCXMids[1],TCYMids[1],0);

  // XY4 plane
  points->InsertNextPoint(PointsX[7],PointsY[7],PointsZ[7]); 
  points->InsertNextPoint(PointsX[8],PointsY[8],PointsZ[8]);
  points->InsertNextPoint(PointsX[10],PointsY[10],PointsZ[10]);
  points->InsertNextPoint(PointsX[9],PointsY[9],PointsZ[9]);
  tcoords->InsertNextTCoord(TCXStarts[2],TCYStarts[2],0);
  tcoords->InsertNextTCoord(TCXNotch[2],TCYStarts[2],0);
  tcoords->InsertNextTCoord(TCXNotch[2],TCYEnds2[2],0);
  tcoords->InsertNextTCoord(TCXStarts[2],TCYEnds2[2],0);

  // XZ4 Plane
  points->InsertNextPoint(PointsX[7],PointsY[7],PointsZ[7]); 
  points->InsertNextPoint(PointsX[11],PointsY[11],PointsZ[11]);
  points->InsertNextPoint(PointsX[12],PointsY[12],PointsZ[12]);
  points->InsertNextPoint(PointsX[8],PointsY[8],PointsZ[8]);
  tcoords->InsertNextTCoord(TCXStarts[0],TCYStarts[2],0);
  tcoords->InsertNextTCoord(TCXStarts[0],TCYEnds[2],0);
  tcoords->InsertNextTCoord(TCXNotch[0],TCYEnds[2],0);
  tcoords->InsertNextTCoord(TCXNotch[0],TCYStarts[2],0);

  // YZ4 Plane
  points->InsertNextPoint(PointsX[7],PointsY[7],PointsZ[7]); 
  points->InsertNextPoint(PointsX[9],PointsY[9],PointsZ[9]);
  points->InsertNextPoint(PointsX[13],PointsY[13],PointsZ[13]);
  points->InsertNextPoint(PointsX[11],PointsY[11],PointsZ[11]);
  tcoords->InsertNextTCoord(TCXStarts[1],TCYStarts[2],0);
  tcoords->InsertNextTCoord(TCXNotch[1],TCYStarts[2],0);
  tcoords->InsertNextTCoord(TCXNotch[1],TCYEnds[2],0);
  tcoords->InsertNextTCoord(TCXStarts[1],TCYEnds[2],0);
}


//----------------------------------------------------------------------------
void vtkChairDisplay::Execute(int recomputeTexture)
{
  vtkImageData *inData = NULL;
  vtkPolyData *output = this->GetOutput();
  int *wholeExtent;
  float *origin;
  float *spacing;
  int dimensions[3];
  int p2x, p2y;
  int txsize, tysize;
  vtkScalars *scalars;
  
  
  vtkDebugMacro("Starting Execute Method");
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "No Input");
    return;
    }

  this->Input->UpdateImageInformation();

  // do we need to recompute the full texture ?
  if (recomputeTexture && this->Scalars)
    {
    this->Scalars->Delete();
    this->Scalars = NULL;
    }
  if (!this->Scalars || recomputeTexture)
    {
    this->Scalars = vtkScalars::New(this->Input->GetScalarType(),
			      this->Input->GetNumberOfScalarComponents());
    }
  scalars = this->Scalars;
  
  wholeExtent = this->Input->GetWholeExtent();
  origin = this->Input->GetOrigin();
  spacing = this->Input->GetSpacing();
  this->Input->GetDimensions(dimensions);

  // check the notch size
  if (this->XNotchSize >= dimensions[0] ||
      this->YNotchSize >= dimensions[1] ||
      this->ZNotchSize >= dimensions[2])
    {
    vtkWarningMacro("NotchSize is larger than available data!");
    return;
    }
  
  this->MaxYZSize = dimensions[2];
  if (dimensions[1] > this->MaxYZSize)
    {
    this->MaxYZSize = dimensions[1];
    }

  txsize = 2*dimensions[0] + dimensions[1];
  tysize = 3*this->MaxYZSize;

  p2x = 1; 
  while (p2x < txsize) 
     {
     p2x = p2x*2;
     }
  
  p2y = 1; 
  while (p2y < tysize) 
     {
     p2y = p2y*2;
     }

  this->TextureOutput->SetDimensions(p2x,p2y,1);
  scalars->SetNumberOfScalars(p2x*p2y);

  // Generate poly data
  vtkPoints *points = vtkPoints::New();
  vtkCellArray *polys = vtkCellArray::New();
  vtkTCoords *tcoords = vtkTCoords::New();
  
  this->GeneratePolyData(dimensions, origin, spacing, p2x, p2y,
			 polys, points, tcoords);
			
  if ( this->StartMethod )
    {
    (*this->StartMethod)(this->StartMethodArg);
    }

  if (recomputeTexture)
    {
    // Get the chunk from the input
    this->Input->SetUpdateExtent(wholeExtent[0],wholeExtent[1],
				 wholeExtent[2], wholeExtent[3],
				 wholeExtent[4], wholeExtent[4]);
    inData = this->Input->UpdateAndReturnData();
    this->GenerateTexture(inData, scalars, dimensions[0]+dimensions[1], 0,
			  dimensions[0], dimensions[1],p2x);
    
    this->Input->SetUpdateExtent(wholeExtent[0],wholeExtent[1],
				 wholeExtent[2], wholeExtent[2],
				 wholeExtent[4], wholeExtent[5]);
    inData = this->Input->UpdateAndReturnData();
    this->GenerateTexture(inData, scalars, 0, 0,
			  dimensions[0], dimensions[2],p2x);
    
    this->Input->SetUpdateExtent(wholeExtent[0],wholeExtent[0],
				 wholeExtent[2], wholeExtent[3],
				 wholeExtent[4], wholeExtent[5]);
    inData = this->Input->UpdateAndReturnData();
    this->GenerateTexture(inData, scalars, dimensions[0], 0,
			  dimensions[1], dimensions[2],p2x);
    
    // now the second set of planes
    this->Input->SetUpdateExtent(wholeExtent[0],wholeExtent[1],
				 wholeExtent[2], wholeExtent[3],
				 wholeExtent[5], wholeExtent[5]);
    inData = this->Input->UpdateAndReturnData();
    this->GenerateTexture(inData, scalars, dimensions[0]+dimensions[1], 
			  this->MaxYZSize,
			  dimensions[0], dimensions[1],p2x);
    
    this->Input->SetUpdateExtent(wholeExtent[0],wholeExtent[1],
				 wholeExtent[3], wholeExtent[3],
				 wholeExtent[4], wholeExtent[5]);
    inData = this->Input->UpdateAndReturnData();
    this->GenerateTexture(inData, scalars, 0, this->MaxYZSize,
			  dimensions[0], dimensions[2],p2x);
    
    this->Input->SetUpdateExtent(wholeExtent[1],wholeExtent[1],
				 wholeExtent[2], wholeExtent[3],
				 wholeExtent[4], wholeExtent[5]);
    inData = this->Input->UpdateAndReturnData();
    this->GenerateTexture(inData, scalars, dimensions[0], this->MaxYZSize,
			  dimensions[1], dimensions[2],p2x);
    }
  
  // now the third set of planes if the notch exists
  if (this->XNotchSize*this->YNotchSize*this->ZNotchSize > 0)
    {
    this->Input->SetUpdateExtent(wholeExtent[1] - this->XNotchSize + 1, 
				 wholeExtent[1],
				 wholeExtent[3] - this->YNotchSize + 1, 
				 wholeExtent[3],
				 wholeExtent[5] - this->ZNotchSize + 1, 
				 wholeExtent[5] - this->ZNotchSize + 1);
    inData = this->Input->UpdateAndReturnData();
    this->GenerateTexture(inData, scalars, dimensions[0]+dimensions[1], 
			  2*this->MaxYZSize,
			  this->XNotchSize, this->YNotchSize,p2x);
    
    this->Input->SetUpdateExtent(wholeExtent[1] - this->XNotchSize + 1, 
				 wholeExtent[1],
				 wholeExtent[3] - this->YNotchSize + 1, 
				 wholeExtent[3] - this->YNotchSize + 1,
				 wholeExtent[5] - this->ZNotchSize + 1, 
				 wholeExtent[5]);
    inData = this->Input->UpdateAndReturnData();
    this->GenerateTexture(inData, scalars, 0, 2*this->MaxYZSize,
			  this->XNotchSize, this->ZNotchSize, p2x);
    
    this->Input->SetUpdateExtent(wholeExtent[1] - this->XNotchSize + 1, 
				 wholeExtent[1] - this->XNotchSize + 1,
				 wholeExtent[3] - this->YNotchSize + 1, 
				 wholeExtent[3],
				 wholeExtent[5] - this->ZNotchSize + 1, 
				 wholeExtent[5]);
    inData = this->Input->UpdateAndReturnData();
    this->GenerateTexture(inData, scalars, dimensions[0], 2*this->MaxYZSize,
			  this->YNotchSize, this->ZNotchSize, p2x);
    }
  
  if ( !this->AbortExecute ) 
    {
    this->UpdateProgress(1.0);
    }
  
  if ( this->EndMethod ) 
    {
    (*this->EndMethod)(this->EndMethodArg);
    }
  
  if (this->Input->ShouldIReleaseData())
    {
    this->Input->ReleaseData();
    }
  
  output->SetPoints(points);
  points->Delete();
  
  output->GetPointData()->SetTCoords(tcoords);
  tcoords->Delete();
  
  output->SetPolys(polys);
  polys->Delete();

  this->TextureOutput->GetPointData()->SetScalars(scalars);
  // scalars get deleted by the destructor
}





//----------------------------------------------------------------------------
template <class T>
static void vtkChairDisplayCopy(vtkChairDisplay *self,
                                vtkImageData *inData, T *ptr,
                                vtkScalars *scalars, int xstart,
                                int ystart, int xsize, int ysize,
                                int p2x)
{
  int idx0, idx1, idx2;
  int inc0, inc1, inc2;
  T *outPtr;
  int count;
  int numComp;
  int MaxX, MaxY, MaxZ;
  int *uExtent;
  
  // Get information to loop through images.
  numComp = inData->GetNumberOfScalarComponents();
  
  uExtent = self->GetInput()->GetUpdateExtent();
  outPtr = (T *)(scalars->GetVoidPointer(0));
  inData->GetContinuousIncrements(self->GetInput()->GetUpdateExtent(),
                                  inc0, inc1, inc2);

  outPtr = outPtr + (xstart + ystart*p2x)*numComp;
  MaxX = (uExtent[1] - uExtent[0] + 1)*numComp;
  MaxY = uExtent[3] - uExtent[2] + 1;
  MaxZ = uExtent[5] - uExtent[4] + 1;

  count = 0;
  for (idx2 = 0; idx2 < MaxZ; idx2++)
    {    
    for (idx1 = 0; idx1 < MaxY; ++idx1)
      {
      for (idx0 = 0; idx0 < MaxX; ++idx0)
        {
        *outPtr = *ptr;
	outPtr++;
        count++;
	ptr++;
	}
      if (count >= xsize)
        {
        count = 0;
        outPtr = outPtr + (p2x-xsize)*numComp;
        }
      ptr += inc1;
      }
    ptr += inc2;
    }
}



//----------------------------------------------------------------------------
// This method calls the proper templade function.
void vtkChairDisplay::GenerateTexture(vtkImageData *inData, 
                                      vtkScalars *scalars,
                                      int xstart, int ystart,
                                      int xsize, int ysize, int p2x)
{
  void *ptr = inData->GetScalarPointer();
  
  switch (inData->GetScalarType())
    {
    case VTK_FLOAT:
      vtkChairDisplayCopy(this, inData, (float *)(ptr),
                          scalars, xstart, ystart, xsize, ysize, p2x);
      break;
    case VTK_INT:
      vtkChairDisplayCopy(this, inData, (int *)(ptr),
                          scalars, xstart, ystart, xsize, ysize, p2x);
      break;
    case VTK_SHORT:
       vtkChairDisplayCopy(this, inData, (short *)(ptr),
                           scalars, xstart, ystart, xsize, ysize, p2x);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkChairDisplayCopy(this, inData, (unsigned short *)(ptr), 
                          scalars, xstart, ystart, xsize, ysize, p2x);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkChairDisplayCopy(this, inData, (unsigned char *)(ptr),
                          scalars, xstart, ystart, xsize, ysize, p2x);
      break;
    default:
      cerr << "Copy: Unknown output ScalarType";
      return;
    }
}

//----------------------------------------------------------------------------
void vtkChairDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataSource::PrintSelf(os,indent);

  if ( this->Input )
    {
    os << indent << "Input: " << this->Input << "\n";
    }
  else
    {
    os << indent << "Input: (none)\n";
    }

  os << indent << "XNotchSize: " << this->XNotchSize << "\n";
  os << indent << "YNotchSize: " << this->YNotchSize << "\n";
  os << indent << "ZNotchSize: " << this->ZNotchSize << "\n";

}

