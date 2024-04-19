// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGenericClip
 * @brief   clip any dataset with an implicit function or scalar data
 *
 * vtkGenericClip is a filter that any type of dataset using either
 * any subclass of vtkImplicitFunction, or the input scalar
 * data. Clipping means that it actually "cuts" through the cells of
 * the dataset, returning everything inside of the specified implicit
 * function (or greater than the scalar value) including "pieces" of
 * a cell. (Compare this with vtkExtractGeometry, which pulls out
 * entire, uncut cells.) The output of this filter is an unstructured
 * grid.
 *
 * To use this filter, you must decide if you will be clipping with an
 * implicit function, or whether you will be using the input scalar
 * data.  If you want to clip with an implicit function, you must:
 * 1) define an implicit function
 * 2) set it with the SetClipFunction method
 * 3) apply the GenerateClipScalarsOn method
 * If a ClipFunction is not specified, or GenerateClipScalars is off
 * (the default), then the input's scalar data will be used to clip
 * the polydata.
 *
 * You can also specify a scalar value, which is used to decide what is
 * inside and outside of the implicit function. You can also reverse the
 * sense of what inside/outside is by setting the InsideOut instance
 * variable. (The clipping algorithm proceeds by computing an implicit
 * function value or using the input scalar data for each point in the
 * dataset. This is compared to the scalar value to determine
 * inside/outside.)
 *
 * This filter can be configured to compute a second output. The
 * second output is the part of the cell that is clipped away. Set the
 * GenerateClippedData boolean on if you wish to access this output data.
 *
 * This filter has been implemented to operate on generic datasets, rather
 * than the typical vtkDataSet (and subclasses). vtkGenericDataSet is a more
 * complex cousin of vtkDataSet, typically consisting of nonlinear,
 * higher-order cells. To process this type of data, generic cells are
 * automatically tessellated into linear cells prior to isocontouring.
 *
 * @sa
 * vtkClipDataSet vtkClipPolyData vtkClipVolume vtkImplicitFunction
 * vtkGenericDataSet
 */

#ifndef vtkGenericClip_h
#define vtkGenericClip_h

#include "vtkFiltersGenericModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkImplicitFunction;

class vtkPointData;
class vtkCellData;
class vtkIncrementalPointLocator;

class VTKFILTERSGENERIC_EXPORT vtkGenericClip : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkGenericClip, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct with user-specified implicit function; InsideOut turned off;
   * value set to 0.0; and generate clip scalars turned off.
   */
  static vtkGenericClip* New();

  ///@{
  /**
   * Set the clipping value of the implicit function (if clipping with
   * implicit function) or scalar value (if clipping with
   * scalars). The default value is 0.0.
   */
  vtkSetMacro(Value, double);
  vtkGetMacro(Value, double);
  ///@}

  ///@{
  /**
   * Set/Get the InsideOut flag. When off, a vertex is considered
   * inside the implicit function if its value is greater than the
   * Value ivar. When InsideOutside is turned on, a vertex is
   * considered inside the implicit function if its implicit function
   * value is less than or equal to the Value ivar.  InsideOut is off
   * by default.
   */
  vtkSetMacro(InsideOut, vtkTypeBool);
  vtkGetMacro(InsideOut, vtkTypeBool);
  vtkBooleanMacro(InsideOut, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify the implicit function with which to perform the
   * clipping. If you do not define an implicit function,
   * then the selected input scalar data will be used for clipping.
   */
  virtual void SetClipFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(ClipFunction, vtkImplicitFunction);
  ///@}

  ///@{
  /**
   * If this flag is enabled, then the output scalar values will be
   * interpolated from the implicit function values, and not the
   * input scalar data. If you enable this flag but do not provide an
   * implicit function an error will be reported.
   */
  vtkSetMacro(GenerateClipScalars, vtkTypeBool);
  vtkGetMacro(GenerateClipScalars, vtkTypeBool);
  vtkBooleanMacro(GenerateClipScalars, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Control whether a second output is generated. The second output
   * contains the polygonal data that's been clipped away.
   */
  vtkSetMacro(GenerateClippedOutput, vtkTypeBool);
  vtkGetMacro(GenerateClippedOutput, vtkTypeBool);
  vtkBooleanMacro(GenerateClippedOutput, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set the tolerance for merging clip intersection points that are near
   * the vertices of cells. This tolerance is used to prevent the generation
   * of degenerate primitives. Note that only 3D cells actually use this
   * instance variable.
   */
  vtkSetClampMacro(MergeTolerance, double, 0.0001, 0.25);
  vtkGetMacro(MergeTolerance, double);
  ///@}

  ///@{
  /**
   * Return the Clipped output.
   */
  vtkUnstructuredGrid* GetClippedOutput();
  virtual int GetNumberOfOutputs();
  ///@}

  ///@{
  /**
   * Specify a spatial locator for merging points. By default, an
   * instance of vtkMergePoints is used.
   */
  void SetLocator(vtkIncrementalPointLocator* locator);
  vtkGetObjectMacro(Locator, vtkIncrementalPointLocator);
  ///@}

  /**
   * Create default locator. Used to create one when none is specified. The
   * locator is used to merge coincident points.
   */
  void CreateDefaultLocator();

  /**
   * Return the mtime also considering the locator and clip function.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * If you want to clip by an arbitrary array, then set its name here.
   * By default this in nullptr and the filter will use the active scalar array.
   */
  vtkGetStringMacro(InputScalarsSelection);
  void SelectInputScalars(const char* fieldName) { this->SetInputScalarsSelection(fieldName); }
  ///@}

protected:
  vtkGenericClip(vtkImplicitFunction* cf = nullptr);
  ~vtkGenericClip() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

  vtkImplicitFunction* ClipFunction;

  vtkIncrementalPointLocator* Locator;
  vtkTypeBool InsideOut;
  double Value;
  vtkTypeBool GenerateClipScalars;

  vtkTypeBool GenerateClippedOutput;
  double MergeTolerance;

  char* InputScalarsSelection;
  vtkSetStringMacro(InputScalarsSelection);

  // Used internal by vtkGenericAdaptorCell::Clip()
  vtkPointData* InternalPD;
  vtkPointData* SecondaryPD;
  vtkCellData* SecondaryCD;

private:
  vtkGenericClip(const vtkGenericClip&) = delete;
  void operator=(const vtkGenericClip&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
