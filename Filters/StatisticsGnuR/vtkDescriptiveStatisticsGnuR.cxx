/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDescriptiveStatisticsGnuR.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkToolkits.h"

#include "vtkDescriptiveStatisticsGnuR.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkRInterface.h"
#include "vtkStringArray.h"

#include <sstream>

vtkStandardNewMacro(vtkDescriptiveStatisticsGnuR);

// ----------------------------------------------------------------------
vtkDescriptiveStatisticsGnuR::vtkDescriptiveStatisticsGnuR()
{
}

// ----------------------------------------------------------------------
vtkDescriptiveStatisticsGnuR::~vtkDescriptiveStatisticsGnuR()
{
}

// ----------------------------------------------------------------------
void vtkDescriptiveStatisticsGnuR::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

vtkDoubleArray* vtkDescriptiveStatisticsGnuR::CalculatePValues(vtkDoubleArray* statCol)
{
  // Prepare VTK - R interface
  vtkRInterface* ri = vtkRInterface::New();

  // Use the calculated Jarque-Bera statistics as input to the Chi square function
  ri->AssignVTKDataArrayToRVariable( statCol, "jb" );

  // Calculate the p-values (p+1=2 degrees of freedom)
  ri->EvalRscript( "p=1-pchisq(jb,2)" );

  // Retrieve the p-values
  vtkDoubleArray* testCol = vtkArrayDownCast<vtkDoubleArray>( ri->AssignRVariableToVTKDataArray( "p" ) );
  if ( ! testCol || testCol->GetNumberOfTuples() != statCol->GetNumberOfTuples() )
  {
    vtkWarningMacro( "Something went wrong with the R calculations. Reported p-values will be invalid." );
    testCol = this->Superclass::CalculatePValues( statCol );
  }
  else
  {
    // increment ref count on testCol so its not cleaned up when the R interface goes away
    testCol->Register(NULL);
  }

  // Clean up
  ri->Delete();

  return testCol;
}

