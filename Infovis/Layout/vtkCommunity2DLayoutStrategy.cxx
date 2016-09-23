/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCommunity2DLayoutStrategy.cxx

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

#include "vtkCommunity2DLayoutStrategy.h"

#include "vtkBitArray.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkEdgeListIterator.h"
#include "vtkFastSplatter.h"
#include "vtkFloatArray.h"
#include "vtkGraph.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIdTypeArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkTree.h"

vtkStandardNewMacro(vtkCommunity2DLayoutStrategy);

// Cool-down function.
static inline float CoolDown(float t, float r)
{
  return t-(t/r);
}

// ----------------------------------------------------------------------

vtkCommunity2DLayoutStrategy::vtkCommunity2DLayoutStrategy()
{

  // Create internal vtk classes
  this->DensityGrid = vtkSmartPointer<vtkFastSplatter>::New();
  this->SplatImage = vtkSmartPointer<vtkImageData>::New();
  this->RepulsionArray = vtkSmartPointer<vtkFloatArray>::New();
  this->AttractionArray = vtkSmartPointer<vtkFloatArray>::New();

  this->RandomSeed = 123;
  this->MaxNumberOfIterations = 200;
  this->IterationsPerLayout = 200;
  this->InitialTemperature = 5;
  this->CoolDownRate = 50.0;
  this->LayoutComplete = 0;
  this->EdgeWeightField = 0;
  this->SetEdgeWeightField("weight");
  this->RestDistance = 0;
  this->EdgeArray = 0;
  this->CommunityArrayName = 0;
  this->SetCommunityArrayName("community");
  this->CommunityStrength = 1.0;
}

// ----------------------------------------------------------------------

vtkCommunity2DLayoutStrategy::~vtkCommunity2DLayoutStrategy()
{
  this->SetEdgeWeightField(0);
  this->SetCommunityArrayName(0);
  delete [] this->EdgeArray;
}


// Helper functions
void vtkCommunity2DLayoutStrategy::GenerateCircularSplat(vtkImageData *splat, int x, int y)
{
  splat->SetDimensions(x, y, 1);
  splat->AllocateScalars(VTK_FLOAT, 1);

  const int *dimensions = splat->GetDimensions();

  // Circular splat: 1 in the middle and 0 at the corners and sides
  for (int row = 0; row < dimensions[1]; ++row)
  {
    for (int col = 0; col < dimensions[0]; ++col)
    {
      float splatValue;

      // coordinates will range from -1 to 1
      float xCoord = (col - dimensions[0]/2.0) / (dimensions[0]/2.0);
      float yCoord = (row - dimensions[1]/2.0) / (dimensions[1]/2.0);

      float radius = sqrt(xCoord*xCoord + yCoord*yCoord);
      if ((1 - radius) > 0)
      {
        splatValue = 1-radius;
      }
      else
      {
        splatValue = 0;
      }

      // Set value
      splat->SetScalarComponentFromFloat(col,row,0,0,splatValue);
    }
  }
}

void vtkCommunity2DLayoutStrategy::GenerateGaussianSplat(vtkImageData *splat, int x, int y)
{
  splat->SetDimensions(x, y, 1);
  splat->AllocateScalars(VTK_FLOAT, 1);

  const int *dimensions = splat->GetDimensions();

  // Gaussian splat
  float falloff = 10; // fast falloff
  float e= 2.71828182845904;

  for (int row = 0; row < dimensions[1]; ++row)
  {
    for (int col = 0; col < dimensions[0]; ++col)
    {
      float splatValue;

      // coordinates will range from -1 to 1
      float xCoord = (col - dimensions[0]/2.0) / (dimensions[0]/2.0);
      float yCoord = (row - dimensions[1]/2.0) / (dimensions[1]/2.0);

      splatValue = pow(e,-((xCoord*xCoord + yCoord*yCoord) * falloff));

      // Set value
      splat->SetScalarComponentFromFloat(col,row,0,0,splatValue);
    }
  }
}

// ----------------------------------------------------------------------
// Set the graph that will be laid out
void vtkCommunity2DLayoutStrategy::Initialize()
{
  vtkMath::RandomSeed(this->RandomSeed);

  // Set up some quick access variables
  vtkPoints* pts = this->Graph->GetPoints();
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
  vtkFloatArray *array = vtkArrayDownCast<vtkFloatArray>(pts->GetData());
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
    this->RestDistance = sqrt(1.0 / div);
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

  // Jitter x and y, skip z
  for (vtkIdType i=0; i<numVertices*3; i+=3)
  {
    rawPointData[i] += this->RestDistance*(vtkMath::Random() - .5);
    rawPointData[i+1] += this->RestDistance*(vtkMath::Random() - .5);
  }

  // Get the weight array
  vtkDataArray* weightArray = NULL;
  double weight, maxWeight = 1;
  if (this->WeightEdges && this->EdgeWeightField != NULL)
  {
    weightArray = vtkArrayDownCast<vtkDataArray>(this->Graph->GetEdgeData()->GetAbstractArray(this->EdgeWeightField));
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

  // Set up the image splatter
  this->GenerateGaussianSplat(this->SplatImage, 41, 41);
  this->DensityGrid->SetInputData(1, this->SplatImage);
  this->DensityGrid->SetOutputDimensions(100, 100, 1);

}

// ----------------------------------------------------------------------

// Simple graph layout method
void vtkCommunity2DLayoutStrategy::Layout()
{
  // Do I have a graph to layout
  if (this->Graph == NULL)
  {
    vtkErrorMacro("Graph Layout called with Graph==NULL, call SetGraph(g) first");
    this->LayoutComplete = 1;
    return;
  }

  // Set my graph as input into the density grid
  this->DensityGrid->SetInputData(this->Graph);

  // Set up some variables
  vtkPoints* pts = this->Graph->GetPoints();
  vtkIdType numVertices = this->Graph->GetNumberOfVertices();
  vtkIdType numEdges = this->Graph->GetNumberOfEdges();

  // Get a quick pointer to the community array
  vtkDataArray *community =
    this->Graph->GetVertexData()->GetArray(this->CommunityArrayName);
  if (community == NULL)
  {
    vtkWarningMacro("vtkCommunity2DLayoutStrategy did not find a \"community\" array." <<
                    "\n so the layout will not pull communities together like it should");
  }

  // Get a quick pointer to the point data
  vtkFloatArray *array = vtkArrayDownCast<vtkFloatArray>(pts->GetData());
  float *rawPointData = array->GetPointer(0);

  // This is the mega, uber, triple inner loop
  // ye of weak hearts, tread no further!
  float delta[]={0,0,0};
  float disSquared;
  float attractValue;
  float epsilon = 1e-5;
  vtkIdType rawSourceIndex=0;
  vtkIdType rawTargetIndex=0;
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

    // Compute bounds of graph going into the density grid
    double bounds[6], paddedBounds[6];
    this->Graph->ComputeBounds();
    this->Graph->GetBounds(bounds);

    // Give bounds a 10% padding
    paddedBounds[0] = bounds[0] - (bounds[1]-bounds[0])*.1;
    paddedBounds[1] = bounds[1] + (bounds[1]-bounds[0])*.1;
    paddedBounds[2] = bounds[2] - (bounds[3]-bounds[2])*.1;
    paddedBounds[3] = bounds[3] + (bounds[3]-bounds[2])*.1;
    paddedBounds[4] = paddedBounds[5] = 0;

    // Update the density grid
    this->DensityGrid->SetModelBounds(paddedBounds);
    this->DensityGrid->Update();

    // Sanity check scalar type
    if (this->DensityGrid->GetOutput()->GetScalarType() != VTK_FLOAT)
    {
      vtkErrorMacro("DensityGrid expected to be of type float");
      return;
    }

    // Get the array handle
    float *densityArray = static_cast<float*>
      (this->DensityGrid->GetOutput()->GetScalarPointer());

    // Get the dimensions of the density grid
    int dims[3];
    this->DensityGrid->GetOutputDimensions(dims);


    // Calculate the repulsive forces
    float *rawRepulseArray = this->RepulsionArray->GetPointer(0);
    for(vtkIdType j=0; j<numVertices; ++j)
    {
      rawSourceIndex = j * 3;

      // Compute indices into the density grid
      int indexX = static_cast<int>(
                   (rawPointData[rawSourceIndex]-paddedBounds[0]) /
                   (paddedBounds[1]-paddedBounds[0]) * dims[0] + .5);
      int indexY = static_cast<int>(
                   (rawPointData[rawSourceIndex+1]-paddedBounds[2]) /
                   (paddedBounds[3]-paddedBounds[2]) * dims[1] + .5);

      // Look up the gradient density within the density grid
      float x1 = densityArray[indexY * dims[0] + indexX-1];
      float x2 = densityArray[indexY * dims[0] + indexX+1];
      float y1 = densityArray[(indexY-1) * dims[0] + indexX];
      float y2 = densityArray[(indexY+1) * dims[0] + indexX];

      rawRepulseArray[rawSourceIndex]   = (x1-x2); // Push away from higher
      rawRepulseArray[rawSourceIndex+1] = (y1-y2);
    }

    // Calculate the attractive forces
    float *rawAttractArray = this->AttractionArray->GetPointer(0);
    for (vtkIdType j=0; j<numEdges; ++j)
    {
      rawSourceIndex = this->EdgeArray[j].from * 3;
      rawTargetIndex = this->EdgeArray[j].to * 3;

      // No need to attract points to themselves
      if (rawSourceIndex == rawTargetIndex) continue;

      delta[0] = rawPointData[rawSourceIndex] -
             rawPointData[rawTargetIndex];
      delta[1] = rawPointData[rawSourceIndex+1] -
              rawPointData[rawTargetIndex+1];
      disSquared = delta[0]*delta[0] + delta[1]*delta[1];

      // Compute a bunch of parameters used below
      int sourceIndex = this->EdgeArray[j].from;
      int targetIndex = this->EdgeArray[j].to;

      // Clustering: Get close to other nodes that are
      // part of your community
      float communityWeight = 1;
      if (community)
      {
        int sourceComm = static_cast<int> (community->GetTuple1(sourceIndex));
        int targetComm = static_cast<int> (community->GetTuple1(targetIndex));

        // Often -1 is used for no/unspecified community
        // if either node is marked as such then just skip
        if ((sourceComm == -1) || (targetComm == -1))
        {
          continue;
        }

        // If the source and target are the same
        // then increase the weight between them
        if (sourceComm == targetComm)
        {
          communityWeight = 1 + 10 * this->CommunityStrength;
        }

        // If source and target are different
        // then decrease the weight between them
        else
        {
          communityWeight = 1.1 - this->CommunityStrength;
        }
      } // if community

      // Perform weight adjustment
      attractValue = this->EdgeArray[j].weight*communityWeight*disSquared -
                     this->RestDistance;

      rawAttractArray[rawSourceIndex]   -= delta[0] * attractValue;
      rawAttractArray[rawSourceIndex+1] -= delta[1] * attractValue;
      rawAttractArray[rawTargetIndex]   += delta[0] * attractValue;
      rawAttractArray[rawTargetIndex+1] += delta[1] * attractValue;
    } // for each edge

    // Okay now set new positions based on replusion
    // and attraction 'forces'
    for(vtkIdType j=0; j<numVertices; ++j)
    {
      rawSourceIndex = j * 3;

      // Get forces for this node
      float forceX = rawAttractArray[rawSourceIndex] + rawRepulseArray[rawSourceIndex];
      float forceY = rawAttractArray[rawSourceIndex+1] + rawRepulseArray[rawSourceIndex+1];

      // Forces can get extreme so limit them
      // Note: This is psuedo-normalization of the
      //       force vector, just to save some cycles

      // Avoid divide by zero
      float forceDiv = fabs(forceX) + fabs(forceY) + epsilon;
      float pNormalize = vtkMath::Min(1.0f, 1.0f/forceDiv);
      pNormalize *= this->Temp;
      forceX *= pNormalize;
      forceY *= pNormalize;

      rawPointData[rawSourceIndex] += forceX;
      rawPointData[rawSourceIndex+1] += forceY;
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

    // Make sure no vertex is on top of another vertex
    this->ResolveCoincidentVertices();

    // I'm done
    this->LayoutComplete = 1;
  }

  // Mark points as modified
  this->Graph->GetPoints()->Modified();
}

void vtkCommunity2DLayoutStrategy::ResolveCoincidentVertices()
{

  // Note: This algorithm is stupid but was easy to implement
  //       please change or improve if you'd like. :)

  // Basically see if the vertices are within a tolerance
  // of each other (do they fall into the same bucket).
  // If the vertices do fall into the same bucket give them
  // some random displacements to resolve coincident and
  // repeat until we have no coincident vertices

  // Get the number of vertices in the graph datastructure
  vtkIdType numVertices = this->Graph->GetNumberOfVertices();

  // Get a quick pointer to the point data
  vtkPoints* pts = this->Graph->GetPoints();
  vtkFloatArray *array = vtkArrayDownCast<vtkFloatArray>(pts->GetData());
  float *rawPointData = array->GetPointer(0);

  // Place the vertices into a giant grid (100xNumVertices)
  // and see if you have any collisions
  vtkBitArray *giantGrid = vtkBitArray::New();
  vtkIdType xDim =
    static_cast<int>(sqrt(static_cast<double>(numVertices)) * 10);
  vtkIdType yDim =
    static_cast<int>(sqrt(static_cast<double>(numVertices)) * 10);
  vtkIdType gridSize = xDim * yDim;
  giantGrid->SetNumberOfValues(gridSize);

  // Initialize array to zeros
  for(vtkIdType i=0; i<gridSize; ++i)
  {
    giantGrid->SetValue(i, 0);
  }

  double bounds[6], paddedBounds[6];
  this->Graph->GetBounds(bounds);

  // Give bounds a 10% padding
  paddedBounds[0] = bounds[0] - (bounds[1]-bounds[0])*.1;
  paddedBounds[1] = bounds[1] + (bounds[1]-bounds[0])*.1;
  paddedBounds[2] = bounds[2] - (bounds[3]-bounds[2])*.1;
  paddedBounds[3] = bounds[3] + (bounds[3]-bounds[2])*.1;
  paddedBounds[4] = paddedBounds[5] = 0;

  int totalCollisionOps = 0;

  for(vtkIdType i=0; i<numVertices; ++i)
  {
    int rawIndex = i * 3;

    // Compute indices into the buckets
    int indexX = static_cast<int>(
                 (rawPointData[rawIndex]-paddedBounds[0]) /
                 (paddedBounds[1]-paddedBounds[0]) * (xDim-1) + .5);
    int indexY = static_cast<int>(
                 (rawPointData[rawIndex+1]-paddedBounds[2]) /
                 (paddedBounds[3]-paddedBounds[2]) * (yDim-1) + .5);

    // See if you collide with another vertex
    if (giantGrid->GetValue(indexX + indexY*xDim))
    {

      // Oh my... try to get yourself out of this
      // by randomly jumping to a place that doesn't
      // have another vertex
      bool collision = true;
      float jumpDistance = 5.0*(paddedBounds[1]-paddedBounds[0])/xDim; // 2.5 grid spaces max
      int collisionOps = 0;

      // You get 10 trys and then we have to punt
      while (collision && (collisionOps < 10))
      {
        collisionOps++;

        // Move
        rawPointData[rawIndex] += jumpDistance*(vtkMath::Random() - .5);
        rawPointData[rawIndex+1] += jumpDistance*(vtkMath::Random() - .5);

        // Test
        indexX = static_cast<int>(
                 (rawPointData[rawIndex]-paddedBounds[0]) /
                 (paddedBounds[1]-paddedBounds[0]) * (xDim-1) + .5);
        indexY = static_cast<int>(
                     (rawPointData[rawIndex+1]-paddedBounds[2]) /
                     (paddedBounds[3]-paddedBounds[2]) * (yDim-1) + .5);
        if (!giantGrid->GetValue(indexX + indexY*xDim))
        {
          collision = false; // yea
        }
      } // while
        totalCollisionOps += collisionOps;
    } // if collide

    // Put into a bucket
    giantGrid->SetValue(indexX + indexY*xDim, 1);
  }

  // Delete giantGrid
  giantGrid->Initialize();
  giantGrid->Delete();

  // Report number of collision operations just for sanity check
  // vtkWarningMacro("Collision Ops: " << totalCollisionOps);
}

void vtkCommunity2DLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "RandomSeed: " << this->RandomSeed << endl;
  os << indent << "MaxNumberOfIterations: " << this->MaxNumberOfIterations << endl;
  os << indent << "IterationsPerLayout: " << this->IterationsPerLayout << endl;
  os << indent << "InitialTemperature: " << this->InitialTemperature << endl;
  os << indent << "CoolDownRate: " << this->CoolDownRate << endl;
  os << indent << "RestDistance: " << this->RestDistance << endl;
  os << indent << "EdgeWeightField: " << (this->EdgeWeightField ? this->EdgeWeightField : "(none)") << endl;
  os << indent << "CommunityArrayName: " << (this->CommunityArrayName ? this->CommunityArrayName : "(none)") << endl;
  os << indent << "CommunityStrength: " << this->CommunityStrength << endl;

}
