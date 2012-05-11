/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStackedTreeLayoutStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkStackedTreeLayoutStrategy.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkFloatArray.h"
#include "vtkDataArray.h"
#include "vtkIntArray.h"
#include "vtkTree.h"
#include "vtkTreeDFSIterator.h"
#include "vtkTreeLevelsFilter.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name)                                  \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkStandardNewMacro(vtkStackedTreeLayoutStrategy);

vtkStackedTreeLayoutStrategy::vtkStackedTreeLayoutStrategy()
{
  this->InteriorRadius = 6.0;
  this->RingThickness = 1.0;
  this->RootStartAngle = 0.;
  this->RootEndAngle = 360.;
  this->UseRectangularCoordinates = false;
  this->Reverse = false;
  this->InteriorLogSpacingValue = 1.0;
}

vtkStackedTreeLayoutStrategy::~vtkStackedTreeLayoutStrategy()
{
}

void vtkStackedTreeLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InteriorRadius: " << this->InteriorRadius << endl;
  os << indent << "RingThickness: " << this->RingThickness << endl;
  os << indent << "RootStartAngle: " << this->RootStartAngle << endl;
  os << indent << "RootEndAngle: " << this->RootEndAngle << endl;
  os << indent << "UseRectangularCoordinates: " << this->UseRectangularCoordinates << endl;
  os << indent << "Reverse: " << this->Reverse << endl;
  os << indent << "InteriorLogSpacingValue: " << this->InteriorLogSpacingValue << endl;
}

void vtkStackedTreeLayoutStrategy::Layout(vtkTree* inputTree,
                                          vtkDataArray* coordsArray,
                                          vtkDataArray* sizeArray)
{
  if( !inputTree || inputTree->GetNumberOfVertices() == 0 )
    {
    return;
    }
  if (!coordsArray)
    {
    vtkErrorMacro("Area array not defined.");
    return;
    }

  vtkDataSetAttributes* data = inputTree->GetVertexData();

  VTK_CREATE(vtkDoubleArray, textRotationArray);
  textRotationArray->SetName( "TextRotation" );
  textRotationArray->SetNumberOfComponents(1);
  textRotationArray->SetNumberOfTuples(inputTree->GetNumberOfVertices());
  data->AddArray( textRotationArray );

  VTK_CREATE(vtkDoubleArray, textBoundedSizeArray);
  textBoundedSizeArray->SetName( "TextBoundedSize" );
  textBoundedSizeArray->SetNumberOfComponents(2);
  textBoundedSizeArray->SetNumberOfTuples(inputTree->GetNumberOfVertices());
  data->AddArray( textBoundedSizeArray );

  double outer_radius = 0.0;
  if (this->Reverse)
    {
    VTK_CREATE(vtkTreeLevelsFilter, levelFilter);
    VTK_CREATE(vtkTree, newTree);
    newTree->ShallowCopy( inputTree );
    levelFilter->SetInputData( newTree );
    levelFilter->Update();
    vtkTree* levelTree = levelFilter->GetOutput();

    vtkIntArray *levelArray = vtkIntArray::SafeDownCast(
        levelTree->GetVertexData()->GetAbstractArray("level"));
    int max_level = 0;
    for( int i = 0; i < levelTree->GetNumberOfVertices(); i++ )
      {
      int level = levelArray->GetValue(i);
      if( level > max_level )
        {
        max_level = level;
        }
      }
    outer_radius = max_level*this->RingThickness + this->InteriorRadius;
    }

  // Get the root vertex and set it
  vtkIdType rootId = inputTree->GetRoot();
  float coords[4] = {this->RootStartAngle, this->RootEndAngle, 0.0, 0.0};
  if (this->Reverse)
    {
    coords[2] = outer_radius - this->RingThickness;
    coords[3] = outer_radius;
    }
  else
    {
    coords[3] = this->InteriorRadius;
    }
  coordsArray->SetTuple(rootId, coords);

  // Now layout the children vertices
  this->LayoutChildren(inputTree, coordsArray, sizeArray,
      inputTree->GetNumberOfChildren(rootId),
      rootId, 0, coords[2], coords[3], coords[0], coords[1]);

  vtkPoints* points = vtkPoints::New();
  vtkIdType numVerts = inputTree->GetNumberOfVertices();
  points->SetNumberOfPoints(numVerts);
  for( vtkIdType i = 0; i < numVerts; i++ )
    {
    double sector_coords[4];
    coordsArray->GetTuple( i, sector_coords );
    double x, y, z;
    if( this->UseRectangularCoordinates )
      {
      x = 0.5*(sector_coords[0] + sector_coords[1]);
      y = 0.5*(sector_coords[2] + sector_coords[3]);
      z = 0.;

      textRotationArray->SetValue( i, 0 );
      textBoundedSizeArray->SetValue( 2*i, sector_coords[1] - sector_coords[0]);
      textBoundedSizeArray->SetValue( 2*i + 1, sector_coords[3] - sector_coords[2] );
      }
    else
      {
      if( i == rootId )
        {
        x = y = z = 0.;

        textRotationArray->SetValue( i, 0 );
        textBoundedSizeArray->SetValue( 2*i, 0 );
        textBoundedSizeArray->SetValue( 2*i + 1, 0 );
        }
      else
        {
        double r = (0.5*(sector_coords[3] - sector_coords[2])) + sector_coords[2];
        double theta = sector_coords[0] + (0.5*(sector_coords[1]-sector_coords[0]));
        x = r * cos( vtkMath::RadiansFromDegrees( theta ) );
        y = r * sin( vtkMath::RadiansFromDegrees( theta ) );
        z = 0.;

        double sector_arc_length = r * vtkMath::RadiansFromDegrees(sector_coords[1] - sector_coords[0]);
        double radial_arc_length = sector_coords[3] - sector_coords[2];
        double aspect_ratio = sector_arc_length / radial_arc_length;
        if( aspect_ratio > 1 )
          {
          //sector length is greater than radial length;
          // align text with the sector
          if( theta > 0. && theta < 180. )
            {
            textRotationArray->SetValue( i, theta - 90. );
            }
          else
            {
            textRotationArray->SetValue( i, theta + 90. );
            }
          textBoundedSizeArray->SetValue( 2*i, sector_arc_length );
          textBoundedSizeArray->SetValue( 2*i + 1, radial_arc_length );
          }
        else
          {
          //radial length is greater than sector length;
          // align text radially...
          if( theta > 90. && theta < 270. )
            {
            textRotationArray->SetValue( i, theta - 180. );
            }
          else
            {
            textRotationArray->SetValue( i, theta );
            }
          textBoundedSizeArray->SetValue( 2*i, radial_arc_length );
          textBoundedSizeArray->SetValue( 2*i + 1, sector_arc_length );
          }
        }
      }
    points->SetPoint(i, x, y, z);
    }
  inputTree->SetPoints(points);
  points->Delete();
}

void vtkStackedTreeLayoutStrategy::LayoutEdgePoints(
  vtkTree* inputTree,
  vtkDataArray* sectorsArray,
  vtkDataArray* vtkNotUsed(sizeArray),
  vtkTree* outputTree)
{
  VTK_CREATE(vtkTreeLevelsFilter, levelFilter);
  VTK_CREATE(vtkTree, newTree);
  newTree->ShallowCopy( inputTree );
  levelFilter->SetInputData( newTree );
  levelFilter->Update();
  vtkTree* levelTree = levelFilter->GetOutput();
  outputTree->ShallowCopy( levelTree );

  vtkIntArray* levelArray = vtkIntArray::SafeDownCast(
      levelTree->GetVertexData()->GetAbstractArray("level"));

  double exteriorRadius = VTK_DOUBLE_MAX;
  double sector_coords[4];
  int max_level = 0;
  for( int i = 0; i < outputTree->GetNumberOfVertices(); i++ )
    {
    int level = levelArray->GetValue(i);
    if( level > max_level )
      {
      max_level = level;
      }
    if (inputTree->IsLeaf(i))
      {
      sectorsArray->GetTuple( i, sector_coords );
      if (sector_coords[2] < exteriorRadius)
        {
        exteriorRadius = sector_coords[2];
        }
      }
    }

  double spacing = this->InteriorLogSpacingValue;

  // The distance between level L-1 and L is s^L.
  // Thus, if s < 1 then the distance between levels gets smaller in higher levels,
  //       if s = 1 the distance remains the same, and
  //       if s > 1 the distance get larger.
  // The height (distance from the root) of level L, then, is
  // s + s^2 + s^3 + ... + s^L, where s is the log spacing value.
  // The max height (used for normalization) is
  // s + s^2 + s^3 + ... + s^maxLevel.
  // The quick formula for computing this is
  // sum_{i=1}^{n} s^i = (s^(n+1) - 1)/(s - 1) - 1        if s != 1
  //                   = n                                if s == 1
  double maxHeight = max_level;
  double eps = 1e-8;
  double diff = spacing - 1.0 > 0 ? spacing - 1.0 : 1.0 - spacing;
  if (diff > eps)
    {
    maxHeight = (pow(spacing, max_level+1.0) - 1.0)/(spacing - 1.0) - 1.0;
    }

  vtkPoints* points = vtkPoints::New();
  vtkIdType rootId = outputTree->GetRoot();
  vtkIdType numVerts = outputTree->GetNumberOfVertices();
  points->SetNumberOfPoints(numVerts);
  for( vtkIdType i = 0; i < numVerts; i++ )
    {
    if( !this->UseRectangularCoordinates && i == rootId )
      {
      points->SetPoint( i, 0, 0, 0 );
      continue;
      }

    sectorsArray->GetTuple( i, sector_coords );

    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    if( this->UseRectangularCoordinates )
      {
      if( inputTree->IsLeaf(i) )
        {
        if( this->Reverse )
          {
          y = sector_coords[2];
          }
        else
          {
          y = sector_coords[3];
          }
        }
      else
        {
        if( this->Reverse )
          {
          y = this->InteriorRadius - this->RingThickness*(maxHeight + maxHeight - inputTree->GetLevel(i));
          }
        else
          {
          y = this->InteriorRadius + this->RingThickness*(maxHeight + maxHeight - inputTree->GetLevel(i));
          }
        }
      x = 0.5*(sector_coords[0] + sector_coords[1]);
      z = 0.;
      }
    else
      {
      double r;
      if( inputTree->IsLeaf(i) )
        {
        r = sector_coords[2];
        }
      else
        {
        if (diff <= eps)
          {
          r = outputTree->GetLevel(i)/maxHeight;
          }
        else
          {
          r = ((pow(spacing, outputTree->GetLevel(i)+1.0) - 1.0)/(spacing - 1.0) - 1.0)/maxHeight;
          }
        // scale the spacing value based on the radius of the
        // circle we have to work with...
        r *= exteriorRadius;
        }

      double theta = sector_coords[0] + (0.5*(sector_coords[1]-sector_coords[0]));
      x = r * cos( vtkMath::RadiansFromDegrees( theta ) );
      y = r * sin( vtkMath::RadiansFromDegrees( theta ) );
      z = 0.;
      }
    points->SetPoint(i, x, y, z);
    }
  outputTree->SetPoints(points);
  points->Delete();
}

void vtkStackedTreeLayoutStrategy::LayoutChildren(
  vtkTree *tree, vtkDataArray *coordsArray, vtkDataArray *sizeArray,
  vtkIdType nchildren, vtkIdType parent, vtkIdType begin,
  float parentInnerRad, float parentOuterRad,
  float parentStartAng, float parentEndAng)
{
  double new_interior_rad = 0.0;
  double new_outer_rad = 0.0;
  if (this->Reverse)
    {
    new_interior_rad = parentInnerRad - this->RingThickness;
    new_outer_rad = parentInnerRad;
    }
  else
    {
    new_interior_rad = parentOuterRad;
    new_outer_rad = new_interior_rad + this->RingThickness;
    }
  //FIXME - we may want to do this instead...
  //double new_outer_rad = new_interior_rad +this->RingThickness[level];

  double radial_spacing = this->ShrinkPercentage * this->RingThickness;
  new_outer_rad -= radial_spacing;
  //new_interior_rad += 0.5*radial_spacing;

  //now calculate the width of each of the sectors for each vertex
  // first calculate the total summed weight for each of the children vertices
  double total_weighted_sum = 0;
  vtkIdType i;
  for( i = begin; i < nchildren; i++)
    {
    if (sizeArray)
      {
      total_weighted_sum +=
        static_cast<float>(sizeArray->GetTuple1(tree->GetChild(parent, i)));
      }
    else
      {
      total_weighted_sum += 1.0;
      }
    }

  // If we are doing radial layout, put extra space on the full rings
  // so the first and last children don't butt up against each other.
  vtkIdType num_spaces = nchildren - 1;
  double parent_angle = parentEndAng - parentStartAng;
  if (!this->UseRectangularCoordinates && parent_angle == 360.0)
    {
    num_spaces = nchildren;
    }
  double available_angle = parent_angle;
  double conversion = vtkMath::Pi()/180.0;
  double spacing = 0.0;
  if (nchildren > 1)
    {
    double parent_length;
    if (this->UseRectangularCoordinates)
      {
      parent_length = parent_angle;
      }
    else
      {
      parent_length = conversion * parent_angle * new_outer_rad;
      }
    double spacing_length;
    if (radial_spacing * num_spaces > 0.25 * parent_length)
      {
      spacing_length = 0.25 * parent_length;
      }
    else
      {
      spacing_length = radial_spacing * num_spaces;
      }
    double total_space;
    if (this->UseRectangularCoordinates)
      {
      total_space = spacing_length;
      }
    else
      {
      total_space = spacing_length / new_outer_rad / conversion;
      }
    spacing = total_space / num_spaces;
    available_angle -= total_space;
    }

  float coords[4];
  double current_angle = parentStartAng;
  for( i = begin; i < nchildren; i++)
    {
    int id = tree->GetChild(parent, i);
    float cur_size = 1.0;
    if (sizeArray)
      {
      cur_size = static_cast<float>(sizeArray->GetTuple1(id));
      }
    double this_arc = available_angle *
      ( cur_size / total_weighted_sum );

    coords[2] = new_interior_rad;
    coords[3] = new_outer_rad;
    coords[0] = current_angle;
    coords[1] = current_angle + this_arc;

    coordsArray->SetTuple(id, coords);

    current_angle += this_arc + spacing;

    vtkIdType numNewChildren = tree->GetNumberOfChildren(id);
    if (numNewChildren > 0)
      {
      this->LayoutChildren(tree, coordsArray, sizeArray, numNewChildren, id, 0,
                           coords[2], coords[3], coords[0], coords[1]);
      }
    }
}

vtkIdType vtkStackedTreeLayoutStrategy::FindVertex(
    vtkTree* otree,
    vtkDataArray* array,
    float pnt[2])
{
  if (this->UseRectangularCoordinates)
    {
    float blimits[4];
    vtkIdType vertex = otree->GetRoot();
    if(vertex < 0)
      {
      return vertex;
      }
    vtkFloatArray *boundsInfo = vtkFloatArray::SafeDownCast(array);

    // Now try to find the vertex that contains the point
    boundsInfo->GetTupleValue(vertex, blimits); // Get the extents of the root
    if( ((pnt[1] > blimits[2]) && (pnt[1] < blimits[3])) &&
        ((pnt[0] > blimits[0]) && (pnt[0] < blimits[1])) )
      {
      // Point is at the root vertex.
      return vertex;
      }

    // Now traverse the children to try and find
    // the vertex that contains the point
    vtkIdType child;
    VTK_CREATE(vtkTreeDFSIterator, it);
    it->SetTree( otree );
    it->SetStartVertex( vertex );

    while (it->HasNext())
      {
      child = it->Next();
      boundsInfo->GetTupleValue(child, blimits); // Get the extents of the child
      bool beyond_radial_bounds = false;
      bool beyond_angle_bounds = false;
      if( (pnt[1] < blimits[2]) || (pnt[1] > blimits[3]))
        beyond_radial_bounds = true;
      if( (pnt[0] < blimits[0]) || (pnt[0] > blimits[1]))
        beyond_angle_bounds = true;

      if( beyond_radial_bounds || beyond_angle_bounds )
        {
        continue;
        }
      // If we are here then the point is contained by the child
      return child;
      }
    }
  else
    {
    // Radial layout
    float polar_location[2];
    polar_location[0] = sqrt( ( pnt[0] * pnt[0] ) + ( pnt[1] * pnt[1] ) );
    polar_location[1] = vtkMath::DegreesFromRadians( atan2( pnt[1], pnt[0] ) );
    if( polar_location[1] < 0 )
      polar_location[1] += 360.;

    float blimits[4];
    vtkIdType vertex = otree->GetRoot();
    if(vertex < 0)
      {
      return vertex;
      }
    vtkFloatArray *boundsInfo = vtkFloatArray::SafeDownCast(array);

    // Now try to find the vertex that contains the point
    boundsInfo->GetTupleValue(vertex, blimits); // Get the extents of the root
    if( ((polar_location[0] > blimits[2]) && (polar_location[0] < blimits[3])) &&
        ((polar_location[1] > blimits[0]) && (polar_location[1] < blimits[1])) )
      {
      // Point is at the root vertex.
      // but we don't want the root to be pickable, so return -1.
      // This won't work for blimits spanning the 0/360 rollover, but the test below
      // catches that case.
      return -1;
      }

    // Now traverse the children to try and find
    // the vertex that contains the point
    vtkIdType child;
    VTK_CREATE(vtkTreeDFSIterator, it);
    it->SetTree( otree );
    it->SetStartVertex( vertex );

    while (it->HasNext())
      {
      child = it->Next();
      // if the root boundary starts anywhere but zero, the root node will have passed
      // the earlier test.  This will skip the root and prevent it from being picked.
      if (child == vertex)
        {
        continue;
        }
      boundsInfo->GetTupleValue(child, blimits); // Get the extents of the child

      // the range checking below doesn't work if either or both of blimits > 360
      if ((blimits[0] > 360.0) && (blimits[1] > 360.0))
        {
        blimits[0] -= 360.0;
        blimits[1] -= 360.0;
        }
      else if ((blimits[0] < 360.0) && (blimits[1] > 360.0) && (polar_location[1] < 360.0))
        {  // if the range spans the rollover at 0/360 on the circle
        if (polar_location[1] < 90.0)
          {
          blimits[0] = 0.0;
          blimits[1] -= 360.0;
          }
        else if (polar_location[1] > 270.)
          {
          blimits[1] = 360.0;
          }
        }
      bool beyond_radial_bounds = false;
      bool beyond_angle_bounds = false;
      if( (polar_location[0] < blimits[2]) || (polar_location[0] > blimits[3]))
        beyond_radial_bounds = true;
      if( (polar_location[1] < blimits[0]) || (polar_location[1] > blimits[1]))
        beyond_angle_bounds = true;

      if( beyond_radial_bounds || beyond_angle_bounds )
        {
        continue;
        }
      // If we are here then the point is contained by the child
      return child;
      }
    }
  return -1;
}

