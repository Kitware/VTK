/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGlyphSource2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Tim Smith who sponsored and encouraged the development
             of this class.


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
#include "vtkGlyphSource2D.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"

//--------------------------------------------------------------------------
vtkGlyphSource2D* vtkGlyphSource2D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkGlyphSource2D");
  if(ret)
    {
    return (vtkGlyphSource2D*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkGlyphSource2D;
}

//----------------------------------------------------------------------------
vtkGlyphSource2D::vtkGlyphSource2D()
{
  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;
  this->Scale = 1.0;
  this->Scale2 = 1.5;
  this->Color[0] = 1.0;
  this->Color[1] = 1.0;
  this->Color[2] = 1.0;
  this->Filled = 1;
  this->Cross = 0;
  this->Dash = 0;
  this->RotationAngle = 0.0;
  this->GlyphType = VTK_VERTEX_GLYPH;
}

//----------------------------------------------------------------------------
void vtkGlyphSource2D::Execute()
{
  vtkPolyData *output = (vtkPolyData *)this->GetOutput();

  vtkDebugMacro(<<"Generating 2D glyph");
  
  //Allocate storage
  vtkPoints *pts = vtkPoints::New();
  pts->Allocate(6,6);
  vtkCellArray *verts = vtkCellArray::New();
  verts->Allocate(verts->EstimateSize(1,1),1);
  vtkCellArray *lines = vtkCellArray::New();
  lines->Allocate(lines->EstimateSize(4,2),2);
  vtkCellArray *polys = vtkCellArray::New();
  polys->Allocate(polys->EstimateSize(1,4),4);
  vtkUnsignedCharArray *colors = vtkUnsignedCharArray::New();
  colors->SetNumberOfComponents(3);
  colors->Allocate(2,2);
  
  this->ConvertColor();

  //Special options
  if ( this->Dash )
    {
    int filled = this->Filled;
    this->Filled = 0;
    this->CreateDash(pts,lines,polys,colors,this->Scale2);
    this->Filled = filled;
    }
  if ( this->Cross )
    {
    int filled = this->Filled;
    this->Filled = 0;
    this->CreateCross(pts,lines,polys,colors,this->Scale2);
    this->Filled = filled;
    }
  
  //Call the right function
  switch (this->GlyphType)
    {
    case VTK_NO_GLYPH:
      break;
    case VTK_VERTEX_GLYPH:
      this->CreateVertex(pts,verts,colors);
      break;
    case VTK_DASH_GLYPH:
      this->CreateDash(pts,lines,polys,colors,this->Scale);
      break;
    case VTK_CROSS_GLYPH:
      this->CreateCross(pts,lines,polys,colors,this->Scale);
      break;
    case VTK_THICKCROSS_GLYPH:
      this->CreateThickCross(pts,lines,polys,colors);
      break;
    case VTK_TRIANGLE_GLYPH:
      this->CreateTriangle(pts,lines,polys,colors);
      break;
    case VTK_SQUARE_GLYPH:
      this->CreateSquare(pts,lines,polys,colors);
      break;
    case VTK_CIRCLE_GLYPH:
      this->CreateCircle(pts,lines,polys,colors);
      break;
    case VTK_DIAMOND_GLYPH:
      this->CreateDiamond(pts,lines,polys,colors);
      break;
    case VTK_ARROW_GLYPH:
      this->CreateArrow(pts,lines,polys,colors);
      break;
    case VTK_THICKARROW_GLYPH:
      this->CreateThickArrow(pts,lines,polys,colors);
      break;
    case VTK_HOOKEDARROW_GLYPH:
      this->CreateHookedArrow(pts,lines,polys,colors);
      break;
    }
  
  this->TransformGlyph(pts);

  //Clean up
  output->SetPoints(pts);
  pts->Delete();

  output->SetVerts(verts);
  verts->Delete();
  
  output->SetLines(lines);
  lines->Delete();
  
  output->SetPolys(polys);
  polys->Delete();

  output->GetCellData()->SetScalars(colors);
  colors->Delete();
}

void vtkGlyphSource2D::ConvertColor()
{
  this->RGB[0] = (unsigned char) (255.0 * this->Color[0]);
  this->RGB[1] = (unsigned char) (255.0 * this->Color[1]);
  this->RGB[2] = (unsigned char) (255.0 * this->Color[2]);
}

void vtkGlyphSource2D::TransformGlyph(vtkPoints *pts)
{
  float x[3];
  int i;
  int numPts=pts->GetNumberOfPoints();
  
  if ( this->RotationAngle == 0.0 )
    {
    for (i=0; i<numPts; i++)
      {
      pts->GetPoint(i,x);
      x[0] = this->Center[0] + this->Scale * x[0];
      x[1] = this->Center[1] + this->Scale * x[1];
      pts->SetPoint(i,x);
      }
    }
  else
    {
    float angle = this->RotationAngle * vtkMath::DegreesToRadians();
    float xt;
    for (i=0; i<numPts; i++)
      {
      pts->GetPoint(i,x);
      xt = x[0]*cos(angle) - x[1]*sin(angle);
      x[1] = x[0]*sin(angle) + x[1]*cos(angle);
      x[0] = xt;
      x[0] = this->Center[0] + this->Scale * x[0];
      x[1] = this->Center[1] + this->Scale * x[1];
      pts->SetPoint(i,x);
      }
    }
}

void vtkGlyphSource2D::CreateVertex(vtkPoints *pts, vtkCellArray *verts,
                                    vtkUnsignedCharArray *colors)
{
  vtkIdType ptIds[1];
  ptIds[0] = pts->InsertNextPoint(0.0,0.0,0.0);
  verts->InsertNextCell(1,ptIds);
  colors->InsertNextValue(this->RGB[0]);
  colors->InsertNextValue(this->RGB[1]);
  colors->InsertNextValue(this->RGB[2]);
}

void vtkGlyphSource2D::CreateCross(vtkPoints *pts, vtkCellArray *lines,
                                   vtkCellArray *polys, vtkUnsignedCharArray *colors, 
                                   float scale)
{
  vtkIdType ptIds[4];

  if ( this->Filled )
    {
    this->CreateThickCross(pts,lines,polys,colors);
    }
  else
    {
    ptIds[0] = pts->InsertNextPoint(-0.5*scale, 0.0, 0.0);
    ptIds[1] = pts->InsertNextPoint( 0.5*scale, 0.0, 0.0);
    lines->InsertNextCell(2,ptIds);
    colors->InsertNextValue(this->RGB[0]);
    colors->InsertNextValue(this->RGB[1]);
    colors->InsertNextValue(this->RGB[2]);
    ptIds[0] = pts->InsertNextPoint(0.0, -0.5*scale, 0.0);
    ptIds[1] = pts->InsertNextPoint(0.0,  0.5*scale, 0.0);
    lines->InsertNextCell(2,ptIds);
    colors->InsertNextValue(this->RGB[0]);
    colors->InsertNextValue(this->RGB[1]);
    colors->InsertNextValue(this->RGB[2]);
    }
}

void vtkGlyphSource2D::CreateThickCross(vtkPoints *pts, vtkCellArray *lines,
                                        vtkCellArray *polys, vtkUnsignedCharArray *colors)
{
  if ( this->Filled )
    {
    vtkIdType ptIds[4];
    ptIds[0] = pts->InsertNextPoint(-0.5, -0.1, 0.0);
    ptIds[1] = pts->InsertNextPoint( 0.5, -0.1, 0.0);
    ptIds[2] = pts->InsertNextPoint( 0.5,  0.1, 0.0);
    ptIds[3] = pts->InsertNextPoint(-0.5,  0.1, 0.0);
    polys->InsertNextCell(4,ptIds);
    colors->InsertNextValue(this->RGB[0]);
    colors->InsertNextValue(this->RGB[1]);
    colors->InsertNextValue(this->RGB[2]);
    ptIds[0] = pts->InsertNextPoint(-0.1, -0.5, 0.0);
    ptIds[1] = pts->InsertNextPoint( 0.1, -0.5, 0.0);
    ptIds[2] = pts->InsertNextPoint( 0.1,  0.5, 0.0);
    ptIds[3] = pts->InsertNextPoint(-0.1,  0.5, 0.0);
    polys->InsertNextCell(4,ptIds);
    colors->InsertNextValue(this->RGB[0]);
    colors->InsertNextValue(this->RGB[1]);
    colors->InsertNextValue(this->RGB[2]);
    }
  else
    {
    vtkIdType ptIds[13];
    ptIds[0] = pts->InsertNextPoint(-0.5, -0.1, 0.0);
    ptIds[1] = pts->InsertNextPoint(-0.1, -0.1, 0.0);
    ptIds[2] = pts->InsertNextPoint(-0.1, -0.5, 0.0);
    ptIds[3] = pts->InsertNextPoint( 0.1, -0.5, 0.0);
    ptIds[4] = pts->InsertNextPoint( 0.1, -0.1, 0.0);
    ptIds[5] = pts->InsertNextPoint( 0.5, -0.1, 0.0);
    ptIds[6] = pts->InsertNextPoint( 0.5,  0.1, 0.0);
    ptIds[7] = pts->InsertNextPoint( 0.1,  0.1, 0.0);
    ptIds[8] = pts->InsertNextPoint( 0.1,  0.5, 0.0);
    ptIds[9] = pts->InsertNextPoint(-0.1,  0.5, 0.0);
    ptIds[10] = pts->InsertNextPoint(-0.1, 0.1, 0.0);
    ptIds[11] = pts->InsertNextPoint(-0.5, 0.1, 0.0);
    ptIds[12] = ptIds[0];
    lines->InsertNextCell(13,ptIds);
    colors->InsertNextValue(this->RGB[0]);
    colors->InsertNextValue(this->RGB[1]);
    colors->InsertNextValue(this->RGB[2]);
    }
}

void vtkGlyphSource2D::CreateTriangle(vtkPoints *pts, vtkCellArray *lines,
                                      vtkCellArray *polys, vtkUnsignedCharArray *colors)
{
  vtkIdType ptIds[4];

  ptIds[0] = pts->InsertNextPoint(-0.375, -0.25, 0.0);
  ptIds[1] = pts->InsertNextPoint( 0.0,  0.5, 0.0);
  ptIds[2] = pts->InsertNextPoint( 0.375, -0.25, 0.0);

  if ( this->Filled )
    {
    polys->InsertNextCell(3,ptIds);
    }
  else
    {
    ptIds[3] = ptIds[0];
    lines->InsertNextCell(4,ptIds);
    }
  colors->InsertNextValue(this->RGB[0]);
  colors->InsertNextValue(this->RGB[1]);
  colors->InsertNextValue(this->RGB[2]);
}

void vtkGlyphSource2D::CreateSquare(vtkPoints *pts, vtkCellArray *lines,
                                    vtkCellArray *polys, vtkUnsignedCharArray *colors)
{
  vtkIdType ptIds[5];

  ptIds[0] = pts->InsertNextPoint(-0.5, -0.5, 0.0);
  ptIds[1] = pts->InsertNextPoint( 0.5, -0.5, 0.0);
  ptIds[2] = pts->InsertNextPoint( 0.5,  0.5, 0.0);
  ptIds[3] = pts->InsertNextPoint(-0.5,  0.5, 0.0);

  if ( this->Filled )
    {
    polys->InsertNextCell(4,ptIds);
    }
  else
    {
    ptIds[4] = ptIds[0];
    lines->InsertNextCell(5,ptIds);
    }
  colors->InsertNextValue(this->RGB[0]);
  colors->InsertNextValue(this->RGB[1]);
  colors->InsertNextValue(this->RGB[2]);
}

void vtkGlyphSource2D::CreateCircle(vtkPoints *pts, vtkCellArray *lines,
                                    vtkCellArray *polys, vtkUnsignedCharArray *colors)
{
  vtkIdType ptIds[9];
  float x[3], theta;

  // generate eight points in a circle
  x[2] = 0.0;
  theta = 2.0 * vtkMath::Pi() / 8.0;
  for (int i=0; i<8; i++)
    {
    x[0] = 0.5 * cos((double)i*theta);
    x[1] = 0.5 * sin((double)i*theta);
    ptIds[i] = pts->InsertNextPoint(x);
    }
  
  if ( this->Filled )
    {
    polys->InsertNextCell(8,ptIds);
    }
  else
    {
    ptIds[8] = ptIds[0];
    lines->InsertNextCell(9,ptIds);
    }
  colors->InsertNextValue(this->RGB[0]);
  colors->InsertNextValue(this->RGB[1]);
  colors->InsertNextValue(this->RGB[2]);
}

void vtkGlyphSource2D::CreateDiamond(vtkPoints *pts, vtkCellArray *lines,
                                     vtkCellArray *polys, vtkUnsignedCharArray *colors)
{
  vtkIdType ptIds[5];

  ptIds[0] = pts->InsertNextPoint( 0.0, -0.5, 0.0);
  ptIds[1] = pts->InsertNextPoint( 0.5,  0.0, 0.0);
  ptIds[2] = pts->InsertNextPoint( 0.0,  0.5, 0.0);
  ptIds[3] = pts->InsertNextPoint(-0.5,  0.0, 0.0);

  if ( this->Filled )
    {
    polys->InsertNextCell(4,ptIds);
    }
  else
    {
    ptIds[4] = ptIds[0];
    lines->InsertNextCell(5,ptIds);
    }
  colors->InsertNextValue(this->RGB[0]);
  colors->InsertNextValue(this->RGB[1]);
  colors->InsertNextValue(this->RGB[2]);
}

void vtkGlyphSource2D::CreateArrow(vtkPoints *pts, vtkCellArray *lines,
                                   vtkCellArray *polys, vtkUnsignedCharArray *colors)
{
  if ( this->Filled ) //create two convex polygons
    {
    this->CreateThickArrow(pts,lines,polys,colors);
    }
  else
    {
    //stem
    vtkIdType ptIds[3];
    ptIds[0] = pts->InsertNextPoint( -0.5, 0.0, 0.0);
    ptIds[1] = pts->InsertNextPoint(  0.5, 0.0, 0.0);
    lines->InsertNextCell(2,ptIds);
    colors->InsertNextValue(this->RGB[0]);
    colors->InsertNextValue(this->RGB[1]);
    colors->InsertNextValue(this->RGB[2]);

    //arrow head
    ptIds[0] = pts->InsertNextPoint( 0.2, -0.1, 0.0);
    ptIds[1] = pts->InsertNextPoint( 0.5,  0.0, 0.0);
    ptIds[2] = pts->InsertNextPoint( 0.2,  0.1, 0.0);
    lines->InsertNextCell(3,ptIds);
    colors->InsertNextValue(this->RGB[0]);
    colors->InsertNextValue(this->RGB[1]);
    colors->InsertNextValue(this->RGB[2]);
    }
}

void vtkGlyphSource2D::CreateThickArrow(vtkPoints *pts, vtkCellArray *lines,
                                         vtkCellArray *polys, vtkUnsignedCharArray *colors)
{
  vtkIdType ptIds[8];

  ptIds[0] = pts->InsertNextPoint( -0.5, -0.1, 0.0);
  ptIds[1] = pts->InsertNextPoint(  0.1, -0.1, 0.0);
  ptIds[2] = pts->InsertNextPoint(  0.1, -0.2, 0.0);
  ptIds[3] = pts->InsertNextPoint(  0.5,  0.0, 0.0);
  ptIds[4] = pts->InsertNextPoint(  0.1,  0.2, 0.0);
  ptIds[5] = pts->InsertNextPoint(  0.1,  0.1, 0.0);
  ptIds[6] = pts->InsertNextPoint( -0.5,  0.1, 0.0);

  if ( this->Filled ) //create two convex polygons
    {
    polys->InsertNextCell(4);
    polys->InsertCellPoint(ptIds[0]);
    polys->InsertCellPoint(ptIds[1]);
    polys->InsertCellPoint(ptIds[5]);
    polys->InsertCellPoint(ptIds[6]);
    colors->InsertNextValue(this->RGB[0]);
    colors->InsertNextValue(this->RGB[1]);
    colors->InsertNextValue(this->RGB[2]);

    polys->InsertNextCell(5,ptIds+1);
    colors->InsertNextValue(this->RGB[0]);
    colors->InsertNextValue(this->RGB[1]);
    colors->InsertNextValue(this->RGB[2]);
    }
  else
    {
    ptIds[7] = ptIds[0];
    lines->InsertNextCell(8,ptIds);
    colors->InsertNextValue(this->RGB[0]);
    colors->InsertNextValue(this->RGB[1]);
    colors->InsertNextValue(this->RGB[2]);
    }
}

void vtkGlyphSource2D::CreateHookedArrow(vtkPoints *pts, vtkCellArray *lines,
                                         vtkCellArray *polys, vtkUnsignedCharArray *colors)
{
  if ( this->Filled )
    {
    //create two convex polygons
    vtkIdType ptIds[4];
    ptIds[0] = pts->InsertNextPoint( -0.5,   -0.1, 0.0);
    ptIds[1] = pts->InsertNextPoint(  0.1,   -0.1, 0.0);
    ptIds[2] = pts->InsertNextPoint(  0.1,  0.075, 0.0);
    ptIds[3] = pts->InsertNextPoint( -0.5,  0.075, 0.0);
    polys->InsertNextCell(4,ptIds);
    colors->InsertNextValue(this->RGB[0]);
    colors->InsertNextValue(this->RGB[1]);
    colors->InsertNextValue(this->RGB[2]);

    ptIds[0] = pts->InsertNextPoint( 0.1, -0.1, 0.0);
    ptIds[1] = pts->InsertNextPoint( 0.5, -0.1, 0.0);
    ptIds[2] = pts->InsertNextPoint( 0.1,  0.2, 0.0);
    polys->InsertNextCell(3,ptIds);
    colors->InsertNextValue(this->RGB[0]);
    colors->InsertNextValue(this->RGB[1]);
    colors->InsertNextValue(this->RGB[2]);
    }
  else
    {
    vtkIdType ptIds[3];
    ptIds[0] = pts->InsertNextPoint( -0.5, 0.0, 0.0);
    ptIds[1] = pts->InsertNextPoint(  0.5, 0.0, 0.0);
    ptIds[2] = pts->InsertNextPoint(  0.2, 0.1, 0.0);
    lines->InsertNextCell(3,ptIds);
    colors->InsertNextValue(this->RGB[0]);
    colors->InsertNextValue(this->RGB[1]);
    colors->InsertNextValue(this->RGB[2]);
    }
}

void vtkGlyphSource2D::CreateDash(vtkPoints *pts, vtkCellArray *lines,
                                  vtkCellArray *polys, vtkUnsignedCharArray *colors,
								  float scale)
{
  vtkIdType ptIds[5];
  ptIds[0] = pts->InsertNextPoint(-0.5, -0.1, 0.0);
  ptIds[1] = pts->InsertNextPoint( 0.5, -0.1, 0.0);
  ptIds[2] = pts->InsertNextPoint( 0.5,  0.1, 0.0);
  ptIds[3] = pts->InsertNextPoint(-0.5,  0.1, 0.0);

  if ( this->Filled )
    {
    polys->InsertNextCell(4,ptIds);
    }
  else
    {
    vtkIdType ptIds2D[2];
    ptIds2D[0] = pts->InsertNextPoint(-0.5*scale, 0.0, 0.0);
    ptIds2D[1] = pts->InsertNextPoint( 0.5*scale, 0.0, 0.0);
    colors->InsertNextValue(this->RGB[0]);
    colors->InsertNextValue(this->RGB[1]);
    colors->InsertNextValue(this->RGB[2]);
    lines->InsertNextCell(2,ptIds2D);
    }
  colors->InsertNextValue(this->RGB[0]);
  colors->InsertNextValue(this->RGB[1]);
  colors->InsertNextValue(this->RGB[2]);
}


//----------------------------------------------------------------------------
void vtkGlyphSource2D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataSource::PrintSelf(os,indent);

  os << indent << "Center: (" << this->Center[0] << ", " 
     << this->Center[1] << ", " << this->Center[2] << ")\n";

  os << indent << "Scale: " << this->Scale << "\n";
  os << indent << "Scale2: " << this->Scale2 << "\n";
  os << indent << "Rotation Angle: " << this->RotationAngle << "\n";

  os << indent << "Color: (" << this->Color[0] << ", " 
     << this->Color[1] << ", " << this->Color[2] << ")\n";

  os << indent << "Filled: " << (this->Filled ? "On\n" : "Off\n");
  os << indent << "Dash: " << (this->Dash ? "On\n" : "Off\n");
  os << indent << "Cross: " << (this->Cross ? "On\n" : "Off\n");

  os << indent << "Glyph Type";
  switch (this->GlyphType)
    {
    case VTK_NO_GLYPH:
      os << "No Glyph\n";
      break;
    case VTK_VERTEX_GLYPH:
      os << "Vertex\n";
      break;
    case VTK_DASH_GLYPH:
      os << "Dash\n";
      break;
    case VTK_CROSS_GLYPH:
      os << "Cross\n";
      break;
    case VTK_THICKCROSS_GLYPH:
      os << "Cross\n";
      break;
    case VTK_TRIANGLE_GLYPH:
      os << "Triangle\n";
      break;
    case VTK_SQUARE_GLYPH:
      os << "Square\n";
      break;
    case VTK_CIRCLE_GLYPH:
      os << "Circle\n";
      break;
    case VTK_DIAMOND_GLYPH:
      os << "Diamond\n";
      break;
    case VTK_ARROW_GLYPH:
      os << "Arrow\n";
      break;
    case VTK_THICKARROW_GLYPH:
      os << "Arrow\n";
      break;
    case VTK_HOOKEDARROW_GLYPH:
      os << "Hooked Arrow\n";
      break;
    }
}

