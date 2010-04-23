/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPCorrelativeStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkToolkits.h"

#include "vtkPCorrelativeStatistics.h"

#include "vtkCommunicator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkTable.h"
#include "vtkVariant.h"

vtkStandardNewMacro(vtkPCorrelativeStatistics);
vtkCxxSetObjectMacro(vtkPCorrelativeStatistics, Controller, vtkMultiProcessController);
//-----------------------------------------------------------------------------
vtkPCorrelativeStatistics::vtkPCorrelativeStatistics()
{
  this->Controller = 0;
  this->SetController( vtkMultiProcessController::GetGlobalController() );
}

//-----------------------------------------------------------------------------
vtkPCorrelativeStatistics::~vtkPCorrelativeStatistics()
{
  this->SetController( 0 );
}

//-----------------------------------------------------------------------------
void vtkPCorrelativeStatistics::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

// ----------------------------------------------------------------------
void vtkPCorrelativeStatistics::Learn( vtkTable* inData,
                                       vtkTable* inParameters,
                                       vtkMultiBlockDataSet* outMeta )
{
  if ( ! outMeta ) 
    { 
    return; 
    } 

  // First calculate correlative statistics on local data set
  this->Superclass::Learn( inData, inParameters, outMeta );

  vtkTable* primaryTab = vtkTable::SafeDownCast( outMeta->GetBlock( 0 ) );
  if ( ! primaryTab ) 
    {
    return;
    }

  vtkIdType nRow = primaryTab->GetNumberOfRows();
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
  if ( ! com )
    {
    vtkErrorMacro("No parallel communicator.");
    }
  
  // (All) gather all sample sizes
  int n_l = primaryTab->GetValueByName( 0, "Cardinality" ).ToInt(); // Cardinality
  int* n_g = new int[np];
  com->AllGather( &n_l, n_g, 1 ); 
  
  // Iterate over all parameter rows
  for ( int r = 0; r < nRow; ++ r )
    {
    // (All) gather all local M statistics
    double M_l[5];
    M_l[0] = primaryTab->GetValueByName( r, "Mean X" ).ToDouble();
    M_l[1] = primaryTab->GetValueByName( r, "Mean Y" ).ToDouble();
    M_l[2] = primaryTab->GetValueByName( r, "M2 X" ).ToDouble();
    M_l[3] = primaryTab->GetValueByName( r, "M2 Y" ).ToDouble();
    M_l[4] = primaryTab->GetValueByName( r, "M XY" ).ToDouble();
    double* M_g = new double[5 * np];
    com->AllGather( M_l, M_g, 5 );

    // Aggregate all local quintuples of M statistics into global ones
    int ns = n_g[0];
    double meanX = M_g[0];
    double meanY = M_g[1];
    double mom2X = M_g[2];
    double mom2Y = M_g[3];
    double momXY = M_g[4];

    for ( int i = 1; i < np; ++ i )
      {
      int ns_l = n_g[i];
      int N = ns + ns_l; 

      int o = 5 * i;
      double meanX_part = M_g[o];
      double meanY_part = M_g[o + 1];
      double mom2X_part = M_g[o + 2];
      double mom2Y_part = M_g[o + 3];
      double momXY_part = M_g[o + 4];
      
      double invN = 1. / static_cast<double>( N );

      double deltaX = meanX_part - meanX;
      double deltaX_sur_N = deltaX * invN;

      double deltaY = meanY_part - meanY;
      double deltaY_sur_N = deltaY * invN;

      int prod_ns = ns * ns_l;
 
      mom2X += mom2X_part 
        + prod_ns * deltaX * deltaX_sur_N;

      mom2Y += mom2Y_part 
        + prod_ns * deltaY * deltaY_sur_N;

      momXY += momXY_part 
        + prod_ns * deltaX * deltaY_sur_N;

      meanX += ns_l * deltaX_sur_N;

      meanY += ns_l * deltaY_sur_N;

      ns = N;
      }

    primaryTab->SetValueByName( r, "Mean X", meanX );
    primaryTab->SetValueByName( r, "Mean Y", meanY );
    primaryTab->SetValueByName( r, "M2 X", mom2X );
    primaryTab->SetValueByName( r, "M2 Y", mom2Y );
    primaryTab->SetValueByName( r, "M XY", momXY );

    // Set global statistics
    primaryTab->SetValueByName( r, "Cardinality", ns );

    // Clean-up
    delete [] M_g;
    }
  delete [] n_g;
}
