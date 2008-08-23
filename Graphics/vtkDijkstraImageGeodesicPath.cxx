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

vtkCxxRevisionMacro(vtkDijkstraImageGeodesicPath, "1.1");
vtkStandardNewMacro(vtkDijkstraImageGeodesicPath);

//----------------------------------------------------------------------------
vtkDijkstraImageGeodesicPath::vtkDijkstraImageGeodesicPath()
{
  this->UseEdgeLengthsAsWeights = 0;
  this->PixelSize = 1.0;

  this->SetNumberOfInputPorts( 2 );
}

//----------------------------------------------------------------------------
vtkDijkstraImageGeodesicPath::~vtkDijkstraImageGeodesicPath()
{
}

//----------------------------------------------------------------------------
void vtkDijkstraImageGeodesicPath::SetCostImage( vtkImageData *image )
{
  bool dimension2D = false;
  image->UpdateInformation();
  int* dimensions = image->GetDimensions();
  int u[2];
  int n = 0;
  for ( int i = 0; i < 3; ++i )
    {
    if ( dimensions[i] == 1 ) 
      {
      dimension2D = true; 
      break;
      }
    else u[n++] = i;
    }
  if ( !dimension2D )
    {
    vtkErrorMacro(<<"Input cost image must be 2D: input dimensions "
      <<dimensions[0]<<","<<dimensions[1]<<","<<dimensions[2]);
    return;
    }

  double* spacing = image->GetSpacing();
  this->PixelSize = sqrt(spacing[u[0]]*spacing[u[0]] + spacing[u[1]]*spacing[u[1]]);
  this->SetInput( 1, image );
}

//----------------------------------------------------------------------------
vtkImageData* vtkDijkstraImageGeodesicPath::GetCostImage()
{
  if ( this->GetNumberOfInputConnections( 1 ) < 1 )
    {
    return NULL;
    }
  return vtkImageData::SafeDownCast(
    this->GetExecutive()->GetInputData( 1, 0 ));
}

//----------------------------------------------------------------------------
int vtkDijkstraImageGeodesicPath::FillInputPortInformation( int port,
  vtkInformation *info )
{
  if ( port == 0 )
    {
    info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData" );
    return 1;
    }
  else if ( port == 1 )
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
  vtkInformation* inInfo   = inputVector[0]->GetInformationObject(0);
  vtkPolyData* poly = 
    vtkPolyData::SafeDownCast( inInfo->Get(vtkDataObject::DATA_OBJECT()) );
  
  if ( !poly ) 
    {
    return 0;
    }

  vtkInformation* costInfo = inputVector[1]->GetInformationObject(0);
  vtkImageData* image = 
    vtkImageData::SafeDownCast( costInfo->Get(vtkDataObject::DATA_OBJECT()) );    
  
  if ( !image ) 
    {
    return 0;
    }

  vtkInformation* outInfo  = outputVector->GetInformationObject(0);
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
// The edge cost function should be implemented as a callback function to
// allow more advanced weighting
double vtkDijkstraImageGeodesicPath::CalculateEdgeCost(
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

  double cost = image->GetScalarComponentAsDouble( ijk1[0], ijk1[1], ijk1[2], 0 );
  cost += image->GetScalarComponentAsDouble( ijk1[0], ijk1[1], ijk1[2], 0 );

  if ( this->UseEdgeLengthsAsWeights )
    {
    cost *= sqrt( vtkMath::Distance2BetweenPoints( p1, p2 ) )/this->PixelSize;
    }

  return cost;
}

//----------------------------------------------------------------------------
// This is probably a horribly inefficient way to do it.
void vtkDijkstraImageGeodesicPath::BuildAdjacency( vtkDataSet *inData )
{
  vtkImageData *image = vtkImageData::SafeDownCast(inData);

  int i;
  
  int npoints = image->GetNumberOfPoints();
  int ncells = image->GetNumberOfCells();
  
  this->DeleteAdjacency();
  
  this->Adjacency = new vtkIdList*[npoints];

  // Remember size, so it can be deleted again
  this->AdjacencyGraphSize = npoints;

  for ( i = 0; i < npoints; ++i )
    {
    this->Adjacency[i] = vtkIdList::New();
    }

  // optimized for cell type VTK_PIXEL
  //
  vtkIdList *ptIds = vtkIdList::New();
  for ( i = 0; i < ncells; ++i )
    {    
    image->GetCellPoints ( i, ptIds );

    vtkIdType u = ptIds->GetId( 0 );
    vtkIdType v = ptIds->GetId( 3 );

    this->Adjacency[u]->InsertUniqueId( v );
    this->Adjacency[v]->InsertUniqueId( u );
    for ( int j = 0; j < 3; ++j )
      {
      vtkIdType u1 = ptIds->GetId( j );
      vtkIdType v1 = ptIds->GetId( j+1 );
      this->Adjacency[u1]->InsertUniqueId( v1 );
      this->Adjacency[v1]->InsertUniqueId( u1 );
      }    
    }

  ptIds->Delete();  

  this->AdjacencyBuildTime.Modified();
}

//----------------------------------------------------------------------------
void vtkDijkstraImageGeodesicPath::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "UseEdgeLengthsAsWeights: ";
  if (this->UseEdgeLengthsAsWeights)
    {
    os << "On\n";
    }
  else
    {
    os << "Off\n";
    }
}
