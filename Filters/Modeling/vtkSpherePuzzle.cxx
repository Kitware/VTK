/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpherePuzzle.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSpherePuzzle.h"

#include "vtkAppendPolyData.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLinearExtrusionFilter.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"

#include <math.h>

vtkStandardNewMacro(vtkSpherePuzzle);

//----------------------------------------------------------------------------
// Construct a new puzzle.
vtkSpherePuzzle::vtkSpherePuzzle()
{
  this->Transform = vtkTransform::New();
  this->Reset();
  this->Active = 0;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
// Construct a new puzzle.
vtkSpherePuzzle::~vtkSpherePuzzle()
{
  this->Transform->Delete();
  this->Transform = NULL;
}

//----------------------------------------------------------------------------
void vtkSpherePuzzle::Reset()
{
  int idx;

  this->Modified();
  for (idx = 0; idx < 32; ++idx)
    {
    this->State[idx] = idx;
    this->PieceMask[idx] = 0;
    }
  this->Transform->Identity();
  for (idx = 0; idx < 4; ++idx)
    {
    this->Colors[0 + idx*8*3] = 255;
    this->Colors[1 + idx*8*3] = 0;
    this->Colors[2 + idx*8*3] = 0;

    this->Colors[3 + idx*8*3] = 255;
    this->Colors[4 + idx*8*3] = 175;
    this->Colors[5 + idx*8*3] = 0;

    this->Colors[6 + idx*8*3] = 255;
    this->Colors[7 + idx*8*3] = 255;
    this->Colors[8 + idx*8*3] = 0;

    this->Colors[9 + idx*8*3] = 0;
    this->Colors[10 + idx*8*3] = 255;
    this->Colors[11 + idx*8*3] = 0;

    this->Colors[12 + idx*8*3] = 0;
    this->Colors[13 + idx*8*3] = 255;
    this->Colors[14 + idx*8*3] = 255;

    this->Colors[15 + idx*8*3] = 0;
    this->Colors[16 + idx*8*3] = 0;
    this->Colors[17 + idx*8*3] = 255;

    this->Colors[18 + idx*8*3] = 175;
    this->Colors[19 + idx*8*3] = 0;
    this->Colors[20 + idx*8*3] = 255;

    this->Colors[21 + idx*8*3] = 255;
    this->Colors[22 + idx*8*3] = 50;
    this->Colors[23 + idx*8*3] = 150;
    }
}

//----------------------------------------------------------------------------
int vtkSpherePuzzle::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // We are about to create/destroy a lot of objects.  Defer garbage
  // collection until we are done.
  vtkGarbageCollector::DeferredCollectionPush();

  int i, j, k, num;
  int color;
  vtkAppendPolyData *append = vtkAppendPolyData::New();
  vtkSphereSource *sphere = vtkSphereSource::New();
  vtkTransformFilter *tf = vtkTransformFilter::New();
  vtkUnsignedCharArray *scalars = vtkUnsignedCharArray::New();
  vtkPolyData *tmp;
  int count = 0;
  unsigned char r, g, b;

  scalars->SetNumberOfComponents(3);

  sphere->SetPhiResolution(4);
  sphere->SetThetaResolution(4);

  tf->SetTransform(this->Transform);
  tf->SetInputConnection(sphere->GetOutputPort());

  for (j = 0; j < 4; ++j)
    {
    for (i = 0; i < 8; ++i)
      {
      color = this->State[count] * 3;
      sphere->SetStartTheta((360.0 * (double)(i) / 8.0));
      sphere->SetEndTheta((360.0 * (double)(i+1) / 8.0));
      sphere->SetStartPhi((180.0 * (double)(j) / 4.0));
      sphere->SetEndPhi((180.0 * (double)(j+1) / 4.0));
      tmp = vtkPolyData::New();
      if (this->PieceMask[count])
        { // Spheres original output is transforms input. Put it back.
        tf->Update();
        tmp->ShallowCopy(tf->GetOutput());
        }
      else
        { // Piece not involved in partial move. Just use the sphere.
        sphere->Update();
        tmp->ShallowCopy(sphere->GetOutput());
        }
      // Now create the colors for the faces.
      num = tmp->GetNumberOfPoints();
      for (k = 0; k < num; ++k)
        {
        r = this->Colors[color];
        g = this->Colors[color+1];
        b = this->Colors[color+2];
        // Lighten the active pieces
        if (this->Active && this->PieceMask[count])
          {
          r = r + (unsigned char)((255 - r) * 0.4);
          g = g + (unsigned char)((255 - g) * 0.4);
          b = b + (unsigned char)((255 - b) * 0.4);
          }
        scalars->InsertNextValue(r);
        scalars->InsertNextValue(g);
        scalars->InsertNextValue(b);
        }

      // append all the pieces.
      append->AddInputData(tmp);
      tmp->FastDelete();
      ++count;
      }
    }

  append->Update();

  // Move the data to the output.
  tmp = output;
  tmp->CopyStructure(append->GetOutput());
  tmp->GetPointData()->PassData(append->GetOutput()->GetPointData());
  tmp->GetPointData()->SetScalars(scalars);

  sphere->Delete();
  scalars->Delete();
  append->Delete();
  tf->Delete();

  // We are done creating/destroying objects.
  vtkGarbageCollector::DeferredCollectionPop();

  return 1;
}

//----------------------------------------------------------------------------
void vtkSpherePuzzle::MarkHorizontal(int section)
{
  int i;

  for (i = 0; i < 32; ++i)
    {
    this->PieceMask[i] = 0;
    }
  // Find the start of the section.
  section = section * 8;
  for (i = 0; i < 8; ++i)
    {
    this->PieceMask[i+section] = 1;
    }
}

//----------------------------------------------------------------------------
void vtkSpherePuzzle::MarkVertical(int section)
{
  int i, j, offset;

  for (i = 0; i < 32; ++i)
    {
    this->PieceMask[i] = 1;
    }
  for (i = 0; i < 4; ++i)
    {
    offset = (section + i) % 8;
    for (j = 0; j < 4; ++j)
      {
      this->PieceMask[offset+(j*8)] = 0;
      }
    }
}

//----------------------------------------------------------------------------
void vtkSpherePuzzle::MoveHorizontal(int slab, int percentage, int rightFlag)
{
  int offset;
  int tmp;
  int i;

  this->Modified();

  // Clear out previous partial moves.
  this->Transform->Identity();
  this->MarkHorizontal(slab);

  // Move zero does nothing.
  if (percentage <= 0)
    {
    return;
    }

  // Offset is used to determine which pieces are involved.
  offset = slab * 8;

  // Move 100 percent changes state.
  if (percentage >= 100)
    { // Just do the state change.
    if (rightFlag)
      {
      tmp = this->State[offset+7];
      for (i = 7; i > 0; --i)
        {
        this->State[i+offset] = this->State[i-1+offset];
        }
      this->State[offset] = tmp;
      }
    else
      {
      tmp = this->State[offset];
      for (i = 0; i < 7; ++i)
        {
        this->State[i+offset] = this->State[i+1+offset];
        }
      this->State[offset+7] = tmp;
      }
    return;
    }

  // Partial move.
  // This does not change the state.  It is ust for animating
  // the move.
  // Setup the pieces that are involved in the move.
  if ( ! rightFlag)
    {
    percentage = -percentage;
    }
  this->Transform->RotateZ(((double)(percentage) / 100.0)
                           * (360.0 / 8.0) );
}

//----------------------------------------------------------------------------
void vtkSpherePuzzle::MoveVertical(int half, int percentage, int rightFlag)
{
  int tmp;
  int off0, off1, off2, off3;
  double theta;

  this->Modified();

  // Clear out previous partial moves.
  this->Transform->Identity();
  this->MarkVertical(half);

  // Move zero does nothing.
  if (percentage <= 0)
    {
    return;
    }

  off0 = (4+half) % 8;
  off1 = (5+half) % 8;
  off2 = (6+half) % 8;
  off3 = (7+half) % 8;

  // Move 100 percent changes state.
  if (percentage >= 100)
    { // Just do the state change.
    tmp = this->State[off0];
    this->State[off0] = this->State[24+off3];
    this->State[24+off3] = tmp;

    tmp = this->State[off1];
    this->State[off1] = this->State[24+off2];
    this->State[24+off2] = tmp;

    tmp = this->State[off2];
    this->State[off2] = this->State[24+off1];
    this->State[24+off1] = tmp;

    tmp = this->State[off3];
    this->State[off3] = this->State[24+off0];
    this->State[24+off0] = tmp;


    tmp = this->State[8+off0];
    this->State[8+off0] = this->State[16+off3];
    this->State[16+off3] = tmp;

    tmp = this->State[8+off1];
    this->State[8+off1] = this->State[16+off2];
    this->State[16+off2] = tmp;

    tmp = this->State[8+off2];
    this->State[8+off2] = this->State[16+off1];
    this->State[16+off1] = tmp;

    tmp = this->State[8+off3];
    this->State[8+off3] = this->State[16+off0];
    this->State[16+off0] = tmp;
    return;
    }

  // Partial move.
  // This does not change the state.  It is use for animating the move.
  if (rightFlag)
    {
    percentage = -percentage;
    }
  theta = (double)(half) * vtkMath::Pi() / 4.0;
  this->Transform->RotateWXYZ(((double)(percentage)/100.0)*(360.0/2.0),
                              sin(theta), -cos(theta), 0.0);

}

//----------------------------------------------------------------------------
int vtkSpherePuzzle::SetPoint(double x, double y, double z)
{
  double pt[3];
  double theta, phi;
  int xi, yi;
  double xp, yp;
  double xn, yn;

  this->Modified();

  if (x < 0.2 && x > -0.2 && y < 0.2 && y > -0.2 && z < 0.2 && z > -0.2)
    {
    this->Active = 0;
    return 0;
    }

  // normalize
  pt[0] = x;
  pt[1] = y;
  pt[2] = z;
  vtkMath::Normalize(pt);

  // Convert this into phi and theta.
  theta = 180.0 - atan2(pt[0], pt[1]) * 180 / vtkMath::Pi();
  phi = 90.0 - asin(pt[2]) * 180 / vtkMath::Pi();

  // Compute the piece the point is in.
  xi = (int)(theta * 8.0 / 360.0);
  yi = (int)(phi * 8 / 360.0);
  xn = (theta/(360.0/8.0)) - (double)(xi);
  yn = (phi/(360.0/8.0)) - (double)(yi);

  vtkDebugMacro("point: " << x << ", " << y << ", " << z);
  vtkDebugMacro("theta: " << theta << ",  phi: " << phi);
  vtkDebugMacro("theta: " << xi << ", " << xn << ",  phi: " << yi << ", " << y);

  xp = 1.0 - xn;
  yp = 1.0 - yn;
  if (xn > 0.2 && xp > 0.2 && yn > 0.2 && yp > 0.2)
    { // Do nothing in the center of the face.
    this->Active = 0;
    return 0;
    }
  this->Active = 1;
  if (xn < xp && xn < yp && xn < yn)
    {
    this->VerticalFlag = 1;
    this->RightFlag = (yn < yp);
    this->Section = xi+2;
    this->MarkVertical(this->Section);
    return this->Section + this->VerticalFlag * 10 + this->RightFlag * 100;
    }
  if (xp < xn && xp < yp && xp < yn)
    {
    this->VerticalFlag = 1;
    this->RightFlag = (yp < yn);
    this->Section = xi+7;
    this->MarkVertical(this->Section);
    return this->Section + this->VerticalFlag * 10 + this->RightFlag * 100;
    }
  // The remaining options move a horizontal slab.
  this->VerticalFlag = 0;
  this->RightFlag = (xn > xp);
  this->Section = yi;
  this->MarkHorizontal(this->Section);
  return this->Section + this->VerticalFlag * 10 + this->RightFlag * 100;
}

//----------------------------------------------------------------------------
void vtkSpherePuzzle::MovePoint(int percentage)
{
  if ( ! this->Active)
    {
    return;
    }
  this->Modified();

  if (this->VerticalFlag)
    {
    this->MoveVertical(this->Section, percentage, this->RightFlag);
    }
  else
    {
    this->MoveHorizontal(this->Section, percentage, this->RightFlag);
    }
}

//----------------------------------------------------------------------------
void vtkSpherePuzzle::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  this->Superclass::PrintSelf(os,indent);

  os << indent << "State: " << this->State[0];
  for (idx = 1; idx < 16; ++idx)
    {
    os << ", " << this->State[idx];
    }
  os << endl;
}
