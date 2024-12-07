// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridSidesCache.h"

#include "vtkBoundingBox.h"
#include "vtkIdTypeArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include <array>
#include <sstream>

#define VTK_DBG_MAX_HASHES 1024

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkCellGridSidesCache);

void vtkCellGridSidesCache::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Hashes: " << this->Hashes.size() << " entries\n";
  vtkIndent i2 = indent.GetNextIndent();
  vtkIndent i3 = i2.GetNextIndent();
  int numPrinted = 0;
  for (const auto& keyVal : this->Hashes)
  {
    os << i2 << std::hex << keyVal.first << std::dec << " (" << keyVal.second.Sides.size() << ")\n";
    for (const auto& side : keyVal.second.Sides)
    {
      os << i3 << side.CellType.Data() << " " << side.SideShape.Data() << " start id " << side.DOF
         << " side " << side.SideId << "\n";
    }
    ++numPrinted;
    if (numPrinted > VTK_DBG_MAX_HASHES)
    {
      if (this->Hashes.size() > static_cast<std::size_t>(numPrinted))
      {
        os << i2 << "... and " << (this->Hashes.size() - numPrinted) << " more.\n";
      }
      break;
    }
  }
}

void vtkCellGridSidesCache::Initialize()
{
  this->Hashes.clear();
  this->Modified();
}

VTK_ABI_NAMESPACE_END
