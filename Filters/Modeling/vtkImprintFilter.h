/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImprintFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImprintFilter
 * @brief   Imprint the contact surface of one object onto another surface
 *
 * This filter imprints the contact surface of one vtkPolyData mesh onto
 * a second, input vtkPolyData mesh. There are two inputs to the filter:
 * the target, which is the surface to be imprinted, and the imprint, which
 * is the object imprinting the target.
 *
 * A high level overview of the algorithm is as follows. 1) The target cells
 * are segregated into two subsets: those that may intersect the imprint
 * surface (the candidate cells determined by bounding box checks), and those
 * that do not. 2) The non-candidates are sent to the output, the candidate
 * intersection cells are further proceesed - eventually they will be
 * triangulated as a result of contact with the imprint, with the result of
 * the triangulation appended to the output. 3) The imprint points are projected
 * onto the candidate cells, determining a classification (on a target point,
 * on a target edge, interior to a target cell, outside the target).  4) The
 * non-outside imprint points are associated with one or more target cells.
 * 5) The imprint edges are intersected with the target cell edges, producing
 * additional points associated with the the candidate cells, as well as
 * "fragments" or portions of edges associated with the candidate target
 * cells. 6) On a per-candidate-target-cell basis, the points and edge
 * fragments associated with that cell are used to triangulate the cell.
 * 7) Finally, the triangulated target cells are appended to the output.
 *
 * Several options exist toSpecify whether to produce an output cell data array that indicates
 * whether the output cells are in the imprinted area. If enabled, this
 * output vtkCharArray will have a value=1 for cells that are in the
 * imprinted area. Otherwise, the value=0 is indicating the cell is not
 * in contact with the imprinted area. The name of this cell data array is
 * "ImprintedCells".
 *
 * Some notes:
 * -- The algorithm assumes that the input target and imprint cells are convex.
 * -- If performing a PROJECTED_IMPRINT, the output is the imprint mesh with
 *    the point coordinates modified by projecting the imprint points onto
 *    the target. If the profection of an imprint point onto the target is
 *    unsuccessful, the imprint point coordinates are not modified.
 * -- If performing a MERGED_IMPRINT, the number of output points is
 *    (numTargetPts + numImprintPts + numEdgeIntPts).
 * -- Not all of the output points may be used, for example if an imprint point
 *    is coincident (within the tolerance) of a target point, the target point
 *    replaces the imprint point.
 * -- Candidate cells which may reside within the bounding box of the imprint
 *    but may not actually intersect the imprint will be appended to the output
 *    without triangulation.
 * -- Candidate cells that are intersected will be triangulated: i.e., triangles
 *    will be produced and appended to the output.
 * -- Triangulation requires combining the points and edge fragments associated
 *    with each target candidate cell, as well as the candidate cell's defining
 *    points and edges, to produce the final triangulation.
 * -- Portions of the algorithm are SMP threaded. For example, steps #1 and #2
 *    (candidate segregation); point projection step #3; cell triangulation
 *    step #6. Future implementations may further parallelize the algorithm.
 * -- The algorithm produces an output cell data array that indicates
 *    which output cells are in the imprinted area. This vtkCharArray has a
 *    value=0 for cells that were originally target cells; a value=2 for
 *    output cells that are in the imprinted region; and a value=1 for cells
 *    that are in the transition region (between target and imprinted
 *    cells). The name of this cell data array is "ImprintedCells".
 *
 */

#ifndef vtkImprintFilter_h
#define vtkImprintFilter_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkStaticCellLocator;

class VTKFILTERSMODELING_EXPORT vtkImprintFilter : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods to instantiate, print and provide type information.
   */
  static vtkImprintFilter* New();
  vtkTypeMacro(vtkImprintFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Specify the first vtkPolyData input connection which defines the
   * surface mesh to imprint (i.e., the target).
   */
  void SetTargetConnection(vtkAlgorithmOutput* algOutput);
  vtkAlgorithmOutput* GetTargetConnection();

  ///@{
  /**
   * Specify the first vtkPolyData input which defines the surface mesh to
   * imprint (i.e., the taregt). The imprint surface is provided by the
   * second input.
   */
  void SetTargetData(vtkDataObject* target);
  vtkDataObject* GetTarget();
  ///@}

  /**
   * Specify the a second vtkPolyData input connection which defines the
   * surface mesh with which to imprint the target (the target is provided by
   * the first input).
   */
  void SetImprintConnection(vtkAlgorithmOutput* algOutput);
  vtkAlgorithmOutput* GetImprintConnection();

  ///@{
  /**
   * Specify the a second vtkPolyData input which defines the surface mesh
   * with which to imprint the target (i.e., the first input).
   */
  void SetImprintData(vtkDataObject* imprint);
  vtkDataObject* GetImprint();
  ///@}

  ///@{
  /**
   * Specify a tolerance which controls how close the imprint surface must be
   * to the target to successfully imprint the surface.
   */
  vtkSetClampMacro(Tolerance, double, 0.0, VTK_FLOAT_MAX);
  vtkGetMacro(Tolerance, double);
  ///@}

  enum SpecifiedOutput
  {
    TARGET_CELLS = 0,
    IMPRINTED_CELLS = 1,
    PROJECTED_IMPRINT = 2,
    IMPRINTED_REGION = 3,
    MERGED_IMPRINT = 5
  };

  ///@{
  /**
   * Control what is output by the filter. This can be useful for debugging
   * or to extract portions of the data. The choices are: TARGET_CELLS -
   * output the target cells in contact (relative to the tolerance) of the
   * imprint mesh; IMPRINTED_CELLS - output the target's imprinted cells
   * after intersection and triangulation with the imprint mesh;
   * PROJECTED_IMPRINT - project the imprint mesh onto the target mesh,
   * modififying the imprint mesh point coordinates to lie on the target
   * mesh; IMPRINTED_REGION - extract just the area of contact between the
   * target and imprint; and MERGED_IMPRINT - merge the target and imprint
   * mesh after the imprint operation. By default, MERGED_IMPRINT is
   * produced.
   */
  vtkSetClampMacro(OutputType, int, TARGET_CELLS, MERGED_IMPRINT);
  vtkGetMacro(OutputType, int);
  void SetOutputTypeToTargetCells() { this->SetOutputType(TARGET_CELLS); }
  void SetOutputTypeToImprintedCells() { this->SetOutputType(IMPRINTED_CELLS); }
  void SetOutputTypeToProjectedImprint() { this->SetOutputType(PROJECTED_IMPRINT); }
  void SetOutputTypeToImprintedRegion() { this->SetOutputType(IMPRINTED_REGION); }
  void SetOutputTypeToMergedImprint() { this->SetOutputType(MERGED_IMPRINT); }
  ///@}

  ///@{
  /**
   * Indicate whether to insert just the boundary edges of the imprint mesh
   * (i.e., do not insert the interior edges). (Boundary edges are mesh edges
   * used by exactly one cell.) If inserting boundary edges, the imprint
   * operation is similar to a cookie cutter operation. By default, boundary
   * edge insertion is off.
   */
  vtkSetMacro(BoundaryEdgeInsertion, bool);
  vtkGetMacro(BoundaryEdgeInsertion, bool);
  vtkBooleanMacro(BoundaryEdgeInsertion, bool);
  ///@}

  enum DebugOutput
  {
    NO_DEBUG_OUTPUT = 0,
    TRIANGULATION_INPUT = 1,
    TRIANGULATION_OUTPUT = 2
  };

  ///@{
  /**
   * Indicate whether the output should be triangulated. By default (i.e.,
   * TriangulateOutputOff) the imprint cells, if not triangles nor intersect
   * target cell boundaries, will not be triangulated. (Cells in the
   * transition region are always triangulated because they are typically
   * concave.)
   */
  vtkSetMacro(TriangulateOutput, bool);
  vtkGetMacro(TriangulateOutput, bool);
  vtkBooleanMacro(TriangulateOutput, bool);
  //@}

  //@{
  /**
   * The following methods support debugging. By default, NO_DEBUG_OUTPUT is
   * produced and the second output of this filter is empty. If TRIANGULATION_INPUT
   * is set, then the input points and edges contained by the target DebugCellId are
   * output to the second output to this filter.  If TRIANGULATION_OUTPUT is
   * set, then the output triangulation for the specified target cellId is
   * placed in a second output to this filter.
   */
  vtkSetClampMacro(DebugOutputType, int, NO_DEBUG_OUTPUT, TRIANGULATION_OUTPUT);
  vtkGetMacro(DebugOutputType, int);
  void SetDebugOutputTypeToNoDebugOutput() { this->SetDebugOutputType(NO_DEBUG_OUTPUT); }
  void SetDebugOutputTypeToTriangulationInput() { this->SetDebugOutputType(TRIANGULATION_INPUT); }
  void SetDebugOutputTypeToTriangulationOutput() { this->SetDebugOutputType(TRIANGULATION_OUTPUT); }
  vtkSetMacro(DebugCellId, vtkIdType);
  vtkGetMacro(DebugCellId, vtkIdType);
  ///@{

  ///@{
  /**
   * Get the output data (in the second output, if the DebugOutput !=
   * NO_DEBUG_OUTPUT).
   */
  vtkPolyData* GetDebugOutput();
  ///@}

protected:
  vtkImprintFilter();
  ~vtkImprintFilter() override;

  double Tolerance;
  int OutputType;
  bool BoundaryEdgeInsertion;
  bool TriangulateOutput;

  int DebugOutputType;
  vtkIdType DebugCellId;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkImprintFilter(const vtkImprintFilter&) = delete;
  void operator=(const vtkImprintFilter&) = delete;
};

#endif
