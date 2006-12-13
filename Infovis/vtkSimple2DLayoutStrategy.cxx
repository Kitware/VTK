/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimple2DLayoutStrategy.cxx

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

// Please note: Lots of this code was directly 'lifted' from 
// the vtkForceDirectedLayoutStrategy class
#include "vtkSimple2DLayoutStrategy.h"

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCommand.h>
#include <vtkDataArray.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>

#include "vtkAbstractGraph.h"
#include "vtkGraph.h"
#include "vtkTree.h"


vtkCxxRevisionMacro(vtkSimple2DLayoutStrategy, "1.6");
vtkStandardNewMacro(vtkSimple2DLayoutStrategy);


// Cool-down function.
static inline float CoolDown(float t, float r) 
{  
  return t-(t/r);
}

// ----------------------------------------------------------------------

vtkSimple2DLayoutStrategy::vtkSimple2DLayoutStrategy()
{
  this->MaxNumberOfIterations = 100;
  this->IterationsPerLayout = 100;
  this->InitialTemperature = 1;
  this->CoolDownRate = 50.0;
  this->LayoutComplete = 0;
  this->ArcWeightField = 0;
}

// ----------------------------------------------------------------------

vtkSimple2DLayoutStrategy::~vtkSimple2DLayoutStrategy()
{
  this->SetArcWeightField(0);
}

// ----------------------------------------------------------------------

// Set the graph that will be laid out
void vtkSimple2DLayoutStrategy::Initialize()
{
  // Set up some quick access variables
  vtkPoints* pts = this->Graph->GetPoints();
  vtkIdType numNodes = this->Graph->GetNumberOfNodes();
  vtkIdType numArcs = this->Graph->GetNumberOfArcs();
  
  // The optimal distance between vertices.
  float optDist = sqrt(1.0 / static_cast<float>(numNodes));
    
  // Put the data into compact, fast access data structures
  this->VArray = new vtkLayoutVertex[numNodes];
  this->ArcArray =  new vtkLayoutArc[numArcs];

  // Load up the vertex data structures
  for (vtkIdType i=0; i<numNodes; ++i)
    {
    // Get point position
    double pointCoords[3];
    pts->GetPoint(i, pointCoords);
    
    // If coordinates are 0 then give some random value
    if ((pointCoords[0]==0) && (pointCoords[1]==0))
      {
      pointCoords[0] = optDist*(static_cast<float>(rand())/RAND_MAX - .5);
      pointCoords[1] = optDist*(static_cast<float>(rand())/RAND_MAX - .5);
      }
      
    // Set point position
    this->VArray[i].x = static_cast<float>(pointCoords[0]);
    this->VArray[i].y = static_cast<float>(pointCoords[1]);
    }

  // Get the weight array
  vtkDataArray* weightArray = NULL;
  double avgWeight = 0;
  if (this->ArcWeightField != NULL)
    {
    weightArray = vtkDataArray::SafeDownCast(this->Graph->GetArcData()->GetAbstractArray(this->ArcWeightField));
    if (weightArray != NULL)
      {
      for (vtkIdType w = 0; w < weightArray->GetNumberOfTuples(); w++)
        {
        double* tuple = weightArray->GetTuple(w);
        avgWeight += tuple[0];
        }
      avgWeight /= weightArray->GetNumberOfTuples();
      }
    }
    
  // Load up the edge data structures
  for (vtkIdType i=0; i<numArcs; ++i)
    {
    this->ArcArray[i].from = this->Graph->GetSourceNode(i);
    this->ArcArray[i].to = this->Graph->GetTargetNode(i);
    if (weightArray != NULL)
      {
      double* tuple = weightArray->GetTuple(i);
      this->ArcArray[i].weight = tuple[0] / avgWeight;
      }
    else
      {
      this->ArcArray[i].weight = 1.0;
      }
    }
    
  // Set some vars
  this->TotalIterations = 0;
  this->LayoutComplete = 0;
  this->Temp = this->InitialTemperature;
}

// ----------------------------------------------------------------------

// Simple graph layout method
void vtkSimple2DLayoutStrategy::Layout()
{
  // Do I have a graph to layout
  if (this->Graph == NULL)
    {
    vtkErrorMacro("Graph Layout called with Graph==NULL, call SetGraph(g) first");
    this->LayoutComplete = 1;
    return;
    }
  
  // Set up some variables
  vtkPoints* pts = this->Graph->GetPoints();
  vtkIdType numNodes = this->Graph->GetNumberOfNodes();
  vtkIdType numArcs = this->Graph->GetNumberOfArcs();
    
  // The optimal distance between vertices.
  float optDist = sqrt(1.0 / static_cast<float>(numNodes));
  

  // This is the mega, uber, triple inner loop
  // ye of weak hearts, tread no further!
  float delta[]={0,0,0};
  float dis, disSquared;
  float repulseValue;
  float attractValue;
  for(int i = 0; i < this->IterationsPerLayout; ++i)
    {
  
    // Calculate the repulsive forces
    for(vtkIdType j=0; j<numNodes; ++j)
      {
      this->VArray[j].dx = 0;
      this->VArray[j].dy = 0;
      for(vtkIdType k=0; k<numNodes; ++k)
        {
        if (k != j)
          {
          delta[0] = this->VArray[j].x - this->VArray[k].x;
          delta[1] = this->VArray[j].y - this->VArray[k].y;
          
          // Traditional k/d replusion
          if (this->Temp > .2)
            {
            dis = fabs(delta[0]) + fabs(delta[1]);
            repulseValue = optDist/dis;
            }
          
          // k/d**2 replusion ( flowering :)
          else
            {
            disSquared = delta[0]*delta[0] + delta[1]*delta[1];
            repulseValue = optDist/disSquared;
            }
          this->VArray[j].dx += delta[0] * repulseValue;
          this->VArray[j].dy += delta[1] * repulseValue;
          }
        }
      }
      
    // Calculate the attractive forces
    for (vtkIdType j=0; j<numArcs; ++j)
      {
      delta[0] = this->VArray[this->ArcArray[j].to].x - 
             this->VArray[this->ArcArray[j].from].x;
      delta[1] = this->VArray[this->ArcArray[j].to].y - 
             this->VArray[this->ArcArray[j].from].y;
      disSquared = delta[0]*delta[0] + delta[1]*delta[1];

      // Emergency action on edges that are 10x 
      // their 'resting' distance
      if (disSquared > 100*optDist)
        {
        float jump = this->Temp * .5;
        this->VArray[this->ArcArray[j].to].x   -= delta[0] * jump;
        this->VArray[this->ArcArray[j].to].y   -= delta[1] * jump;
        this->VArray[this->ArcArray[j].from].x += delta[0] * jump;
        this->VArray[this->ArcArray[j].from].y += delta[1] * jump;
        }

      // Perform weight adjustment
      attractValue = this->ArcArray[j].weight*disSquared/optDist;
      this->VArray[this->ArcArray[j].to].dx   -= delta[0] * attractValue;
      this->VArray[this->ArcArray[j].to].dy   -= delta[1] * attractValue;
      this->VArray[this->ArcArray[j].from].dx += delta[0] * attractValue;
      this->VArray[this->ArcArray[j].from].dy += delta[1] * attractValue;
      }

    // Combine the forces to compute new positions
    float norm;
    for (vtkIdType j=0; j<numNodes; ++j)
      {
      delta[0] = this->VArray[j].dx;
      delta[1] = this->VArray[j].dy;
      norm = vtkMath::Normalize(delta);
      float minimum = (norm < this->Temp ? norm : this->Temp);
      this->VArray[j].x += delta[0] * minimum;
      this->VArray[j].y += delta[1] * minimum;
      }
      
    // Reduce temperature as layout approaches a better configuration.
    this->Temp = CoolDown(this->Temp, this->CoolDownRate);

    // Announce progress
    double progress = (i+this->TotalIterations) / 
                      static_cast<double>(this->MaxNumberOfIterations);
    this->InvokeEvent(vtkCommand::ProgressEvent, static_cast<void *>(&progress));

   } // End loop this->IterationsPerLayout

   // Now take the temporary point coordinate datastructure 
   // and convert back to VTK data structures
   for (vtkIdType i=0; i<numNodes; ++i)
    {
    pts->SetPoint (i, this->VArray[i].x, this->VArray[i].y, 0.0);
    }
    
  // Check for completion of layout
  this->TotalIterations += this->IterationsPerLayout;
  if (this->TotalIterations >= this->MaxNumberOfIterations)
    {
    // I'm done
    this->LayoutComplete = 1;
    }
}

void vtkSimple2DLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InitialTemperature: " << this->InitialTemperature << endl;
  os << indent << "MaxNumberOfIterations: " << this->MaxNumberOfIterations << endl;
  os << indent << "IterationsPerLayout: " << this->IterationsPerLayout << endl;
  os << indent << "CoolDownRate: " << this->CoolDownRate << endl;
  os << indent << "ArcWeightField: " << (this->ArcWeightField ? this->ArcWeightField : "(none)") << endl;
}
