/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProperty2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

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

#include "vtkProperty2D.h"
#include "vtkObjectFactory.h"

//-------------------------------------------------------------------------
vtkProperty2D* vtkProperty2D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkProperty2D");
  if(ret)
    {
    return (vtkProperty2D*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkProperty2D;
}

// Creates a vtkProperty2D with the following default values:
// Opacity 1, Color (1,0,0), CompositingOperator VTK_SRC
vtkProperty2D::vtkProperty2D()
{
  this->Opacity = 1.0;
  this->PointSize = 1.0;
  this->LineWidth = 1.0;
  this->LineStipplePattern = 0xFFFF;
  this->LineStippleRepeatFactor = 1;
  this->Color[0] = 1.0;
  this->Color[1] = 1.0;
  this->Color[2] = 1.0;  
  this->DisplayLocation = VTK_FOREGROUND_LOCATION;
}

vtkProperty2D::~vtkProperty2D()
{
}

// Assign one property to another. 
void vtkProperty2D::DeepCopy(vtkProperty2D *p)
{
  if ( p != NULL )
    {
    this->SetColor(p->GetColor());
    this->SetOpacity(p->GetOpacity());
    this->SetPointSize(p->GetPointSize());
    this->SetLineWidth(p->GetLineWidth());
    this->SetLineStipplePattern(p->GetLineStipplePattern());
    this->SetLineStippleRepeatFactor(p->GetLineStippleRepeatFactor());
    this->SetDisplayLocation(p->GetDisplayLocation());
    }
}

void vtkProperty2D::PrintSelf(ostream& os, vtkIndent indent)
{

  this->vtkObject::PrintSelf(os, indent);

  os << indent << "Opacity: " << this->Opacity << "\n";
  os << indent << "Color: (" << this->Color[0] << ", "
     << this->Color[1] << ", "
     << this->Color[2] << ")\n";
  os << indent << "Point size: " << this->PointSize << "\n";
  os << indent << "Line width: " << this->LineWidth << "\n";
  os << indent << "Line stipple pattern: " << this->LineStipplePattern << "\n";
  os << indent << "Line stipple repeat factor: " << this->LineStippleRepeatFactor << "\n";
  switch ( this->DisplayLocation )
    {
    case VTK_FOREGROUND_LOCATION:
      os << indent << "Display location: foreground\n";
      break;
    case VTK_BACKGROUND_LOCATION:
      os << indent << "Display location: background\n";
      break;
    default:
      os << indent << "Display location: invalid\n";
      break;      
    }
    
}




