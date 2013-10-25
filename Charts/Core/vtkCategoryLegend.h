/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCategoryLegend.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkCategoryLegend - Legend item to display categorical data.
// .SECTION Description
// vtkCategoryLegend will display a label and color patch for each value
// in a categorical data set.  To use this class, you must first populate
// a vtkScalarsToColors by using the SetAnnotation() method.  The other
// input to this class is a vtkVariantArray.  This should contain the
// annotated values from the vtkScalarsToColors that you wish to include
// within the legend.

#ifndef __vtkCategoryLegend_h
#define __vtkCategoryLegend_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkChartLegend.h"
#include "vtkNew.h"              // For vtkNew ivars
#include "vtkStdString.h"        // For vtkStdString ivars
#include "vtkVector.h"           // For vtkRectf

class vtkScalarsToColors;
class vtkTextProperty;
class vtkVariantArray;

class VTKCHARTSCORE_EXPORT vtkCategoryLegend: public vtkChartLegend
{
public:
  vtkTypeMacro(vtkCategoryLegend, vtkChartLegend);
  static vtkCategoryLegend* New();

  // Description:
  // Enum of legend orientation types
  enum {
    VERTICAL = 0,
    HORIZONTAL
  };

  // Description:
  // Paint the legend into a rectangle defined by the bounds.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Compute and return the lower left corner of this legend, along
  // with its width and height.
  virtual vtkRectf GetBoundingRect(vtkContext2D* painter);

  // Description:
  // Get/Set the vtkScalarsToColors used to draw this legend.
  // Since this legend represents categorical data, this
  // vtkScalarsToColors must have been populated using SetAnnotation().
  virtual void SetScalarsToColors(vtkScalarsToColors* stc);
  virtual vtkScalarsToColors * GetScalarsToColors();

  // Description:
  // Get/Set the array of values that will be represented by this legend.
  // This array must contain distinct annotated values from the ScalarsToColors.
  // Each value in this array will be drawn as a separate entry within this
  // legend.
  vtkGetMacro(Values, vtkVariantArray*);
  vtkSetMacro(Values, vtkVariantArray*);

  // Description:
  // Get/set the title text of the legend.
  virtual void SetTitle(const vtkStdString &title);
  virtual vtkStdString GetTitle();

  // Description:
  // Get/set the label to use for outlier data.
  vtkGetMacro(OutlierLabel, vtkStdString);
  vtkSetMacro(OutlierLabel, vtkStdString);

protected:
  vtkCategoryLegend();
  virtual ~vtkCategoryLegend();

  bool                                HasOutliers;
  float                               TitleWidthOffset;
  vtkScalarsToColors*                 ScalarsToColors;
  vtkStdString                        OutlierLabel;
  vtkStdString                        Title;
  vtkNew<vtkTextProperty>             TitleProperties;
  vtkVariantArray*                    Values;

private:
  vtkCategoryLegend(const vtkCategoryLegend &); // Not implemented.
  void operator=(const vtkCategoryLegend &);   // Not implemented.
};

#endif
