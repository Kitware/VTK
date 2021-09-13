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
 * This filter probes the input data set along an input segment of end points `Point1` and
 * `Point2`. It outputs a `vtkPolyData` composed of `vtkLine` cells. Points are sorted along
 * the line, from `Point1` to `Point2`.
 *
 * The probing can have different sampling patterns. Three are available:
 * * `SAMPLE_LINE_AT_CELL_BOUNDARIES`: The intersection between the input line and the input
 * data set is computed. At each intersection point, 2 points are generated, slightly shifted
 * inside each interfacing cell. Points are moved back to the intersection after probing.
 * This sampling pattern should typically be used on input data
 * sets holding cell data if one wants to output a step function profile. It should be noted
 * that `Point1` and `Point2` are duplicated to handle the case where they lie inside a cell
 * boundary. This ensures that the output is always a step function on cell data.
 * If one wants to ignore `Point1` and `Point2` in the output, one can skip the first 2 points
 * and the last 2 points of the output.
 * * `SAMPLE_LINE_AT_SEGMENT_CENTERS`: Center points each consecutive segment formed by the
 * Probing points computed with `SAMPLE_LINE_AT_CELL_BOUNDARIES` are used for probing.
 * This outputs one point per cell, in addition to `Point1` and `Point2`.
 * * `SAMPLE_LINE_UNIFORMLY`: A uniformly sampled line is used for probing. In this instance,
 * `LineResolution` is used to determine how many samples are being generated
 *
 * @note In every sampling pattern, the end points are included in the probing, even if they
 * are outside the input data set.
 */

#ifndef vtkProbeLineFilter_h
#define vtkProbeLineFilter_h

#include "vtkFiltersParallelDIY2Module.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
#include "vtkSmartPointer.h" // For sampling line

#include <vector> // For sampling line

class vtkMultiProcessController;
class vtkPolyData;

class VTKFILTERSPARALLELDIY2_EXPORT vtkProbeLineFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkProbeLineFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkProbeLineFilter* New();

  ///@{
  /**
   * Set and get the controller.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  ///@{
  /**
   * Set and get for `Point1` and `Point2`, which are the points describing the input line
   * on which to probe from.
   */
  vtkSetVector3Macro(Point1, double);
  vtkGetVector3Macro(Point1, double);
  vtkSetVector3Macro(Point2, double);
  vtkGetVector3Macro(Point2, double);
  ///@}

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
  enum SamplingPattern
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

protected:
  vtkProbeLineFilter();
  ~vtkProbeLineFilter() override;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkSmartPointer<vtkPolyData> SampleLineAtEachCell(
    const std::vector<vtkDataSet*>& input, const double tolerance) const;
  vtkSmartPointer<vtkPolyData> SampleLineUniformly() const;

  vtkMultiProcessController* Controller;

  int SamplingPattern;
  int LineResolution;

  double Point1[3];
  double Point2[3];

  bool PassPartialArrays;
  bool PassCellArrays;
  bool PassPointArrays;
  bool PassFieldArrays;
  bool ComputeTolerance;
  double Tolerance;

private:
  vtkProbeLineFilter(const vtkProbeLineFilter&) = delete;
  void operator=(const vtkProbeLineFilter&) = delete;

  struct vtkInternals;
  vtkInternals* Internal;
};

#endif
