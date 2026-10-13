// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellTypeUtilities.h"

#include "vtkCellType.h"
#include "vtkCellTypeTraits.h"
#include "vtkGenericCell.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

#include <array>
#include <cstring>

namespace
{
// The metadata of a cell type, from its vtkCellTypeTraits, see vtkCellTypeUtilities.
struct CellTypeInfo
{
  const char* Name = nullptr;
  const char* ClassName = nullptr;
  int Dimension = -1; // -1 for a cell type without traits
  bool IsLinear = false;
};

// The metadata of the cell types with traits, at their index, built at compile time.
constexpr std::array<CellTypeInfo, VTK_NUMBER_OF_CELL_TYPES> MakeCellTypeInfos()
{
  std::array<CellTypeInfo, VTK_NUMBER_OF_CELL_TYPES> infos{};
  vtkForEachCellType(
    [&infos](auto traits)
    {
      using Traits = decltype(traits);
      infos[Traits::CellType] = { Traits::Name, Traits::ClassName, Traits::Dimension,
        Traits::IsLinear };
    });
  return infos;
}
constexpr std::array<CellTypeInfo, VTK_NUMBER_OF_CELL_TYPES> CellTypeInfos = MakeCellTypeInfos();

// The metadata of the cell type `type`, or nullptr if it has no traits.
const CellTypeInfo* GetCellTypeInfo(int type)
{
  if (type < 0 || type >= VTK_NUMBER_OF_CELL_TYPES || CellTypeInfos[type].Dimension < 0)
  {
    return nullptr;
  }
  return &CellTypeInfos[type];
}
} // anonymous namespace

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellTypeUtilities);

//------------------------------------------------------------------------------
void vtkCellTypeUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
std::string vtkCellTypeUtilities::GetTypeAsString(int typeId)
{
  const CellTypeInfo* info = GetCellTypeInfo(typeId);
  return info && info->Name ? info->Name : "Unknown Cell";
}

//------------------------------------------------------------------------------
int vtkCellTypeUtilities::GetTypeIdFromName(const std::string& name)
{
  for (int type = 0; type < VTK_NUMBER_OF_CELL_TYPES; ++type)
  {
    const char* typeName = CellTypeInfos[type].Name;
    if (typeName && name == typeName)
    {
      return type;
    }
  }
  return VTK_EMPTY_CELL;
}

//------------------------------------------------------------------------------
const char* vtkCellTypeUtilities::GetClassNameFromTypeId(int type)
{
  const CellTypeInfo* info = GetCellTypeInfo(type);
  return info && info->ClassName ? info->ClassName : "UnknownClass";
}

//------------------------------------------------------------------------------
int vtkCellTypeUtilities::GetTypeIdFromClassName(const char* classname)
{
  if (!classname)
  {
    return -1;
  }
  for (int type = 0; type < VTK_NUMBER_OF_CELL_TYPES; ++type)
  {
    const char* typeClassName = CellTypeInfos[type].ClassName;
    if (typeClassName && std::strcmp(typeClassName, classname) == 0)
    {
      return type;
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
int vtkCellTypeUtilities::GetDimension(unsigned char type)
{
  if (const CellTypeInfo* info = GetCellTypeInfo(type))
  {
    return info->Dimension;
  }
  // The other cell types of vtkCellType.h, e.g., the deprecated parametric ones, are 3D. A cell
  // type out of it must be built and queried directly, which is slow.
  if (type < VTK_NUMBER_OF_CELL_TYPES)
  {
    return 3;
  }
  vtkNew<vtkGenericCell> cell;
  cell->SetCellType(type);
  return cell->GetCellDimension();
}

//------------------------------------------------------------------------------
int vtkCellTypeUtilities::IsLinear(unsigned char type)
{
  const CellTypeInfo* info = GetCellTypeInfo(type);
  return info && info->IsLinear;
}

VTK_ABI_NAMESPACE_END
