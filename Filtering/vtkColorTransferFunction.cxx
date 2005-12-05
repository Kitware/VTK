/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorTransferFunction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkColorTransferFunction.h"

#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"

vtkCxxRevisionMacro(vtkColorTransferFunction, "1.56.6.1");
vtkStandardNewMacro(vtkColorTransferFunction);

//----------------------------------------------------------------------------
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

  this->Clamping = 1;
  this->ColorSpace = VTK_CTF_RGB;
  this->HSVWrap = 1; //By default HSV will be wrap
  
  this->Function = NULL;
  this->FunctionSize = 0;
  this->NumberOfPoints = 0;

  this->Table = NULL;
  this->TableSize = 0;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
// Add a point defined in RGB
int vtkColorTransferFunction::AddRGBPoint( double x, double r,
                                           double g, double b )
{
  double *fptr = this->Function;
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
    if (this->Function[4*i+1] != r ||
        this->Function[4*i+2] != g ||
        this->Function[4*i+3] != b)
      {
      this->Function[4*i+1] = r;
      this->Function[4*i+2] = g;
      this->Function[4*i+3] = b;    
      this->Modified();
      }
    
    return i;
    }

  // otherwise we have to add it before the current location
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

    double *tmp = new double[this->FunctionSize*4];
    if ( i > 0 )
      {
      memcpy( tmp, this->Function, i*sizeof(double)*4 );
      }
    if ( i < this->NumberOfPoints )
      {
      memcpy( tmp+((i+1)*4), this->Function+(i*4), 
              (this->NumberOfPoints-i)*sizeof(double)*4 );
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
  
  this->Range[0] = *this->Function;
  this->Range[1] = *(this->Function + (this->NumberOfPoints-1)*4); 
  
  this->Modified();

  return i;
}

//----------------------------------------------------------------------------
// Add a point defined in HSV
int vtkColorTransferFunction::AddHSVPoint( double x, double h,
                                            double s, double v )
{ 
  double r, b, g;
  
  vtkMath::HSVToRGB(h, s, v, &r, &g, &b);
  return this->AddRGBPoint( x, r, g, b );
}

//----------------------------------------------------------------------------
// Remove a point
int vtkColorTransferFunction::RemovePoint( double x )
{
  double *fptr = this->Function;
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

    return i;
    }

  return -1;
}


//----------------------------------------------------------------------------
// Remove all points
void vtkColorTransferFunction::RemoveAllPoints()
{
  if (!this->NumberOfPoints)
    {
    return;
    }

  this->NumberOfPoints = 0;
  
  this->Range[0] = 0;
  this->Range[1] = 0;
  
  this->Modified();
}

//----------------------------------------------------------------------------
// Add a line defined in RGB 
void vtkColorTransferFunction::AddRGBSegment( double x1, double r1, 
                                              double g1, double b1, 
                                              double x2, double r2, 
                                              double g2, double b2 )
{
  double x;
  this->AddRGBPoint( x1, r1, g1, b1 );
  this->AddRGBPoint( x2, r2, g2, b2 );

  int i, j;
  double *fptr = this->Function;
  
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

//----------------------------------------------------------------------------
// Add a line defined in HSV
void vtkColorTransferFunction::AddHSVSegment( double x1, double h1, 
                                              double s1, double v1, 
                                              double x2, double h2, 
                                              double s2, double v2 )
{
  double r1, r2, b1, b2, g1, g2;
  
  vtkMath::HSVToRGB(h1, s1, v1, &r1, &g1, &b1);
  vtkMath::HSVToRGB(h2, s2, v2, &r2, &g2, &b2);
  this->AddRGBSegment( x1, r1, g1, b1, x2, r2, g2, b2 );
}

//----------------------------------------------------------------------------
// Returns the RGBA color evaluated at the specified location
unsigned char *vtkColorTransferFunction::MapValue( double x )
{
  double rgb[3];
  this->GetColor( x, rgb );
  
  this->UnsignedCharRGBAValue[0] = (unsigned char) (255.0*rgb[0]);
  this->UnsignedCharRGBAValue[1] = (unsigned char) (255.0*rgb[1]);
  this->UnsignedCharRGBAValue[2] = (unsigned char) (255.0*rgb[2]);
  this->UnsignedCharRGBAValue[3] = 255;
  return this->UnsignedCharRGBAValue;
}

//----------------------------------------------------------------------------
// Returns the RGB color evaluated at the specified location
void vtkColorTransferFunction::GetColor(double x, double rgb[3])
{
  this->GetTable( x, x, 1, rgb );
}

//----------------------------------------------------------------------------
// Returns the red color evaluated at the specified location
double vtkColorTransferFunction::GetRedValue( double x )
{
  double rgb[3];
  this->GetColor( x, rgb );

  return rgb[0];
}

//----------------------------------------------------------------------------
// Returns the green color evaluated at the specified location
double vtkColorTransferFunction::GetGreenValue( double x )
{
  double rgb[3];
  this->GetColor( x, rgb );

  return rgb[1];
}

//----------------------------------------------------------------------------
// Returns the blue color evaluated at the specified location
double vtkColorTransferFunction::GetBlueValue( double x )
{
  double rgb[3];
  this->GetColor( x, rgb );

  return rgb[2];
}

//----------------------------------------------------------------------------
// Returns a table of RGB colors at regular intervals along the function
void vtkColorTransferFunction::GetTable( double x1, double x2, 
                                         int size, double* table )
{
  double x, xinc=0;
  double *tptr = table;
  double *fptr = this->Function;
  int   loc;
  int   i;
  double weight;
  
  if ( this->NumberOfPoints == 0 )
    {
    vtkErrorMacro( 
      "Attempting to lookup a value with no points in the function");
    return;
    }
  
  if ( size > 1 )
    {
    xinc = (x2 - x1) / (double)(size-1);
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
          double h1, h2, h3, s1, s2, s3, v1, v2, v3;
          vtkMath::RGBToHSV(*(fptr-3), *(fptr-2), *(fptr-1), &h1, &s1, &v1);
          vtkMath::RGBToHSV(*(fptr+1), *(fptr+2), *(fptr+3), &h2, &s2, &v2);
          s3 = (1.0-weight)*s1 + weight*s2;
          v3 = (1.0-weight)*v1 + weight*v2;
          // Do we need to cross the 0/1 boundary?
          if ( this->HSVWrap &&
               (h1 - h2 > 0.5 || h2 - h1 > 0.5) )
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
          else // HSV No Wrap
            {
            // No we are not crossing the boundary
            h3 = (1.0-weight)*h1 + weight*h2;
            }
          // Make sure they are between 0 and 1
          h3 = (h3>1.0)?(1.0):((h3<0.0)?(0.0):(h3));
          s3 = (s3>1.0)?(1.0):((s3<0.0)?(0.0):(s3));
          v3 = (v3>1.0)?(1.0):((v3<0.0)?(0.0):(v3));
          vtkMath::HSVToRGB(h3, s3, v3, tptr, tptr + 1, tptr + 2);
          tptr += 3;
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkColorTransferFunction::GetTable( double x1, double x2, 
                                         int size, float* table )
{
  double x, xinc=0;
  float *tptr = table;
  double *fptr = this->Function;
  int   loc;
  int   i;
  double weight;
  
  if ( this->NumberOfPoints == 0 )
    {
    vtkErrorMacro( 
      "Attempting to lookup a value with no points in the function");
    return;
    }
  
  if ( size > 1 )
    {
    xinc = (x2 - x1) / (double)(size-1);
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
        *(tptr++) = static_cast<float>(*(fptr-3));
        *(tptr++) = static_cast<float>(*(fptr-2));
        *(tptr++) = static_cast<float>(*(fptr-1));        
        }
      else
        {
        *(tptr++) = 0.0f;
        *(tptr++) = 0.0f;
        *(tptr++) = 0.0f;        
        }      
      }
    else
      {
      // Do we have an exact match?
      if ( x == *fptr )
        {
        *(tptr++) = static_cast<float>(*(fptr+1));
        *(tptr++) = static_cast<float>(*(fptr+2));
        *(tptr++) = static_cast<float>(*(fptr+3));
        }
      // Are we before the beginning?
      else if ( loc == 0 )
        {
        if ( this->Clamping )
          {
          *(tptr++) = static_cast<float>(*(fptr+1));
          *(tptr++) = static_cast<float>(*(fptr+2));
          *(tptr++) = static_cast<float>(*(fptr+3));
          }
        else
          {
          *(tptr++) = 0.0f;
          *(tptr++) = 0.0f;
          *(tptr++) = 0.0f;
          }
        }
      // We are somewhere in the middle. Use the correct interpolation.
      else
        {
        weight = (x - *(fptr-4)) / (*fptr - *(fptr-4));
      
        // RGB space
        if ( this->ColorSpace == VTK_CTF_RGB )
          {
          *(tptr++) = 
            static_cast<float>((1.0-weight) * *(fptr-3) + weight * *(fptr+1));
          *(tptr++) = 
            static_cast<float>((1.0-weight) * *(fptr-2) + weight * *(fptr+2));
          *(tptr++) = 
            static_cast<float>((1.0-weight) * *(fptr-1) + weight * *(fptr+3));
          }
        // HSV space
        else
          {
          double h1, h2, h3, s1, s2, s3, v1, v2, v3;
          vtkMath::RGBToHSV(*(fptr-3), *(fptr-2), *(fptr-1), &h1, &s1, &v1);
          vtkMath::RGBToHSV(*(fptr+1), *(fptr+2), *(fptr+3), &h2, &s2, &v2);
          s3 = (1.0-weight)*s1 + weight*s2;
          v3 = (1.0-weight)*v1 + weight*v2;
          // Do we need to cross the 0/1 boundary?
          if ( this->HSVWrap &&
               (h1 - h2 > 0.5 || h2 - h1 > 0.5) )
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
          vtkMath::HSVToRGB(static_cast<float>(h3), 
                            static_cast<float>(s3), 
                            static_cast<float>(v3), 
                            tptr, tptr + 1, tptr + 2);
          tptr += 3;
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
const unsigned char *vtkColorTransferFunction::GetTable( double x1, double x2, 
                                                         int size)
{
  double x, xinc=0;
  double *fptr = this->Function;
  int   loc;
  int   i;
  double weight;
  
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
    xinc = (x2 - x1) / (double)(size-1);
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
          double h1, h2, h3, s1, s2, s3, v1, v2, v3;
          vtkMath::RGBToHSV(*(fptr-3), *(fptr-2), *(fptr-1), &h1, &s1, &v1);
          vtkMath::RGBToHSV(*(fptr+1), *(fptr+2), *(fptr+3), &h2, &s2, &v2);
          s3 = (1.0-weight)*s1 + weight*s2;
          v3 = (1.0-weight)*v1 + weight*v2;
          // Do we need to cross the 0/1 boundary?
          if ( this->HSVWrap &&
               (h1 - h2 > 0.5 || h2 - h1 > 0.5) )
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
          else // HSV No Wrap
            {
            // No we are not crossing the boundary
            h3 = (1.0-weight)*h1 + weight*h2;
            }
          // Make sure they are between 0 and 1
          h3 = (h3>1.0)?(1.0):((h3<0.0)?(0.0):(h3));
          s3 = (s3>1.0)?(1.0):((s3<0.0)?(0.0):(s3));
          v3 = (v3>1.0)?(1.0):((v3<0.0)?(0.0):(v3));
          vtkMath::HSVToRGB(h3, s3, v3, &h1, &s1, &v1);
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

//----------------------------------------------------------------------------
void vtkColorTransferFunction::BuildFunctionFromTable( double x1, double x2,
                                                       int size, double *table)
{
  // We are assuming the table is in ascending order

  double      *fptr;
  double      *tptr = table;
  double      x, xinc;
  int        i;
  
  xinc = (x2 - x1) / (double)(size-1);
  
  this->RemoveAllPoints();

  // Is it big enough?
  if ( this->FunctionSize < size )
    {
    delete [] this->Function;
    this->FunctionSize = size*2;
    this->Function = new double [4*this->FunctionSize];
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


//----------------------------------------------------------------------------
// Print method for vtkColorTransferFunction
void vtkColorTransferFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

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
  else if ( this->ColorSpace == VTK_CTF_HSV && this->HSVWrap )
    {
    os << indent << "Color Space: HSV\n";
    }
  else
    {
    os << indent << "Color Space: HSV (No Wrap)\n";
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


//----------------------------------------------------------------------------
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
    this->Function     = new double [4*this->FunctionSize];
    memcpy(this->Function, f->Function, 4*sizeof(double)*this->FunctionSize);
    }
  else
    {
    this->Function = NULL;
    }
  
  this->Modified();
}

//----------------------------------------------------------------------------
// Accelerate the mapping by copying the data in 32-bit chunks instead
// of 8-bit chunks.  The extra "long" argument is to help broken
// compilers select the non-templates below for unsigned char
// and unsigned short.
template <class T>
void vtkColorTransferFunctionMapData(vtkColorTransferFunction* self,
                                     T* input,
                                     unsigned char* output,
                                     int length, int inIncr,
                                     int outFormat, long)
{
  double          x;
  int            i = length;
  double          rgb[3];
  unsigned char  *optr = output;
  T              *iptr = input;
  unsigned char   alpha = (unsigned char)(self->GetAlpha()*255.0);
  
  if(self->GetSize() == 0)
    {
    vtkGenericWarningMacro("Transfer Function Has No Points!");
    return;
    }

  while (--i >= 0) 
    {
    x = (double) *iptr;
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
      *(optr++) = alpha;
      }
    iptr += inIncr;
    }
}



//----------------------------------------------------------------------------
// Special implementation for unsigned char input.
void vtkColorTransferFunctionMapData(vtkColorTransferFunction* self,
                                     unsigned char* input,
                                     unsigned char* output,
                                     int length, int inIncr,
                                     int outFormat, int)
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

//----------------------------------------------------------------------------
// Special implementation for unsigned short input.
void vtkColorTransferFunctionMapData(vtkColorTransferFunction* self,
                                     unsigned short* input,
                                     unsigned char* output,
                                     int length, int inIncr,
                                     int outFormat, int)
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

//----------------------------------------------------------------------------
void vtkColorTransferFunction::MapScalarsThroughTable2(void *input, 
                                                       unsigned char *output,
                                                       int inputDataType, 
                                                       int numberOfValues,
                                                       int inputIncrement,
                                                       int outputFormat)
{
  switch (inputDataType)
    {
    vtkTemplateMacro(
      vtkColorTransferFunctionMapData(this, static_cast<VTK_TT*>(input),
                                      output, numberOfValues, inputIncrement,
                                      outputFormat, 1)
      );
    default:
      vtkErrorMacro(<< "MapImageThroughTable: Unknown input ScalarType");
      return;
    }
}

//----------------------------------------------------------------------------
void vtkColorTransferFunction::FillFromDataPointer(int nb, double *ptr)
{
  if (nb <= 0 || !ptr)
    {
    return;
    }

  this->RemoveAllPoints();

  while (nb)
    {
    this->AddRGBPoint(ptr[0], ptr[1], ptr[2], ptr[3]);
    ptr += 4;
    nb--;
    }
}

//----------------------------------------------------------------------------
int vtkColorTransferFunction::AdjustRange(double range[2])
{
  if (!range)
    {
    return 0;
    }

  double *function_range = this->GetRange();
  
  // Make sure we have points at each end of the range

  double rgb[3];
  if (function_range[0] < range[0])
    {
    this->GetColor(range[0], rgb);
    this->AddRGBPoint(range[0], rgb[0], rgb[1], rgb[2]);
    }
  else
    {
    this->GetColor(function_range[0], rgb);
    this->AddRGBPoint(range[0], rgb[0], rgb[1], rgb[2]);
    }

  if (function_range[1] > range[1])
    {
    this->GetColor(range[1], rgb);
    this->AddRGBPoint(range[1], rgb[0], rgb[1], rgb[2]);
    }
  else
    {
    this->GetColor(function_range[1], rgb);
    this->AddRGBPoint(range[1], rgb[0], rgb[1], rgb[2]);
    }

  // Remove all points out-of-range

  int func_size = this->GetSize();
  double *func_ptr = this->GetDataPointer();
  
  int i;
  for (i = func_size - 1; i >= 0; i--)
    {
    double x = func_ptr[i * 4];
    if (x < range[0] || x > range[1])
      {
      this->RemovePoint(x);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
void vtkColorTransferFunction::SetColorSpaceToHSVNoWrap()
{
  this->SetColorSpace(VTK_CTF_HSV);
  this->SetHSVWrap(0);
};
#endif
