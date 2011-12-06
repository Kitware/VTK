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
void vtkPYoungsMaterialInterface::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller: " << this->Controller << endl;
}

//-----------------------------------------------------------------------------
void vtkPYoungsMaterialInterface::Aggregate( int nmat, int* inputsPerMaterial )
{
  vtkIdType nprocs = this->Controller->GetNumberOfProcesses();
  if ( nprocs < 2 )
    {
    return;
    }

  // Now get ready for parallel calculations
  vtkCommunicator* com = this->Controller->GetCommunicator();
  if ( ! com )
    {
    vtkErrorMacro(<<"No parallel communicator.");
    }

  // Gather inputs per material from all processes
  vtkIdType myid = this->Controller->GetLocalProcessId();
  int* tmp = new int[nmat * nprocs];
  com->AllGather( inputsPerMaterial, tmp, nmat );

  // Scan sum : done by all processes, not optimal but easy
  for ( vtkIdType m = 0; m < nmat; ++ m )
    {
    for( vtkIdType p = 1; p < nprocs; ++ p )
      {
      vtkIdType pnmat = p * nmat + m;
      tmp[pnmat] += tmp[pnmat - nmat];
      }
    }

  vtkIdType offset = (nprocs - 1) * nmat;
  this->NumberOfDomains = 0;
  for ( int m = 0; m < nmat; ++ m )
    {
    // Sum all counts from all processes
    int inputsPerMaterialSum = tmp[offset + m];
    if( inputsPerMaterialSum > this->NumberOfDomains )
      {
      this->NumberOfDomains = inputsPerMaterialSum;
      }

    // Calculate partial sum of all preceding processors
    inputsPerMaterial[m] = ( myid ? tmp[( myid - 1) * nmat + m] : 0 );
    }
  delete[] tmp;
}
