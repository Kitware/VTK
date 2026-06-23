//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "Array2D.h"
#include "ArrayConversion.h"

namespace viskores_device
{

Array2D::Array2D(ViskoresDeviceGlobalState* state, const Array2DMemoryDescriptor& d)
  : helium::Array2D(state, d)
{
  state->objectCounts.arrays++;
}

Array2D::~Array2D()
{
  asViskoresDeviceState(deviceState())->objectCounts.arrays--;
}

void Array2D::unmap()
{
  this->helium::Array2D::unmap();
  // Invalidate Viskores ArrayHandle
  this->m_ViskoresArray.ReleaseResources();
}

viskores::cont::UnknownArrayHandle Array2D::dataAsViskoresArray() const
{
  if (!this->m_ViskoresArray.IsValid())
  {
    // Pull data from ANARI into Viskores.
    const_cast<Array2D*>(this)->m_ViskoresArray = ANARIArrayToViskoresArray(this);
  }

  return this->m_ViskoresArray;
}

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_DEFINITION(viskores_device::Array2D*);
