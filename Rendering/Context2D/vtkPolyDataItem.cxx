/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyDataItem.h"
#include "vtkAbstractMapper.h"
#include "vtkContext2D.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPolyData.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkPolyDataItem);

vtkCxxSetObjectMacro(vtkPolyDataItem, PolyData, vtkPolyData);

vtkCxxSetObjectMacro(vtkPolyDataItem, MappedColors, vtkUnsignedCharArray);

class vtkPolyDataItem::DrawHintsHelper
{

public:
  DrawHintsHelper()
  {
    this->previousLineType = 0;
    this->previousLineWidth = 0.0f;
  }

  /**
   * Retrieve drawing hints as field data from the polydata and use the
   * provided context2D to apply them
   */
  void ApplyDrawHints(vtkContext2D* painter, vtkPolyData* polyData)
  {
    vtkFieldData* fieldData = polyData->GetFieldData();

    vtkIntArray* stippleArray =
      vtkIntArray::SafeDownCast(fieldData->GetAbstractArray("StippleType"));

    vtkFloatArray* lineWidthArray =
      vtkFloatArray::SafeDownCast(fieldData->GetAbstractArray("LineWidth"));

    vtkPen* pen = painter->GetPen();

    this->previousLineType = pen->GetLineType();
    this->previousLineWidth = pen->GetWidth();

    if (stippleArray != nullptr)
    {
      pen->SetLineType(stippleArray->GetValue(0));
    }

    if (lineWidthArray != nullptr)
    {
      pen->SetWidth(lineWidthArray->GetValue(0));
    }
  }

  /**
   * "Un-apply" hints by restoring saved drawing state
   */
  void RemoveDrawHints(vtkContext2D* painter)
  {
    vtkPen* pen = painter->GetPen();
    pen->SetLineType(this->previousLineType);
    pen->SetWidth(this->previousLineWidth);
  };

private:
  DrawHintsHelper(const DrawHintsHelper&) = delete;
  void operator=(const DrawHintsHelper&) = delete;

  int previousLineType;
  float previousLineWidth;
};

//-----------------------------------------------------------------------------
vtkPolyDataItem::vtkPolyDataItem()
  : PolyData(nullptr)
  , MappedColors(nullptr)
  , ScalarMode(VTK_SCALAR_MODE_USE_POINT_DATA)
{
  this->Position[0] = this->Position[1] = 0;
  this->HintHelper = new vtkPolyDataItem::DrawHintsHelper();
}

//-----------------------------------------------------------------------------
vtkPolyDataItem::~vtkPolyDataItem()
{
  this->SetPolyData(nullptr);
  this->SetMappedColors(nullptr);
  delete this->HintHelper;
}

//-----------------------------------------------------------------------------
bool vtkPolyDataItem::Paint(vtkContext2D* painter)
{
  if (this->PolyData && this->MappedColors)
  {
    this->HintHelper->ApplyDrawHints(painter, this->PolyData);

    // Draw the PolyData in the bottom left corner of the item.
    painter->DrawPolyData(
      this->Position[0], this->Position[1], this->PolyData, this->MappedColors, this->ScalarMode);

    this->HintHelper->RemoveDrawHints(painter);
  }

  return true;
}

//-----------------------------------------------------------------------------
void vtkPolyDataItem::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}
