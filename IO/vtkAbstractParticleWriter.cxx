/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractParticleWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAbstractParticleWriter.h"

vtkCxxRevisionMacro(vtkAbstractParticleWriter, "1.1");

// Construct with no start and end write methods or arguments.
vtkAbstractParticleWriter::vtkAbstractParticleWriter()
{
  this->TimeStep = 0;
  this->FileName = NULL;
}

vtkAbstractParticleWriter::~vtkAbstractParticleWriter()
{
  if (this->FileName)
    {
    delete []this->FileName;
    }
}

void vtkAbstractParticleWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "TimeStep: " << this->TimeStep << endl;
  os << indent << "FileName: " << 
    (this->FileName ? this->FileName : "NONE") << endl;
}
