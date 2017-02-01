/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPKMeansStatistics.h

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
/**
 * @class   vtkPKMeansStatisitcs
 * @brief   A class for parallel k means clustering
 *
 * vtkPKMeansStatistics is vtkKMeansStatistics subclass for parallel datasets.
 * It learns and derives the global statistical model on each node, but assesses each
 * individual data points on the node that owns it.
 *
 * @par Thanks:
 * Thanks to Janine Bennett, Philippe Pebay and David Thompson from Sandia National Laboratories for implementing this class.
*/

#ifndef vtkPKMeansStatistics_h
#define vtkPKMeansStatistics_h

#include "vtkFiltersParallelStatisticsModule.h" // For export macro
#include "vtkKMeansStatistics.h"

class vtkMultiProcessController;
class vtkCommunicator;

class VTKFILTERSPARALLELSTATISTICS_EXPORT vtkPKMeansStatistics : public vtkKMeansStatistics
{
public:
  static vtkPKMeansStatistics* New();
  vtkTypeMacro(vtkPKMeansStatistics, vtkKMeansStatistics);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the multiprocess controller. If no controller is set,
   * single process is assumed.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  /**
   * Subroutine to update new cluster centers from the old centers.
   */
  virtual void UpdateClusterCenters( vtkTable* newClusterElements,
                                     vtkTable* curClusterElements,
                                     vtkIdTypeArray* numMembershipChanges,
                                     vtkIdTypeArray* numElementsInCluster,
                                     vtkDoubleArray* error,
                                     vtkIdTypeArray* startRunID,
                                     vtkIdTypeArray* endRunID,
                                     vtkIntArray *computeRun ) VTK_OVERRIDE;

  /**
   * Subroutine to get the total number of data objects.
   */
  virtual vtkIdType GetTotalNumberOfObservations( vtkIdType numObservations ) VTK_OVERRIDE;

  /**
   * Subroutine to initialize cluster centerss if not provided by the user.
   */
  virtual void CreateInitialClusterCenters(vtkIdType numToAllocate,
                                           vtkIdTypeArray* numberOfClusters,
                                           vtkTable* inData,
                                           vtkTable* curClusterElements,
                                           vtkTable* newClusterElements) VTK_OVERRIDE;


protected:
  vtkPKMeansStatistics();
  ~vtkPKMeansStatistics();

  vtkMultiProcessController* Controller;


private:
  vtkPKMeansStatistics(const vtkPKMeansStatistics&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPKMeansStatistics&) VTK_DELETE_FUNCTION;
};

#endif
