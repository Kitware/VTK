/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBezierContourLineInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBezierContourLineInterpolator.h"

#include "vtkContourRepresentation.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkIntArray.h"

vtkStandardNewMacro(vtkBezierContourLineInterpolator);

//----------------------------------------------------------------------
vtkBezierContourLineInterpolator::vtkBezierContourLineInterpolator()
{
  this->MaximumCurveError        = 0.005;
  this->MaximumCurveLineSegments = 100;
}

//----------------------------------------------------------------------
vtkBezierContourLineInterpolator::~vtkBezierContourLineInterpolator()
{
}

//----------------------------------------------------------------------
int vtkBezierContourLineInterpolator::InterpolateLine( vtkRenderer *vtkNotUsed(ren),
                                                       vtkContourRepresentation *rep,
                                                       int idx1, int idx2 )
{
  int maxRecursion = 0;
  int tmp = 3;

  while ( 2*tmp < this->MaximumCurveLineSegments )
  {
    tmp *= 2;
    maxRecursion++;
  }

  if ( maxRecursion == 0 )
  {
    return 1;
  }

  // There are four control points with 3 components each, plus one
  // value for the recursion depth of this point
  double *controlPointsStack = new double[(3*4+1)*(maxRecursion+1)];
  int stackCount = 0;

  double slope1[3];
  double slope2[3];

  rep->GetNthNodeSlope( idx1, slope1 );
  rep->GetNthNodeSlope( idx2, slope2 );

  controlPointsStack[0] = 0;
  double *p1 = controlPointsStack+1;
  double *p2 = controlPointsStack+4;
  double *p3 = controlPointsStack+7;
  double *p4 = controlPointsStack+10;

  rep->GetNthNodeWorldPosition( idx1, p1 );
  rep->GetNthNodeWorldPosition( idx2, p4 );

  double distance = sqrt( vtkMath::Distance2BetweenPoints( p1, p4 ) );

  p2[0] = p1[0] + .333*distance*slope1[0];
  p2[1] = p1[1] + .333*distance*slope1[1];
  p2[2] = p1[2] + .333*distance*slope1[2];

  p3[0] = p4[0] - .333*distance*slope2[0];
  p3[1] = p4[1] - .333*distance*slope2[1];
  p3[2] = p4[2] - .333*distance*slope2[2];

  stackCount++;

  while ( stackCount )
  {
    //process last point on stack
    int recursionLevel = static_cast<int>(controlPointsStack[13*(stackCount-1)]);

    p1 = controlPointsStack + 13*(stackCount-1)+1;
    p2 = controlPointsStack + 13*(stackCount-1)+4;
    p3 = controlPointsStack + 13*(stackCount-1)+7;
    p4 = controlPointsStack + 13*(stackCount-1)+10;

    double totalDist = 0;
    totalDist += sqrt(vtkMath::Distance2BetweenPoints(p1,p2));
    totalDist += sqrt(vtkMath::Distance2BetweenPoints(p2,p3));
    totalDist += sqrt(vtkMath::Distance2BetweenPoints(p3,p4));

    distance = sqrt(vtkMath::Distance2BetweenPoints(p1,p4));

    if ( recursionLevel >= maxRecursion || distance == 0 ||
         (totalDist - distance)/distance < this->MaximumCurveError )
    {
      rep->AddIntermediatePointWorldPosition( idx1, p2 );
      rep->AddIntermediatePointWorldPosition( idx1, p3 );

      if ( stackCount > 1 )
      {
        rep->AddIntermediatePointWorldPosition( idx1, p4 );
      }
      stackCount--;
    }
    else
    {
      double p12[3], p23[3], p34[3], p123[3], p234[3], p1234[3];

      this->ComputeMidpoint( p1, p2, p12 );
      this->ComputeMidpoint( p2, p3, p23 );
      this->ComputeMidpoint( p3, p4, p34 );
      this->ComputeMidpoint( p12, p23, p123 );
      this->ComputeMidpoint( p23, p34, p234 );
      this->ComputeMidpoint( p123, p234, p1234 );

      // add these two points to the stack
      controlPointsStack[13*(stackCount-1)] = recursionLevel+1;
      controlPointsStack[13*(stackCount)] = recursionLevel+1;

      double *newp1 = controlPointsStack + 13*(stackCount)+1;
      double *newp2 = controlPointsStack + 13*(stackCount)+4;
      double *newp3 = controlPointsStack + 13*(stackCount)+7;
      double *newp4 = controlPointsStack + 13*(stackCount)+10;

      newp1[0] = p1[0];
      newp1[1] = p1[1];
      newp1[2] = p1[2];

      newp2[0] = p12[0];
      newp2[1] = p12[1];
      newp2[2] = p12[2];

      newp3[0] = p123[0];
      newp3[1] = p123[1];
      newp3[2] = p123[2];

      newp4[0] = p1234[0];
      newp4[1] = p1234[1];
      newp4[2] = p1234[2];

      p1[0] = p1234[0];
      p1[1] = p1234[1];
      p1[2] = p1234[2];

      p2[0] = p234[0];
      p2[1] = p234[1];
      p2[2] = p234[2];

      p3[0] = p34[0];
      p3[1] = p34[1];
      p3[2] = p34[2];

      stackCount++;
    }
  }

  delete [] controlPointsStack;

  return 1;
}

//----------------------------------------------------------------------
void vtkBezierContourLineInterpolator::GetSpan( int nodeIndex,
                                          vtkIntArray *nodeIndices,
                                          vtkContourRepresentation *rep)
{
  int start = nodeIndex - 2;
  int end   = nodeIndex - 1;
  int index[2];

  // Clear the array
  nodeIndices->Reset();
  nodeIndices->Squeeze();
  nodeIndices->SetNumberOfComponents(2);

  for ( int i = 0; i < 4; i++ )
  {
    index[0] = start++;
    index[1] = end++;

    if ( rep->GetClosedLoop() )
    {
      if ( index[0] < 0 )
      {
        index[0] += rep->GetNumberOfNodes();
      }
      if ( index[1] < 0 )
      {
        index[1] += rep->GetNumberOfNodes();
      }
      if ( index[0] >= rep->GetNumberOfNodes() )
      {
        index[0] -= rep->GetNumberOfNodes();
      }
      if ( index[1] >= rep->GetNumberOfNodes() )
      {
        index[1] -= rep->GetNumberOfNodes();
      }
    }

    if ( index[0] >= 0 && index[0] < rep->GetNumberOfNodes() &&
         index[1] >= 0 && index[1] < rep->GetNumberOfNodes() )
    {
      nodeIndices->InsertNextTypedTuple( index );
    }
  }
}

//----------------------------------------------------------------------
void vtkBezierContourLineInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Maximum Curve Error: " << this->MaximumCurveError << "\n";
  os << indent << "Maximum Curve Line Segments: "
     << this->MaximumCurveLineSegments << "\n";
}

