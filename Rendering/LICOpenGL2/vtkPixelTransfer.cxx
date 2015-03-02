#include "vtkPixelTransfer.h"

//-----------------------------------------------------------------------------
int vtkPixelTransfer::Blit(
       const vtkPixelExtent &srcWholeExt,
       const vtkPixelExtent &srcExt,
       const vtkPixelExtent &destWholeExt,
       const vtkPixelExtent &destExt,
       int nSrcComps,
       int srcType,
       void *srcData,
       int nDestComps,
       int destType,
       void *destData)
{
  // first layer of dispatch
  switch(srcType)
    {
    vtkTemplateMacro(
        return vtkPixelTransfer::Blit(
            srcWholeExt,
            srcExt,
            destWholeExt,
            destExt,
            nSrcComps,
            (VTK_TT*)srcData,
            nDestComps,
            destType,
            destData));
    }
  return 0;
}
