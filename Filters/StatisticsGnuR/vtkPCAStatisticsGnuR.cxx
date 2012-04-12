/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPCAStatisticsGnuR.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPCAStatisticsGnuR.h"

#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkRInterface.h"
#include "vtkObjectFactory.h"

#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkPCAStatisticsGnuR);

// ----------------------------------------------------------------------
vtkPCAStatisticsGnuR::vtkPCAStatisticsGnuR()
{
}

// ----------------------------------------------------------------------
vtkPCAStatisticsGnuR::~vtkPCAStatisticsGnuR()
{
}

// ----------------------------------------------------------------------
void vtkPCAStatisticsGnuR::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}
// Use R to obtain the p-values for the Chi square distribution with 2 DOFs
vtkDoubleArray* vtkPCAStatisticsGnuR::CalculatePValues(vtkIdTypeArray* dimCol,
                                                       vtkDoubleArray* statCol)
{
  // Prepare VTK - R interface
  vtkRInterface* ri = vtkRInterface::New();

  // Use the calculated Jarque-Bera-Srivastava statistics as input to the Chi square function
  ri->AssignVTKDataArrayToRVariable( statCol, "jbs" );
  ri->AssignVTKDataArrayToRVariable( dimCol, "d" );

  // Calculate the p-values (p+1 degrees of freedom)
  // Now prepare R script and calculate the p-values (in a single R script evaluation for efficiency)
  vtksys_ios::ostringstream rs;
  rs << "p<-c();"
     << "for(i in 1:"
     << dimCol->GetNumberOfTuples()
     << "){"
     << "p<-c(p,1-pchisq(jbs[i],d[i]));"
     << "}";
  ri->EvalRscript( rs.str().c_str() );

  // Retrieve the p-values
  vtkDoubleArray* testCol = vtkDoubleArray::SafeDownCast( ri->AssignRVariableToVTKDataArray( "p" ) );
  if ( ! testCol || testCol->GetNumberOfTuples() != statCol->GetNumberOfTuples() )
    {
    vtkWarningMacro( "Something went wrong with the R calculations. Reported p-values will be invalid." );
    testCol = this->Superclass::CalculatePValues( dimCol, statCol );
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

