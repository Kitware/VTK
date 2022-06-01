/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridPProbeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridPProbeFilter
 * @brief   probe a vtkHyperTreeGrid in parallel
 *
 * Heavily modeled after the vtkPProbeFilter and vtkProbeFilter, this class
 *  is meant to be used to probe vtkHyperTreeGrid objects in parallel.
 *
 * This filter works correctly only if the whole geometry dataset
 * (that specify the point locations used to probe input) is present on all
 * nodes.
 *
 * Possible optimizations:
 * - Enrich the parallelism logic allowing for both distributed sources and input/outputs
 */

#ifndef vtkHyperTreeGridPProbeFilter_h
#define vtkHyperTreeGridPProbeFilter_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersParallelModule.h"
#include "vtkSmartPointer.h"

class vtkMultiProcessController;
class vtkIdList;
class vtkDataSet;
class vtkHyperTreeGrid;
class vtkHyperTreeGridLocator;

class VTKFILTERSPARALLEL_EXPORT vtkHyperTreeGridPProbeFilter : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkHyperTreeGridPProbeFilter, vtkDataSetAlgorithm);

  static vtkHyperTreeGridPProbeFilter* New();

  ///@{
  /**
   * Specify the data set that will be probed at the input points.
   * The Input gives the geometry (the points and cells) for the output,
   * while the Source is probed (interpolated) to generate the scalars,
   * vectors, etc. for the output points based on the point locations.
   */
  void SetSourceData(vtkHyperTreeGrid* source);
  vtkHyperTreeGrid* GetSource();
  ///@}

  /**
   * Specify the data set that will be probed at the input points.
   * The Input gives the geometry (the points and cells) for the output,
   * while the Source is probed (interpolated) to generate the scalars,
   * vectors, etc. for the output points based on the point locations.
   */
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);

  ///@{
  /**
   * Set and get the controller.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  ///@{
  /**
   * Set and get the locator object
   */
  virtual vtkHyperTreeGridLocator* GetLocator();
  virtual void SetLocator(vtkHyperTreeGridLocator*);
  ///@}

  ///@{
  /**
   * Shallow copy the input cell data arrays to the output.
   * Off by default.
   */
  vtkSetMacro(PassCellArrays, vtkTypeBool);
  vtkBooleanMacro(PassCellArrays, vtkTypeBool);
  vtkGetMacro(PassCellArrays, vtkTypeBool);
  ///@}
  ///@{
  /**
   * Shallow copy the input point data arrays to the output
   * Off by default.
   */
  vtkSetMacro(PassPointArrays, vtkTypeBool);
  vtkBooleanMacro(PassPointArrays, vtkTypeBool);
  vtkGetMacro(PassPointArrays, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set whether to pass the field-data arrays from the Input i.e. the input
   * providing the geometry to the output. On by default.
   */
  vtkSetMacro(PassFieldArrays, vtkTypeBool);
  vtkBooleanMacro(PassFieldArrays, vtkTypeBool);
  vtkGetMacro(PassFieldArrays, vtkTypeBool);
  ///@}
protected:
  ///@{
  /**
   * Construction methods
   */
  vtkHyperTreeGridPProbeFilter();
  virtual ~vtkHyperTreeGridPProbeFilter();
  ///@}

  ///@{
  /**
   * Methods for processing requests
   */
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  ///@}

  ///@{
  /**
   * Input port should have 2 inputs: input (a dataset) and a source (an HTG).
   * Output should be same as input
   */
  int FillInputPortInformation(int, vtkInformation*) override;
  ///@}

  ///@{
  /**
   * Helper method for initializing the output and local arrays for all processes
   */
  bool Initialize(vtkDataSet* input, vtkHyperTreeGrid* source, vtkDataSet* output);

  /**
   * Helper method for passing data from input to output
   */
  bool PassAttributeData(vtkDataSet* input, vtkDataSet* output);

  /**
   * Helper method for perfoming the probing
   */
  bool DoProbing(
    vtkDataSet* input, vtkHyperTreeGrid* source, vtkDataSet* output, vtkIdList* localPointIds);

  /**
   * Helper method for reducing the distributed data to the master process
   */
  bool Reduce(vtkHyperTreeGrid* source, vtkDataSet* output, vtkIdList* localPointIds);
  ///@}

  enum
  {
    HYPERTREEGRID_PROBE_COMMUNICATION_TAG = 4242
  };

  vtkMultiProcessController* Controller;

  vtkSmartPointer<vtkHyperTreeGridLocator> Locator;

  vtkTypeBool PassCellArrays;
  vtkTypeBool PassPointArrays;
  vtkTypeBool PassFieldArrays;

private:
  vtkHyperTreeGridPProbeFilter(const vtkHyperTreeGridPProbeFilter&) = delete;
  void operator=(const vtkHyperTreeGridPProbeFilter&) = delete;

  class ProbingWorklet;
}; // vtkHyperTreeGridPProbeFilter

#endif // vtkHyperTreeGridPProbeFilter_h
