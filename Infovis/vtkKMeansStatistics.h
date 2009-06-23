/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkKMeansStatistics.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
// .NAME vtkKMeansStatistics - A class for KMeans clustering
//
// .SECTION Description
// This class takes as input an optional vtkTable on port LEARN_PARAMETERS 
// specifying initial  set(s) of cluster values of the following form:  
// <pre>
//           K     | Col1               |  ...    | ColN    
//      -----------+--------------------+---------+---------------
//           M     |clustCoord(1, 1, 1) |  ...    | clustCoord(1, 1, N)
//           M     |clustCoord(1, 2, 1) |  ...    | clustCoord(1, 2, N)
//           .     |       .            |   .     |        .
//           .     |       .            |   .     |        .
//           .     |       .            |   .     |        .
//           M     |clustCoord(1, M, 1) |  ...    | clustCoord(1, M, N)
//           L     |clustCoord(1, 1, 1) |  ...    | clustCoord(0, 1, N)
//           L     |clustCoord(1, 2, 1) |  ...    | clustCoord(0, 2, N)
//           .     |       .            |   .     |        .
//           .     |       .            |   .     |        .
//           .     |       .            |   .     |        .
//           L     |clustCoord(1, L, 1) |  ...    | clustCoord(1, L, N)
// </pre>
//
// When P initial cluster sets are specified, the algorithm is run P times.  
// All user specified clusters must be of the same
// dimension, N. Consequently the table has N+1 columns.  The first column
// identifies the number of clusters associated with each run.
//
// If the user does not supply an initial set of clusters on port LEARN_PARAMETERS, then the
// class uses the first DefaultNumberOfClusters input data elements as initial
// cluster centers and a single run is performed.
//
// This class requires a single set of columns of interest.  If initial cluster
// centers were provided by the user, then the column names of the requests should be a subset 
// of the column names specified in the table on port LEARN_PARAMETERS. Any requests beyond the
// first set will be ignored.
//
// Given this selection of columns of interest, this class provides the
// following functionalities, depending on the execution mode it is executed in:
// * Learn: calculates new cluster centers for each run.  The output metadata on 
//   port OUTPUT_MODEL is a multiblock dataset containing at a minimum
//   one vtkTable with columns specifying the following for each run:
//   the run ID, number of clusters, number of iterations required for convergence, 
//   RMS error associated with the cluster, the number of elements in the cluster, and the new
//   cluster coordinates.
//
// *Derive:  An additional vtkTable is stored in the multiblock dataset output on port OUTPUT_MODEL.
//   This table contains columns that store for each run: the runID, number of clusters, 
//   total error for all clusters in the run, local rank, and global rank.
//   The local rank is computed by comparing RMS errors of all runs with
//   the same number of clusters.  The global rank is computed analagously across all runs.
//
// * Assess: This requires a multiblock dataset (as computed from Learn and Derive) on input port INPUT_MODEL
//   and tabular data on input port INPUT_DATA that contains column names matching those
//   of the tables on input port INPUT_MODEL. The assess mode reports the closest cluster center
//   and associated distance of each observation in port INPUT_DATA's table to the cluster centers for
//   each run in the multiblock dataset provided on port INPUT_MODEL.
//  
// .SECTION Thanks
// Thanks to Janine Bennett, David Thomposn, and Philippe Pebay of
// Sandia National Laboratories for implementing this class.

#ifndef __vtkKMeansStatistics_h
#define __vtkKMeansStatistics_h

#include "vtkStatisticsAlgorithm.h"

class vtkIdTypeArray;
class vtkIntArray;
class vtkDoubleArray;
class vtkKMeansDistanceFunctor;

class VTK_INFOVIS_EXPORT vtkKMeansStatistics : public vtkStatisticsAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkKMeansStatistics, vtkStatisticsAlgorithm);
  virtual void PrintSelf( ostream& os, vtkIndent indent );
  static vtkKMeansStatistics* New();
 
//BTX 
  // Description:
  // Set the DistanceFunctor.
  vtkSetMacro( DistanceFunctor, vtkKMeansDistanceFunctor* );
//ETX 

  // Description:
  // Set the DefaultNumberOfClusters.
  vtkSetMacro( DefaultNumberOfClusters, int );

  // Description:
  // Set the KValuesArrayName.
  vtkSetStringMacro(KValuesArrayName);

  // Description:
  // Set the MaxNumIterations.
  vtkSetMacro( MaxNumIterations, int );

  // Description:
  // Set the Toleratnce.
  vtkSetMacro( Tolerance, double );

//BTX
  // Description:
  // Get the DistanceFunctor.
  vtkGetMacro( DistanceFunctor, vtkKMeansDistanceFunctor* );
//ETX

  // Description:
  // Get the DefaultNumberOfClusters.
  vtkGetMacro( DefaultNumberOfClusters, int );

  // Description:
  // Get the KValuesArrayName.
  vtkGetStringMacro(KValuesArrayName);

  // Description:
  // Get the MaxNumIterations.
  vtkGetMacro( MaxNumIterations, int );

  // Description:
  // Get the Tolerance.
  vtkGetMacro( Tolerance, double );

protected:
  vtkKMeansStatistics();
  ~vtkKMeansStatistics();

  // Description:
  // This algorithm returns a multiblock dataset containing several tables for
  // its meta output (port OUTPUT_MODEL) instead of a single vtkTable.
  // FillOutputPortInformation overridden accordingly.
  virtual int FillOutputPortInformation( int port, vtkInformation* info );
  virtual int FillInputPortInformation( int port, vtkInformation* info );

  // Description:
  // Execute the calculations required by the Derive option.
  virtual void ExecuteDerive( vtkDataObject* );

  // Description:
  // Execute the calculations required by the Derive option.
  virtual void ExecuteAssess( vtkTable*, vtkDataObject*, vtkTable*, vtkDataObject* );

  // Description:
  // Execute the calculations required by the Learn option.
  virtual void ExecuteLearn( vtkTable* inData,
                             vtkTable* inParameters,
                             vtkDataObject* outMeta );
  //BTX  
  // Description:
  // Provide the appropriate assessment functor.
  virtual void SelectAssessFunctor( vtkTable* inData, 
                                    vtkDataObject* inMeta,
                                    vtkStringArray* rowNames,
                                    AssessFunctor*& dfunc );
  //ETX  
  // Description:
  // Subroutine to update new cluster centers from the old centers.
  // Called from within ExecuteLearn (and will be overridden by vtkPKMeansStatistics
  // to handle distributed datasets).
  virtual void UpdateClusterCenters( vtkTable* newClusterElements, 
                                     vtkTable* curClusterElements, 
                                     vtkIdTypeArray* numMembershipChanges,
                                     vtkIdTypeArray* numElementsInCluster, 
                                     vtkIdTypeArray* startRunID, 
                                     vtkIdTypeArray* endRunID, 
                                     vtkIntArray *computeRun );

  // Description:
  // Subroutine to get the total number of observations.
  // Called from within ExecuteLearn (and will be overridden by vtkPKMeansStatistics
  // to handle distributed datasets).
  virtual vtkIdType GetTotalNumberOfObservations( vtkIdType numObservations );

  // Description:
  // Subroutine to initalize the cluster centers using those provided by the user
  // in input port LEARN_PARAMETERS.  If no cluster centers are provided, the subroutine uses the 
  // first DefaultNumberOfClusters input data points as initial cluster centers.
  // Called from within ExecuteLearn.
  int InitializeDataAndClusterCenters(vtkTable* inParameters,
                                      vtkTable* inData,
                                      vtkTable*  dataElements,
                                      vtkIdTypeArray*  numberOfClusters,
                                      vtkTable*  curClusterElements,
                                      vtkTable*  newClusterElements,
                                      vtkIdTypeArray*  startRunID,
                                      vtkIdTypeArray*  endRunID);

  // Description:
  // This is the default number of clusters used when the user does not provide initial cluster centers.
  int DefaultNumberOfClusters;
  // Description:
  // This is the name of the column that specifies the number of clusters in each run. 
  // This is only used if the user has not specified initial clusters.
  char* KValuesArrayName;
  // Description:
  // This is the maximum number of iterations allowed if the new cluster centers have not yet converged. 
  int MaxNumIterations;
  // Description:
  // This is the percentage of data elements that swap cluster IDs 
  double Tolerance;
  // Description:
  // This is the Distance functor.  The default is Euclidean distance, however this can be overridden.
  vtkKMeansDistanceFunctor* DistanceFunctor;
  
private:
  vtkKMeansStatistics( const vtkKMeansStatistics& ); // Not implemented
  void operator=( const vtkKMeansStatistics& );  // Not implemented
};

#endif
