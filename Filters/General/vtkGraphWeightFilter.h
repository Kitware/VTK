/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphWeightFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGraphWeightFilter
 * @brief   Base class for filters that weight graph
 * edges.
 *
 *
 * vtkGraphWeightFilter is the abstract base class that provides an interface
 * for classes that apply weights to graph edges. The weights are added
 * as a vtkFloatArray named "Weights."
 * The ComputeWeight function must be implemented to provide the function of two
 * vertices which determines the weight of each edge.
 * The CheckRequirements function can be implemented if you wish to ensure
 * that the input graph has all of the properties that will be required
 * by the ComputeWeight function.
*/

#ifndef vtkGraphWeightFilter_h
#define vtkGraphWeightFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkGraphAlgorithm.h"

class vtkGraph;

class VTKFILTERSGENERAL_EXPORT vtkGraphWeightFilter : public vtkGraphAlgorithm
{
public:
  vtkTypeMacro(vtkGraphWeightFilter, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkGraphWeightFilter(){}
  ~vtkGraphWeightFilter() override {}

  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *) override;

  /**
   * Compute the weight on the 'graph' for a particular 'edge'.
   * This is a pure virtual function that must be implemented in subclasses.
   */
  virtual float ComputeWeight(vtkGraph* const graph, const vtkEdgeType& edge) const = 0;

  /**
   * Ensure that the 'graph' is has all properties that are needed to compute
   * the weights. For example, in vtkGraphWeightEuclideanDistanceFilter,
   * 'graph' must have Points set for each vertex, as the ComputeWeight
   * function calls GetPoint.
   */
  virtual bool CheckRequirements(vtkGraph* const graph) const;

private:
  vtkGraphWeightFilter(const vtkGraphWeightFilter&) = delete;
  void operator=(const vtkGraphWeightFilter&) = delete;
};

#endif
