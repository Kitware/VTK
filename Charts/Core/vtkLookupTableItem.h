// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkLookupTableItem_h
#define vtkLookupTableItem_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkScalarsToColorsItem.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkLookupTable;

// Description:
// vtkPlot::Color, vtkPlot::Brush, vtkScalarsToColors::DrawPolyLine,
// vtkScalarsToColors::MaskAboveCurve have no effect here.
class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkLookupTableItem : public vtkScalarsToColorsItem
{
public:
  static vtkLookupTableItem* New();
  vtkTypeMacro(vtkLookupTableItem, vtkScalarsToColorsItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void SetLookupTable(vtkLookupTable* t);
  vtkGetObjectMacro(LookupTable, vtkLookupTable);

protected:
  vtkLookupTableItem();
  ~vtkLookupTableItem() override;

  // Description:
  // Reimplemented to return the range of the lookup table
  void ComputeBounds(double bounds[4]) override;

  void ComputeTexture() override;
  vtkLookupTable* LookupTable;

private:
  vtkLookupTableItem(const vtkLookupTableItem&) = delete;
  void operator=(const vtkLookupTableItem&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
