/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCompositer.h"
#include "vtkObjectFactory.h"
#include "vtkToolkits.h"

vtkCxxRevisionMacro(vtkCompositer, "1.1");
vtkStandardNewMacro(vtkCompositer);

//-------------------------------------------------------------------------
vtkCompositer::vtkCompositer()
{
  this->Controller = vtkMultiProcessController::GetGlobalController();
  this->Controller->Register(this);
}
  
//-------------------------------------------------------------------------
vtkCompositer::~vtkCompositer()
{
  this->SetController(NULL);
}

//-------------------------------------------------------------------------
void vtkCompositer::CompositeBuffer(vtkDataArray *pBuf, vtkFloatArray *zBuf,
				                            vtkDataArray *pTmp, vtkFloatArray *zTmp)
{
  pBuf = pBuf;
  zBuf = zBuf;
  pTmp = pTmp;
  zTmp = zTmp;
}

//-------------------------------------------------------------------------
void vtkCompositer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
}



