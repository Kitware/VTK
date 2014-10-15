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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/


#include "vtkSimple2DLayoutStrategy.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkEdgeListIterator.h"
#include "vtkFloatArray.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkTree.h"

vtkStandardNewMacro(vtkSimple2DLayoutStrategy);

#ifndef MIN
#define MIN(x, y)       ((x) < (y) ? (x) : (y))
#endif

// Cool-down function.
static inline float CoolDown(float t, float r)
{
  return t-(t/r);
}

// ----------------------------------------------------------------------

vtkSimple2DLayoutStrategy::vtkSimple2DLayoutStrategy()
{

  // Create internal vtk classes
  this->RepulsionArray = vtkFloatArray::New();
  this->AttractionArray = vtkFloatArray::New();

  this->RandomSeed = 123;
  this->IterationsPerLayout = 200;
  this->InitialTemperature = 1;
  this->CoolDownRate = 50.0;
  this->LayoutComplete = 0;
  this->EdgeWeightField = 0;
  this->SetEdgeWeightField("weight");
  this->RestDistance = 0;
  this->Jitter = true;
  this->MaxNumberOfIterations = 200;
  this->EdgeArray = 0;
}

// ----------------------------------------------------------------------

vtkSimple2DLayoutStrategy::~vtkSimple2DLayoutStrategy()
{
  this->SetEdgeWeightField(0);
  this->RepulsionArray->Delete();
  this->AttractionArray->Delete();
  delete [] this->EdgeArray;
  this->EdgeArray = NULL;
}

// ----------------------------------------------------------------------

// Set the graph that will be laid out
void vtkSimple2DLayoutStrategy::Initialize()
{
  vtkMath::RandomSeed(this->RandomSeed);

  // Set up some quick access variables
  vtkPoints *pts = this->Graph->GetPoints();
  vtkIdType numVertices = this->Graph->GetNumberOfVertices();
  vtkIdType numEdges = this->Graph->GetNumberOfEdges();

  // Make sure output point type is float
  if (pts->GetData()->GetDataType() != VTK_FLOAT)
    {
    vtkErrorMacro("Layout strategy expects to have points of type float");
    this->LayoutComplete = 1;
    return;
    }

  // Get a quick pointer to the point data
  vtkFloatArray *array = vtkFloatArray::SafeDownCast(pts->GetData());
  float *rawPointData = array->GetPointer(0);

  // Avoid divide by zero
  float div = 1;
  if (numVertices > 0)
    {
    div = static_cast<float>(numVertices);
    }

  // The optimal distance between vertices.
  if (this->RestDistance == 0)
    {
    this->RestDistance = 1.0/div;
    }

  // Set up array to store repulsion values
  this->RepulsionArray->SetNumberOfComponents(3);
  this->RepulsionArray->SetNumberOfTuples(numVertices);
  for (vtkIdType i=0; i<numVertices*3; ++i)
    {
    this->RepulsionArray->SetValue(i, 0);
    }

  // Set up array to store attraction values
  this->AttractionArray->SetNumberOfComponents(3);
  this->AttractionArray->SetNumberOfTuples(numVertices);
  for (vtkIdType i=0; i<numVertices*3; ++i)
    {
    this->AttractionArray->SetValue(i, 0);
    }

  // Put the edge data into compact, fast access edge data structure
  delete [] this->EdgeArray;
  this->EdgeArray = new vtkLayoutEdge[numEdges];

  // If jitter then do it now at initialization
  if (Jitter)
    {

    // Jitter x and y, skip z
    for (vtkIdType i=0; i<numVertices*3; i+=3)
      {
      rawPointData[i] += this->RestDistance*(vtkMath::Random() - .5);
      rawPointData[i+1] += this->RestDistance*(vtkMath::Random() - .5);
      }
    }

  // Get the weight array
  vtkDataArray* weightArray = NULL;
  double weight, maxWeight = 1;
  if (this->WeightEdges && this->EdgeWeightField != NULL)
    {
    weightArray = vtkDataArray::SafeDownCast(this->Graph->GetEdgeData()->GetAbstractArray(this->EdgeWeightField));
    if (weightArray != NULL)
      {
      for (vtkIdType w = 0; w < weightArray->GetNumberOfTuples(); w++)
        {
        weight = weightArray->GetTuple1(w);
        if (weight > maxWeight)
          {
          maxWeight = weight;
          }
        }
      }
    }

  // Load up the edge data structures
  vtkSmartPointer<vtkEdgeListIterator> edges =
    vtkSmartPointer<vtkEdgeListIterator>::New();
  this->Graph->GetEdges(edges);
  while (edges->HasNext())
    {
    vtkEdgeType e = edges->Next();
    this->EdgeArray[e.Id].from = e.Source;
    this->EdgeArray[e.Id].to = e.Target;
    if (weightArray != NULL)
      {
      weight = weightArray->GetTuple1(e.Id);
      this->EdgeArray[e.Id].weight = weight / maxWeight;
      }
    else
      {
      this->EdgeArray[e.Id].weight = 1.0;
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
  vtkIdType numVertices = this->Graph->GetNumberOfVertices();
  vtkIdType numEdges = this->Graph->GetNumberOfEdges();

  // Get a quick pointer to the point data
  vtkFloatArray *array = vtkFloatArray::SafeDownCast(pts->GetData());
  float *rawPointData = array->GetPointer(0);

  // This is the mega, uber, triple inner loop
  // ye of weak hearts, tread no further!
  float delta[]={0,0,0};
  float disSquared;
  float attractValue;
  float epsilon = 1e-5;
  vtkIdType pointIndex1=0;
  vtkIdType pointIndex2=0;
  for(int i = 0; i < this->IterationsPerLayout; ++i)
    {

    // Initialize the repulsion and attraction arrays
    for (vtkIdType j=0; j<numVertices*3; ++j)
      {
      this->RepulsionArray->SetValue(j, 0);
      }

    // Set up array to store attraction values
    for (vtkIdType j=0; j<numVertices*3; ++j)
      {
      this->AttractionArray->SetValue(j, 0);
      }

    // Calculate the repulsive forces
    float *rawRepulseArray = this->RepulsionArray->GetPointer(0);
    for(vtkIdType j=0; j<numVertices; ++j)
      {
      pointIndex1 = j * 3;

      for(vtkIdType k=0; k<numVertices; ++k)
        {
        // Don't repulse against yourself :)
        if (k == j) continue;

        pointIndex2 = k * 3;

        delta[0] = rawPointData[pointIndex1] -
                  rawPointData[pointIndex2];
        delta[1] = rawPointData[pointIndex1+1] -
                  rawPointData[pointIndex2+1];
        disSquared = delta[0]*delta[0] + delta[1]*delta[1];
        // Avoid divide by zero
        disSquared += epsilon;
        rawRepulseArray[pointIndex1]   += delta[0]/disSquared;
        rawRepulseArray[pointIndex1+1] += delta[1]/disSquared;
        }
      }

    // Calculate the attractive forces
    float *rawAttractArray = this->AttractionArray->GetPointer(0);
    for (vtkIdType j=0; j<numEdges; ++j)
      {
      pointIndex1 = this->EdgeArray[j].to * 3;
      pointIndex2 = this->EdgeArray[j].from * 3;

      // No need to attract points to themselves
      if (pointIndex1 == pointIndex2) continue;

      delta[0] = rawPointData[pointIndex1] -
             rawPointData[pointIndex2];
      delta[1] = rawPointData[pointIndex1+1] -
              rawPointData[pointIndex2+1];
      disSquared = delta[0]*delta[0] + delta[1]*delta[1];

      // Perform weight adjustment
      attractValue = this->EdgeArray[j].weight*disSquared-this->RestDistance;

      rawAttractArray[pointIndex1]   -= delta[0] * attractValue;
      rawAttractArray[pointIndex1+1] -= delta[1] * attractValue;
      rawAttractArray[pointIndex2]   += delta[0] * attractValue;
      rawAttractArray[pointIndex2+1] += delta[1] * attractValue;
      }

    // Okay now set new positions based on replusion
    // and attraction 'forces'
    for(vtkIdType j=0; j<numVertices; ++j)
      {
      pointIndex1 = j * 3;

      // Get forces for this node
      float forceX = rawAttractArray[pointIndex1] + rawRepulseArray[pointIndex1];
      float forceY = rawAttractArray[pointIndex1+1] + rawRepulseArray[pointIndex1+1];

      // Forces can get extreme so limit them
      // Note: This is psuedo-normalization of the
      //       force vector, just to save some cycles

      // Avoid divide by zero
      float forceDiv = fabs(forceX) + fabs(forceY) + epsilon;
      float pNormalize = MIN(1, 1.0/forceDiv);
      pNormalize *= this->Temp;
      forceX *= pNormalize;
      forceY *= pNormalize;

      rawPointData[pointIndex1] += forceX;
      rawPointData[pointIndex1+1] += forceY;
      }

    // The point coordinates have been modified
    this->Graph->GetPoints()->Modified();

    // Reduce temperature as layout approaches a better configuration.
    this->Temp = CoolDown(this->Temp, this->CoolDownRate);

    // Announce progress
    double progress = (i+this->TotalIterations) /
                      static_cast<double>(this->MaxNumberOfIterations);
    this->InvokeEvent(vtkCommand::ProgressEvent, static_cast<void *>(&progress));

   } // End loop this->IterationsPerLayout


  // Check for completion of layout
  this->TotalIterations += this->IterationsPerLayout;
  if (this->TotalIterations >= this->MaxNumberOfIterations)
    {
    // I'm done
    this->LayoutComplete = 1;
    }

  // Mark the points as modified
  this->Graph->GetPoints()->Modified();
}

void vtkSimple2DLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "RandomSeed: " << this->RandomSeed << endl;
  os << indent << "InitialTemperature: " << this->InitialTemperature << endl;
  os << indent << "MaxNumberOfIterations: " << this->MaxNumberOfIterations << endl;
  os << indent << "IterationsPerLayout: " << this->IterationsPerLayout << endl;
  os << indent << "CoolDownRate: " << this->CoolDownRate << endl;
  os << indent << "Jitter: " << (this->Jitter ? "True" : "False") << endl;
  os << indent << "RestDistance: " << this->RestDistance << endl;
}
