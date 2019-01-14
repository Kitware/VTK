/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabeledDataMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLabeledDataMapper
 * @brief   draw text labels at dataset points
 *
 * vtkLabeledDataMapper is a mapper that renders text at dataset
 * points. Various items can be labeled including point ids, scalars,
 * vectors, normals, texture coordinates, tensors, and field data components.
 *
 * The format with which the label is drawn is specified using a
 * printf style format string. The font attributes of the text can
 * be set through the vtkTextProperty associated to this mapper.
 *
 * By default, all the components of multi-component data such as
 * vectors, normals, texture coordinates, tensors, and multi-component
 * scalars are labeled. However, you can specify a single component if
 * you prefer. (Note: the label format specifies the format to use for
 * a single component. The label is creating by looping over all components
 * and using the label format to render each component.)
 * The character separator between components can be set. By default,
 * it is set to a single whitespace.
 *
 * @warning
 * Use this filter in combination with vtkSelectVisiblePoints if you want
 * to label only points that are visible. If you want to label cells rather
 * than points, use the filter vtkCellCenters to generate points at the
 * center of the cells. Also, you can use the class vtkIdFilter to
 * generate ids as scalars or field data, which can then be labeled.
 *
 * @sa
 * vtkMapper2D vtkActor2D vtkTextMapper vtkTextProperty vtkSelectVisiblePoints
 * vtkIdFilter vtkCellCenters
*/

#ifndef vtkLabeledDataMapper_h
#define vtkLabeledDataMapper_h

#include "vtkRenderingLabelModule.h" // For export macro
#include "vtkMapper2D.h"

#include <cassert> // For assert macro

class vtkDataObject;
class vtkDataSet;
class vtkTextMapper;
class vtkTextProperty;
class vtkTransform;

#define VTK_LABEL_IDS        0
#define VTK_LABEL_SCALARS    1
#define VTK_LABEL_VECTORS    2
#define VTK_LABEL_NORMALS    3
#define VTK_LABEL_TCOORDS    4
#define VTK_LABEL_TENSORS    5
#define VTK_LABEL_FIELD_DATA 6

class VTKRENDERINGLABEL_EXPORT vtkLabeledDataMapper : public vtkMapper2D
{
public:
  /**
   * Instantiate object with %%-#6.3g label format. By default, point ids
   * are labeled.
   */
  static vtkLabeledDataMapper *New();

  vtkTypeMacro(vtkLabeledDataMapper,vtkMapper2D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
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
  //@}

  //@{
  /**
   * Set/Get the component number to label if the data to print has
   * more than one component. For example, all the components of
   * scalars, vectors, normals, etc. are labeled by default
   * (LabeledComponent=(-1)). However, if this ivar is nonnegative,
   * then only the one component specified is labeled.
   */
  vtkSetMacro(LabeledComponent,int);
  vtkGetMacro(LabeledComponent,int);
  //@}

  //@{
  /**
   * Set/Get the separator between components.
   */
  vtkSetMacro(ComponentSeparator,char);
  vtkGetMacro(ComponentSeparator,char);
  //@}

  //@{
  /**
   * Set/Get the field data array to label. This instance variable is
   * only applicable if field data is labeled.  This will clear
   * FieldDataName when set.
   */
  void SetFieldDataArray(int arrayIndex);
  vtkGetMacro(FieldDataArray,int);
  //@}

  //@{
  /**
   * Set/Get the name of the field data array to label.  This instance
   * variable is only applicable if field data is labeled.  This will
   * override FieldDataArray when set.
   */
  void SetFieldDataName(const char *arrayName);
  vtkGetStringMacro(FieldDataName);
  //@}

  /**
   * Set the input dataset to the mapper. This mapper handles any type of data.
   */
  virtual void SetInputData(vtkDataObject*);

  /**
   * Use GetInputDataObject() to get the input data object for composite
   * datasets.
   */
  vtkDataSet *GetInput();

  //@{
  /**
   * Specify which data to plot: IDs, scalars, vectors, normals, texture coords,
   * tensors, or field data. If the data has more than one component, use
   * the method SetLabeledComponent to control which components to plot.
   * The default is VTK_LABEL_IDS.
   */
  vtkSetMacro(LabelMode, int);
  vtkGetMacro(LabelMode, int);
  void SetLabelModeToLabelIds() {this->SetLabelMode(VTK_LABEL_IDS);};
  void SetLabelModeToLabelScalars() {this->SetLabelMode(VTK_LABEL_SCALARS);};
  void SetLabelModeToLabelVectors() {this->SetLabelMode(VTK_LABEL_VECTORS);};
  void SetLabelModeToLabelNormals() {this->SetLabelMode(VTK_LABEL_NORMALS);};
  void SetLabelModeToLabelTCoords() {this->SetLabelMode(VTK_LABEL_TCOORDS);};
  void SetLabelModeToLabelTensors() {this->SetLabelMode(VTK_LABEL_TENSORS);};
  void SetLabelModeToLabelFieldData()
            {this->SetLabelMode(VTK_LABEL_FIELD_DATA);};
  //@}

  //@{
  /**
   * Set/Get the text property.
   * If an integer argument is provided, you may provide different text
   * properties for different label types. The type is determined by an
   * optional type input array.
   */
  virtual void SetLabelTextProperty(vtkTextProperty *p)
    { this->SetLabelTextProperty(p, 0); }
  virtual vtkTextProperty* GetLabelTextProperty()
    { return this->GetLabelTextProperty(0); }
  virtual void SetLabelTextProperty(vtkTextProperty *p, int type);
  virtual vtkTextProperty* GetLabelTextProperty(int type);
  //@}

  //@{
  /**
   * Draw the text to the screen at each input point.
   */
  void RenderOpaqueGeometry(vtkViewport* viewport, vtkActor2D* actor) override;
  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor) override;
  //@}

  /**
   * Release any graphics resources that are being consumed by this actor.
   */
  void ReleaseGraphicsResources(vtkWindow *) override;

  //@{
  /**
   * The transform to apply to the labels before mapping to 2D.
   */
  vtkGetObjectMacro(Transform, vtkTransform);
  void SetTransform(vtkTransform* t);
  //@}

  /// Coordinate systems that output dataset may use.
  enum Coordinates
  {
    WORLD=0,           //!< Output 3-D world-space coordinates for each label anchor.
    DISPLAY=1          //!< Output 2-D display coordinates for each label anchor (3 components but only 2 are significant).
  };

  //@{
  /**
   * Set/get the coordinate system used for output labels.
   * The output datasets may have point coordinates reported in the world space or display space.
   */
  vtkGetMacro(CoordinateSystem,int);
  vtkSetClampMacro(CoordinateSystem,int,WORLD,DISPLAY);
  void CoordinateSystemWorld() { this->SetCoordinateSystem( vtkLabeledDataMapper::WORLD ); }
  void CoordinateSystemDisplay() { this->SetCoordinateSystem( vtkLabeledDataMapper::DISPLAY ); }
  //@}

  /**
   * Return the modified time for this object.
   */
  vtkMTimeType GetMTime() override;

  //@{
  /**
   * Return the number of labels rendered by the mapper.
   */
  vtkGetMacro(NumberOfLabels, int)
  //@}

  //@{
  /**
   * Return the position of the requested label.
   */
  void GetLabelPosition(int label, double pos[3])
  {
    assert("label index range" && label >= 0 && label < this->NumberOfLabels);
    pos[0] = this->LabelPositions[3 * label];
    pos[1] = this->LabelPositions[3 * label + 1];
    pos[2] = this->LabelPositions[3 * label + 2];
  }
  //@}

  /**
   * Return the text for the requested label.
   */
  const char *GetLabelText(int label);

protected:
  vtkLabeledDataMapper();
  ~vtkLabeledDataMapper() override;

  vtkDataSet *Input;

  char  *LabelFormat;
  int   LabelMode;
  int   LabeledComponent;
  int   FieldDataArray;
  char  *FieldDataName;
  int CoordinateSystem;

  char  ComponentSeparator;

  vtkTimeStamp BuildTime;

  int NumberOfLabels;
  int NumberOfLabelsAllocated;
  vtkTextMapper **TextMappers;
  double* LabelPositions;
  vtkTransform *Transform;

  int FillInputPortInformation(int, vtkInformation*) override;

  void AllocateLabels(int numLabels);
  void BuildLabels();
  void BuildLabelsInternal(vtkDataSet*);

  class Internals;
  Internals* Implementation;

private:
  vtkLabeledDataMapper(const vtkLabeledDataMapper&) = delete;
  void operator=(const vtkLabeledDataMapper&) = delete;
};

#endif

