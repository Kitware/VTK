// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkThreshold
 * @brief   extracts cells where scalar value in cell satisfies threshold criterion
 *
 * vtkThreshold is a filter that extracts cells from any dataset type that
 * satisfy a threshold criterion. A cell satisfies the criterion if the
 * scalar value of (every or any) point satisfies the criterion. The
 * criterion can take three forms: 1) greater than a particular value; 2)
 * less than a particular value; or 3) between two values. The output of this
 * filter is an unstructured grid.
 *
 * Note that scalar values are available from the point and cell attribute
 * data.  By default, point data is used to obtain scalars, but you can
 * control this behavior. See the AttributeMode ivar below.
 *
 * By default only the first scalar value is used in the decision. Use the ComponentMode
 * and SelectedComponent ivars to control this behavior. Note that magnitude can be
 * selected if SelectedComponent is set to the number of components of the array.
 *
 * @warning
 * This class is templated. It may run slower than serial execution if the code
 * is not optimized during compilation. Build in Release or ReleaseWithDebugInfo.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkThresholdPoints vtkThresholdTextureCoords, vtkMultiThreshold, vtkSplitByCellScalarFilter,
 * vtkExplodeDataSet
 */

#ifndef vtkThreshold_h
#define vtkThreshold_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

#define VTK_ATTRIBUTE_MODE_DEFAULT 0
#define VTK_ATTRIBUTE_MODE_USE_POINT_DATA 1
#define VTK_ATTRIBUTE_MODE_USE_CELL_DATA 2

// order / values are important because of the SetClampMacro
#define VTK_COMPONENT_MODE_USE_SELECTED 0
#define VTK_COMPONENT_MODE_USE_ALL 1
#define VTK_COMPONENT_MODE_USE_ANY 2

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkIdList;

class VTKFILTERSCORE_EXPORT vtkThreshold : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkThreshold* New();
  vtkTypeMacro(vtkThreshold, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Possible values for the threshold function:
   * - THRESHOLD_BETWEEN - Keep values between the lower and upper thresholds.
   * - THRESHOLD_LOWER - Keep values below the lower threshold.
   * - THRESHOLD_UPPER - Keep values above the upper threshold.
   */
  enum ThresholdType
  {
    THRESHOLD_BETWEEN = 0,
    THRESHOLD_LOWER,
    THRESHOLD_UPPER
  };

  ///@{
  /**
   * Get/Set the threshold method, defining which threshold bounds to use. The default method is
   * vtkThreshold::Between.
   */
  void SetThresholdFunction(int function);
  int GetThresholdFunction();
  ///@}

  ///@{
  /**
   * Set/get the upper and lower thresholds. The default values are set to +infinity and -infinity,
   * respectively.
   */
  vtkSetMacro(UpperThreshold, double);
  vtkSetMacro(LowerThreshold, double);
  vtkGetMacro(UpperThreshold, double);
  vtkGetMacro(LowerThreshold, double);
  ///@}

  ///@{
  /**
   * Control how the decision of in / out is made with multi-component data.
   * The choices are to use the selected component (specified in the
   * SelectedComponent ivar), or to look at all components. When looking at
   * all components, the evaluation can pass if all the components satisfy
   * the rule (UseAll) or if any satisfy is (UseAny). The default value is
   * UseSelected.
   */
  vtkSetClampMacro(ComponentMode, int, VTK_COMPONENT_MODE_USE_SELECTED, VTK_COMPONENT_MODE_USE_ANY);
  vtkGetMacro(ComponentMode, int);
  void SetComponentModeToUseSelected() { this->SetComponentMode(VTK_COMPONENT_MODE_USE_SELECTED); }
  void SetComponentModeToUseAll() { this->SetComponentMode(VTK_COMPONENT_MODE_USE_ALL); }
  void SetComponentModeToUseAny() { this->SetComponentMode(VTK_COMPONENT_MODE_USE_ANY); }
  const char* GetComponentModeAsString();
  ///@}

  ///@{
  /**
   * When the component mode is UseSelected, this ivar indicated the selected
   * component. If set to the number of components of the array, threshold
   * will apply on array's magnitude.
   * The default value is 0.
   */
  vtkSetClampMacro(SelectedComponent, int, 0, VTK_INT_MAX);
  vtkGetMacro(SelectedComponent, int);
  ///@}

  ///@{
  /**
   * If using scalars from point data, all scalars for all points in a cell
   * must satisfy the threshold criterion if AllScalars is set. Otherwise,
   * just a single scalar value satisfying the threshold criterion enables
   * will extract the cell.
   */
  vtkSetMacro(AllScalars, vtkTypeBool);
  vtkGetMacro(AllScalars, vtkTypeBool);
  vtkBooleanMacro(AllScalars, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If this is on (default is off), we will use the continuous interval
   * [minimum cell scalar, maximum cell scalar] to intersect the threshold bound
   * , rather than the set of discrete scalar values from the vertices
   * *WARNING*: For higher order cells, the scalar range of the cell is
   * not the same as the vertex scalar interval used here, so the
   * result will not be accurate.
   */
  vtkSetMacro(UseContinuousCellRange, vtkTypeBool);
  vtkGetMacro(UseContinuousCellRange, vtkTypeBool);
  vtkBooleanMacro(UseContinuousCellRange, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Invert the threshold results. That is, cells that would have been in the output with this
   * option off are excluded, while cells that would have been excluded from the output are
   * included.
   */
  vtkSetMacro(Invert, bool);
  vtkGetMacro(Invert, bool);
  vtkBooleanMacro(Invert, bool);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

  ///@{
  /**
   * Methods used for thresholding. vtkThreshold::Lower returns true if s is lower than the lower
   * threshold, vtkThreshold::Upper returns true if s is larger than the upper threshold, and
   * vtkThreshold::Between returns true if s is between the lower and upper thresholds.
   *
   * @warning These methods use threshold values that can be set with
   * vtkThreshold::SetLowerThreshold and vtkThreshold::SetUpperThreshold. The threshold
   * method can be set using vtkThreshold::SetThresholdFunction.
   *
   * @note They are not protected members for inheritance purposes. The addresses of those methods
   * are stored in one of this class attributes to figure out which version of the threshold to
   * apply, which are inaccessible if protected.
   */
  int Lower(double s) const;
  int Upper(double s) const;
  int Between(double s) const;
  ///@}
protected:
  vtkThreshold();
  ~vtkThreshold() override;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  double LowerThreshold;
  double UpperThreshold;
  vtkTypeBool AllScalars = 1;
  vtkTypeBool UseContinuousCellRange = 0;
  bool Invert = false;
  int AttributeMode = -1;
  int ComponentMode = VTK_COMPONENT_MODE_USE_SELECTED;
  int SelectedComponent = 0;
  int OutputPointsPrecision = DEFAULT_PRECISION;

  int (vtkThreshold::*ThresholdFunction)(double s) const = &vtkThreshold::Between;

  template <typename TScalarArray>
  struct EvaluateCellsFunctor;
  struct EvaluateCellsWorker;

  template <typename TScalarsArray>
  int EvaluateComponents(TScalarsArray& scalars, vtkIdType id);
  template <typename TScalarsArray>
  int EvaluateCell(TScalarsArray& scalars, const vtkIdType* cellPts, vtkIdType numCellPts);
  template <typename TScalarsArray>
  int EvaluateCell(TScalarsArray& scalars, int c, const vtkIdType* cellPts, vtkIdType numCellPts);

private:
  /**
   * Returns whether magnitude was computed.
   * Is true if selected component equals number of components
   * and if number of components > 1.
   */
  template <typename TScalarsArray>
  bool ComputeMagnitude(double& magnitude, const TScalarsArray& scalars, vtkIdType id);

  vtkThreshold(const vtkThreshold&) = delete;
  void operator=(const vtkThreshold&) = delete;

  int NumberOfComponents;
};

VTK_ABI_NAMESPACE_END
#endif
