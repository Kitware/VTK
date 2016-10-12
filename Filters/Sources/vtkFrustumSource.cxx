/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFrustumSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFrustumSource.h"
#include "vtkObjectFactory.h"
#include "vtkPlanes.h"
#include "vtkPlane.h"
#include <cassert>
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkCellArray.h"

vtkStandardNewMacro(vtkFrustumSource);
vtkCxxSetObjectMacro(vtkFrustumSource,Planes,vtkPlanes);

// ----------------------------------------------------------------------------
vtkFrustumSource::vtkFrustumSource()
{
  this->Planes=0;
  this->ShowLines=true;
  this->LinesLength=1.0;
  this->OutputPointsPrecision = vtkAlgorithm::SINGLE_PRECISION;

  // a source has no input port.
  this->SetNumberOfInputPorts(0);
}

// ----------------------------------------------------------------------------
vtkFrustumSource::~vtkFrustumSource()
{
  if(this->Planes!=0)
  {
    this->Planes->Delete();
  }
}

// ----------------------------------------------------------------------------
int vtkFrustumSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  if(this->Planes==0 || this->Planes->GetNumberOfPlanes()!=6)
  {
    vtkErrorMacro(<<" 6 planes required.");
    return 0;
  }
  if(this->ShowLines)
  {
    if(this->LinesLength<=0.0)
    {
      vtkErrorMacro(<<" LinesLength<=0.0");
      return 0;
    }
  }

  // get the info object
  vtkInformation *outInfo=outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output=vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Geometry
  vtkIdType nbPts=8;

  bool leftRightNull=false;
  bool bottomTopNull=false;
  bool parallelFrustum=false;

  double n0[3];
  double n1[3];
  double c[3];

  // angle between left and right planes
  this->Planes->GetPlane(0)->GetNormal(n0);
  this->Planes->GetPlane(1)->GetNormal(n1);

  vtkMath::Normalize(n0);
  vtkMath::Normalize(n1);
  vtkMath::Dot(n0,n1);

  vtkMath::Cross(n0,n1,c);
  vtkMath::Norm(c);

   // angle between bottom and top planes
  this->Planes->GetPlane(2)->GetNormal(n0);
  this->Planes->GetPlane(3)->GetNormal(n1);

  vtkMath::Normalize(n0);
  vtkMath::Normalize(n1);
  vtkMath::Dot(n0,n1);

  vtkMath::Cross(n0,n1,c);
  vtkMath::Norm(c);

  if(this->ShowLines)
  {
    double left[3];
    this->Planes->GetPlane(0)->GetNormal(left);
    double right[3];
    this->Planes->GetPlane(1)->GetNormal(right);
    double bottom[3];
    this->Planes->GetPlane(2)->GetNormal(bottom);
    double top[3];
    this->Planes->GetPlane(3)->GetNormal(top);

    double leftRight[3];
    vtkMath::Cross(left,right,leftRight);
    leftRightNull=leftRight[0]==0.0 && leftRight[1]==0.0
      && leftRight[2]==0.0;

     double bottomTop[3];
     vtkMath::Cross(bottom,top,bottomTop);
     bottomTopNull=bottomTop[0]==0.0 && bottomTop[1]==0.0
       && bottomTop[2]==0.0;
     parallelFrustum=leftRightNull && bottomTopNull;

     if(parallelFrustum)
     {
       // start at near points, just add the 4 extra far points.
       nbPts+=4;
     }
     else
     {
       if(leftRightNull || bottomTopNull)
       {
         // two extra starting points , and 4 extra far points.
         nbPts+=6;
       }
       else
       {
         // there is an apex, and 4 extra far points
         nbPts+=5;
       }
     }
     // parallel frustum = bottom//top && left//right
  }

  vtkPoints *newPoints=vtkPoints::New();

  // Set the desired precision for the points in the output.
  if(this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPoints->SetDataType(VTK_DOUBLE);
  }
  else
  {
    newPoints->SetDataType(VTK_FLOAT);
  }

  newPoints->SetNumberOfPoints(nbPts);
  // Ref: Real-Time Rendering, 3rd edition, Thomas Akenine-Moller, Eric Haines,
  // Naty Hoffman, page 783, section 16.17,
  // "Intersection Between Three Planes"

  double pt[3];
  int planes[3];

  planes[0]=0; // left
  planes[1]=2; // bottom
  planes[2]=5; // near 5
  this->ComputePoint(planes,pt);
  newPoints->SetPoint(0,pt);

  planes[0]=1;
  this->ComputePoint(planes,pt);
  newPoints->SetPoint(1,pt);

  planes[1]=3;
  this->ComputePoint(planes,pt);
  newPoints->SetPoint(2,pt);

  planes[0]=0;
  this->ComputePoint(planes,pt);
  newPoints->SetPoint(3,pt);

  planes[1]=2;
  planes[2]=4; //4
  this->ComputePoint(planes,pt);
  newPoints->SetPoint(4,pt);

  planes[0]=1;
  this->ComputePoint(planes,pt);
  newPoints->SetPoint(5,pt);

  planes[1]=3;
  this->ComputePoint(planes,pt);
  newPoints->SetPoint(6,pt);

  planes[0]=0;
  this->ComputePoint(planes,pt);
  newPoints->SetPoint(7,pt);

  newPoints->Modified();

  // Topology
  vtkIdType numPolys=6;
  vtkCellArray *newPolys=vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(numPolys,4));

  vtkIdType pts[4];

  // left
  pts[0]=4;
  pts[1]=0;
  pts[2]=3;
  pts[3]=7;
  newPolys->InsertNextCell(4,pts);

  // right
  pts[0]=1;
  pts[1]=5;
  pts[2]=6;
  pts[3]=2;
  newPolys->InsertNextCell(4,pts);

  // bottom
  pts[0]=0;
  pts[1]=4;
  pts[2]=5;
  pts[3]=1;
  newPolys->InsertNextCell(4,pts);

  // top
  pts[0]=3;
  pts[1]=2;
  pts[2]=6;
  pts[3]=7;
  newPolys->InsertNextCell(4,pts);

  // near
  pts[0]=0;
  pts[1]=1;
  pts[2]=2;
  pts[3]=3;
  newPolys->InsertNextCell(4,pts);

  // far
  pts[0]=4;
  pts[1]=7;
  pts[2]=6;
  pts[3]=5;
  newPolys->InsertNextCell(4,pts);

  vtkCellArray *newLines=0;
  if(this->ShowLines)
  {
    vtkIdType numLines=4;

    newLines=vtkCellArray::New();
    newLines->Allocate(newLines->EstimateSize(numLines,2));

    pts[0]=12; // apex, or first of the two extra near points.

    // line from lower-left corner
    if(parallelFrustum)
    {
      pts[0]=0;
    }
    pts[1]=8;
    newLines->InsertNextCell(2,pts);

    // line from lower-right corner
    if(parallelFrustum)
    {
      ++pts[0];
    }
    else
    {
      if(leftRightNull)
      {
        pts[0]=13;
      }
    }
    ++pts[1];
    newLines->InsertNextCell(2,pts);

    // line from upper-right corner
    if(parallelFrustum)
    {
      ++pts[0];
    }
    else
    {
       if(bottomTopNull)
       {
         pts[0]=13;
       }
    }
    ++pts[1];
    newLines->InsertNextCell(2,pts);

    // line from upper-left corner
    if(parallelFrustum)
    {
      ++pts[0];
    }
    else
    {
      if(leftRightNull)
      {
        pts[0]=12;
      }
    }
    ++pts[1];
    newLines->InsertNextCell(2,pts);
  }

  output->SetPoints(newPoints);
  newPoints->Delete();

  if(newLines!=0)
  {
    newLines->Squeeze(); // since we've estimated size; reclaim some space
    output->SetLines(newLines);
    newLines->Delete();
  }

  newPolys->Squeeze(); // since we've estimated size; reclaim some space
  output->SetPolys(newPolys);
  newPolys->Delete();

  return 1;
}

// Description:
// Compute the intersection of 3 planes.
void vtkFrustumSource::ComputePoint(int planes[3],
                                    double *pt)
{
  // Ref: Real-Time Rendering, 3rd edition, Thomas Akenine-Moller, Eric Haines,
  // Naty Hoffman, page 783, section 16.17,
  // "Intersection Between Three Planes"

  vtkPlane *plane=this->Planes->GetPlane(planes[0]);

  double n0[3];
  double p0[3];
  plane->GetNormal(n0);
  plane->GetOrigin(p0);

  this->Planes->GetPlane(planes[1]); // return the same pointer.

  double n1[3];
  double p1[3];
  plane->GetNormal(n1);
  plane->GetOrigin(p1);


  this->Planes->GetPlane(planes[2]); // return the same pointer.


  double n2[3];
  double p2[3];
  plane->GetNormal(n2);
  plane->GetOrigin(p2);

  double d0=vtkMath::Dot(p0,n0);
  double d1=vtkMath::Dot(p1,n1);
  double d2=vtkMath::Dot(p2,n2);

  double c12[3];
  vtkMath::Cross(n1,n2,c12);
  double c20[3];
  vtkMath::Cross(n2,n0,c20);
  double c01[3];
  vtkMath::Cross(n0,n1,c01);

  double d=vtkMath::Determinant3x3(n0,n1,n2);

  int i=0;
  while(i<3)
  {
    pt[i]=(d0*c12[i]+d1*c20[i]+d2*c01[i])/d;
    ++i;
  }
}

// ----------------------------------------------------------------------------
// Description:
// Modified GetMTime because of Planes.
vtkMTimeType vtkFrustumSource::GetMTime()
{
  vtkMTimeType mTime=this->Superclass::GetMTime();
  if(this->Planes!=0)
  {
    vtkMTimeType time;
    time = this->Planes->GetMTime();
    if(time>mTime)
    {
      mTime=time;
    }
  }
  return mTime;
}

// ----------------------------------------------------------------------------
void vtkFrustumSource::PrintSelf(ostream &os,
                                 vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Planes:";
  if(this->Planes!=0)
  {
    this->Planes->PrintSelf(os,indent);
  }
  else
  {
    os << "(none)" <<endl;
  }

  os << indent << "ShowLines:";
  if(this->ShowLines)
  {
    os << "true" << endl;
  }
  else
  {
    os << "false" << endl;
  }

  os << indent << "LinesLength:" << this->LinesLength << endl;
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision
     << endl;
}
