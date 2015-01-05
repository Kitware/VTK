/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractStructuredGridHelper.h"

// VTK includes
#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkIdList.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStructuredData.h"
#include "vtkStructuredExtent.h"

// C/C++ includes
#include <algorithm>
#include <cassert>
#include <vector>

// Some usefull extent macros
#define EMIN(ext, dim) (ext[2*dim])
#define EMAX(ext, dim) (ext[2*dim+1])
#define IMIN(ext) (ext[0])
#define IMAX(ext) (ext[1])
#define JMIN(ext) (ext[2])
#define JMAX(ext) (ext[3])
#define KMIN(ext) (ext[4])
#define KMAX(ext) (ext[5])

#define I(ijk) (ijk[0])
#define J(ijk) (ijk[1])
#define K(ijk) (ijk[2])

namespace vtk
{
namespace detail
{

struct vtkIndexMap
{
  std::vector<int> Mapping[3];
};

} // End namespace detail
} // End namespace vtk

vtkStandardNewMacro(vtkExtractStructuredGridHelper);

//-----------------------------------------------------------------------------
vtkExtractStructuredGridHelper::vtkExtractStructuredGridHelper()
{
  this->IndexMap = new vtk::detail::vtkIndexMap;
  this->Invalidate();
}

//-----------------------------------------------------------------------------
vtkExtractStructuredGridHelper::~vtkExtractStructuredGridHelper()
{
  delete this->IndexMap;
}

//-----------------------------------------------------------------------------
void vtkExtractStructuredGridHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
void vtkExtractStructuredGridHelper::Invalidate()
{
  this->VOI[0]= 0;
  this->VOI[1]=-1;
  this->VOI[2]= 0;
  this->VOI[3]=-1;
  this->VOI[4]= 0;
  this->VOI[5]=-1;
  this->InputWholeExtent[0]= 0;
  this->InputWholeExtent[1]=-1;
  this->InputWholeExtent[2]= 0;
  this->InputWholeExtent[3]=-1;
  this->InputWholeExtent[4]= 0;
  this->InputWholeExtent[5]=-1;
  this->SampleRate[0] = 0;
  this->SampleRate[1] = 0;
  this->SampleRate[2] = 0;
  this->IncludeBoundary = true;

  this->OutputWholeExtent[0]= 0;
  this->OutputWholeExtent[1]=-1;
  this->OutputWholeExtent[2]= 0;
  this->OutputWholeExtent[3]=-1;
  this->OutputWholeExtent[4]= 0;
  this->OutputWholeExtent[5]=-1;
}

//-----------------------------------------------------------------------------
void vtkExtractStructuredGridHelper::Initialize(
      int inVoi[6], int wholeExtent[6], int sampleRate[3], bool includeBoundary)
{
  assert("pre: NULL index map" && (this->IndexMap != NULL) );

  // Copy the VOI because we'll clamp it later:
  int voi[6];
  std::copy(inVoi, inVoi + 6, voi);

  // Have the parameters actually changed?
  if (std::equal(voi, voi + 6, this->VOI) &&
      std::equal(wholeExtent, wholeExtent + 6, this->InputWholeExtent) &&
      std::equal(sampleRate, sampleRate + 3, this->SampleRate) &&
      includeBoundary == this->IncludeBoundary)
    {
    // Nope.
    return;
    }

  // Save the input parameters so we'll know when the map is out of date
  std::copy(voi, voi + 6, this->VOI);
  std::copy(wholeExtent, wholeExtent + 6, this->InputWholeExtent);
  std::copy(sampleRate, sampleRate + 3, this->SampleRate);
  this->IncludeBoundary = includeBoundary;

  vtkBoundingBox wExtB(wholeExtent[0], wholeExtent[1], wholeExtent[2],
                       wholeExtent[3], wholeExtent[4], wholeExtent[5]);
  vtkBoundingBox voiB(voi[0],voi[1],voi[2],voi[3],voi[4],voi[5]);

  if(!wExtB.Intersects(voiB))
    {
    for(int i=0; i < 3; ++i)
      {
      this->IndexMap->Mapping[ i ].clear();
      }
    this->Invalidate();
    vtkDebugMacro(<< "Extent ["
                  << wholeExtent[0] << ", " << wholeExtent[1] << ", "
                  << wholeExtent[2] << ", " << wholeExtent[3] << ", "
                  << wholeExtent[4] << ", " << wholeExtent[5]
                  << "] does not contain VOI ["
                  << voi[0] << ", " << voi[1] << ", " << voi[2] << ", "
                  << voi[3] << ", " << voi[4] << ", " << voi[5] << "].");
    return;
    }

  // Clamp VOI to Whole Extent
  vtkStructuredExtent::Clamp(voi,wholeExtent);

  // Create mapping between output extent and input extent.
  // Compute the output whole extent in the process.
  for(int dim=0; dim < 3; ++dim)
    {
    this->IndexMap->Mapping[dim].resize(voi[2*dim+1]-voi[2*dim]+2);

    int outIdx = 0;
    // the start inIdx should account for the extent index offset
    int inIdx = voi[2 * dim] - wholeExtent[2 * dim];
    int idxSize = voi[2 * dim + 1] - wholeExtent[2 * dim];
    while (inIdx <= idxSize)
      {
      this->IndexMap->Mapping[dim][outIdx++] = inIdx;
      inIdx += sampleRate[dim];
      } // END for all points in this dimension, strided by the sample rate

    if (includeBoundary &&
        this->IndexMap->Mapping[dim][outIdx-1] != idxSize)
      {
      this->IndexMap->Mapping[dim][outIdx++] = idxSize;
      }
    this->IndexMap->Mapping[dim].resize(outIdx);

    // Update output whole extent
    this->OutputWholeExtent[2*dim]   = 0;
    this->OutputWholeExtent[2*dim+1] =
        static_cast<int>( this->IndexMap->Mapping[dim].size()-1 );
    } // END for all dimensions
}

//-----------------------------------------------------------------------------
bool vtkExtractStructuredGridHelper::IsValid() const
{
  return this->OutputWholeExtent[0] <= this->OutputWholeExtent[1] &&
         this->OutputWholeExtent[2] <= this->OutputWholeExtent[3] &&
         this->OutputWholeExtent[4] <= this->OutputWholeExtent[5];
}

//-----------------------------------------------------------------------------
int vtkExtractStructuredGridHelper::GetMappedIndex(int dim, int outIdx)
{
  // Sanity Checks
  assert("pre: dimension dim is out-of-bounds!" && dim >= 0 && dim < 3);
  assert("pre: point index out-of-bounds!" &&
         outIdx >= 0 && outIdx < this->GetSize(dim));
  return this->IndexMap->Mapping[dim][outIdx];
}

//-----------------------------------------------------------------------------
int vtkExtractStructuredGridHelper::GetMappedIndexFromExtentValue(int dim,
                                                                  int outExtVal)
{
  // Sanity Checks
  assert("pre: dimension dim is out-of-bounds!" && dim >= 0 && dim < 3);
  assert("pre: extent value out-of-bounds!" &&
         outExtVal >= this->OutputWholeExtent[2 * dim] &&
         outExtVal <= this->OutputWholeExtent[2 * dim + 1]);
  int outIdx = outExtVal - this->OutputWholeExtent[2 * dim];
  return this->IndexMap->Mapping[dim][outIdx];
}

//-----------------------------------------------------------------------------
int vtkExtractStructuredGridHelper::GetMappedExtentValue(int dim, int outExtVal)
{
  // Sanity Checks
  assert("pre: dimension dim is out-of-bounds!" && dim >= 0 && dim < 3);
  assert("pre: extent value out-of-bounds!" &&
         outExtVal >= this->OutputWholeExtent[2 * dim] &&
         outExtVal <= this->OutputWholeExtent[2 * dim + 1]);
  int outIdx = outExtVal - this->OutputWholeExtent[2 * dim];
  return this->IndexMap->Mapping[dim][outIdx] + this->InputWholeExtent[2 * dim];
}

//-----------------------------------------------------------------------------
int vtkExtractStructuredGridHelper::GetMappedExtentValueFromIndex(int dim,
                                                                  int outIdx)
{
  // Sanity Checks
  assert("pre: dimension dim is out-of-bounds!" && dim >= 0 && dim < 3);
  assert("pre: point index out-of-bounds!" &&
         outIdx >= 0 && outIdx < this->GetSize(dim));
  return this->IndexMap->Mapping[dim][outIdx] + this->InputWholeExtent[2 * dim];
}

//-----------------------------------------------------------------------------
int vtkExtractStructuredGridHelper::GetSize(const int dim)
{
  assert("pre: dimension dim is out-of-bounds!" && (dim >= 0) && (dim < 3) );
  return( static_cast<int>( this->IndexMap->Mapping[ dim ].size() ) );
}

//-----------------------------------------------------------------------------
namespace
{
int roundToInt(double r)
{
  return r > 0.0 ? r + 0.5 : r - 0.5;
}
}

//-----------------------------------------------------------------------------
void vtkExtractStructuredGridHelper::ComputeBeginAndEnd(
        int inExt[6], int voi[6], int begin[3], int end[3])
{
  vtkBoundingBox inExtB(inExt[0],inExt[1],inExt[2],inExt[3],inExt[4],inExt[5]);
  vtkBoundingBox uExtB(voi[0],voi[1],voi[2],voi[3],voi[4],voi[5]);
  std::fill(begin,begin+3,0);
  std::fill(end,end+3,-1);

  int uExt[6];
  if( uExtB.IntersectBox(inExtB) )
    {

    for(int i=0; i < 6; ++i)
      {
      uExt[i] = static_cast<int>( roundToInt(uExtB.GetBound(i) ) );
      }

    // Find the first and last indices in the map that are
    // within data extents. These are the extents of the
    // output data.
    for(int dim=0; dim < 3; ++dim)
      {
      for(int idx=0; idx < this->GetSize(dim); ++idx)
        {
        int extVal = this->GetMappedExtentValueFromIndex(dim, idx);
        if (extVal >= uExt[2*dim] &&
            extVal <= uExt[2*dim+1] )
          {
          begin[dim] = idx;
          break;
          }
        } // END for all indices with

      for(int idx=this->GetSize(dim)-1; idx >= 0; --idx)
        {
        int extVal = this->GetMappedExtentValueFromIndex(dim, idx);
        if (extVal <= uExt[2*dim+1] &&
            extVal >= uExt[2*dim] )
          {
          end[dim] = idx;
          break;
          }
        } // END for all indices

      } // END for all dimensions

    } // END if box intersects
}

//-----------------------------------------------------------------------------
void vtkExtractStructuredGridHelper::CopyPointsAndPointData(
          int inExt[6], int outExt[6],
          vtkPointData* pd, vtkPoints* inpnts,
          vtkPointData* outPD, vtkPoints* outpnts,
          int sampleRate[3])
{
  assert("pre: NULL input point-data!" && (pd != NULL) );
  assert("pre: NULL output point-data!" && (outPD != NULL) );

  // short-circuit
  if( (pd->GetNumberOfArrays()==0) && (inpnts==NULL) )
    {
    // nothing to copy
    return;
    }

  // Get the size of the input and output
  vtkIdType inSize = vtkStructuredData::GetNumberOfPoints(inExt);
  vtkIdType outSize = vtkStructuredData::GetNumberOfPoints(outExt);
  (void)inSize; // Prevent warnings, this is only used in debug builds.

  // Check if we can use some optimizations:
  bool canCopyRange = sampleRate && I(sampleRate) == 1;
  bool useMapping = !(canCopyRange && J(sampleRate) == 1 && K(sampleRate) == 1);

  if( inpnts != NULL )
    {
    assert("pre: output points data-structure is NULL!" && (outpnts != NULL) );
    outpnts->SetDataType( inpnts->GetDataType() );
    outpnts->SetNumberOfPoints( outSize );
    }
  outPD->CopyAllocate(pd,outSize,outSize);

  // Lists for batching copy operations:
  vtkNew<vtkIdList> srcIds;
  vtkNew<vtkIdList> dstIds;
  if (!canCopyRange)
    {
    vtkIdType bufferSize = IMAX(outExt) - IMIN(outExt) + 1;
    srcIds->Allocate(bufferSize);
    dstIds->Allocate(bufferSize);
    }

  int ijk[3];
  int src_ijk[3];
  for( K(ijk)=KMIN(outExt); K(ijk) <= KMAX(outExt); ++K(ijk) )
    {
    K(src_ijk) = useMapping ? this->GetMappedExtentValue(2,K(ijk)) : K(ijk);

    for( J(ijk)=JMIN(outExt); J(ijk) <= JMAX(outExt); ++J(ijk) )
      {
      J(src_ijk) = useMapping ? this->GetMappedExtentValue(1,J(ijk)) : J(ijk);

      if (canCopyRange)
        {
        // Find the first point id:
        I(ijk) = IMIN(outExt);
        I(src_ijk) = I(ijk);

        vtkIdType srcStart =
            vtkStructuredData::ComputePointIdForExtent(inExt, src_ijk);
        vtkIdType dstStart =
            vtkStructuredData::ComputePointIdForExtent(outExt, ijk);
        vtkIdType num = IMAX(outExt) - IMIN(outExt) + 1;

          // Sanity checks
          assert( "pre: srcStart out of bounds" && (srcStart >= 0) &&
                  (srcStart < inSize) );
          assert( "pre: dstStart out of bounds" && (dstStart >= 0) &&
                  (dstStart < outSize) );

        if (inpnts != NULL)
          {
          outpnts->InsertPoints(dstStart, num, srcStart, inpnts);
          }
        outPD->CopyData(pd, dstStart, num, srcStart);
        }
      else // canCopyRange
        {
        for( I(ijk)=IMIN(outExt); I(ijk) <= IMAX(outExt); ++I(ijk) )
          {
          I(src_ijk) = useMapping ? this->GetMappedExtentValue(0,I(ijk))
                                  : I(ijk);

          vtkIdType srcIdx =
              vtkStructuredData::ComputePointIdForExtent(inExt,src_ijk);
          vtkIdType targetIdx =
              vtkStructuredData::ComputePointIdForExtent(outExt,ijk);

          // Sanity checks
          assert( "pre: srcIdx out of bounds" && (srcIdx >= 0) &&
                  (srcIdx < inSize) );
          assert( "pre: targetIdx out of bounds" && (targetIdx >= 0) &&
                  (targetIdx < outSize) );

          srcIds->InsertNextId(srcIdx);
          dstIds->InsertNextId(targetIdx);

          } // END for all i

        if( inpnts != NULL )
          {
          outpnts->InsertPoints(dstIds.GetPointer(), srcIds.GetPointer(), inpnts);
          } // END if
        outPD->CopyData(pd, srcIds.GetPointer(), dstIds.GetPointer());
        srcIds->Reset();
        dstIds->Reset();

        } // END else canCopyRange

      } // END for all j

    } // END for all k

}

//-----------------------------------------------------------------------------
void vtkExtractStructuredGridHelper::CopyCellData(int inExt[6], int outExt[6],
           vtkCellData* cd, vtkCellData* outCD,
           int sampleRate[3])
{
  assert("pre: NULL input cell-data!" && (cd != NULL) );
  assert("pre: NULL output cell-data!" && (outCD != NULL) );

  // short-circuit
  if( cd->GetNumberOfArrays()==0 )
    {
    // nothing to copy
    return;
    }

  // Get the size of the output & allocate output
  vtkIdType inSize = vtkStructuredData::GetNumberOfCells(inExt);
  vtkIdType outSize = vtkStructuredData::GetNumberOfCells(outExt);
  (void)inSize; // Prevent warnings, this is only used in debug builds.
  outCD->CopyAllocate(cd,outSize,outSize);

  // Check if we can use some optimizations:
  bool canCopyRange = sampleRate && I(sampleRate) == 1;
  bool useMapping = !(canCopyRange && J(sampleRate) == 1 && K(sampleRate) == 1);

  int inpCellExt[6];
  vtkStructuredData::GetCellExtentFromPointExtent(inExt,inpCellExt);

  int outCellExt[6];
  vtkStructuredData::GetCellExtentFromPointExtent(outExt,outCellExt);

  // Lists for batching copy operations:
  vtkNew<vtkIdList> srcIds;
  vtkNew<vtkIdList> dstIds;
  if (!canCopyRange)
    {
    vtkIdType bufferSize = IMAX(outCellExt) - IMIN(outCellExt) + 1;
    srcIds->Allocate(bufferSize);
    dstIds->Allocate(bufferSize);
    }

  int ijk[3];
  int src_ijk[3];
  for( K(ijk)=KMIN(outCellExt); K(ijk) <= KMAX(outCellExt); ++K(ijk) )
    {
    K(src_ijk) = useMapping ? this->GetMappedExtentValue(2, K(ijk)) : K(ijk);

    for( J(ijk)=JMIN(outCellExt); J(ijk) <= JMAX(outCellExt); ++J(ijk) )
      {
      J(src_ijk) = useMapping ? this->GetMappedExtentValue(1, J(ijk)) : J(ijk);

      if (canCopyRange)
        {
        // Find the first cell id:
        I(ijk) = IMIN(outCellExt);
        I(src_ijk) = I(ijk);

        // NOTE: since we are operating on cell extents, ComputePointID below
        // really returns the cell ID
        vtkIdType srcStart =
          vtkStructuredData::ComputePointIdForExtent(inpCellExt, src_ijk);
        vtkIdType dstStart =
          vtkStructuredData::ComputePointIdForExtent(outCellExt, ijk);
        vtkIdType num = IMAX(outCellExt) - IMIN(outCellExt) + 1;

        // Sanity checks
        assert( "pre: srcStart out of bounds" && (srcStart >= 0) &&
                (srcStart < inSize) );
        assert( "pre: dstStart out of bounds" && (dstStart >= 0) &&
                (dstStart < outSize) );

        outCD->CopyData(cd, dstStart, num, srcStart);
        }
      else // canCopyRange
        {
        for( I(ijk)=IMIN(outCellExt); I(ijk) <= IMAX(outCellExt); ++I(ijk) )
          {
          I(src_ijk) = useMapping ? this->GetMappedExtentValue(0, I(ijk))
                                  : I(ijk);

          // NOTE: since we are operating on cell extents, ComputePointID below
          // really returns the cell ID
          vtkIdType srcIdx =
              vtkStructuredData::ComputePointIdForExtent(inpCellExt, src_ijk);

          vtkIdType targetIdx =
              vtkStructuredData::ComputePointIdForExtent(outCellExt, ijk);

          srcIds->InsertNextId(srcIdx);
          dstIds->InsertNextId(targetIdx);
          } // END for all i

        outCD->CopyData(cd, srcIds.GetPointer(), dstIds.GetPointer());
        srcIds->Reset();
        dstIds->Reset();

        }// END else canCopyRange
      } // END for all j
    } // END for all k
}

//------------------------------------------------------------------------------
void vtkExtractStructuredGridHelper::GetPartitionedVOI(const int globalVOI[6],
  const int partitionedExtent[6], const int sampleRate[3], bool includeBoundary,
  int partitionedVOI[6])
{
  // 1D Example:
  //   InputWholeExtent = [0, 20]
  //   GlobalVOI = [3, 17]
  //   SampleRate = 2
  //   OutputWholeExtent = [0, 7]
  //   Processes = 2
  //
  // Process 0:
  //   PartitionedInputExtent = [0, 10]
  //   ClampedVOI = [3, 10]
  //   PartitionedVOI = [3, 9] (due to sampling)
  //
  // Process 1:
  //   PartitionedInputExtent = [10, 20]
  //   ClampedVOI = [10, 17]
  //   PartitionedVOI = [11, 17] (offset due to sampling)
  //
  // This method calculates the PartitionedVOI.


  // Start with filter's VOI (Ex: [3, 17] | [3, 17] )
  std::copy(globalVOI, globalVOI + 6, partitionedVOI);

  // Clamp to paritioned data (Ex: [3, 10] | [10, 17] )
  vtkStructuredExtent::Clamp(partitionedVOI, partitionedExtent);

  // Adjust for spacing: (Ex: [3, 9] | [11, 17] )
  for (int dim = 0; dim < 3; ++dim)
    {
    // Minimia:
    // Ex: 0 | 7
    int delta = EMIN(partitionedVOI, dim) - EMIN(globalVOI, dim);
    // Ex: 0 | 1
    delta %= sampleRate[dim];
    if (delta != 0)
      {
      delta = sampleRate[dim] - delta;
      }
    // Ex: 3 | 11
    EMIN(partitionedVOI, dim) += delta;

    if (includeBoundary && EMAX(partitionedVOI, dim) == EMAX(globalVOI, dim))
      {
      continue;
      }

    // Maxima:
    // Ex: 7 | 6
    delta = EMAX(partitionedVOI, dim) - EMIN(partitionedVOI, dim);
    // Ex: 1 | 0
    delta %= sampleRate[dim];
    EMAX(partitionedVOI, dim) -= delta;
    }
}

//------------------------------------------------------------------------------
void vtkExtractStructuredGridHelper::GetPartitionedOutputExtent(
    const int globalVOI[6], const int partitionedVOI[6],
    const int outputWholeExtent[6], const int sampleRate[3],
    bool includeBoundary, int partitionedOutputExtent[6])
{
  // 1D Example:
  //   InputWholeExtent = [0, 20]
  //   GlobalVOI = [3, 17]
  //   SampleRate = 2
  //   OutputWholeExtent = [0, 7]
  //   Processes = 2
  //
  // Process 0:
  //   PartitionedInputExtent = [0, 10]
  //   PartitionedVOI = [3, 9] (due to sampling)
  //   SerialOutputExtent = [0, 3]
  //   PartitionedOutputExtent = [0, 3]
  //
  // Process 1:
  //   PartitionedInputExtent = [10, 20]
  //   PartitionedVOI = [11, 17] (offset due to sampling)
  //   SerialOutputExtent = [0, 3]
  //   PartitionedOutputExtent = [4, 7]
  //
  // This method computes the PartitionedOutputExtent. The gap [3, 4] will be
  // cleaned up by the parallel filter using vtkStructuredImplicitConnectivity.
  for (int dim = 0; dim < 3; ++dim)
    {
    // Ex: 0 | 4
    EMIN(partitionedOutputExtent, dim) =
        (EMIN(partitionedVOI, dim) - EMIN(globalVOI, dim)) / sampleRate[dim];

    if (includeBoundary && EMAX(partitionedVOI, dim) == EMAX(globalVOI, dim))
      {
      int length = EMAX(partitionedVOI, dim) - EMIN(globalVOI, dim);
      EMAX(partitionedOutputExtent, dim) = length / sampleRate[dim];
      EMAX(partitionedOutputExtent, dim) +=
          ((length % sampleRate[dim]) == 0) ? 0 : 1;
      }
    else {
      // Ex: 3 | 7
      EMAX(partitionedOutputExtent, dim) =
          (EMAX(partitionedVOI, dim) - EMIN(globalVOI, dim)) / sampleRate[dim];
      }

    // Account for any offsets in the OutputWholeExtent:
    EMIN(partitionedOutputExtent, dim) += EMIN(outputWholeExtent, dim);
    EMAX(partitionedOutputExtent, dim) += EMIN(outputWholeExtent, dim);
    }
}
