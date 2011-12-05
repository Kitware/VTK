/*=========================================================================

Program:   Visualization Toolkit
Module:    $RCSfile: vtkYoungsMaterialInterface.cxx,v $

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// This file is part of the generalized Youngs material interface reconstruction algorithm contributed by
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br>
// BP12, F-91297 Arpajon, France. <br>
// Implementation by Thierry Carrard and Philippe Pebay

#include "vtkPYoungsMaterialInterface.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"

vtkStandardNewMacro(vtkPYoungsMaterialInterface);
vtkCxxSetObjectMacro(vtkPYoungsMaterialInterface, Controller, vtkMultiProcessController);
//-----------------------------------------------------------------------------
vtkPYoungsMaterialInterface::vtkPYoungsMaterialInterface()
{
  this->Controller = 0 ;
  this->SetController( vtkMultiProcessController::GetGlobalController() );

  vtkDebugMacro(<<"vtkPYoungsMaterialInterface::vtkPYoungsMaterialInterface() ok\n");
}

//-----------------------------------------------------------------------------
vtkPYoungsMaterialInterface::~vtkPYoungsMaterialInterface()
{
  this->SetController( 0 );
}

//-----------------------------------------------------------------------------
void vtkPYoungsMaterialInterface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller: " << this->Controller << endl;
}
