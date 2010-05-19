/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPPCAStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkToolkits.h"

#include "vtkPPCAStatistics.h"

#include "vtkCommunicator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"
#include "vtkTable.h"
#include "vtkPMultiCorrelativeStatistics.h"
#include "vtkVariant.h"

#include <vtkstd/map>

vtkStandardNewMacro(vtkPPCAStatistics);
vtkCxxSetObjectMacro(vtkPPCAStatistics, Controller, vtkMultiProcessController);
//-----------------------------------------------------------------------------
vtkPPCAStatistics::vtkPPCAStatistics()
{
  this->Controller = 0;
  this->SetController( vtkMultiProcessController::GetGlobalController() );
}

//-----------------------------------------------------------------------------
vtkPPCAStatistics::~vtkPPCAStatistics()
{
  this->SetController( 0 );
}

//-----------------------------------------------------------------------------
void vtkPPCAStatistics::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

// ----------------------------------------------------------------------
void vtkPPCAStatistics::Learn( vtkTable* inData,
                                      vtkTable* inParameters,
                                      vtkDataObject* outMetaDO )
{
  vtkMultiBlockDataSet* outMeta = vtkMultiBlockDataSet::SafeDownCast( outMetaDO );
  if ( ! outMeta )
    {
    return;
    }

  // First calculate correlative statistics on local data set
  this->Superclass::Learn( inData, inParameters, outMeta );

  // Get a hold of the (sparse) covariance matrix
  vtkTable* sparseCov = vtkTable::SafeDownCast( outMeta->GetBlock( 0 ) );
  if ( ! sparseCov )
    {
    return;
    }

  vtkPMultiCorrelativeStatistics::GatherStatistics( this->Controller, sparseCov );
}
