/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRadiusOutlierRemoval.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRadiusOutlierRemoval.h"

#include "vtkObjectFactory.h"
#include "vtkAbstractPointLocator.h"
#include "vtkStaticPointLocator.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkIdList.h"
#include "vtkSMPTools.h"
#include "vtkSMPThreadLocalObject.h"


vtkStandardNewMacro(vtkRadiusOutlierRemoval);
vtkCxxSetObjectMacro(vtkRadiusOutlierRemoval,Locator,vtkAbstractPointLocator);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace {

//----------------------------------------------------------------------------
// The threaded core of the algorithm (first pass)
template <typename T>
struct RemoveOutliers
{
  const T *Points;
  vtkAbstractPointLocator *Locator;
  double Radius;
  int NumNeighbors;
  vtkIdType *PointMap;

  // Don't want to allocate working arrays on every thread invocation. Thread local
  // storage lots of new/delete.
  vtkSMPThreadLocalObject<vtkIdList> PIds;

  RemoveOutliers(T *points, vtkAbstractPointLocator *loc, double radius, int numNei,
                 vtkIdType *map) : Points(points), Locator(loc), Radius(radius),
                                   NumNeighbors(numNei), PointMap(map)
  {
  }

  // Just allocate a little bit of memory to get started.
  void Initialize()
  {
    vtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); //allocate some memory
  }

  void operator() (vtkIdType ptId, vtkIdType endPtId)
  {
      const T *p = this->Points + 3*ptId;
      vtkIdType *map = this->PointMap + ptId;
      double x[3];
      vtkIdList*& pIds = this->PIds.Local();

      for ( ; ptId < endPtId; ++ptId)
      {
        x[0] = static_cast<double>(*p++);
        x[1] = static_cast<double>(*p++);
        x[2] = static_cast<double>(*p++);

        this->Locator->FindPointsWithinRadius(this->Radius, x, pIds);
        vtkIdType numPts = pIds->GetNumberOfIds();

        // Keep in mind that The FindPoints method will always return at
        // least one point (itself).
        *map++ = ( numPts > this->NumNeighbors ? 1 : -1 );
      }
  }

  void Reduce()
  {
  }

  static void Execute(vtkRadiusOutlierRemoval *self, vtkIdType numPts,
                      T *points, vtkIdType *map)
  {
      RemoveOutliers remove(points, self->GetLocator(), self->GetRadius(),
                            self->GetNumberOfNeighbors(), map);
      vtkSMPTools::For(0, numPts, remove);
  }

}; //RemoveOutliers

} //anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
vtkRadiusOutlierRemoval::vtkRadiusOutlierRemoval()
{
  this->Radius = 1.0;
  this->NumberOfNeighbors = 2;
  this->Locator = vtkStaticPointLocator::New();
}

//----------------------------------------------------------------------------
vtkRadiusOutlierRemoval::~vtkRadiusOutlierRemoval()
{
  this->SetLocator(NULL);
}

//----------------------------------------------------------------------------
// Traverse all the input points to see how many neighbors each point has
// within a specified radius, and populate the map which indicates how points
// are to be copied to the output.
int vtkRadiusOutlierRemoval::FilterPoints(vtkPointSet *input)
{
  // Perform the point removal
  // Start by building the locator
  if ( !this->Locator )
  {
    vtkErrorMacro(<<"Point locator required\n");
    return 0;
  }
  this->Locator->SetDataSet(input);
  this->Locator->BuildLocator();

  // Determine which points, if any, should be removed. We create a map
  // to keep track. The bulk of the algorithmic work is done in this pass.
  vtkIdType numPts = input->GetNumberOfPoints();
  void *inPtr = input->GetPoints()->GetVoidPointer(0);
  switch (input->GetPoints()->GetDataType())
  {
    vtkTemplateMacro(RemoveOutliers<VTK_TT>::
                     Execute(this, numPts, (VTK_TT *)inPtr, this->PointMap));
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkRadiusOutlierRemoval::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Number of Neighbors: " << this->NumberOfNeighbors << "\n";
  os << indent << "Locator: " << this->Locator << "\n";
}
