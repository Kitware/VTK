/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProperty2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

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

#include "vtkProperty2D.h"

// Description:
// Creates a vtkProperty2D with the following default values:
// Opacity 1, Color (1,0,0), CompositingOperator VTK_SRC
vtkProperty2D::vtkProperty2D()
{
  this->Opacity = 1.0;
  this->Color[0] = 1.0;
  this->Color[1] = 0.0;
  this->Color[2] = 0.0;  
  this->CompositingOperator = VTK_SRC;
}

vtkProperty2D::~vtkProperty2D()
{

}

void vtkProperty2D::PrintSelf(ostream& os, vtkIndent indent)
{

  this->vtkObject::PrintSelf(os, indent);

  os << indent << "Opacity: " << this->Opacity << "\n";
  os << indent << "Color: (" << this->Color[0] << ", "
			     << this->Color[1] << ", "
			     << this->Color[2] << ")\n";

  char compString[100];

  switch (this->CompositingOperator)
    {
    case VTK_BLACK:
	strcpy(compString, "VTK_BLACK\0");
	break;
    case VTK_NOT_DEST:
        strcpy(compString, "VTK_NOT_DEST\0");
 	break;
    case VTK_SRC_AND_DEST:
	strcpy(compString, "VTK_SRC_AND_DEST\0");
	break;
    case VTK_SRC_OR_DEST:
	strcpy(compString, "VTK_SRC_OR_DEST\0");
	break;
    case VTK_NOT_SRC:
	strcpy(compString, "VTK_NOT_SRC\0");
	break;
    case VTK_SRC_XOR_DEST:
	strcpy(compString, "VTK_SRC_XOR_DEST\0");
	break;
    case VTK_SRC_AND_notDEST:
	strcpy(compString, "VTK_SRC_AND_notDEST\0");
	break;
    case VTK_SRC:
	strcpy(compString, "VTK_SRC\0");
	break;
    case VTK_WHITE:
 	strcpy(compString, "VTK_WHITE\0");
	break;
    default:
	strcpy(compString, "UNKNOWN!\0");
	break;
    }

  os << indent << "Compositing Operator: " << compString << "\n";
}




