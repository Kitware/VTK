// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkBlockProperties.h"

#include "vtkAlgorithm.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkBlockProperties);

//------------------------------------------------------------------------------
vtkBlockProperties::vtkBlockProperties() = default;

//------------------------------------------------------------------------------
vtkBlockProperties::~vtkBlockProperties() = default;

//------------------------------------------------------------------------------
void vtkBlockProperties::SetMapper(vtkCompositePolyDataMapper* mapper)
{
  if (this->Mapper == mapper)
  {
    return;
  }
  this->Mapper = mapper;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkBlockProperties::SetRepresentation(vtkAlgorithm* representation)
{
  if (this->Representation == representation)
  {
    return;
  }
  this->Representation = representation;
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkBlockProperties::Prepare()
{
  if (!this->Mapper || !this->Representation)
  {
    return false;
  }

  // A flat index is resolved by walking the data it names a block of, so the
  // representation has to have built that data before an index means anything.
  if (this->Representation->GetNumberOfInputConnections(0) < 1)
  {
    vtkWarningMacro("Per-block properties need the representation to have an input.");
    return false;
  }
  this->Representation->Update();

  if (!this->Mapper->GetInputDataObject(0, 0))
  {
    vtkWarningMacro("Per-block properties need the representation to have been built.");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkBlockProperties::SetVisibility(unsigned int index, bool visible)
{
  if (!this->Prepare())
  {
    return;
  }
  this->Mapper->SetBlockVisibility(index, visible);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkBlockProperties::GetVisibility(unsigned int index)
{
  if (!this->Prepare())
  {
    return true;
  }
  return this->Mapper->GetBlockVisibility(index);
}

//------------------------------------------------------------------------------
void vtkBlockProperties::SetColor(unsigned int index, double r, double g, double b)
{
  if (!this->Prepare())
  {
    return;
  }
  const double color[3] = { r, g, b };
  this->Mapper->SetBlockColor(index, color);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkBlockProperties::GetColor(unsigned int index, double color[3])
{
  if (!this->Prepare())
  {
    return;
  }
  this->Mapper->GetBlockColor(index, color);
}

//------------------------------------------------------------------------------
void vtkBlockProperties::SetOpacity(unsigned int index, double opacity)
{
  if (!this->Prepare())
  {
    return;
  }
  this->Mapper->SetBlockOpacity(index, opacity);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkBlockProperties::GetOpacity(unsigned int index)
{
  if (!this->Prepare())
  {
    return 1.0;
  }
  return this->Mapper->GetBlockOpacity(index);
}

//------------------------------------------------------------------------------
void vtkBlockProperties::Reset()
{
  if (!this->Mapper)
  {
    return;
  }
  // Nothing is resolved here, so this works whether or not there is data.
  this->Mapper->RemoveBlockVisibilities();
  this->Mapper->RemoveBlockColors();
  this->Mapper->RemoveBlockOpacities();
  this->Modified();
}

//------------------------------------------------------------------------------
vtkCompositeDataDisplayAttributes* vtkBlockProperties::GetDisplayAttributes()
{
  return this->Mapper ? this->Mapper->GetCompositeDataDisplayAttributes() : nullptr;
}

//------------------------------------------------------------------------------
void vtkBlockProperties::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Mapper: " << this->Mapper.GetPointer() << "\n";
  os << indent << "Representation: " << this->Representation.GetPointer() << "\n";
  os << indent << "DisplayAttributes: " << this->GetDisplayAttributes() << "\n";
}

VTK_ABI_NAMESPACE_END
