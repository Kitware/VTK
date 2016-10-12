/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRuledSurfaceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRuledSurfaceFilter.h"

#include "vtkCellArray.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyLine.h"

vtkStandardNewMacro(vtkRuledSurfaceFilter);

vtkRuledSurfaceFilter::vtkRuledSurfaceFilter()
{
  this->DistanceFactor = 3.0;
  this->OnRatio = 1;
  this->Offset = 0;
  this->CloseSurface = 0;
  this->RuledMode = VTK_RULED_MODE_RESAMPLE;
  this->Resolution[0] = 1;
  this->Resolution[1] = 1;
  this->PassLines = 0;
  this->OrientLoops = 0;
  this->Ids = vtkIdList::New();
  this->Ids->SetNumberOfIds(4);
}

vtkRuledSurfaceFilter::~vtkRuledSurfaceFilter()
{
  this->Ids->Delete();
}

int vtkRuledSurfaceFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints *inPts, *newPts = NULL;
  vtkIdType i, numPts, numLines;
  vtkCellArray *inLines, *newPolys, *newStrips;
  vtkIdType *pts = 0;
  vtkIdType *pts2 = 0;
  vtkIdType npts = 0;
  vtkIdType npts2 = 0;
  vtkPointData *inPD=input->GetPointData(), *outPD=output->GetPointData();

  // Check input, pass data if requested
  //
  vtkDebugMacro(<<"Creating a ruled surface");

  inPts = input->GetPoints();
  inLines = input->GetLines();
  if ( !inPts || !inLines)
  {
    return 1;
  }
  numLines=inLines->GetNumberOfCells();
  numPts = inPts->GetNumberOfPoints();
  if (numPts < 1 || numLines < 2 )
  {
    return 1;
  }

  if ( this->PassLines )
  {
    output->SetLines(inLines);
  }

  if ( this->RuledMode == VTK_RULED_MODE_RESAMPLE ) //generating new points
  {
    newPts = vtkPoints::New();
    output->SetPoints(newPts);
    outPD->InterpolateAllocate(inPD,numPts);
    if ( this->PassLines ) //need to copy input points
    {
      newPts->DeepCopy(inPts);
      for (i=0; i<numPts; i++)
      {
        outPD->CopyData(inPD,i,i);
      }
    }
    newPts->Delete();
    newStrips = vtkCellArray::New();
    newStrips->Allocate(
      2*(this->Resolution[1]+1)*this->Resolution[0]*(numLines-1));
    output->SetStrips(newStrips);
    newStrips->Delete();
  }
  else //using original points
  {
    output->SetPoints(inPts);
    output->GetPointData()->PassData(input->GetPointData());
    newPolys = vtkCellArray::New();
    newPolys->Allocate(2*numPts);
    output->SetPolys(newPolys);
    newPolys->Delete();
  }

  // For each pair of lines (as selected by Offset and OnRatio), create a
  // stripe (a ruled surfac between two lines).
  //
  inLines->InitTraversal();
  inLines->GetNextCell(npts,pts);
  for (i=0; i<numLines; i++)
  {
    //abort/progress methods
    this->UpdateProgress ((double)i/numLines);
    if (this->GetAbortExecute())
    {
      break; //out of line loop
    }

    inLines->GetNextCell(npts2,pts2); //get the next edge

    // Determine whether this stripe should be generated
    if ( (i-this->Offset) >= 0 && !((i - this->Offset ) % this->OnRatio) &&
      npts >= 2 && npts2 >= 2)
    {
      switch (this->RuledMode)
      {
        case VTK_RULED_MODE_RESAMPLE:
          this->Resample(output, input, inPts, newPts, npts, pts, npts2, pts2);
          break;
        case VTK_RULED_MODE_POINT_WALK:
          this->PointWalk(output, inPts, npts, pts, npts2, pts2);
          break;
      }//switch
    }//generate this stripe

    //Get the next line for generating the next stripe
    npts = npts2;
    pts = pts2;
    if ( i == (numLines-2) )
    {
      if ( this->CloseSurface )
      {
        inLines->InitTraversal();
      }
      else
      {
        i++; //will cause the loop to end
      }
    }//add far boundary of surface
  }//for all selected line pairs

  return 1;
}

void  vtkRuledSurfaceFilter::Resample(vtkPolyData *output, vtkPolyData *input,
                                      vtkPoints *inPts, vtkPoints *newPts,
                                      int npts, vtkIdType *pts,
                                      int npts2, vtkIdType *pts2)
{
  vtkIdType offset, id;
  int i, j;
  double length, length2;
  vtkCellArray *newStrips;
  vtkPointData *inPD=input->GetPointData();
  vtkPointData *outPD=output->GetPointData();
  double v, uu, vv;
  double deltaV;
  double deltaS, deltaT;
  double d0, d1, l0, l1;
  double pt[3], pt0[3], pt1[3], pt00[3], pt01[3], pt10[3], pt11[3];
  int i00, i01, i10, i11;

  if (this->Resolution[0] < 1)
  {
    vtkErrorMacro(<< "Resolution[0] must be greater than 0");
    return;
  }
  if (this->Resolution[1] < 1)
  {
    vtkErrorMacro(<< "Resolution[1] must be greater than 0");
    return;
  }

  // Measure the length of each boundary line
  //
  // first line
  length = 0.0;
  for (i=0; i < npts-1; i++)
  {
    inPts->GetPoint(pts[i], pt00);
    inPts->GetPoint(pts[i+1], pt01);
    length += sqrt(vtkMath::Distance2BetweenPoints(pt00, pt01));
  }

  // second line
  length2 = 0.0;
  for (i=0; i < npts2-1; i++)
  {
    inPts->GetPoint(pts2[i], pt00);
    inPts->GetPoint(pts2[i+1], pt01);
    length2 += sqrt(vtkMath::Distance2BetweenPoints(pt00, pt01));
  }

  // Create the ruled surface as a set of triangle strips. Allocate
  // additional memory for new points.
  //
  // forces allocation so that SetPoint() can be safely used
  offset = newPts->GetNumberOfPoints();
  newPts->InsertPoint(offset+(this->Resolution[0]+1)*(this->Resolution[1]+1)-1,
                      0.0, 0.0, 0.0);
  newStrips = output->GetStrips();

  // We'll construct the points for the ruled surface in column major order,
  // i.e. all the points between the first point of the two polylines.
  for (i=0; i < this->Resolution[0]; i++)
  {
    newStrips->InsertNextCell( 2*(this->Resolution[1]+1) );
    for (j=0; j < this->Resolution[1] + 1; j++)
    {
      newStrips->InsertCellPoint(offset + i*(this->Resolution[1]+1)+j);
      newStrips->InsertCellPoint(offset + (i+1)*(this->Resolution[1]+1)+j);
    }
  }

  // Now, compute all the points.
  //
  // parametric delta
  deltaV = 1.0 / (double) (this->Resolution[1]);

  // arc-length deltas
  deltaS = length / (double) (this->Resolution[0]);
  deltaT = length2 / (double) (this->Resolution[0]);

  d0 = d1 = 0.0;
  l0 = l1 = 0.0;
  i00 = 0;
  i01 = 1;
  i10 = 0;
  i11 = 1;

  inPts->GetPoint(pts[0], pt00);
  inPts->GetPoint(pts[1], pt01);
  inPts->GetPoint(pts2[0], pt10);
  inPts->GetPoint(pts2[1], pt11);

  for (i=0; i < this->Resolution[0]+1; i++)
  {
    // compute the end points a rule, one point from the first polyline,
    // one point from the second line
    double s = i*deltaS;
    double t = i*deltaT;

    // find for the interval containing s
    while (s > l0 && i00 < (npts - 1))
    {
      inPts->GetPoint(pts[i00], pt00);
      inPts->GetPoint(pts[i01], pt01);
      d0 = sqrt(vtkMath::Distance2BetweenPoints(pt00, pt01));
      // doubleing point discrepancy: sgi needs the following test to be
      // s <= length while win32 needs it to be s < length.  We account
      // for this by using the <= test here and adjusting the maximum parameter
      // value below (see #1)
      if ((s > l0 + d0) && (s <= length))
      {
        // s's interval is still to the right
        l0 += d0;
        i00++;
        i01++;
      }
      else
      {
        // found the correct interval
        break;
      }
    }

    // compute the point at s on the first polyline
    if (i01 > (npts - 1))
    {
      i00--;
      i01--;
    }
    this->Ids->SetId(0,pts[i00]);
    this->Ids->SetId(1,pts[i01]);
    if (d0 == 0.0)
    {
      uu = 0.0;
    }
    else
    {
      uu = (s - l0) / d0;
    }
    // #1: fix to address the win32/sgi doubleing point differences
    if (s >= length)
    {
      uu = 1.0;
    }
    pt0[0] = (1.0-uu) * pt00[0] + uu * pt01[0];
    pt0[1] = (1.0-uu) * pt00[1] + uu * pt01[1];
    pt0[2] = (1.0-uu) * pt00[2] + uu * pt01[2];

    // find for the interval containing t
    while (t > l1 && i10 < (npts2 - 1))
    {
      inPts->GetPoint(pts2[i10], pt10);
      inPts->GetPoint(pts2[i11], pt11);
      d1 = sqrt(vtkMath::Distance2BetweenPoints(pt10, pt11));
      // doubleing point discrepancy: sgi needs the following test to be
      // t <= length2 while win32 needs it to be t < length2.  We account
      // for this by using the <= test here and adjusting the maximum parameter
      // value below (see #1)
      if ((t > l1 + d1) && (t <= length2))
      {
        // t's interval is still to the right
        l1 += d1;
        i10++;
        i11++;
      }
      else
      {
        // found the correct interval
        break;
      }
    }
    // compute the point at t on the second polyline
    if (i11 > npts2 - 1)
    {
      i10--;
      i11--;
    }
    this->Ids->SetId(2,pts2[i10]);
    this->Ids->SetId(3,pts2[i11]);
    if (d1 == 0.0)
    {
      vv = 0.0;
    }
    else
    {
      vv = (t - l1) / d1;
    }
    // #1: fix to address the win32/sgi doubleing point differences
    if (t >= length2)
    {
      vv = 1.0;
    }
    pt1[0] = (1.0-vv) * pt10[0] + vv * pt11[0];
    pt1[1] = (1.0-vv) * pt10[1] + vv * pt11[1];
    pt1[2] = (1.0-vv) * pt10[2] + vv * pt11[2];

    // Now, compute the points along the rule
    for (j=0; j < this->Resolution[1]+1; j++)
    {
      v = j*deltaV;
      pt[0] = (1.0 - v) * pt0[0] + v * pt1[0];
      pt[1] = (1.0 - v) * pt0[1] + v * pt1[1];
      pt[2] = (1.0 - v) * pt0[2] + v * pt1[2];

      id = offset + i*(this->Resolution[1] + 1) + j;
      newPts->SetPoint(id, pt);
      this->Weights[0] = (1.0 - v)*(1.0 - uu);
      this->Weights[1] = (1.0 - v)*uu;
      this->Weights[2] = v*(1.0 - vv);
      this->Weights[3] = v*vv;
      outPD->InterpolatePoint(inPD, id, this->Ids, this->Weights);
    }
  }
}

void  vtkRuledSurfaceFilter::PointWalk(vtkPolyData *output, vtkPoints *inPts,
                                       int npts, vtkIdType *pts,
                                       int npts2, vtkIdType *pts2)
{
  int loc, loc2, next2;
  vtkCellArray *newPolys=output->GetPolys();
  double x[3], y[3], a[3], b[3], xa, xb, ya, distance2;

  // Compute distance factor based on first two points
  //

  vtkIdType endLoop2, startLoop2, i;

  if (!this->OrientLoops)
  {
    endLoop2 = npts2 - 1;
    startLoop2 = 0;
    inPts->GetPoint(pts[0],x);
    inPts->GetPoint(pts2[0],y);
    distance2 = vtkMath::Distance2BetweenPoints(x,y) *
      this->DistanceFactor * this->DistanceFactor;
  }
  else
  {
    double minD2;
    startLoop2 = 0;
    inPts->GetPoint(pts[0],x);
    inPts->GetPoint(pts2[0],y);
    minD2 = vtkMath::Distance2BetweenPoints(x,y);
    for (i = 1; i != npts2; i++)
    {
      inPts->GetPoint(pts2[i],y);
      distance2 = vtkMath::Distance2BetweenPoints(x,y);
      if (distance2 < minD2)
      {
        minD2 = distance2;
        startLoop2 = i;
      }
    }

    // If the starting point is not 0 then the end is behind us
    if (startLoop2)
    {
      endLoop2 = startLoop2 -1;
    }
    else
    {
      endLoop2 = npts2 -1;
    }

    distance2 = minD2 * this->DistanceFactor * this->DistanceFactor;
  }

  // Walk "edge" along the two lines maintaining closest distance
  // and generating triangles as we go.
  loc = 0;
  loc2 = startLoop2;
  bool endOfLoop2 = false;
  while ( loc < (npts-1) || (!endOfLoop2))
  {

    // Determine the next point in loop 2
    next2 = loc2+1;
    if ((!startLoop2) && (next2 == endLoop2))
    {
      // If we started 0 then when we hit the end of the loop
      // we are done
      endOfLoop2 = true;
    }
    else if (next2 == startLoop2)
    {
      // If we are here we have reached the end of the loop
      // though we need to still process the starting point a second time
      // to close the surface
      endOfLoop2 = true;
    }
    else if (next2 == npts2)
    {
      // The only way we would reach the end of the original
      // loop is if we did not start with the 0th point - since
      // this point is repeated (its the same at the npts2-1 point
      // we need to skip it
      next2 = 1;
    }

    if ( loc >= (npts-1) ) //clamped at end of first line
    {
      inPts->GetPoint(pts[loc],x);
      inPts->GetPoint(pts2[loc2],a);
      inPts->GetPoint(pts2[next2],b);
      xa = vtkMath::Distance2BetweenPoints(x,a);
      xb = vtkMath::Distance2BetweenPoints(x,b);
      if ( xa <= distance2 && xb <= distance2 )
      {
        newPolys->InsertNextCell(3);
        newPolys->InsertCellPoint(pts[loc]); //x
        newPolys->InsertCellPoint(pts2[next2]); //b
        newPolys->InsertCellPoint(pts2[loc2]); //a
      }
      loc2 = next2;
    }
    else if ( loc2 == endLoop2 ) //clamped at end of second line
    {
      inPts->GetPoint(pts[loc],x);
      inPts->GetPoint(pts[loc+1],y);
      inPts->GetPoint(pts2[loc2],a);
      xa = vtkMath::Distance2BetweenPoints(x,a);
      ya = vtkMath::Distance2BetweenPoints(y,a);
      if ( xa <= distance2 && ya <= distance2 )
      {
        newPolys->InsertNextCell(3);
        newPolys->InsertCellPoint(pts[loc]); //x
        newPolys->InsertCellPoint(pts[loc+1]); //y
        newPolys->InsertCellPoint(pts2[loc2]); //a
      }
      loc++;
    }
    else //not at either end
    {
      inPts->GetPoint(pts[loc],x);
      inPts->GetPoint(pts[loc+1],y);
      inPts->GetPoint(pts2[loc2],a);
      inPts->GetPoint(pts2[next2],b);
      xa = vtkMath::Distance2BetweenPoints(x,a);
      xb = vtkMath::Distance2BetweenPoints(x,b);
      ya = vtkMath::Distance2BetweenPoints(a,y);
      if ( xb <= ya )
      {
        if ( xb <= distance2 && xa <= distance2 )
        {
          newPolys->InsertNextCell(3);
          newPolys->InsertCellPoint(pts[loc]); //x
          newPolys->InsertCellPoint(pts2[next2]); //b
          newPolys->InsertCellPoint(pts2[loc2]); //a
        }
        loc2 = next2;
      }
      else
      {
        if ( ya <= distance2 && xa <= distance2 )
        {
          newPolys->InsertNextCell(3);
          newPolys->InsertCellPoint(pts[loc]); //x
          newPolys->InsertCellPoint(pts[loc+1]); //y
          newPolys->InsertCellPoint(pts2[loc2]); //a
        }
        loc++;
      }
    }//where in the lines
  }//while still building the stripe
}

const char *vtkRuledSurfaceFilter::GetRuledModeAsString(void)
{
  if ( this->RuledMode == VTK_RULED_MODE_RESAMPLE )
  {
    return "Resample";
  }
  else //if ( this->RuledMode == VTK_RULED_MODE_POINT_WALK )
  {
    return "PointWalk";
  }
}

void vtkRuledSurfaceFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Distance Factor: " << this->DistanceFactor << "\n";
  os << indent << "On Ratio: " << this->OnRatio << "\n";
  os << indent << "Offset: " << this->Offset << "\n";
  os << indent << "Close Surface: " << (this->CloseSurface ? "On\n" : "Off\n");
  os << indent << "Ruled Mode: " << this->GetRuledModeAsString() << "\n";
  os << indent << "Resolution: (" << this->Resolution[0]
     << ", " << this->Resolution[1] << ")" << endl;
  os << indent << "Orient Loops: " << (this->OrientLoops ? "On\n" : "Off\n");
  os << indent << "Pass Lines: " << (this->PassLines ? "On\n" : "Off\n");
}

