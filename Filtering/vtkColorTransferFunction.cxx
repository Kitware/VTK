/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorTransferFunction.cxx
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
  // Remove these when old methods are removed
  this->Red = vtkPiecewiseFunction::New();
  this->Green = vtkPiecewiseFunction::New();
  this->Blue = vtkPiecewiseFunction::New();
  
  this->UnsignedCharRGBAValue[0] = 0;
  this->UnsignedCharRGBAValue[1] = 0;
  this->UnsignedCharRGBAValue[2] = 0;
  this->UnsignedCharRGBAValue[3] = 0;
  
  this->Range[0] = 0;
  this->Range[1] = 0;

  this->Clamping = 0;
  this->ColorSpace = VTK_CTF_RGB;
  
  this->Function = NULL;
  this->FunctionSize = 0;
  this->NumberOfPoints = 0;

  this->Table = NULL;
  this->TableSize = 0;
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
  
  delete [] this->Function;
  delete [] this->Table;
}


void vtkColorTransferFunction::RGBToHSV(float R, float G, float B,
                                        float &H, float &S, float &V)
{
  float cmax, cmin;
  
  cmax = R;
  cmin = R;
  if (G > cmax)
    {
    cmax = G;
    }
  else if (G < cmin)
    {
    cmin = G;
    }
  if (B > cmax)
    {
    cmax = B;
    }
  else if (B < cmin)
    {
    cmin = B;
    }
  V = cmax;

  if (V > 0.0)
    {
    S = (cmax - cmin)/cmax;
    }
  else 
    {
    S = 0.0;
    }
  if (S > 0)
    {
    if (R == cmax)
      {
      H = 0.17*(G - B)/(cmax - cmin);
      }
    else if (G == cmax)
      {
      H = 0.33 + 0.17*(B - R)/(cmax - cmin);
      }
    else
      {
      H = 0.67 + 0.17*(R - G)/(cmax - cmin);
      }
    if (H < 0.0)
      {
      H = H + 1.0;
      }
    }
  else
    {
    H = 0.0;
    }
}

void vtkColorTransferFunction::HSVToRGB(float hue, float sat, float V,
                                        float &R, float &G, float &B)
{
  // compute RGB from HSV
  if (hue > 0.17 && hue <= 0.33) // green/red
    {
    G = 1.0;
    R = (0.33-hue)/0.16;
    B = 0.0;
    }
  else if (hue > 0.33 && hue <= 0.5) // green/blue
    {
    G = 1.0;
    B = (hue - 0.33)/0.17;
    R = 0.0;
    }
  else if (hue > 0.5 && hue <= 0.67) // blue/green
    {
    B = 1.0;
    G = (0.67 - hue)/0.17;
    R = 0.0;
    }
  else if (hue > 0.67 && hue <= 0.83) // blue/red
    {
    B = 1.0;
    R = (hue - 0.67)/0.16;
    G = 0.0;
    }
  else if (hue > 0.83 && hue <= 1.0) // red/blue
    {
    R = 1.0;
    B = (1.0-hue)/0.17;
    G = 0.0;
    }
  else // red/green
    {
    R = 1.0;
    G = hue/0.17;
    B = 0.0;
    }
  
  // add Saturation to the equation.
  R = (sat*R + (1.0 - sat));
  G = (sat*G + (1.0 - sat));
  B = (sat*B + (1.0 - sat));
  
  R = R * V;
  G = G * V;
  B = B * V;
}

// Add a point defined in RGB
void vtkColorTransferFunction::AddRGBPoint( float x, float r,
                                            float g, float b )
{
  float *fptr = this->Function;
  int i;
  
  
  for ( i = 0; i < this->NumberOfPoints; i++, fptr+=4 )
    {
    if ( x <= *fptr )
      {
      break;
      }
    }
  
  // Do we have an exact match?
  if ( i < this->NumberOfPoints && this->Function[4*i] == x )
    {
    this->Function[4*i  ] = x;
    this->Function[4*i+1] = r;
    this->Function[4*i+2] = g;
    this->Function[4*i+3] = b;    
    }
  // otherwise we have to add it before the current location
  else
    {
    // We need more space
    if ( this->NumberOfPoints == this->FunctionSize )
      {
      if ( this->FunctionSize )
	{
        this->FunctionSize *= 2;
	}
      else
	{
        this->FunctionSize = 100;
	}

      float *tmp = new float[this->FunctionSize*4];
      if ( i > 0 )
        {
        memcpy( tmp, this->Function, i*sizeof(float)*4 );
        }
      if ( i < this->NumberOfPoints )
        {
        memcpy( tmp+i+1, this->Function+i, 
                (this->NumberOfPoints-i)*sizeof(float)*4 );
        }
      if ( this->Function )
	{
	delete [] this->Function;
	}
      this->Function = tmp;
      }
    else
      {
      for ( int j = this->NumberOfPoints - 1; j >= i; j-- )
        {
        this->Function[4*(j+1)  ] = this->Function[4*j  ];
        this->Function[4*(j+1)+1] = this->Function[4*j+1];
        this->Function[4*(j+1)+2] = this->Function[4*j+2];
        this->Function[4*(j+1)+3] = this->Function[4*j+3];
        }
      }
    this->Function[i*4  ] = x;
    this->Function[i*4+1] = r;
    this->Function[i*4+2] = g;
    this->Function[i*4+3] = b;
    
    this->NumberOfPoints++;
    }
  
  this->Range[0] = *this->Function;
  this->Range[1] = *(this->Function + (this->NumberOfPoints-1)*4); 
  
  this->Modified();
}

// Add a point defined in HSV
void vtkColorTransferFunction::AddHSVPoint( float x, float h,
                                            float s, float v )
{ 
  float r, b, g;
  
  this->HSVToRGB( h, s, v, r, g, b );
  this->AddRGBPoint( x, r, g, b );
}

// Remove a point
void vtkColorTransferFunction::RemovePoint( float x )
{
  float *fptr = this->Function;
  int i;
  

  // find the point to remove
  for ( i = 0; i < this->NumberOfPoints; i++, fptr+=4 )
    {
    if ( x == *fptr )
      {
      break;
      }
    }
  
  if ( i < this->NumberOfPoints )
    {
    this->NumberOfPoints--;
    
    for ( int j = i; j < this->NumberOfPoints; j++ )
      {
      this->Function[4*j  ] = this->Function[4*(j+1)  ];
      this->Function[4*j+1] = this->Function[4*(j+1)+1];
      this->Function[4*j+2] = this->Function[4*(j+1)+2];
      this->Function[4*j+3] = this->Function[4*(j+1)+3];
      }
    }
  
  if ( this->NumberOfPoints )
    {
    this->Range[0] = *this->Function;
    this->Range[1] = *(this->Function + (this->NumberOfPoints-1)*4); 
    }
  else
    {
    this->Range[0] = 0.0;
    this->Range[1] = 0.0;    
    }
  
  this->Modified();
}


// Remove all points
void vtkColorTransferFunction::RemoveAllPoints()
{
  this->NumberOfPoints = 0;
  
  this->Range[0] = 0;
  this->Range[1] = 0;
  
  this->Modified();
}

// Add a line defined in RGB 
void vtkColorTransferFunction::AddRGBSegment( float x1, float r1, 
                                              float g1, float b1, 
                                              float x2, float r2, 
                                              float g2, float b2 )
{
  float x;
  this->AddRGBPoint( x1, r1, g1, b1 );
  this->AddRGBPoint( x2, r2, g2, b2 );

  int i, j;
  float *fptr = this->Function;
  
  // swap them if necessary
  if ( x1 > x2 )
    {
    x = x1;
    x1 = x2;
    x2 = x;
    }
  
  // find the first point
  for ( i = 0; i < this->NumberOfPoints; i++, fptr+=4 )
    {
    if ( x1 == *fptr )
      {
      break;
      }
    }
  
  // find the next one
  for ( j = i; j < this->NumberOfPoints; j++, fptr+=4 )
    {
    if ( x2 == *fptr )
      {
      break;
      }
    }
  
  int d = j-i-1;
  
  if ( j < this->NumberOfPoints && d )
    {
    this->NumberOfPoints -= d;
    for ( int k = i+1; k < this->NumberOfPoints; k++ )
      {
      this->Function[4*k  ] = this->Function[4*(k+d)  ];
      this->Function[4*k+1] = this->Function[4*(k+d)+1];
      this->Function[4*k+2] = this->Function[4*(k+d)+2];
      this->Function[4*k+3] = this->Function[4*(k+d)+3];
      }    
    }
  
  this->Range[0] = *this->Function;
  this->Range[1] = *(this->Function + (this->NumberOfPoints-1)*4); 
  
  this->Modified();
}

// Add a line defined in HSV
void vtkColorTransferFunction::AddHSVSegment( float x1, float h1, 
                                              float s1, float v1, 
                                              float x2, float h2, 
                                              float s2, float v2 )
{
  float r1, r2, b1, b2, g1, g2;
  
  this->HSVToRGB( h1, s1, v1, r1, g1, b1 );
  this->HSVToRGB( h2, s2, v2, r2, g2, b2 );
  this->AddRGBSegment( x1, r1, g1, b1, x2, r2, g2, b2 );
}

// Returns the RGBA color evaluated at the specified location
unsigned char *vtkColorTransferFunction::MapValue( float x )
{
  float rgb[3];
  this->GetColor( x, rgb );
  
  this->UnsignedCharRGBAValue[0] = (unsigned char) (255.0*rgb[0]);
  this->UnsignedCharRGBAValue[1] = (unsigned char) (255.0*rgb[1]);
  this->UnsignedCharRGBAValue[2] = (unsigned char) (255.0*rgb[2]);
  this->UnsignedCharRGBAValue[3] = 255;
  return( this->UnsignedCharRGBAValue );
}

// Returns the RGB color evaluated at the specified location
void vtkColorTransferFunction::GetColor(float x, float rgb[3])
{
  this->GetTable( x, x, 1, rgb );
}

// Returns the red color evaluated at the specified location
float vtkColorTransferFunction::GetRedValue( float x )
{
  float rgb[3];
  this->GetColor( x, rgb );

  return rgb[0];
}

// Returns the green color evaluated at the specified location
float vtkColorTransferFunction::GetGreenValue( float x )
{
  float rgb[3];
  this->GetColor( x, rgb );

  return rgb[1];
}

// Returns the blue color evaluated at the specified location
float vtkColorTransferFunction::GetBlueValue( float x )
{
  float rgb[3];
  this->GetColor( x, rgb );

  return rgb[2];
}

// Returns a table of RGB colors at regular intervals along the function
void vtkColorTransferFunction::GetTable( float x1, float x2, 
					 int size, float* table )
{
  float x, xinc=0;
  float *tptr = table;
  float *fptr = this->Function;
  int   loc;
  int   i;
  float weight;
  
  if ( this->NumberOfPoints == 0 )
    {
    vtkErrorMacro( 
      "Attempting to lookup a value with no points in the function");
    return;
    }
  
  if ( size > 1 )
    {
    xinc = (x2 - x1) / (float)(size-1);
    }
  
  loc  = 0;
  
  for ( i = 0, x = x1; i < size; i++, x += xinc )
    {
    while ( (loc < this->NumberOfPoints) && (x > *fptr) )
      {
      loc++;
      fptr+=4;
      }
    
    // Are we past the outside edge? if so, fill in according to Clamping
    if ( loc == this->NumberOfPoints )
      {
      if ( this->Clamping )
        {
        *(tptr++) = *(fptr-3);
        *(tptr++) = *(fptr-2);
        *(tptr++) = *(fptr-1);        
        }
      else
        {
        *(tptr++) = 0.0;
        *(tptr++) = 0.0;
        *(tptr++) = 0.0;        
        }      
      }
    else
      {
      // Do we have an exact match?
      if ( x == *fptr )
        {
        *(tptr++) = *(fptr+1);
        *(tptr++) = *(fptr+2);
        *(tptr++) = *(fptr+3);
        }
      // Are we before the beginning?
      else if ( loc == 0 )
        {
        if ( this->Clamping )
          {
          *(tptr++) = *(fptr+1);
          *(tptr++) = *(fptr+2);
          *(tptr++) = *(fptr+3);
          }
        else
          {
          *(tptr++) = 0.0;
          *(tptr++) = 0.0;
          *(tptr++) = 0.0;
          }
        }
      // We are somewhere in the middle. Use the correct interpolation.
      else
        {
        weight = (x - *(fptr-4)) / (*fptr - *(fptr-4));
      
        // RGB space
        if ( this->ColorSpace == VTK_CTF_RGB )
          {
          *(tptr++) = (1.0-weight) * *(fptr-3) + weight * *(fptr+1);
          *(tptr++) = (1.0-weight) * *(fptr-2) + weight * *(fptr+2);
          *(tptr++) = (1.0-weight) * *(fptr-1) + weight * *(fptr+3);
          }
        // HSV space
        else
          {
          float h1, h2, h3, s1, s2, s3, v1, v2, v3;
          this->RGBToHSV(*(fptr-3), *(fptr-2), *(fptr-1), h1, s1, v1);
          this->RGBToHSV(*(fptr+1), *(fptr+2), *(fptr+3), h2, s2, v2);
          s3 = (1.0-weight)*s1 + weight*s2;
          v3 = (1.0-weight)*v1 + weight*v2;
          // Do we need to cross the 0/1 boundary?
          if ( h1 - h2 > 0.5 || h2 - h1 > 0.5 )
            {
            //Yes, we are crossing the boundary
            if ( h1 > h2 )
              {
              h1 -= 1.0;
              }
            else
              {
              h2 -= 1.0;
              }
            h3 = (1.0-weight)*h1 + weight*h2;
            if ( h3 < 0.0 )
              {
              h3 += 1.0;
              }
            }
          else
            {
            // No we are not crossing the boundary
            h3 = (1.0-weight)*h1 + weight*h2;
            }
          // Make sure they are between 0 and 1
          h3 = (h3>1.0)?(1.0):((h3<0.0)?(0.0):(h3));
          s3 = (s3>1.0)?(1.0):((s3<0.0)?(0.0):(s3));
          v3 = (v3>1.0)?(1.0):((v3<0.0)?(0.0):(v3));
          this->HSVToRGB(h3, s3, v3, *tptr, *(tptr+1), *(tptr+2) );
          tptr += 3;
          }
        }
      }
    }
}

const unsigned char *vtkColorTransferFunction::GetTable( float x1, float x2, 
                                                         int size)
{
  float x, xinc=0;
  float *fptr = this->Function;
  int   loc;
  int   i;
  float weight;
  
  if (this->GetMTime() <= this->BuildTime &&
      this->TableSize == size)
    {
    return this->Table;
    }

  if ( this->NumberOfPoints == 0 )
    {
    vtkErrorMacro( 
      "Attempting to lookup a value with no points in the function");
    return this->Table;
    }
  
  if (this->TableSize != size)
    {
    delete [] this->Table;
    this->Table = new unsigned char [size*3];
    this->TableSize = size;
    }
  unsigned char *tptr = this->Table;
  
  if ( size > 1 )
    {
    xinc = (x2 - x1) / (float)(size-1);
    }
  
  loc  = 0;
  for ( i = 0, x = x1; i < size; i++, x += xinc )
    {
    while ( (loc < this->NumberOfPoints) && (x > *fptr) )
      {
      loc++;
      fptr+=4;
      }
    
    // Are we past the outside edge? if so, fill in according to Clamping
    if ( loc == this->NumberOfPoints )
      {
      if ( this->Clamping )
        {
        *(tptr++) = (unsigned char)(*(fptr-3)*255);
        *(tptr++) = (unsigned char)(*(fptr-2)*255);
        *(tptr++) = (unsigned char)(*(fptr-1)*255);        
        }
      else
        {
        *(tptr++) = 0;
        *(tptr++) = 0;
        *(tptr++) = 0;        
        }      
      }
    else
      {
      // Do we have an exact match?
      if ( x == *fptr )
        {
        *(tptr++) = (unsigned char)(*(fptr+1)*255);
        *(tptr++) = (unsigned char)(*(fptr+2)*255);
        *(tptr++) = (unsigned char)(*(fptr+3)*255);
        }
      // Are we before the beginning?
      else if ( loc == 0 )
        {
        if ( this->Clamping )
          {
          *(tptr++) = (unsigned char)(*(fptr+1)*255);
          *(tptr++) = (unsigned char)(*(fptr+2)*255);
          *(tptr++) = (unsigned char)(*(fptr+3)*255);
          }
        else
          {
          *(tptr++) = 0;
          *(tptr++) = 0;
          *(tptr++) = 0;
          }
        }
      // We are somewhere in the middle. Use the correct interpolation.
      else
        {
        weight = (x - *(fptr-4)) / (*fptr - *(fptr-4));
      
        // RGB space
        if ( this->ColorSpace == VTK_CTF_RGB )
          {
          *(tptr++) = (unsigned char)
            (255*((1.0-weight) * *(fptr-3) + weight * *(fptr+1)));
          *(tptr++) = (unsigned char)
            (255*((1.0-weight) * *(fptr-2) + weight * *(fptr+2)));
          *(tptr++) = (unsigned char)
            (255*((1.0-weight) * *(fptr-1) + weight * *(fptr+3)));
          }
        // HSV space
        else
          {
          float h1, h2, h3, s1, s2, s3, v1, v2, v3;
          this->RGBToHSV(*(fptr-3), *(fptr-2), *(fptr-1), h1, s1, v1);
          this->RGBToHSV(*(fptr+1), *(fptr+2), *(fptr+3), h2, s2, v2);
          s3 = (1.0-weight)*s1 + weight*s2;
          v3 = (1.0-weight)*v1 + weight*v2;
          // Do we need to cross the 0/1 boundary?
          if ( h1 - h2 > 0.5 || h2 - h1 > 0.5 )
            {
            //Yes, we are crossing the boundary
            if ( h1 > h2 )
              {
              h1 -= 1.0;
              }
            else
              {
              h2 -= 1.0;
              }
            h3 = (1.0-weight)*h1 + weight*h2;
            if ( h3 < 0.0 )
              {
              h3 += 1.0;
              }
            }
          else
            {
            // No we are not crossing the boundary
            h3 = (1.0-weight)*h1 + weight*h2;
            }
          // Make sure they are between 0 and 1
          h3 = (h3>1.0)?(1.0):((h3<0.0)?(0.0):(h3));
          s3 = (s3>1.0)?(1.0):((s3<0.0)?(0.0):(s3));
          v3 = (v3>1.0)?(1.0):((v3<0.0)?(0.0):(v3));
          this->HSVToRGB(h3, s3, v3, h1, s1, v1 );
          *(tptr++) = (unsigned char)(255*h1);
          *(tptr++) = (unsigned char)(255*s1);
          *(tptr++) = (unsigned char)(255*v1);
          }
        }
      }
    }
  this->BuildTime.Modified();
  return this->Table;
}

void vtkColorTransferFunction::BuildFunctionFromTable( float x1, float x2,
						       int size, float *table)
{
  // We are assuming the table is in ascending order

  float      *fptr;
  float      *tptr = table;
  float      x, xinc;
  int        i;
  
  xinc = (x2 - x1) / (float)(size-1);
  
  this->RemoveAllPoints();

  // Is it big enough?
  if ( this->FunctionSize < size )
    {
    delete [] this->Function;
    this->FunctionSize = size*2;
    this->Function = new float [4*this->FunctionSize];
    }
  
  fptr = this->Function;
  
  for (i = 0, x = x1; i < size; i++, x += xinc )
    {
    *(fptr++) = x;
    *(fptr++) = *(tptr++);
    *(fptr++) = *(tptr++);
    *(fptr++) = *(tptr++);    
    }
  
  this->NumberOfPoints = size;
  
  this->Modified();
}


// Print method for vtkColorTransferFunction
void vtkColorTransferFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkScalarsToColors::PrintSelf(os, indent);

  os << indent << "Size: " << this->NumberOfPoints << endl;
  if ( this->Clamping )
    {
    os << indent << "Clamping: On\n";
    }
  else
    {
    os << indent << "Clamping: Off\n";
    }

  if ( this->ColorSpace == VTK_CTF_RGB )
    {
    os << indent << "Color Space: RGB\n";
    }
  else
    {
    os << indent << "Color Space: HSV\n";
    }
  
  os << indent << "Range: " << this->Range[0] << " to " 
     << this->Range[1] << endl;
  
  if ( this->NumberOfPoints < 100 )
    {
    for ( int i = 0; i < this->NumberOfPoints; i++ )
      {
      os << indent << "  Point " << i << ": " << this->Function[i*4] << " maps to " 
         << this->Function[i*4+1] << " " 
         << this->Function[i*4+2] << " " 
         << this->Function[i*4+3] << endl;
      }
    }
  
  // These are old
  //  os << indent << "Color Transfer Function Total Points: " << this->GetTotalSize() << "\n";
  //  os << indent << "Red Transfer Function: ";
  //  os << this->Red << "\n";
  //  os << indent << "Green Transfer Function: ";
  //  os << this->Green << "\n";
  //  os << indent << "Blue Transfer Function: ";
  //  os << this->Blue << "\n";

}


void vtkColorTransferFunction::DeepCopy( vtkColorTransferFunction *f )
{
  delete [] this->Function;
  delete [] this->Table;
  this->TableSize = 0;

  this->Clamping       = f->Clamping;
  this->ColorSpace     = f->ColorSpace;
  this->FunctionSize   = f->FunctionSize;
  this->NumberOfPoints = f->NumberOfPoints;
  this->Range[0]       = f->Range[0];
  this->Range[1]       = f->Range[1];
  
  if ( this->FunctionSize > 0 )
    {
    this->Function     = new float [4*this->FunctionSize];
    memcpy(this->Function, f->Function, 4*sizeof(float)*this->FunctionSize);
    }
  else
    {
    this->Function = NULL;
    }
  
  this->Modified();
}

// accelerate the mapping by copying the data in 32-bit chunks instead
// of 8-bit chunks
template <class T>
static void 
vtkColorTransferFunctionMapData(vtkColorTransferFunction *self, 
                                T *input, 
                                unsigned char *output, 
                                int length, int inIncr, int outFormat)
{
  float          x;
  int            i = length;
  float          rgb[3];
  unsigned char  *optr = output;
  T              *iptr = input;
  
  if(self->GetSize() == 0)
    {
    vtkGenericWarningMacro("Transfer Function Has No Points!");
    return;
    }

  while (--i >= 0) 
    {
    x = (float) *iptr;
    self->GetColor(x, rgb);
    
    if (outFormat == VTK_RGB || outFormat == VTK_RGBA)
      {
      *(optr++) = (unsigned char)(rgb[0]*255.0);
      *(optr++) = (unsigned char)(rgb[1]*255.0);
      *(optr++) = (unsigned char)(rgb[2]*255.0);
      }
    else // LUMINANCE  use coeffs of (0.30  0.59  0.11)*255.0
      {
      *(optr++) = (unsigned char)(rgb[0]*76.5 + rgb[1]*150.45 + rgb[2]*28.05); 
      }
    
    if (outFormat == VTK_RGBA || outFormat == VTK_LUMINANCE_ALPHA)
      {
      *(optr++) = 255;
      }
    iptr += inIncr;
    }
}



static void 
vtkColorTransferFunctionMapUnsignedCharData(vtkColorTransferFunction *self, 
                                            unsigned char *input, 
                                            unsigned char *output, 
                                            int length, int inIncr, 
                                            int outFormat)
{
  int            x;
  int            i = length;
  unsigned char  *optr = output;
  unsigned char  *iptr = input;

  if(self->GetSize() == 0)
    {
    vtkGenericWarningMacro("Transfer Function Has No Points!");
    return;
    }
  
  const unsigned char *table = self->GetTable(0,255,256);
  switch (outFormat)
    {
    case VTK_RGB:
      while (--i >= 0) 
        {
        x = *iptr*3;
        *(optr++) = table[x];
        *(optr++) = table[x+1];
        *(optr++) = table[x+2];
        iptr += inIncr;
        }
      break;
    case VTK_RGBA:
      while (--i >= 0) 
        {
        x = *iptr*3;
        *(optr++) = table[x];
        *(optr++) = table[x+1];
        *(optr++) = table[x+2];
        *(optr++) = 255;
        iptr += inIncr;
        }
      break;
    case VTK_LUMINANCE_ALPHA:
      while (--i >= 0) 
        {
        x = *iptr*3;
        *(optr++) = table[x];
        *(optr++) = 255;
        iptr += inIncr;
        }
      break;
    case VTK_LUMINANCE:
      while (--i >= 0) 
        {
        x = *iptr*3;
        *(optr++) = table[x];
        iptr += inIncr;
        }
      break;
    }  
}

static void 
vtkColorTransferFunctionMapUnsignedShortData(vtkColorTransferFunction *self, 
                                             unsigned short *input, 
                                             unsigned char *output, 
                                             int length, int inIncr, 
                                             int outFormat)
{
  int            x;
  int            i = length;
  unsigned char  *optr = output;
  unsigned short *iptr = input;

  if(self->GetSize() == 0)
    {
    vtkGenericWarningMacro("Transfer Function Has No Points!");
    return;
    }
  

  const unsigned char *table = self->GetTable(0,65535,65536);
  switch (outFormat)
    {
    case VTK_RGB:
      while (--i >= 0) 
        {
        x = *iptr*3;
        *(optr++) = table[x];
        *(optr++) = table[x+1];
        *(optr++) = table[x+2];
        iptr += inIncr;
        }
      break;
    case VTK_RGBA:
      while (--i >= 0) 
        {
        x = *iptr*3;
        *(optr++) = table[x];
        *(optr++) = table[x+1];
        *(optr++) = table[x+2];
        *(optr++) = 255;
        iptr += inIncr;
        }
      break;
    case VTK_LUMINANCE_ALPHA:
      while (--i >= 0) 
        {
        x = *iptr*3;
        *(optr++) = table[x];
        *(optr++) = 255;
        iptr += inIncr;
        }
      break;
    case VTK_LUMINANCE:
      while (--i >= 0) 
        {
        x = *iptr*3;
        *(optr++) = table[x];
        iptr += inIncr;
        }
      break;
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
      vtkColorTransferFunctionMapUnsignedCharData(this,(unsigned char *)input,
                                                  output,numberOfValues,
                                                  inputIncrement,outputFormat);
      break;
      
    case VTK_SHORT:
      vtkColorTransferFunctionMapData(this,(short *)input,output,
				      numberOfValues,inputIncrement,
				      outputFormat);
      break;
      
    case VTK_UNSIGNED_SHORT:
      vtkColorTransferFunctionMapUnsignedShortData(this,
                                                   (unsigned short *)input,
                                                   output,numberOfValues,
                                                   inputIncrement,
                                                   outputFormat);
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




// These are old methods which should be removed in 2 releases from VTK 3.1.2

// Old method - don't use. (depricated after VTK 3.1.2)
int vtkColorTransferFunction::GetTotalSize()
{
  vtkWarningMacro( "GetTotalSize() is a depricated method." << endl << 
                   "Please use GetSize() instead" << endl <<
                   "Since vtkColorTransferFunction does not" << endl <<
                   "use 3 vtkPiecewiseFunctions anymore" << endl <<
                   "there is no difference between size and total size" );
  
  return( this->GetSize() );
}

// Old method - don't use. (depricated after VTK 3.1.2)
int vtkColorTransferFunction::GetRedSize()
{
  vtkWarningMacro( "GetRedSize() is a depricated method." << endl << 
                   "Please use GetSize() instead" << endl <<
                   "Since vtkColorTransferFunction does not" << endl <<
                   "use 3 vtkPiecewiseFunctions anymore" << endl <<
                   "there is no difference between size and red size" );
  
  return( this->GetSize() );
}

// Old method - don't use. (depricated after VTK 3.1.2)
int vtkColorTransferFunction::GetGreenSize()
{
  vtkWarningMacro( "GetGreenSize() is a depricated method." << endl << 
                   "Please use GetSize() instead" << endl <<
                   "Since vtkColorTransferFunction does not" << endl <<
                   "use 3 vtkPiecewiseFunctions anymore" << endl <<
                   "there is no difference between size and green size" );
  
  return( this->GetSize() );
}

// Old method - don't use. (depricated after VTK 3.1.2)
int vtkColorTransferFunction::GetBlueSize()
{
  vtkWarningMacro( "GetBlueSize() is a depricated method." << endl << 
                   "Please use GetSize() instead" << endl <<
                   "Since vtkColorTransferFunction does not" << endl <<
                   "use 3 vtkPiecewiseFunctions anymore" << endl <<
                   "there is no difference between size and blue size" );
  
  return( this->GetSize() );
}

// Add a point to the red function
void vtkColorTransferFunction::AddRedPoint( float x, float r )
{
  vtkWarningMacro( "AddRedPoint() is a depricated method." << endl << 
                   "Please use AddRGBPoint() instead." );
  float rgb[3];
  this->GetColor( x, rgb );
  this->AddRGBPoint( x, r, rgb[1], rgb[2] );
}

// Add a point to the green function
void vtkColorTransferFunction::AddGreenPoint( float x, float g )
{
  vtkWarningMacro( "AddGreenPoint() is a depricated method." << endl << 
                   "Please use AddRGBPoint() instead." );
  float rgb[3];
  this->GetColor( x, rgb );
  this->AddRGBPoint( x, rgb[0], g, rgb[2] );
}

// Add a point to the blue function
void vtkColorTransferFunction::AddBluePoint( float x, float b )
{
  vtkWarningMacro( "AddBluePoint() is a depricated method." << endl << 
                   "Please use AddRGBPoint() instead." );
  float rgb[3];
  this->GetColor( x, rgb );
  this->AddRGBPoint( x, rgb[0], rgb[1], b );
}

// Remove a point from the red function
void vtkColorTransferFunction::RemoveRedPoint( float x )
{
  vtkWarningMacro( "RemoveRedPoint() is a depricated method." << endl << 
                   "Please use RemovePoint() instead." );
  this->RemovePoint(x);
}

// Remove a point from the green function
void vtkColorTransferFunction::RemoveGreenPoint( float x )
{
  vtkWarningMacro( "RemoveGreenPoint() is a depricated method." << endl << 
                   "Please use RemovePoint() instead." );
  this->RemovePoint(x);
}

// Remove a point from the blue function
void vtkColorTransferFunction::RemoveBluePoint( float x )
{
  vtkWarningMacro( "RemoveBluePoint() is a depricated method." << endl << 
                   "Please use RemovePoint() instead." );
  this->RemovePoint(x);
}

// Remove a point from all three functions (RGB)
void vtkColorTransferFunction::RemoveRGBPoint( float x )
{
  vtkWarningMacro( "RemoveRGBPoint() is a depricated method." << endl << 
                   "Please use RemovePoint() instead." );
  this->RemovePoint(x);
}

// Add a line to the red function
void vtkColorTransferFunction::AddRedSegment( float x1, float r1,
     float x2, float r2 )
{
  vtkWarningMacro( "AddRedSegment() is a depricated method." << endl << 
                   "Please use AddRGBSegment() instead." );
  float rgb1[3], rgb2[3];
  this->GetColor( x1, rgb1 );
  this->GetColor( x2, rgb2 );
  this->AddRGBSegment( x1, r1, rgb1[1], rgb1[2], x2, r2, rgb2[1], rgb2[2] );
}

// Add a line to the green function
void vtkColorTransferFunction::AddGreenSegment( float x1, float g1,
     float x2, float g2 )
{
  vtkWarningMacro( "AddGreenSegment() is a depricated method." << endl << 
                   "Please use AddRGBSegment() instead." );
  float rgb1[3], rgb2[3];
  this->GetColor( x1, rgb1 );
  this->GetColor( x2, rgb2 );
  this->AddRGBSegment( x1, rgb1[0], g1, rgb1[2], x2, rgb2[0], g2, rgb2[2] );
}

// Add a line to the blue function
void vtkColorTransferFunction::AddBlueSegment( float x1, float b1,
     float x2, float b2 )
{
  vtkWarningMacro( "AddBlueSegment() is a depricated method." << endl << 
                   "Please use AddRGBSegment() instead." );
  float rgb1[3], rgb2[3];
  this->GetColor( x1, rgb1 );
  this->GetColor( x2, rgb2 );
  this->AddRGBSegment( x1, rgb1[0], rgb1[1], b1, x2, rgb2[0], rgb2[1], b2 );
}



