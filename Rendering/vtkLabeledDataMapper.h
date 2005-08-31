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
// .NAME vtkLabeledDataMapper - draw text labels at dataset points
// .SECTION Description
// vtkLabeledDataMapper is a mapper that renders text at dataset
// points. Various items can be labeled including point ids, scalars,
// vectors, normals, texture coordinates, tensors, and field data components.
//
// The format with which the label is drawn is specified using a
// printf style format string. The font attributes of the text can
// be set through the vtkTextProperty associated to this mapper. 
//
// By default, all the components of multi-component data such as
// vectors, normals, texture coordinates, tensors, and multi-component
// scalars are labeled. However, you can specify a single component if
// you prefer. (Note: the label format specifies the format to use for
// a single component. The label is creating by looping over all components
// and using the label format to render each component.)

// .SECTION Caveats
// Use this filter in combination with vtkSelectVisiblePoints if you want
// to label only points that are visible. If you want to label cells rather
// than points, use the filter vtkCellCenters to generate points at the
// center of the cells. Also, you can use the class vtkIdFilter to
// generate ids as scalars or field data, which can then be labeled.

// .SECTION See Also
// vtkMapper2D vtkActor2D vtkTextMapper vtkTextProperty vtkSelectVisiblePoints 
// vtkIdFilter vtkCellCenters

#ifndef __vtkLabeledDataMapper_h
#define __vtkLabeledDataMapper_h

#include "vtkMapper2D.h"

class vtkDataSet;
class vtkTextMapper;
class vtkTextProperty;

#define VTK_LABEL_IDS        0
#define VTK_LABEL_SCALARS    1
#define VTK_LABEL_VECTORS    2
#define VTK_LABEL_NORMALS    3
#define VTK_LABEL_TCOORDS    4
#define VTK_LABEL_TENSORS    5
#define VTK_LABEL_FIELD_DATA 6

class VTK_RENDERING_EXPORT vtkLabeledDataMapper : public vtkMapper2D
{
public:
  // Description:
  // Instantiate object with %%-#6.3g label format. By default, point ids
  // are labeled.
  static vtkLabeledDataMapper *New();

  vtkTypeRevisionMacro(vtkLabeledDataMapper,vtkMapper2D);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the format with which to print the labels. The format needs
  // to change depending on what you're trying to print. For example, if
  // you're printing a vector, 3 values are printed, whereas when printing an
  // id only one value is printed. See also the ivar LabeledComponent which
  // can be used to specify the component to print if you want to only print
  // one of several.
  vtkSetStringMacro(LabelFormat);
  vtkGetStringMacro(LabelFormat);

  // Description:
  // Set/Get the component number to label if the data to print has
  // more than one component. For example, all the components of
  // scalars, vectors, normals, etc. are labeled by default
  // (LabeledComponent=(-1)). However, if this ivar is nonnegative,
  // then only the one component specified is labeled.
  vtkSetMacro(LabeledComponent,int);
  vtkGetMacro(LabeledComponent,int);

  // Description:
  // Set/Get the field data array to label. This instance variable is
  // only applicable if field data is labeled.
  vtkSetClampMacro(FieldDataArray,int,0,VTK_LARGE_INTEGER);
  vtkGetMacro(FieldDataArray,int);

  // Description:
  // Set the input dataset to the mapper. This mapper handles any type of data.
  virtual void SetInput(vtkDataSet*);
  vtkDataSet *GetInput();

  // Description:
  // Specify which data to plot: scalars, vectors, normals, texture coords,
  // tensors, or field data. If the data has more than one component, use
  // the method SetLabeledComponent to control which components to plot.
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

  // Description:
  // Set/Get the text property.
  virtual void SetLabelTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(LabelTextProperty,vtkTextProperty);

  // Description:
  // Draw the text to the screen at each input point.
  void RenderOpaqueGeometry(vtkViewport* viewport, vtkActor2D* actor);
  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  virtual void ReleaseGraphicsResources(vtkWindow *);

protected:
  vtkLabeledDataMapper();
  ~vtkLabeledDataMapper();

  vtkDataSet *Input;
  vtkTextProperty *LabelTextProperty;

  char  *LabelFormat;
  int   LabelMode;
  int   LabeledComponent;
  int   FieldDataArray;

  vtkTimeStamp BuildTime;

private:
  int NumberOfLabels;
  int NumberOfLabelsAllocated;
  vtkTextMapper **TextMappers;

  virtual int FillInputPortInformation(int, vtkInformation*);

private:
  vtkLabeledDataMapper(const vtkLabeledDataMapper&);  // Not implemented.
  void operator=(const vtkLabeledDataMapper&);  // Not implemented.
};

#endif

