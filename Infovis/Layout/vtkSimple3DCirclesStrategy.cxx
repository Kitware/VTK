/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimple3DCirclesStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSimple3DCirclesStrategy.h"

#include "vtkAbstractArray.h"
#include "vtkCharArray.h"            // For temporary store for point ordering
#include "vtkDirectedGraph.h"        // For this->Graph type check
#include "vtkIdTypeArray.h"          // For Ordered array
#include "vtkInEdgeIterator.h"       // For in edge(s) checks
#include "vtkIntArray.h"             // For hierarchy layers
#include "vtkMath.h"                 // For cross, outer, norm and dot
#include "vtkObjectFactory.h"        // For VTK ::New() function
#include "vtkOutEdgeIterator.h"      // For out edge(s) checks
#include "vtkPoints.h"               // For output target
#include "vtkSmartPointer.h"         // For good memory handling

#include <cmath>                     // For abs, sin, cos and tan

#include <algorithm>          // For min, max, swap, etc.
#include <list>               // For internal store

template <class T> bool IsZero( T value )
{
  return ( ( value < VTK_DBL_EPSILON ) && ( value > ( -1.0 * VTK_DBL_EPSILON ) ) );
};

class vtkSimple3DCirclesStrategyInternal
{
public:
  vtkSimple3DCirclesStrategyInternal( void )
  {
  };
  vtkSimple3DCirclesStrategyInternal( const vtkSimple3DCirclesStrategyInternal &from )
  {
    if ( &from != this )
      this->store = from.store;
  };
  vtkSimple3DCirclesStrategyInternal & operator = ( const vtkSimple3DCirclesStrategyInternal &from )
  {
    if ( &from != this )
      this->store = from.store;
    return *this;
  };
  vtkSimple3DCirclesStrategyInternal & operator = ( const std::list<vtkIdType> &from )
  {
    this->store = from;
    return *this;
  };
  vtkIdType front( void )
  {
    return this->store.front();
  };
  void pop_front( void )
  {
    this->store.pop_front();
  };
  std::size_t size( void )
  {
    return this->store.size();
  };
  void push_back( const vtkIdType &value )
  {
    this->store.push_back( value );
  };
  ~vtkSimple3DCirclesStrategyInternal( void )
  {
    this->store.clear();
  };
private:
  std::list<vtkIdType> store;
};

vtkStandardNewMacro(vtkSimple3DCirclesStrategy);

void vtkSimple3DCirclesStrategy::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "Radius : " << this->Radius << endl;

  os << indent << "Height : " << this->Height << endl;

  os << indent << "Origin  : (" << this->Origin[0] << "," << this->Origin[1] << "," << this->Origin[2] << ")" << endl;

  os << indent << "Direction  : (" << this->Direction[0] << "," << this->Direction[1] << "," << this->Direction[2] << ")" << endl;

  os << indent << "Rotate matrix : [[" << this->T[0][0] << ";" << this->T[1][0] << ";" << this->T[2][0] << "]";
  os << "[" << this->T[0][1] << ";" << this->T[1][1] << ";" << this->T[2][1] << "]";
  os << "[" << this->T[0][2] << ";" << this->T[1][2] << ";" << this->T[2][2] << "]]" << endl;

  os << indent << "Method : ";
  if ( this->Method == FixedRadiusMethod )
    os << "fixed radius method" << endl;
  else if ( this->Method == FixedDistanceMethod )
    os << "fixed distance method" << endl;

  os << indent << "MarkValue : " << this->MarkedValue << endl;

  os << indent << "Auto height : ";
  if ( this->AutoHeight == 1 )
    os << "On" << endl;
  else
    os << "Off" << endl;

  os << indent << "Minimum degree for autoheight : " << this->MinimumRadian << " rad [" << vtkMath::DegreesFromRadians( this->MinimumRadian ) << " deg]" << endl;

  os << indent << "Registered MarkedStartPoints :";
  if ( this->MarkedStartVertices == 0 )
    os << " (none)" << endl;
  else
  {
    os << endl;
    this->MarkedStartVertices->PrintSelf( os, indent.GetNextIndent() );
  }

  os << indent << "Registered HierarchicalLayers :";
  if ( this->HierarchicalLayers == 0 )
    os << " (none)" << endl;
  else
  {
    os << endl;
    this->HierarchicalLayers->PrintSelf( os, indent.GetNextIndent() );
  }

  os << indent << "Registered HierarchicalOrder :";
  if ( this->HierarchicalOrder == 0 )
    os << " (none)" << endl;
  else
  {
    os << endl;
    this->HierarchicalOrder->PrintSelf( os, indent.GetNextIndent() );
  }

  os << indent << "ForceToUseUniversalStartPointsFinder :"
     << this->ForceToUseUniversalStartPointsFinder << endl;
}

void vtkSimple3DCirclesStrategy::SetDirection( double dx, double dy, double dz )
{
  vtkDebugMacro( << this->GetClassName() << " (" << this << "): setting Direction to (" << dx << "," << dy << "," << dz << ")" );

  if ( ( this->Direction[0] != dx ) || ( this->Direction[1] != dy ) || ( this->Direction[2] != dz ) )
  {
    double global[3], local[3];
    global[0] = dx;
    global[1] = dy;
    global[2] = dz;

    local[0] = 0.0;
    local[1] = 1.0;
    local[2] = 0.0;

    double length_global = vtkMath::Norm( global );

    if ( IsZero( length_global ) )
    {
      vtkWarningMacro( << "The length of direction vector is zero! Direction has not been changed!" );
      return;
    }

    double cosfi, n[3], E[3][3], U[3][3], u[3][3], number;

    global[0] = global[0] / length_global;
    global[1] = global[1] / length_global;
    global[2] = global[2] / length_global;

    // http://en.citizendium.org/wiki/Rotation_matrix
    // we are going from local to global.
    // cos(fi) = local.global -> cosfi, because |local|=1 and |global|=1
    cosfi = vtkMath::Dot( local, global );
    // if fi == 2*Pi -> cosfi = -1
    if ( IsZero( cosfi + 1.0 ) )
    {
      // if "local" is on "z" axes
      if ( IsZero( local[2] + 1.0 ) || IsZero( local[2] - 1.0 ) )
      {
        this->T[0][0] = this->T[2][2] = -1.0;
        this->T[1][1] = 1.0;
        this->T[0][1] = this->T[1][0] = this->T[0][2] = this->T[2][0] = this->T[1][2] = this->T[2][1] = 0.0;
      }
      // if local != ( (0,0,1) or (0,0,-1) )
      else
      {
        // n vector
        n[0] = 1.0 / (1.0 - local[2]*local[2] ) * local[1];
        n[1] = -1.0 / (1.0 - local[2]*local[2] ) * local[0];
        n[2] = 0.0;
        // u = n X n
        vtkMath::Outer( n, n, u );

        // -E
        E[0][0] = E[1][1] = E[2][2] = -1.0;
        E[0][1] = E[1][0] = E[0][2] = E[2][0] = E[1][2] = E[2][1] = 0.0;

        // T = -E + 2*u
        int i,j;
        for ( i = 0; i < 3; ++i )
          for ( j = 0; j < 3; ++j )
            this->T[i][j] = E[i][j] + ( u[i][j] * 2.0 );

      }
    }
    // fi < 2*Pi
    else
    {
      // n = local x global -> n(nx,ny,nz)
      vtkMath::Cross( local, global, n );
      //
      // cos(fi)*E
      //
      E[0][0] = E[1][1] = E[2][2] = cosfi;
      E[0][1] = E[1][0] = E[0][2] = E[2][0] = E[1][2] = E[2][1] = 0.0;
      //                 |  0  -nz  ny |
      // U = sin(fi)*N = |  nz  0  -nx |
      //                 | -ny  nx  0  |
      U[0][0] = U[1][1] = U[2][2] = 0.0;
      U[0][1] = -1.0 * n[2]; U[1][0] =        n[2];
      U[0][2] =        n[1]; U[2][0] = -1.0 * n[1];
      U[1][2] = -1.0 * n[0]; U[2][1] =        n[0];

      // u = n X n
      vtkMath::Outer( n, n, u );

      int i,j;
      // T = cos(fi)*E + U + 1/(1+cos(fi))*[n X n]
      // [ number = 1/(1+cos(fi)) ]
      number = 1.0 / ( 1.0 + cosfi );
      for ( i = 0; i < 3; ++i )
        for ( j = 0; j < 3; ++j )
          this->T[i][j] = E[i][j] + U[i][j] + ( u[i][j] * number );
    }


    this->Direction[0] = dx;
    this->Direction[1] = dy;
    this->Direction[2] = dz;

    vtkDebugMacro( << "Transformation matrix : [[" << this->T[0][0] << "," << this->T[1][0] << "," << this->T[2][0] << "][" << this->T[0][1] << "," << this->T[1][1] << "," << this->T[2][1] << "][" << this->T[0][2] << "," << this->T[1][2] << "," << this->T[2][2] << "]]" );

    this->Modified();
  }
}

void vtkSimple3DCirclesStrategy::SetDirection( double d[3] )
{
  this->SetDirection( d[0], d[1], d[2] );
}

vtkCxxSetObjectMacro(vtkSimple3DCirclesStrategy,MarkedStartVertices,vtkAbstractArray);

void vtkSimple3DCirclesStrategy::SetMarkedValue( vtkVariant val )
{
  if ( !this->MarkedValue.IsEqual(val) )
  {
    this->MarkedValue = val;
    vtkDebugMacro( << "Setting MarkedValue : " << this->MarkedValue );
    this->Modified();
  }
}

vtkVariant vtkSimple3DCirclesStrategy::GetMarkedValue( void )
{
  return this->MarkedValue;
}

void vtkSimple3DCirclesStrategy::SetMinimumDegree( double degree )
{
  this->SetMinimumRadian( vtkMath::RadiansFromDegrees( degree ) );
}

double vtkSimple3DCirclesStrategy::GetMinimumDegree( void )
{
  return vtkMath::DegreesFromRadians( this->GetMinimumRadian() );
}

vtkCxxSetObjectMacro(vtkSimple3DCirclesStrategy,HierarchicalLayers,vtkIntArray);
vtkCxxSetObjectMacro(vtkSimple3DCirclesStrategy,HierarchicalOrder,vtkIdTypeArray);

vtkSimple3DCirclesStrategy::vtkSimple3DCirclesStrategy( void )
: Radius(1), Height(1), Method(FixedRadiusMethod), MarkedStartVertices(0), ForceToUseUniversalStartPointsFinder(0), AutoHeight(0), MinimumRadian(vtkMath::Pi()/6.0), HierarchicalLayers(0), HierarchicalOrder(0)
{
  this->Direction[0] = this->Direction[1] = 0.0; this->Direction[2] = 1.0;
  this->T[0][1] = this->T[0][2] = this->T[1][2] = 0.0;
  this->T[1][0] = this->T[2][0] = this->T[2][1] = 0.0;
  this->T[0][0] = this->T[1][1] = this->T[2][2] = 1.0;
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
}

vtkSimple3DCirclesStrategy::~vtkSimple3DCirclesStrategy( void )
{
  this->SetMarkedStartVertices(0);
  this->SetHierarchicalLayers(0);
  this->SetHierarchicalOrder(0);
}

void vtkSimple3DCirclesStrategy::Layout( void )
{
  if ( this->Graph == 0 )
  {
    vtkErrorMacro( << "Graph is null!" );
    return;
  }
  if ( this->Graph->GetNumberOfVertices() == 0 )
  {
    vtkDebugMacro( << "Graph is empty (no no vertices)!" );
    return;
  }

  vtkSmartPointer<vtkDirectedGraph> target = vtkSmartPointer<vtkDirectedGraph>::New();
  if ( ! target->CheckedShallowCopy( this->Graph ) )
  {
    vtkErrorMacro( << "Graph must be directed graph!" );
    return;
  }

  vtkSimple3DCirclesStrategyInternal start_points, order_points, stand_alones;

  // Layers begin
  vtkSmartPointer<vtkIntArray> layers = 0;
  if ( this->HierarchicalLayers != 0 )
  {
    if ( ( this->HierarchicalLayers->GetMaxId() + 1 ) == target->GetNumberOfVertices() )
    {
        layers = this->HierarchicalLayers;
    }
  }
  if ( layers == 0 )
  {
    layers = vtkSmartPointer<vtkIntArray>::New();
    if ( this->HierarchicalLayers != 0 )
      this->HierarchicalLayers->UnRegister(this);
    this->HierarchicalLayers = layers;
    this->HierarchicalLayers->Register(this);

    layers->SetNumberOfValues( target->GetNumberOfVertices() );
    for ( vtkIdType i = 0; i <= layers->GetMaxId(); ++i )
      layers->SetValue(i,-1);

    if ( this->UniversalStartPoints( target, &start_points, &stand_alones, layers ) == -1 )
    {
      vtkErrorMacro( << "There is no start point!" );
      return;
    }
    order_points = start_points;
    this->BuildLayers( target, &start_points, layers );
  }
  else
  {
    for ( vtkIdType i = 0; i <= layers->GetMaxId(); ++i )
    {
      if ( layers->GetValue(i) == 0 )
        order_points.push_back(i);
      else if ( layers->GetValue(i) == -2 )
        stand_alones.push_back(i);
    }
  }
  // Layers end

  // Order begin
  vtkSmartPointer<vtkIdTypeArray> order = 0;
  if ( this->HierarchicalOrder != 0 )
  {
    if ( ( this->HierarchicalOrder->GetMaxId() + 1 ) == target->GetNumberOfVertices() )
    {
        order = this->HierarchicalOrder;
    }
  }

  if ( order == 0 )
  {
    order = vtkSmartPointer<vtkIdTypeArray>::New();
    if ( this->HierarchicalOrder != 0 )
      this->HierarchicalOrder->UnRegister(this);
    this->HierarchicalOrder = order;
    this->HierarchicalOrder->Register(this);
    order->SetNumberOfValues( target->GetNumberOfVertices() );
    for ( vtkIdType i = 0; i <= order->GetMaxId(); ++i )
      order->SetValue(i,-1);

    this->BuildPointOrder( target, &order_points, &stand_alones, layers, order );
  }
  // Order end

  if ( order->GetValue( order->GetMaxId() ) == -1 )
  {
    vtkErrorMacro( << "Not all parts of the graph is accessible. There may be a loop." );
    return;
  }

  int index = 0;
  int layer = 0;
  int start = 0;
  double R = this->Radius;
  double Rprev = 0.0;
  double localXYZ[3], globalXYZ[3], localHeight = this->Height;
  double alfa = 0.0;
  double tangent = tan( vtkMath::Pi() / 2 - this->MinimumRadian );
  int ind = 0;

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetNumberOfPoints( target->GetNumberOfVertices() );

  while ( index <= order->GetMaxId() )
  {
    start = index;
    R = this->Radius;
    layer = layers->GetValue( order->GetValue(index) );
    while ( index <= order->GetMaxId() )
    {
      if ( layers->GetValue( order->GetValue(index) ) == layer )
        ++index;
      else
        break;
    }

    alfa = 2.0 * vtkMath::Pi() / double( index - start );

    if ( this->Method == FixedDistanceMethod )
    {
      R = double( index - start - 1 ) * this->Radius / vtkMath::Pi();
    }
    else if ( this->Method == FixedRadiusMethod )
    {
      if ( ( index - start ) == 1 )
        R = 0.0;
    }
    else
    {
      vtkErrorMacro( << "Method must be FixedRadiusMethod or FixedDistanceMethod!" );
      return;
    }

    if ( ( this->AutoHeight == 1 ) && ( this->Method == FixedDistanceMethod ) )
    {
      if ( fabs( tangent * ( R - Rprev ) ) > this->Height )
        localHeight = fabs( tangent * ( R - Rprev ) );
      else
        localHeight = this->Height;
    }

    if ( layer != 0 )
      localXYZ[2] = localXYZ[2] + localHeight;
    else
      localXYZ[2] = 0.0;

    for ( ind = start; ind < index; ++ind )
    {
      localXYZ[0] = R * cos( double(ind - start) * alfa );
      localXYZ[1] = R * sin( double(ind - start) * alfa );
      this->Transform( localXYZ, globalXYZ );
      points->SetPoint( order->GetValue(ind), globalXYZ );
    }

    Rprev = R;
  }

  this->Graph->SetPoints( points );
  vtkDebugMacro( << "vtkPoints is added to the graph. Vertex layout is ready." );

  return;
}

void vtkSimple3DCirclesStrategy::SetGraph( vtkGraph * graph )
{
  if ( this->Graph != graph )
  {
    this->Superclass::SetGraph( graph );
    if ( this->HierarchicalLayers != 0 )
    {
      this->HierarchicalLayers->UnRegister(this);
      this->HierarchicalLayers = 0;
    }
    if ( this->HierarchicalOrder != 0 )
    {
      this->HierarchicalOrder->UnRegister(this);
      this->HierarchicalOrder = 0;
    }
  }
}

int vtkSimple3DCirclesStrategy::UniversalStartPoints( vtkDirectedGraph * input, vtkSimple3DCirclesStrategyInternal *target, vtkSimple3DCirclesStrategyInternal *StandAlones, vtkIntArray * layers )
{
  if ( ( this->MarkedStartVertices != 0 ) && ( this->ForceToUseUniversalStartPointsFinder == 0 ) )
  {
    if ( this->MarkedStartVertices->GetMaxId() == layers->GetMaxId() )
    {
      for ( vtkIdType i = 0; i < input->GetNumberOfVertices(); ++i )
      {
        if ( ( input->GetInDegree(i) == 0 ) && ( input->GetOutDegree(i) > 0 ) )
        {
          target->push_back(i);
          layers->SetValue( i, 0 );
        }
        else if ( ( input->GetInDegree(i) == 0 ) && ( input->GetOutDegree(i) == 0 ) )
        {
          layers->SetValue( i, -2 );
          StandAlones->push_back(i);
        }
        else if ( ( this->MarkedStartVertices->GetVariantValue(i) == this->MarkedValue ) && ( input->GetOutDegree(i) > 0 ) )
        {
          target->push_back(i);
          layers->SetValue( i, 0 );
        }
      }

      vtkDebugMacro( << "StartPoint finder: Universal start point finder was used. Number of start point(s): " << target->size() << "; Number of stand alone point(s): " << StandAlones->size() );
      return static_cast<int>(target->size());
    }
    else
    {
      vtkErrorMacro( << "MarkedStartPoints number is NOT equal number of vertices!" );
      return -1;
    }
  }

  for ( vtkIdType i = 0; i < input->GetNumberOfVertices(); ++i )
  {
    if ( ( input->GetInDegree(i) == 0 ) && ( input->GetOutDegree(i) > 0 ) )
    {
      target->push_back(i);
      layers->SetValue( i, 0 );
    }
    else if ( ( input->GetInDegree(i) == 0 ) && ( input->GetOutDegree(i) == 0 ) )
    {
      layers->SetValue( i, -2 );
      StandAlones->push_back(i);
    }
  }

  vtkDebugMacro( << "StartPoint finder: Universal start point finder was used. Number of start points: " << target->size() << "; Number of stand alone point(s): " << StandAlones->size() );
  return static_cast<int>(target->size());
}

int vtkSimple3DCirclesStrategy::BuildLayers( vtkDirectedGraph * input, vtkSimple3DCirclesStrategyInternal *source, vtkIntArray * layers )
{
  vtkSmartPointer<vtkOutEdgeIterator> edge_out_iterator = vtkSmartPointer<vtkOutEdgeIterator>::New();
  vtkSmartPointer<vtkInEdgeIterator> edge_in_iterator = vtkSmartPointer<vtkInEdgeIterator>::New();
  int layer = 0, flayer = 0;
  vtkInEdgeType in_edge;
  vtkOutEdgeType out_edge;
  bool HasAllInput = true;
  vtkIdType ID = 0;
  int max_layer_id = -1;

  while ( source->size() > 0 )
  {
    ID = source->front();
    source->pop_front();

    input->GetOutEdges( ID, edge_out_iterator );

    while ( edge_out_iterator->HasNext() )
    {
      out_edge = edge_out_iterator->Next();
      if ( layers->GetValue( out_edge.Target ) == -1 )
      {
        input->GetInEdges( out_edge.Target, edge_in_iterator );
        layer = layers->GetValue( ID );
        HasAllInput = true;

        while ( edge_in_iterator->HasNext() && HasAllInput )
        {
          in_edge = edge_in_iterator->Next();
          flayer = layers->GetValue( in_edge.Source );
          if ( flayer == -1 )
            HasAllInput = false;
          layer = std::max( layer, flayer );
        }

        if ( HasAllInput )
        {
          source->push_back( out_edge.Target );
          layers->SetValue( out_edge.Target, layer + 1 );
          max_layer_id = std::max( max_layer_id, layer + 1 );
        }
      }
    }
  }

  vtkDebugMacro( << "Layer building is successful." );
  return max_layer_id;
}

void vtkSimple3DCirclesStrategy::BuildPointOrder( vtkDirectedGraph * input, vtkSimple3DCirclesStrategyInternal *source, vtkSimple3DCirclesStrategyInternal *StandAlones, vtkIntArray * layers, vtkIdTypeArray * order )
{
  vtkSmartPointer<vtkOutEdgeIterator> edge_out_iterator = vtkSmartPointer<vtkOutEdgeIterator>::New();
  vtkSmartPointer<vtkCharArray> mark = vtkSmartPointer<vtkCharArray>::New();
  vtkOutEdgeType out_edge;
  int step = 0;
  int layer = 0;
  vtkIdType ID = 0;

  mark->SetNumberOfValues( input->GetNumberOfVertices() );
  for ( vtkIdType i = 0; i <= mark->GetMaxId(); ++i )
    mark->SetValue(i,0);

  while ( source->size() > 0 )
  {
    ID = source->front();
    source->pop_front();

    order->SetValue( step, ID );
    input->GetOutEdges( ID, edge_out_iterator );
    layer = layers->GetValue( ID ) + 1;

    while ( edge_out_iterator->HasNext() )
    {
      out_edge = edge_out_iterator->Next();
      if ( mark->GetValue( out_edge.Target ) == 0 )
      {
        if ( layers->GetValue( out_edge.Target ) == layer )
        {
          mark->SetValue( out_edge.Target, 1 );
          source->push_back( out_edge.Target );
        }
      }
    }

    ++step;
  }

  while ( StandAlones->size() > 0 )
  {
    order->SetValue( step, StandAlones->front() );
    StandAlones->pop_front();
    ++step;
  }

  vtkDebugMacro( << "Vertex order building is successful." );
}

void vtkSimple3DCirclesStrategy::Transform( double Local[], double Global[] )
{
  vtkMath::Multiply3x3( this->T, Local, Global );
  Global[0] = this->Origin[0] + Global[0];
  Global[1] = this->Origin[1] + Global[1];
  Global[2] = this->Origin[2] + Global[2];
}

