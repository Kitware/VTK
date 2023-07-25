// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPixelTransfer.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
int vtkPixelTransfer::Blit(const vtkPixelExtent& srcWholeExt, const vtkPixelExtent& srcExt,
  const vtkPixelExtent& destWholeExt, const vtkPixelExtent& destExt, int nSrcComps, int srcType,
  void* srcData, int nDestComps, int destType, void* destData)
{
  // first layer of dispatch
  switch (srcType)
  {
    vtkTemplateMacro(return vtkPixelTransfer::Blit(srcWholeExt, srcExt, destWholeExt, destExt,
      nSrcComps, static_cast<VTK_TT*>(srcData), nDestComps, destType, destData));
  }
  return 0;
}
VTK_ABI_NAMESPACE_END
