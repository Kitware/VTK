
#include "vtkColorTransferFunction.h"

// Description:
// Construct a new vtkColorTransferFunction with default values
vtkColorTransferFunction::vtkColorTransferFunction()
{
  this->ColorValue[0] = 0;
  this->ColorValue[1] = 0;
  this->ColorValue[2] = 0;

  this->Range[0] = 0;
  this->Range[1] = 0;
}

// Description:
// Destruct a vtkColorTransferFunction
vtkColorTransferFunction::~vtkColorTransferFunction()
{
}

// Description:
// Returns the sum of the number of function points used to specify 
// the three independent functions (R,G,B)
int vtkColorTransferFunction::GetTotalSize()
{
  int  size;

  // Sum all three function sizes
  size = this->Red.GetSize() +
         this->Green.GetSize() +
         this->Blue.GetSize();

  return( size );
}

// Description:
// Add a point to the red function
void vtkColorTransferFunction::AddRedPoint( float x, float r )
{
  this->Red.AddPoint( x, r );

  this->UpdateRange();
}

// Description:
// Add a point to the green function
void vtkColorTransferFunction::AddGreenPoint( float x, float g )
{
  this->Green.AddPoint( x, g );

  this->UpdateRange();
}

// Description:
// Add a point to the blue function
void vtkColorTransferFunction::AddBluePoint( float x, float b )
{
  this->Blue.AddPoint( x, b );

  this->UpdateRange();
}

// Description:
// Add a point to all three functions (RGB)
void vtkColorTransferFunction::AddRGBPoint( float x, float r,
     float g, float b )
{
  this->Red.AddPoint( x, r );
  this->Green.AddPoint( x, g );
  this->Blue.AddPoint( x, b );

  this->UpdateRange();
}

// Description:
// Remove a point from the red function
void vtkColorTransferFunction::RemoveRedPoint( float x )
{
  this->Red.RemovePoint( x );

  this->UpdateRange();
}

// Description:
// Remove a point from the green function
void vtkColorTransferFunction::RemoveGreenPoint( float x )
{
  this->Green.RemovePoint( x );

  this->UpdateRange();
}

// Description:
// Remove a point from the blue function
void vtkColorTransferFunction::RemoveBluePoint( float x )
{
  this->Blue.RemovePoint( x );

  this->UpdateRange();
}

// Description:
// Remove a point from all three functions (RGB)
void vtkColorTransferFunction::RemoveRGBPoint( float x )
{
  this->Red.RemovePoint( x );
  this->Green.RemovePoint( x );
  this->Blue.RemovePoint( x );

  this->UpdateRange();
}

// Description:
// Remove all points from all three functions (RGB)
void vtkColorTransferFunction::RemoveAllPoints()
{
  this->Red.RemoveAllPoints();
  this->Green.RemoveAllPoints();
  this->Blue.RemoveAllPoints();

  this->UpdateRange();
}

// Description:
// Add a line to the red function
void vtkColorTransferFunction::AddRedSegment( float x1, float r1,
     float x2, float r2 )
{
  this->Red.AddSegment( x1, r1, x2, r2 );

  this->UpdateRange();
}

// Description:
// Add a line to the green function
void vtkColorTransferFunction::AddGreenSegment( float x1, float g1,
     float x2, float g2 )
{
  this->Green.AddSegment( x1, g1, x2, g2 );

  this->UpdateRange();
}

// Description:
// Add a line to the blue function
void vtkColorTransferFunction::AddBlueSegment( float x1, float b1,
     float x2, float b2 )
{
  this->Blue.AddSegment( x1, b1, x2, b2 );

  this->UpdateRange();
}

// Description:
// Add a line to all three functions (RGB)
void vtkColorTransferFunction::AddRGBSegment( float x1, float r1, 
        float g1, float b1, float x2, float r2, float g2, float b2 )
{
  this->Red.AddSegment( x1, r1, x2, r2 );
  this->Green.AddSegment( x1, g1, x2, g2 );
  this->Blue.AddSegment( x1, b1, x2, b2 );

  this->UpdateRange();
}

// Description:
// Returns the RGB color evaluated at the specified location
float *vtkColorTransferFunction::GetValue( float x )
{
  this->ColorValue[0] = this->Red.GetValue( x );
  this->ColorValue[1] = this->Green.GetValue( x );
  this->ColorValue[2] = this->Blue.GetValue( x );

  return( this->ColorValue );
}

// Description:
// Updates the min/max range for all three functions (RGB)
void vtkColorTransferFunction::UpdateRange()
{
  float *red_range, *green_range, *blue_range;

  red_range   = this->Red.GetRange();
  green_range = this->Green.GetRange();
  blue_range  = this->Blue.GetRange();

  if( red_range[0] < this->Range[0] )
    this->Range[0] = red_range[0];

  if( green_range[0] < this->Range[0] )
    this->Range[0] = green_range[0];

  if( blue_range[0] < this->Range[0] )
    this->Range[0] = blue_range[0];

  if( red_range[1] > this->Range[1] )
    this->Range[1] = red_range[1];

  if( green_range[1] > this->Range[1] )
    this->Range[1] = green_range[1];

  if( blue_range[1] > this->Range[1] )
    this->Range[1] = blue_range[1];
  
  this->Modified();
}

// Description:
// Returns the min/max range for all three functions
float *vtkColorTransferFunction::GetRange()
{
  return( this->Range );
}

// Description:
// Returns a table of RGB colors at regular intervals along the function
void vtkColorTransferFunction::GetTable( float x1, float x2, 
						   int size, float* table )
{
  float x;
  float inc;
  int   i;

  if( x1 == x2 )
    return;

  x = x1;
  if( size > 1 )
    inc = (x2-x1)/(float)(size-1);
  else
    inc = 0;

  for( i=0; i<size; i++ )
    {
    table[i*3  ] = this->Red.GetValue( x );
    table[i*3+1] = this->Green.GetValue( x );
    table[i*3+2] = this->Blue.GetValue( x );
    x += inc;
    }
}

// Description:
// Print method for vtkColorTransferFunction
void vtkColorTransferFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os, indent);

  os << indent << "Color Transfer Function Total Points: " << this->GetTotalSize() << "\n";

  os << indent << "Red Transfer Function: ";
  os << this->Red << "\n";

  os << indent << "Green Transfer Function: ";
  os << this->Green << "\n";

  os << indent << "Blue Transfer Function: ";
  os << this->Blue << "\n";

}

