/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRGhostCellExtruder.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAMRGhostCellExtruder.h"
#include "vtkObjectFactory.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkAMRBox.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkIntArray.h"
#include "vtkUniformGrid.h"
#include "vtkIndent.h"
#include "vtkMultiProcessController.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

#include <cassert>

vtkStandardNewMacro(vtkAMRGhostCellExtruder);

vtkAMRGhostCellExtruder::vtkAMRGhostCellExtruder()
{
  this->Controller          = NULL;
  this->NumberOfGhostLayers = 1;
  this->SetNumberOfOutputPorts( 1 );
}

//------------------------------------------------------------------------------
vtkAMRGhostCellExtruder::~vtkAMRGhostCellExtruder()
{

}

//------------------------------------------------------------------------------
void vtkAMRGhostCellExtruder::PrintSelf( std::ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
int vtkAMRGhostCellExtruder::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(
   vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkHierarchicalBoxDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRGhostCellExtruder::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkHierarchicalBoxDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRGhostCellExtruder::RequestData(
    vtkInformation* vtkNotUsed(rqst), vtkInformationVector** inputVector,
    vtkInformationVector* outputVector )
{
  // STEP 0: Get input & output object
  vtkInformation *input = inputVector[0]->GetInformationObject( 0 );
  assert( "pre: input infomration is NULL" && (input != NULL) );

  vtkHierarchicalBoxDataSet *inputAMR =
   vtkHierarchicalBoxDataSet::SafeDownCast(
     input->Get(vtkDataObject::DATA_OBJECT()));
  assert( "pre: input AMR dataset is NULL" && (inputAMR != NULL) );

  vtkInformation *output = outputVector->GetInformationObject(0);
  vtkHierarchicalBoxDataSet *outputAMR =
    vtkHierarchicalBoxDataSet::SafeDownCast(
        output->Get(vtkDataObject::DATA_OBJECT() ) );
  assert( "pre: output AMR dataset is NULL" && (outputAMR != NULL) );

  // STEP 1: Construct output AMR dataset
  this->ConstructExtrudedDataSet( inputAMR, outputAMR );

  // STEP 2: Synchronize
  if( this->Controller != NULL )
    this->Controller->Barrier( );

  return 1;
}

//------------------------------------------------------------------------------
void vtkAMRGhostCellExtruder::ConstructExtrudedDataSet(
     vtkHierarchicalBoxDataSet *inAMR, vtkHierarchicalBoxDataSet *outAMR )
{
  assert( "pre: input AMR dataset is NULL" && (inAMR != NULL) );
  assert( "pre: output AMR dataset is NULL" && (outAMR != NULL ) );
  assert( "pre: number of exrtusion layers is less that 1" &&
            (this->NumberOfGhostLayers >= 1) );

  vtkUniformGrid *ugPtr     = NULL;
  unsigned int currentLevel = 0;
   for( ;currentLevel < inAMR->GetNumberOfLevels(); ++currentLevel )
     {
       unsigned int dataIdx = 0;
       for( ;dataIdx < inAMR->GetNumberOfDataSets(currentLevel); ++dataIdx )
         {

           vtkAMRBox myBox;
           inAMR->GetMetaData( currentLevel, dataIdx, myBox );
           vtkUniformGrid *myGrid = inAMR->GetDataSet(currentLevel, dataIdx );

           switch( currentLevel )
             {
               case 0:

                 if( myGrid != NULL )
                   {
                     ugPtr = this->CloneGrid( myGrid );
                     assert( "grid clone is NULL" && (ugPtr != NULL) );
                     outAMR->SetDataSet(currentLevel, dataIdx, ugPtr );
                     ugPtr->Delete();
                     ugPtr = NULL;
                   }
                 else
                   {
                     outAMR->SetDataSet(currentLevel,dataIdx,NULL);
                   }
                 outAMR->SetMetaData(currentLevel,dataIdx, myBox );
                 break;

               default:

                 if( myGrid != NULL )
                   {
                     ugPtr = this->GetExtrudedGrid( myGrid );
                     assert( "extruded grid is NULL" && (ugPtr != NULL) );
                     outAMR->SetDataSet(currentLevel,dataIdx,ugPtr);
                     ugPtr->Delete();
                     ugPtr= NULL;
                   }
                 else
                   {
                     outAMR->SetDataSet(currentLevel,dataIdx,NULL);
                   }
                 myBox.Grow( this->NumberOfGhostLayers );
                 outAMR->SetMetaData(currentLevel,dataIdx, myBox );
             }

         } // END for all data at the curent level

        outAMR->SetRefinementRatio(
           currentLevel, inAMR->GetRefinementRatio( currentLevel ) );

     } // END for all levels
   outAMR->GenerateVisibilityArrays();
}

//------------------------------------------------------------------------------
void vtkAMRGhostCellExtruder::AttachCellGhostInformation(
    vtkUniformGrid *extrudedGrid, int *realCellExtent)
{
  // Sanity Check
  assert( "pre: Input grid is NULL!" && (extrudedGrid != NULL) );
  assert( "pre: real extent array is NULL!" && (realCellExtent != NULL) );

  vtkIntArray *ghostArray = vtkIntArray::New();
  ghostArray->SetName( "GHOST" );
  ghostArray->SetNumberOfTuples( extrudedGrid->GetNumberOfCells() );
  ghostArray->SetNumberOfComponents( 1 );

  int celldims[3];
  extrudedGrid->GetDimensions(celldims);
  celldims[0]--; celldims[1]--; celldims[2]--;
  celldims[0] = (celldims[0] < 1)? 1 : celldims[0];
  celldims[1] = (celldims[1] < 1)? 1 : celldims[1];
  celldims[2] = (celldims[2] < 1)? 1 : celldims[2];

  for( int i=0; i < celldims[0]; ++i )
    {
      for( int j=0; j < celldims[1]; ++j )
        {
          for( int k=0; k < celldims[2]; ++k )
            {
              int ijk[3];
              ijk[0]=i; ijk[1]=j; ijk[2]=k;

              // Since celldims consists of the cell dimensions, ComputePointId
              // is sufficient to get the corresponding linear cell index!
              int cellIdx = vtkStructuredData::ComputePointId( celldims, ijk );

              assert(
               "Cell Index Out-of-range" &&
               (cellIdx >= 0) && (cellIdx < extrudedGrid->GetNumberOfCells()));

              if( (i >= realCellExtent[0]) && (i <= realCellExtent[1]) &&
                  (j >= realCellExtent[2]) && (j <= realCellExtent[3]) &&
                  (k >= realCellExtent[4]) && (k <= realCellExtent[5]) )
                  ghostArray->InsertValue( cellIdx, 1 );
              else
                  ghostArray->InsertValue( cellIdx, 0 );

            } // END for all k
        } // END for all j
    } // END for all i

  extrudedGrid->GetCellData()->AddArray( ghostArray );
  ghostArray->Delete();
}

//------------------------------------------------------------------------------
void vtkAMRGhostCellExtruder::CopyPointData(
    vtkUniformGrid *, vtkUniformGrid *, int * )
{
  // TODO: node-centered support is low priority at this point.
}

//------------------------------------------------------------------------------
void vtkAMRGhostCellExtruder::CopyCellData(
    vtkUniformGrid *src, vtkUniformGrid *t, int *re )
{
  // Sanity check
  assert( "pre: source grid is NULL" && (src != NULL) );
  assert( "pre: target grid is NULL" && (t != NULL) );
  assert( "pre: real extent is NULL" && (re != NULL) );
  assert( "pre: source cell-data is NULL" && (src->GetCellData() != NULL) );

  if( src->GetCellData()->GetNumberOfArrays() == 0)
    return;

  for( int array=0; array < src->GetCellData()->GetNumberOfArrays(); ++array )
    {
      vtkDataArray *arrayPtr = src->GetCellData()->GetArray( array );
      assert( "post: arrayPtr != NULL" && (arrayPtr != NULL) );


      vtkDataArray *newArray =
       vtkDataArray::CreateDataArray( arrayPtr->GetDataType() );
      newArray->SetName( arrayPtr->GetName() );
      newArray->SetNumberOfComponents( arrayPtr->GetNumberOfComponents() );
      newArray->SetNumberOfTuples(t->GetNumberOfCells() );

      // Loop through the real extent and copy the cell data from the source
      // grid to the target grid
      int tijk[3]; /* target grid ijk */
      int sijk[3]; /* source grid ijk */
      for( sijk[0]=0,tijk[0]=re[0]; tijk[0]<=re[1]; ++tijk[0],++sijk[0] )
      {
        for( sijk[1]=0,tijk[1]=re[2]; tijk[1]<=re[3]; ++tijk[1],++sijk[1] )
          {
            for( sijk[2]=0,tijk[2]=re[4]; tijk[2]<=re[5]; ++tijk[2],++sijk[2] )
              {
                // Get the source cell index w.r.t. the source grid
                int sIdx=
                vtkStructuredData::ComputeCellId( src->GetDimensions(), sijk);
                assert( "post: source cell index out-of-bounds!" &&
                         (sIdx >= 0) && (sIdx < src->GetNumberOfCells() ) );

                // Get the target cell index w.r.t. the target grid
                int tIdx=
                vtkStructuredData::ComputeCellId( t->GetDimensions(), tijk );
                assert( "post: target cell index out-of-bounds!" &&
                        (tIdx >= 0) && (tIdx < t->GetNumberOfCells() ) );

                int component = 0;
                for( ;component<newArray->GetNumberOfComponents(); ++component)
                  {
                      newArray->SetComponent(
                       tIdx,component,arrayPtr->GetComponent(sIdx, component));
                  } // END for all array components

              } // END for all k
          } // END for all j
      } // END for all i


      t->GetCellData()->AddArray( newArray );
      newArray->Delete();
    } // END for all cell arrays

}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkAMRGhostCellExtruder::GetExtrudedGrid(
    vtkUniformGrid* srcGrid )
{
  // Sanity check
  assert( "pre: SourceGrid != NULL" && (srcGrid != NULL) );
  vtkUniformGrid *extrudedGrid = vtkUniformGrid::New();

  int    realCellExtent[6];
  int    ndim[3];
  double origin[3];
  double h[3];

  // STEP 0: Initialize
  for( int i=0; i < 6; ++i )
    {
      realCellExtent[i] = ndim[i%3] = 0;
      origin[i%3]       = h[i%3]    = 0.0;
    }

  // STEP 1: Constructed extruded grid
  srcGrid->GetDimensions( ndim );
  srcGrid->GetOrigin( origin );
  srcGrid->GetSpacing( h );

  for( int i=0; i < srcGrid->GetDataDimension(); ++i )
    {
      ndim[i]              += 2*this->NumberOfGhostLayers;
      origin[ i ]          -= h[i]*this->NumberOfGhostLayers;
      realCellExtent[i*2]   = this->NumberOfGhostLayers;
      realCellExtent[i*2+1] = ndim[i] - 2*this->NumberOfGhostLayers - 1;
    }
  extrudedGrid->Initialize();
  extrudedGrid->SetDimensions( ndim );
  extrudedGrid->SetSpacing( h );
  extrudedGrid->SetOrigin( origin );

  // STEP 2: Compute ghost cell information
  this->AttachCellGhostInformation( extrudedGrid, realCellExtent );

  // STEP 3: Copy PointData
  this->CopyPointData( srcGrid, extrudedGrid, realCellExtent );

  // STEP 4: Copy CellData
  this->CopyCellData( srcGrid, extrudedGrid, realCellExtent );

  return( extrudedGrid );
}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkAMRGhostCellExtruder::CloneGrid( vtkUniformGrid *ug)
{
  // Sanity check
  assert( "pre: input grid is NULL" && (ug != NULL) );

  // STEP 0: Construct the topology
  vtkUniformGrid *cloneGrid = vtkUniformGrid::New();
  cloneGrid->Initialize();
  cloneGrid->SetOrigin( ug->GetOrigin() );
  cloneGrid->SetDimensions( ug->GetDimensions() );
  cloneGrid->SetSpacing( ug->GetSpacing() );

  // STEP 1: Copy point & cell data from the original grid
  cloneGrid->GetPointData()->DeepCopy( ug->GetPointData() );
  cloneGrid->GetCellData()->DeepCopy( ug->GetCellData() );

  int numCells = cloneGrid->GetNumberOfCells();

  // STEP 2: Attach ghost array information
  vtkIntArray *ghostArray = vtkIntArray::New();
  ghostArray->SetName( "GHOST" );
  ghostArray->SetNumberOfTuples( numCells );
  ghostArray->SetNumberOfComponents( 1 );
  ghostArray->FillComponent(0,1);
  cloneGrid->GetCellData()->AddArray( ghostArray );
  ghostArray->Delete();

  return( cloneGrid );
}
