/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphToPolyData.cxx

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
#include "vtkGraphToPolyData.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDirectedGraph.h"
#include "vtkDoubleArray.h"
#include "vtkEdgeListIterator.h"
#include "vtkGlyph3D.h"
#include "vtkGlyphSource2D.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"

#include <vtksys/stl/map>
#include <vtksys/stl/utility>
#include <vtksys/stl/vector>

vtkCxxRevisionMacro(vtkGraphToPolyData, "1.11");
vtkStandardNewMacro(vtkGraphToPolyData);

vtkGraphToPolyData::vtkGraphToPolyData()
{
  this->EdgeGlyphOutput = false;
  this->EdgeGlyphPosition = 1.0;
  this->ArcEdges = false;
  this->NumberOfArcSubdivisions = 10;
  this->SetNumberOfOutputPorts(2);
}

int vtkGraphToPolyData::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  return 1;
}

int vtkGraphToPolyData::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation *arrowInfo = outputVector->GetInformationObject(1);

  // get the input and ouptut
  vtkGraph *input = vtkGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *arrowOutput = vtkPolyData::SafeDownCast(
    arrowInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataArray* edgeGhostLevels = vtkDataArray::SafeDownCast(
    input->GetEdgeData()->GetAbstractArray("vtkGhostLevels"));

  if (this->ArcEdges)
    {
    bool directed = vtkDirectedGraph::SafeDownCast(input) != 0;
    vtksys_stl::map<vtksys_stl::pair<vtkIdType, vtkIdType>, int> edgeCount;
    vtksys_stl::map<vtksys_stl::pair<vtkIdType, vtkIdType>, int> edgeNumber;
    vtksys_stl::vector<vtkEdgeType> edgeVector(input->GetNumberOfEdges());
    vtkSmartPointer<vtkEdgeListIterator> it = 
      vtkSmartPointer<vtkEdgeListIterator>::New();
    input->GetEdges(it);
    double avgEdgeLength = 0.0;
    while (it->HasNext())
      {
      vtkEdgeType e = it->Next();
      vtkIdType src, tgt;
      if (directed || e.Source < e.Target)
        {
        src = e.Source;
        tgt = e.Target;
        }
      else
        {
        src = e.Target;
        tgt = e.Source;
        }
      edgeCount[vtksys_stl::pair<vtkIdType, vtkIdType>(src, tgt)]++;
      edgeVector[e.Id] = e;

      // Compute edge length
      double sourcePt[3];
      double targetPt[3];
      input->GetPoint(e.Source, sourcePt);
      input->GetPoint(e.Target, targetPt);
      avgEdgeLength += 
        sqrt(vtkMath::Distance2BetweenPoints(sourcePt, targetPt));
      }
    vtkIdType numEdges = input->GetNumberOfEdges();
    avgEdgeLength /= numEdges;
    double maxLoopHeight = avgEdgeLength / 10.0;
    vtkSmartPointer<vtkCellArray> newLines =
      vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkPoints> newPoints =
      vtkSmartPointer<vtkPoints>::New();
    for (vtkIdType eid = 0; eid < numEdges; ++eid)
      {
      vtkEdgeType e = edgeVector[eid];
      vtkIdType src, tgt;
      if (directed || e.Source < e.Target)
        {
        src = e.Source;
        tgt = e.Target;
        }
      else
        {
        src = e.Target;
        tgt = e.Source;
        }
      // Lookup the total number of edges with this source
      // and target, as well as how many times this pair
      // has been found so far.
      vtksys_stl::pair<vtkIdType,vtkIdType> p(src, tgt);
      edgeNumber[p]++;
      int cur = edgeNumber[p];
      int total = edgeCount[p];
      vtksys_stl::pair<vtkIdType,vtkIdType> revP(tgt, src);
      int revTotal = edgeCount[revP];

      double sourcePt[3];
      double targetPt[3];
      input->GetPoint(e.Source, sourcePt);
      input->GetPoint(e.Target, targetPt);

      // If only one edge between source and target,
      // just draw a straight line.
      if (total + revTotal == 1)
        {
        newLines->InsertNextCell(2);
        newLines->InsertCellPoint(newPoints->InsertNextPoint(sourcePt));
        newLines->InsertCellPoint(newPoints->InsertNextPoint(targetPt));
        continue;
        }

      // Create the new cell
      newLines->InsertNextCell(this->NumberOfArcSubdivisions);

      // Find vector from source to target
      double delta[3];
      for (int c = 0; c < 3; ++c)
        {
        delta[c] = targetPt[c] - sourcePt[c];
        }
      double dist = vtkMath::Norm(delta);

      // If the distance is zero, draw a loop.
      if (dist == 0)
        {
        double radius = maxLoopHeight*cur/total;
        double u[3] = {1.0, 0.0, 0.0};
        double v[3] = {0.0, 1.0, 0.0};
        double center[3] = {sourcePt[0] - radius, sourcePt[1], sourcePt[2]};
        // Use the general equation for a circle in three dimensions
        // to draw a loop.
        for (int s = 0; s < this->NumberOfArcSubdivisions; ++s)
          {
          double angle = 2.0*vtkMath::Pi()
            *s/(this->NumberOfArcSubdivisions-1);
          double circlePt[3];
          for (int c = 0; c < 3; ++c)
            {
            circlePt[c] = center[c] 
              + radius*cos(angle)*u[c] 
              + radius/2.0*sin(angle)*v[c];
            }
          vtkIdType newPt = newPoints->InsertNextPoint(circlePt);
          newLines->InsertCellPoint(newPt);
          }
        continue;
        }

      // Find vector perpendicular to delta
      // and (0,0,1).
      double z[3] = {0.0, 0.0, 1.0};
      double w[3];
      vtkMath::Cross(delta, z, w);
      vtkMath::Normalize(w);

      // Really bad ascii art:
      //    ___-------___
      //   /      |height\ <-- the drawn arc
      // src----dist-----tgt
      //   \      |      /
      //    \     |offset
      //     \    |    /
      //    u \   |   / x
      //       \  |  /
      //        \ | /
      //         \|/
      //        center
      // The center of the circle used to draw the arc is a
      // point along the vector w a certain distance (offset)
      // from the midpoint of sourcePt and targetPt.
      // The offset is computed to give a certain arc height
      // based on cur and total.
      double maxHeight = dist/8.0;
      double height;
      int sign = 1;
      if (directed)
        {
        // Directed edges will go on one side or the other
        // automatically based on the order of source and target.
        height = (static_cast<double>(cur)/total)*maxHeight;
        }
      else
        {
        // For undirected edges, place every other edge on one
        // side or the other.
        height = (static_cast<double>((cur+1)/2)/(total/2))*maxHeight;
        if (cur % 2)
          {
          sign = -1;
          }
        }
      // This formula computes offset given dist and height.
      // You can pull out your trig formulas and verify it :)
      double offset = (dist*dist/4.0 - height*height)/(2.0*height);
      double center[3];
      for (int c = 0; c < 3; ++c)
        {
        center[c] = (targetPt[c] + sourcePt[c])/2.0 + sign*offset*w[c];
        }

      // The vectors u and x are unit vectors pointing from the
      // center of the circle to the two endpoints of the arc,
      // sourcePt and targetPt, respectively.
      double u[3], x[3];
      for (int c = 0; c < 3; ++c)
        {
        u[c] = sourcePt[c] - center[c];
        x[c] = targetPt[c] - center[c];
        }
      double radius = vtkMath::Norm(u);
      vtkMath::Normalize(u);
      vtkMath::Normalize(x);

      // Find the angle that the arc spans.
      double theta = acos(vtkMath::Dot(u, x));

      // We need two perpendicular vectors on the plane of the circle
      // in order to draw the circle.  First we calculate n, a vector
      // normal to the circle, by crossing u and w.  Next, we cross
      // n and u in order to get a vector v in the plane of the circle
      // that is perpendicular to u.
      double n[3];
      vtkMath::Cross(u, w, n);
      vtkMath::Normalize(n);
      double v[3];
      vtkMath::Cross(n, u, v);
      vtkMath::Normalize(v);

      // Use the general equation for a circle in three dimensions
      // to draw an arc from the last point to the current point.
      for (int s = 0; s < this->NumberOfArcSubdivisions; ++s)
        {
        double angle = -sign*s*theta/(this->NumberOfArcSubdivisions - 1.0);
        double circlePt[3];
        for (int c = 0; c < 3; ++c)
          {
          circlePt[c] = center[c] 
            + radius*cos(angle)*u[c] 
            + radius*sin(angle)*v[c];
          }
        vtkIdType newPt = newPoints->InsertNextPoint(circlePt);
        newLines->InsertCellPoint(newPt);
        }
      }
    output->SetLines(newLines);
    output->SetPoints(newPoints);

    // Points do NOT correspond to vertices here, so do not
    // pass that data along.

    // Cells correspond to edges, so pass the cell data along.
    output->GetCellData()->PassData(input->GetEdgeData());
    }
  else if (edgeGhostLevels == NULL)
    {
    vtkSmartPointer<vtkIdTypeArray> cells = 
      vtkSmartPointer<vtkIdTypeArray>::New();
    cells->SetNumberOfTuples(3*input->GetNumberOfEdges());
    vtkSmartPointer<vtkEdgeListIterator> it = 
      vtkSmartPointer<vtkEdgeListIterator>::New();
    input->GetEdges(it);
    while (it->HasNext())
      {
      vtkEdgeType e = it->Next();
      cells->SetValue(3*e.Id + 0, 2);
      cells->SetValue(3*e.Id + 1, e.Source);
      cells->SetValue(3*e.Id + 2, e.Target);
      }
    vtkSmartPointer<vtkCellArray> newLines = 
      vtkSmartPointer<vtkCellArray>::New();
    newLines->SetCells(input->GetNumberOfEdges(), cells);

    // Send the data to output.
    output->SetPoints(input->GetPoints());
    output->SetLines(newLines);

    // Points correspond to vertices, so pass the data along.
    output->GetPointData()->PassData(input->GetVertexData());

    // Cells correspond to edges, so pass the cell data along.
    output->GetCellData()->PassData(input->GetEdgeData());
    }
  else
    {
    vtkIdType numEdges = input->GetNumberOfEdges();
    vtkDataSetAttributes *inputCellData = input->GetEdgeData();
    vtkCellData *outputCellData = output->GetCellData();
    outputCellData->CopyAllocate(inputCellData);
    vtkSmartPointer<vtkCellArray> newLines = 
      vtkSmartPointer<vtkCellArray>::New();
    newLines->Allocate(newLines->EstimateSize(numEdges, 2));
    vtkIdType points[2];

    // Only create lines for non-ghost edges
    vtkSmartPointer<vtkEdgeListIterator> it = 
      vtkSmartPointer<vtkEdgeListIterator>::New();
    input->GetEdges(it);
    while (it->HasNext())
      {
      vtkEdgeType e = it->Next();
      if (edgeGhostLevels->GetComponent(e.Id, 0) == 0) 
        {
        points[0] = e.Source;
        points[1] = e.Target;
        vtkIdType ind = newLines->InsertNextCell(2, points);
        outputCellData->CopyData(inputCellData, e.Id, ind);
        }
      } 

    // Send data to output
    output->SetPoints(input->GetPoints());
    output->SetLines(newLines);
    output->GetPointData()->PassData(input->GetVertexData());

    // Clean up
    output->Squeeze();
    }

  if (this->EdgeGlyphOutput)
    {
    vtkDataSetAttributes *inputCellData = input->GetEdgeData();
    
    vtkPointData* arrowPointData = arrowOutput->GetPointData();
    arrowPointData->CopyAllocate(inputCellData);
    vtkPoints* newPoints = vtkPoints::New();
    arrowOutput->SetPoints(newPoints);
    newPoints->Delete();
    vtkDoubleArray* orientArr = vtkDoubleArray::New();
    orientArr->SetNumberOfComponents(3);
    orientArr->SetName("orientation");
    arrowPointData->AddArray(orientArr);
    arrowPointData->SetVectors(orientArr);
    orientArr->Delete();
    double sourcePt[3] = {0, 0, 0};
    double targetPt[3] = {0, 0, 0};
    double pt[3] = {0, 0, 0};
    double orient[3] = {0, 0, 0};
    vtkSmartPointer<vtkEdgeListIterator> it = 
      vtkSmartPointer<vtkEdgeListIterator>::New();
    input->GetEdges(it);
    while (it->HasNext())
      {
      vtkEdgeType e = it->Next();
      if (!edgeGhostLevels || edgeGhostLevels->GetComponent(e.Id, 0) == 0) 
        {
        vtkIdType source = e.Source;
        vtkIdType target = e.Target;
        // Do not render arrows for self loops.
        if (source != target)
          {
          input->GetPoint(source, sourcePt);
          input->GetPoint(target, targetPt);
          for (int j = 0; j < 3; j++)
            {
            pt[j] = (1 - this->EdgeGlyphPosition)*sourcePt[j] + this->EdgeGlyphPosition*targetPt[j];
            orient[j] = targetPt[j] - sourcePt[j];
            }
          vtkIdType ind = newPoints->InsertNextPoint(pt);
          orientArr->InsertNextTuple(orient);
          arrowPointData->CopyData(inputCellData, e.Id, ind);
          }
        }
      }    
    }

  return 1;
}

void vtkGraphToPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "EdgeGlyphOutput: "
    << (this->EdgeGlyphOutput ? "on" : "off") << endl;
  os << indent << "EdgeGlyphPosition: " << this->EdgeGlyphPosition << endl;
  os << indent << "ArcEdges: " << (this->ArcEdges ? "on" : "off") << endl;
  os << indent << "NumberOfArcSubdivisions: "
    << this->NumberOfArcSubdivisions << endl;
}
