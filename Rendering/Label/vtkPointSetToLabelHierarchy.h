// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkPointSetToLabelHierarchy
 * @brief   build a label hierarchy for a graph or point set.
 *
 *
 *
 * Every point in the input vtkPoints object is taken to be an
 * anchor point for a label. Statistics on the input points
 * are used to subdivide an octree referencing the points
 * until the points each octree node contains have a variance
 * close to the node size and a limited population (< 100).
 */

#ifndef vtkPointSetToLabelHierarchy_h
#define vtkPointSetToLabelHierarchy_h

#include "vtkLabelHierarchyAlgorithm.h"
#include "vtkRenderingLabelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkTextProperty;

class VTKRENDERINGLABEL_EXPORT vtkPointSetToLabelHierarchy : public vtkLabelHierarchyAlgorithm
{
public:
  static vtkPointSetToLabelHierarchy* New();
  vtkTypeMacro(vtkPointSetToLabelHierarchy, vtkLabelHierarchyAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/get the "ideal" number of labels to associate with each node in the output hierarchy.
   */
  vtkSetMacro(TargetLabelCount, int);
  vtkGetMacro(TargetLabelCount, int);
  ///@}

  ///@{
  /**
   * Set/get the maximum tree depth in the output hierarchy.
   */
  vtkSetMacro(MaximumDepth, int);
  vtkGetMacro(MaximumDepth, int);
  ///@}

  ///@{
  /**
   * Set/get the label array name.
   */
  virtual void SetLabelArrayName(const char* name);
  virtual const char* GetLabelArrayName();
  ///@}

  ///@{
  /**
   * Set/get the priority array name.
   */
  virtual void SetSizeArrayName(const char* name);
  virtual const char* GetSizeArrayName();
  ///@}

  ///@{
  /**
   * Set/get the priority array name.
   */
  virtual void SetPriorityArrayName(const char* name);
  virtual const char* GetPriorityArrayName();
  ///@}

  ///@{
  /**
   * Set/get the icon index array name.
   */
  virtual void SetIconIndexArrayName(const char* name);
  virtual const char* GetIconIndexArrayName();
  ///@}

  ///@{
  /**
   * Set/get the text orientation array name.
   */
  virtual void SetOrientationArrayName(const char* name);
  virtual const char* GetOrientationArrayName();
  ///@}

  ///@{
  /**
   * Set/get the maximum text width (in world coordinates) array name.
   */
  virtual void SetBoundedSizeArrayName(const char* name);
  virtual const char* GetBoundedSizeArrayName();
  ///@}

  ///@{
  /**
   * Set/get the text property assigned to the hierarchy.
   */
  virtual void SetTextProperty(vtkTextProperty* tprop);
  vtkGetObjectMacro(TextProperty, vtkTextProperty);
  ///@}

protected:
  vtkPointSetToLabelHierarchy();
  ~vtkPointSetToLabelHierarchy() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int TargetLabelCount;
  int MaximumDepth;
  vtkTextProperty* TextProperty;

private:
  vtkPointSetToLabelHierarchy(const vtkPointSetToLabelHierarchy&) = delete;
  void operator=(const vtkPointSetToLabelHierarchy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPointSetToLabelHierarchy_h
