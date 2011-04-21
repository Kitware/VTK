/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRSliceFilter.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "vtkAMRSliceFilter.h"
#include "vtkObjectFactory.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkCell.h"
#include "vtkAMRUtilities.h"
#include "vtkPlane.h"
#include "vtkAMRBox.h"
#include "vtkUniformGrid.h"

#include <cassert>

vtkStandardNewMacro(vtkAMRSliceFilter);

vtkAMRSliceFilter::vtkAMRSliceFilter()
{
  this->SetNumberOfInputPorts( 1 );
  this->SetNumberOfOutputPorts( 1 );
  this->OffSetFromOrigin = 0.0;
  this->Normal           = 1;
  this->Controller       = NULL;
}

//------------------------------------------------------------------------------
vtkAMRSliceFilter::~vtkAMRSliceFilter()
{
  // TODO Auto-generated destructor stub
}

//------------------------------------------------------------------------------
void vtkAMRSliceFilter::PrintSelf( std::ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
int vtkAMRSliceFilter::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(
   vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkHierarchicalBoxDataSet" );
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRSliceFilter::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(
    vtkDataObject::DATA_TYPE_NAME(),"vtkHierarchicalBoxDataSet");
  return 1;
}

//------------------------------------------------------------------------------
bool vtkAMRSliceFilter::IsAMRData2D( vtkHierarchicalBoxDataSet *input )
{
  assert( "pre: Input AMR dataset is NULL" && (input != NULL)  );

  vtkAMRBox box;
  input->GetMetaData( 0, 0, box );

  if( box.GetDimensionality() == 2 )
    return true;

  return false;
}

//------------------------------------------------------------------------------
vtkPlane* vtkAMRSliceFilter::GetCutPlane( vtkHierarchicalBoxDataSet *inp )
{
  vtkPlane *pl = vtkPlane::New();

  double amrorigin[3];
  vtkAMRUtilities::ComputeDataSetOrigin( amrorigin, inp, this->Controller );

  double porigin[3];
  for( int i=0; i < 3; ++i )
    porigin[i]=amrorigin[i];

  switch( this->Normal )
    {
      case 1:
        // X-Normal
        pl->SetNormal(1.0,0.0,0.0);
        porigin[0] += this->OffSetFromOrigin;
        break;
      case 2:
        // Y-Normal
        pl->SetNormal(0.0,1.0,0.0);
        porigin[1] += this->OffSetFromOrigin;
        break;
      case 3:
        // Z-Normal
        pl->SetNormal(0.0,0.0,1.0);
        porigin[2] += this->OffSetFromOrigin;
        break;
      default:
        vtkErrorMacro( "Undefined plane normal" );
    }
  pl->SetOrigin( porigin );
  return( pl );

}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkAMRSliceFilter::GetSlice(
    double porigin[3], vtkUniformGrid *grid )
{
  assert( "pre: input grid is NULL" && (grid != NULL) );
  assert( "pre: input grid must be a 3-D grid"&&(grid->GetDataDimension()==3));

  vtkUniformGrid *slice = vtkUniformGrid::New();

  // Get the 3-D Grid dimensions
  int dims[3];
  grid->GetDimensions( dims );

  // Storage for dimensions of the 2-D slice grid & its origin
  int    sliceDims[3];
  double sliceOrigin[3];

  switch( this->Normal )
    {
    case 1:
      // X-Normal -- YZ plane
      sliceDims[0] = 1;
      sliceDims[1] = dims[1];
      sliceDims[2] = dims[2];

      sliceOrigin[0] = porigin[0];
      sliceOrigin[1] = grid->GetOrigin()[1];
      sliceOrigin[2] = grid->GetOrigin()[2];
      break;
    case 2:
      // Y-Normal -- XZ plane
      sliceDims[0] = dims[0];
      sliceDims[1] = 1;
      sliceDims[2] = dims[2];

      sliceOrigin[0] = grid->GetOrigin()[0];
      sliceOrigin[1] = porigin[1];
      sliceOrigin[2] = grid->GetOrigin()[2];
      break;
    case 3:
      // Z-Normal -- XY plane
      sliceDims[0] = dims[0];
      sliceDims[1] = dims[1];
      sliceDims[2] = 1;

      sliceOrigin[0] = grid->GetOrigin()[0];
      sliceOrigin[1] = grid->GetOrigin()[1];
      sliceOrigin[2] = porigin[2];
      break;
    default:
      vtkErrorMacro( "Undefined normal" );
    }

  slice->SetOrigin( sliceOrigin );
  slice->SetDimensions( sliceDims );
  slice->SetSpacing( grid->GetSpacing() );

  return( slice );
}

//------------------------------------------------------------------------------
bool vtkAMRSliceFilter::PlaneIntersectsAMRBox(double plane[4],double bounds[6])
{
  bool lowPnt  = false;
  bool highPnt = false;

  for( int i=0; i < 8; ++i )
    {
      // Get box coordinates
      double x = ( i&1 ? bounds[1] : bounds[0] );
      double y = ( i&2 ? bounds[3] : bounds[2] );
      double z = ( i&3 ? bounds[5] : bounds[4] );

      // Plug-in coordinates to the plane equation
      double v = plane[3] - plane[0]*x - plane[1]*y - plane[2]*z;

      if( v == 0.0 ) // Point is on a plane
        return true;

      if( v < 0.0 )
        lowPnt = true;
      else
        highPnt = true;

      if( lowPnt && highPnt )
        return true;
    }

  return false;
}

//------------------------------------------------------------------------------
void vtkAMRSliceFilter::GetAMRSliceInPlane(
    vtkPlane *p, vtkHierarchicalBoxDataSet *inp,
    vtkHierarchicalBoxDataSet *out )
{
  assert( "pre: input AMR dataset is NULL" && (inp != NULL) );
  assert( "pre: output AMR dataset is NULL" && (out != NULL) );
  assert( "pre: cut plane is NULL" && (p != NULL) );

  // Store A,B,C,D from the plane equation
  double plane[4];
  plane[0] = p->GetNormal()[0];
  plane[1] = p->GetNormal()[1];
  plane[2] = p->GetNormal()[2];
  plane[3] = p->GetNormal()[0]*p->GetOrigin()[0] +
             p->GetNormal()[1]*p->GetOrigin()[1] +
             p->GetNormal()[2]*p->GetOrigin()[2];

  // Storage for the AMR box bounds
  double bounds[6];

  unsigned int level=0;
  for( ; level < inp->GetNumberOfLevels(); ++level )
    {
      unsigned int dataIdx=0;
      for( ; dataIdx < inp->GetNumberOfDataSets(level); ++dataIdx )
        {
          vtkAMRBox box;
          inp->GetMetaData( level, dataIdx, box );

          vtkUniformGrid *grid = inp->GetDataSet( level, dataIdx );
          if( grid != NULL )
          {
            bounds[0] = box.GetMinX();
            bounds[1] = box.GetMaxX();
            bounds[2] = box.GetMinY();
            bounds[3] = box.GetMaxY();
            bounds[4] = box.GetMinZ();
            bounds[5] = box.GetMaxZ();

            if( this->PlaneIntersectsAMRBox( plane, bounds ) )
              {
                vtkUniformGrid *slice = this->GetSlice( p->GetOrigin(), grid );
                assert( "Dimension of slice must be 2-D" &&
                         (slice->GetDataDimension()==2) );
                assert( "2-D slice is NULL" && (slice != NULL) );
                unsigned int blockIdx =
                    out->GetNumberOfDataSets( box.GetLevel() );
                out->SetDataSet( box.GetLevel(), blockIdx, slice );
                slice->Delete();
              }

          }

        } // END for all data
    } // END for all levels


  vtkAMRUtilities::GenerateMetaData( out, this->Controller );
  out->GenerateVisibilityArrays();
}

//------------------------------------------------------------------------------
void vtkAMRSliceFilter::ComputeCellCenter(
    vtkUniformGrid *ug, const int cellIdx, double centroid[3] )
{
    assert( "pre: Input grid is NULL" && (ug != NULL) );
    assert( "pre: cell index out-of-bounds!" &&
            ( (cellIdx >= 0) && (cellIdx < ug->GetNumberOfCells() ) ) );

    vtkCell *myCell = ug->GetCell( cellIdx );
    assert( "post: cell is NULL" && (myCell != NULL) );

    double pCenter[3];
    double *weights = new double[ myCell->GetNumberOfPoints() ];
    int subId       = myCell->GetParametricCenter( pCenter );
    myCell->EvaluateLocation( subId, pCenter, centroid, weights );
    delete [] weights;
    myCell->Delete();
}

//------------------------------------------------------------------------------
void vtkAMRSliceFilter::GetSliceCellData(
    vtkUniformGrid *slice, vtkUniformGrid *grid3D )
{
  assert( "pre: AMR slice grid is NULL" && (slice != NULL) );
  assert( "pre: 3-D AMR slice grid is NULL" && (grid3D != NULL) );

  // STEP 1: Allocate data-structures
//  vtkCellData *sourceCD = grid3D->GetCellData();
//  assert( "pre: source cell data is NULL" && (sourceCD != NULL) );
//
//  vtkCellData *targetCD = slice->GetCellData();
//  assert( "pre: target cell data is NULL" && (targetCD != NULL) );
//
//  int numCells = slice->GetNumberOfCells();
//  for( int arrayIdx=0; arrayIdx < sourceCD->GetNumberOfArrays(); ++arrayIdx )
//    {
//      vtkDataArray *array = NULL;
//      switch( sourceCD->GetArray( arrayIdx )->GetDataType() )
//        {
//          case VTK_DOUBLE:
//            array = vtkDoubleArray::New();
//            break;
//          case VTK_INT:
//            array = vtkIntArray::New();
//            break;
//          default:
//            vtkErrorMacro( "Cannot handle datatype!" );
//        }
//      assert( "pre: slice grid data array is NULL!" && (array != NULL) );
//
//      array->SetNumberOfTuples( numCells );
//      array->SetNumberOfComponents(
//       sourceCD->GetArray(arrayIdx)->GetNumberOfComponents() );
//    } // END for all arrays

  // STEP 2: Fill in slice data-arrays
//  int dims[3];
//  slice->GetDimensions( dims );
//  --dims[0]; --dims[1]; --dims[2];
//   dims[0] = (dims[0] < 1)? 1 : dims[0];
//   dims[1] = (dims[1] < 1)? 1 : dims[1];
//   dims[2] = (dims[2] < 1)? 1 : dims[2];
//
//   int ijk[3];
//   for( ijk[0]=0; ijk[0] <= dims[0]; ++ijk[0] )
//     {
//       for( ijk[1]=0; ijk[1] <= dims[1]; ++ijk[1] )
//         {
//           for( ijk[2]=0; ijk[2] <= dims[2]; ++ijk[2] )
//             {
//
//               // Note: The dimensions here are cells based, so ComputePointId
//               // will return linear index of the cell
//               int cellIdx = vtkStructuredData::ComputePointId( dims, ijk );
//
//               double probePnt[3];
//               this->ComputeCellCenter( slice, cellIdx, probePnt );
//
//               double weights[8];
//               double pcoords[3];
//               int subId;
//               vtkIdType sourceCellIdx =
//                   grid3D->FindCell(
//                       probePnt,NULL, 0, 1.e-9,subId,pcoords,weights);
//
//               if( sourceCellIdx != -1 )
//                 {
//
//                   int arrayIdx = 0;
//                   for(;arrayIdx < sourceCD->GetNumberOfArrays(); ++arrayIdx )
//                     {
//
//                       vtkDataArray *sourceArray = sourceCD->GetArray(arrayIdx);
//                       assert( "pre: src array is NULL" &&
//                               (sourceArray != NULL) );
//
//                       const char *name = sourceArray->GetName();
//                       assert( "pre: target cell data must have array" &&
//                                (targetCD->HasArray( name ) ) );
//                       vtkDataArray *targetArray = targetCD->GetArray( name );
//                       assert( "pre: target array is NULL" &&
//                               (targetArray != NULL ) );
//                       assert( "pre: numcom mismatch" &&
//                               (sourceArray->GetNumberOfComponents()==
//                                targetArray->GetNumberOfComponents() ) );
//
//                       int ncomp     = sourceArray->GetNumberOfComponents();
//                       int component = 0;
//                       for( ; component < ncomp; ++component )
//                         {
//                           targetArray->SetComponent(
//                               cellIdx,component,
//                                 sourceArray->GetComponent(
//                                     sourceCellIdx, component ) );
//                         } // END for all components
//
//                     } // END for all arrays
//
//
//                 } // If source cell is found
//
//             } // END for all k
//         } // END for all j
//     } // END for all i

}

//------------------------------------------------------------------------------
int vtkAMRSliceFilter::RequestData(
    vtkInformation* vtkNotUsed(request), vtkInformationVector** inputVector,
    vtkInformationVector* outputVector )
{
  // STEP 0: Get input object
  vtkInformation *input = inputVector[0]->GetInformationObject( 0 );
  assert( "pre: input information object is NULL" && (input != NULL) );
  vtkHierarchicalBoxDataSet *inputAMR=
      vtkHierarchicalBoxDataSet::SafeDownCast(
          input->Get(vtkDataObject::DATA_OBJECT() ) );

  // STEP 1: Get output object
  vtkInformation *output = outputVector->GetInformationObject( 0 );
  assert( "pre: output information object is NULL" && (output != NULL) );
  vtkHierarchicalBoxDataSet *outputAMR=
      vtkHierarchicalBoxDataSet::SafeDownCast(
        output->Get( vtkDataObject::DATA_OBJECT() ) );

  if( this->IsAMRData2D( inputAMR ) )
    {
      outputAMR->ShallowCopy( inputAMR );
      return 1;
    }

  // STEP 2: Compute global origin
  vtkPlane *cutPlane = this->GetCutPlane( inputAMR );
  assert( "Cut plane is NULL" && (cutPlane != NULL) );

  // STEP 3: Get the AMR slice
  this->GetAMRSliceInPlane( cutPlane, inputAMR, outputAMR );
  cutPlane->Delete();

  return 1;
}
