// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2011 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkPKMeansStatisitcs
 * @brief   A class for parallel k means clustering
 *
 * vtkPKMeansStatistics is vtkKMeansStatistics subclass for parallel datasets.
 * It learns and derives the global statistical model on each node, but assesses each
 * individual data points on the node that owns it.
 *
 * @par Thanks:
 * Thanks to Janine Bennett, Philippe Pebay and David Thompson from Sandia National Laboratories for
 * implementing this class.
 */

#ifndef vtkPKMeansStatistics_h
#define vtkPKMeansStatistics_h

#include "vtkFiltersParallelStatisticsModule.h" // For export macro
#include "vtkKMeansStatistics.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;
class vtkCommunicator;

class VTKFILTERSPARALLELSTATISTICS_EXPORT vtkPKMeansStatistics : public vtkKMeansStatistics
{
public:
  static vtkPKMeansStatistics* New();
  vtkTypeMacro(vtkPKMeansStatistics, vtkKMeansStatistics);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the multiprocess controller. If no controller is set,
   * single process is assumed.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  /**
   * Subroutine to update new cluster centers from the old centers.
   */
  void UpdateClusterCenters(vtkTable* newClusterElements, vtkTable* curClusterElements,
    vtkIdTypeArray* numMembershipChanges, vtkIdTypeArray* numElementsInCluster,
    vtkDoubleArray* error, vtkIdTypeArray* startRunID, vtkIdTypeArray* endRunID,
    vtkIntArray* computeRun) override;

  /**
   * Subroutine to get the total number of data objects.
   */
  vtkIdType GetTotalNumberOfObservations(vtkIdType numObservations) override;

  /**
   * Subroutine to initialize cluster centerss if not provided by the user.
   */
  void CreateInitialClusterCenters(vtkIdType numToAllocate, vtkIdTypeArray* numberOfClusters,
    vtkTable* inData, vtkTable* curClusterElements, vtkTable* newClusterElements) override;

protected:
  vtkPKMeansStatistics();
  ~vtkPKMeansStatistics() override;

  vtkMultiProcessController* Controller;

private:
  vtkPKMeansStatistics(const vtkPKMeansStatistics&) = delete;
  void operator=(const vtkPKMeansStatistics&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
