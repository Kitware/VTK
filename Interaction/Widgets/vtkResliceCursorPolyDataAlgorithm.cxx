/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResliceCursorPolyDataAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkResliceCursorPolyDataAlgorithm.h"

#include "vtkResliceCursor.h"
#include "vtkCutter.h"
#include "vtkBox.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkClipPolyData.h"
#include "vtkExecutive.h"
#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkPlane.h"
#include "vtkSmartPointer.h"
#include "vtkLinearExtrusionFilter.h"
#include "vtkMath.h"
#include <math.h>
#include <algorithm>

vtkStandardNewMacro(vtkResliceCursorPolyDataAlgorithm);
vtkCxxSetObjectMacro(vtkResliceCursorPolyDataAlgorithm, ResliceCursor,
                     vtkResliceCursor )

//---------------------------------------------------------------------------
vtkResliceCursorPolyDataAlgorithm::vtkResliceCursorPolyDataAlgorithm()
{
  this->ResliceCursor = NULL;
  this->ReslicePlaneNormal = vtkResliceCursorPolyDataAlgorithm::XAxis;
  this->Cutter        = vtkCutter::New();
  this->Box           = vtkBox::New();
  this->ClipWithBox   = vtkClipPolyData::New();
  this->Extrude = 0;
  this->ExtrusionFilter1 = vtkLinearExtrusionFilter::New();
  this->ExtrusionFilter2 = vtkLinearExtrusionFilter::New();
  this->ExtrusionFilter2->SetInputConnection(
    this->ExtrusionFilter1->GetOutputPort());

  for (int i = 0; i < 6; i++)
    {
    this->SliceBounds[i] = 0;
    }
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(4);

  for (int i = 0; i < 2; i++)
    {
    this->ThickAxes[i] = vtkPolyData::New();

    vtkSmartPointer< vtkPoints > points
      = vtkSmartPointer< vtkPoints >::New();
    vtkSmartPointer< vtkCellArray > lines
      = vtkSmartPointer< vtkCellArray >::New();

    this->ThickAxes[i]->SetPoints(points);
    this->ThickAxes[i]->SetLines(lines);
    }
}

//---------------------------------------------------------------------------
vtkResliceCursorPolyDataAlgorithm::~vtkResliceCursorPolyDataAlgorithm()
{
  this->SetResliceCursor(NULL);
  this->Cutter->Delete();
  this->Box->Delete();
  this->ClipWithBox->Delete();
  this->ExtrusionFilter1->Delete();
  this->ExtrusionFilter2->Delete();

  for (int i = 0; i < 2; i++)
    {
    this->ThickAxes[i]->Delete();
    }
}

//---------------------------------------------------------------------------
void vtkResliceCursorPolyDataAlgorithm::BuildResliceSlabAxisTopology()
{
  for (int i = 0; i < 2; i++)
    {
    const int nPoints = this->GetResliceCursor()->GetHole() ? 8 : 4;
    this->ThickAxes[i]->GetPoints()->SetNumberOfPoints(nPoints);

    this->ThickAxes[i]->GetLines()->Reset();

    vtkIdType ptIds[2];
    for (int j = 0; j < nPoints/2; j++)
      {
      ptIds[0] = 2*j;
      ptIds[1] = 2*j+1;
      this->ThickAxes[i]->GetLines()->InsertNextCell(2, ptIds);
      }
    }

}

//---------------------------------------------------------------------------
vtkPolyData *vtkResliceCursorPolyDataAlgorithm::GetCenterlineAxis1()
{
  return vtkPolyData::SafeDownCast(
    this->GetExecutive()->GetOutputData(0));
}


//---------------------------------------------------------------------------
vtkPolyData *vtkResliceCursorPolyDataAlgorithm::GetCenterlineAxis2()
{
  return vtkPolyData::SafeDownCast(
    this->GetExecutive()->GetOutputData(1));
}


//---------------------------------------------------------------------------
vtkPolyData *vtkResliceCursorPolyDataAlgorithm::GetThickSlabAxis1()
{
  return vtkPolyData::SafeDownCast(
    this->GetExecutive()->GetOutputData(2));
}


//---------------------------------------------------------------------------
vtkPolyData *vtkResliceCursorPolyDataAlgorithm::GetThickSlabAxis2()
{
  return vtkPolyData::SafeDownCast(
    this->GetExecutive()->GetOutputData(3));
}

//---------------------------------------------------------------------------
int vtkResliceCursorPolyDataAlgorithm::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector))
{
  if (!this->ResliceCursor)
    {
    vtkErrorMacro( << "Reslice Cursor not set !" );
    return -1;
    }

  this->BuildResliceSlabAxisTopology();

  // Cut the reslice cursor with the plane on which we are viewing.

  const int axis1 = this->GetAxis1();
  const int axis2 = this->GetAxis2();

  this->CutAndClip(this->ResliceCursor->GetCenterlineAxisPolyData(axis1),
                                               this->GetCenterlineAxis1());
  this->CutAndClip(this->ResliceCursor->GetCenterlineAxisPolyData(axis2),
                                               this->GetCenterlineAxis2());

  if (this->ResliceCursor->GetThickMode())
    {
    this->GetSlabPolyData(axis1, this->GetPlaneAxis1(), this->ThickAxes[0]);
    this->CutAndClip(this->ThickAxes[0], this->GetThickSlabAxis1());

    this->GetSlabPolyData(axis2, this->GetPlaneAxis2(), this->ThickAxes[1]);
    this->CutAndClip(this->ThickAxes[1], this->GetThickSlabAxis2());
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkResliceCursorPolyDataAlgorithm
::GetSlabPolyData( int axis, int planeAxis, vtkPolyData *pd )
{
  double normal[3], thicknessDirection[3];
  this->ResliceCursor->GetPlane(this->ReslicePlaneNormal)->GetNormal(normal);

  double *axisVector = this->ResliceCursor->GetAxis(axis);
  vtkMath::Cross(normal, axisVector, thicknessDirection);
  vtkMath::Normalize(thicknessDirection);

  const double thickness = this->ResliceCursor->GetThickness()[planeAxis];

  vtkPolyData *cpd = this->ResliceCursor->GetCenterlineAxisPolyData(axis);

  vtkPoints *pts = pd->GetPoints();

  double p[3], pPlus[3], pMinus[3];
  const int nPoints = cpd->GetNumberOfPoints();

  // Set the slab points
  for (int i = 0; i < nPoints; i++)
    {
    cpd->GetPoint(i, p);
    for (int j = 0; j < 3; j++)
      {
      pPlus[j] = p[j] + thickness * thicknessDirection[j];
      pMinus[j] = p[j] - thickness * thicknessDirection[j];
      }
    pts->SetPoint(i, pPlus);
    pts->SetPoint(nPoints + i, pMinus);
    }

  pd->Modified();

}

//---------------------------------------------------------------------------
int vtkResliceCursorPolyDataAlgorithm::GetAxis1()
{
  if (this->ReslicePlaneNormal == 2)
    {
    return 1;
    }
  if (this->ReslicePlaneNormal == 1)
    {
    return 2;
    }
  return 2;
}

//---------------------------------------------------------------------------
int vtkResliceCursorPolyDataAlgorithm::GetAxis2()
{
  if (this->ReslicePlaneNormal == 2)
    {
    return 0;
    }
  if (this->ReslicePlaneNormal == 1)
    {
    return 0;
    }
  return 1;
}

//---------------------------------------------------------------------------
int vtkResliceCursorPolyDataAlgorithm::GetPlaneAxis1()
{
  if (this->ReslicePlaneNormal == 2)
    {
    return 0;
    }
  if (this->ReslicePlaneNormal == 1)
    {
    return 0;
    }
  return 1;
}

//---------------------------------------------------------------------------
int vtkResliceCursorPolyDataAlgorithm::GetPlaneAxis2()
{
  if (this->ReslicePlaneNormal == 2)
    {
    return 1;
    }
  if (this->ReslicePlaneNormal == 1)
    {
    return 2;
    }
  return 2;
}

//---------------------------------------------------------------------------
int vtkResliceCursorPolyDataAlgorithm::GetOtherPlaneForAxis( int p )
{
  for (int i = 0; i < 3; i++)
    {
    if (i != p && i != this->ReslicePlaneNormal)
      {
      return i;
      }
    }

  return -1;
}

//---------------------------------------------------------------------------
void vtkResliceCursorPolyDataAlgorithm
::CutAndClip( vtkPolyData * input, vtkPolyData * output )
{
  this->ClipWithBox->SetClipFunction(this->Box);
  this->ClipWithBox->GenerateClipScalarsOff();
  this->ClipWithBox->GenerateClippedOutputOff();
  this->Box->SetBounds(this->ResliceCursor->GetImage()->GetBounds());

  double s[3];
  this->ResliceCursor->GetImage()->GetSpacing(s);
  const double smax = std::max(std::max(s[0], s[1]), s[2]);
  this->ExtrusionFilter1->SetScaleFactor(smax);
  this->ExtrusionFilter2->SetScaleFactor(smax);

  this->ClipWithBox->SetInputData(input);
  this->ClipWithBox->Update();
  this->ExtrusionFilter1->SetInputData(input);

  double normal[3];
  this->ResliceCursor->GetPlane(this->ReslicePlaneNormal)->GetNormal(normal);
  this->ExtrusionFilter1->SetVector(normal);
  this->ExtrusionFilter2->SetVector(-normal[0], -normal[1], -normal[2]);
  //std::cout << normal[0] << " " << normal[1] << " " << normal[2] << std::endl;

  this->ExtrusionFilter2->Update();

  output->DeepCopy(this->ExtrusionFilter2->GetOutput());
}

//-------------------------------------------------------------------------
unsigned long int vtkResliceCursorPolyDataAlgorithm::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  if (this->ResliceCursor)
    {
    unsigned long time;
    time = this->ResliceCursor->GetMTime();
    if (time > mTime)
      {
      mTime = time;
      }
    }
  return mTime;
}

//---------------------------------------------------------------------------
void vtkResliceCursorPolyDataAlgorithm::PrintSelf(
                    ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ResliceCursor: " << this->ResliceCursor << "\n";
  if (this->ResliceCursor)
    {
    this->ResliceCursor->PrintSelf(os, indent);
    }
  os << indent << "Cutter: " << this->Cutter << "\n";
  if (this->Cutter)
    {
    this->Cutter->PrintSelf(os, indent);
    }
  os << indent << "ExtrusionFilter1: " << this->ExtrusionFilter1 << "\n";
  if (this->ExtrusionFilter1)
    {
    this->ExtrusionFilter1->PrintSelf(os, indent);
    }
  os << indent << "ExtrusionFilter2: " << this->ExtrusionFilter2 << "\n";
  if (this->ExtrusionFilter2)
    {
    this->ExtrusionFilter2->PrintSelf(os, indent);
    }
  os << indent << "ReslicePlaneNormal: " << this->ReslicePlaneNormal << endl;
  os << indent << "Extrude: " << this->Extrude << endl;

  // this->SliceBounds;
  // this->ThickAxes[2];
  // this->Box;
  // this->ClipWithBox;
  // this->SlicePlane;

}
