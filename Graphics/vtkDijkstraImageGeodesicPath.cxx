/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDijkstraImageGeodesicPath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDijkstraImageGeodesicPath.h"

#include "vtkCellArray.h"
#include "vtkExecutive.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

#include "vtkDijkstraGraphInternals.h"

vtkCxxRevisionMacro(vtkDijkstraImageGeodesicPath, "1.7");
vtkStandardNewMacro(vtkDijkstraImageGeodesicPath);

//----------------------------------------------------------------------------
vtkDijkstraImageGeodesicPath::vtkDijkstraImageGeodesicPath()
{
  this->PixelSize = 1.0;
  this->ImageWeight = 1.0;
  this->EdgeLengthWeight = 0.0;
  this->CurvatureWeight = 0.0;
}

//----------------------------------------------------------------------------
vtkDijkstraImageGeodesicPath::~vtkDijkstraImageGeodesicPath()
{
}

//----------------------------------------------------------------------------
void vtkDijkstraImageGeodesicPath::SetInput( vtkDataObject *input )
{
  vtkImageData* image = vtkImageData::SafeDownCast( input );
  if ( !image )
    {
    return;
    }

  image->UpdateInformation();
  int* dimensions = image->GetDimensions();
  int u[2];
  int n = 0;
  for ( int i = 0; i < 3; ++i )
    {
    if ( dimensions[i] != 1 ) 
      {
      u[n++] = i;
      }
    }
  if ( n != 2 )
    {
    vtkErrorMacro(<<"Input cost image must be 2D: input dimensions "
      <<dimensions[0]<<","<<dimensions[1]<<","<<dimensions[2]);
    return;
    }

  double* spacing = image->GetSpacing();
  this->PixelSize = sqrt(spacing[u[0]]*spacing[u[0]] + spacing[u[1]]*spacing[u[1]]);
  this->Superclass::SetInput( image );
}

//----------------------------------------------------------------------------
vtkImageData* vtkDijkstraImageGeodesicPath::GetInputAsImageData()
{
  if ( this->GetNumberOfInputConnections( 0 ) < 1 )
    {
    return NULL;
    }
  return vtkImageData::SafeDownCast(
    this->GetExecutive()->GetInputData( 0, 0 ));
}

//----------------------------------------------------------------------------
int vtkDijkstraImageGeodesicPath::FillInputPortInformation( int port,
  vtkInformation *info )
{
  if ( port == 0 )
    {
    info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData" );
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkDijkstraImageGeodesicPath::RequestData( vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector ) 
{
  vtkInformation* costInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo  = outputVector->GetInformationObject(0);

  vtkImageData* image = 
    vtkImageData::SafeDownCast( costInfo->Get(vtkDataObject::DATA_OBJECT()) );    
  
  if ( !image )
    {
    return 0;
    }

  vtkPolyData* output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if ( !output )
    {
    return 0;
    }

  if ( this->AdjacencyBuildTime.GetMTime() < image->GetMTime() )
    {
    this->Initialize( image );
    }
  else
    {
    this->Reset();
    }

  this->ShortestPath( image, this->StartVertex, this->EndVertex );
  this->TraceShortestPath( image, output, this->StartVertex, this->EndVertex ); 

  return 1;
}

//----------------------------------------------------------------------------
double vtkDijkstraImageGeodesicPath::CalculateStaticEdgeCost(
     vtkDataSet *inData, vtkIdType u, vtkIdType v)
{
  vtkImageData *image = vtkImageData::SafeDownCast(inData);

  double p1[3];
  image->GetPoint( u, p1 );
  double p2[3];
  image->GetPoint( v, p2 );

  double pcoords[3];
  int ijk1[3];
  int ijk2[3];
  image->ComputeStructuredCoordinates( p1, ijk1, pcoords );
  image->ComputeStructuredCoordinates( p2, ijk2, pcoords );

  double cost = this->ImageWeight*(
    image->GetScalarComponentAsDouble( ijk1[0], ijk1[1], ijk1[2], 0 ) +
    image->GetScalarComponentAsDouble( ijk1[0], ijk1[1], ijk1[2], 0 ) );

  if ( this->EdgeLengthWeight != 0.0 )
    {
    cost += this->EdgeLengthWeight*( sqrt( 
      vtkMath::Distance2BetweenPoints( p1, p2 ) )/this->PixelSize );
    }

  return cost;
}

//----------------------------------------------------------------------------
double vtkDijkstraImageGeodesicPath::CalculateDynamicEdgeCost(
     vtkDataSet *inData, vtkIdType u, vtkIdType v)
{
  double cost = 0.0;

  if ( this->CurvatureWeight != 0.0 )
    {
    int t = this->Internals->Predecessors[u];
    if( t != -1 )
      {
      vtkImageData *image = vtkImageData::SafeDownCast(inData);

      double p0[3];
      image->GetPoint( t, p0 );
      double p1[3];
      image->GetPoint( u, p1 );
      double p2[3];
      image->GetPoint( v, p2 );
   
      double p10[3] = { p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2] };
      double p21[3] = { p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2] };

      vtkMath::Normalize( p10 );
      vtkMath::Normalize( p21 );
    
      // the range of dot product of two unit vectors is [-1, 1] so normalize
      // the maximum curvature from 2 to 1
      cost = this->CurvatureWeight*( 0.5*fabs( vtkMath::Dot( p10, p21 ) - 1.0 ) );
      }
    }

  return cost;
}

//----------------------------------------------------------------------------
// This is probably a horribly inefficient way to do it.
void vtkDijkstraImageGeodesicPath::BuildAdjacency( vtkDataSet *inData )
{
  vtkImageData *image = vtkImageData::SafeDownCast(inData);

  int ncells = image->GetNumberOfCells();

  // optimized for cell type VTK_PIXEL
  //
  vtkIdList *ptIds = vtkIdList::New();
  vtkstd::pair<vtkstd::map<int,double>::iterator,bool> ret;

  vtkIdType uId[6] = {0,1,2,3,0,1};
  vtkIdType vId[6] = {1,2,3,0,2,3};
  float cost;
  vtkIdType u, v;

  for ( int i = 0; i < ncells; ++i )
    {    
    image->GetCellPoints ( i, ptIds );

    for( int j = 0; j < 6; ++j )
      {
      u = ptIds->GetId( vId[j] );
      v = ptIds->GetId( uId[j] );

      cost = this->CalculateStaticEdgeCost( image, u, v );
      ret = this->Internals->Adjacency[u].insert( vtkstd::pair<int,double>( v, cost ) );
      if ( !ret.second )
        {
        this->Internals->Adjacency[u][v] = cost;
        }
      
      cost = this->CalculateStaticEdgeCost( image, v, u );
      ret = this->Internals->Adjacency[u].insert( vtkstd::pair<int,double>( u, cost ) );
      if ( !ret.second )
        {
        this->Internals->Adjacency[v][u] = cost;
        }
      }
    }

  ptIds->Delete();

  this->AdjacencyBuildTime.Modified();
}

//----------------------------------------------------------------------------
void vtkDijkstraImageGeodesicPath::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ImageWeight: " << this->ImageWeight << endl;
  os << indent << "EdgeLengthWeight: " << this->EdgeLengthWeight << endl;
  os << indent << "CurvatureWeight: " << this->CurvatureWeight << endl;
}
