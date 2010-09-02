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

#ifndef __vtkLookupTableItem_h
#define __vtkLookupTableItem_h

#include "vtkScalarsToColorsItem.h"

class vtkLookupTable;

// Description:
// vtkPlot::Color, vtkPlot::Brush, vtkScalarsToColors::DrawPolyLine,
// vtkScalarsToColors::MaskAboveCurve have no effect here.
class VTK_CHARTS_EXPORT vtkLookupTableItem: public vtkScalarsToColorsItem
{
public:
  static vtkLookupTableItem* New();
  vtkTypeMacro(vtkLookupTableItem, vtkScalarsToColorsItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Reimplemented to return the range of the lookup table
  virtual void GetBounds(double bounds[4]);

  void SetLookupTable(vtkLookupTable* t);
  vtkGetObjectMacro(LookupTable, vtkLookupTable);

protected:
  vtkLookupTableItem();
  virtual ~vtkLookupTableItem();

  virtual void ComputeTexture();
  vtkLookupTable* LookupTable;

private:
  vtkLookupTableItem(const vtkLookupTableItem &); // Not implemented.
  void operator=(const vtkLookupTableItem &); // Not implemented.
};

#endif
