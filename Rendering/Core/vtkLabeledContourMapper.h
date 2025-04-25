// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLabeledContourMapper
 * @brief   Draw labeled isolines.
 *
 * Draw isolines with 3D inline labels.
 *
 * The lines in the input polydata will be drawn with labels displaying the
 * scalar value.
 *
 * For this mapper to function properly, stenciling must be enabled in the
 * render window (it is disabled by default). Otherwise the lines will be
 * drawn through the labels.
 */

#ifndef vtkLabeledContourMapper_h
#define vtkLabeledContourMapper_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

#include "vtkMapper.h"
#include "vtkNew.h"          // For vtkNew
#include "vtkSmartPointer.h" // For vtkSmartPointer

VTK_ABI_NAMESPACE_BEGIN
class vtkDoubleArray;
class vtkTextActor3D;
class vtkTextProperty;
class vtkTextPropertyCollection;
class vtkPolyData;
class vtkPolyDataMapper;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkLabeledContourMapper : public vtkMapper
{
public:
  static vtkLabeledContourMapper* New();
  vtkTypeMacro(vtkLabeledContourMapper, vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Render(vtkRenderer* ren, vtkActor* act) override;

  ///@{
  /**
   * Specify the input data to map.
   */
  void SetInputData(vtkPolyData* in);
  vtkPolyData* GetInput();
  ///@}

  ///@{
  /**
   * Return bounding box (array of six doubles) of data expressed as
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   */
  double* GetBounds() override;
  void GetBounds(double bounds[6]) override;
  ///@}

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

  ///@{
  /**
   * The polydata mapper used to render the contours.
   */
  vtkGetNewMacro(PolyDataMapper, vtkPolyDataMapper);
  virtual void SetPolyDataMapper(vtkPolyDataMapper*);
  ///@}

  void ReleaseGraphicsResources(vtkWindow*) override;

protected:
  vtkLabeledContourMapper();
  ~vtkLabeledContourMapper() override;

  virtual void ComputeBounds();

  int FillInputPortInformation(int, vtkInformation*) override;

  void Reset();

  bool CheckInputs(vtkRenderer* ren);
  bool CheckRebuild(vtkRenderer* ren, vtkActor* act);
  bool PrepareRender(vtkRenderer* ren, vtkActor* act);
  bool PlaceLabels();
  bool ResolveLabels();
  virtual bool CreateLabels(vtkActor* actor);
  bool BuildStencilQuads();
  virtual bool ApplyStencil(vtkRenderer* ren, vtkActor* act);
  bool RenderPolyData(vtkRenderer* ren, vtkActor* act);
  virtual bool RemoveStencil(vtkRenderer* ren);
  bool RenderLabels(vtkRenderer* ren, vtkActor* act);

  bool AllocateTextActors(vtkIdType num);
  bool FreeTextActors();

  double SkipDistance = false;

  bool LabelVisibility = true;
  vtkIdType NumberOfTextActors = 0;
  vtkIdType NumberOfUsedTextActors = 0;
  vtkTextActor3D** TextActors = nullptr;

  vtkNew<vtkPolyDataMapper> PolyDataMapper;
  vtkSmartPointer<vtkTextPropertyCollection> TextProperties;
  vtkSmartPointer<vtkDoubleArray> TextPropertyMapping;

  float* StencilQuads = nullptr;
  vtkIdType StencilQuadsSize = 0;
  unsigned int* StencilQuadIndices = nullptr;
  vtkIdType StencilQuadIndicesSize = 0;
  void FreeStencilQuads();

  vtkTimeStamp LabelBuildTime;

private:
  vtkLabeledContourMapper(const vtkLabeledContourMapper&) = delete;
  void operator=(const vtkLabeledContourMapper&) = delete;

  struct Private;
  Private* Internal;
};

VTK_ABI_NAMESPACE_END
#endif
