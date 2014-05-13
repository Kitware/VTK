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
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStructuredData.h"
#include "vtkStructuredExtent.h"

// C/C++ includes
#include <cassert>
#include <vector>

// Some usefull extent macros
#define IMIN(ext) ext[0]
#define IMAX(ext) ext[1]
#define JMIN(ext) ext[2]
#define JMAX(ext) ext[3]
#define KMIN(ext) ext[4]
#define KMAX(ext) ext[5]

#define I(ijk) ijk[0]
#define J(ijk) ijk[1]
#define K(ijk) ijk[2]

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
  if( this->IndexMap != NULL )
    {
    delete this->IndexMap;
    }
}

//-----------------------------------------------------------------------------
void vtkExtractStructuredGridHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
void vtkExtractStructuredGridHelper::Invalidate()
{
  this->OutputWholeExtent[0]= 0;
  this->OutputWholeExtent[1]=-1;
  this->OutputWholeExtent[2]= 0;
  this->OutputWholeExtent[3]=-1;
  this->OutputWholeExtent[4]= 0;
  this->OutputWholeExtent[5]=-1;
}

//-----------------------------------------------------------------------------
void vtkExtractStructuredGridHelper::Initialize(
      int voi[6], int wholeExtent[6], int sampleRate[3], bool includeBoundary)
{
  assert("pre: NULL index map" && (this->IndexMap != NULL) );

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
    return;
    }

  // Clamp VOI to Whole Extent
  vtkStructuredExtent::Clamp(voi,wholeExtent);

  // Create mapping between output extent and input extent.
  // Compute the output whole extent in the process.
  for(int dim=0; dim < 3; ++dim)
    {

    this->IndexMap->Mapping[dim].resize(voi[2*dim+1]-voi[2*dim]+2);

    int idx,i;
    for(idx=0, i=voi[2*dim]; i <= voi[2*dim+1]; i+=sampleRate[dim])
      {
      this->IndexMap->Mapping[dim][idx++] = i;
      } // END for all points in this dimension, strided by the sample rate

    if(includeBoundary &&
       this->IndexMap->Mapping[dim][idx-1] != voi[2*dim+1])
      {
      this->IndexMap->Mapping[dim][idx++] = voi[2*dim+1];
      }
    this->IndexMap->Mapping[dim].resize(idx);

    // Update output whole extent
    this->OutputWholeExtent[2*dim]   = 0;
    this->OutputWholeExtent[2*dim+1] =
        static_cast<int>( this->IndexMap->Mapping[dim].size()-1 );
    } // END for all dimensions
}

//-----------------------------------------------------------------------------
int vtkExtractStructuredGridHelper::GetMapping(const int dim, const int i)
{
  // Sanity Checks
  assert( "pre: dimension dim is out-of-bounds!" && (dim >= 0) && (dim < 3) );
  assert( "pre: point index out-of-bounds!" &&
          (i >= 0) && (i < this->GetSize(dim) ) );
  return( this->IndexMap->Mapping[ dim ][ i ] );
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
        if( this->GetMapping(dim,idx) >= uExt[2*dim] &&
            this->GetMapping(dim,idx) <= uExt[2*dim+1] )
          {
          begin[dim] = idx;
          break;
          }
        } // END for all indices with

      for(int idx=this->GetSize(dim)-1; idx >= 0; --idx)
        {
        if( this->GetMapping(dim,idx) <= uExt[2*dim+1] &&
            this->GetMapping(dim,idx) >= uExt[2*dim] )
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
          bool useMapping)
{
  assert("pre: NULL input point-data!" && (pd != NULL) );
  assert("pre: NULL output point-data!" && (outPD != NULL) );

  // short-circuit
  if( (pd->GetNumberOfArrays()==0) && (inpnts==NULL) )
    {
    // nothing to copy
    return;
    }

  // Get input and output data description
  int inputDesc = vtkStructuredData::GetDataDescriptionFromExtent(inExt);
  int outDesc   = vtkStructuredData::GetDataDescriptionFromExtent(outExt);

  // Get the size of the output
  vtkIdType outSize = vtkStructuredData::GetNumberOfPoints(outExt,outDesc);

  if( inpnts != NULL )
    {
    assert("pre: output points data-structure is NULL!" && (outpnts != NULL) );
    outpnts->SetDataType( inpnts->GetDataType() );
    outpnts->SetNumberOfPoints( outSize );
    }
  outPD->CopyAllocate(pd,outSize,outSize);

  int ijk[3];
  int src_ijk[3];
  for( K(ijk)=KMIN(outExt); K(ijk) <= KMAX(outExt); ++K(ijk) )
    {
    K(src_ijk) = (useMapping)? this->GetMapping(2,K(ijk)) : K(ijk);

    for( J(ijk)=JMIN(outExt); J(ijk) <= JMAX(outExt); ++J(ijk) )
      {
      J(src_ijk) = (useMapping)? this->GetMapping(1,J(ijk)) : J(ijk);

      for( I(ijk)=IMIN(outExt); I(ijk) <= IMAX(outExt); ++I(ijk) )
        {
        I(src_ijk) = (useMapping)? this->GetMapping(0,I(ijk)) : I(ijk);

        vtkIdType srcIdx =
            vtkStructuredData::ComputePointIdForExtent(inExt,src_ijk,inputDesc);
        vtkIdType targetIdx =
            vtkStructuredData::ComputePointIdForExtent(outExt,ijk,outDesc);

        // Sanity checks
        assert( "pre: srcIdx out of bounds" && (srcIdx >= 0) &&
            (srcIdx < vtkStructuredData::GetNumberOfPoints(inExt,inputDesc)));
        assert( "pre: targetIdx out of bounds" && (targetIdx >= 0) &&
            (targetIdx < vtkStructuredData::GetNumberOfPoints(outExt,outDesc)));

        if( inpnts != NULL )
          {
          outpnts->SetPoint(targetIdx,inpnts->GetPoint(srcIdx));
          } // END if
        outPD->CopyData(pd,srcIdx,targetIdx);
        } // END for all i
      } // END for all j
    } // END for all k

}

//-----------------------------------------------------------------------------
void vtkExtractStructuredGridHelper::CopyCellData(
           int inExt[6], int outExt[6],
           vtkCellData* cd, vtkCellData* outCD,
           bool useMapping)
{
  assert("pre: NULL input cell-data!" && (cd != NULL) );
  assert("pre: NULL output cell-data!" && (outCD != NULL) );

  // short-circuit
  if( cd->GetNumberOfArrays()==0 )
    {
    // nothing to copy
    return;
    }

  // Get input and output data description
  int inputDesc = vtkStructuredData::GetDataDescriptionFromExtent(inExt);
  int outDesc   = vtkStructuredData::GetDataDescriptionFromExtent(outExt);

  // Get the size of the output & allocate output
  vtkIdType outSize = vtkStructuredData::GetNumberOfCells(outExt,outDesc);
  outCD->CopyAllocate(cd,outSize,outSize);

  int inpCellExt[6];
  vtkStructuredData::GetCellExtentFromPointExtent(inExt,inpCellExt,inputDesc);

  int outCellExt[6];
  vtkStructuredData::GetCellExtentFromPointExtent(outExt,outCellExt,outDesc);

  int ijk[3];
  int src_ijk[3];
  for( K(ijk)=KMIN(outCellExt); K(ijk) <= KMAX(outCellExt); ++K(ijk) )
    {
    K(src_ijk) = (useMapping)? this->GetMapping(2,K(ijk)) : K(ijk);

    for( J(ijk)=JMIN(outCellExt); J(ijk) <= JMAX(outCellExt); ++J(ijk) )
      {
      J(src_ijk) = (useMapping)? this->GetMapping(1,J(ijk)) : J(ijk);

      for( I(ijk)=IMIN(outCellExt); I(ijk) <= IMAX(outCellExt); ++I(ijk) )
        {
        I(src_ijk) = (useMapping)? this->GetMapping(0,I(ijk)) : I(ijk);

        // NOTE: since we are operating on cell extents, ComputePointID below
        // really returns the cell ID
        vtkIdType srcIdx =
          vtkStructuredData::ComputePointIdForExtent(
              inpCellExt,src_ijk,inputDesc);

        vtkIdType targetIdx =
          vtkStructuredData::ComputePointIdForExtent(outCellExt,ijk,outDesc);
        outCD->CopyData(cd,srcIdx,targetIdx);
        } // END for all i
      } // END for all j
    } // END for all k
}
