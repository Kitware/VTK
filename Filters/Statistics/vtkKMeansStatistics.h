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
  Copyright 2010 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
/**
 * @class   vtkKMeansStatistics
 * @brief   A class for KMeans clustering
 *
 *
 * This class takes as input an optional vtkTable on port LEARN_PARAMETERS
 * specifying initial  set(s) of cluster values of the following form:
 * <pre>
 *           K     | Col1            |  ...    | ColN
 *      -----------+-----------------+---------+---------------
 *           M     |clustCoord(1, 1) |  ...    | clustCoord(1, N)
 *           M     |clustCoord(2, 1) |  ...    | clustCoord(2, N)
 *           .     |       .         |   .     |        .
 *           .     |       .         |   .     |        .
 *           .     |       .         |   .     |        .
 *           M     |clustCoord(M, 1) |  ...    | clustCoord(M, N)
 *           L     |clustCoord(1, 1) |  ...    | clustCoord(1, N)
 *           L     |clustCoord(2, 1) |  ...    | clustCoord(2, N)
 *           .     |       .         |   .     |        .
 *           .     |       .         |   .     |        .
 *           .     |       .         |   .     |        .
 *           L     |clustCoord(L, 1) |  ...    | clustCoord(L, N)
 * </pre>
 *
 * Because the desired value of K is often not known in advance and the
 * results of the algorithm are dependent on the initial cluster centers,
 * we provide a mechanism for the user to test multiple runs or sets of cluster centers
 * within a single call to the Learn phase.  The first column of the table identifies
 * the number of clusters K in the particular run (the entries in this column should be
 * of type vtkIdType), while the remaining columns are a
 * subset of the columns contained in the table on port INPUT_DATA.  We require that
 * all user specified clusters be of the same dimension N and consequently, that the
 * LEARN_PARAMETERS table have N+1 columns. Due to this restriction, only one request
 * can be processed for each call to the Learn phase and subsequent requests are
 * silently ignored. Note that, if the first column of the LEARN_PARAMETERS table is not
 * of type vtkIdType, then the table will be ignored and a single run will be performed using
 * the first DefaultNumberOfClusters input data observations as initial cluster centers.
 *
 * When the user does not supply an initial set of clusters, then the first
 * DefaultNumberOfClusters input data observations are used as initial cluster
 * centers and a single run is performed.
 *
 *
 * This class provides the following functionalities, depending on the operation
 * in which it is executed:
 * * Learn: calculates new cluster centers for each run.  The output metadata on
 *   port OUTPUT_MODEL is a multiblock dataset containing at a minimum
 *   one vtkTable with columns specifying the following for each run:
 *   the run ID, number of clusters, number of iterations required for convergence,
 *   total error associated with the cluster (sum of squared Euclidean distance from each observation
 *   to its nearest cluster center), the cardinality of the cluster, and the new
 *   cluster coordinates.
 *
 * * Derive:  An additional vtkTable is stored in the multiblock dataset output on port OUTPUT_MODEL.
 *   This table contains columns that store for each run: the runID, number of clusters,
 *   total error for all clusters in the run, local rank, and global rank.
 *   The local rank is computed by comparing squared Euclidean errors of all runs with
 *   the same number of clusters.  The global rank is computed analagously across all runs.
 *
 * * Assess: This requires a multiblock dataset (as computed from Learn and Derive) on input port INPUT_MODEL
 *   and tabular data on input port INPUT_DATA that contains column names matching those
 *   of the tables on input port INPUT_MODEL. The assess mode reports the closest cluster center
 *   and associated squared Euclidean distance of each observation in port INPUT_DATA's table to the cluster centers for
 *   each run in the multiblock dataset provided on port INPUT_MODEL.
 *
 * The code can handle a wide variety of data types as it operates on vtkAbstractArrays
 * and is not limited to vtkDataArrays.  A default distance functor that
 * computes the sum of the squares of the Euclidean distance between two objects is provided
 * (vtkKMeansDistanceFunctor). The default distance functor can be overridden to use alternative distance metrics.
 *
 * @par Thanks:
 * Thanks to Janine Bennett, David Thompson, and Philippe Pebay of
 * Sandia National Laboratories for implementing this class.
 * Updated by Philippe Pebay, Kitware SAS 2012
*/

#ifndef vtkKMeansStatistics_h
#define vtkKMeansStatistics_h

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkStatisticsAlgorithm.h"

class vtkIdTypeArray;
class vtkIntArray;
class vtkDoubleArray;
class vtkKMeansDistanceFunctor;
class vtkMultiBlockDataSet;

class VTKFILTERSSTATISTICS_EXPORT vtkKMeansStatistics : public vtkStatisticsAlgorithm
{
public:
  vtkTypeMacro(vtkKMeansStatistics, vtkStatisticsAlgorithm);
  void PrintSelf( ostream& os, vtkIndent indent ) VTK_OVERRIDE;
  static vtkKMeansStatistics* New();

  //@{
  /**
   * Set the DistanceFunctor.
   */
  virtual void SetDistanceFunctor( vtkKMeansDistanceFunctor* );
  vtkGetObjectMacro(DistanceFunctor,vtkKMeansDistanceFunctor);
  //@}

  //@{
  /**
   * Set/get the \a DefaultNumberOfClusters, used when no initial cluster coordinates are specified.
   */
  vtkSetMacro(DefaultNumberOfClusters, int);
  vtkGetMacro(DefaultNumberOfClusters, int);
  //@}

  //@{
  /**
   * Set/get the KValuesArrayName.
   */
  vtkSetStringMacro(KValuesArrayName);
  vtkGetStringMacro(KValuesArrayName);
  //@}

  //@{
  /**
   * Set/get the MaxNumIterations used to terminate iterations on
   * cluster center coordinates when the relative tolerance can not be met.
   */
  vtkSetMacro( MaxNumIterations, int );
  vtkGetMacro( MaxNumIterations, int );
  //@}

  //@{
  /**
   * Set/get the relative \a Tolerance used to terminate iterations on
   * cluster center coordinates.
   */
  vtkSetMacro( Tolerance, double );
  vtkGetMacro( Tolerance, double );
  //@}

  /**
   * Given a collection of models, calculate aggregate model
   * NB: not implemented
   */
  void Aggregate( vtkDataObjectCollection*,
                  vtkMultiBlockDataSet* ) VTK_OVERRIDE { return; };

  /**
   * A convenience method for setting properties by name.
   */
  bool SetParameter(
    const char* parameter, int index, vtkVariant value ) VTK_OVERRIDE;

protected:
  vtkKMeansStatistics();
  ~vtkKMeansStatistics() VTK_OVERRIDE;

  /**
   * Execute the calculations required by the Learn option.
   */
  void Learn( vtkTable*,
              vtkTable*,
              vtkMultiBlockDataSet* ) VTK_OVERRIDE;

  /**
   * Execute the calculations required by the Derive option.
   */
  void Derive( vtkMultiBlockDataSet* ) VTK_OVERRIDE;

  /**
   * Execute the calculations required by the Assess option.
   */
  void Assess( vtkTable*,
               vtkMultiBlockDataSet*,
               vtkTable* ) VTK_OVERRIDE;

  /**
   * Execute the calculations required by the Test option.
   */
  void Test( vtkTable*,
                     vtkMultiBlockDataSet*,
                     vtkTable* ) VTK_OVERRIDE { return; };

  /**
   * Provide the appropriate assessment functor.
   */
  void SelectAssessFunctor( vtkTable* inData,
                            vtkDataObject* inMeta,
                            vtkStringArray* rowNames,
                            AssessFunctor*& dfunc ) VTK_OVERRIDE;

  /**
   * Subroutine to update new cluster centers from the old centers.
   * Called from within Learn (and will be overridden by vtkPKMeansStatistics
   * to handle distributed datasets).
   */
  virtual void UpdateClusterCenters( vtkTable* newClusterElements,
                                     vtkTable* curClusterElements,
                                     vtkIdTypeArray* numMembershipChanges,
                                     vtkIdTypeArray* numElementsInCluster,
                                     vtkDoubleArray* error,
                                     vtkIdTypeArray* startRunID,
                                     vtkIdTypeArray* endRunID,
                                     vtkIntArray *computeRun );

  /**
   * Subroutine to get the total number of observations.
   * Called from within Learn (and will be overridden by vtkPKMeansStatistics
   * to handle distributed datasets).
   */
  virtual vtkIdType GetTotalNumberOfObservations( vtkIdType numObservations );

  /**
   * Subroutine to initalize the cluster centers using those provided by the user
   * in input port LEARN_PARAMETERS.  If no cluster centers are provided, the subroutine uses the
   * first DefaultNumberOfClusters input data points as initial cluster centers.
   * Called from within Learn.
   */
  int InitializeDataAndClusterCenters(vtkTable* inParameters,
                                      vtkTable* inData,
                                      vtkTable*  dataElements,
                                      vtkIdTypeArray*  numberOfClusters,
                                      vtkTable*  curClusterElements,
                                      vtkTable*  newClusterElements,
                                      vtkIdTypeArray*  startRunID,
                                      vtkIdTypeArray*  endRunID);

  /**
   * Subroutine to initialize cluster centerss if not provided by the user.
   * Called from within Learn (and will be overridden by vtkPKMeansStatistics
   * to handle distributed datasets).
   */
  virtual void CreateInitialClusterCenters(vtkIdType numToAllocate,
                                           vtkIdTypeArray* numberOfClusters,
                                           vtkTable* inData,
                                           vtkTable* curClusterElements,
                                           vtkTable* newClusterElements);


  /**
   * This is the default number of clusters used when the user does not provide initial cluster centers.
   */
  int DefaultNumberOfClusters;
  /**
   * This is the name of the column that specifies the number of clusters in each run.
   * This is only used if the user has not specified initial clusters.
   */
  char* KValuesArrayName;
  /**
   * This is the maximum number of iterations allowed if the new cluster centers have not yet converged.
   */
  int MaxNumIterations;
  /**
   * This is the percentage of data elements that swap cluster IDs
   */
  double Tolerance;
  /**
   * This is the Distance functor.  The default is Euclidean distance, however this can be overridden.
   */
  vtkKMeansDistanceFunctor* DistanceFunctor;

private:
  vtkKMeansStatistics( const vtkKMeansStatistics& ) VTK_DELETE_FUNCTION;
  void operator=( const vtkKMeansStatistics& ) VTK_DELETE_FUNCTION;
};

#endif
