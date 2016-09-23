/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkForceDirectedLayoutStrategy.cxx

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

#include "vtkForceDirectedLayoutStrategy.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkEdgeListIterator.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOutEdgeIterator.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkTree.h"

vtkStandardNewMacro(vtkForceDirectedLayoutStrategy);

vtkForceDirectedLayoutStrategy::vtkForceDirectedLayoutStrategy()
{
  this->RandomSeed = 123;
  this->GraphBounds[0] = this->GraphBounds[2] = this->GraphBounds[4] = -0.5;
  this->GraphBounds[1] = this->GraphBounds[3] = this->GraphBounds[5] =  0.5;
  this->MaxNumberOfIterations = 50;
  this->IterationsPerLayout = 50;
  this->InitialTemperature = 10.0;
  this->CoolDownRate = 10.0;
  this->LayoutComplete = 0;
  this->AutomaticBoundsComputation = false;
  this->ThreeDimensionalLayout = false;
  this->RandomInitialPoints = true;
  this->v = NULL;
  this->e = NULL;
}

vtkForceDirectedLayoutStrategy::~vtkForceDirectedLayoutStrategy()
{
  delete[] this->v;
  delete[] this->e;
}


// Cool-down function.
static inline double CoolDown(double t, double r)
{
  if (t<.01) return .01;
  return t-(t/r);
}

static inline double forceAttract(double x, double k)
{
  return (x * x) / k;
}

static inline double forceRepulse(double x, double k)
{
  if (x != 0.0)
  {
    return k * k / x;
  }
  else
  {
    return VTK_DOUBLE_MAX;
  }
}


// In the future this method should setup data
// structures, etc... so that Layout doesn't have to
// do that every time it's called
void vtkForceDirectedLayoutStrategy::Initialize()
{
  vtkPoints* pts = this->Graph->GetPoints();
  vtkIdType numVertices = this->Graph->GetNumberOfVertices();
  vtkIdType numEdges = this->Graph->GetNumberOfEdges();

  // Generate bounds automatically if necessary. It's the same
  // as the input bounds.
  if ( this->AutomaticBoundsComputation )
  {
    pts->GetBounds(this->GraphBounds);
  }

  for (int i = 0; i < 3; i++)
  {
    if ( this->GraphBounds[2*i+1] <= this->GraphBounds[2*i] )
    {
      this->GraphBounds[2*i+1] = this->GraphBounds[2*i] + 1;
    }
  }

  delete[] this->v;
  delete[] this->e;
  this->v = new vtkLayoutVertex[numVertices];
  this->e = new vtkLayoutEdge[numEdges];

  int maxCoord = this->ThreeDimensionalLayout ? 3 : 2;

  // Get the points, either x,y,0 or x,y,z or random
  if (this->RandomInitialPoints)
  {
    vtkMath::RandomSeed(this->RandomSeed);

    for (vtkIdType i = 0; i < numVertices; i++)
    {
      for (int j = 0; j < maxCoord; j++)
      {
        double r = vtkMath::Random();
        v[i].x[j] = (this->GraphBounds[2*j+1] - this->GraphBounds[2*j])*r + this->GraphBounds[2*j];
      }
      if (!this->ThreeDimensionalLayout)
      {
        v[i].x[2] = 0;
      }
    }
  }
  else
  {
    for (vtkIdType i = 0; i < numVertices; i++)
    {
      pts->GetPoint(i, v[i].x);
      if (!this->ThreeDimensionalLayout)
      {
        v[i].x[2] = 0;
      }
    }
  }

  // Get the edges
  vtkSmartPointer<vtkEdgeListIterator> edges =
    vtkSmartPointer<vtkEdgeListIterator>::New();
  this->Graph->GetEdges(edges);
  while (edges->HasNext())
  {
    vtkEdgeType edge = edges->Next();
    //cerr << edge.Id << ": " << edge.Source << "," << edge.Target << endl;
    e[edge.Id].t = edge.Source;
    e[edge.Id].u = edge.Target;
  }

  //cerr << endl << endl;
  //vtkSmartPointer<vtkOutEdgeIterator> it =
  //  vtkSmartPointer<vtkOutEdgeIterator>::New();
  //for (vtkIdType i = 0; i < this->Graph->GetNumberOfVertices(); ++i)
  //  {
  //  this->Graph->GetOutEdges(i, it);
  //  cerr << i << ": ";
  //  while (it->HasNext())
  //    {
  //    vtkOutEdgeType edge = it->Next();
  //    cerr << edge.Target << " ";
  //    }
  //  cerr << endl;
  //  }

  // More variable definitions
  double volume = (this->GraphBounds[1] - this->GraphBounds[0]) *
    (this->GraphBounds[3] - this->GraphBounds[2]) *
    (this->GraphBounds[5] - this->GraphBounds[4]);

  this->Temp = sqrt( (this->GraphBounds[1]-this->GraphBounds[0])*
                     (this->GraphBounds[1]-this->GraphBounds[0]) +
                     (this->GraphBounds[3]-this->GraphBounds[2])*
                     (this->GraphBounds[3]-this->GraphBounds[2]) +
                     (this->GraphBounds[5]-this->GraphBounds[4])*
                     (this->GraphBounds[5]-this->GraphBounds[4]) );
  if (this->InitialTemperature > 0)
  {
    this->Temp = this->InitialTemperature;
  }
  // The optimal distance between vertices.
  this->optDist = pow(volume / numVertices, 0.33333);

  // Set some vars
  this->TotalIterations = 0;
  this->LayoutComplete = 0;

};

// ForceDirected graph layout method
void vtkForceDirectedLayoutStrategy::Layout()
{

  vtkIdType numVertices = this->Graph->GetNumberOfVertices();
  vtkIdType numEdges = this->Graph->GetNumberOfEdges();


  // Begin iterations.
  double norm, fr, fa, minimum;
  double diff[3];
  for(int i = 0; i < this->IterationsPerLayout; i++)
  {
    // Calculate the repulsive forces.
    for(vtkIdType j = 0; j < numVertices; j++)
    {
      v[j].d[0] = 0.0;
      v[j].d[1] = 0.0;
      v[j].d[2] = 0.0;
      for(vtkIdType l = 0; l < numVertices; l++)
      {
        if (j != l)
        {
          diff[0] = v[j].x[0] - v[l].x[0];
          diff[1] = v[j].x[1] - v[l].x[1];
          diff[2] = v[j].x[2] - v[l].x[2];
          norm = vtkMath::Normalize(diff);
          if (norm > 2*optDist)
          {
            fr = 0;
          }
          else
          {
            fr = forceRepulse(norm,optDist);
          }
          v[j].d[0] += diff[0] * fr;
          v[j].d[1] += diff[1] * fr;
          v[j].d[2] += diff[2] * fr;
        }
      }
    }

    // Calculate the attractive forces.
    for (vtkIdType j = 0; j < numEdges; j++)
    {
      diff[0] = v[e[j].u].x[0] - v[e[j].t].x[0];
      diff[1] = v[e[j].u].x[1] - v[e[j].t].x[1];
      diff[2] = v[e[j].u].x[2] - v[e[j].t].x[2];
      norm = vtkMath::Normalize(diff);
      fa = forceAttract(norm,optDist);
      v[e[j].u].d[0] -= diff[0] * fa;
      v[e[j].u].d[1] -= diff[1] * fa;
      v[e[j].u].d[2] -= diff[2] * fa;
      v[e[j].t].d[0] += diff[0] * fa;
      v[e[j].t].d[1] += diff[1] * fa;
      v[e[j].t].d[2] += diff[2] * fa;
    }

    // Combine the forces for a new configuration
    for (vtkIdType j = 0; j < numVertices; j++)
    {
      norm = vtkMath::Normalize(v[j].d);
      minimum = (norm < this->Temp ? norm : this->Temp);
      v[j].x[0] += v[j].d[0] * minimum;
      v[j].x[1] += v[j].d[1] * minimum;
      v[j].x[2] += v[j].d[2] * minimum;
    }

    // Reduce temperature as layout approaches a better configuration.
    this->Temp = CoolDown(this->Temp, this->CoolDownRate);
  }

  // Get the bounds of the graph and scale and translate to
  // bring them within the bounds specified.
  vtkPoints *newPts = vtkPoints::New();
  newPts->SetNumberOfPoints(numVertices);
  for (vtkIdType i = 0; i < numVertices; i++)
  {
    newPts->SetPoint(i, v[i].x);
  }
  double bounds[6], sf[3], x[3], xNew[3];
  double center[3], graphCenter[3];
  double len;
  newPts->GetBounds(bounds);
  for (int i = 0; i < 3; i++)
  {
    if ( (len=(bounds[2*i+1] - bounds[2*i])) == 0.0 )
    {
      len = 1.0;
    }
    sf[i] = (this->GraphBounds[2*i+1] - this->GraphBounds[2*i]) / len;
    center[i] = (bounds[2*i+1] + bounds[2*i])/2.0;
    graphCenter[i] = (this->GraphBounds[2*i+1] + this->GraphBounds[2*i])/2.0;
  }

  double scale = sf[0];
  scale = (scale < sf[1] ? scale : sf[1]);
  scale = (scale < sf[2] ? scale : sf[2]);

  for (vtkIdType i = 0; i < numVertices; i++)
  {
    newPts->GetPoint(i, x);
    for (int j = 0; j < 3; j++)
    {
      xNew[j] = graphCenter[j] + scale*(x[j] - center[j]);
    }
    newPts->SetPoint(i, xNew);
  }

  // Send the data to output.
  this->Graph->SetPoints(newPts);

  // Clean up.
  newPts->Delete();


  // Check for completion of layout
  this->TotalIterations += this->IterationsPerLayout;
  if (this->TotalIterations >= this->MaxNumberOfIterations)
  {
    // I'm done
    this->LayoutComplete = 1;
  }
}

void vtkForceDirectedLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "RandomSeed: " << this->RandomSeed << endl;
  os << indent << "AutomaticBoundsComputation: "
     << (this->AutomaticBoundsComputation ? "On\n" : "Off\n");
  os << indent << "CoolDownRate: " << this->CoolDownRate << endl;
  os << indent << "GraphBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->GraphBounds[0] << ", "
     << this->GraphBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->GraphBounds[2] << ", "
     << this->GraphBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->GraphBounds[4] << ", "
     << this->GraphBounds[5] << ")\n";
  os << indent << "InitialTemperature: " << this->InitialTemperature << endl;
  os << indent << "IterationsPerLayout: " << this->IterationsPerLayout << endl;
  os << indent << "MaxNumberOfIterations: " << this->MaxNumberOfIterations << endl;
  os << indent << "RandomInitialPoints: "
     << (this->RandomInitialPoints ? "On\n" : "Off\n");
  os << indent << "Three Dimensional Layout: "
     << (this->ThreeDimensionalLayout ? "On\n" : "Off\n");
}
