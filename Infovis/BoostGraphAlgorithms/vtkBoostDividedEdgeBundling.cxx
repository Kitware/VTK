/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoostDividedEdgeBundling.cxx

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
#include "vtkBoostDividedEdgeBundling.h"

#include "vtkBoostGraphAdapter.h"
#include "vtkDataSetAttributes.h"
#include "vtkDirectedGraph.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkVectorOperators.h"

#include <boost/property_map/property_map.hpp>
#include <boost/graph/johnson_all_pairs_shortest.hpp>
#include <algorithm>

vtkStandardNewMacro(vtkBoostDividedEdgeBundling);

vtkBoostDividedEdgeBundling::vtkBoostDividedEdgeBundling()
{
}

class vtkBundlingMetadata
{
public:
  vtkBundlingMetadata(vtkBoostDividedEdgeBundling *alg, vtkDirectedGraph *g)
    : Outer(alg), Graph(g)
  {
    this->Nodes = reinterpret_cast<vtkVector3f*>(
      vtkArrayDownCast<vtkFloatArray>(g->GetPoints()->GetData())->GetPointer(0));
    this->Edges.resize(g->GetNumberOfEdges());
    for (vtkIdType e = 0; e < g->GetNumberOfEdges(); ++e)
    {
      this->Edges[e] = std::make_pair(g->GetSourceVertex(e), g->GetTargetVertex(e));
    }
    this->VelocityDamping = 0.1f;
    this->EdgeCoulombConstant = 0.5f;
    //this->EdgeCoulombConstant = 50.0f;
    this->EdgeCoulombDecay = 35.0f;
    this->EdgeSpringConstant = 0.1f;
    //this->EdgeSpringConstant = 0.0005f;
    this->EdgeLaneWidth = 25.0f;
    this->UseNewForce = true;
  }

  void ProjectOnto(vtkIdType e1, vtkIdType e2, vtkVector3f& s, vtkVector3f& t);
  void NormalizeNodePositions();
  void DenormalizeNodePositions();
  void CalculateNodeDistances();
  float AngleCompatibility(vtkIdType e1, vtkIdType e2);
  float ScaleCompatibility(vtkIdType e1, vtkIdType e2);
  float PositionCompatibility(vtkIdType e1, vtkIdType e2);
  float VisibilityCompatibility(vtkIdType e1, vtkIdType e2);
  float ConnectivityCompatibility(vtkIdType e1, vtkIdType e2);
  void CalculateEdgeLengths();
  void CalculateEdgeCompatibilities();
  void InitializeEdgeMesh();
  void DoubleEdgeMeshResolution();
  void SimulateEdgeStep();
  void LayoutEdgePoints();
  void SmoothEdges();

  float SimulationStep;
  int CycleIterations;
  int MeshCount;
  float VelocityDamping;
  float EdgeCoulombConstant;
  float EdgeCoulombDecay;
  float EdgeSpringConstant;
  float EdgeLaneWidth;
  bool UseNewForce;
  vtkBoostDividedEdgeBundling *Outer;
  vtkDirectedGraph *Graph;
  vtkVector3f *Nodes;
  std::vector<std::pair<vtkIdType, vtkIdType> > Edges;
  std::vector<std::vector<float> > NodeDistances;
  std::vector<float> EdgeLengths;
  std::vector<std::vector<float> > EdgeCompatibilities;
  std::vector<std::vector<float> > EdgeDots;
  std::vector<std::vector<vtkVector3f> > EdgeMesh;
  std::vector<std::vector<vtkVector3f> > EdgeMeshVelocities;
  std::vector<std::vector<vtkVector3f> > EdgeMeshAccelerations;
  //std::vector<std::vector<float> > EdgeMeshGroupCounts;
  vtkVector2f XRange;
  vtkVector2f YRange;
  vtkVector2f ZRange;
  float Scale;
};

void vtkBundlingMetadata::NormalizeNodePositions()
{
  this->XRange = vtkVector2f(VTK_FLOAT_MAX, VTK_FLOAT_MIN);
  this->YRange = vtkVector2f(VTK_FLOAT_MAX, VTK_FLOAT_MIN);
  this->ZRange = vtkVector2f(VTK_FLOAT_MAX, VTK_FLOAT_MIN);
  for (vtkIdType i = 0; i < this->Graph->GetNumberOfVertices(); ++i)
  {
    vtkVector3f p = this->Nodes[i];
    this->XRange[0] = std::min(this->XRange[0], p[0]);
    this->XRange[1] = std::max(this->XRange[1], p[0]);
    this->YRange[0] = std::min(this->YRange[0], p[1]);
    this->YRange[1] = std::max(this->YRange[1], p[1]);
    this->ZRange[0] = std::min(this->ZRange[0], p[2]);
    this->ZRange[1] = std::max(this->ZRange[1], p[2]);
  }
  float dx = this->XRange[1] - this->XRange[0];
  float dy = this->YRange[1] - this->YRange[0];
  float dz = this->ZRange[1] - this->ZRange[0];
  this->Scale = std::max(dx, std::max(dy, dz));
  for (vtkIdType i = 0; i < this->Graph->GetNumberOfVertices(); ++i)
  {
    vtkVector3f p = this->Nodes[i];
    this->Nodes[i] = vtkVector3f(
      (p[0] - this->XRange[0])/this->Scale * 1000.0f,
      (p[1] - this->YRange[0])/this->Scale * 1000.0f,
      (p[2] - this->ZRange[0])/this->Scale * 1000.0f);
  }
}

void vtkBundlingMetadata::DenormalizeNodePositions()
{
  for (vtkIdType i = 0; i < this->Graph->GetNumberOfVertices(); ++i)
  {
    vtkVector3f p = this->Nodes[i];
    this->Nodes[i] = vtkVector3f(
      p[0] / 1000.0f * this->Scale + this->XRange[0],
      p[1] / 1000.0f * this->Scale + this->YRange[0],
      p[2] / 1000.0f * this->Scale + this->ZRange[0]);
  }
  for (vtkIdType i = 0; i < (int)this->EdgeMesh.size(); ++i)
  {
    for (vtkIdType j = 0; j < (int)this->EdgeMesh[i].size(); ++j)
    {
      vtkVector3f p = this->EdgeMesh[i][j];
      this->EdgeMesh[i][j] = vtkVector3f(
        p[0] / 1000.0f * this->Scale + this->XRange[0],
        p[1] / 1000.0f * this->Scale + this->YRange[0],
        p[2] / 1000.0f * this->Scale + this->ZRange[0]);
    }
  }
}

void vtkBundlingMetadata::CalculateNodeDistances()
{
  vtkIdType numVerts = this->Graph->GetNumberOfVertices();
  vtkIdType numEdges = this->Graph->GetNumberOfEdges();
  this->NodeDistances.resize(numVerts, std::vector<float>(numVerts, VTK_FLOAT_MAX));
  std::vector<float> weights(numVerts, 1.0f);
  vtkNew<vtkFloatArray> weightMap;
  weightMap->SetNumberOfTuples(numEdges);
  for (vtkIdType e = 0; e < numEdges; ++e)
  {
    weightMap->SetValue(e, 1.0f);
  }
  boost::vtkGraphEdgePropertyMapHelper<vtkFloatArray*> weightProp(weightMap.GetPointer());
  boost::johnson_all_pairs_shortest_paths(
    this->Graph, this->NodeDistances,
    boost::weight_map(weightProp));
}

float vtkBundlingMetadata::AngleCompatibility(vtkIdType e1, vtkIdType e2)
{
  if (this->EdgeLengths[e1] == 0.0f || this->EdgeLengths[e2] == 0.0f)
  {
    return 0.0f;
  }
  vtkVector3f s1 = this->Nodes[this->Edges[e1].first];
  vtkVector3f t1 = this->Nodes[this->Edges[e1].second];
  vtkVector3f s2 = this->Nodes[this->Edges[e2].first];
  vtkVector3f t2 = this->Nodes[this->Edges[e2].second];
  vtkVector3f p1 = s1 - t1;
  vtkVector3f p2 = s2 - t2;
  float compatibility = p1.Dot(p2) / (this->EdgeLengths[e1]*this->EdgeLengths[e2]);
  return fabs(compatibility);
}

float vtkBundlingMetadata::ScaleCompatibility(vtkIdType e1, vtkIdType e2)
{
  float len1 = this->EdgeLengths[e1];
  float len2 = this->EdgeLengths[e2];
  float average = (len1 + len2) / 2.0f;
  if (average == 0.0f)
  {
    return 0.0f;
  }
  return 2.0f / (average / std::min(len1, len2) + std::max(len1, len2) / average);
}

float vtkBundlingMetadata::PositionCompatibility(vtkIdType e1, vtkIdType e2)
{
  float len1 = this->EdgeLengths[e1];
  float len2 = this->EdgeLengths[e2];
  float average = (len1 + len2) / 2.0f;
  if (average == 0.0f)
  {
    return 0.0f;
  }
  vtkVector3f s1 = this->Nodes[this->Edges[e1].first];
  vtkVector3f t1 = this->Nodes[this->Edges[e1].second];
  vtkVector3f s2 = this->Nodes[this->Edges[e2].first];
  vtkVector3f t2 = this->Nodes[this->Edges[e2].second];
  vtkVector3f mid1 = 0.5*(s1 + t1);
  vtkVector3f mid2 = 0.5*(s2 + t2);
  return average / (average + (mid1 - mid2).Norm());
}

void vtkBundlingMetadata::ProjectOnto(vtkIdType e1, vtkIdType e2, vtkVector3f& s, vtkVector3f& t)
{
  vtkVector3f s1 = this->Nodes[this->Edges[e1].first];
  vtkVector3f t1 = this->Nodes[this->Edges[e1].second];
  vtkVector3f s2 = this->Nodes[this->Edges[e2].first];
  vtkVector3f t2 = this->Nodes[this->Edges[e2].second];
  vtkVector3f norm = t2 - s2;
  norm.Normalize();
  vtkVector3f toHead = s1 - s2;
  vtkVector3f toTail = t1 - s2;
  vtkVector3f headOnOther = norm * norm.Dot(toHead);
  vtkVector3f tailOnOther = norm * norm.Dot(toTail);
  s = s2 + headOnOther;
  t = s2 + tailOnOther;
}

float vtkBundlingMetadata::VisibilityCompatibility(vtkIdType e1, vtkIdType e2)
{
  vtkVector3f is;
  vtkVector3f it;
  vtkVector3f js;
  vtkVector3f jt;
  this->ProjectOnto(e1, e2, is, it);
  this->ProjectOnto(e2, e1, js, jt);
  float ilen = (is - it).Norm();
  float jlen = (js - jt).Norm();
  if (ilen == 0.0f || jlen == 0.0f)
  {
    return 0.0f;
  }
  vtkVector3f s1 = this->Nodes[this->Edges[e1].first];
  vtkVector3f t1 = this->Nodes[this->Edges[e1].second];
  vtkVector3f s2 = this->Nodes[this->Edges[e2].first];
  vtkVector3f t2 = this->Nodes[this->Edges[e2].second];
  vtkVector3f mid1 = 0.5*(s1 + t1);
  vtkVector3f mid2 = 0.5*(s2 + t2);
  vtkVector3f imid = 0.5*(is + it);
  vtkVector3f jmid = 0.5*(js + jt);
  float midQI = (mid2 - imid).Norm();
  float vpq = std::max(0.0f, 1.0f - (2.0f * midQI) / ilen);
  float midPJ = (mid1 - jmid).Norm();
  float vqp = std::max(0.0f, 1.0f - (2.0f * midPJ) / jlen);

  return std::min(vpq, vqp);
}

float vtkBundlingMetadata::ConnectivityCompatibility(vtkIdType e1, vtkIdType e2)
{
  vtkIdType s1 = this->Edges[e1].first;
  vtkIdType t1 = this->Edges[e1].second;
  vtkIdType s2 = this->Edges[e2].first;
  vtkIdType t2 = this->Edges[e2].second;
  if (s1 == s2 || s1 == t2 || t1 == s2 || t1 == t2)
  {
    return 1.0f;
  }
  float minPath = std::min(this->NodeDistances[s1][s2], std::min(this->NodeDistances[s1][t2],
    std::min(this->NodeDistances[t1][s2], this->NodeDistances[t1][t2])));
  return 1.0f / (minPath + 1.0f);
}

void vtkBundlingMetadata::CalculateEdgeLengths()
{
  vtkIdType numEdges = this->Graph->GetNumberOfEdges();
  this->EdgeLengths.resize(numEdges);
  for (vtkIdType e = 0; e < numEdges; ++e)
  {
    vtkVector3f s = this->Nodes[this->Edges[e].first];
    vtkVector3f t = this->Nodes[this->Edges[e].second];
    this->EdgeLengths[e] = (s - t).Norm();
  }
}

void vtkBundlingMetadata::CalculateEdgeCompatibilities()
{
  vtkIdType numEdges = this->Graph->GetNumberOfEdges();
  this->EdgeCompatibilities.resize(numEdges, std::vector<float>(numEdges, 1.0f));
  this->EdgeDots.resize(numEdges, std::vector<float>(numEdges, 1.0f));
  for (vtkIdType e1 = 0; e1 < numEdges; ++e1)
  {
    vtkVector3f s1 = this->Nodes[this->Edges[e1].first];
    vtkVector3f t1 = this->Nodes[this->Edges[e1].second];
    vtkVector3f r1 = s1 - t1;
    r1.Normalize();
    for (vtkIdType e2 = e1 + 1; e2 < numEdges; ++e2)
    {
      float compatibility = 1.0f;
      compatibility *= this->AngleCompatibility(e1, e2);
      compatibility *= this->ScaleCompatibility(e1, e2);
      compatibility *= this->PositionCompatibility(e1, e2);
      compatibility *= this->VisibilityCompatibility(e1, e2);
      compatibility *= this->ConnectivityCompatibility(e1, e2);
      this->EdgeCompatibilities[e1][e2] = compatibility;
      this->EdgeCompatibilities[e2][e1] = compatibility;

      vtkVector3f s2 = this->Nodes[this->Edges[e2].first];
      vtkVector3f t2 = this->Nodes[this->Edges[e2].second];
      vtkVector3f r2 = s2 - t2;
      r2.Normalize();
      float dot = r1.Dot(r2);
      this->EdgeDots[e1][e2] = dot;
      this->EdgeDots[e2][e1] = dot;
    }
  }
}

void vtkBundlingMetadata::InitializeEdgeMesh()
{
  this->MeshCount = 2;
  vtkIdType numEdges = this->Graph->GetNumberOfEdges();
  this->EdgeMesh.resize(numEdges, std::vector<vtkVector3f>(2));
  this->EdgeMeshVelocities.resize(numEdges, std::vector<vtkVector3f>(2));
  this->EdgeMeshAccelerations.resize(numEdges, std::vector<vtkVector3f>(2));
  //this->EdgeMeshGroupCounts.resize(numEdges, std::vector<float>(2, 1.0f));
  for (vtkIdType e = 0; e < numEdges; ++e)
  {
    this->EdgeMesh[e][0] = this->Nodes[this->Edges[e].first];
    this->EdgeMesh[e][1] = this->Nodes[this->Edges[e].second];
  }
}

void vtkBundlingMetadata::DoubleEdgeMeshResolution()
{
  int newMeshCount = (this->MeshCount - 1)*2 + 1;
  vtkIdType numEdges = this->Graph->GetNumberOfEdges();
  std::vector<std::vector<vtkVector3f> > newEdgeMesh(
      numEdges, std::vector<vtkVector3f>(newMeshCount));
  std::vector<std::vector<vtkVector3f> > newEdgeMeshVelocities(
      numEdges, std::vector<vtkVector3f>(newMeshCount, vtkVector3f(0.0f, 0.0f, 0.0f)));
  std::vector<std::vector<vtkVector3f> > newEdgeMeshAccelerations(
      numEdges, std::vector<vtkVector3f>(newMeshCount, vtkVector3f(0.0f, 0.0f, 0.0f)));
  //std::vector<std::vector<float> > newEdgeMeshGroupCounts(
  //    numEdges, std::vector<float>(newMeshCount, 1.0f));
  for (vtkIdType e = 0; e < numEdges; ++e)
  {
    for (int m = 0; m < newMeshCount; ++m)
    {
      float indexFloat = (this->MeshCount - 1.0f)*m/(newMeshCount - 1.0f);
      int index = static_cast<int>(indexFloat);
      float alpha = indexFloat - index;
      vtkVector3f before = this->EdgeMesh[e][index];
      if (alpha > 0)
      {
        vtkVector3f after = this->EdgeMesh[e][index+1];
        newEdgeMesh[e][m] = before + alpha*(after - before);
      }
      else
      {
        newEdgeMesh[e][m] = before;
      }
    }
  }
  this->MeshCount = newMeshCount;
  this->EdgeMesh = newEdgeMesh;
  this->EdgeMeshVelocities = newEdgeMeshVelocities;
  this->EdgeMeshAccelerations = newEdgeMeshAccelerations;
  //this->EdgeMeshGroupCounts = newEdgeMeshGroupCounts;
}

void vtkBundlingMetadata::SimulateEdgeStep()
{
  vtkIdType numEdges = this->Graph->GetNumberOfEdges();

  for (vtkIdType e1 = 0; e1 < numEdges; ++e1)
  {
    float weight1 = 1.0f;
    for (int m1 = 0; m1 < this->MeshCount; ++m1)
    {
      // Immovable
      if (m1 <= 0 || m1 >= this->MeshCount - 1)
      {
        continue;
      }

      // Move the point according to dynamics
      vtkVector3f position = this->EdgeMesh[e1][m1];
      vtkVector3f velocity = this->EdgeMeshVelocities[e1][m1];
      vtkVector3f acceleration = this->EdgeMeshAccelerations[e1][m1];
      velocity = velocity + acceleration * this->SimulationStep * 0.5f;
      velocity = velocity * this->VelocityDamping;
      position = position + velocity * this->SimulationStep;
      this->EdgeMesh[e1][m1] = position;

      acceleration = vtkVector3f(0.0f, 0.0f, 0.0f);

      // Spring force
      vtkVector3f prevPosition = this->EdgeMesh[e1][m1-1];
      vtkVector3f prevDirection = prevPosition - position;
      float prevDist = prevDirection.Norm();
      float prevForce = this->EdgeSpringConstant / 1000.0f * (this->MeshCount - 1) * prevDist * weight1;
      prevDirection.Normalize();
      acceleration = acceleration + prevForce * prevDirection;

      vtkVector3f nextPosition = this->EdgeMesh[e1][m1+1];
      vtkVector3f nextDirection = nextPosition - position;
      float nextDist = nextDirection.Norm();
      float nextForce = this->EdgeSpringConstant / 1000.0f * (this->MeshCount - 1) * nextDist * weight1;
      nextDirection.Normalize();
      acceleration = acceleration + nextForce * nextDirection;

      // Coulomb force
      float normalizedEdgeCoulombConstant = this->EdgeCoulombConstant / sqrt(static_cast<float>(numEdges));

      for (vtkIdType e2 = 0; e2 < numEdges; ++e2)
      {
        if (e1 == e2)
        {
          continue;
        }

        float compatibility = this->EdgeCompatibilities[e1][e2];
        if (compatibility <= 0.05)
        {
          continue;
        }

        float dot = this->EdgeDots[e1][e2];
        float weight2 = 1.0f;

        int m2;
        if (dot >= 0.0f)
        {
          m2 = m1;
        }
        else
        {
          m2 = this->MeshCount - 1 - m1;
        }

        vtkVector3f position2;
        // If we're going the same direction is edge1, then the potential minimum is at the point.
        if (dot >= 0.0f)
        {
          position2 = this->EdgeMesh[e2][m2];
        }
        // If we're going the opposite direction, the potential minimum is edgeLaneWidth to the "right."
        else
        {
          vtkVector3f tangent = this->EdgeMesh[e2][m2+1] - this->EdgeMesh[e2][m2-1];
          tangent.Normalize();
          // This assumes 2D
          vtkVector3f normal(-tangent[1], tangent[0], 0.0f);
          position2 = this->EdgeMesh[e2][m2] + normal*this->EdgeLaneWidth;
        }

        vtkVector3f direction = position2 - position;
        float distance = direction.Norm();

        // Inverse force.
        float force;
        if (!this->UseNewForce)
        {
          force = normalizedEdgeCoulombConstant * 30.0f / (this->MeshCount - 1) / (distance + 0.01f);
        }
        // New force.
        else
        {
          force = 4.0f * 10000.0f / (this->MeshCount - 1) * this->EdgeCoulombDecay * normalizedEdgeCoulombConstant * distance / (3.1415926f * pow(this->EdgeCoulombDecay * this->EdgeCoulombDecay + distance * distance, 2));
        }
        force *= weight2;
        force *= compatibility;

        if (distance > 0.0f)
        {
          direction.Normalize();
          acceleration = acceleration + force * direction;
        }
      }

      velocity = velocity + acceleration * this->SimulationStep * 0.5f;
      this->EdgeMeshVelocities[e1][m1] = velocity;
      this->EdgeMeshAccelerations[e1][m1] = acceleration;
    }
  }
}

void vtkBundlingMetadata::SmoothEdges()
{
  // From Mathematica Total[GaussianMatrix[{3, 3}]]
  int kernelSize = 3;
  // Has to sum to 1.0 to be correct.
  float gaussianKernel[] = {0.10468, 0.139936, 0.166874, 0.177019, 0.166874, 0.139936, 0.10468};
  vtkIdType numEdges = this->Graph->GetNumberOfEdges();
  std::vector<std::vector<vtkVector3f> > smoothedEdgeMesh(
      numEdges, std::vector<vtkVector3f>(this->MeshCount));
  for (vtkIdType e = 0; e < numEdges; ++e)
  {
    for (int m = 1; m < this->MeshCount - 1; ++m)
    {
      vtkVector3f smoothed(0.0f, 0.0f, 0.0f);
      for (int kernelIndex = 0; kernelIndex < kernelSize * 2 + 1; kernelIndex++)
      {
        int m2 = m + kernelIndex - kernelSize;
        m2 = std::max(0, std::min(this->MeshCount - 1, m2));

        vtkVector3f pt = this->EdgeMesh[e][m2];
        smoothed = smoothed + gaussianKernel[kernelIndex] * pt;
      }
      smoothedEdgeMesh[e][m] = smoothed;
    }
  }
  this->EdgeMesh = smoothedEdgeMesh;
}

void vtkBundlingMetadata::LayoutEdgePoints()
{
  this->InitializeEdgeMesh();
  this->SimulationStep = 40.0f;
  this->CycleIterations = 30;
  for (int i = 0; i < 5; ++i)
  {
    vtkDebugWithObjectMacro(this->Outer, "vtkBoostDividedEdgeBundling cycle " << i);
    this->CycleIterations = this->CycleIterations * 2 / 3;
    this->SimulationStep = 0.85f*this->SimulationStep;
    this->DoubleEdgeMeshResolution();
    for (int j = 0; j < this->CycleIterations; ++j)
    {
      vtkDebugWithObjectMacro(this->Outer, "vtkBoostDividedEdgeBundling iteration " << j);
      this->SimulateEdgeStep();
    }
  }
  this->SmoothEdges();
}

int vtkBoostDividedEdgeBundling::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *graphInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDirectedGraph *g = vtkDirectedGraph::SafeDownCast(
    graphInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDirectedGraph *output = vtkDirectedGraph::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkBundlingMetadata *meta = new vtkBundlingMetadata(this, g);

  meta->NormalizeNodePositions();
  meta->CalculateEdgeLengths();
  meta->CalculateNodeDistances();
  meta->CalculateEdgeCompatibilities();
  meta->LayoutEdgePoints();
  meta->DenormalizeNodePositions();

  output->ShallowCopy(g);

  for (vtkIdType e = 0; e < g->GetNumberOfEdges(); ++e)
  {
    output->ClearEdgePoints(e);
    for (int m = 1; m < meta->MeshCount-1; ++m)
    {
      vtkVector3f edgePoint = meta->EdgeMesh[e][m];
      output->AddEdgePoint(e, edgePoint[0], edgePoint[1], edgePoint[2]);
    }
  }

  delete meta;

  return 1;
}

void vtkBoostDividedEdgeBundling::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

