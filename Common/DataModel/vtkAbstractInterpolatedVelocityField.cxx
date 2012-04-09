/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractInterpolatedVelocityField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAbstractInterpolatedVelocityField.h"

#include "vtkMath.h"
#include "vtkDataSet.h"
#include "vtkDataArray.h"
#include "vtkPointData.h"
#include "vtkGenericCell.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
const double vtkAbstractInterpolatedVelocityField::TOLERANCE_SCALE = 1.0E-8;

//---------------------------------------------------------------------------
vtkAbstractInterpolatedVelocityField::vtkAbstractInterpolatedVelocityField()
{
  this->NumFuncs     = 3; // u, v, w
  this->NumIndepVars = 4; // x, y, z, t
  this->Weights      = 0;
  this->WeightsSize  = 0;
 
  this->Caching    = true; // Caching on by default
  this->CacheHit   = 0;
  this->CacheMiss  = 0;
  
  this->LastCellId = -1;
  this->LastDataSet= 0;
  this->LastDataSetIndex = 0;
  this->LastPCoords[0] = 0.0;
  this->LastPCoords[1] = 0.0;
  this->LastPCoords[2] = 0.0;
  
  this->VectorsSelection = 0;
  this->NormalizeVector  = false;

  this->Cell     = vtkGenericCell::New();
  this->GenCell  = vtkGenericCell::New();
  this->DataSets = new vtkAbstractInterpolatedVelocityFieldDataSetsType;
}

//---------------------------------------------------------------------------
vtkAbstractInterpolatedVelocityField::~vtkAbstractInterpolatedVelocityField()
{
  this->NumFuncs     = 0;
  this->NumIndepVars = 0;
  
  this->LastDataSet  = 0;
  this->SetVectorsSelection( 0 );
  
  if ( this->Weights )
    {
    delete[] this->Weights;
    this->Weights = 0;
    }
  
  if ( this->Cell )
    {
    this->Cell->Delete();
    this->Cell = NULL;
    }
  
  if ( this->GenCell )
    {
    this->GenCell->Delete();
    this->GenCell = NULL;
    }
  
  if ( this->DataSets )
    {    
    delete this->DataSets;
    this->DataSets = NULL;
    }
}

//---------------------------------------------------------------------------
int vtkAbstractInterpolatedVelocityField::FunctionValues
  ( vtkDataSet * dataset, double * x, double * f )
{
  int i, j, subId , numPts, id;
  vtkDataArray * vectors = NULL;
  double vec[3];
  double dist2;
  int ret;
  
  f[0] = f[1] = f[2] = 0.0;

  // See if a dataset has been specified and if there are input vectors
  if ( !dataset || 
       !(  vectors = 
           dataset->GetPointData()->GetVectors( this->VectorsSelection )
        )
     )
    {
    vtkErrorMacro( << "Can't evaluate dataset!" );
    vectors = NULL;
    return 0;
    }

  double tol2 = dataset->GetLength() * 
                vtkAbstractInterpolatedVelocityField::TOLERANCE_SCALE;

  int found = 0;

  if ( this->Caching )
    {
    // See if the point is in the cached cell
    if ( this->LastCellId == -1 || 
         !(  ret = this->GenCell->EvaluatePosition
                   ( x, 0, subId, this->LastPCoords, dist2, this->Weights)
          )
        || ret == -1
       )
      {
      // if not, find and get it
      if ( this->LastCellId != - 1 )
        {
        this->CacheMiss ++;

        dataset->GetCell( this->LastCellId, this->Cell );
        
        this->LastCellId = 
          dataset->FindCell( x, this->Cell, this->GenCell, this->LastCellId, 
                             tol2, subId, this->LastPCoords, this->Weights );
                             
        if ( this->LastCellId != -1 )
          {
          dataset->GetCell( this->LastCellId, this->GenCell );
          found = 1;
          }
        }
      }
    else
      {
      this->CacheHit ++;
      found = 1;
      }
    }

  if ( !found )
    {
    // if the cell is not found, do a global search (ignore initial
    // cell if there is one)
    this->LastCellId = 
      dataset->FindCell( x, 0, this->GenCell, -1, tol2, 
                         subId, this->LastPCoords, this->Weights );
                         
    if ( this->LastCellId != -1 )
      {
      dataset->GetCell( this->LastCellId, this->GenCell );
      }
    else
      {
      vectors = NULL;
      return  0;
      }
    }
                                
  // if the cell is valid
  if ( this->LastCellId >= 0 )
    {
    numPts = this->GenCell->GetNumberOfPoints();
    
    // interpolate the vectors
    for ( j = 0; j < numPts; j ++ )
      {
      id = this->GenCell->PointIds->GetId( j );
      vectors->GetTuple( id, vec );
      for ( i = 0; i < 3; i ++ )
        {
        f[i] +=  vec[i] * this->Weights[j];
        }
      }
      
    if ( this->NormalizeVector == true )
      {
      vtkMath::Normalize( f );
      }  
    }
  // if not, return false
  else
    {
    vectors = NULL;
    return  0;
    }

  vectors = NULL;
  return  1;
}

//----------------------------------------------------------------------------
int vtkAbstractInterpolatedVelocityField::GetLastWeights( double * w )
{
  if ( this->LastCellId < 0 )
    {
    return 0;
    }
  
  int   numPts = this->GenCell->GetNumberOfPoints();
  for ( int i = 0; i < numPts; i ++ )
    {
    w[i] = this->Weights[i];
    }
    
  return 1;
}

//----------------------------------------------------------------------------
int vtkAbstractInterpolatedVelocityField::GetLastLocalCoordinates( double pcoords[3] )
{
  if ( this->LastCellId < 0 )
    {
    return 0;
    }
    
  pcoords[0] = this->LastPCoords[0];
  pcoords[1] = this->LastPCoords[1];
  pcoords[2] = this->LastPCoords[2];
    
  return 1;
}

//----------------------------------------------------------------------------
void vtkAbstractInterpolatedVelocityField::FastCompute
  ( vtkDataArray * vectors, double f[3] )
{
  int    pntIdx;
  int    numPts = this->GenCell->GetNumberOfPoints();
  double vector[3];
  f[0] = f[1] = f[2] = 0.0;
  
  for ( int i = 0; i < numPts; i ++ )
    {
    pntIdx = this->GenCell->PointIds->GetId( i );
    vectors->GetTuple( pntIdx, vector );
    f[0] += vector[0] * this->Weights[i];
    f[1] += vector[1] * this->Weights[i];
    f[2] += vector[2] * this->Weights[i];
    }
}

//----------------------------------------------------------------------------
bool vtkAbstractInterpolatedVelocityField::InterpolatePoint
  ( vtkPointData * outPD, vtkIdType outIndex )
{
  if ( !this->LastDataSet )
    {
    return 0;
    }
    
  outPD->InterpolatePoint( this->LastDataSet->GetPointData(), outIndex, 
                           this->GenCell->PointIds, this->Weights );
  return 1;
}

//----------------------------------------------------------------------------
void vtkAbstractInterpolatedVelocityField::PrintSelf( ostream & os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  
  os << indent << "VectorsSelection: " 
     << ( this->VectorsSelection ? this->VectorsSelection : "(none)" ) << endl;
  os << indent << "NormalizeVector: "
     << ( this->NormalizeVector ? "on." : "off." ) << endl;
     
  os << indent << "Caching Status: "     << ( this->Caching ? "on." : "off." )
     << endl;
  os << indent << "Cache Hit: "          << this->CacheHit         << endl;
  os << indent << "Cache Miss: "         << this->CacheMiss        << endl;
  os << indent << "Weights Size: "       << this->WeightsSize      << endl;
  
  os << indent << "DataSets: "           << this->DataSets         << endl;
  os << indent << "Last Dataset Index: " << this->LastDataSetIndex << endl;
  os << indent << "Last Dataset: "       << this->LastDataSet      << endl;
  
  os << indent << "Last Cell Id: "       << this->LastCellId       << endl;
  os << indent << "Last Cell: "          << this->Cell             << endl;
  os << indent << "Current Cell: "       << this->GenCell          << endl;
  os << indent << "Last P-Coords: "      << this->LastPCoords[0]
               << ", "                   << this->LastPCoords[1]
               << ", "                   << this->LastPCoords[2]   << endl;
  os << indent << "Last Weights: "       << this->Weights          << endl;
}
