/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkContingencyStatisticsGnuR.cxx

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

#include "vtkContingencyStatisticsGnuR.h"
#include "vtkRInterface.h"
#include "vtkToolkits.h"
#include "vtkStatisticsAlgorithmPrivate.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <map>
#include <vector>

#include <sstream>


vtkStandardNewMacro(vtkContingencyStatisticsGnuR);

vtkContingencyStatisticsGnuR::vtkContingencyStatisticsGnuR()
{
  VTK_LEGACY_BODY(vtkContingencyStatisticsGnuR::vtkContingencyStatisticsGnuR, "VTK 8.0");

}

// ----------------------------------------------------------------------
vtkContingencyStatisticsGnuR::~vtkContingencyStatisticsGnuR()
{

}

// ----------------------------------------------------------------------
void vtkContingencyStatisticsGnuR::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

// ----------------------------------------------------------------------
void vtkContingencyStatisticsGnuR::CalculatePValues( vtkTable* outTab)
{
  vtkIdTypeArray* dimCol = vtkArrayDownCast<vtkIdTypeArray>( outTab->GetColumn(0) );
  vtkDoubleArray* chi2Col = vtkArrayDownCast<vtkDoubleArray>( outTab->GetColumn(1));
  vtkDoubleArray* chi2yCol = vtkArrayDownCast<vtkDoubleArray>( outTab->GetColumn(2));

  // Prepare VTK - R interface
  vtkRInterface* ri = vtkRInterface::New();

  // Use the calculated DOFs and Chi square statistics as inputs to the Chi square function
  ri->AssignVTKDataArrayToRVariable( dimCol, "d" );
  ri->AssignVTKDataArrayToRVariable( chi2Col, "chi2" );
  ri->AssignVTKDataArrayToRVariable( chi2yCol, "chi2y" );

  // Now prepare R script and calculate the p-values (in a single R script evaluation for efficiency)
  std::ostringstream rs;
  rs << "p<-c();"
     << "py<-c();"
     << "for(i in 1:"
     << dimCol->GetNumberOfTuples()
     << "){"
     << "p<-c(p,1-pchisq(chi2[i],d[i]));"
     << "py<-c(py,1-pchisq(chi2y[i],d[i]))"
     << "}";
  ri->EvalRscript( rs.str().c_str() );

  // Retrieve the p-values
  vtkDoubleArray* testChi2Col = vtkArrayDownCast<vtkDoubleArray>( ri->AssignRVariableToVTKDataArray( "p" ) );
  vtkDoubleArray* testChi2yCol = vtkArrayDownCast<vtkDoubleArray>( ri->AssignRVariableToVTKDataArray( "py" ) );
  if ( ! testChi2Col || ! testChi2yCol
       || testChi2Col->GetNumberOfTuples() != dimCol->GetNumberOfTuples()
       || testChi2yCol->GetNumberOfTuples() != dimCol->GetNumberOfTuples() )
  {
    vtkWarningMacro( "Something went wrong with the R calculations. Reported p-values will be invalid." );
    this->Superclass::CalculatePValues( outTab );
  }
  else
  {
    outTab->AddColumn( testChi2Col );
    outTab->AddColumn( testChi2yCol );
  }

  testChi2Col->SetName( "P" );
  testChi2yCol->SetName( "P Yates" );

  // Clean up
  ri->Delete();
}

