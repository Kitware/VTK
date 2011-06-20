/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKCoreLayout.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkKCoreLayout.h"

#include <vtkAdjacentVertexIterator.h>
#include <vtkDataSetAttributes.h>
#include <vtkDirectedGraph.h>
#include <vtkDoubleArray.h>
#include <vtkEdgeListIterator.h>
#include <vtkGraph.h>
#include <vtkIdTypeArray.h>
#include <vtkInEdgeIterator.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkIntArray.h>
#include <vtkObjectFactory.h>
#include <vtkSelection.h>
#include <vtkSmartPointer.h>
#include <vtkUndirectedGraph.h>

//#include <vtkstd/iterator>
//#include <vtkstd/vector>
#include <iostream>

#define _USE_MATH_DEFINES
#include <cstdlib>
#include <math.h>

using std::cout;
using std::endl;

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

///////////////////////////////////////////////////////////////////////////////////
// vtkKCoreLayout

vtkStandardNewMacro(vtkKCoreLayout);

// Default Constructor
vtkKCoreLayout::vtkKCoreLayout()
{
  this->SetNumberOfInputPorts(1);
  this->KCoreLabelArrayName        = NULL;
  this->PolarCoordsRadiusArrayName = NULL;
  this->PolarCoordsAngleArrayName  = NULL;
  this->CartesianCoordsXArrayName  = NULL;
  this->CartesianCoordsYArrayName  = NULL;
  this->Cartesian = false;
}

// Default Destructor
vtkKCoreLayout::~vtkKCoreLayout()
{
  this->KCoreLabelArrayName = NULL;
}

void vtkKCoreLayout::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkKCoreLayout::SetGraphConnection(vtkAlgorithmOutput* input)
{
  this->SetInputConnection(0, input);
}

int vtkKCoreLayout::FillInputPortInformation(int port, vtkInformation* info)
{
  if(port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
    return 1;
    }

  return 0;
}

// -------------------------------------------------------------------------------------------------
int vtkKCoreLayout::RequestData(vtkInformation* vtkNotUsed(request),
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector)
{
  float radius = 0.0f;
  float angle = 0.0f;
  float unit_radius = 1.0f;
  float epsilon = 0.2f;
  int   max_core_level = -1;

  std::cout << ">\tvtkKCoreLayout.RequestData()" << std::endl;

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkGraph * input  = vtkGraph::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkGraph * output = vtkGraph::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Send the data to output
  output->ShallowCopy(input);

//  vtkGraph* const input  = vtkGraph::GetData(inputVector[0]);
//  vtkGraph* const output = vtkGraph::GetData(outputVector);

//  output->CheckedShallowCopy(input);

  // graph size
  vtkIdType num_verts = output->GetNumberOfVertices();
  vtkIdType num_edges = output->GetNumberOfEdges();


  if(this->KCoreLabelArrayName)
    {
    }
  else
    {
    this->KCoreLabelArrayName = (char*)"kcore";
    }

  // Get the kcore attribute array
  vtkIntArray * kcore_array = NULL;
  kcore_array = vtkIntArray::SafeDownCast(output->GetVertexData()->GetArray(this->KCoreLabelArrayName));

  if(!kcore_array)
    {
    vtkErrorMacro(<<"Vertex attribute array " << this->KCoreLabelArrayName << " is not a vtkIntArray.");
    return 0;
    }

  VTK_CREATE(vtkDoubleArray, arrayX);
  VTK_CREATE(vtkDoubleArray, arrayY);
  VTK_CREATE(vtkDoubleArray, arrayRadius);
  VTK_CREATE(vtkDoubleArray, arrayAngle);

  // Create output arrays (radius, angle) for polar coordinates
  if(this->Cartesian)
    {
    arrayX->SetNumberOfTuples( output->GetNumberOfVertices() );
    if(this->CartesianCoordsXArrayName)
      {
      arrayX->SetName(this->CartesianCoordsXArrayName);
      }
    else
      {
      arrayX->SetName("coord_x");
      }

    arrayY->SetNumberOfTuples( output->GetNumberOfVertices() );
    if(this->CartesianCoordsXArrayName)
      {
      arrayY->SetName(this->CartesianCoordsXArrayName);
      }
    else
      {
      arrayY->SetName("coord_x");
      }
    output->GetVertexData()->AddArray( arrayX );
    output->GetVertexData()->AddArray( arrayY );
    }
  else
    {
    arrayRadius->SetNumberOfTuples( output->GetNumberOfVertices() );

    if(this->PolarCoordsRadiusArrayName)
      {
      arrayRadius->SetName(this->PolarCoordsRadiusArrayName);
      }
    else
      {
      arrayRadius->SetName("coord_radius");
      }

    arrayAngle->SetNumberOfTuples( output->GetNumberOfVertices() );
    if(this->PolarCoordsAngleArrayName)
      {
      arrayAngle->SetName(this->PolarCoordsAngleArrayName);
      }
    else
      {
      arrayAngle->SetName("coord_angle");
      }

    output->GetVertexData()->AddArray( arrayRadius );
    output->GetVertexData()->AddArray( arrayAngle );
    }


  // find the maximum core level
  for(vtkIdType i=0; i<kcore_array->GetNumberOfTuples(); i++)
    {
    max_core_level = max_core_level < kcore_array->GetValue(i) ? kcore_array->GetValue(i) : max_core_level;
    }

  cout << "max core level: " << max_core_level << endl;

  // Loop over each vertex and calculate its position
  for(vtkIdType vidx=0; vidx<num_verts; vidx++)
    {
    int current_level = kcore_array->GetValue(vidx);
    if(current_level == max_core_level)
      {
      double radius = unit_radius;
      double angle  = float(rand()%100000)/100000 * 2.0 * M_PI;

      cout << vidx << "\t(" << radius << "," << angle << ")" << endl;
      if(this->Cartesian)
        {
        arrayX->SetValue(vidx, radius * cos(angle) );
        arrayY->SetValue(vidx, radius * sin(angle) );
        }
      else
        {
        arrayRadius->SetValue(vidx, radius);
        arrayAngle->SetValue(vidx, angle);
        }
      }
    else
      {
      // vertices in shells other than the innermost are positioned in rings around the shells
      // farther in.  By default, each ring is 1 unit out from the next one.  The innermost has
      // a radius of 1.
      int vertex_native_ring = (max_core_level - current_level) + 1;

      // the radius is adjusted by how many of this vertex's neighbors are in the same or
      // higher numbered shells.
      //int neighbors_same_or_higher = 0;
      vtkIdTypeArray * neighbors_same_or_higher = vtkIdTypeArray::New();

      vtkAdjacentVertexIterator * it = vtkAdjacentVertexIterator::New();
      output->GetAdjacentVertices( vidx, it );
      while(it->HasNext())
        {
        vtkIdType nid = it->Next();
        if( kcore_array->GetValue(nid) >= current_level )
          {
          neighbors_same_or_higher->InsertNextValue( nid );
          }
        }
      it->Delete();

//      cout << vidx << "\tneighbors_same_or_higher = " << neighbors_same_or_higher->GetNumberOfTuples() << endl;

      int neighbor_average_ring = 0;
      if( neighbors_same_or_higher->GetNumberOfTuples() > 0 )
        {
        for( vtkIdType i=0; i<neighbors_same_or_higher->GetNumberOfTuples(); i++ )
          {
          vtkIdType neighbor_idx = neighbors_same_or_higher->GetValue(i);
          neighbor_average_ring += (1 + max_core_level - kcore_array->GetValue( neighbor_idx ));
          }
        }
      else
        {
        // No neighbors - do something semi-reasonable and pretend they're all in the regular
        // ring anyway.
        neighbor_average_ring = vertex_native_ring;
        }
//      cout << vidx << "\tneighbor_average_ring = " << neighbor_average_ring << endl;

      // Use epsilon as an interpolation factor between the two ring levels
      radius = (1-epsilon) * vertex_native_ring + epsilon * neighbor_average_ring;
      radius *= unit_radius;

      cout << "DEBUG: Vertex in shell level "<< current_level << " (native ring " << vertex_native_ring
           << ", max core level " << max_core_level << ") and average neighbor ring " << neighbor_average_ring
           << " (" << neighbors_same_or_higher->GetNumberOfTuples() << " neighbor(s)) is at radius "
           << radius << " with epsilon " << epsilon << endl;

      // Need the angle.
      angle = float(rand()%100000)/100000 * (2.0 * M_PI);

      // set the values
      if(this->Cartesian)
        {
        arrayX->SetValue(vidx, radius * cos(angle) );
        arrayY->SetValue(vidx, radius * sin(angle) );
        }
      else
        {
        arrayRadius->SetValue(vidx, radius);
        arrayAngle->SetValue(vidx, angle);
        }

      neighbors_same_or_higher->Delete();
      }
    }


  for(vtkIdType vidx=0; vidx<num_verts; vidx++)
    {

    //cout << vidx << "\t";
    if(this->Cartesian)
      {
      printf("%d\t%.4f\t%.4f\n", vidx, arrayX->GetValue(vidx), arrayY->GetValue(vidx));
      //cout << arrayX->GetValue(vidx) << "\t" << arrayY->GetValue(vidx);
      }
    else
      {
      printf("%d\t%.4f\t%.4f\n", vidx, arrayRadius->GetValue(vidx), arrayRadius->GetValue(vidx));
      //cout << arrayRadius->GetValue(vidx) << "\t" << arrayAngle->GetValue(vidx);
      }
    //cout << endl;
    }

  return 1;
}
