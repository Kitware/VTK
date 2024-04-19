// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExtractVectorComponents
 * @brief   extract components of vector as separate scalars
 *
 * vtkExtractVectorComponents is a filter that extracts vector components as
 * separate scalars. This is accomplished by creating three different outputs.
 * Each output is the same as the input, except that the scalar values will be
 * one of the three components of the vector. These can be found in the
 * VxComponent, VyComponent, and VzComponent.
 * Alternatively, if the ExtractToFieldData flag is set, the filter will
 * put all the components in the field data. The first component will be
 * the scalar and the others will be non-attribute arrays.
 *
 * @warning
 * This filter is unusual in that it creates multiple outputs.
 * If you use the GetOutput() method, you will be retrieving the x vector
 * component.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 */

#ifndef vtkExtractVectorComponents_h
#define vtkExtractVectorComponents_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersExtractionModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractVectorComponents : public vtkDataSetAlgorithm
{
public:
  static vtkExtractVectorComponents* New();
  vtkTypeMacro(vtkExtractVectorComponents, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Specify the input data or filter.
   */
  virtual void SetInputData(vtkDataSet* input);

  /**
   * Get the output dataset representing velocity x-component. If output is
   * nullptr then input hasn't been set, which is necessary for abstract
   * objects. (Note: this method returns the same information as the
   * GetOutput() method with an index of 0.)
   */
  vtkDataSet* GetVxComponent();

  /**
   * Get the output dataset representing velocity y-component. If output is
   * nullptr then input hasn't been set, which is necessary for abstract
   * objects. (Note: this method returns the same information as the
   * GetOutput() method with an index of 1.)
   * Note that if ExtractToFieldData is true, this output will be empty.
   */
  vtkDataSet* GetVyComponent();

  /**
   * Get the output dataset representing velocity z-component. If output is
   * nullptr then input hasn't been set, which is necessary for abstract
   * objects. (Note: this method returns the same information as the
   * GetOutput() method with an index of 2.)
   * Note that if ExtractToFieldData is true, this output will be empty.
   */
  vtkDataSet* GetVzComponent();

  ///@{
  /**
   * Determines whether the vector components will be put
   * in separate outputs or in the first output's field data
   */
  vtkSetMacro(ExtractToFieldData, vtkTypeBool);
  vtkGetMacro(ExtractToFieldData, vtkTypeBool);
  vtkBooleanMacro(ExtractToFieldData, vtkTypeBool);
  ///@}

protected:
  vtkExtractVectorComponents();
  ~vtkExtractVectorComponents() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  vtkTypeBool ExtractToFieldData;
  int OutputsInitialized;

private:
  vtkExtractVectorComponents(const vtkExtractVectorComponents&) = delete;
  void operator=(const vtkExtractVectorComponents&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
