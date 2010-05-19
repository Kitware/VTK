/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAttributeClustering2DLayoutStrategy.cxx

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

#include "vtkAttributeClustering2DLayoutStrategy.h"

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
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkTree.h"

#include <vtksys/stl/vector>

class vtkAttributeClustering2DLayoutStrategy::Internals
{
public:

  // An edge consists of two vertices joined together.
  // This struct acts as a "pointer" to those two vertices.
  typedef struct 
  {
    vtkIdType from;
    vtkIdType to;
    int dead_edge; // I'm making this an int so that the edge array is
                   // word boundary aligned... but I'm not sure what
                   // really happens in these days of magical compilers
  } vtkLayoutEdge;
  
  vtksys_stl::vector<vtkLayoutEdge> Edges;
};


vtkStandardNewMacro(vtkAttributeClustering2DLayoutStrategy);

// This is just a convenient macro for smart pointers
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()
  
#ifndef MIN
#define MIN(x, y)       ((x) < (y) ? (x) : (y))
#endif
  
  
// Cool-down function.
static inline float CoolDown(float t, float r) 
{  
  return t-(t/r);
}

// ----------------------------------------------------------------------

vtkAttributeClustering2DLayoutStrategy::vtkAttributeClustering2DLayoutStrategy() :
  Implementation(new Internals())
{

  // Create internal vtk classes
  this->DensityGrid = vtkSmartPointer<vtkFastSplatter>::New();
  this->SplatImage = vtkSmartPointer<vtkImageData>::New();
  this->RepulsionArray = vtkSmartPointer<vtkFloatArray>::New();
  this->AttractionArray = vtkSmartPointer<vtkFloatArray>::New();
  this->EdgeCountArray = vtkSmartPointer<vtkIntArray>::New();
    
  this->RandomSeed = 123;
  this->MaxNumberOfIterations = 200;
  this->IterationsPerLayout = 200;
  this->InitialTemperature = 5;
  this->CoolDownRate = 50.0;
  this->LayoutComplete = 0;
  this->EdgeWeightField = 0;
  this->SetEdgeWeightField("weight");
  this->RestDistance = 0;
  this->CuttingThreshold=0;
  this->VertexAttribute = NULL;
}

// ----------------------------------------------------------------------

vtkAttributeClustering2DLayoutStrategy::~vtkAttributeClustering2DLayoutStrategy()
{
  this->SetEdgeWeightField(0);
  this->SetVertexAttribute(0);
  delete this->Implementation;
}

void vtkAttributeClustering2DLayoutStrategy::SetVertexAttribute(const char* att)
{
  // This method is a cut and paste of vtkSetStringMacro
  // except for the call to Initialize at the end :)
  if ( this->VertexAttribute == NULL && att == NULL) { return;}
  if ( this->VertexAttribute && att && (!strcmp(this->VertexAttribute,att))) { return;}
  if (this->VertexAttribute) { delete [] this->VertexAttribute; }
  if (att)
    {
    size_t n = strlen(att) + 1;
    char *cp1 =  new char[n];
    const char *cp2 = (att);
    this->VertexAttribute = cp1;
    do { *cp1++ = *cp2++; } while ( --n );
    }
   else
    {
    this->VertexAttribute = NULL;
    }
  
  this->Modified();

  if(this->Graph)
    {
    this->Initialize();
    }
}


// Helper functions
void vtkAttributeClustering2DLayoutStrategy::GenerateCircularSplat(vtkImageData *splat, int x, int y)
{
  splat->SetScalarTypeToFloat();
  splat->SetNumberOfScalarComponents(1);
  splat->SetDimensions(x, y, 1);
  splat->AllocateScalars();
  
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

void vtkAttributeClustering2DLayoutStrategy::GenerateGaussianSplat(vtkImageData *splat, int x, int y)
{
  splat->SetScalarTypeToFloat();
  splat->SetNumberOfScalarComponents(1);
  splat->SetDimensions(x, y, 1);
  splat->AllocateScalars();
  
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
void vtkAttributeClustering2DLayoutStrategy::Initialize()
{
  if(!this->VertexAttribute)
    {
    vtkErrorMacro("Layout strategy requires VertexAttribute to be set");
    this->LayoutComplete = 1;
    return;
    }

  vtkMath::RandomSeed(this->RandomSeed);

  // Set up some quick access variables
  vtkPoints* pts = this->Graph->GetPoints();
  vtkIdType numVertices = this->Graph->GetNumberOfVertices();
  
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
    
  // Jitter x and y, skip z
  for (vtkIdType i=0; i<numVertices*3; i+=3)
    {
    rawPointData[i] += this->RestDistance*(vtkMath::Random() - .5);
    rawPointData[i+1] += this->RestDistance*(vtkMath::Random() - .5);
    }

  this->Implementation->Edges.clear();

  // Given the vertex attribute provided, 
  // construct phantom edges between vertices with matching values
  vtkAbstractArray* vertexArr = this->Graph->GetVertexData()->GetAbstractArray(this->VertexAttribute);
  vtkSmartPointer<vtkIdList> ids = vtkSmartPointer<vtkIdList>::New();
  this->EdgeCountArray->SetNumberOfComponents(1);
  this->EdgeCountArray->SetNumberOfTuples(numVertices);
  this->EdgeCountArray->FillComponent(0,0);
  for(int i=0; i<vertexArr->GetNumberOfTuples(); ++i)
    {
    vtkVariant v_source = vertexArr->GetVariantValue(i);
    for(int k=i; k<vertexArr->GetNumberOfTuples(); ++k)
      {
      vtkVariant v_target = vertexArr->GetVariantValue(k);
      if(v_source == v_target)
        {
        Internals::vtkLayoutEdge e;
        e.from = i;
        e.to = k;
        e.dead_edge = 0;
        this->Implementation->Edges.push_back(e);
        // Store the number of edges associated with each vertex  
        this->EdgeCountArray->SetValue(i, this->EdgeCountArray->GetValue(i) + 1);
        }
      }
    }
  

  // Set some vars
  this->TotalIterations = 0;
  this->LayoutComplete = 0;
  this->Temp = this->InitialTemperature;
  this->CuttingThreshold = 10000*this->RestDistance; // Max cut length
  
  // Set up the image splatter
  this->GenerateGaussianSplat(this->SplatImage, 41, 41);
  this->DensityGrid->SetInput(1, this->SplatImage);
  this->DensityGrid->SetOutputDimensions(100, 100, 1);

}

// ----------------------------------------------------------------------

// Simple graph layout method
void vtkAttributeClustering2DLayoutStrategy::Layout()
{
  // Do I have a graph to layout?
  if (this->Graph == NULL)
    {
    vtkErrorMacro("Graph Layout called with Graph==NULL, call SetGraph(g) first");
    this->LayoutComplete = 1;
    return;
    }
    
  // Is the layout already considered complete?
  if (this->IsLayoutComplete())
  {
      vtkErrorMacro("Graph Layout already considered complete");
      return;
  }

  // Set my graph as input into the density grid
  this->DensityGrid->SetInput(this->Graph);
  
  // Set up some variables
  vtkPoints* pts = this->Graph->GetPoints();
  vtkIdType numVertices = this->Graph->GetNumberOfVertices();
  
  // Get a quick pointer to the point data
  vtkFloatArray *array = vtkFloatArray::SafeDownCast(pts->GetData());
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
    vtksys_stl::vector<Internals::vtkLayoutEdge>::iterator iter;
    for (iter = this->Implementation->Edges.begin(); iter != this->Implementation->Edges.end(); ++iter)
      {
      // Check for dead edge
      if ((*iter).dead_edge)
        {
        continue;
        }
        
      rawSourceIndex = (*iter).from * 3;
      rawTargetIndex = (*iter).to * 3;
      
      // No need to attract points to themselves
      if (rawSourceIndex == rawTargetIndex) continue;
      
      delta[0] = rawPointData[rawSourceIndex] - 
             rawPointData[rawTargetIndex];
      delta[1] = rawPointData[rawSourceIndex+1] - 
              rawPointData[rawTargetIndex+1];
      disSquared = delta[0]*delta[0] + delta[1]*delta[1];
 
      // Compute a bunch of parameters used below
      int sourceIndex = (*iter).from;
      int targetIndex = (*iter).to;
      int numSourceEdges = this->EdgeCountArray->GetValue(sourceIndex);
      int numTargetEdges = this->EdgeCountArray->GetValue(targetIndex);

      // Perform weight adjustment
      attractValue = 1.0*disSquared-this->RestDistance;
      rawAttractArray[rawSourceIndex]   -= delta[0] * attractValue;
      rawAttractArray[rawSourceIndex+1] -= delta[1] * attractValue;
      rawAttractArray[rawTargetIndex]   += delta[0] * attractValue;
      rawAttractArray[rawTargetIndex+1] += delta[1] * attractValue;

      // This logic forces edge lengths to be short
      if (numSourceEdges < 10)
        {
        rawPointData[rawSourceIndex]   -= delta[0]*.45;
        rawPointData[rawSourceIndex+1] -= delta[1]*.45;
        }
      else if (numTargetEdges < 10)
        {
        rawPointData[rawTargetIndex]   += delta[0]*.45;
        rawPointData[rawTargetIndex+1] += delta[1]*.45;
        }

      // Cutting edges for clustering
      if (disSquared > this->CuttingThreshold)
        {
        if (((numSourceEdges > 1) && (numTargetEdges > 1)))
          {
          (*iter).dead_edge = 1;
          this->EdgeCountArray->SetValue(sourceIndex, numSourceEdges-1);
          this->EdgeCountArray->SetValue(targetIndex, numTargetEdges-1);
          }
        }
      }
      
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
      float pNormalize = MIN(1, 1.0/forceDiv);
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
    
    // Adjust cutting
    float maxCutLength = 10000*this->RestDistance;
    float minCutLength = 100*this->RestDistance;
    this->CuttingThreshold = maxCutLength*(1-progress)*(1-progress) + minCutLength;

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

void vtkAttributeClustering2DLayoutStrategy::ResolveCoincidentVertices()
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
  vtkFloatArray *array = vtkFloatArray::SafeDownCast(pts->GetData());
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

void vtkAttributeClustering2DLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "RandomSeed: " << this->RandomSeed << endl;
  os << indent << "MaxNumberOfIterations: " << this->MaxNumberOfIterations << endl;
  os << indent << "IterationsPerLayout: " << this->IterationsPerLayout << endl;
  os << indent << "InitialTemperature: " << this->InitialTemperature << endl;
  os << indent << "CoolDownRate: " << this->CoolDownRate << endl;
  os << indent << "RestDistance: " << this->RestDistance << endl;
  os << indent << "CuttingThreshold: " << this->CuttingThreshold << endl;
  os << indent << "EdgeWeightField: " << (this->EdgeWeightField ? this->EdgeWeightField : "(none)") << endl; 
  os << indent << "VertexAttribute: " << (this->VertexAttribute ? this->VertexAttribute : "(none)") << endl; 
}
