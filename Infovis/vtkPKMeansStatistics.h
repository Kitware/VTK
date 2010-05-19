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
// .NAME vtkPKMeansStatisitcs - A class for parallel k means clustering
// .SECTION Description
// vtkPKMeansStatistics is vtkKMeansStatistics subclass for parallel datasets.
// It learns and derives the global statistical model on each node, but assesses each 
// individual data points on the node that owns it.

// .SECTION Thanks
// Thanks to Janine Bennett, Philippe Pebay and David Thompson from Sandia National Laboratories for implementing this class.

#ifndef __vtkPKMeanStatistics_h
#define __vtkPKMeanStatistics_h

#include "vtkKMeansStatistics.h"

class vtkMultiProcessController;
class vtkCommunicator;

class VTK_INFOVIS_EXPORT vtkPKMeansStatistics : public vtkKMeansStatistics
{
public:
  static vtkPKMeansStatistics* New();
  vtkTypeMacro(vtkPKMeansStatistics, vtkKMeansStatistics);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the multiprocess controller. If no controller is set,
  // single process is assumed.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // Subroutine to update new cluster centers from the old centers.
  virtual void UpdateClusterCenters( vtkTable* newClusterElements,
                                     vtkTable* curClusterElements,
                                     vtkIdTypeArray* numMembershipChanges,
                                     vtkIdTypeArray* numElementsInCluster,
                                     vtkDoubleArray* error,
                                     vtkIdTypeArray* startRunID,
                                     vtkIdTypeArray* endRunID,
                                     vtkIntArray *computeRun );

  // Description:
  // Subroutine to get the total number of data objects.
  virtual vtkIdType GetTotalNumberOfObservations( vtkIdType numObservations );

  // Description:
  // Subroutine to initialize cluster centerss if not provided by the user.
  virtual void CreateInitialClusterCenters(vtkIdType numToAllocate,
                                           vtkIdTypeArray* numberOfClusters,     
                                           vtkTable* inData,
                                           vtkTable* curClusterElements,
                                           vtkTable* newClusterElements);


protected:
  vtkPKMeansStatistics();
  ~vtkPKMeansStatistics();

  vtkMultiProcessController* Controller;


private:
  vtkPKMeansStatistics(const vtkPKMeansStatistics&); // Not implemented.
  void operator=(const vtkPKMeansStatistics&); // Not implemented.
};

#endif

