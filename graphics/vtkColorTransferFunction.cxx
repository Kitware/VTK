/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorTransferFunction.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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

#include "vtkColorTransferFunction.h"

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
  this->ColorValue2[3] = 1.0;
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
  float x;
  float inc;
  int   i;

  if( x1 == x2 )
    {
    return;
    }

  x = x1;
  if( size > 1 )
    {
    inc = (x2-x1)/(float)(size-1);
    }
  else
    {
    inc = 0;
    }

  for( i=0; i<size; i++ )
    {
    table[i*3  ] = this->Red->GetValue( x );
    table[i*3+1] = this->Green->GetValue( x );
    table[i*3+2] = this->Blue->GetValue( x );
    x += inc;
    }
}

// Print method for vtkColorTransferFunction
void vtkColorTransferFunction::PrintSelf(ostream& os, vtkIndent indent)
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
vtkColorTransferFunctionMapDataToRGBAClamp(vtkColorTransferFunction *self, 
                                           T *input, 
                                           unsigned char *output, 
                                           int length, int incr)
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
    findx = *input;

    // do red
    if( findx < RRange[0] ) 
      {
      *output = 255*RFunc[0];
      }
    else if( findx > RRange[1] )
      {
      *output = 255*RFunc[(RSize-1)*2];
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
        *output++ = 255*RFunc[(i2*2 + 1)];
        }
      else
        {
        i1 = i2 - 1;
        x1 = RFunc[(i1*2)];
        y1 = RFunc[(i1*2 +1)];
        slope = (y2-y1)/(x2-x1);
        value = y1 + slope*(findx-x1);
        
        *output++ = 255*value;
        }
      }
    
    // do green
    if( findx < GRange[0] ) 
      {
      *output = 255*GFunc[0];
      }
    else if( findx > GRange[1] )
      {
      *output = 255*GFunc[(GSize-1)*2];
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
        *output++ = 255*GFunc[(i2*2 + 1)];
        }
      else
        {
        i1 = i2 - 1;
        x1 = GFunc[(i1*2)];
        y1 = GFunc[(i1*2 +1)];
        slope = (y2-y1)/(x2-x1);
        value = y1 + slope*(findx-x1);
        
        *output++ = 255*value;
        }
      }
    
    // do blue
    if( findx < BRange[0] ) 
      {
      *output = 255*BFunc[0];
      }
    else if( findx > BRange[1] )
      {
      *output = 255*BFunc[(BSize-1)*2];
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
        *output++ = 255*BFunc[(i2*2 + 1)];
        }
      else
        {
        i1 = i2 - 1;
        x1 = BFunc[(i1*2)];
        y1 = BFunc[(i1*2 +1)];
        slope = (y2-y1)/(x2-x1);
        value = y1 + slope*(findx-x1);
        
        *output++ = 255*value;
        }
      }
    
    *output++ = 255;
    input += incr;
    }
}

// accelerate the mapping by copying the data in 32-bit chunks instead
// of 8-bit chunks
template<class T>
static void 
vtkColorTransferFunctionMapDataToRGBANoClamp(vtkColorTransferFunction *self, 
                                             T *input, 
                                             unsigned char *output, 
                                             int length, int incr)
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
    findx = *input;

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
        *output++ = 255*RFunc[(i2*2 + 1)];
        }
      else
        {
        i1 = i2 - 1;
        x1 = RFunc[(i1*2)];
        y1 = RFunc[(i1*2 +1)];
        slope = (y2-y1)/(x2-x1);
        value = y1 + slope*(findx-x1);
        
        *output++ = 255*value;
        }
      }
    
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
        *output++ = 255*GFunc[(i2*2 + 1)];
        }
      else
        {
        i1 = i2 - 1;
        x1 = GFunc[(i1*2)];
        y1 = GFunc[(i1*2 +1)];
        slope = (y2-y1)/(x2-x1);
        value = y1 + slope*(findx-x1);
        
        *output++ = 255*value;
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
        *output++ = 255*BFunc[(i2*2 + 1)];
        }
      else
        {
        i1 = i2 - 1;
        x1 = BFunc[(i1*2)];
        y1 = BFunc[(i1*2 +1)];
        slope = (y2-y1)/(x2-x1);
        value = y1 + slope*(findx-x1);
        
        *output++ = 255*value;
        }
      }

    *output++ = 255;
    input += incr;
    }
}

// accelerate the mapping by copying the data in 32-bit chunks instead
// of 8-bit chunks
template<class T>
static void 
vtkColorTransferFunctionMapDataToRGBA(vtkColorTransferFunction *self, 
                                      T *input, 
                                      unsigned char *output, 
                                      int length, int incr)
{
  if (self->GetClamping())
    {
    vtkColorTransferFunctionMapDataToRGBAClamp(self,input,output,
                                               length,incr);
    }
  else
    {
    vtkColorTransferFunctionMapDataToRGBANoClamp(self,input,output,
                                                 length,incr);
    }
}

void vtkColorTransferFunction::MapScalarsThroughTable2(void *input, 
                                                       unsigned char *output,
                                                       int inputDataType, 
                                                       int numberOfValues,
                                                       int inputIncrement)
{
  switch (inputDataType)
    {
    case VTK_CHAR:
      vtkColorTransferFunctionMapDataToRGBA(this,(char *)input,output,
                                            numberOfValues,inputIncrement);
      break;
      
    case VTK_UNSIGNED_CHAR:
      vtkColorTransferFunctionMapDataToRGBA(this,(unsigned char *)input,
                                            output,numberOfValues,
                                            inputIncrement);
      break;
      
    case VTK_SHORT:
      vtkColorTransferFunctionMapDataToRGBA(this,(short *)input,output,
                                            numberOfValues,inputIncrement);
      break;
      
    case VTK_UNSIGNED_SHORT:
      vtkColorTransferFunctionMapDataToRGBA(this,(unsigned short *)input,
                                            output,numberOfValues,
                                            inputIncrement);
      break;
      
    case VTK_INT:
      vtkColorTransferFunctionMapDataToRGBA(this,(int *)input,output,
                                            numberOfValues,inputIncrement);
      break;
      
    case VTK_UNSIGNED_INT:
      vtkColorTransferFunctionMapDataToRGBA(this,(unsigned int *)input,output,
                                            numberOfValues,inputIncrement);
      break;
      
    case VTK_LONG:
      vtkColorTransferFunctionMapDataToRGBA(this,(long *)input,output,
                                            numberOfValues,inputIncrement);
      break;
      
    case VTK_UNSIGNED_LONG:
      vtkColorTransferFunctionMapDataToRGBA(this,(unsigned long *)input,output,
                                            numberOfValues,inputIncrement);
      break;
      
    case VTK_FLOAT:
      vtkColorTransferFunctionMapDataToRGBA(this,(float *)input,output,
                                            numberOfValues,inputIncrement);
      break;
      
    case VTK_DOUBLE:
      vtkColorTransferFunctionMapDataToRGBA(this,(double *)input,output,
                                            numberOfValues,inputIncrement);
      break;
      
    default:
      vtkErrorMacro(<< "MapImageThroughTable: Unknown input ScalarType");
      return;
    }
}  
