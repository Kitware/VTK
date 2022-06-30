/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridProbeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridProbeFilter
 * @brief   probe a vtkHyperTreeGrid
 *
 * Heavily modeled after the vtkProbeFilter, this class
 *  is meant to be used to probe vtkHyperTreeGrid objects.
 *
 */

#ifndef vtkHyperTreeGridProbeFilter_h
#define vtkHyperTreeGridProbeFilter_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersCoreModule.h" //For export Macro
#include "vtkSmartPointer.h"      //For members

class vtkCharArray;
class vtkIdList;
class vtkIdTypeArray;
class vtkDataSet;
class vtkHyperTreeGrid;
class vtkHyperTreeGridLocator;

class VTKFILTERSCORE_EXPORT vtkHyperTreeGridProbeFilter : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkHyperTreeGridProbeFilter, vtkDataSetAlgorithm);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkHyperTreeGridProbeFilter* New();

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

  ///@{
  /**
   * Returns the name of the char array added to the output with values 1 for
   * valid points and 0 for invalid points.
   * Set to "vtkValidPointMask" by default.
   */
  vtkSetStringMacro(ValidPointMaskArrayName);
  vtkGetStringMacro(ValidPointMaskArrayName);
  ///@}

  ///@{
  /**
   * Get the list of point ids in the output that contain attribute data
   * from the source.
   */
  vtkIdTypeArray* GetValidPoints();
  ///@}
protected:
  ///@{
  /**
   * Construction methods
   */
  vtkHyperTreeGridProbeFilter();
  virtual ~vtkHyperTreeGridProbeFilter() { this->SetValidPointMaskArrayName(nullptr); };
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
   * Helper method for reducing the data after probing
   */
  bool Reduce(vtkHyperTreeGrid* source, vtkDataSet* output, vtkIdList* localPointIds);

  /**
   * Helper method for filling arrays with default values
   */
  void FillDefaultArray(vtkDataArray* da) const;
  ///@}

  vtkSmartPointer<vtkHyperTreeGridLocator> Locator;

  vtkTypeBool PassCellArrays = false;
  vtkTypeBool PassPointArrays = false;
  vtkTypeBool PassFieldArrays = true;

  char* ValidPointMaskArrayName;
  vtkSmartPointer<vtkIdTypeArray> ValidPoints;
  vtkSmartPointer<vtkCharArray> MaskPoints;

private:
  vtkHyperTreeGridProbeFilter(const vtkHyperTreeGridProbeFilter&) = delete;
  void operator=(const vtkHyperTreeGridProbeFilter&) = delete;

  class ProbingWorklet;
}; // vtkHyperTreeGridProbeFilter

#endif // vtkHyperTreeGridProbeFilter_h
