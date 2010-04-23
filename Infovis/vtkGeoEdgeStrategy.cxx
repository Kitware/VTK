/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoEdgeStrategy.cxx

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
#include "vtkGeoEdgeStrategy.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkEdgeListIterator.h"
#include "vtkFloatArray.h"
#include "vtkGeoMath.h"
#include "vtkGraph.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"

#include <vtksys/stl/utility>
#include <vtksys/stl/vector>
#include <vtksys/stl/map>

vtkStandardNewMacro(vtkGeoEdgeStrategy);

vtkGeoEdgeStrategy::vtkGeoEdgeStrategy()
{
  this->GlobeRadius = vtkGeoMath::EarthRadiusMeters();
  this->ExplodeFactor = 0.2;
  this->NumberOfSubdivisions = 20;
}

void vtkGeoEdgeStrategy::Layout()
{
  vtksys_stl::map<vtksys_stl::pair<vtkIdType, vtkIdType>, int> edgeCount;
  vtksys_stl::map<vtksys_stl::pair<vtkIdType, vtkIdType>, int> edgeNumber;
  vtksys_stl::vector<vtkEdgeType> edgeVector(this->Graph->GetNumberOfEdges());
  vtkSmartPointer<vtkEdgeListIterator> it = 
    vtkSmartPointer<vtkEdgeListIterator>::New();
  this->Graph->GetEdges(it);
  while (it->HasNext())
    {
    vtkEdgeType e = it->Next();
    vtkIdType src, tgt;
    if (e.Source < e.Target)
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
    }
  vtkIdType numEdges = this->Graph->GetNumberOfEdges();
  double* pts = new double[this->NumberOfSubdivisions*3];
  for (vtkIdType eid = 0; eid < numEdges; ++eid)
    {
    vtkEdgeType e = edgeVector[eid];
    vtkIdType src, tgt;
    if (e.Source < e.Target)
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

    double sourcePt[3];
    double targetPt[3];
    this->Graph->GetPoint(e.Source, sourcePt);
    this->Graph->GetPoint(e.Target, targetPt);

    // Find w, a unit vector pointing from the center of the
    // earth directly inbetween the two endpoints.
    double w[3];
    for (int c = 0; c < 3; ++c)
      {
      w[c] = (sourcePt[c] + targetPt[c])/2.0;
      }
    vtkMath::Normalize(w);
    
    // The center of the circle used to draw the arc is a
    // point along the vector w scaled by the explode factor.
    // Use cur and total to separate parallel arcs.
    double center[3];
    for (int c = 0; c < 3; ++c)
      {
      center[c] = this->ExplodeFactor * this->GlobeRadius * w[c]
                  * (cur + 1) / total;
      }
    
    // The vectors u and x are unit vectors pointing from the
    // center of the circle to the two endpoints of the arc,
    // lastPoint and curPoint, respectively.
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
    
    // If the vectors u, x point toward the center of the earth, take
    // the larger angle between the vectors.
    // We determine whether u points toward the center of the earth
    // by checking whether the dot product of u and w is negative.
    if (vtkMath::Dot(w, u) < 0)
      {
      theta = 2.0*vtkMath::Pi() - theta;
      }

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
    for (int s = 0; s < this->NumberOfSubdivisions; ++s)
      {
      double angle = (this->NumberOfSubdivisions - 1.0 - s)
        * theta / (this->NumberOfSubdivisions - 1.0);
      for (int c = 0; c < 3; ++c)
        {
        pts[3*s + c] = center[c]
          + radius*cos(angle)*u[c]
          + radius*sin(angle)*v[c];
        }
      }
    this->Graph->SetEdgePoints(e.Id, this->NumberOfSubdivisions, pts);

    if (eid % 1000 == 0)
      {
      double progress = eid / static_cast<double>(numEdges);
      this->InvokeEvent(vtkCommand::ProgressEvent, static_cast<void*>(&progress));
      }
    }
  double progress = 1.0;
  this->InvokeEvent(vtkCommand::ProgressEvent, static_cast<void*>(&progress));
  delete [] pts;
}

void vtkGeoEdgeStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "GlobeRadius: " << this->GlobeRadius << endl;
  os << indent << "ExplodeFactor: " << this->ExplodeFactor << endl;
  os << indent << "NumberOfSubdivisions: " << this->NumberOfSubdivisions << endl;
}

