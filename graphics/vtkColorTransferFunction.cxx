/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorTransferFunction.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkColorTransferFunction.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkColorTransferFunction* vtkColorTransferFunction::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkColorTransferFunction");
  if(ret)
    {
    return (vtkColorTransferFunction*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkColorTransferFunction;
}




// Construct a new vtkColorTransferFunction with default values
vtkColorTransferFunction::vtkColorTransferFunction()
{
  this->Red = vtkPiecewiseFunction::New();
  this->Green = vtkPiecewiseFunction::New();
  this->Blue = vtkPiecewiseFunction::New();
  
  this->ColorValue[0] = 0;
  this->ColorValue[1] = 0;
  this->ColorValue[2] = 0;

  this->Range[0] = 0;
  this->Range[1] = 0;

  this->Clamping = 0;
}

// Destruct a vtkColorTransferFunction
vtkColorTransferFunction::~vtkColorTransferFunction()
{
  this->Red->Delete();
  this->Red = NULL;
  this->Green->Delete();
  this->Green = NULL;
  this->Blue->Delete();
  this->Blue = NULL;  
}

unsigned long int vtkColorTransferFunction::GetMTime()
{
  unsigned long mTime=this->vtkScalarsToColors::GetMTime();
  unsigned long time;

  if ( this->Red != NULL )
    {
    time = this->Red->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  if ( this->Green != NULL )
    {
    time = this->Green->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  if ( this->Blue != NULL )
    {
    time = this->Blue->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

// Returns the sum of the number of function points used to specify 
// the three independent functions (R,G,B)
int vtkColorTransferFunction::GetTotalSize()
{
  int  size;

  // Sum all three function sizes
  size = this->Red->GetSize() +
         this->Green->GetSize() +
         this->Blue->GetSize();

  return( size );
}

// Add a point to the red function
void vtkColorTransferFunction::AddRedPoint( float x, float r )
{
  this->Red->AddPoint( x, r );

  this->UpdateRange();
}

// Add a point to the green function
void vtkColorTransferFunction::AddGreenPoint( float x, float g )
{
  this->Green->AddPoint( x, g );

  this->UpdateRange();
}

// Add a point to the blue function
void vtkColorTransferFunction::AddBluePoint( float x, float b )
{
  this->Blue->AddPoint( x, b );

  this->UpdateRange();
}

// Add a point to all three functions (RGB)
void vtkColorTransferFunction::AddRGBPoint( float x, float r,
     float g, float b )
{
  this->Red->AddPoint( x, r );
  this->Green->AddPoint( x, g );
  this->Blue->AddPoint( x, b );

  this->UpdateRange();
}

// Remove a point from the red function
void vtkColorTransferFunction::RemoveRedPoint( float x )
{
  this->Red->RemovePoint( x );

  this->UpdateRange();
}

// Remove a point from the green function
void vtkColorTransferFunction::RemoveGreenPoint( float x )
{
  this->Green->RemovePoint( x );

  this->UpdateRange();
}

// Remove a point from the blue function
void vtkColorTransferFunction::RemoveBluePoint( float x )
{
  this->Blue->RemovePoint( x );

  this->UpdateRange();
}

// Remove a point from all three functions (RGB)
void vtkColorTransferFunction::RemoveRGBPoint( float x )
{
  this->Red->RemovePoint( x );
  this->Green->RemovePoint( x );
  this->Blue->RemovePoint( x );

  this->UpdateRange();
}

// Remove all points from all three functions (RGB)
void vtkColorTransferFunction::RemoveAllPoints()
{
  this->Red->RemoveAllPoints();
  this->Green->RemoveAllPoints();
  this->Blue->RemoveAllPoints();

  this->UpdateRange();
}

// Add a line to the red function
void vtkColorTransferFunction::AddRedSegment( float x1, float r1,
     float x2, float r2 )
{
  this->Red->AddSegment( x1, r1, x2, r2 );

  this->UpdateRange();
}

// Add a line to the green function
void vtkColorTransferFunction::AddGreenSegment( float x1, float g1,
     float x2, float g2 )
{
  this->Green->AddSegment( x1, g1, x2, g2 );

  this->UpdateRange();
}

// Add a line to the blue function
void vtkColorTransferFunction::AddBlueSegment( float x1, float b1,
     float x2, float b2 )
{
  this->Blue->AddSegment( x1, b1, x2, b2 );

  this->UpdateRange();
}

// Add a line to all three functions (RGB)
void vtkColorTransferFunction::AddRGBSegment( float x1, float r1, 
        float g1, float b1, float x2, float r2, float g2, float b2 )
{
  this->Red->AddSegment( x1, r1, x2, r2 );
  this->Green->AddSegment( x1, g1, x2, g2 );
  this->Blue->AddSegment( x1, b1, x2, b2 );

  this->UpdateRange();
}

// Returns the RGB color evaluated at the specified location
unsigned char *vtkColorTransferFunction::MapValue( float x )
{
  this->ColorValue2[0] = (unsigned char)
    (255.0*this->Red->GetValue( x ));
  this->ColorValue2[1] = (unsigned char)
    (255.0*this->Green->GetValue( x ));
  this->ColorValue2[2] = (unsigned char)
    (255.0*this->Blue->GetValue( x ));
  this->ColorValue2[3] = 1;
  return( this->ColorValue2 );
}

// Returns the RGB color evaluated at the specified location
float *vtkColorTransferFunction::GetValue( float x )
{
  this->ColorValue[0] = this->Red->GetValue( x );
  this->ColorValue[1] = this->Green->GetValue( x );
  this->ColorValue[2] = this->Blue->GetValue( x );

  return( this->ColorValue );
}

// Updates the min/max range for all three functions (RGB)
void vtkColorTransferFunction::UpdateRange()
{
  float *red_range, *green_range, *blue_range;

  red_range   = this->Red->GetRange();
  green_range = this->Green->GetRange();
  blue_range  = this->Blue->GetRange();

  if( red_range[0] < this->Range[0] )
    {
    this->Range[0] = red_range[0];
    }

  if( green_range[0] < this->Range[0] )
    {
    this->Range[0] = green_range[0];
    }

  if( blue_range[0] < this->Range[0] )
    {
    this->Range[0] = blue_range[0];
    }

  if( red_range[1] > this->Range[1] )
    {
    this->Range[1] = red_range[1];
    }

  if( green_range[1] > this->Range[1] )
    {
    this->Range[1] = green_range[1];
    }

  if( blue_range[1] > this->Range[1] )
    {
    this->Range[1] = blue_range[1];
    }
  
  this->Modified();
}

// Returns the min/max range for all three functions
float *vtkColorTransferFunction::GetRange()
{
  return( this->Range );
}

// Returns a table of RGB colors at regular intervals along the function
void vtkColorTransferFunction::GetTable( float x1, float x2, 
					 int size, float* table )
{
  if( x1 == x2 )
    {
    return;
    }

  if (size > 0)
    {
    this->Red->GetTable(x1, x2, size, &(table[0]), 3);
    this->Green->GetTable(x1, x2, size, &(table[1]), 3);
    this->Blue->GetTable(x1, x2, size, &(table[2]), 3);
    }
}

void vtkColorTransferFunction::BuildFunctionFromTable( float x1, float x2,
						       int size, float *table)
{
  this->Red->BuildFunctionFromTable(x1, x2, size, &(table[0]), 3);
  this->Green->BuildFunctionFromTable(x1, x2, size, &(table[1]), 3);
  this->Blue->BuildFunctionFromTable(x1, x2, size, &(table[2]), 3);

  this->Range[0] = x1;
  this->Range[1] = x2;

  this->Modified();
}


// Print method for vtkColorTransferFunction
void vtkColorTransferFunction::PrintSelf(vtkOstream& os, vtkIndent indent)
{
  vtkScalarsToColors::PrintSelf(os, indent);

  os << indent << "Color Transfer Function Total Points: " << this->GetTotalSize() << "\n";

  os << indent << "Red Transfer Function: ";
  os << this->Red << "\n";

  os << indent << "Green Transfer Function: ";
  os << this->Green << "\n";

  os << indent << "Blue Transfer Function: ";
  os << this->Blue << "\n";

}

// Sets the clamping for each of the R,G,B transfer functions
void vtkColorTransferFunction::SetClamping(int val) {
	this->Clamping = val;
	this->Red->SetClamping(this->Clamping);
	this->Green->SetClamping(this->Clamping);
	this->Blue->SetClamping(this->Clamping);
}

// Gets the clamping value
int vtkColorTransferFunction::GetClamping() {
  return this->Clamping;
}

void vtkColorTransferFunction::DeepCopy( vtkColorTransferFunction *f )
{
  this->Red->DeepCopy(f->GetRedFunction());
  this->Green->DeepCopy(f->GetGreenFunction());
  this->Blue->DeepCopy(f->GetBlueFunction());
  
  this->Clamping     = f->Clamping;
  this->UpdateRange();
}

// accelerate the mapping by copying the data in 32-bit chunks instead
// of 8-bit chunks
template<class T>
static void 
vtkColorTransferFunctionMapDataClamp(vtkColorTransferFunction *self, 
				     T *input, 
				     unsigned char *output, 
				     int length, int inIncr, int outFormat)
{
  float findx;
  int i = length;
  vtkPiecewiseFunction *R, *G, *B;
  R = self->GetRedFunction();
  G = self->GetGreenFunction();
  B = self->GetBlueFunction();
  float *RRange = R->GetRange();
  float *GRange = G->GetRange();
  float *BRange = B->GetRange();
  float *RFunc = R->GetDataPointer();
  float *GFunc = G->GetDataPointer();
  float *BFunc = B->GetDataPointer();
  int RSize = R->GetSize();
  int GSize = G->GetSize();
  int BSize = B->GetSize();
  
  int   i1, i2;
  float x1, y1;	// Point before x
  float x2, y2;	// Point after x

  float slope;
  float value;

  R->Update();
  G->Update();
  B->Update();

  if(RSize == 0  || GSize == 0 || BSize == 0)
    {
    vtkGenericWarningMacro("Transfer Function Has No Points!");
    return;
    }

  while (--i >= 0) 
    {
    findx = (float) *input;

    // do red
    if( findx < RRange[0] ) 
      {
      *output = (unsigned char)(255*RFunc[0]);
      }
    else if( findx > RRange[1] )
      {
      *output = (unsigned char)(255*RFunc[(RSize-1)*2]);
      }
    else
      {
      i2 = 0;
      x2 = RFunc[0];
      y2 = RFunc[1];
      
      while( (x2 < findx) && (i2 < RSize) )
        {
        i2 += 1;
        x2 = RFunc[(i2*2)];
        y2 = RFunc[(i2*2+1)];
        }
      
      // Check if we have found the exact point
      if( x2 == findx )
        {
        *output++ = (unsigned char)(255*RFunc[(i2*2 + 1)]);
        }
      else
        {
        i1 = i2 - 1;
        x1 = RFunc[(i1*2)];
        y1 = RFunc[(i1*2 +1)];
        slope = (y2-y1)/(x2-x1);
        value = y1 + slope*(findx-x1);
        
        *output++ = (unsigned char)(255*value);
        }
      }
    
    if (outFormat == VTK_RGB || outFormat == VTK_RGBA)
      {
      // do green
      if( findx < GRange[0] ) 
        {
        *output = (unsigned char)(255*GFunc[0]);
        }
      else if( findx > GRange[1] )
        {
        *output = (unsigned char)(255*GFunc[(GSize-1)*2]);
        }
      else
        {
        i2 = 0;
        x2 = GFunc[0];
        y2 = GFunc[1];
      
        while( (x2 < findx) && (i2 < GSize) )
          {
          i2 += 1;
          x2 = GFunc[(i2*2)];
          y2 = GFunc[(i2*2+1)];
          }
      
        // Check if we have found the exact point
        if( x2 == findx )
          {
          *output++ = (unsigned char)(255*GFunc[(i2*2 + 1)]);
          }
        else
          {
          i1 = i2 - 1;
          x1 = GFunc[(i1*2)];
          y1 = GFunc[(i1*2 +1)];
          slope = (y2-y1)/(x2-x1);
          value = y1 + slope*(findx-x1);
          
          *output++ = (unsigned char)(255*value);
          }
        }
      
      // do blue
      if( findx < BRange[0] ) 
        {
        *output = (unsigned char)(255*BFunc[0]);
        }
      else if( findx > BRange[1] )
        {
        *output = (unsigned char)(255*BFunc[(BSize-1)*2]);
        }
      else
        {
        i2 = 0;
        x2 = BFunc[0];
        y2 = BFunc[1];
      
        while( (x2 < findx) && (i2 < BSize) )
          {
          i2 += 1;
          x2 = BFunc[(i2*2)];
          y2 = BFunc[(i2*2+1)];
          }
        
        // Check if we have found the exact point
        if( x2 == findx )
          {
          *output++ = (unsigned char)(255*BFunc[(i2*2 + 1)]);
          }
        else
          {
          i1 = i2 - 1;
          x1 = BFunc[(i1*2)];
          y1 = BFunc[(i1*2 +1)];
          slope = (y2-y1)/(x2-x1);
          value = y1 + slope*(findx-x1);
          
          *output++ = (unsigned char)(255*value);
          }
        }
      }
    
    if (outFormat == VTK_RGBA || outFormat == VTK_LUMINANCE_ALPHA)
      {
      *output++ = 255;
      }
    input += inIncr;
    }
}

// accelerate the mapping by copying the data in 32-bit chunks instead
// of 8-bit chunks
template<class T>
static void 
vtkColorTransferFunctionMapDataNoClamp(vtkColorTransferFunction *self, 
				       T *input, 
				       unsigned char *output, 
				       int length, int inIncr, int outFormat)
{
  float findx;
  int i = length;
  vtkPiecewiseFunction *R, *G, *B;
  R = self->GetRedFunction();
  G = self->GetGreenFunction();
  B = self->GetBlueFunction();
  float *RRange = R->GetRange();
  float *GRange = G->GetRange();
  float *BRange = B->GetRange();
  float *RFunc = R->GetDataPointer();
  float *GFunc = G->GetDataPointer();
  float *BFunc = B->GetDataPointer();
  int RSize = R->GetSize();
  int GSize = G->GetSize();
  int BSize = B->GetSize();
    
  int   i1, i2;
  float x1, y1;	// Point before x
  float x2, y2;	// Point after x

  float slope;
  float value;

  R->Update();
  G->Update();
  B->Update();

  if(RSize == 0  || GSize == 0 || BSize == 0)
    {
    vtkGenericWarningMacro("Transfer Function Has No Points!");
    return;
    }

  while (--i >= 0) 
    {
    findx = (float) *input;

    // do red
    if( findx < RRange[0] ) 
      {
      *output = 0;
      }
    else if( findx > RRange[1] )
      {
      *output = 0;
      }
    else
      {
      i2 = 0;
      x2 = RFunc[0];
      y2 = RFunc[1];
      
      while( (x2 < findx) && (i2 < RSize) )
        {
        i2 += 1;
        x2 = RFunc[(i2*2)];
        y2 = RFunc[(i2*2+1)];
        }
      
      // Check if we have found the exact point
      if( x2 == findx )
        {
        *output++ = (unsigned char)(255*RFunc[(i2*2 + 1)]);
        }
      else
        {
        i1 = i2 - 1;
        x1 = RFunc[(i1*2)];
        y1 = RFunc[(i1*2 +1)];
        slope = (y2-y1)/(x2-x1);
        value = y1 + slope*(findx-x1);
        
        *output++ = (unsigned char)(255*value);
        }
      }
    
    if (outFormat == VTK_RGBA || outFormat == VTK_RGB)
      {
      // do green
      if( findx < GRange[0] ) 
	{
	*output = 0;
	}
      else if( findx > GRange[1] )
	{
	*output = 0;
	}
      else
	{
	i2 = 0;
	x2 = GFunc[0];
	y2 = GFunc[1];
      
	while( (x2 < findx) && (i2 < GSize) )
	  {
	  i2 += 1;
	  x2 = GFunc[(i2*2)];
	  y2 = GFunc[(i2*2+1)];
	  }
      
	// Check if we have found the exact point
	if( x2 == findx )
	  {
	  *output++ = (unsigned char)(255*GFunc[(i2*2 + 1)]);
	  }
	else
	  {
	  i1 = i2 - 1;
	  x1 = GFunc[(i1*2)];
	  y1 = GFunc[(i1*2 +1)];
	  slope = (y2-y1)/(x2-x1);
	  value = y1 + slope*(findx-x1);
	  
	  *output++ = (unsigned char)(255*value);
	  }
	}
    
      // do blue
      if( findx < BRange[0] ) 
	{
	*output = 0;
	}
      else if( findx > BRange[1] )
	{
	*output = 0;
	}
      else
	{
	i2 = 0;
	x2 = BFunc[0];
	y2 = BFunc[1];
      
	while( (x2 < findx) && (i2 < BSize) )
	  {
	  i2 += 1;
	  x2 = BFunc[(i2*2)];
	  y2 = BFunc[(i2*2+1)];
	  }
      
	// Check if we have found the exact point
	if( x2 == findx )
	  {
	  *output++ = (unsigned char)(255*BFunc[(i2*2 + 1)]);
	  }
	else
	  {
	  i1 = i2 - 1;
	  x1 = BFunc[(i1*2)];
	  y1 = BFunc[(i1*2 +1)];
	  slope = (y2-y1)/(x2-x1);
	  value = y1 + slope*(findx-x1);
        
	  *output++ = (unsigned char)(255*value);
	  }
	}
      }

    if (outFormat == VTK_RGBA || outFormat == VTK_LUMINANCE_ALPHA)
      {
      *output++ = 255;
      }
    input += inIncr;
    }
}

// accelerate the mapping by copying the data in 32-bit chunks instead
// of 8-bit chunks
template<class T>
static void 
vtkColorTransferFunctionMapData(vtkColorTransferFunction *self, 
				T *input, 
				unsigned char *output, 
				int length, int inIncr, int outFormat)
{
  if (self->GetClamping())
    {
    vtkColorTransferFunctionMapDataClamp(self,input,output,
					 length,inIncr,outFormat);
    }
  else
    {
    vtkColorTransferFunctionMapDataNoClamp(self,input,output,
					   length,inIncr,outFormat);
    }
}

void vtkColorTransferFunction::MapScalarsThroughTable2(void *input, 
                                                       unsigned char *output,
                                                       int inputDataType, 
                                                       int numberOfValues,
                                                       int inputIncrement,
						       int outputFormat)
{
  switch (inputDataType)
    {
    case VTK_CHAR:
      vtkColorTransferFunctionMapData(this,(char *)input,output,
				      numberOfValues,inputIncrement,
				      outputFormat);
      break;
      
    case VTK_UNSIGNED_CHAR:
      vtkColorTransferFunctionMapData(this,(unsigned char *)input,
				      output,numberOfValues,
				      inputIncrement,outputFormat);
      break;
      
    case VTK_SHORT:
      vtkColorTransferFunctionMapData(this,(short *)input,output,
				      numberOfValues,inputIncrement,
				      outputFormat);
      break;
      
    case VTK_UNSIGNED_SHORT:
      vtkColorTransferFunctionMapData(this,(unsigned short *)input,
				      output,numberOfValues,
				      inputIncrement,outputFormat);
      break;
      
    case VTK_INT:
      vtkColorTransferFunctionMapData(this,(int *)input,output,
				      numberOfValues,inputIncrement,
				      outputFormat);
      break;
      
    case VTK_UNSIGNED_INT:
      vtkColorTransferFunctionMapData(this,(unsigned int *)input,output,
				      numberOfValues,inputIncrement,
				      outputFormat);
      break;
      
    case VTK_LONG:
      vtkColorTransferFunctionMapData(this,(long *)input,output,
				      numberOfValues,inputIncrement,
				      outputFormat);
      break;
      
    case VTK_UNSIGNED_LONG:
      vtkColorTransferFunctionMapData(this,(unsigned long *)input,output,
				      numberOfValues,inputIncrement,
				      outputFormat);
      break;
      
    case VTK_FLOAT:
      vtkColorTransferFunctionMapData(this,(float *)input,output,
				      numberOfValues,inputIncrement,
				      outputFormat);
      break;
      
    case VTK_DOUBLE:
      vtkColorTransferFunctionMapData(this,(double *)input,output,
				      numberOfValues,inputIncrement,
				      outputFormat);
      break;
      
    default:
      vtkErrorMacro(<< "MapImageThroughTable: Unknown input ScalarType");
      return;
    }
}  
