/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellLocatorInterpolatedVelocityField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellLocatorInterpolatedVelocityField.h"

#include "vtkMath.h"
#include "vtkDataSet.h"
#include "vtkDataArray.h"
#include "vtkPointData.h"
#include "vtkGenericCell.h"
#include "vtkCellLocator.h"
#include "vtkSmartPointer.h"
#include "vtkObjectFactory.h"
#include "vtkModifiedBSPTree.h"

vtkStandardNewMacro ( vtkCellLocatorInterpolatedVelocityField );
vtkCxxSetObjectMacro( vtkCellLocatorInterpolatedVelocityField, CellLocatorPrototype, vtkAbstractCellLocator );

//----------------------------------------------------------------------------
typedef std::vector< vtkSmartPointer < vtkAbstractCellLocator > > CellLocatorsTypeBase;
class vtkCellLocatorInterpolatedVelocityFieldCellLocatorsType : public CellLocatorsTypeBase { };

//----------------------------------------------------------------------------
vtkCellLocatorInterpolatedVelocityField::vtkCellLocatorInterpolatedVelocityField()
{
  this->LastCellLocator  = 0;
  this->CellLocatorPrototype = 0;
  this->CellLocators = new vtkCellLocatorInterpolatedVelocityFieldCellLocatorsType;
}

//----------------------------------------------------------------------------
vtkCellLocatorInterpolatedVelocityField::~vtkCellLocatorInterpolatedVelocityField()
{
  this->LastCellLocator = 0;
  this->SetCellLocatorPrototype( 0 );

  delete this->CellLocators;
  this->CellLocators = 0;
}

//----------------------------------------------------------------------------
void vtkCellLocatorInterpolatedVelocityField::SetLastCellId
  ( vtkIdType c, int dataindex )
{
  this->LastCellId       = c;
  this->LastDataSet      = ( *this->DataSets )[dataindex];
  this->LastCellLocator  = ( *this->CellLocators )[dataindex].GetPointer();
  this->LastDataSetIndex = dataindex;

  // If the dataset changes, then the cached cell is invalidated. We might as
  // well prefetch the cached cell either way.
  if ( this->LastCellId != -1 )
  {
    this->LastDataSet->GetCell( this->LastCellId, this->GenCell );
  }
}

//----------------------------------------------------------------------------
int vtkCellLocatorInterpolatedVelocityField::FunctionValues
  ( double * x, double * f )
{
  vtkDataSet *             vds = NULL;
  vtkAbstractCellLocator * loc = NULL;

  if( !this->LastDataSet && !this->DataSets->empty() )
    {
    vds = ( *this->DataSets )[0];
    loc = ( *this->CellLocators )[0].GetPointer();
    this->LastDataSet      = vds;
    this->LastCellLocator  = loc;
    this->LastDataSetIndex = 0;
    }
  else
    {
    vds = this->LastDataSet;
    loc = this->LastCellLocator;
    }

  int retVal;
  if ( loc )
    {
    // resort to vtkAbstractCellLocator::FindCell()
    retVal = this->FunctionValues( vds, loc, x, f );
    }
  else
    {
    // turn to vtkImageData/vtkRectilinearGrid::FindCell()
    retVal = this->FunctionValues( vds, x, f );
    }

  if ( !retVal )
    {
    for( this->LastDataSetIndex = 0;
         this->LastDataSetIndex < static_cast<int>( this->DataSets->size() );
         this->LastDataSetIndex ++ )
      {
      vds = this->DataSets->operator[]( this->LastDataSetIndex );
      loc = this->CellLocators->operator[]( this->LastDataSetIndex ).GetPointer();
      if( vds && vds != this->LastDataSet )
        {
        this->ClearLastCellId();

        if ( loc )
          {
          // resort to vtkAbstractCellLocator::FindCell()
          retVal = this->FunctionValues( vds, loc, x, f );
          }
        else
          {
          // turn to vtkImageData/vtkRectilinearGrid::FindCell()
          retVal = this->FunctionValues( vds, x, f );
          }

        if ( retVal )
          {
          this->LastDataSet     = vds;
          this->LastCellLocator = loc;
          vds = NULL;
          loc = NULL;
          return retVal;
          }
        }
      }

    this->LastCellId       = -1;
    this->LastDataSet      = ( *this->DataSets )[0];
    this->LastCellLocator  = ( *this->CellLocators )[0].GetPointer();
    this->LastDataSetIndex = 0;
    vds = NULL;
    loc = NULL;
    return 0;
    }

  vds = NULL;
  loc = NULL;
  return retVal;
}

//----------------------------------------------------------------------------
int vtkCellLocatorInterpolatedVelocityField::FunctionValues
  ( vtkDataSet * dataset, vtkAbstractCellLocator * loc, double * x, double * f )
{
  f[0] = f[1] = f[2] = 0.0;
  vtkDataArray * vectors = NULL;

  if ( !dataset || !loc || !dataset->IsA( "vtkPointSet" ) ||
       !( vectors = dataset->GetPointData()
                           ->GetVectors( this->VectorsSelection )
        )
     )
    {
    vtkErrorMacro( <<"Can't evaluate dataset!" );
    vectors = NULL;
    return  0;
    }

  int    i;
  int    subIdx;
  int    numPts;
  int    pntIdx;
  int    bFound = 0;
  double vector[3];
  double dstns2 = 0.0;
  double toler2 = dataset->GetLength() *
                  vtkCellLocatorInterpolatedVelocityField::TOLERANCE_SCALE;

  // check if the point is in the cached cell AND can be successfully evaluated
  if ( this->LastCellId != -1 &&
       this->GenCell->EvaluatePosition
             ( x, 0, subIdx, this->LastPCoords, dstns2, this->Weights ) == 1
     )
    {
    bFound = 1;
    this->CacheHit ++;
    }

  if ( !bFound )
    {
    // cache missing or evaluation failure and then we have to find the cell
    this->CacheMiss += !(  !( this->LastCellId + 1 )  );
    this->LastCellId = loc->FindCell( x, toler2, this->GenCell,
                                      this->LastPCoords, this->Weights );
    bFound = !(  !( this->LastCellId + 1 )  );
    }

  // interpolate vectors if possible
  if ( bFound )
    {
    numPts = this->GenCell->GetNumberOfPoints();
    for ( i = 0; i < numPts; i ++ )
      {
      pntIdx = this->GenCell->PointIds->GetId( i );
      vectors->GetTuple( pntIdx, vector );
      f[0] += vector[0] * this->Weights[i];
      f[1] += vector[1] * this->Weights[i];
      f[2] += vector[2] * this->Weights[i];
      }

    if ( this->NormalizeVector == true )
      {
      vtkMath::Normalize( f );
      }
    }

  vectors = NULL;
  return  bFound;
}

//----------------------------------------------------------------------------
void vtkCellLocatorInterpolatedVelocityField::AddDataSet( vtkDataSet * dataset )
{
  if ( !dataset )
    {
    vtkErrorMacro( <<"Dataset NULL!" );
    return;
    }

  // insert the dataset (do NOT register the dataset to 'this')
  this->DataSets->push_back( dataset );

  // We need to attach a valid vtkAbstractCellLocator to any vtkPointSet for
  // robust cell location as vtkPointSet::FindCell() may incur failures. For
  // any non-vtkPointSet dataset, either vtkImageData or vtkRectilinearGrid,
  // we do not need to associate a vtkAbstractCellLocator with it (though a
  // NULL vtkAbstractCellLocator is still inserted to this->CellLocators to
  // enable proper access to those valid cell locators) since these two kinds
  // of datasets themselves are able to guarantee robust as well as fast cell
  // location via vtkImageData/vtkRectilinearGrid::FindCell().
  vtkSmartPointer< vtkAbstractCellLocator > locator = 0; // MUST inited with 0
  if (  dataset->IsA( "vtkPointSet" )  )
    {

    if ( !this->CellLocatorPrototype )
      {
      locator = vtkSmartPointer < vtkModifiedBSPTree >::New();
      }
    else
      {
      locator.TakeReference( this->CellLocatorPrototype->NewInstance() );
      }

    locator->SetLazyEvaluation( 1 );
    locator->SetDataSet( dataset );
    }
  this->CellLocators->push_back( locator );

  int  size = dataset->GetMaxCellSize();
  if ( size > this->WeightsSize )
    {
    this->WeightsSize = size;
    delete[] this->Weights;
    this->Weights = new double[size];
    }
}

//----------------------------------------------------------------------------
void vtkCellLocatorInterpolatedVelocityField::CopyParameters
  ( vtkAbstractInterpolatedVelocityField * from )
{
  vtkAbstractInterpolatedVelocityField::CopyParameters( from );

  if (  from->IsA( "vtkCellLocatorInterpolatedVelocityField" )  )
    {
    this->SetCellLocatorPrototype
          (  vtkCellLocatorInterpolatedVelocityField::SafeDownCast( from )
             ->GetCellLocatorPrototype()
          );
    }
}

//----------------------------------------------------------------------------
void vtkCellLocatorInterpolatedVelocityField::PrintSelf( ostream & os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "CellLocators: "     << this->CellLocators     << endl;
  if ( this->CellLocators )
    {
    os << indent << "Number of Cell Locators: " << this->CellLocators->size();
    }
  os << indent << "LastCellLocator: "      << this->LastCellLocator      << endl;
  os << indent << "CellLocatorPrototype: " << this->CellLocatorPrototype << endl;
}
