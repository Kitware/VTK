/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLookupTableItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkLookupTableItem_h
#define vtkLookupTableItem_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkScalarsToColorsItem.h"

class vtkLookupTable;

// Description:
// vtkPlot::Color, vtkPlot::Brush, vtkScalarsToColors::DrawPolyLine,
// vtkScalarsToColors::MaskAboveCurve have no effect here.
class VTKCHARTSCORE_EXPORT vtkLookupTableItem: public vtkScalarsToColorsItem
{
public:
  static vtkLookupTableItem* New();
  vtkTypeMacro(vtkLookupTableItem, vtkScalarsToColorsItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  void SetLookupTable(vtkLookupTable* t);
  vtkGetObjectMacro(LookupTable, vtkLookupTable);

protected:
  vtkLookupTableItem();
  virtual ~vtkLookupTableItem();

  // Description:
  // Reimplemented to return the range of the lookup table
  virtual void ComputeBounds(double bounds[4]);


  virtual void ComputeTexture();
  vtkLookupTable* LookupTable;

private:
  vtkLookupTableItem(const vtkLookupTableItem &); // Not implemented.
  void operator=(const vtkLookupTableItem &); // Not implemented.
};

#endif
