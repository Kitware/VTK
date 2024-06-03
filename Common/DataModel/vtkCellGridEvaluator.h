// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridEvaluator
 * @brief   Evaluate a field (vtkCellAttribute) at some points inside cells.
 *
 * This class is a cell-grid query whose purpose is to determine the
 * value a vtkCellAttribute takes on at one or more points inside
 * the domain of a vtkCellGrid.
 *
 * This class performs its work in two phases:
 * + Classification. Input points are classified by the type and index
 *   of cell in the grid in which they lie.
 * + Attribute/field interpolation. Each cell is asked to interpolate
 *   the value of a cell-attribute at each point classified to it.
 *
 * As an example, consider a cell-grid holding 10 triangles and 20 quads
 * and a query that is provided 5 points. The first phase will identify
 * which of the 5 points are insides triangles, which lie in quadrilaterals,
 * and which lie in neither. Say that 2 lie inside triangles, 2 inside
 * quadrilaterals, and 1 is outside the domain.
 * Furthermore, the first phase will identify which particular triangles
 * or quadrilaterals contain the input points. The two points which
 * lie in triangles will report a number in [0,10[ while the two points
 * which lie in quadrilaterals will report a number in [0, 20[.
 * Finally, for cells which have a reference element, the parametric
 * coordinates of each input point are computed.
 *
 * The second phase additionally interpolates a cell-attribute (let's
 * say "Velocity" in our example) at each input point.
 *
 * You may configure the query to skip either phase (classification or
 * interpolation). If you skip classification, you must provide the
 * the classification information for the input points.
 * The method you call (ClassifyPoints, InterpolatePoints, or
 * InterpolateCellParameters) determines which phase(s) are applied
 * during evaluation.
 *
 * When running in InterpolatePoints mode (both classification and
 * interpolation phases are performed), the output from our example
 * is reported like so:
 *
 * + `GetClassifierCellTypes()` – returns an array with a cell-type hash
 *   for each type of cell containing an input point. The hash value can
 *   be used to construct a vtkStringToken.
 *   Our example would return an array with 3 values which might be
 *   ordered: "vtkDGTri"_hash, "vtkDGQuad"_hash, and 0 (an "invalid" hash).
 * + `GetClassifierCellOffsets()` – returns an array with the same number
 *   of values as the call above. Each value specifies the start of
 *   reporting for points contained in the corresponding cell type.
 *   Our example would return [0, 2, 4] to match the values above.
 * + `GetClassifierPointIDs()` – returns an array whose length matches
 *   the number of input points. Each value is the index of an input
 *   point. Input points do not have their order preserved so that
 *   all the points contained in a single cell can be reported together.
 *   Our example might return [4, 2, 1, 0, 3]. This will always be a
 *   permutation of the counting integers and, for our example, always
 *   hold integers in [0, 5[.
 * + `GetClassifierCellIndices()` – returns an array whose length matches
 *   the number of input points. Each value is the index into cells
 *   of the corresponding type, indicating which cell contains
 *   the input point.
 *   For our example, the first two numbers will be in [0, 10[, the second
 *   two will be in [0, 20[, and the last will be -1. (This is because
 *   we have two points inside 10 triangles, two points inside 20 quads,
 *   and one un-classifiable input point.)
 * + `GetClassifierPointParameters()` – returns an array whose length
 *   matches the number of input points. Each value is a 3-tuple of
 *   reference-cell coordinates (or indeterminate if the cell type does
 *   not provide a reference cell).
 * + `GetInterpolatedValues()` – returns an array whose number of tuples
 *   matches the number of input points and whose number of components
 *   matches the number of components of the requested cell-attribute.
 *   For our example, an array with 5 tuples of 3 components each would be
 *   returned; it would be named "Velocity" (matching the cell-attribute's
 *   name).
 *
 * Note that because you can pass in the arrays above (except the interpolated
 * values) to short-circuit classification, it is possible to evaluate
 * multiple cell-attributes without duplicating the classification work.
 *
 * In InterpolateCellParameters mode, calling the methods above which begin
 * with `GetClassifier…` will simply return the input arrays you passed to
 * configure the query.
 *
 * ## Warnings
 *
 * The output arrays above generally match the number of input points, but
 * will sometimes exceed the number of input points. This will occur when
 * multiple cells contain an input point (either on a shared boundary or
 * because the cells overlap).
 *
 * Note that the output should never have fewer points than the input as
 * even points outside the cells will be classified as such.
 *
 * Currently, this class is limited to evaluating numeric attributes;
 * string or variant arrays are not supported.
 */

#ifndef vtkCellGridEvaluator_h
#define vtkCellGridEvaluator_h

#include "vtkCellAttribute.h" // For Attribute ivar.
#include "vtkCellGridQuery.h"
#include "vtkNew.h"             // For ivars.
#include "vtkStringToken.h"     // For Allocations ivar.
#include "vtkTypeUInt32Array.h" // For ivars.
#include "vtkTypeUInt64Array.h" // For ivars.

#include <set>
#include <unordered_map>

VTK_ABI_NAMESPACE_BEGIN

class vtkStaticPointLocator;
class vtkPointSet;

class VTKCOMMONDATAMODEL_EXPORT vtkCellGridEvaluator : public vtkCellGridQuery
{
public:
  static vtkCellGridEvaluator* New();
  vtkTypeMacro(vtkCellGridEvaluator, vtkCellGridQuery);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Indicate which phases of the query to perform.
  enum Phases
  {
    None,
    Classify,
    ClassifyAndInterpolate,
    Interpolate
  };

  /// Hold per-type input point assignment and an offset for output arrays.
  struct VTKCOMMONDATAMODEL_EXPORT AllocationsByCellType
  {
    std::map<vtkIdType, std::set<vtkIdType>> InputPoints;
    vtkIdType Offset{ 0 };

    vtkIdType GetNumberOfOutputPoints() const;
  };

  /// Set/get the cell-attribute to be evaluated.
  ///
  /// This must be set before the query is evaluated.
  vtkSetObjectMacro(CellAttribute, vtkCellAttribute);
  vtkGetObjectMacro(CellAttribute, vtkCellAttribute);

  /// Get the input points to be classified and possibly interpolated.
  /// These points are in world coordinates (not in reference cell coordinates).
  vtkGetObjectMacro(InputPoints, vtkDataArray);

  /// Get the discovered types of cells containing input points.
  vtkGetObjectMacro(ClassifierCellTypes, vtkTypeUInt32Array);

  /// Get the offset into the result arrays where each cell-type's points reside.
  vtkGetObjectMacro(ClassifierCellOffsets, vtkTypeUInt64Array);

  /// Get the array holding indices into the \a InputPoints array of each output.
  vtkGetObjectMacro(ClassifierPointIDs, vtkTypeUInt64Array);

  /// Get the array holding indices into cells of a given type that contain input points.
  vtkGetObjectMacro(ClassifierCellIndices, vtkTypeUInt64Array);

  /// Get the array holding (r, s, t)-coordinates where each input point is located.
  vtkGetObjectMacro(ClassifierPointParameters, vtkDataArray);

  /// Get the array holding cell-attribute values interpolated to each input point.
  vtkGetObjectMacro(InterpolatedValues, vtkDataArray);

  /// Configure the query to run the classifier but not the interpolator.
  void ClassifyPoints(vtkDataArray* points);

  /// Configure the query to run the classifier followed by the interpolator.
  void InterpolatePoints(vtkDataArray* points);

  /// Configure the query to run only the interpolator.
  ///
  /// Because interpolation usually requires classification information and
  /// parametric coordinates for each classified point, you must provide arrays
  /// holding this information.
  void InterpolateCellParameters(vtkTypeUInt32Array* cellTypes, vtkTypeUInt64Array* cellOffsets,
    vtkTypeUInt64Array* cellIndices, vtkDataArray* pointParameters);

  /// Return what work the query has been configured to do.
  vtkGetEnumMacro(PhasesToPerform, vtkCellGridEvaluator::Phases);

  /// Invoked during evaluation before any cell-grid responders are run.
  bool Initialize() override;
  /// Invoked at the start of each pass.
  void StartPass() override;
  /// Invoked at the end of each pass.
  bool IsAnotherPassRequired() override;
  /// Invoked during evaluation after all cell-grid responders are run.
  bool Finalize() override;

  // Return a point locator that can be used to find input points quickly.
  // vtkStaticPointLocator* GetLocator();
  vtkGetObjectMacro(Locator, vtkStaticPointLocator);

  /// Return a *reference* to a \a cellType's allocated input points
  /// for responders to fill out.
  AllocationsByCellType& GetAllocationsForCellType(vtkStringToken cellType);

protected:
  vtkCellGridEvaluator();
  ~vtkCellGridEvaluator() override;

  /// Set the input points to be classified and possibly interpolated.
  /// These points are in world coordinates (not in reference cell coordinates).
  vtkSetObjectMacro(InputPoints, vtkDataArray);

  /// Set the discovered types of cells containing input points.
  vtkSetObjectMacro(ClassifierCellTypes, vtkTypeUInt32Array);

  /// Set the offset into the result arrays where each cell-type's points reside.
  vtkSetObjectMacro(ClassifierCellOffsets, vtkTypeUInt64Array);

  /// Set the array holding indices into the \a InputPoints array of each output.
  vtkSetObjectMacro(ClassifierPointIDs, vtkTypeUInt64Array);

  /// Set the array holding indices into cells of a given type that contain input points.
  vtkSetObjectMacro(ClassifierCellIndices, vtkTypeUInt64Array);

  /// Set the array holding (r, s, t)-coordinates where each input point is located.
  vtkSetObjectMacro(ClassifierPointParameters, vtkDataArray);

  /// Set the array holding cell-attribute values interpolated to each input point.
  vtkSetObjectMacro(InterpolatedValues, vtkDataArray);

  void AllocateClassificationOutput();
  void AllocatePositionOutput();
  void AllocateInterpolationOutput();

  vtkCellGrid* CellGrid{ nullptr };
  vtkCellAttribute* CellAttribute{ nullptr };
  vtkDataArray* InputPoints{ nullptr };
  vtkTypeUInt32Array* ClassifierCellTypes{ nullptr };
  vtkTypeUInt64Array* ClassifierCellOffsets{ nullptr };
  vtkTypeUInt64Array* ClassifierPointIDs{ nullptr };
  vtkTypeUInt64Array* ClassifierCellIndices{ nullptr };
  vtkDataArray* ClassifierPointParameters{ nullptr };
  vtkDataArray* InterpolatedValues{ nullptr };

  vtkNew<vtkStaticPointLocator> Locator;

  /// Which of the phases are the arrays above configured to perform?
  Phases PhasesToPerform{ Phases::None };
  /// The total number of output points (across all cell types).
  vtkIdType NumberOfOutputPoints{ 0 };
  /// Internal state used during classification to compute the size of the output arrays.
  std::unordered_map<vtkStringToken, AllocationsByCellType> Allocations;

private:
  vtkCellGridEvaluator(const vtkCellGridEvaluator&) = delete;
  void operator=(const vtkCellGridEvaluator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridEvaluator_h
