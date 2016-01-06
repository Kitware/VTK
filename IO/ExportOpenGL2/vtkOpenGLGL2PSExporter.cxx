/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLGL2PSExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLGL2PSExporter.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkOpenGLGL2PSExporter)

//------------------------------------------------------------------------------
void vtkOpenGLGL2PSExporter::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkOpenGLGL2PSExporter::vtkOpenGLGL2PSExporter()
{
}

//------------------------------------------------------------------------------
vtkOpenGLGL2PSExporter::~vtkOpenGLGL2PSExporter()
{
}

//------------------------------------------------------------------------------
void vtkOpenGLGL2PSExporter::WriteData()
{
  vtkErrorMacro(<<"Not yet implemented.");
}
