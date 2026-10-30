// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDeviceArrayFactory.h"

#include "vtkDeviceDataArray.h"
#include "vtkDeviceMemory.h"
#include "vtkMemoryDescriptor.h"
#include "vtkObjectFactory.h"

#include <cstring>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDeviceArrayFactory);

//------------------------------------------------------------------------------
vtkDeviceArrayFactory::vtkDeviceArrayFactory() = default;

//------------------------------------------------------------------------------
vtkDeviceArrayFactory::~vtkDeviceArrayFactory() = default;

//------------------------------------------------------------------------------
void vtkDeviceArrayFactory::AddBuffer(vtkMemoryDescriptor* descriptor)
{
  this->Buffer = descriptor;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkDeviceArrayFactory::ClearBuffers()
{
  this->Buffer = nullptr;
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkDeviceArrayFactory::GetNumberOfBuffers() const
{
  return this->Buffer ? 1 : 0;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkDeviceArrayFactory::CreateArray()
{
  if (!this->Buffer)
  {
    vtkErrorMacro("No buffer to make an array of.");
    return nullptr;
  }
  const char* role = this->Buffer->GetRole();
  if (role && std::strcmp(role, "data") != 0)
  {
    vtkErrorMacro(
      "Only interleaved values (a \"data\" buffer) are supported, not '" << role << "'.");
    return nullptr;
  }

  vtkDataArray* array = nullptr;
  switch (this->DataType)
  {
    vtkTemplateMacro({
      auto* typed = vtkDeviceDataArray<VTK_TT>::New();
      if (typed->SetDescriptor(this->Buffer, this->NumberOfTuples, this->NumberOfComponents))
      {
        array = typed;
      }
      else
      {
        typed->Delete();
      }
    });
    default:
      vtkErrorMacro("VTK data type " << this->DataType << " cannot be held in device memory.");
      break;
  }
  return array;
}

//------------------------------------------------------------------------------
bool vtkDeviceArrayFactory::IsMemorySpaceAvailable(const char* space)
{
  vtkDataArray::MemorySpace device;
  return vtkDeviceMemory::SpaceFromName(space, device) && vtkDeviceMemory::IsAvailable(device);
}

//------------------------------------------------------------------------------
void vtkDeviceArrayFactory::SetLevelZeroContext(
  vtkTypeUInt64 driver, vtkTypeUInt64 device, vtkTypeUInt64 context)
{
  vtkDeviceMemory::SetLevelZeroContext(reinterpret_cast<void*>(driver),
    reinterpret_cast<void*>(device), reinterpret_cast<void*>(context));
}

//------------------------------------------------------------------------------
bool vtkDeviceArrayFactory::HasLevelZeroContext()
{
  return vtkDeviceMemory::HasLevelZeroContext();
}

namespace
{
// One of VTK's Level Zero handles, making the context if there is none: 0, the
// driver, the device or the context.
vtkTypeUInt64 LevelZeroHandle(int which)
{
  void* handles[3];
  if (!vtkDeviceMemory::GetLevelZeroContext(handles[0], handles[1], handles[2]))
  {
    return 0;
  }
  return reinterpret_cast<vtkTypeUInt64>(handles[which]);
}
}

//------------------------------------------------------------------------------
vtkTypeUInt64 vtkDeviceArrayFactory::GetLevelZeroDriverHandle()
{
  return LevelZeroHandle(0);
}

//------------------------------------------------------------------------------
vtkTypeUInt64 vtkDeviceArrayFactory::GetLevelZeroDeviceHandle()
{
  return LevelZeroHandle(1);
}

//------------------------------------------------------------------------------
vtkTypeUInt64 vtkDeviceArrayFactory::GetLevelZeroContextHandle()
{
  return LevelZeroHandle(2);
}

//------------------------------------------------------------------------------
int vtkDeviceArrayFactory::GetDeviceOrdinal(vtkMemoryDescriptor* descriptor)
{
  vtkDataArray::MemorySpace space;
  if (!descriptor || !vtkDeviceMemory::SpaceFromName(descriptor->GetMemorySpace(), space))
  {
    return -1;
  }
  return vtkDeviceMemory::DeviceOrdinal(
    space, reinterpret_cast<const void*>(descriptor->GetPointer()));
}

//------------------------------------------------------------------------------
void vtkDeviceArrayFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfTuples: " << this->NumberOfTuples << "\n";
  os << indent << "NumberOfComponents: " << this->NumberOfComponents << "\n";
  os << indent << "DataType: " << this->DataType << "\n";
  os << indent << "Buffer: " << (this->Buffer ? "set" : "(none)") << "\n";
}

VTK_ABI_NAMESPACE_END
