/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPixelTransfer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPixelTransfer -- For movement of pixel data described by
// pixel extents
//
// .SECTION Description
// Class to handle non-contiguous data transfers of data described
// by pixel extents within a process. For transfering data between
// processes see vtkPPixelTransfer.
//
// .SECTION See also
// vtkPixelExtent vtkPPixelTransfer

#ifndef __vtkPixelTransfer_h
#define __vtkPixelTransfer_h

#include "vtkRenderingLICModule.h" // for export
#include "vtkSetGet.h" // for macros
#include "vtkPixelExtent.h" // for pixel extent
#include <cstring> // for memcpy

class VTKRENDERINGLIC_EXPORT vtkPixelTransfer
{
public:
  vtkPixelTransfer(){}
  ~vtkPixelTransfer(){}

  // Description:
  // for  memory to memory transfers. Conveinience api for working
  // with vtk type enum rather than c-data types and simple extents.
  static
  int Blit(
         const vtkPixelExtent &ext,
         int nComps,
         int srcType,
         void *srcData,
         int destType,
         void *destData);

  // Description:
  // for  memory to memory transfers. Conveinience api for working
  // with vtk type enum rather than c-data types.
  static
  int Blit(
         const vtkPixelExtent &srcWhole,
         const vtkPixelExtent &srcSubset,
         const vtkPixelExtent &destWhole,
         const vtkPixelExtent &destSubset,
         int nSrcComps,
         int srcType,
         void *srcData,
         int nDestComps,
         int destType,
         void *destData);

  // Description:
  // for local memory to memory transfers
  template<typename SOURCE_TYPE, typename DEST_TYPE>
  static
  int Blit(
         const vtkPixelExtent &srcWhole,
         const vtkPixelExtent &srcSubset,
         const vtkPixelExtent &destWhole,
         const vtkPixelExtent &destSubset,
         int nSrcComps,
         SOURCE_TYPE *srcData,
         int nDestComps,
         DEST_TYPE *destData);

private:
  // distpatch helper for vtk data type enum
  template<typename SOURCE_TYPE>
  static
  int Blit(
         const vtkPixelExtent &srcWhole,
         const vtkPixelExtent &srcSubset,
         const vtkPixelExtent &destWhole,
         const vtkPixelExtent &destSubset,
         int nSrcComps,
         SOURCE_TYPE *srcData,
         int nDestComps,
         int destType,
         void *destData);
};

//-----------------------------------------------------------------------------
inline
int vtkPixelTransfer::Blit(
         const vtkPixelExtent &ext,
         int nComps,
         int srcType,
         void *srcData,
         int destType,
         void *destData)
{
  return vtkPixelTransfer::Blit(
        ext,
        ext,
        ext,
        ext,
        nComps,
        srcType,
        srcData,
        nComps,
        destType,
        destData);
}


//-----------------------------------------------------------------------------
template<typename SOURCE_TYPE>
int vtkPixelTransfer::Blit(
       const vtkPixelExtent &srcWholeExt,
       const vtkPixelExtent &srcExt,
       const vtkPixelExtent &destWholeExt,
       const vtkPixelExtent &destExt,
       int nSrcComps,
       SOURCE_TYPE *srcData,
       int nDestComps,
       int destType,
       void *destData)
{
  // second layer of dispatch
  switch(destType)
    {
    vtkTemplateMacro(
        return vtkPixelTransfer::Blit(
            srcWholeExt,
            srcExt,
            destWholeExt,
            destExt,
            nSrcComps,
            srcData,
            nDestComps,
            (VTK_TT*)destData););
    }
  return 0;
}

//-----------------------------------------------------------------------------
template<typename SOURCE_TYPE, typename DEST_TYPE>
int vtkPixelTransfer::Blit(
       const vtkPixelExtent &srcWholeExt,
       const vtkPixelExtent &srcSubset,
       const vtkPixelExtent &destWholeExt,
       const vtkPixelExtent &destSubset,
       int nSrcComps,
       SOURCE_TYPE *srcData,
       int nDestComps,
       DEST_TYPE *destData)
{
  if ( (srcData == NULL) || (destData == NULL) )
    {
    return -1;
    }
  if ( (srcWholeExt == srcSubset)
    && (destWholeExt == destSubset)
    && (nSrcComps == nDestComps) )
    {
    // buffers are contiguous
    size_t n = srcWholeExt.Size()*nSrcComps;
    for (size_t i=0; i<n; ++i)
      {
      destData[i] = static_cast<DEST_TYPE>(srcData[i]);
      }
    }
  else
    {
    // buffers are not contiguous
    int tmp[2];

    // get the dimensions of the arrays
    srcWholeExt.Size(tmp);
    int swnx = tmp[0];

    destWholeExt.Size(tmp);
    int dwnx = tmp[0];

    // move from logical extent to memory extent
    vtkPixelExtent srcExt(srcSubset);
    srcExt.Shift(srcWholeExt);

    vtkPixelExtent destExt(destSubset);
    destExt.Shift(destWholeExt);

    // get size of sub-set to copy (it's the same in src and dest)
    int nxny[2];
    srcExt.Size(nxny);

    // use smaller ncomps for loop index to avoid reading/writing
    // invalid mem
    int nCopyComps = nSrcComps < nDestComps ? nSrcComps : nDestComps;

    for (int j=0; j<nxny[1]; ++j)
      {
      int sjj = swnx*(srcExt[2]+j)+srcExt[0];
      int djj = dwnx*(destExt[2]+j)+destExt[0];
      for (int i=0; i<nxny[0]; ++i)
        {
        int sidx = nSrcComps*(sjj+i);
        int didx = nDestComps*(djj+i);
        // copy values from source
        for (int p=0; p<nCopyComps; ++p)
          {
          destData[didx+p] = static_cast<DEST_TYPE>(srcData[sidx+p]);
          }
        // ensure all dest comps are initialized
        for (int p=nCopyComps; p<nDestComps; ++p)
          {
          destData[didx+p] = static_cast<DEST_TYPE>(0);
          }
        }
      }
    }
  return 0;
}

ostream &operator<<(ostream &os, const vtkPixelTransfer &gt);

#endif
// VTK-HeaderTest-Exclude: vtkPixelTransfer.h
