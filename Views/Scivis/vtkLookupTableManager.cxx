// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkLookupTableManager.h"

#include "vtkColor.h"
#include "vtkColorSeries.h"
#include "vtkColorTransferFunction.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkScalarsToColors.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <string>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkLookupTableManager);

//------------------------------------------------------------------------------
class vtkLookupTableManager::vtkInternals
{
public:
  std::map<std::string, vtkSmartPointer<vtkScalarsToColors>> Tables;
};

//------------------------------------------------------------------------------
vtkLookupTableManager::vtkLookupTableManager()
  : Internals(new vtkInternals)
  , DefaultColorScheme(vtkColorSeries::BREWER_DIVERGING_SPECTRAL_11)
  , NumberOfTableValues(256)
{
}

//------------------------------------------------------------------------------
vtkLookupTableManager::~vtkLookupTableManager()
{
  delete this->Internals;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkLookupTableManager::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  for (const auto& entry : this->Internals->Tables)
  {
    if (entry.second)
    {
      mTime = std::max(mTime, entry.second->GetMTime());
    }
  }
  return mTime;
}

//------------------------------------------------------------------------------
void vtkLookupTableManager::BuildDefaultLookupTable(vtkLookupTable* lut)
{
  if (!lut)
  {
    return;
  }

  const int count = std::max(this->NumberOfTableValues, 1);
  lut->SetNumberOfTableValues(count);

  vtkNew<vtkColorSeries> series;
  series->SetColorScheme(this->DefaultColorScheme);
  const int swatches = series->GetNumberOfColors();
  if (swatches < 1)
  {
    // Nothing to interpolate; leave the table to build its own default colors.
    vtkWarningMacro(<< "Color scheme " << this->DefaultColorScheme
                    << " has no colors. Falling back to the default lookup table colors.");
    lut->ForceBuild();
    return;
  }

  // A color series is a handful of swatches, but a scalar field wants a
  // continuous ramp. Spread the swatches evenly over the unit interval and let
  // a transfer function interpolate between them.
  vtkNew<vtkColorTransferFunction> ramp;
  for (int i = 0; i < swatches; ++i)
  {
    const vtkColor3ub color = series->GetColor(i);
    const double x = (swatches > 1) ? static_cast<double>(i) / (swatches - 1) : 0.0;
    ramp->AddRGBPoint(x, color.GetRed() / 255.0, color.GetGreen() / 255.0, color.GetBlue() / 255.0);
  }

  // Filling the entries individually is what keeps them: a table whose values
  // were inserted rather than generated is left alone by vtkLookupTable::Build().
  const int last = count - 1;
  for (int i = 0; i < count; ++i)
  {
    double rgb[3];
    ramp->GetColor((last > 0) ? static_cast<double>(i) / last : 0.0, rgb);
    lut->SetTableValue(i, rgb[0], rgb[1], rgb[2], 1.0);
  }
}

//------------------------------------------------------------------------------
vtkScalarsToColors* vtkLookupTableManager::GetLookupTable(const char* arrayName)
{
  if (!arrayName || arrayName[0] == '\0')
  {
    return nullptr;
  }

  auto& tables = this->Internals->Tables;
  auto it = tables.find(arrayName);
  if (it != tables.end())
  {
    return it->second;
  }

  vtkNew<vtkLookupTable> lut;
  this->BuildDefaultLookupTable(lut);
  tables[arrayName] = lut;
  this->Modified();
  return lut;
}

//------------------------------------------------------------------------------
bool vtkLookupTableManager::HasLookupTable(const char* arrayName)
{
  if (!arrayName || arrayName[0] == '\0')
  {
    return false;
  }
  return this->Internals->Tables.count(arrayName) > 0;
}

//------------------------------------------------------------------------------
void vtkLookupTableManager::SetLookupTable(const char* arrayName, vtkScalarsToColors* lut)
{
  if (!arrayName || arrayName[0] == '\0')
  {
    return;
  }

  if (!lut)
  {
    this->RemoveLookupTable(arrayName);
    return;
  }

  auto& tables = this->Internals->Tables;
  auto it = tables.find(arrayName);
  if (it != tables.end() && it->second == lut)
  {
    return;
  }

  tables[arrayName] = lut;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkLookupTableManager::RemoveLookupTable(const char* arrayName)
{
  if (!arrayName || arrayName[0] == '\0')
  {
    return;
  }

  if (this->Internals->Tables.erase(arrayName) > 0)
  {
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkLookupTableManager::RemoveAllLookupTables()
{
  if (!this->Internals->Tables.empty())
  {
    this->Internals->Tables.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkLookupTableManager::GetNumberOfLookupTables()
{
  return static_cast<int>(this->Internals->Tables.size());
}

//------------------------------------------------------------------------------
const char* vtkLookupTableManager::GetLookupTableName(int index)
{
  if (index < 0 || index >= this->GetNumberOfLookupTables())
  {
    return nullptr;
  }

  auto it = this->Internals->Tables.begin();
  std::advance(it, index);
  return it->first.c_str();
}

//------------------------------------------------------------------------------
void vtkLookupTableManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DefaultColorScheme: " << this->DefaultColorScheme << "\n";
  os << indent << "NumberOfTableValues: " << this->NumberOfTableValues << "\n";
  os << indent << "Number of lookup tables: " << this->Internals->Tables.size() << "\n";
  for (const auto& entry : this->Internals->Tables)
  {
    os << indent.GetNextIndent() << entry.first << ": ";
    if (entry.second)
    {
      os << entry.second->GetClassName() << " (" << entry.second << ")\n";
    }
    else
    {
      os << "(none)\n";
    }
  }
}

VTK_ABI_NAMESPACE_END
