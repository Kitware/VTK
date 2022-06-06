/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProbeLineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkProbeLineFilter
 * @brief   probe dataset along a line in parallel
 *
 * This filter probes the input data set along an source dataset composed of `vtkLine` and/or
 * `vtkPolyLine` cells. It outputs a `vtkPolyData`, or a `vtkMultiBlockDataSet` if asked. In
 * the case of a multiblock, each block contains the probe result for a single cell of the
 * source. In the case of a polydata output and multiple input lines, only the first line
 * results are returned. Points are sorted along the line or the polyline. Cells are outputted
 * in the same order they were defined in the input (i.e. cells 0 is block 0 of the output).
 *
 * The probing can have different sampling patterns. Three are available:
 * * `SAMPLE_LINE_AT_CELL_BOUNDARIES`: The intersection between the input line and the input
 * data set is computed. At each intersection point, 2 points are generated, slightly shifted
 * inside each interfacing cell. Points are moved back to the intersection after probing.
 * This sampling pattern should typically be used on input data
 * sets holding cell data if one wants to output a step function profile.
 * * `SAMPLE_LINE_AT_SEGMENT_CENTERS`: Center points each consecutive segment formed by the
 * Probing points computed with `SAMPLE_LINE_AT_CELL_BOUNDARIES` are used for probing.
 * This outputs one point per cell, in addition to the end points of a segment.
 * * `SAMPLE_LINE_UNIFORMLY`: A uniformly sampled line is used for probing. In this instance,
 * `LineResolution` is used to determine how many samples are being generated
 *
 * @note In every sampling pattern, the end points are included in the probing, even if they
 * are outside the input data set.
 *
 * @attention In case of a distributed pipeline, the dataset used to determine the lines to
 * probe from (ie the dataset on port 1) will always be the one from rank 0, and will be
 * broadcasted to all other ranks.
 */

#ifndef vtkProbeLineFilter_h
#define vtkProbeLineFilter_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersParallelDIY2Module.h" // For export macro
#include "vtkSmartPointer.h"              // For sampling line

#include <vector> // For sampling line

class vtkDataSet;
class vtkHyperTreeGrid;
class vtkIdList;
class vtkMultiProcessController;
class vtkPoints;
class vtkPolyData;
class vtkVector3d;

class VTKFILTERSPARALLELDIY2_EXPORT vtkProbeLineFilter : public vtkDataObjectAlgorithm
{
public:
  vtkTypeMacro(vtkProbeLineFilter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkProbeLineFilter* New();

  ///@{
  /**
   * Set and get the controller.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  /**
   * Set the source for creating the lines to probe from. Only cells of type VTK_LINE or
   * VTK_POLY_LINE will be processed.
   */
  virtual void SetSourceConnection(vtkAlgorithmOutput* input);

  ///@{
  /**
   * Shallow copy the input cell data arrays to the output.
   * Off by default.
   */
  vtkSetMacro(PassCellArrays, bool);
  vtkBooleanMacro(PassCellArrays, bool);
  vtkGetMacro(PassCellArrays, bool);
  ///@}
  ///@{
  /**
   * Shallow copy the input point data arrays to the output
   * Off by default.
   */
  vtkSetMacro(PassPointArrays, bool);
  vtkBooleanMacro(PassPointArrays, bool);
  vtkGetMacro(PassPointArrays, bool);
  ///@}

  ///@{
  /**
   * Set whether to pass the field-data arrays from the Input i.e. the input
   * providing the geometry to the output. On by default.
   */
  vtkSetMacro(PassFieldArrays, bool);
  vtkBooleanMacro(PassFieldArrays, bool);
  vtkGetMacro(PassFieldArrays, bool);
  ///@}

  ///@{
  /**
   * Set the tolerance used to compute whether a point in the
   * source is in a cell of the input.  This value is only used
   * if ComputeTolerance is off.
   */
  vtkSetMacro(Tolerance, double);
  vtkGetMacro(Tolerance, double);
  ///@}

  ///@{
  /**
   * Set whether to use the Tolerance field or precompute the tolerance.
   * When on, the tolerance will be computed and the field
   * value is ignored. On by default.
   */
  vtkSetMacro(ComputeTolerance, bool);
  vtkBooleanMacro(ComputeTolerance, bool);
  vtkGetMacro(ComputeTolerance, bool);
  ///@}

  ///@{
  /**
   * When dealing with composite datasets, partial arrays are common i.e.
   * data-arrays that are not available in all of the blocks. By default, this
   * filter only passes those point and cell data-arrays that are available in
   * all the blocks i.e. partial array are removed.  When PassPartialArrays is
   * turned on, this behavior is changed to take a union of all arrays present
   * thus partial arrays are passed as well. However, for composite dataset
   * input, this filter still produces a non-composite output. For all those
   * locations in a block of where a particular data array is missing, this
   * filter uses vtkMath::Nan() for double and float arrays, while 0 for all
   * other types of arrays i.e int, char etc.
   */
  vtkSetMacro(PassPartialArrays, bool);
  vtkGetMacro(PassPartialArrays, bool);
  vtkBooleanMacro(PassPartialArrays, bool);
  ///@}

  /**
   * Sampling pattern enumeration. Please refer to class description.
   */
  enum SamplingPatternEnum
  {
    SAMPLE_LINE_AT_CELL_BOUNDARIES = 0,
    SAMPLE_LINE_AT_SEGMENT_CENTERS = 1,
    SAMPLE_LINE_UNIFORMLY = 2
  };

  ///@{
  /**
   * Setter and getter for `SamplingPattern` (values to be taken from the enumeration
   * of the same name).
   */
  vtkGetMacro(SamplingPattern, int);
  vtkSetClampMacro(SamplingPattern, int, 0, 2);
  ///@}

  ///@{
  /**
   * Setter and getter for `LineResolution`. This attribute is only used if sampling
   * using `SamplingPattern::SAMPLE_LINE_UNIFORMLY`. It sets the number of points
   * in the sampling line.
   */
  vtkGetMacro(LineResolution, int);
  vtkSetMacro(LineResolution, int);
  ///@}

  ///@{
  /**
   * If false then each result from an input line to probe results in a block in a
   * vtkMultiBlockDataSet. If true then each block is aggregated as a cell in a
   * single dataset and the output type of the filter becomes a vtkPolyData.
   *
   * Default is true.
   */
  vtkGetMacro(AggregateAsPolyData, bool);
  vtkSetMacro(AggregateAsPolyData, bool);
  vtkBooleanMacro(AggregateAsPolyData, bool);
  ///@}

protected:
  vtkProbeLineFilter();
  ~vtkProbeLineFilter() override;

  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Generate sampling point for a given cell. Supports line and polyline cells.
   * This functions is expected to return a polydata with a single polyline in it.
   */
  vtkSmartPointer<vtkPolyData> CreateSamplingPolyLine(
    vtkPoints* points, vtkIdList* pointIds, vtkDataObject* input, double tol) const;

  ///@{
  /**
   * Generate sampling point between p1 and p2 according to @c SamplingPattern.
   * This functions is expected to return a polydata with a single polyline in it.
   */
  vtkSmartPointer<vtkPolyData> SampleLineAtEachCell(const vtkVector3d& p1, const vtkVector3d& p2,
    vtkDataObject* input, const double tolerance) const;
  vtkSmartPointer<vtkPolyData> SampleLineAtEachCell(const vtkVector3d& p1, const vtkVector3d& p2,
    const std::vector<vtkDataSet*>& input, const double tolerance) const;
  vtkSmartPointer<vtkPolyData> SampleLineAtEachCell(const vtkVector3d& p1, const vtkVector3d& p2,
    vtkHyperTreeGrid* input, const double tolerance) const;
  vtkSmartPointer<vtkPolyData> SampleLineUniformly(
    const vtkVector3d& p1, const vtkVector3d& p2) const;
  ///@}

  vtkMultiProcessController* Controller = nullptr;

  int SamplingPattern = SAMPLE_LINE_AT_CELL_BOUNDARIES;
  int LineResolution = 1000;

  bool AggregateAsPolyData = true;
  bool PassPartialArrays = false;
  bool PassCellArrays = false;
  bool PassPointArrays = false;
  bool PassFieldArrays = false;
  bool ComputeTolerance = true;
  double Tolerance = 1.0;

private:
  vtkProbeLineFilter(const vtkProbeLineFilter&) = delete;
  void operator=(const vtkProbeLineFilter&) = delete;

  struct vtkInternals;
  vtkInternals* Internal;
};

#endif
