// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLabeledContourPolyDataItem
 * @brief   Filter that translate a vtkPolyData 2D mesh into vtkContextItems.
 *
 * @warning
 * The input vtkPolyData should be a 2D mesh.
 *
 */

#ifndef vtkLabeledContourPolyDataItem_h
#define vtkLabeledContourPolyDataItem_h

#include "vtkPolyDataItem.h"
#include "vtkRect.h"                     // For vtkRect/vtkVector/vtkTuple
#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkSmartPointer.h"             // For vtkSmartPointer
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkContext2D;
class vtkDoubleArray;
class vtkRenderer;
class vtkTextActor3D;
class vtkTextProperty;
class vtkTextPropertyCollection;
struct PDILabelHelper;

class VTKRENDERINGCONTEXT2D_EXPORT VTK_MARSHALAUTO vtkLabeledContourPolyDataItem
  : public vtkPolyDataItem
{
public:
  vtkTypeMacro(vtkLabeledContourPolyDataItem, vtkPolyDataItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkLabeledContourPolyDataItem* New();

  /**
   * Paint event for the item.
   */
  bool Paint(vtkContext2D* painter) override;

  /**
   * The text property used to label the lines. Note that both vertical and
   * horizontal justifications will be reset to "Centered" prior to rendering.
   * @note This is a convenience method that clears TextProperties and inserts
   * the argument as the only property in the collection.
   * @sa SetTextProperties
   */
  virtual void SetTextProperty(vtkTextProperty* tprop);

  ///@{
  /**
   * The text properties used to label the lines. Note that both vertical and
   * horizontal justifications will be reset to "Centered" prior to rendering.

   * If the TextPropertyMapping array exists, then it is used to identify which
   * text property to use for each label as follows: If the scalar value of a
   * line is found in the mapping, the index of the value in mapping is used to
   * lookup the text property in the collection. If there are more mapping
   * values than properties, the properties are looped through until the
   * mapping is exhausted.

   * Lines with scalar values missing from the mapping are assigned text
   * properties in a round-robin fashion starting from the beginning of the
   * collection, repeating from the start of the collection as necessary.
   * @sa SetTextProperty
   * @sa SetTextPropertyMapping
   */
  virtual void SetTextProperties(vtkTextPropertyCollection* coll);
  virtual vtkTextPropertyCollection* GetTextProperties();
  ///@}

  ///@{
  /**
   * Values in this array correspond to vtkTextProperty objects in the
   * TextProperties collection. If a contour line's scalar value exists in
   * this array, the corresponding text property is used for the label.
   * See SetTextProperties for more information.
   */
  virtual vtkDoubleArray* GetTextPropertyMapping();
  virtual void SetTextPropertyMapping(vtkDoubleArray* mapping);
  ///@}

  ///@{
  /**
   * If true, labels will be placed and drawn during rendering. Otherwise,
   * only the mapper returned by GetPolyDataMapper() will be rendered.
   * The default is to draw labels.
   */
  vtkSetMacro(LabelVisibility, bool);
  vtkGetMacro(LabelVisibility, bool);
  vtkBooleanMacro(LabelVisibility, bool);
  ///@}

  ///@{
  /**
   * Ensure that there are at least SkipDistance pixels between labels. This
   * is only enforced on labels along the same line. The default is 0.
   */
  vtkSetMacro(SkipDistance, double);
  vtkGetMacro(SkipDistance, double);
  ///@}

protected:
  vtkLabeledContourPolyDataItem();
  ~vtkLabeledContourPolyDataItem() override;

  virtual void ComputeBounds();

  void Reset();

  bool CheckInputs();
  bool CheckRebuild();
  bool PrepareRender();
  bool PlaceLabels();
  bool ResolveLabels();
  virtual bool CreateLabels();
  bool RenderLabels(vtkContext2D* painter);

  bool AllocateTextActors(vtkIdType num);
  bool FreeTextActors();

  double SkipDistance;

  bool LabelVisibility;
  vtkIdType NumberOfTextActors;
  vtkIdType NumberOfUsedTextActors;
  vtkTextActor3D** TextActors;

  PDILabelHelper** LabelHelpers;

  vtkSmartPointer<vtkTextPropertyCollection> TextProperties;
  vtkSmartPointer<vtkDoubleArray> TextPropertyMapping;

  vtkTimeStamp LabelBuildTime;

private:
  vtkLabeledContourPolyDataItem(const vtkLabeledContourPolyDataItem&) = delete;
  void operator=(const vtkLabeledContourPolyDataItem&) = delete;

  struct Private;
  Private* Internal;
};

VTK_ABI_NAMESPACE_END
#endif
