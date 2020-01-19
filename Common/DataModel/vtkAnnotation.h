// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkAnnotation
 * @brief   Stores a collection of annotation artifacts.
 *
 *
 * vtkAnnotation is a collection of annotation properties along with
 * an associated selection indicating the portion of data the annotation
 * refers to.
 *
 * @par Thanks:
 * Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories
 * contributed code to this class.
 */

#ifndef vtkAnnotation_h
#define vtkAnnotation_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkInformationStringKey;
class vtkInformationDoubleVectorKey;
class vtkInformationIntegerVectorKey;
class vtkInformationDataObjectKey;
class vtkSelection;

class VTKCOMMONDATAMODEL_EXPORT vtkAnnotation : public vtkDataObject
{
public:
  vtkTypeMacro(vtkAnnotation, vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkAnnotation* New();

  /**
   * Returns `VTK_ANNOTATION`.
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_ANNOTATION; }

  ///@{
  /**
   * The selection to which this set of annotations will apply.
   */
  vtkGetObjectMacro(Selection, vtkSelection);
  virtual void SetSelection(vtkSelection* selection);
  ///@}

  ///@{
  /**
   * Retrieve a vtkAnnotation stored inside an information object.
   */
  static vtkAnnotation* GetData(vtkInformation* info);
  static vtkAnnotation* GetData(vtkInformationVector* v, int i = 0);
  ///@}

  /**
   * The label for this annotation.
   */
  static vtkInformationStringKey* LABEL();

  /**
   * The color for this annotation.
   * This is stored as an RGB triple with values between 0 and 1.
   */
  static vtkInformationDoubleVectorKey* COLOR();

  /**
   * The color for this annotation.
   * This is stored as a value between 0 and 1.
   */
  static vtkInformationDoubleKey* OPACITY();

  /**
   * An icon index for this annotation.
   */
  static vtkInformationIntegerKey* ICON_INDEX();

  /**
   * Whether or not this annotation is enabled.
   * A value of 1 means enabled, 0 disabled.
   */
  static vtkInformationIntegerKey* ENABLE();

  /**
   * Whether or not this annotation is visible.
   */
  static vtkInformationIntegerKey* HIDE();

  /**
   * Associate a vtkDataObject with this annotation
   */
  static vtkInformationDataObjectKey* DATA();

  /**
   * Initialize the annotation to an empty state.
   */
  void Initialize() override;

  /**
   * Make this annotation have the same properties and have
   * the same selection of another annotation.
   */
  void ShallowCopy(vtkDataObject* other) override;

  /**
   * Make this annotation have the same properties and have
   * a copy of the selection of another annotation.
   */
  void DeepCopy(vtkDataObject* other) override;

  /**
   * Get the modified time of this object.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkAnnotation();
  ~vtkAnnotation() override;

  vtkSelection* Selection;

private:
  vtkAnnotation(const vtkAnnotation&) = delete;
  void operator=(const vtkAnnotation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
