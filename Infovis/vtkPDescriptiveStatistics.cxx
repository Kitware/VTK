/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDescriptiveStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkToolkits.h"

#include "vtkPDescriptiveStatistics.h"

#include "vtkCommunicator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"
#include "vtkTable.h"
#include "vtkVariant.h"

vtkStandardNewMacro(vtkPDescriptiveStatistics);
vtkCxxSetObjectMacro(vtkPDescriptiveStatistics, Controller, vtkMultiProcessController);
//-----------------------------------------------------------------------------
vtkPDescriptiveStatistics::vtkPDescriptiveStatistics()
{
  this->Controller = 0;
  this->SetController( vtkMultiProcessController::GetGlobalController() );
}

//-----------------------------------------------------------------------------
vtkPDescriptiveStatistics::~vtkPDescriptiveStatistics()
{
  this->SetController( 0 );
}

//-----------------------------------------------------------------------------
void vtkPDescriptiveStatistics::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

// ----------------------------------------------------------------------
void vtkPDescriptiveStatistics::Learn( vtkTable* inData,
                                       vtkTable* inParameters,
                                       vtkDataObject* outMetaDO )
{
  vtkTable* outMeta = vtkTable::SafeDownCast( outMetaDO );
  if ( ! outMeta ) 
    { 
    return; 
    } 

  // First calculate descriptive statistics on local data set
  this->Superclass::Learn( inData, inParameters, outMeta );

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
  if ( ! com )
    {
    vtkErrorMacro("No parallel communicator.");
    }

  // (All) gather all sample sizes
  int n_l = outMeta->GetValueByName( 0, "Cardinality" ).ToInt(); // Cardinality
  int* n_g = new int[np];
  com->AllGather( &n_l, n_g, 1 ); 
  
  // Iterate over all parameter rows
  for ( int r = 0; r < nRow; ++ r )
    {
    // Reduce to global extrema
    double extrema_l[2];
    extrema_l[0] = outMeta->GetValueByName( r, "Minimum" ).ToDouble();
    // Collect - max instead of max so a single reduce op. (minimum) can process both extrema at a time
    extrema_l[1] = - outMeta->GetValueByName( r, "Maximum" ).ToDouble();

    double extrema_g[2];
    com->AllReduce( extrema_l, 
                    extrema_g, 
                    2, 
                    vtkCommunicator::MIN_OP );
    outMeta->SetValueByName( r, "Minimum", extrema_g[0] );
    // max = - min ( - max )
    outMeta->SetValueByName( r, "Maximum", - extrema_g[1] );

    // (All) gather all local M statistics
    double M_l[4];
    M_l[0] = outMeta->GetValueByName( r, "Mean" ).ToDouble();
    M_l[1] = outMeta->GetValueByName( r, "M2" ).ToDouble();
    M_l[2] = outMeta->GetValueByName( r, "M3" ).ToDouble();
    M_l[3] = outMeta->GetValueByName( r, "M4" ).ToDouble();
    double* M_g = new double[4 * np];
    com->AllGather( M_l, M_g, 4 );

    // Aggregate all local quadruples of M statistics into global ones
    int ns = n_g[0];
    double mean = M_g[0];
    double mom2 = M_g[1];
    double mom3 = M_g[2];
    double mom4 = M_g[3];

    for ( int i = 1; i < np; ++ i )
      {
      int ns_l = n_g[i];
      int N = ns + ns_l; 

      int o = 4 * i;
      double mean_part = M_g[o];
      double mom2_part = M_g[o + 1];
      double mom3_part = M_g[o + 2];
      double mom4_part = M_g[o + 3];
      
      double delta = mean_part - mean;
      double delta_sur_N = delta / static_cast<double>( N );
      double delta2_sur_N2 = delta_sur_N * delta_sur_N;

      int ns2 = ns * ns;
      int ns_l2 = ns_l * ns_l;
      int prod_ns = ns * ns_l;
 
      mom4 += mom4_part 
        + prod_ns * ( ns2 - prod_ns + ns_l2 ) * delta * delta_sur_N * delta2_sur_N2
        + 6. * ( ns2 * mom2_part + ns_l2 * mom2 ) * delta2_sur_N2
        + 4. * ( ns * mom3_part - ns_l * mom3 ) * delta_sur_N;

      mom3 += mom3_part 
        + prod_ns * ( ns - ns_l ) * delta * delta2_sur_N2
        + 3. * ( ns * mom2_part - ns_l * mom2 ) * delta_sur_N;

      mom2 += mom2_part 
        + prod_ns * delta * delta_sur_N;

      mean += ns_l * delta_sur_N;

      ns = N;
      }

    outMeta->SetValueByName( r, "Mean", mean );
    outMeta->SetValueByName( r, "M2", mom2 );
    outMeta->SetValueByName( r, "M3", mom3 );
    outMeta->SetValueByName( r, "M4", mom4 );

    // Set global statistics
    outMeta->SetValueByName( r, "Cardinality", ns );

    // Clean-up
    delete [] M_g;
    }
  delete [] n_g;
}
