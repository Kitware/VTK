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
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
#include "vtkToolkits.h"

#include "vtkPMultiCorrelativeStatistics.h"

#include "vtkAbstractArray.h"
#include "vtkCommunicator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPOrderStatistics.h"
#include "vtkTable.h"
#include "vtkVariant.h"

#include <map>

vtkStandardNewMacro(vtkPMultiCorrelativeStatistics);
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
void vtkPMultiCorrelativeStatistics::Learn( vtkTable* inData,
                                            vtkTable* inParameters,
                                            vtkMultiBlockDataSet* outMeta )
{
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

  if ( !this->MedianAbsoluteDeviation )
    {
    vtkPMultiCorrelativeStatistics::GatherStatistics( this->Controller, sparseCov );
    }
}

// ----------------------------------------------------------------------
void vtkPMultiCorrelativeStatistics::GatherStatistics( vtkMultiProcessController *curController,
                                                       vtkTable* sparseCov )
{
  vtkIdType nRow = sparseCov->GetNumberOfRows();
  if ( nRow <= 0 )
    {
    // No statistics were calculated.
    return;
    }

  // Make sure that parallel updates are needed, otherwise leave it at that.
  int np = curController->GetNumberOfProcesses();
  if ( np < 2 )
    {
    return;
    }

  // Now get ready for parallel calculations
  vtkCommunicator* com = curController->GetCommunicator();
  if ( ! com )
    {
    vtkGenericWarningMacro("No parallel communicator.");

    return;
    }

  // (All) gather all sample sizes
  int n_l = sparseCov->GetValueByName( 0, "Entries" ).ToInt(); // Cardinality
  int* n_g = new int[np];
  com->AllGather( &n_l, n_g, 1 );

  // Iterate over all mean and MXY entries
  // NB: two passes are required as there is no guarantee that all means
  //     are stored before MXYs
  int nM = nRow - 1;
  double* M_l = new double[nM];

  // First, load all means and create a name-to-index lookup table
  std::map<vtkStdString, vtkIdType> meanIndex;
  for ( vtkIdType r = 1; r < nRow; ++ r )
    {
    if ( sparseCov->GetValueByName( r, "Column2" ).ToString() == "" )
      {
      meanIndex[sparseCov->GetValueByName( r, "Column1" ).ToString()] = r - 1;

      M_l[r - 1] = sparseCov->GetValueByName( r, "Entries" ).ToDouble();
      }
    }
  vtkIdType nMeans = meanIndex.size();

  // Second, load all MXYs and create an index-to-index-pair lookup table
  std::map<vtkIdType, std::pair<vtkIdType, vtkIdType> > covToMeans;
  for ( vtkIdType r = 1; r < nRow; ++ r )
    {
    vtkStdString col2 = sparseCov->GetValueByName( r, "Column2" ).ToString();
    if ( col2  != "" )
      {
      covToMeans[r - 1] = std::pair<vtkIdType, vtkIdType>
        ( meanIndex[sparseCov->GetValueByName( r, "Column1" ).ToString()],
          meanIndex[col2] );

      M_l[r - 1] = sparseCov->GetValueByName( r, "Entries" ).ToDouble();
      }
    }

  // (All) gather all local means and MXY statistics
  double* M_g = new double[nM * np];
  com->AllGather( M_l, M_g, nM );

  // Aggregate all local nM-tuples of M statistics into global ones
  int ns = n_g[0];
  for ( int i = 0; i < nM; ++ i )
    {
    M_l[i] = M_g[i];
    }

  for ( int i = 1; i < np; ++ i )
    {
    int ns_l = n_g[i];
    int N = ns + ns_l;
    int prod_ns = ns * ns_l;

    double invN = 1. / static_cast<double>( N );

    double* M_part = new double[nM];
    double* delta  = new double[nMeans];
    double* delta_sur_N  = new double[nMeans];
    int o = nM * i;

    // First, calculate deltas for all means
    for ( int j = 0; j < nMeans; ++ j )
      {
      M_part[j] = M_g[o + j];

      delta[j] = M_part[j] - M_l[j];
      delta_sur_N[j] = delta[j] * invN;
      }

    // Then, update covariances
    for ( int j = nMeans; j < nM; ++ j )
      {
      M_part[j] = M_g[o + j];

      M_l[j] += M_part[j]
        + prod_ns * delta[covToMeans[j].first] * delta_sur_N[covToMeans[j].second];
      }

    // Then, update means
    for ( int j = 0; j < nMeans; ++ j )
      {
      M_l[j] += ns_l * delta_sur_N[j];
      }

    // Last, update cardinality
    ns = N;

    // Clean-up
    delete [] M_part;
    delete [] delta;
    delete [] delta_sur_N;
    }

  for ( int i = 0; i < nM; ++ i )
    {
    sparseCov->SetValueByName( i + 1, "Entries", M_l[i] );
    }

  sparseCov->SetValueByName( 0, "Entries", ns );

  // Clean-up
  delete [] M_l;
  delete [] M_g;
  delete [] n_g;
}

// ----------------------------------------------------------------------
vtkOrderStatistics* vtkPMultiCorrelativeStatistics::CreateOrderStatisticsInstance()
{
  return vtkPOrderStatistics::New();
}
