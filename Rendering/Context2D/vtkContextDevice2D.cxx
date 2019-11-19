/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextDevice2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkContextDevice2D.h"
#include "vtkAbstractMapper.h" // for VTK_SCALAR_MODE defines
#include "vtkBrush.h"
#include "vtkCellIterator.h"
#include "vtkMathTextUtilities.h"
#include "vtkPen.h"
#include "vtkPolyData.h"
#include "vtkRect.h"
#include "vtkStdString.h"
#include "vtkTextProperty.h"
#include "vtkUnsignedCharArray.h"

#include "vtkObjectFactory.h"
#include <cassert>
#include <vector>

vtkAbstractObjectFactoryNewMacro(vtkContextDevice2D);

vtkContextDevice2D::vtkContextDevice2D()
{
  this->Geometry[0] = 0;
  this->Geometry[1] = 0;
  this->BufferId = nullptr;
  this->Pen = vtkPen::New();
  this->Brush = vtkBrush::New();
  this->TextProp = vtkTextProperty::New();
}

//-----------------------------------------------------------------------------
vtkContextDevice2D::~vtkContextDevice2D()
{
  this->Pen->Delete();
  this->Brush->Delete();
  this->TextProp->Delete();
}

//-----------------------------------------------------------------------------
bool vtkContextDevice2D::MathTextIsSupported()
{
  return vtkMathTextUtilities::GetInstance() != nullptr;
}

//-----------------------------------------------------------------------------
void vtkContextDevice2D::DrawPolyData(
  float p[2], float scale, vtkPolyData* polyData, vtkUnsignedCharArray* colors, int scalarMode)
{
  std::vector<float> verts;
  std::vector<unsigned char> vertColors;

  vtkCellIterator* cell = polyData->NewCellIterator();
  cell->InitTraversal();
  for (; !cell->IsDoneWithTraversal(); cell->GoToNextCell())
  {
    // To match the original implementation on the OpenGL2 backend, we only
    // handle polygons and lines:
    int cellType = cell->GetCellType();
    switch (cellType)
    {
      case VTK_LINE:
      case VTK_POLY_LINE:
      case VTK_TRIANGLE:
      case VTK_QUAD:
      case VTK_POLYGON:
        break;

      default:
        continue;
    }

    // Allocate temporary arrays:
    vtkIdType numPoints = cell->GetNumberOfPoints();
    if (numPoints == 0)
    {
      continue;
    }
    verts.resize(static_cast<size_t>(numPoints) * 2);
    vertColors.resize(static_cast<size_t>(numPoints) * 4);

    vtkIdType cellId = cell->GetCellId();
    vtkIdList* pointIds = cell->GetPointIds();
    vtkPoints* points = cell->GetPoints();

    for (vtkIdType i = 0; i < numPoints; ++i)
    {
      const size_t vertsIdx = 2 * static_cast<size_t>(i);
      const size_t colorIdx = 4 * static_cast<size_t>(i);

      const double* point = points->GetPoint(i);
      verts[vertsIdx] = (static_cast<float>(point[0]) + p[0]) * scale;
      verts[vertsIdx + 1] = (static_cast<float>(point[1]) + p[1]) * scale;

      if (scalarMode == VTK_SCALAR_MODE_USE_POINT_DATA)
      {
        colors->GetTypedTuple(pointIds->GetId(i), vertColors.data() + colorIdx);
      }
      else
      {
        colors->GetTypedTuple(cellId, vertColors.data() + colorIdx);
      }
    }

    if (cellType == VTK_LINE || cellType == VTK_POLY_LINE )
    {
      this->DrawPoly(verts.data(), numPoints, vertColors.data(), 4);
    }
    else
    {
      this->DrawColoredPolygon(verts.data(), numPoints, vertColors.data(), 4);
    }
  }
  cell->Delete();
}

//-----------------------------------------------------------------------------
void vtkContextDevice2D::ApplyPen(vtkPen* pen)
{
  this->Pen->DeepCopy(pen);
}

//-----------------------------------------------------------------------------
void vtkContextDevice2D::ApplyBrush(vtkBrush* brush)
{
  this->Brush->DeepCopy(brush);
}

//-----------------------------------------------------------------------------
void vtkContextDevice2D::ApplyTextProp(vtkTextProperty* prop)
{
  // This is a deep copy, but is called shallow for some reason...
  this->TextProp->ShallowCopy(prop);
}

// ----------------------------------------------------------------------------
bool vtkContextDevice2D::GetBufferIdMode() const
{
  return this->BufferId != nullptr;
}

// ----------------------------------------------------------------------------
void vtkContextDevice2D::BufferIdModeBegin(vtkAbstractContextBufferId* bufferId)
{
  assert("pre: not_yet" && !this->GetBufferIdMode());
  assert("pre: bufferId_exists" && bufferId != nullptr);

  this->BufferId = bufferId;

  assert("post: started" && this->GetBufferIdMode());
}

// ----------------------------------------------------------------------------
void vtkContextDevice2D::BufferIdModeEnd()
{
  assert("pre: started" && this->GetBufferIdMode());

  this->BufferId = nullptr;

  assert("post: done" && !this->GetBufferIdMode());
}

//-----------------------------------------------------------------------------
void vtkContextDevice2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Pen: ";
  this->Pen->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Brush: ";
  this->Brush->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Text Property: ";
  this->TextProp->PrintSelf(os, indent.GetNextIndent());
}

//-----------------------------------------------------------------------------
void vtkContextDevice2D::DrawMarkers(int, bool, float*, int, unsigned char*, int) {}

//-----------------------------------------------------------------------------
void vtkContextDevice2D::DrawColoredPolygon(float*, int, unsigned char*, int)
{
  vtkErrorMacro("DrawColoredPolygon not implemented on this device.");
}
