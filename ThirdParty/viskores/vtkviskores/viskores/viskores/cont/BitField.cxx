//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/BitField.h>
#include <viskores/cont/Logging.h>

namespace viskores
{
namespace cont
{

namespace detail
{

} // namespace detail

BitField::BitField()
{
  this->Buffer.SetMetaData(internal::BitFieldMetaData{});
}

viskores::Id BitField::GetNumberOfBits() const
{
  return this->Buffer.GetMetaData<internal::BitFieldMetaData>().NumberOfBits;
}

void BitField::Allocate(viskores::Id numberOfBits,
                        viskores::CopyFlag preserve,
                        viskores::cont::Token& token) const
{
  const viskores::BufferSizeType bytesNeeded = (numberOfBits + CHAR_BIT - 1) / CHAR_BIT;
  const viskores::BufferSizeType blocksNeeded = (bytesNeeded + BlockSize - 1) / BlockSize;
  const viskores::BufferSizeType numBytes = blocksNeeded * BlockSize;

  VISKORES_LOG_F(viskores::cont::LogLevel::MemCont,
                 "BitField Allocation: %llu bits, blocked up to %s bytes.",
                 static_cast<unsigned long long>(numberOfBits),
                 viskores::cont::GetSizeString(static_cast<viskores::UInt64>(numBytes)).c_str());

  this->Buffer.SetNumberOfBytes(numBytes, preserve, token);
  this->Buffer.GetMetaData<internal::BitFieldMetaData>().NumberOfBits = numberOfBits;
}

void BitField::FillImpl(const void* word,
                        viskores::BufferSizeType wordSize,
                        viskores::cont::Token& token) const
{
  this->Buffer.Fill(word, wordSize, 0, this->Buffer.GetNumberOfBytes(), token);
}

void BitField::ReleaseResourcesExecution()
{
  this->Buffer.ReleaseDeviceResources();
}

void BitField::ReleaseResources()
{
  viskores::cont::Token token;
  this->Buffer.SetNumberOfBytes(0, viskores::CopyFlag::Off, token);
  this->Buffer.GetMetaData<internal::BitFieldMetaData>().NumberOfBits = 0;
}

void BitField::SyncControlArray() const
{
  viskores::cont::Token token;
  this->Buffer.ReadPointerHost(token);
}

bool BitField::IsOnDevice(viskores::cont::DeviceAdapterId device) const
{
  return this->Buffer.IsAllocatedOnDevice(device);
}

BitField::WritePortalType BitField::WritePortal() const
{
  viskores::cont::Token token;
  return WritePortalType(this->Buffer.WritePointerHost(token),
                         this->Buffer.GetMetaData<internal::BitFieldMetaData>().NumberOfBits);
}

BitField::ReadPortalType BitField::ReadPortal() const
{
  viskores::cont::Token token;
  return ReadPortalType(this->Buffer.ReadPointerHost(token),
                        this->Buffer.GetMetaData<internal::BitFieldMetaData>().NumberOfBits);
}

BitField::ReadPortalType BitField::PrepareForInput(viskores::cont::DeviceAdapterId device,
                                                   viskores::cont::Token& token) const
{
  return ReadPortalType(this->Buffer.ReadPointerDevice(device, token),
                        this->Buffer.GetMetaData<internal::BitFieldMetaData>().NumberOfBits);
}

BitField::WritePortalType BitField::PrepareForOutput(viskores::Id numBits,
                                                     viskores::cont::DeviceAdapterId device,
                                                     viskores::cont::Token& token) const
{
  this->Allocate(numBits, viskores::CopyFlag::Off, token);
  return WritePortalType(this->Buffer.WritePointerDevice(device, token),
                         this->Buffer.GetMetaData<internal::BitFieldMetaData>().NumberOfBits);
}

BitField::WritePortalType BitField::PrepareForInPlace(viskores::cont::DeviceAdapterId device,
                                                      viskores::cont::Token& token) const
{
  return WritePortalType(this->Buffer.WritePointerDevice(device, token),
                         this->Buffer.GetMetaData<internal::BitFieldMetaData>().NumberOfBits);
}

}
} // namespace viskores::cont
