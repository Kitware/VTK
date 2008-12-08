/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPMultiCorrelativeStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkToolkits.h"

#include "vtkPMultiCorrelativeStatistics.h"

#include "vtkCommunicator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"
#include "vtkTable.h"
#include "vtkVariant.h"

vtkStandardNewMacro(vtkPMultiCorrelativeStatistics);
vtkCxxRevisionMacro(vtkPMultiCorrelativeStatistics, "1.1");
vtkCxxSetObjectMacro(vtkPMultiCorrelativeStatistics, Controller, vtkMultiProcessController);
//-----------------------------------------------------------------------------
vtkPMultiCorrelativeStatistics::vtkPMultiCorrelativeStatistics()
{
  this->Controller = 0;
  this->SetController( vtkMultiProcessController::GetGlobalController() );
}

//-----------------------------------------------------------------------------
vtkPMultiCorrelativeStatistics::~vtkPMultiCorrelativeStatistics()
{
  this->SetController( 0 );
}

//-----------------------------------------------------------------------------
void vtkPMultiCorrelativeStatistics::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

// ----------------------------------------------------------------------
void vtkPMultiCorrelativeStatistics::ExecuteLearn( vtkTable* inData,
                                                   vtkDataObject* outMetaDO )
{
  vtkTable* outMeta = vtkTable::SafeDownCast( outMetaDO ); 
  if ( ! outMeta ) 
    { 
    return; 
    } 

  // First calculate correlative statistics on local data set
  this->Superclass::ExecuteLearn( inData, outMeta );

  vtkIdType nRow = outMeta->GetNumberOfRows();
  if ( ! nRow )
    {
    // No statistics were calculated.
    return;
    }

  // Make sure that parallel updates are needed, otherwise leave it at that.
  int np = this->Controller->GetNumberOfProcesses();
  if ( np < 2 )
    {
    return;
    }

  // Now get ready for parallel calculations
  vtkCommunicator* com = this->Controller->GetCommunicator();
  
  // (All) gather all sample sizes
  int n_l = this->SampleSize;
  int* n_g = new int[np];
  com->AllGather( &n_l, n_g, 1 ); 
  
  // Parallel updates not implemented yet

  delete [] n_g;
}
