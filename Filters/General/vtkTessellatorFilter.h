// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2003 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-NVIDIA-USGov
#ifndef vtkTessellatorFilter_h
#define vtkTessellatorFilter_h

/**
 * @class   vtkTessellatorFilter
 * @brief   approximate nonlinear FEM elements with simplices
 *
 * This class approximates nonlinear FEM elements with linear simplices.
 *
 * <b>Warning</b>: This class is temporary and will go away at some point
 * after ParaView 1.4.0.
 *
 * This filter rifles through all the cells in an input vtkDataSet. It
 * tessellates each cell and uses the vtkStreamingTessellator and
 * vtkDataSetEdgeSubdivisionCriterion classes to generate simplices that
 * approximate the nonlinear mesh using some approximation metric (encoded
 * in the particular vtkDataSetEdgeSubdivisionCriterion::EvaluateLocationAndFields
 * implementation). The simplices are placed into the filter's output
 * vtkDataSet object by the callback routines AddATetrahedron,
 * AddATriangle, and AddALine, which are registered with the triangulator.
 *
 * The output mesh will have geometry and any fields specified as
 * attributes in the input mesh's point data.  The attribute's copy flags
 * are honored, except for normals.
 *
 *
 * @par Internals:
 * The filter's main member function is RequestData(). This function first
 * calls SetupOutput() which allocates arrays and some temporary variables
 * for the primitive callbacks (OutputTriangle and OutputLine which are
 * called by AddATriangle and AddALine, respectively).  Each cell is given
 * an initial tessellation, which results in one or more calls to
 * OutputTetrahedron, OutputTriangle or OutputLine to add elements to the
 * OutputMesh. Finally, Teardown() is called to free the filter's working
 * space.
 *
 * @sa
 * vtkDataSetToUnstructuredGridFilter vtkDataSet vtkStreamingTessellator
 * vtkDataSetEdgeSubdivisionCriterion
 */

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkDataSet;
class vtkDataSetEdgeSubdivisionCriterion;
class vtkPointLocator;
class vtkPoints;
class vtkStreamingTessellator;
class vtkEdgeSubdivisionCriterion;
class vtkUnstructuredGrid;

class VTKFILTERSGENERAL_EXPORT vtkTessellatorFilter : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkTessellatorFilter, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkTessellatorFilter* New();

  virtual void SetTessellator(vtkStreamingTessellator*);
  vtkGetObjectMacro(Tessellator, vtkStreamingTessellator);

  virtual void SetSubdivider(vtkDataSetEdgeSubdivisionCriterion*);
  vtkGetObjectMacro(Subdivider, vtkDataSetEdgeSubdivisionCriterion);

  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set the dimension of the output tessellation.
   * Cells in dimensions higher than the given value will have
   * their boundaries of dimension \a OutputDimension tessellated.
   * For example, if \a OutputDimension is 2, a hexahedron's
   * quadrilateral faces would be tessellated rather than its
   * interior.
   */
  vtkSetClampMacro(OutputDimension, int, 1, 3);
  vtkGetMacro(OutputDimension, int);
  ///@}

// With VTK_USE_FUTURE_CONST, vtkGetMacro already makes the member const.
#if !VTK_USE_FUTURE_CONST
  int GetOutputDimension() const;
#endif

  ///@{
  /**
   * These are convenience routines for setting properties maintained by the
   * tessellator and subdivider. They are implemented here for ParaView's
   * sake.
   */
  virtual void SetMaximumNumberOfSubdivisions(int num_subdiv_in);
  int GetMaximumNumberOfSubdivisions();
  virtual void SetChordError(double ce);
  double GetChordError();
  ///@}

  ///@{
  /**
   * These methods are for the ParaView client.
   */
  virtual void ResetFieldCriteria();
  virtual void SetFieldCriterion(int field, double err);
  ///@}

  ///@{
  /**
   * The adaptive tessellation will output vertices that are not shared
   * among cells, even where they should be. This can be corrected to
   * some extents with a vtkMergeFilter.
   * By default, the filter is off and vertices will not be shared.
   */
  vtkGetMacro(MergePoints, vtkTypeBool);
  vtkSetMacro(MergePoints, vtkTypeBool);
  vtkBooleanMacro(MergePoints, vtkTypeBool);
  ///@}

protected:
  vtkTessellatorFilter();
  ~vtkTessellatorFilter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Called by RequestData to set up a multitude of member variables used by
   * the per-primitive output functions (OutputLine, OutputTriangle, and
   * maybe one day... OutputTetrahedron).
   */
  void SetupOutput(vtkDataSet* input, vtkUnstructuredGrid* output);

  /**
   * Called by RequestData to merge output points.
   */
  void MergeOutputPoints(vtkUnstructuredGrid* input, vtkUnstructuredGrid* output);

  /**
   * Reset the temporary variables used during the filter's RequestData() method.
   */
  void Teardown();

  /**
   * Run the filter; produce a polygonal approximation to the grid.
   */
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  vtkStreamingTessellator* Tessellator;
  vtkDataSetEdgeSubdivisionCriterion* Subdivider;
  int OutputDimension;
  vtkTypeBool MergePoints;
  vtkPointLocator* Locator;

  ///@{
  /**
   * These member variables are set by SetupOutput for use inside the
   * callback members OutputLine and OutputTriangle.
   */
  vtkUnstructuredGrid* OutputMesh;
  vtkPoints* OutputPoints;
  vtkDataArray** OutputAttributes;
  int* OutputAttributeIndices;
  ///@}

  static void AddAPoint(const double*, vtkEdgeSubdivisionCriterion*, void*, const void*);
  static void AddALine(
    const double*, const double*, vtkEdgeSubdivisionCriterion*, void*, const void*);
  static void AddATriangle(
    const double*, const double*, const double*, vtkEdgeSubdivisionCriterion*, void*, const void*);
  static void AddATetrahedron(const double*, const double*, const double*, const double*,
    vtkEdgeSubdivisionCriterion*, void*, const void*);
  void OutputPoint(const double*);
  void OutputLine(const double*, const double*);
  void OutputTriangle(const double*, const double*, const double*);
  void OutputTetrahedron(const double*, const double*, const double*, const double*);

private:
  vtkTessellatorFilter(const vtkTessellatorFilter&) = delete;
  void operator=(const vtkTessellatorFilter&) = delete;
};

// With VTK_USE_FUTURE_CONST, vtkGetMacro already makes the member const.
#if !VTK_USE_FUTURE_CONST
inline int vtkTessellatorFilter::GetOutputDimension() const
{
  return this->OutputDimension;
}
#endif

VTK_ABI_NAMESPACE_END
#endif // vtkTessellatorFilter_h
