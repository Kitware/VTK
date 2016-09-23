/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResliceCursor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkResliceCursor.h"
#include "vtkMath.h"
#include "vtkImageData.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkPlane.h"
#include "vtkPlaneCollection.h"
#include "vtkCellArray.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <cmath>


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkResliceCursor);
vtkCxxSetObjectMacro(vtkResliceCursor, Image, vtkImageData);

//----------------------------------------------------------------------------
vtkResliceCursor::vtkResliceCursor()
{
  this->XAxis[0]                = 1.0;
  this->XAxis[1]                = 0.0;
  this->XAxis[2]                = 0.0;

  this->YAxis[0]                = 0.0;
  this->YAxis[1]                = 1.0;
  this->YAxis[2]                = 0.0;

  this->ZAxis[0]                = 0.0;
  this->ZAxis[1]                = 0.0;
  this->ZAxis[2]                = 1.0;

  this->Center[0]               = 0.0;
  this->Center[1]               = 0.0;
  this->Center[2]               = 0.0;

  this->Thickness[0]            = 0.0;
  this->Thickness[1]            = 0.0;
  this->Thickness[2]            = 0.0;

  this->HoleWidth               = 5.0;
  this->HoleWidthInPixels       = 16.0;
  this->Hole                    = 1;

  this->ThickMode               = 1;

  this->Image                   = NULL;

  this->PolyData                = vtkPolyData::New();
  vtkSmartPointer< vtkPoints > points
    = vtkSmartPointer< vtkPoints >::New();
  vtkSmartPointer< vtkCellArray > lines
    = vtkSmartPointer< vtkCellArray >::New();
  this->PolyData->SetPoints(points);
  this->PolyData->SetLines(lines);

  this->ReslicePlanes           = vtkPlaneCollection::New();

  // Reslice planes along the X, Y and Z axes. And the centerline and slab
  // polydata.

  for (int i = 0; i < 3; i++)
  {
    vtkSmartPointer< vtkPlane > plane = vtkSmartPointer< vtkPlane >::New();
    this->ReslicePlanes->AddItem(plane);

    // Centerline polydata.

    this->CenterlineAxis[i] = vtkPolyData::New();

    vtkSmartPointer< vtkPoints > pointsc
      = vtkSmartPointer< vtkPoints >::New();
    vtkSmartPointer< vtkCellArray > linesc
      = vtkSmartPointer< vtkCellArray >::New();

    this->CenterlineAxis[i]->SetPoints(pointsc);
    this->CenterlineAxis[i]->SetLines(linesc);

  }

  this->ReslicePlanes->GetItem(0)->SetNormal(1,0,0);
  this->ReslicePlanes->GetItem(1)->SetNormal(0,-1,0);
  this->ReslicePlanes->GetItem(2)->SetNormal(0,0,1);

  this->BuildCursorTopology();
}

//----------------------------------------------------------------------------
vtkResliceCursor::~vtkResliceCursor()
{
  this->SetImage(NULL);
  this->PolyData->Delete();
  this->ReslicePlanes->Delete();

  for (int i = 0; i < 3; i++)
  {
    this->CenterlineAxis[i]->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkResliceCursor::BuildCursorTopology()
{
  if (this->Hole)
  {
    this->BuildCursorTopologyWithHole();
  }
  else
  {
    this->BuildCursorTopologyWithoutHole();
  }
}

//----------------------------------------------------------------------------
void vtkResliceCursor::BuildCursorTopologyWithoutHole()
{
  vtkIdType ptIds[2];

  for (int i = 0; i < 3; i++)
  {
    this->CenterlineAxis[i]->GetPoints()->SetNumberOfPoints(2);
    this->CenterlineAxis[i]->GetLines()->Reset();

    ptIds[0] = 0;
    ptIds[1] = 1;
    this->CenterlineAxis[i]->GetLines()->InsertNextCell(2, ptIds);
  }
}

//----------------------------------------------------------------------------
void vtkResliceCursor::BuildCursorTopologyWithHole()
{
  vtkIdType ptIds[2];

  for (int i = 0; i < 3; i++)
  {
    this->CenterlineAxis[i]->GetPoints()->SetNumberOfPoints(4);
    this->CenterlineAxis[i]->GetLines()->Reset();

    ptIds[0] = 0;
    ptIds[1] = 1;
    this->CenterlineAxis[i]->GetLines()->InsertNextCell(2, ptIds);
    ptIds[0] = 2;
    ptIds[1] = 3;
    this->CenterlineAxis[i]->GetLines()->InsertNextCell(2, ptIds);
  }
}

//----------------------------------------------------------------------------
// Reset the cursor to the default position, ie with the axes, normal
// to each other and axis aligned and with the cursor pointed at the
// center of the image.
//
void vtkResliceCursor::Reset()
{
  this->XAxis[0]                = 1.0;
  this->XAxis[1]                = 0.0;
  this->XAxis[2]                = 0.0;

  this->YAxis[0]                = 0.0;
  this->YAxis[1]                = 1.0;
  this->YAxis[2]                = 0.0;

  this->ZAxis[0]                = 0.0;
  this->ZAxis[1]                = 0.0;
  this->ZAxis[2]                = 1.0;

  if (this->GetImage())
  {
    this->GetImage()->GetCenter(this->Center);
  }
  else
  {
    this->Center[0]               = 0.0;
    this->Center[1]               = 0.0;
    this->Center[2]               = 0.0;
  }

  for (int i = 0; i < 3; i++)
  {
    this->GetPlane(i)->SetOrigin(this->Center);
  }

  this->ReslicePlanes->GetItem(0)->SetNormal(1,0,0);
  this->ReslicePlanes->GetItem(1)->SetNormal(0,-1,0);
  this->ReslicePlanes->GetItem(2)->SetNormal(0,0,1);

  this->BuildCursorTopology();
  this->BuildCursorGeometry();

  this->Modified();
}

//----------------------------------------------------------------------------
vtkPlane * vtkResliceCursor::GetPlane( int i )
{
  return this->ReslicePlanes->GetItem(i);
}

//----------------------------------------------------------------------------
vtkPolyData * vtkResliceCursor::GetPolyData()
{
  this->Update();
  return this->PolyData;
}

//----------------------------------------------------------------------------
void vtkResliceCursor::Update()
{
  if (!this->Image)
  {
    vtkErrorMacro( << "Image not set !" );
    return;
  }

  if (this->GetMTime() > this->PolyDataBuildTime)
  {
    this->BuildCursorTopology();
    this->BuildCursorGeometry();
  }
}

//----------------------------------------------------------------------------
void vtkResliceCursor::ComputeAxes()
{
  double normals[3][3];
  for (int i = 0; i < 3; i++)
  {
    this->GetPlane(i)->GetNormal(normals[i]);
  }

  // The axes are the intersections of the plane normals.

  vtkMath::Cross(normals[0], normals[1], this->ZAxis);
  vtkMath::Cross(normals[1], normals[2], this->XAxis);
  vtkMath::Cross(normals[2], normals[0], this->YAxis);
}

//----------------------------------------------------------------------------
void vtkResliceCursor::BuildCursorGeometryWithHole()
{
  this->ComputeAxes();

  double bounds[6];
  this->Image->GetBounds(bounds);

  // Length of the principal diagonal.
  const double pdLength = 20 * 0.5 * sqrt(
    (bounds[1] - bounds[0])*(bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2])*(bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4])*(bounds[5] - bounds[4]));


  // Precompute prior to use within the loop.
  const double holeHalfWidth = this->HoleWidth / 2.0;


  double pts[12][3];
  for (int i = 0; i < 3; i++)
  {
    pts[0][i] = this->Center[i] - pdLength * this->XAxis[i];
    pts[1][i] = this->Center[i] + pdLength * this->XAxis[i];
    pts[2][i] = this->Center[i] - pdLength * this->YAxis[i];
    pts[3][i] = this->Center[i] + pdLength * this->YAxis[i];
    pts[4][i] = this->Center[i] - pdLength * this->ZAxis[i];
    pts[5][i] = this->Center[i] + pdLength * this->ZAxis[i];

    // Break in the polydata to satisfy the hole

    pts[6][i] = this->Center[i] - holeHalfWidth * this->XAxis[i];
    pts[7][i] = this->Center[i] + holeHalfWidth * this->XAxis[i];
    pts[8][i] = this->Center[i] - holeHalfWidth * this->YAxis[i];
    pts[9][i] = this->Center[i] + holeHalfWidth * this->YAxis[i];
    pts[10][i] = this->Center[i] - holeHalfWidth * this->ZAxis[i];
    pts[11][i] = this->Center[i] + holeHalfWidth * this->ZAxis[i];
  }

  for (int j = 0; j < 3; j++)
  {
    vtkPoints *centerlinePoints = this->CenterlineAxis[j]->GetPoints();
    centerlinePoints->SetPoint(0, pts[2*j]);
    centerlinePoints->SetPoint(1, pts[6+2*j]);
    centerlinePoints->SetPoint(2, pts[6+2*j+1]);
    centerlinePoints->SetPoint(3, pts[2*j+1]);

    this->CenterlineAxis[j]->Modified();
  }

  this->PolyDataBuildTime.Modified();
}

//----------------------------------------------------------------------------
void vtkResliceCursor::BuildCursorGeometryWithoutHole()
{
  this->ComputeAxes();

  double bounds[6];
  this->Image->GetBounds(bounds);

  // Length of the principal diagonal.
  const double pdLength = 20 * 0.5 * sqrt(
    (bounds[1] - bounds[0])*(bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2])*(bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4])*(bounds[5] - bounds[4]));


  // Precompute prior to use within the loop.
  double pts[6][3];
  for (int i = 0; i < 3; i++)
  {
    pts[0][i] = this->Center[i] - pdLength * this->XAxis[i];
    pts[1][i] = this->Center[i] + pdLength * this->XAxis[i];
    pts[2][i] = this->Center[i] - pdLength * this->YAxis[i];
    pts[3][i] = this->Center[i] + pdLength * this->YAxis[i];
    pts[4][i] = this->Center[i] - pdLength * this->ZAxis[i];
    pts[5][i] = this->Center[i] + pdLength * this->ZAxis[i];
  }

  for (int j = 0; j < 3; j++)
  {
    vtkPoints *centerlinePoints = this->CenterlineAxis[j]->GetPoints();
    centerlinePoints->SetPoint(0, pts[2*j]);
    centerlinePoints->SetPoint(1, pts[2*j+1]);

    this->CenterlineAxis[j]->Modified();
  }

  this->PolyDataBuildTime.Modified();
}

//----------------------------------------------------------------------------
void vtkResliceCursor::BuildCursorGeometry()
{
  if (this->Hole)
  {
    this->BuildCursorGeometryWithHole();
  }
  else
  {
    this->BuildCursorGeometryWithoutHole();
  }
}

//----------------------------------------------------------------------------
void vtkResliceCursor::BuildPolyData()
{
  this->ComputeAxes();

  double bounds[6];
  this->Image->GetBounds(bounds);

  // Length of the principal diagonal.
  const double pdLength = 20 * 0.5 * sqrt(
    (bounds[1] - bounds[0])*(bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2])*(bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4])*(bounds[5] - bounds[4]));


  vtkSmartPointer< vtkPoints > points
    = vtkSmartPointer< vtkPoints >::New();
  vtkSmartPointer< vtkCellArray > lines
    = vtkSmartPointer< vtkCellArray >::New();

  // Precompute the half thickness prior to use within the loop.
  const double ht[3] = { this->Thickness[0] / 2.0,
                         this->Thickness[1] / 2.0,
                         this->Thickness[2] / 2.0 };

  points->Allocate(24);
  lines->Allocate(lines->Allocate(lines->EstimateSize(18,4)));

  double pts[30][3];
  for (int i = 0; i < 3; i++)
  {
    pts[0][i] = this->Center[i] - pdLength * this->XAxis[i];
    pts[1][i] = this->Center[i] + pdLength * this->XAxis[i];
    pts[2][i] = pts[0][i] - ht[1] * this->YAxis[i] - ht[2] * this->ZAxis[i];
    pts[3][i] = pts[1][i] - ht[1] * this->YAxis[i] - ht[2] * this->ZAxis[i];
    pts[4][i] = pts[0][i] + ht[1] * this->YAxis[i] - ht[2] * this->ZAxis[i];
    pts[5][i] = pts[1][i] + ht[1] * this->YAxis[i] - ht[2] * this->ZAxis[i];
    pts[6][i] = pts[0][i] + ht[1] * this->YAxis[i] + ht[2] * this->ZAxis[i];
    pts[7][i] = pts[1][i] + ht[1] * this->YAxis[i] + ht[2] * this->ZAxis[i];
    pts[8][i] = pts[0][i] - ht[1] * this->YAxis[i] + ht[2] * this->ZAxis[i];
    pts[9][i] = pts[1][i] - ht[1] * this->YAxis[i] + ht[2] * this->ZAxis[i];

    pts[10][i] = this->Center[i] - pdLength * this->YAxis[i];
    pts[11][i] = this->Center[i] + pdLength * this->YAxis[i];
    pts[12][i] = pts[10][i] - ht[0] * this->XAxis[i] - ht[2] * this->ZAxis[i];
    pts[13][i] = pts[11][i] - ht[0] * this->XAxis[i] - ht[2] * this->ZAxis[i];
    pts[14][i] = pts[10][i] + ht[0] * this->XAxis[i] - ht[2] * this->ZAxis[i];
    pts[15][i] = pts[11][i] + ht[0] * this->XAxis[i] - ht[2] * this->ZAxis[i];
    pts[16][i] = pts[10][i] + ht[0] * this->XAxis[i] + ht[2] * this->ZAxis[i];
    pts[17][i] = pts[11][i] + ht[0] * this->XAxis[i] + ht[2] * this->ZAxis[i];
    pts[18][i] = pts[10][i] - ht[0] * this->XAxis[i] + ht[2] * this->ZAxis[i];
    pts[19][i] = pts[11][i] - ht[0] * this->XAxis[i] + ht[2] * this->ZAxis[i];

    pts[20][i] = this->Center[i] - pdLength * this->ZAxis[i];
    pts[21][i] = this->Center[i] + pdLength * this->ZAxis[i];
    pts[22][i] = pts[20][i] - ht[1] * this->YAxis[i] - ht[0] * this->XAxis[i];
    pts[23][i] = pts[21][i] - ht[1] * this->YAxis[i] - ht[0] * this->XAxis[i];
    pts[24][i] = pts[20][i] + ht[1] * this->YAxis[i] - ht[0] * this->XAxis[i];
    pts[25][i] = pts[21][i] + ht[1] * this->YAxis[i] - ht[0] * this->XAxis[i];
    pts[26][i] = pts[20][i] + ht[1] * this->YAxis[i] + ht[0] * this->XAxis[i];
    pts[27][i] = pts[21][i] + ht[1] * this->YAxis[i] + ht[0] * this->XAxis[i];
    pts[28][i] = pts[20][i] - ht[1] * this->YAxis[i] + ht[0] * this->XAxis[i];
    pts[29][i] = pts[21][i] - ht[1] * this->YAxis[i] + ht[0] * this->XAxis[i];
  }

  vtkIdType ptIds[2];

  vtkIdType facePtIds[6][4] =
    { { 0, 2, 4, 6 }, { 1, 7, 5, 3 }, { 1,3,2,0 }, { 0,6,7,1 }, { 2,3,5,4 }, {6,4,5,7} };

  for (int j = 0; j < 3; j++)
  {

    vtkPoints *centerlinePoints = this->CenterlineAxis[j]->GetPoints();

    for (int i = 0; i < 4; i++)
    {
      ptIds[0] = 10 * j + 2 + 2*i;
      ptIds[1] = ptIds[0] + 1;

      points->InsertNextPoint(pts[ptIds[0]]);
      points->InsertNextPoint(pts[ptIds[1]]);
    }

    centerlinePoints->SetPoint(0, pts[10*j]);
    centerlinePoints->SetPoint(1, pts[10*j+1]);

    vtkSmartPointer< vtkCellArray > slabPolys =
      vtkSmartPointer< vtkCellArray >::New();
    slabPolys->Allocate(slabPolys->EstimateSize(6,4));

    for (int i = 0; i < 6; i++)
    {
      vtkIdType currFacePtIds[4] = {facePtIds[i][0] + 8*j,
                                    facePtIds[i][1] + 8*j,
                                    facePtIds[i][2] + 8*j,
                                    facePtIds[i][3] + 8*j };
      lines->InsertNextCell(4,currFacePtIds);
      slabPolys->InsertNextCell(4,facePtIds[i]);
    }

    this->CenterlineAxis[j]->Modified();
  }

  this->PolyData->SetPolys(lines);

  this->PolyData->SetPoints(points);
  this->PolyData->Modified();

  this->PolyDataBuildTime.Modified();
}

//----------------------------------------------------------------------------
void vtkResliceCursor::SetCenter( double _arg1, double _arg2, double _arg3 )
{
  if (  (this->Center[0] != _arg1)
      ||(this->Center[1] != _arg2)
      ||(this->Center[2] != _arg3))
  {

    // Ensure that the center of the cursor lies within the image bounds.

    if (this->Image)
    {
      double bounds[6];
      this->Image->GetBounds(bounds);
      if (_arg1 < bounds[0] || _arg1 > bounds[1] ||
          _arg2 < bounds[2] || _arg2 > bounds[3] ||
          _arg3 < bounds[4] || _arg3 > bounds[5])
      {
        return;
      }
    }

    this->Center[0] = _arg1;
    this->Center[1] = _arg2;
    this->Center[2] = _arg3;
    this->Modified();

    this->GetPlane(0)->SetOrigin(this->Center);
    this->GetPlane(1)->SetOrigin(this->Center);
    this->GetPlane(2)->SetOrigin(this->Center);
  }
}

//----------------------------------------------------------------------------
void vtkResliceCursor::SetCenter(double _arg[3])
{
  this->SetCenter(_arg[0], _arg[1], _arg[2]);
}

//----------------------------------------------------------------------------
vtkPolyData * vtkResliceCursor::GetCenterlineAxisPolyData( int axis )
{
  this->Update();
  return this->CenterlineAxis[axis];
}

//----------------------------------------------------------------------------
double * vtkResliceCursor::GetAxis( int i )
{
  if (i == 0)
  {
    return this->XAxis;
  }
  else if (i == 1)
  {
    return this->YAxis;
  }
  else
  {
    return this->ZAxis;
  }
}

//----------------------------------------------------------------------------
vtkMTimeType vtkResliceCursor::GetMTime()
{
  vtkMTimeType mTime=this->Superclass::GetMTime();
  for (int i = 0; i < 3; i++)
  {
    vtkMTimeType time = this->GetPlane(i)->GetMTime();
    if (time > mTime)
    {
      mTime = time;
    }
  }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkResliceCursor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Hole: ";
  if (this->Hole)
  {
    os << indent << "On" << "\n";
  }
  else
  {
    os << indent << "Off" << "\n";
  }
  os << indent << "ThickMode: ";
  if (this->ThickMode)
  {
    os << indent << "On" << "\n";
  }
  else
  {
    os << indent << "Off" << "\n";
  }
  os << indent << "HoleWidth: " << this->HoleWidth << endl;
  os << indent << "HoleWidthInPixels: " << this->HoleWidthInPixels << endl;
  os << indent << "Thickness: (" << this->Thickness[0] << ","
     << this->Thickness[1] << "," << this->Thickness[2] << ")" << endl;
  os << indent << "Center: (" << this->Center[0] << "," << this->Center[1]
     << this->Center[2] << endl;
  os << indent << "XAxis: (" << this->XAxis[0] << "," << this->XAxis[1]
     << this->XAxis[2] << endl;
  os << indent << "YAxis: (" << this->YAxis[0] << "," << this->YAxis[1]
     << this->YAxis[2] << endl;
  os << indent << "ZAxis: (" << this->ZAxis[0] << "," << this->ZAxis[1]
     << this->ZAxis[2] << endl;
  os << indent << "Center: (" << this->Center[0] << "," << this->Center[1]
     << this->Center[2] << endl;
  os << indent << "Image: " << this->Image << "\n";
  if (this->Image)
  {
    this->Image->PrintSelf(os, indent);
  }
  os << indent << "PolyData: " << this->PolyData << "\n";
  if (this->PolyData)
  {
    this->PolyData->PrintSelf(os, indent);
  }
  os << indent << "ReslicePlanes: " << this->ReslicePlanes << "\n";
  if (this->ReslicePlanes)
  {
    this->ReslicePlanes->PrintSelf(os, indent);
  }

  // this->PolyDataBuildTime;
  // this->CenterlineAxis[3];

}
