/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabeledDataMapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkLabeledDataMapper - draw text labels at dataset points
// .SECTION Description
// vtkLabeledDataMapper is a mapper that renders text at dataset
// points. Various items can be labeled including point ids, scalars,
// vectors, normals, texture coordinates, tensors, and field data components.
//
// The format with which the label is drawn is specified using a
// printf style format string. The font attributes of the text can
// also be set (font style, size, bold, italic, shadow). The color of
// the text is controlled by the actor2D's color property (i.e., the
// actor associated with this mapper). 
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
// vtkMapper2D vtkActor2D vtkTextMapper vtkSelectVisiblePoints 
// vtkIdFilter vtkCellCenters

#ifndef __vtkLabeledDataMapper_h
#define __vtkLabeledDataMapper_h

#include "vtkMapper2D.h"
#include "vtkTextMapper.h"

#define VTK_LABEL_IDS        0
#define VTK_LABEL_SCALARS    1
#define VTK_LABEL_VECTORS    2
#define VTK_LABEL_NORMALS    3
#define VTK_LABEL_TCOORDS    4
#define VTK_LABEL_TENSORS    5
#define VTK_LABEL_FIELD_DATA 6

class vtkDataSet;

class VTK_EXPORT vtkLabeledDataMapper : public vtkMapper2D
{
public:

  // Description:
  // Instantiate object with font size 12 of font Arial (bolding,
  // italic, shadows on) and %%-#6.3g label format. By default, point ids
  // are labeled.
  vtkLabeledDataMapper();
  
  ~vtkLabeledDataMapper();
  const char *GetClassName() {return "vtkLabeledDataMapper";};
  static vtkLabeledDataMapper *New() {return new vtkLabeledDataMapper;};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Draw the text to the screen at each input point.
  void Render(vtkViewport*, vtkActor2D*);

  // Description:
  // Set the input dataset to the mapper.  
  vtkGetObjectMacro(Input, vtkDataSet);
  vtkSetObjectMacro(Input, vtkDataSet);

  // Description:
  // Set/Get the font family for the annotation text. Three font types 
  // are available: Arial (VTK_ARIAL), Courier (VTK_COURIER), and 
  // Times (VTK_TIMES).
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
  // Set/Get the suggested font size used to label the data.
  // (Suggested because not all font sizes may be available.)
  // The value is expressed in points.
  vtkSetClampMacro(FontSize,int,0,VTK_LARGE_INTEGER);
  vtkGetMacro(FontSize,int);

  // Description:
  // Enable/Disable bolding labels.
  vtkSetMacro(Bold, int);
  vtkGetMacro(Bold, int);
  vtkBooleanMacro(Bold, int);

  // Description:
  // Enable/Disable italicizing labels.
  vtkSetMacro(Italic, int);
  vtkGetMacro(Italic, int);
  vtkBooleanMacro(Italic, int);

  // Description:
  // Enable/Disable creating shadows on the labels. Shadows make 
  // the text easier to read.
  vtkSetMacro(Shadow, int);
  vtkGetMacro(Shadow, int);
  vtkBooleanMacro(Shadow, int);

  // Description:
  // Set/Get the font family for the labels. Three font types 
  // are available: Arial (VTK_ARIAL), Courier (VTK_COURIER), and 
  // Times (VTK_TIMES).
  vtkSetMacro(FontFamily, int);
  vtkGetMacro(FontFamily, int);
  void SetFontFamilyToArial() {this->SetFontFamily(VTK_ARIAL);};
  void SetFontFamilyToCourier() {this->SetFontFamily(VTK_COURIER);};
  void SetFontFamilyToTimes() {this->SetFontFamily(VTK_TIMES);};

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

protected:
  vtkDataSet *Input;
  int LabelMode;

  int   FontSize;
  int	Bold;
  int   Italic;
  int   Shadow;
  int   FontFamily;

  char  *LabelFormat;
  int   LabeledComponent;
  int   FieldDataArray;

  vtkTimeStamp BuildTime;

private:
  int NumberOfLabels;
  vtkTextMapper **TextMappers;

};

#endif

