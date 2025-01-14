// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkFastLabeledDataMapper
 * @brief   draw text labels at dataset points
 *
 * vtkFastLabeledDataMapper is a mapper that renders text at dataset
 * points quickly. The API is similar to but the implementation is different
 * from vtkLabeledDataMapper which this class is meant to replace. This new
 * class is faster than its predecessor because it renders all of the labels
 * at once via shaders instead of deferring to helper instances for each
 * individual label.
 *
 * @sa
 * vtkLabeledDataMapper
 */

#ifndef vtkFastLabeledDataMapper_h
#define vtkFastLabeledDataMapper_h

#include "vtkLabeledDatatypeDefinitions.h" // For Data type Definitions
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

#include <memory> // For unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObject;
class vtkDataSet;
class vtkFloatArray;
class vtkIntArray;
class vtkTextProperty;
class vtkTransform;
VTK_ABI_NAMESPACE_END

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkFastLabeledDataMapper
  : public vtkOpenGLPolyDataMapper
{
public:
  static vtkFastLabeledDataMapper* New();
  vtkTypeMacro(vtkFastLabeledDataMapper, vtkOpenGLPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the format with which to print the labels.  This should
   * be a printf-style format string.

   * By default, the mapper will try to print each component of the
   * tuple using a sane format: %d for integers, %f for floats, %g for
   * doubles, %ld for longs, et cetera.  If you need a different
   * format, set it here.  You can do things like limit the number of
   * significant digits, add prefixes/suffixes, basically anything
   * that printf can do.  If you only want to print one component of a
   * vector, see the ivar LabeledComponent.
   */
  vtkSetStringMacro(LabelFormat);
  vtkGetStringMacro(LabelFormat);
  ///@}

  ///@{
  /**
   * Set/Get the component number to label if the data to print has
   * more than one component. For example, all the components of
   * scalars, vectors, normals, etc. are labeled by default
   * (LabeledComponent=(-1)). However, if this ivar is nonnegative,
   * then only the one component specified is labeled.
   */
  vtkSetMacro(LabeledComponent, int);
  vtkGetMacro(LabeledComponent, int);
  ///@}

  ///@{
  /**
   * Set/Get the separator between components.
   */
  vtkSetMacro(ComponentSeparator, char);
  vtkGetMacro(ComponentSeparator, char);
  ///@}

  ///@{
  /**
   * Set/Get the field data array to label. This instance variable is
   * only applicable if field data is labeled.  This will clear
   * FieldDataName when set.
   */
  vtkSetClampMacro(FieldDataArray, int, 0, VTK_INT_MAX);
  vtkGetMacro(FieldDataArray, int);
  ///@}

  ///@{
  /**
   * Set/Get the name of the field data array to label.  This instance
   * variable is only applicable if field data is labeled.  This will
   * override FieldDataArray when set.
   */
  vtkSetStringMacro(FieldDataName)
  vtkGetStringMacro(FieldDataName);
  ///@}

  ///@{
  /**
   * Specify which data to plot: IDs, scalars, vectors, normals, texture coords,
   * tensors, or field data. If the data has more than one component, use
   * the method SetLabeledComponent to control which components to plot.
   * The default is VTK_LABEL_IDS.
   */
  vtkSetMacro(LabelMode, int);
  vtkGetMacro(LabelMode, int);
  void SetLabelModeToLabelIds() { this->SetLabelMode(VTK_LABEL_IDS); };
  void SetLabelModeToLabelScalars() { this->SetLabelMode(VTK_LABEL_SCALARS); };
  void SetLabelModeToLabelVectors() { this->SetLabelMode(VTK_LABEL_VECTORS); };
  void SetLabelModeToLabelNormals() { this->SetLabelMode(VTK_LABEL_NORMALS); };
  void SetLabelModeToLabelTCoords() { this->SetLabelMode(VTK_LABEL_TCOORDS); };
  void SetLabelModeToLabelTensors() { this->SetLabelMode(VTK_LABEL_TENSORS); };
  void SetLabelModeToLabelFieldData() { this->SetLabelMode(VTK_LABEL_FIELD_DATA); };
  ///@}

  ///@{
  /**
   * Set/Get the text property.
   * If an integer argument is provided, you may provide different text
   * properties for different label types. The type is determined by an
   * optional type input array.
   */
  virtual void SetLabelTextProperty(vtkTextProperty* p) { this->SetLabelTextProperty(p, 0); }
  virtual vtkTextProperty* GetLabelTextProperty() { return this->GetLabelTextProperty(0); }
  virtual void SetLabelTextProperty(vtkTextProperty* p, int type);
  virtual vtkTextProperty* GetLabelTextProperty(int type);
  ///@}

  /**
   * Override TextProperty frame colors with a named, point aligned
   * color array.
   */
  vtkSetStringMacro(FrameColorsName);
  vtkGetStringMacro(FrameColorsName);

  ///@{
  /**
   * Anchor option for labels. Default is LowerLeft.
   */
  enum TextAnchor
  {
    LowerLeft = 0, ///< Uses the lower left corner.
    LowerRight,    ///< Uses the lower right corner.
    UpperLeft,     ///< Uses the upper left corner.
    UpperRight,    ///< Uses the upper right corner.
    LowerEdge,     ///< Uses the lower edge center.
    RightEdge,     ///< Uses the right edge center.
    LeftEdge,      ///< Uses the left edge center
    UpperEdge,     ///< Uses the upper edge center.
    Center         ///< Uses the exact center.
  };
  ///@}

  ///@{
  /**
   * Set/Get the text to be displayed for each corner
   * \sa TextPosition
   */
  vtkSetMacro(TextAnchor, int);
  vtkGetMacro(TextAnchor, int);
  ///@}

  ///@{
  /**
   * Overridden to rebuild labels if necessary.
   */
  void RenderPiece(vtkRenderer* ren, vtkActor* act) override;
  ///@}

  ///@{
  /**
   * Overridden to setup textureobject.
   */
  void RenderPieceStart(vtkRenderer* ren, vtkActor* act) override;
  ///@}

  ///@{
  /**
   * Overridden to teardown textureobject.
   */
  void RenderPieceFinish(vtkRenderer* ren, vtkActor* act) override;
  ///@}

  ///@{
  /**
   * Set the input dataset to the mapper. This mapper handles any vtkDataSet.
   */
  virtual void SetInputData(vtkDataSet*);
  ///@}

  ///@{
  /**
   * Overridden to take into account LabelTextProperty's mtime
   */
  vtkMTimeType GetMTime() override;
  ///@}

  ///@{
  /**
   * Overridden to release internal textureobject.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;
  ///@}

protected:
  vtkFastLabeledDataMapper();
  ~vtkFastLabeledDataMapper() override;

  ///@{
  /**
   * Overridden to set up uniforms for the shaders
   */
  void SetMapperShaderParameters(vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* act) override;
  ///@}

  ///@{
  /**
   * Overridden to declare support for any vtkDataSet, not just vtkPolyData
   */
  int FillInputPortInformation(int, vtkInformation*) override;
  ///@}

  void AllocateLabels(int numLabels);
  void BuildLabels();
  void BuildLabelsInternal(vtkDataSet*);
  void MakeShaderArrays(
    int numCurLabels, const std::vector<std::string>&, vtkIntArray*, vtkFloatArray*);
  void MakeupShaders(vtkOpenGLShaderProperty* sp);

  vtkDataSet* Input = nullptr;
  char* LabelFormat = nullptr;
  int LabelMode = VTK_LABEL_IDS;
  int LabeledComponent = -1;
  int FieldDataArray = 0;
  char* FieldDataName = nullptr;
  char ComponentSeparator = ' ';
  int TextAnchor = vtkFastLabeledDataMapper::Center;

  int NumberOfLabels;
  int NumberOfLabelsAllocated = 0;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Implementation;

  char* FrameColorsName = nullptr;

private:
  vtkFastLabeledDataMapper(const vtkFastLabeledDataMapper&) = delete;
  void operator=(const vtkFastLabeledDataMapper&) = delete;

protected:
  void BuildShaders(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act) override;
};
VTK_ABI_NAMESPACE_END

#endif
