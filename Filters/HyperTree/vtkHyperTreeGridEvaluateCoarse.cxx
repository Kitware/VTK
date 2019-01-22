/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridEvaluateCoarse.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridEvaluateCoarse.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkBitArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkPointData.h"
#include "vtkNew.h"

#include "vtkUniformHyperTreeGrid.h"

#include "vtkHyperTreeGridNonOrientedCursor.h"

#include <cmath>

vtkStandardNewMacro(vtkHyperTreeGridEvaluateCoarse);

//-----------------------------------------------------------------------------
vtkHyperTreeGridEvaluateCoarse::vtkHyperTreeGridEvaluateCoarse()
{
  this->Operator = vtkHyperTreeGridEvaluateCoarse::OPERATOR_DON_T_CHANGE;
  this->Mask = 0;

  this->Default = 0.;

  this->BranchFactor = 0;
  this->Dimension = 0;
  this->SplattingFactor = 1;

  // JB Pour sortir un maillage de meme type que celui en entree
  this->AppropriateOutput = true;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridEvaluateCoarse::~vtkHyperTreeGridEvaluateCoarse()
{
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridEvaluateCoarse::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridEvaluateCoarse::FillOutputPortInformation( int, vtkInformation* info)
{
  info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid" );
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridEvaluateCoarse::ProcessTrees( vtkHyperTreeGrid* input,
                                                  vtkDataObject* outputDO )
{
  // Downcast output data object to hyper tree grid
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast( outputDO );
  if ( ! output )
  {
    vtkErrorMacro( "Incorrect type of output: "
                   << outputDO->GetClassName() );
    return 0;
  }

  output->ShallowCopy( input );

  if( this->Operator == vtkHyperTreeGridEvaluateCoarse::OPERATOR_DON_T_CHANGE_FAST )
  {
    return 1;
  }

  this->Mask = output->HasMask() ? output->GetMask() : 0;

  this->BranchFactor = output->GetBranchFactor();
  this->Dimension = output->GetDimension();
  this->SplattingFactor = std::pow( this->BranchFactor, this->Dimension - 1 );
  this->NumberOfChildren = output->GetNumberOfChildren();

  this->NbChilds = input->GetNumberOfChildren();
  this->InData = input->GetPointData();
  this->OutData = output->GetPointData();
  this->OutData->CopyAllocate( this->InData );
  int nbArray = this->InData->GetNumberOfArrays();
  this->Arrays.clear();
  for( int i = 0; i < nbArray; ++ i )
  {
    vtkDataArray *arr = this->OutData->GetArray( i );
    // Just for quantities with one component
    // (what choice for a vector for evaluation max, min...?)
    if( arr->GetNumberOfComponents() == 1 )
    {
      this->Arrays.push_back( arr );
    }
  }
  // Iterate over all input and output hyper trees
  vtkIdType index;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator in;
  output->InitializeTreeIterator( in );
  vtkNew<vtkHyperTreeGridNonOrientedCursor> outCursor;
  while ( in.GetNextTree( index ) )
  {
    // Initialize new cursor at root of current output tree
    output->InitializeNonOrientedCursor( outCursor, index );
    // Recursively
    this->ProcessNode( outCursor );
    // Clean up
  }
  this->UpdateProgress( 1. );
  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridEvaluateCoarse::ProcessNode( vtkHyperTreeGridNonOrientedCursor *outCursor ) {
  vtkIdType id = outCursor->GetGlobalNodeIndex( );
  if ( outCursor->IsLeaf() )
  {
    this->OutData->CopyData( this->InData, id, id );
    return;
  }
  //Si pas d'operation
  if ( this->Operator == vtkHyperTreeGridEvaluateCoarse::OPERATOR_DON_T_CHANGE )
  {
    this->OutData->CopyData( this->InData, id, id );
    //Coarse
    for ( int ichild = 0; ichild < this->NbChilds; ++ ichild )
    {
      outCursor->ToChild( ichild );
      //On parcourt les filles
      ProcessNode( outCursor );
      outCursor->ToParent();
    }
    return;
  }
  //
  std::vector< std::vector<double> > values( this->Arrays.size() );
  //Coarse
  for ( int ichild = 0; ichild < this->NbChilds; ++ ichild )
  {
    outCursor->ToChild( ichild );
    //On parcourt les filles
    ProcessNode( outCursor );
    //On stocke les valeurs de la fille
    vtkIdType idChild = outCursor->GetGlobalNodeIndex( );
    int i = 0;
    for( std::vector<vtkDataArray *>::iterator it = this->Arrays.begin();
         it != this->Arrays.end();
         ++ it, ++ i )
    {
      if ( ! this->Mask || ! this->Mask->GetTuple1( idChild ) )
      {
        values[ i ].push_back( (*it)->GetTuple1( idChild ) );
      }
    }
    outCursor->ToParent();
  }
  //Operation de reduction
  int i = 0;
  for( std::vector<vtkDataArray *>::iterator it = this->Arrays.begin();
       it != this->Arrays.end();
       ++ it, ++ i )
  {
    (*it)->SetTuple1( id, EvalCoarse( values[i] ) );
    values[i].clear();
  }
}

//----------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::EvalCoarse( const std::vector<double>& array )
{
  switch( this->Operator )
  {
    case vtkHyperTreeGridEvaluateCoarse::OPERATOR_ELDER_CHILD:
    {
      return  this->ElderChild( array );
    }
    case vtkHyperTreeGridEvaluateCoarse::OPERATOR_MIN:
    {
      return  this->Min( array );
    }
    case vtkHyperTreeGridEvaluateCoarse::OPERATOR_MAX:
    {
      return  this->Max( array );
    }
    case vtkHyperTreeGridEvaluateCoarse::OPERATOR_SUM:
    {
      return  this->Sum( array );
    }
    case vtkHyperTreeGridEvaluateCoarse::OPERATOR_AVERAGE:
    {
      return  this->Average( array );
    }
    case vtkHyperTreeGridEvaluateCoarse::OPERATOR_MATERIAL_AVERAGE:
    {
      return  this->MaterialAverage( array );
    }
    case vtkHyperTreeGridEvaluateCoarse::OPERATOR_SPLATTING_AVERAGE:
    {
      return this->SplattingAverage( array );
    }
    default:
    {
      break;
    }
  }
  return NAN;
}

//----------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::Min( const std::vector<double>& array )
{
  if ( array.size() == 0 )
  {
    return NAN;
  }
  double val = array[0];
  for( std::vector<double>::const_iterator it = array.begin() + 1;
       it != array.end();
       ++ it )
  {
    if( *it < val )
    {
      val = *it;
    }
  }
  return val;
}

//----------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::Max( const std::vector<double>& array )
{
  if ( array.size() == 0 )
  {
    return NAN;
  }
  double val = array[0];
  for( std::vector<double>::const_iterator it = array.begin() + 1;
       it != array.end();
       ++ it )
  {
    if( *it > val )
    {
      val = *it;
    }
  }
  return val;
}

//----------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::Sum( const std::vector<double>& array )
{
  double val = array[0];
  for( std::vector<double>::const_iterator it = array.begin() + 1;
       it != array.end();
       ++ it )
  {
    val += *it;
  }
  return val;
}

//----------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::Average( const std::vector<double>& array )
{
  if ( array.size() == 0 )
  {
    return this->Default;
  }
  double sum = Sum( array );
  if ( this->Default != 0. )
  {
     sum += this->Default * ( this->NumberOfChildren - array.size() );
  }
  return sum / this->NumberOfChildren;
}

//----------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::MaterialAverage( const std::vector<double>& array )
{
  if ( array.size() == 0 )
  {
    return NAN;
  }
  return Sum( array ) / array.size();
}

//----------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::ElderChild( const std::vector<double>& array )
{
  if ( array.size() == 0 )
  {
    return NAN;
  }
  return array[0];
}

//----------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::SplattingAverage( const std::vector<double>& array )
{
  if ( array.size() == 0 )
  {
    return this->Default;
  }
  double sum = Sum( array );
  if ( this->Default != 0. )
  {
     sum += this->Default * ( this->NumberOfChildren - array.size() );
  }
  return sum / this->SplattingFactor;
}
