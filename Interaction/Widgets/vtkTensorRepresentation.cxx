/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTensorRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTensorRepresentation.h"

#include "vtkActor.h"
#include "vtkAssemblyPath.h"
#include "vtkBoundingBox.h"
#include "vtkBox.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellPicker.h"
#include "vtkDoubleArray.h"
#include "vtkEventData.h"
#include "vtkInteractorObserver.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPickingManager.h"
#include "vtkPlane.h"
#include "vtkPlanes.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkQuaternion.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkVectorOperators.h"
#include "vtkWindow.h"

#include <cassert>

vtkStandardNewMacro(vtkTensorRepresentation);

//------------------------------------------------------------------------------
vtkTensorRepresentation::vtkTensorRepresentation()
{
  // The current tensor and derivative information
  std::fill_n(this->Tensor, 9, 0.0);
  this->Tensor[0] = this->Tensor[5] = this->Tensor[8] = 1;
  std::fill_n(this->Eigenvalues, 3, 1.0);
  std::fill_n(this->Eigenvectors[0], 3, 0.0);
  std::fill_n(this->Eigenvectors[1], 3, 0.0);
  std::fill_n(this->Eigenvectors[2], 3, 0.0);
  this->Eigenvectors[0][0] = this->Eigenvectors[1][1] = this->Eigenvectors[2][2] = 1.0;
  std::fill_n(this->TensorPosition, 3, 0.0);

  // Internal data members for performance
  this->Transform = vtkTransform::New();
  this->Matrix = vtkMatrix4x4::New();
  this->TmpPoints = vtkPoints::New(VTK_DOUBLE);
  this->PlanePoints = vtkPoints::New(VTK_DOUBLE);
  this->PlanePoints->SetNumberOfPoints(6);
  this->PlaneNormals = vtkDoubleArray::New();
  this->PlaneNormals->SetNumberOfComponents(3);
  this->PlaneNormals->SetNumberOfTuples(6);

  // The initial state
  this->InteractionState = vtkTensorRepresentation::Outside;
  this->SnappedOrientation[0] = false;
  this->SnappedOrientation[1] = false;
  this->SnappedOrientation[2] = false;
  this->SnapToAxes = false;

  // Handle size is in pixels for this widget
  this->HandleSize = 5.0;

  // Control visibility of features
  this->InsideOut = false;
  this->OutlineFaceWires = false;
  this->OutlineCursorWires = true;
  this->TensorEllipsoid = true;

  for (int i = 0; i < 6; ++i)
  {
    this->Planes[i] = vtkPlane::New();
  }

  // Set up the initial properties
  this->CreateDefaultProperties();

  // Construct the poly data representing the hex
  this->HexPolyData = vtkPolyData::New();
  this->HexMapper = vtkPolyDataMapper::New();
  this->HexMapper->SetInputData(HexPolyData);
  this->HexActor = vtkActor::New();
  this->HexActor->SetMapper(this->HexMapper);
  this->HexActor->SetProperty(this->OutlineProperty);

  // Construct initial points. The first 8 points are the cube vertices
  // (0,1,2,3 is bottom face in counterclockwise order; the next four
  // points 4,5,6,7 is the top face in counterclockwise order; points
  // 8,9 are -/+ xfaces; 10,11 are -/+ yfaces; 12,13 are -/+ zfaces.
  // Point 14 is the center point.
  this->Points = vtkPoints::New(VTK_DOUBLE);
  this->Points->SetNumberOfPoints(15); // 8 corners; 6 faces; 1 center
  this->HexPolyData->SetPoints(this->Points);

  // Construct connectivity for the faces. These are used to perform the
  // face picking. Hex is defined like vtkHexahedron (0,1,2,3) bottom face;
  // (4,5,6,7) top face, ordered so normals point out.
  int i;
  vtkIdType pts[4];
  vtkCellArray* cells = vtkCellArray::New();
  cells->AllocateEstimate(6, 4);
  pts[0] = 3;
  pts[1] = 0;
  pts[2] = 4;
  pts[3] = 7;
  cells->InsertNextCell(4, pts);
  pts[0] = 1;
  pts[1] = 2;
  pts[2] = 6;
  pts[3] = 5;
  cells->InsertNextCell(4, pts);
  pts[0] = 0;
  pts[1] = 1;
  pts[2] = 5;
  pts[3] = 4;
  cells->InsertNextCell(4, pts);
  pts[0] = 2;
  pts[1] = 3;
  pts[2] = 7;
  pts[3] = 6;
  cells->InsertNextCell(4, pts);
  pts[0] = 0;
  pts[1] = 3;
  pts[2] = 2;
  pts[3] = 1;
  cells->InsertNextCell(4, pts);
  pts[0] = 4;
  pts[1] = 5;
  pts[2] = 6;
  pts[3] = 7;
  cells->InsertNextCell(4, pts);
  this->HexPolyData->SetPolys(cells);
  cells->Delete();
  this->HexPolyData->BuildCells();

  // The face of the hexahedra
  cells = vtkCellArray::New();
  cells->AllocateEstimate(1, 4);
  cells->InsertNextCell(4, pts); // temporary, replaced later
  this->HexFacePolyData = vtkPolyData::New();
  this->HexFacePolyData->SetPoints(this->Points);
  this->HexFacePolyData->SetPolys(cells);
  this->HexFaceMapper = vtkPolyDataMapper::New();
  this->HexFaceMapper->SetInputData(HexFacePolyData);
  this->HexFace = vtkActor::New();
  this->HexFace->SetMapper(this->HexFaceMapper);
  this->HexFace->SetProperty(this->FaceProperty);
  cells->Delete();

  // Create the outline for the hex
  this->OutlinePolyData = vtkPolyData::New();
  this->OutlinePolyData->SetPoints(this->Points);
  this->OutlineMapper = vtkPolyDataMapper::New();
  this->OutlineMapper->SetInputData(this->OutlinePolyData);
  this->HexOutline = vtkActor::New();
  this->HexOutline->SetMapper(this->OutlineMapper);
  this->HexOutline->SetProperty(this->OutlineProperty);
  cells = vtkCellArray::New();
  cells->AllocateEstimate(15, 2);
  this->OutlinePolyData->SetLines(cells);
  cells->Delete();

  // Create the outline
  this->GenerateOutline();

  // Create the handles
  this->Handle = new vtkActor*[7];
  this->HandleMapper = new vtkPolyDataMapper*[7];
  this->HandleGeometry = new vtkSphereSource*[7];
  for (i = 0; i < 7; i++)
  {
    this->HandleGeometry[i] = vtkSphereSource::New();
    this->HandleGeometry[i]->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
    this->HandleGeometry[i]->SetThetaResolution(16);
    this->HandleGeometry[i]->SetPhiResolution(8);
    this->HandleMapper[i] = vtkPolyDataMapper::New();
    this->HandleMapper[i]->SetInputConnection(this->HandleGeometry[i]->GetOutputPort());
    this->Handle[i] = vtkActor::New();
    this->Handle[i]->SetMapper(this->HandleMapper[i]);
    this->Handle[i]->SetProperty(this->HandleProperty);
  }

  // Create the optional tensor glyph
  this->EllipsoidSource = vtkSphereSource::New();
  this->EllipsoidSource->SetRadius(1.0); // easier scaling and such
  this->EllipsoidSource->SetThetaResolution(128);
  this->EllipsoidSource->SetPhiResolution(64);
  this->EllipsoidMapper = vtkPolyDataMapper::New();
  this->EllipsoidMapper->SetInputConnection(this->EllipsoidSource->GetOutputPort());
  this->EllipsoidTransform = vtkTransform::New();
  this->EllipsoidMatrix = vtkMatrix4x4::New();
  this->EllipsoidActor = vtkActor::New();
  this->EllipsoidActor->SetMapper(this->EllipsoidMapper);
  this->EllipsoidActor->SetProperty(this->EllipsoidProperty);
  this->EllipsoidActor->SetUserTransform(this->EllipsoidTransform);

  // Define the point coordinates
  double bounds[6];
  bounds[0] = -0.5;
  bounds[1] = 0.5;
  bounds[2] = -0.5;
  bounds[3] = 0.5;
  bounds[4] = -0.5;
  bounds[5] = 0.5;
  // Points 8-14 are down by PositionHandles();
  this->BoundingBox = vtkBox::New();
  this->PlaceWidget(bounds);

  // Manage the picking stuff
  this->HandlePicker = vtkCellPicker::New();
  this->HandlePicker->SetTolerance(0.001);
  for (i = 0; i < 7; i++)
  {
    this->HandlePicker->AddPickList(this->Handle[i]);
  }
  this->HandlePicker->PickFromListOn();

  this->HexPicker = vtkCellPicker::New();
  this->HexPicker->SetTolerance(0.001);
  this->HexPicker->AddPickList(HexActor);
  this->HexPicker->PickFromListOn();

  this->CurrentHandle = nullptr;

  this->TranslationAxis = Axis::NONE;
}

//------------------------------------------------------------------------------
vtkTensorRepresentation::~vtkTensorRepresentation()
{
  this->HexActor->Delete();
  this->HexMapper->Delete();
  this->HexPolyData->Delete();
  this->TmpPoints->Delete();
  this->Points->Delete();

  this->HexFace->Delete();
  this->HexFaceMapper->Delete();
  this->HexFacePolyData->Delete();

  this->HexOutline->Delete();
  this->OutlineMapper->Delete();
  this->OutlinePolyData->Delete();

  for (int i = 0; i < 7; i++)
  {
    this->HandleGeometry[i]->Delete();
    this->HandleMapper[i]->Delete();
    this->Handle[i]->Delete();
  }
  delete[] this->Handle;
  delete[] this->HandleMapper;
  delete[] this->HandleGeometry;

  this->EllipsoidActor->Delete();
  this->EllipsoidTransform->Delete();
  this->EllipsoidMatrix->Delete();
  this->EllipsoidMapper->Delete();
  this->EllipsoidSource->Delete();

  this->HandlePicker->Delete();
  this->HexPicker->Delete();

  this->Transform->Delete();
  this->Matrix->Delete();
  this->BoundingBox->Delete();
  this->PlanePoints->Delete();
  this->PlaneNormals->Delete();

  this->HandleProperty->Delete();
  this->SelectedHandleProperty->Delete();
  this->FaceProperty->Delete();
  this->SelectedFaceProperty->Delete();
  this->OutlineProperty->Delete();
  this->SelectedOutlineProperty->Delete();
  this->EllipsoidProperty->Delete();

  for (int i = 0; i < 6; ++i)
  {
    this->Planes[i]->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::SetSymmetricTensor(double symTensor[6])
{
  double tensor[9];
  vtkMath::TensorFromSymmetricTensor(symTensor, tensor);
  this->SetTensor(tensor);
}

//------------------------------------------------------------------------------
// Given a 3x3 symmetric tensor, update the widget accordingly.
void vtkTensorRepresentation::SetTensor(double tensor[9])
{
  std::copy(tensor, tensor + 9, this->Tensor);

  // Evaluate eigenfunctions: set up working matrices
  double *m[3], *v[3];
  double m0[3], m1[3], m2[3];
  m[0] = m0;
  m[1] = m1;
  m[2] = m2;
  v[0] = this->Eigenvectors[0];
  v[1] = this->Eigenvectors[1];
  v[2] = this->Eigenvectors[2];

  for (auto j = 0; j < 3; j++)
  {
    for (auto i = 0; i < 3; i++)
    {
      m[i][j] = 0.5 * (tensor[i + 3 * j] + tensor[j + 3 * i]);
    }
  }
  vtkMath::Jacobi(m, this->Eigenvalues, v);

  // Now update the widget/representation from the tensor
  this->PositionHandles();
  this->UpdateWidgetFromTensor();
}

//------------------------------------------------------------------------------
// Set the position of the tensor. This means translating the representation.
void vtkTensorRepresentation::SetPosition(double pos[3])
{
  double* pts = static_cast<vtkDoubleArray*>(this->Points->GetData())->GetPointer(0);
  double* center = pts + 3 * 14;
  this->Translate(center, pos);
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::GetPolyData(vtkPolyData* pd)
{
  pd->SetPoints(this->HexPolyData->GetPoints());
  pd->SetPolys(this->HexPolyData->GetPolys());
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::StartWidgetInteraction(double e[2])
{
  // Store the start position
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->StartEventPosition[2] = 0.0;

  // Store the start position
  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;

  this->ComputeInteractionState(static_cast<int>(e[0]), static_cast<int>(e[1]), 0);
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::StartComplexInteraction(
  vtkRenderWindowInteractor*, vtkAbstractWidget*, unsigned long, void* calldata)
{
  vtkEventData* edata = static_cast<vtkEventData*>(calldata);
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (edd)
  {
    edd->GetWorldPosition(this->StartEventPosition);
    this->LastEventPosition[0] = this->StartEventPosition[0];
    this->LastEventPosition[1] = this->StartEventPosition[1];
    this->LastEventPosition[2] = this->StartEventPosition[2];
    edd->GetWorldOrientation(this->StartEventOrientation);
    std::copy(
      this->StartEventOrientation, this->StartEventOrientation + 4, this->LastEventOrientation);
    for (int i = 0; i < 3; ++i)
    {
      if (this->SnappedOrientation[i])
      {
        std::copy(this->StartEventOrientation, this->StartEventOrientation + 4,
          this->SnappedEventOrientations[i]);
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::WidgetInteraction(double e[2])
{
  // Convert events to appropriate coordinate systems
  vtkCamera* camera = this->Renderer->GetActiveCamera();
  if (!camera)
  {
    return;
  }
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z, vpn[3];
  camera->GetViewPlaneNormal(vpn);

  // Compute the two points defining the motion vector
  double pos[3];
  if (this->LastPicker == this->HexPicker)
  {
    this->HexPicker->GetPickPosition(pos);
  }
  else
  {
    this->HandlePicker->GetPickPosition(pos);
  }
  vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, pos[0], pos[1], pos[2], focalPoint);
  z = focalPoint[2];
  vtkInteractorObserver::ComputeDisplayToWorld(
    this->Renderer, this->LastEventPosition[0], this->LastEventPosition[1], z, prevPickPoint);
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, e[0], e[1], z, pickPoint);

  // Process the motion
  if (this->InteractionState == vtkTensorRepresentation::MoveF0)
  {
    this->MoveMinusXFace(prevPickPoint, pickPoint, true);
  }

  else if (this->InteractionState == vtkTensorRepresentation::MoveF1)
  {
    this->MovePlusXFace(prevPickPoint, pickPoint, true);
  }

  else if (this->InteractionState == vtkTensorRepresentation::MoveF2)
  {
    this->MoveMinusYFace(prevPickPoint, pickPoint, true);
  }

  else if (this->InteractionState == vtkTensorRepresentation::MoveF3)
  {
    this->MovePlusYFace(prevPickPoint, pickPoint, true);
  }

  else if (this->InteractionState == vtkTensorRepresentation::MoveF4)
  {
    this->MoveMinusZFace(prevPickPoint, pickPoint, true);
  }

  else if (this->InteractionState == vtkTensorRepresentation::MoveF5)
  {
    this->MovePlusZFace(prevPickPoint, pickPoint, true);
  }

  else if (this->InteractionState == vtkTensorRepresentation::Translating)
  {
    this->Translate(prevPickPoint, pickPoint);
  }

  else if (this->InteractionState == vtkTensorRepresentation::Scaling)
  {
    this->Scale(prevPickPoint, pickPoint, static_cast<int>(e[0]), static_cast<int>(e[1]));
  }

  else if (this->InteractionState == vtkTensorRepresentation::Rotating)
  {
    this->Rotate(static_cast<int>(e[0]), static_cast<int>(e[1]), prevPickPoint, pickPoint, vpn);
  }

  // Store the start position
  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::ComplexInteraction(
  vtkRenderWindowInteractor*, vtkAbstractWidget*, unsigned long, void* calldata)
{
  vtkEventData* edata = static_cast<vtkEventData*>(calldata);
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (edd)
  {
    // all others
    double eventPos[3];
    edd->GetWorldPosition(eventPos);
    double eventDir[4];
    edd->GetWorldOrientation(eventDir);

    double* prevPickPoint = this->LastEventPosition;
    double* pickPoint = eventPos;

    if (this->InteractionState == vtkTensorRepresentation::MoveF0)
    {
      this->MoveMinusXFace(prevPickPoint, pickPoint, true);
    }

    else if (this->InteractionState == vtkTensorRepresentation::MoveF1)
    {
      this->MovePlusXFace(prevPickPoint, pickPoint, true);
    }

    else if (this->InteractionState == vtkTensorRepresentation::MoveF2)
    {
      this->MoveMinusYFace(prevPickPoint, pickPoint, true);
    }

    else if (this->InteractionState == vtkTensorRepresentation::MoveF3)
    {
      this->MovePlusYFace(prevPickPoint, pickPoint, true);
    }

    else if (this->InteractionState == vtkTensorRepresentation::MoveF4)
    {
      this->MoveMinusZFace(prevPickPoint, pickPoint, true);
    }

    else if (this->InteractionState == vtkTensorRepresentation::MoveF5)
    {
      this->MovePlusZFace(prevPickPoint, pickPoint, true);
    }

    else if (this->InteractionState == vtkTensorRepresentation::Translating)
    {
      this->UpdatePose(this->LastEventPosition, this->LastEventOrientation, eventPos, eventDir);
    }

    // Book keeping
    std::copy(eventPos, eventPos + 3, this->LastEventPosition);
    std::copy(eventDir, eventDir + 4, this->LastEventOrientation);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::StepForward()
{
  double* pts = static_cast<vtkDoubleArray*>(this->Points->GetData())->GetPointer(0);
  this->Translate(pts, pts + 3);
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::StepBackward()
{
  double* pts = static_cast<vtkDoubleArray*>(this->Points->GetData())->GetPointer(0);
  this->Translate(pts + 3, pts);
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::EndComplexInteraction(
  vtkRenderWindowInteractor*, vtkAbstractWidget*, unsigned long, void*)
{
}

//------------------------------------------------------------------------------
// This actually moves
void vtkTensorRepresentation::MoveFace(const double* p1, const double* p2, const double* dir,
  double* x1, double* x2, double* x3, double* x4, double* x5)
{
  int i;
  double v[3], v2[3];

  for (i = 0; i < 3; i++)
  {
    v[i] = p2[i] - p1[i];
    v2[i] = dir[i];
  }

  vtkMath::Normalize(v2);
  double f = vtkMath::Dot(v, v2);

  for (i = 0; i < 3; i++)
  {
    v[i] = f * v2[i];

    x1[i] += v[i];
    x2[i] += v[i];
    x3[i] += v[i];
    x4[i] += v[i];
    x5[i] += v[i];
  }
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::GetDirection(
  const double Nx[3], const double Ny[3], const double Nz[3], double dir[3])
{
  double dotNy, dotNz;
  double y[3];

  if (vtkMath::Dot(Nx, Nx) != 0)
  {
    dir[0] = Nx[0];
    dir[1] = Nx[1];
    dir[2] = Nx[2];
  }
  else
  {
    dotNy = vtkMath::Dot(Ny, Ny);
    dotNz = vtkMath::Dot(Nz, Nz);
    if (dotNy != 0 && dotNz != 0)
    {
      vtkMath::Cross(Ny, Nz, dir);
    }
    else if (dotNy != 0)
    {
      // dir must have been initialized to the
      // corresponding coordinate direction before calling
      // this method
      vtkMath::Cross(Ny, dir, y);
      vtkMath::Cross(y, Ny, dir);
    }
    else if (dotNz != 0)
    {
      // dir must have been initialized to the
      // corresponding coordinate direction before calling
      // this method
      vtkMath::Cross(Nz, dir, y);
      vtkMath::Cross(y, Nz, dir);
    }
  }
}

//------------------------------------------------------------------------------
// Move faces in pairs (e.g., minus x, plus x). This maintains a more natural
// transformation of a tensor. That is, it decouples scaling from translation.
//------------------------------------------------------------------------------
void vtkTensorRepresentation::MovePlusXFace(const double* p1, const double* p2, bool entry)
{
  double* pts = static_cast<vtkDoubleArray*>(this->Points->GetData())->GetPointer(0);

  // Plus x face
  double* h1 = pts + 3 * 9;
  double* x1 = pts + 3 * 1;
  double* x2 = pts + 3 * 2;
  double* x3 = pts + 3 * 5;
  double* x4 = pts + 3 * 6;

  double dir[3] = { 1, 0, 0 };
  this->ComputeNormals();
  this->GetDirection(this->N[1], this->N[3], this->N[5], dir);
  this->MoveFace(p1, p2, dir, x1, x2, x3, x4, h1);

  // Minus x face if event entry function
  if (entry)
  {
    this->MoveMinusXFace(p2, p1, false);
    this->PositionHandles();
    this->UpdateTensorFromWidget();
  }
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::MoveMinusXFace(const double* p1, const double* p2, bool entry)
{
  double* pts = static_cast<vtkDoubleArray*>(this->Points->GetData())->GetPointer(0);

  // Minus x face
  double* h1 = pts + 3 * 8;
  double* x1 = pts + 3 * 0;
  double* x2 = pts + 3 * 3;
  double* x3 = pts + 3 * 4;
  double* x4 = pts + 3 * 7;

  double dir[3] = { -1, 0, 0 };
  this->ComputeNormals();
  this->GetDirection(this->N[0], this->N[4], this->N[2], dir);
  this->MoveFace(p1, p2, dir, x1, x2, x3, x4, h1);

  // Plus x face if event entry function
  if (entry)
  {
    this->MovePlusXFace(p2, p1, false);
    this->PositionHandles();
    this->UpdateTensorFromWidget();
  }
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::MovePlusYFace(const double* p1, const double* p2, bool entry)
{
  double* pts = static_cast<vtkDoubleArray*>(this->Points->GetData())->GetPointer(0);

  double* h1 = pts + 3 * 11;
  double* x1 = pts + 3 * 2;
  double* x2 = pts + 3 * 3;
  double* x3 = pts + 3 * 6;
  double* x4 = pts + 3 * 7;

  double dir[3] = { 0, 1, 0 };
  this->ComputeNormals();
  this->GetDirection(this->N[3], this->N[5], this->N[1], dir);
  this->MoveFace(p1, p2, dir, x1, x2, x3, x4, h1);

  // Minus y face if event entry function
  if (entry)
  {
    this->MoveMinusYFace(p2, p1, false);
    this->PositionHandles();
    this->UpdateTensorFromWidget();
  }
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::MoveMinusYFace(const double* p1, const double* p2, bool entry)
{
  double* pts = static_cast<vtkDoubleArray*>(this->Points->GetData())->GetPointer(0);

  double* h1 = pts + 3 * 10;
  double* x1 = pts + 3 * 0;
  double* x2 = pts + 3 * 1;
  double* x3 = pts + 3 * 4;
  double* x4 = pts + 3 * 5;

  double dir[3] = { 0, -1, 0 };
  this->ComputeNormals();
  this->GetDirection(this->N[2], this->N[0], this->N[4], dir);
  this->MoveFace(p1, p2, dir, x1, x2, x3, x4, h1);

  // Plus y face if event entry function
  if (entry)
  {
    this->MovePlusYFace(p2, p1, false);
    this->PositionHandles();
    this->UpdateTensorFromWidget();
  }
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::MovePlusZFace(const double* p1, const double* p2, bool entry)
{
  double* pts = static_cast<vtkDoubleArray*>(this->Points->GetData())->GetPointer(0);

  double* h1 = pts + 3 * 13;

  double* x1 = pts + 3 * 4;
  double* x2 = pts + 3 * 5;
  double* x3 = pts + 3 * 6;
  double* x4 = pts + 3 * 7;

  double dir[3] = { 0, 0, 1 };
  this->ComputeNormals();
  this->GetDirection(this->N[5], this->N[1], this->N[3], dir);

  this->MoveFace(p1, p2, dir, x1, x2, x3, x4, h1);

  // Minus z face if event entry function
  if (entry)
  {
    this->MoveMinusZFace(p2, p1, false);
    this->PositionHandles();
    this->UpdateTensorFromWidget();
  }
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::MoveMinusZFace(const double* p1, const double* p2, bool entry)
{
  double* pts = static_cast<vtkDoubleArray*>(this->Points->GetData())->GetPointer(0);

  double* h1 = pts + 3 * 12;
  double* x1 = pts + 3 * 0;
  double* x2 = pts + 3 * 1;
  double* x3 = pts + 3 * 2;
  double* x4 = pts + 3 * 3;

  double dir[3] = { 0, 0, -1 };
  this->ComputeNormals();
  this->GetDirection(this->N[4], this->N[2], this->N[0], dir);
  this->MoveFace(p1, p2, dir, x1, x2, x3, x4, h1);

  // Plus z face if event entry function
  if (entry)
  {
    this->MovePlusZFace(p2, p1, false);
    this->PositionHandles();
    this->UpdateTensorFromWidget();
  }
}

//------------------------------------------------------------------------------
// Loop through all points and translate them
void vtkTensorRepresentation::Translate(const double* p1, const double* p2)
{
  double* pts = static_cast<vtkDoubleArray*>(this->Points->GetData())->GetPointer(0);
  double v[3] = { 0, 0, 0 };

  if (!this->IsTranslationConstrained())
  {
    v[0] = p2[0] - p1[0];
    v[1] = p2[1] - p1[1];
    v[2] = p2[2] - p1[2];
  }
  else
  {
    assert(this->TranslationAxis > -1 && this->TranslationAxis < 3 &&
      "this->TranslationAxis out of bounds");
    v[this->TranslationAxis] = p2[this->TranslationAxis] - p1[this->TranslationAxis];
  }

  // Move the corners
  for (int i = 0; i < 8; i++)
  {
    *pts++ += v[0];
    *pts++ += v[1];
    *pts++ += v[2];
  }

  // Position the handles
  this->PositionHandles();
  this->UpdateTensorFromWidget();
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::Scale(
  const double* vtkNotUsed(p1), const double* vtkNotUsed(p2), int vtkNotUsed(X), int Y)
{
  double* pts = static_cast<vtkDoubleArray*>(this->Points->GetData())->GetPointer(0);
  double* center = static_cast<vtkDoubleArray*>(this->Points->GetData())->GetPointer(3 * 14);
  double sf;

  if (Y > this->LastEventPosition[1])
  {
    sf = 1.03;
  }
  else
  {
    sf = 0.97;
  }

  // Move the corners
  for (int i = 0; i < 8; i++, pts += 3)
  {
    pts[0] = sf * (pts[0] - center[0]) + center[0];
    pts[1] = sf * (pts[1] - center[1]) + center[1];
    pts[2] = sf * (pts[2] - center[2]) + center[2];
  }
  this->PositionHandles();
  this->UpdateTensorFromWidget();
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::ComputeNormals()
{
  double* pts = static_cast<vtkDoubleArray*>(this->Points->GetData())->GetPointer(0);
  double* p0 = pts;
  double* px = pts + 3 * 1;
  double* py = pts + 3 * 3;
  double* pz = pts + 3 * 4;
  int i;

  for (i = 0; i < 3; i++)
  {
    this->N[0][i] = p0[i] - px[i];
    this->N[2][i] = p0[i] - py[i];
    this->N[4][i] = p0[i] - pz[i];
  }
  vtkMath::Normalize(this->N[0]);
  vtkMath::Normalize(this->N[2]);
  vtkMath::Normalize(this->N[4]);
  for (i = 0; i < 3; i++)
  {
    this->N[1][i] = -this->N[0][i];
    this->N[3][i] = -this->N[2][i];
    this->N[5][i] = -this->N[4][i];
  }
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::Rotate(
  int X, int Y, const double* p1, const double* p2, const double* vpn)
{
  double* center = static_cast<vtkDoubleArray*>(this->Points->GetData())->GetPointer(3 * 14);
  double v[3];    // vector of motion
  double axis[3]; // axis of rotation
  double theta;   // rotation angle
  int i;

  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Create axis of rotation and angle of rotation
  vtkMath::Cross(vpn, v, axis);
  if (vtkMath::Normalize(axis) == 0.0)
  {
    return;
  }
  int* size = this->Renderer->GetSize();
  double l2 = (X - this->LastEventPosition[0]) * (X - this->LastEventPosition[0]) +
    (Y - this->LastEventPosition[1]) * (Y - this->LastEventPosition[1]);
  theta = 360.0 * sqrt(l2 / (size[0] * size[0] + size[1] * size[1]));

  // Manipulate the transform to reflect the rotation
  this->Transform->Identity();
  this->Transform->Translate(center[0], center[1], center[2]);
  this->Transform->RotateWXYZ(theta, axis);
  this->Transform->Translate(-center[0], -center[1], -center[2]);

  // Set the corner points.
  this->TmpPoints->Reset();
  this->Transform->TransformPoints(this->Points, this->TmpPoints);
  for (i = 0; i < 8; i++)
  {
    this->Points->SetPoint(i, this->TmpPoints->GetPoint(i));
  }

  // Update the other points.
  this->PositionHandles();
  this->UpdateTensorFromWidget();
}

//------------------------------------------------------------------------------
namespace
{
bool snapToAxis(vtkVector3d& in, vtkVector3d& out, double snapAngle)
{
  int largest = 0;
  if (fabs(in[1]) > fabs(in[0]))
  {
    largest = 1;
  }
  if (fabs(in[2]) > fabs(in[largest]))
  {
    largest = 2;
  }
  vtkVector3d axis(0, 0, 0);
  axis[largest] = 1.0;
  // 3 degrees of sticky
  if (fabs(in.Dot(axis)) > cos(vtkMath::Pi() * snapAngle / 180.0))
  {
    if (in.Dot(axis) < 0)
    {
      axis[largest] = -1;
    }
    out = axis;
    return true;
  }
  return false;
}
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::UpdatePose(
  const double* pos1, const double* orient1, const double* pos2, const double* orient2)
{
  bool newSnap[3];
  vtkVector3d basis[3];
  double basisSize[3];

  vtkQuaternion<double> q2;
  q2.SetRotationAngleAndAxis(
    vtkMath::RadiansFromDegrees(orient2[0]), orient2[1], orient2[2], orient2[3]);

  for (int i = 0; i < 3; ++i)
  {
    newSnap[i] = false;
    // compute the net rotation
    vtkQuaternion<double> q1;
    if (this->SnappedOrientation[i])
    {
      q1.SetRotationAngleAndAxis(vtkMath::RadiansFromDegrees(this->SnappedEventOrientations[i][0]),
        this->SnappedEventOrientations[i][1], this->SnappedEventOrientations[i][2],
        this->SnappedEventOrientations[i][3]);
    }
    else
    {
      q1.SetRotationAngleAndAxis(
        vtkMath::RadiansFromDegrees(orient1[0]), orient1[1], orient1[2], orient1[3]);
    }
    q1.Conjugate();
    vtkQuaternion<double> q3 = q2 * q1;
    double axis[4];
    axis[0] = vtkMath::DegreesFromRadians(q3.GetRotationAngleAndAxis(axis + 1));

    // Manipulate the transform to reflect the rotation
    this->Transform->Identity();
    this->Transform->RotateWXYZ(axis[0], axis[1], axis[2], axis[3]);

    // Set the corners
    this->TmpPoints->Reset();
    this->Transform->TransformPoints(this->Points, this->TmpPoints);

    vtkVector3d p0(this->TmpPoints->GetPoint(0));
    vtkVector3d p1(this->TmpPoints->GetPoint((i > 0 ? i + 2 : 1)));
    basis[i] = p1 - p0;
    basisSize[i] = 0.5 * basis[i].Normalize();
    if (this->SnapToAxes)
    {
      // 14 degrees to snap in, 16 to snap out
      // avoids noise on the boundary
      newSnap[i] = snapToAxis(basis[i], basis[i], (this->SnappedOrientation[i] ? 16 : 14));
    }
  }

  // orthogonalize the resulting basis
  for (int i = 0; i < 3; ++i)
  {
    if (newSnap[i] || this->SnappedOrientation[i])
    {
      // orthogonalize the other axes
      vtkVector3d& b0 = basis[i];
      vtkVector3d& b1 = basis[(i + 1) % 3];
      vtkVector3d& b2 = basis[(i + 2) % 3];

      double val = b1.Dot(b0);
      b1 = b1 - b0 * val;
      b1.Normalize();
      b2 = b0.Cross(b1);
      b2.Normalize();

      if (!this->SnappedOrientation[i])
      {
        std::copy(orient2, orient2 + 4, this->SnappedEventOrientations[i]);
      }
    }
    this->SnappedOrientation[i] = newSnap[i];
  }

  // get the translation
  vtkVector3d trans;
  for (int i = 0; i < 3; i++)
  {
    trans[i] = pos2[i] - pos1[i];
  }

  vtkQuaternion<double> q1;
  q1.SetRotationAngleAndAxis(
    vtkMath::RadiansFromDegrees(orient1[0]), orient1[1], orient1[2], orient1[3]);
  q1.Conjugate();
  vtkQuaternion<double> q3 = q2 * q1;
  double axis[4];
  axis[0] = vtkMath::DegreesFromRadians(q3.GetRotationAngleAndAxis(axis + 1));

  // compute the new center based on the rotation
  // point of rotation and translation
  vtkVector3d center(static_cast<vtkDoubleArray*>(this->Points->GetData())->GetPointer(3 * 14));

  this->Transform->Identity();
  this->Transform->Translate(pos1[0], pos1[1], pos1[2]);
  this->Transform->RotateWXYZ(axis[0], axis[1], axis[2], axis[3]);
  this->Transform->Translate(-(pos1[0]), -(pos1[1]), -(pos1[2]));
  this->Transform->Translate(center[0], center[1], center[2]);

  this->Transform->GetPosition(center.GetData());
  center = center + trans;

  // rebuild points based on basis vectors
  this->Points->SetPoint(0,
    (center - basis[0] * basisSize[0] - basis[1] * basisSize[1] - basis[2] * basisSize[2])
      .GetData());
  this->Points->SetPoint(1,
    (center + basis[0] * basisSize[0] - basis[1] * basisSize[1] - basis[2] * basisSize[2])
      .GetData());
  this->Points->SetPoint(2,
    (center + basis[0] * basisSize[0] + basis[1] * basisSize[1] - basis[2] * basisSize[2])
      .GetData());
  this->Points->SetPoint(3,
    (center - basis[0] * basisSize[0] + basis[1] * basisSize[1] - basis[2] * basisSize[2])
      .GetData());
  this->Points->SetPoint(4,
    (center - basis[0] * basisSize[0] - basis[1] * basisSize[1] + basis[2] * basisSize[2])
      .GetData());
  this->Points->SetPoint(5,
    (center + basis[0] * basisSize[0] - basis[1] * basisSize[1] + basis[2] * basisSize[2])
      .GetData());
  this->Points->SetPoint(6,
    (center + basis[0] * basisSize[0] + basis[1] * basisSize[1] + basis[2] * basisSize[2])
      .GetData());
  this->Points->SetPoint(7,
    (center - basis[0] * basisSize[0] + basis[1] * basisSize[1] + basis[2] * basisSize[2])
      .GetData());

  this->PositionHandles();
  this->UpdateTensorFromWidget();
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::CreateDefaultProperties()
{
  // Handle properties
  this->HandleProperty = vtkProperty::New();
  this->HandleProperty->SetColor(1, 1, 1);

  this->SelectedHandleProperty = vtkProperty::New();
  this->SelectedHandleProperty->SetColor(1, 0, 0);

  // Face properties
  this->FaceProperty = vtkProperty::New();
  this->FaceProperty->SetColor(1, 1, 1);
  this->FaceProperty->SetOpacity(0.0);

  this->SelectedFaceProperty = vtkProperty::New();
  this->SelectedFaceProperty->SetColor(1, 1, 0);
  this->SelectedFaceProperty->SetOpacity(0.25);

  // Outline properties
  this->OutlineProperty = vtkProperty::New();
  this->OutlineProperty->SetRepresentationToWireframe();
  this->OutlineProperty->SetAmbient(1.0);
  this->OutlineProperty->SetAmbientColor(1.0, 1.0, 1.0);
  this->OutlineProperty->SetLineWidth(2.0);

  this->SelectedOutlineProperty = vtkProperty::New();
  this->SelectedOutlineProperty->SetRepresentationToWireframe();
  this->SelectedOutlineProperty->SetAmbient(1.0);
  this->SelectedOutlineProperty->SetAmbientColor(0.0, 1.0, 0.0);
  this->SelectedOutlineProperty->SetLineWidth(2.0);

  // Tensor ellipsoid properties
  this->EllipsoidProperty = vtkProperty::New();
  this->EllipsoidProperty->SetRepresentationToSurface();
  this->EllipsoidProperty->SetColor(0.6, 0.6, 0.8);
  this->EllipsoidProperty->SetOpacity(0.25);
}

//------------------------------------------------------------------------------
// Place a tensor at a specified position
void vtkTensorRepresentation::PlaceTensor(double tensor[9], double pos[3])
{
  // Now update the widget/representation from the tensor
  this->SetTensor(tensor);

  // Now translate the tensor
  double* pts = static_cast<vtkDoubleArray*>(this->Points->GetData())->GetPointer(0);
  double* center = pts + 3 * 14;
  this->Translate(center, pos);
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::PlaceWidget(double bds[6])
{
  int i;
  double bounds[6], center[3];

  this->AdjustBounds(bds, bounds, center);

  this->Points->SetPoint(0, bounds[0], bounds[2], bounds[4]);
  this->Points->SetPoint(1, bounds[1], bounds[2], bounds[4]);
  this->Points->SetPoint(2, bounds[1], bounds[3], bounds[4]);
  this->Points->SetPoint(3, bounds[0], bounds[3], bounds[4]);
  this->Points->SetPoint(4, bounds[0], bounds[2], bounds[5]);
  this->Points->SetPoint(5, bounds[1], bounds[2], bounds[5]);
  this->Points->SetPoint(6, bounds[1], bounds[3], bounds[5]);
  this->Points->SetPoint(7, bounds[0], bounds[3], bounds[5]);

  for (i = 0; i < 6; i++)
  {
    this->InitialBounds[i] = bounds[i];
  }
  this->InitialLength = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));

  this->PositionHandles();
  this->UpdateTensorFromWidget();
  this->ComputeNormals();
  this->ValidPick = 1; // since we have set up widget
  this->SizeHandles();
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::SetOutlineFaceWires(bool newValue)
{
  if (this->OutlineFaceWires != newValue)
  {
    this->OutlineFaceWires = newValue;
    this->Modified();
    // the outline is dependent on this value, so we have to regen
    this->GenerateOutline();
  }
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::SetOutlineCursorWires(bool newValue)
{
  if (this->OutlineCursorWires != newValue)
  {
    this->OutlineCursorWires = newValue;
    this->Modified();
    // the outline is dependent on this value, so we have to regen
    this->GenerateOutline();
  }
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::GenerateOutline()
{
  // Whatever the case may be, we have to reset the Lines of the
  // OutlinePolyData (i.e. nuke all current line data)
  vtkCellArray* cells = this->OutlinePolyData->GetLines();
  cells->Reset();
  cells->Modified();

  // Now the outline lines
  if (!this->OutlineFaceWires && !this->OutlineCursorWires)
  {
    return;
  }

  vtkIdType pts[2];

  if (this->OutlineFaceWires)
  {
    pts[0] = 0;
    pts[1] = 7; // the -x face
    cells->InsertNextCell(2, pts);
    pts[0] = 3;
    pts[1] = 4;
    cells->InsertNextCell(2, pts);
    pts[0] = 1;
    pts[1] = 6; // the +x face
    cells->InsertNextCell(2, pts);
    pts[0] = 2;
    pts[1] = 5;
    cells->InsertNextCell(2, pts);

    pts[0] = 1;
    pts[1] = 4; // the -y face
    cells->InsertNextCell(2, pts);
    pts[0] = 0;
    pts[1] = 5;
    cells->InsertNextCell(2, pts);
    pts[0] = 3;
    pts[1] = 6; // the +y face
    cells->InsertNextCell(2, pts);
    pts[0] = 2;
    pts[1] = 7;
    cells->InsertNextCell(2, pts);
    pts[0] = 0;
    pts[1] = 2; // the -z face
    cells->InsertNextCell(2, pts);
    pts[0] = 1;
    pts[1] = 3;
    cells->InsertNextCell(2, pts);
    pts[0] = 4;
    pts[1] = 6; // the +Z face
    cells->InsertNextCell(2, pts);
    pts[0] = 5;
    pts[1] = 7;
    cells->InsertNextCell(2, pts);
  }
  if (this->OutlineCursorWires)
  {
    pts[0] = 8;
    pts[1] = 9; // the x cursor line
    cells->InsertNextCell(2, pts);

    pts[0] = 10;
    pts[1] = 11; // the y cursor line
    cells->InsertNextCell(2, pts);
    pts[0] = 12;
    pts[1] = 13; // the z cursor line
    cells->InsertNextCell(2, pts);
  }
  this->OutlinePolyData->Modified();
  if (this->OutlineProperty)
  {
    this->OutlineProperty->SetRepresentationToWireframe();
    this->SelectedOutlineProperty->SetRepresentationToWireframe();
  }
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::SetTensorEllipsoid(bool newValue)
{
  if (this->TensorEllipsoid != newValue)
  {
    this->TensorEllipsoid = newValue;
    this->UpdateTensorFromWidget();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
// Update the tensor ellipsoid, and associated tensor data (e.g., eigenvalues)
// from the current widget/representation state.
void vtkTensorRepresentation::UpdateTensorFromWidget()
{
  // Obtain the points defining the representation
  double* pts = static_cast<vtkDoubleArray*>(this->Points->GetData())->GetPointer(0);
  double* center = pts + 3 * 14;
  double* x = pts + 3 * 9;
  double* y = pts + 3 * 11;
  double* z = pts + 3 * 13;

  // Gather information about the representation: size (norm) of semi-axes;
  // and the semi-axes vectors. These are the eigenvectors and values of the
  // tensor.
  double tensor[3][3];
  for (auto i = 0; i < 3; ++i)
  {
    tensor[i][0] = x[i] - center[i];
    tensor[i][1] = y[i] - center[i];
    tensor[i][2] = z[i] - center[i];
  }

  // Use the internal transforms to perform the transformation of the
  // ellipsoid.
  this->EllipsoidTransform->Identity();

  // Translate to center of widget
  this->EllipsoidTransform->Translate(center);

  // Next scale and rotate the ellipsoid based on the eigenvectors (which are
  // simply the semi-axes of the widget representation).
  for (auto j = 0; j < 3; ++j)
  {
    for (auto i = 0; i < 3; ++i)
    {
      this->EllipsoidMatrix->Element[i][j] = tensor[i][j];
    }
  }
  this->EllipsoidTransform->Concatenate(this->EllipsoidMatrix);

  // Specify total transformation
  //  this->EllipsoidTransform->Modified();

  // Now update the tensor information
  std::copy(center, center + 3, this->TensorPosition);
  this->UpdateTensorEigenfunctions(tensor);
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::UpdateTensorEigenfunctions(double tensor[3][3])
{
  // Now update the tensor information. The tensor data is sorted
  // from largest to smallest eigenvalues.
  double n[3]; // eigenvector norms
  int order[3] = { -1, -1, -1 };
  n[0] = vtkMath::Norm(tensor[0]);
  n[1] = vtkMath::Norm(tensor[1]);
  n[2] = vtkMath::Norm(tensor[2]);
  order[0] = (n[0] >= n[1] ? (n[0] >= n[2] ? 0 : 2) : (n[1] >= n[2] ? 1 : 2)); // max
  order[2] = (n[0] < n[1] ? (n[0] < n[2] ? 0 : 2) : (n[1] < n[2] ? 1 : 2));    // min
  order[1] = 3 - order[0] - order[2];                                          // neat trick ;-)

  this->Eigenvalues[0] = n[order[0]];
  this->Eigenvalues[1] = n[order[1]];
  this->Eigenvalues[2] = n[order[2]];

  std::copy(tensor[order[0]], tensor[order[0]] + 3, this->Eigenvectors[0]);
  std::copy(tensor[order[1]], tensor[order[1]] + 3, this->Eigenvectors[1]);
  std::copy(tensor[order[2]], tensor[order[2]] + 3, this->Eigenvectors[2]);
}

//------------------------------------------------------------------------------
// Update the tensor ellipsoid, and associated widget/representation from the
// current tensor specification. We'll use the current position.
void vtkTensorRepresentation::UpdateWidgetFromTensor()
{
  // We are using the current tensor position
  double* pts = static_cast<vtkDoubleArray*>(this->Points->GetData())->GetPointer(0);
  double* center = pts + 3 * 14;

  // We'll reset the points to be axis aligned, (-1,1,-1,1,-1,1) box.
  // The feed the eigenvectors into a transformation matrix and
  // transform the eight corner points.
  this->Points->SetPoint(0, -1, -1, -1);
  this->Points->SetPoint(1, 1, -1, -1);
  this->Points->SetPoint(2, 1, 1, -1);
  this->Points->SetPoint(3, -1, 1, -1);
  this->Points->SetPoint(4, -1, -1, 1);
  this->Points->SetPoint(5, 1, -1, 1);
  this->Points->SetPoint(6, 1, 1, 1);
  this->Points->SetPoint(7, -1, 1, 1);

  this->EllipsoidTransform->Identity();
  this->EllipsoidTransform->Translate(center[0], center[1], center[2]);
  this->EllipsoidTransform->Scale(this->Eigenvalues);

  for (auto j = 0; j < 3; ++j)
  {
    for (auto i = 0; i < 3; ++i)
    {
      this->EllipsoidMatrix->Element[i][j] = this->Eigenvectors[i][j];
    }
  }
  this->EllipsoidTransform->Concatenate(this->EllipsoidMatrix);
  this->EllipsoidTransform->Translate(-center[0], -center[1], -center[2]);

  // Transform the 8 corner points
  this->TmpPoints->Reset();
  this->EllipsoidTransform->TransformPoints(this->Points, this->TmpPoints);
  for (auto i = 0; i < 8; i++)
  {
    this->Points->SetPoint(i, this->TmpPoints->GetPoint(i));
  }

  this->PositionHandles();
}

//------------------------------------------------------------------------------
int vtkTensorRepresentation::ComputeInteractionState(int X, int Y, int modify)
{
  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then pick the bounding box.
  if (!this->Renderer || !this->Renderer->IsInViewport(X, Y))
  {
    this->InteractionState = vtkTensorRepresentation::Outside;
    return this->InteractionState;
  }

  // Try and pick a handle first
  this->LastPicker = nullptr;
  this->CurrentHandle = nullptr;

  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->HandlePicker);

  if (path != nullptr)
  {
    this->ValidPick = 1;
    this->LastPicker = this->HandlePicker;
    this->CurrentHandle = reinterpret_cast<vtkActor*>(path->GetFirstNode()->GetViewProp());
    if (this->CurrentHandle == this->Handle[0])
    {
      this->InteractionState = vtkTensorRepresentation::MoveF0;
    }
    else if (this->CurrentHandle == this->Handle[1])
    {
      this->InteractionState = vtkTensorRepresentation::MoveF1;
    }
    else if (this->CurrentHandle == this->Handle[2])
    {
      this->InteractionState = vtkTensorRepresentation::MoveF2;
    }
    else if (this->CurrentHandle == this->Handle[3])
    {
      this->InteractionState = vtkTensorRepresentation::MoveF3;
    }
    else if (this->CurrentHandle == this->Handle[4])
    {
      this->InteractionState = vtkTensorRepresentation::MoveF4;
    }
    else if (this->CurrentHandle == this->Handle[5])
    {
      this->InteractionState = vtkTensorRepresentation::MoveF5;
    }
    else if (this->CurrentHandle == this->Handle[6])
    {
      this->InteractionState = vtkTensorRepresentation::Translating;
    }
  }
  else // see if the hex is picked
  {
    path = this->GetAssemblyPath(X, Y, 0., this->HexPicker);

    if (path != nullptr)
    {
      this->LastPicker = this->HexPicker;
      this->ValidPick = 1;
      if (!modify)
      {
        this->InteractionState = vtkTensorRepresentation::Rotating;
      }
      else
      {
        this->CurrentHandle = this->Handle[6];
        this->InteractionState = vtkTensorRepresentation::Translating;
      }
    }
    else
    {
      this->InteractionState = vtkTensorRepresentation::Outside;
    }
  }

  return this->InteractionState;
}

//------------------------------------------------------------------------------
int vtkTensorRepresentation::ComputeComplexInteractionState(
  vtkRenderWindowInteractor*, vtkAbstractWidget*, unsigned long, void* calldata, int)
{
  this->InteractionState = vtkTensorRepresentation::Outside;

  vtkEventData* edata = static_cast<vtkEventData*>(calldata);
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (edd)
  {
    double pos[3];
    edd->GetWorldPosition(pos);

    // Try and pick a handle first
    this->LastPicker = nullptr;
    this->CurrentHandle = nullptr;

    vtkAssemblyPath* path = this->GetAssemblyPath3DPoint(pos, this->HandlePicker);

    if (path != nullptr)
    {
      this->ValidPick = 1;
      this->LastPicker = this->HandlePicker;
      this->CurrentHandle = reinterpret_cast<vtkActor*>(path->GetFirstNode()->GetViewProp());
      if (this->CurrentHandle == this->Handle[0])
      {
        this->InteractionState = vtkTensorRepresentation::MoveF0;
      }
      else if (this->CurrentHandle == this->Handle[1])
      {
        this->InteractionState = vtkTensorRepresentation::MoveF1;
      }
      else if (this->CurrentHandle == this->Handle[2])
      {
        this->InteractionState = vtkTensorRepresentation::MoveF2;
      }
      else if (this->CurrentHandle == this->Handle[3])
      {
        this->InteractionState = vtkTensorRepresentation::MoveF3;
      }
      else if (this->CurrentHandle == this->Handle[4])
      {
        this->InteractionState = vtkTensorRepresentation::MoveF4;
      }
      else if (this->CurrentHandle == this->Handle[5])
      {
        this->InteractionState = vtkTensorRepresentation::MoveF5;
      }
      else if (this->CurrentHandle == this->Handle[6])
      {
        this->InteractionState = vtkTensorRepresentation::Translating;
      }
    }
    else // see if the hex is picked
    {
      path = this->GetAssemblyPath3DPoint(pos, this->HexPicker);

      if (path != nullptr)
      {
        this->LastPicker = this->HexPicker;
        this->ValidPick = 1;
        this->CurrentHandle = this->Handle[6];
        this->InteractionState = vtkTensorRepresentation::Translating;
      }
    }
  }

  return this->InteractionState;
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::SetInteractionState(int state)
{
  // Clamp to allowable values
  state = (state < vtkTensorRepresentation::Outside
      ? vtkTensorRepresentation::Outside
      : (state > vtkTensorRepresentation::Scaling ? vtkTensorRepresentation::Scaling : state));

  // Depending on state, highlight appropriate parts of representation
  int handle;
  this->InteractionState = state;
  switch (state)
  {
    case vtkTensorRepresentation::MoveF0:
    case vtkTensorRepresentation::MoveF1:
    case vtkTensorRepresentation::MoveF2:
    case vtkTensorRepresentation::MoveF3:
    case vtkTensorRepresentation::MoveF4:
    case vtkTensorRepresentation::MoveF5:
      this->HighlightOutline(0);
      handle = this->HighlightHandle(this->CurrentHandle);
      this->HighlightFace(handle);
      break;
    case vtkTensorRepresentation::Rotating:
      this->HighlightOutline(0);
      this->HighlightHandle(nullptr);
      this->HighlightFace(this->HexPicker->GetCellId());
      break;
    case vtkTensorRepresentation::Translating:
    case vtkTensorRepresentation::Scaling:
      this->HighlightOutline(1);
      this->HighlightHandle(this->Handle[6]);
      this->HighlightFace(-1);
      break;
    default:
      this->HighlightOutline(0);
      this->HighlightHandle(nullptr);
      this->HighlightFace(-1);
  }
}

//------------------------------------------------------------------------------
double* vtkTensorRepresentation::GetBounds()
{
  this->BuildRepresentation();
  this->BoundingBox->SetBounds(this->HexActor->GetBounds());
  return this->BoundingBox->GetBounds();
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::BuildRepresentation()
{
  // Rebuild only if necessary
  if (this->GetMTime() > this->BuildTime ||
    (this->Renderer && this->Renderer->GetVTKWindow() &&
      (this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime ||
        this->Renderer->GetActiveCamera()->GetMTime() > this->BuildTime)))
  {
    this->SizeHandles();
    this->BuildTime.Modified();
  }
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::ReleaseGraphicsResources(vtkWindow* w)
{
  this->HexActor->ReleaseGraphicsResources(w);
  this->HexOutline->ReleaseGraphicsResources(w);
  this->HexFace->ReleaseGraphicsResources(w);
  // render the handles
  for (int j = 0; j < 7; j++)
  {
    this->Handle[j]->ReleaseGraphicsResources(w);
  }

  this->EllipsoidActor->ReleaseGraphicsResources(w);
}

//------------------------------------------------------------------------------
int vtkTensorRepresentation::RenderOpaqueGeometry(vtkViewport* v)
{
  int count = 0;
  this->BuildRepresentation();

  this->HexActor->SetPropertyKeys(this->GetPropertyKeys());
  this->HexOutline->SetPropertyKeys(this->GetPropertyKeys());
  this->HexFace->SetPropertyKeys(this->GetPropertyKeys());

  count += this->HexActor->RenderOpaqueGeometry(v);
  count += this->HexOutline->RenderOpaqueGeometry(v);
  count += this->HexFace->RenderOpaqueGeometry(v);
  for (int j = 0; j < 7; j++)
  {
    if (this->Handle[j]->GetVisibility())
    {
      this->Handle[j]->SetPropertyKeys(this->GetPropertyKeys());
      count += this->Handle[j]->RenderOpaqueGeometry(v);
    }
  }

  if (this->TensorEllipsoid)
  {
    count += this->EllipsoidActor->RenderOpaqueGeometry(v);
  }

  return count;
}

//------------------------------------------------------------------------------
int vtkTensorRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport* v)
{
  int count = 0;
  this->BuildRepresentation();

  this->HexActor->SetPropertyKeys(this->GetPropertyKeys());
  this->HexOutline->SetPropertyKeys(this->GetPropertyKeys());
  this->HexFace->SetPropertyKeys(this->GetPropertyKeys());

  count += this->HexActor->RenderTranslucentPolygonalGeometry(v);
  count += this->HexOutline->RenderTranslucentPolygonalGeometry(v);
  count += this->HexFace->RenderTranslucentPolygonalGeometry(v);
  // render the handles
  for (int j = 0; j < 7; j++)
  {
    if (this->Handle[j]->GetVisibility())
    {
      this->Handle[j]->SetPropertyKeys(this->GetPropertyKeys());
      count += this->Handle[j]->RenderTranslucentPolygonalGeometry(v);
    }
  }

  if (this->TensorEllipsoid)
  {
    count += this->EllipsoidActor->RenderTranslucentPolygonalGeometry(v);
  }

  return count;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkTensorRepresentation::HasTranslucentPolygonalGeometry()
{
  int result = 0;
  this->BuildRepresentation();

  result |= this->HexActor->HasTranslucentPolygonalGeometry();
  result |= this->HexOutline->HasTranslucentPolygonalGeometry();

  // If the face is not selected, we are not really rendering translucent faces,
  // hence don't bother taking it's opacity into consideration.
  // Look at BUG #7301.
  if (this->HexFace->GetProperty() == this->SelectedFaceProperty)
  {
    result |= this->HexFace->HasTranslucentPolygonalGeometry();
  }

  // render the handles
  for (int j = 0; j < 7; j++)
  {
    result |= this->Handle[j]->HasTranslucentPolygonalGeometry();
  }

  if (this->TensorEllipsoid)
  {
    result |= this->EllipsoidActor->HasTranslucentPolygonalGeometry();
  }

  return result;
}

#define VTK_AVERAGE(a, b, c)                                                                       \
  c[0] = (a[0] + b[0]) / 2.0;                                                                      \
  c[1] = (a[1] + b[1]) / 2.0;                                                                      \
  c[2] = (a[2] + b[2]) / 2.0;

//------------------------------------------------------------------------------
// Position the handles based on the current positions of the eight corner points
void vtkTensorRepresentation::PositionHandles()
{
  double* pts = static_cast<vtkDoubleArray*>(this->Points->GetData())->GetPointer(0);
  double* p0 = pts;
  double* p1 = pts + 3 * 1;
  double* p2 = pts + 3 * 2;
  double* p3 = pts + 3 * 3;
  // double *p4 = pts + 3*4; //not used below
  double* p5 = pts + 3 * 5;
  double* p6 = pts + 3 * 6;
  double* p7 = pts + 3 * 7;
  double x[3];

  VTK_AVERAGE(p0, p7, x);
  this->Points->SetPoint(8, x);
  VTK_AVERAGE(p1, p6, x);
  this->Points->SetPoint(9, x);
  VTK_AVERAGE(p0, p5, x);
  this->Points->SetPoint(10, x);
  VTK_AVERAGE(p2, p7, x);
  this->Points->SetPoint(11, x);
  VTK_AVERAGE(p1, p3, x);
  this->Points->SetPoint(12, x);
  VTK_AVERAGE(p5, p7, x);
  this->Points->SetPoint(13, x);
  VTK_AVERAGE(p0, p6, x);
  this->Points->SetPoint(14, x);

  for (int i = 0; i < 7; ++i)
  {
    this->HandleGeometry[i]->SetCenter(this->Points->GetPoint(8 + i));
  }

  for (int i = 0; i < 6; ++i)
  {
    this->Planes[i]->SetOrigin(this->Points->GetPoint(8 + i));
    int mix = 2 * (i % 2);
    vtkVector3d pp1(this->Points->GetPoint(8 + i));
    vtkVector3d pp2(this->Points->GetPoint(9 + i - mix));
    pp2 = pp2 - pp1;
    pp2.Normalize();
    this->Planes[i]->SetNormal(pp2.GetData());
  }

  this->Points->GetData()->Modified();
  this->HexFacePolyData->Modified();
  this->HexPolyData->Modified();
  this->GenerateOutline();
}
#undef VTK_AVERAGE

//------------------------------------------------------------------------------
void vtkTensorRepresentation::HandlesOn()
{
  for (int i = 0; i < 7; i++)
  {
    this->Handle[i]->VisibilityOn();
  }
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::HandlesOff()
{
  for (int i = 0; i < 7; i++)
  {
    this->Handle[i]->VisibilityOff();
  }
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::SizeHandles()
{
  double* center = static_cast<vtkDoubleArray*>(this->Points->GetData())->GetPointer(3 * 14);
  double radius = this->vtkWidgetRepresentation::SizeHandlesInPixels(1.5, center);
  for (int i = 0; i < 7; i++)
  {
    this->HandleGeometry[i]->SetRadius(radius);
  }
}

//------------------------------------------------------------------------------
int vtkTensorRepresentation::HighlightHandle(vtkProp* prop)
{
  // first unhighlight anything picked
  this->HighlightOutline(0);
  if (this->CurrentHandle)
  {
    this->CurrentHandle->SetProperty(this->HandleProperty);
  }

  this->CurrentHandle = static_cast<vtkActor*>(prop);

  if (this->CurrentHandle)
  {
    this->CurrentHandle->SetProperty(this->SelectedHandleProperty);
    for (int i = 0; i < 6; i++) // find attached face
    {
      if (this->CurrentHandle == this->Handle[i])
      {
        return i;
      }
    }
  }

  if (this->CurrentHandle == this->Handle[6])
  {
    this->HighlightOutline(1);
    return 6;
  }

  return -1;
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::HighlightFace(int cellId)
{
  if (cellId >= 0)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    vtkCellArray* cells = this->HexFacePolyData->GetPolys();
    this->HexPolyData->GetCellPoints(cellId, npts, pts);
    this->HexFacePolyData->Modified();
    cells->ReplaceCellAtId(0, npts, pts);
    cells->Modified();
    this->CurrentHexFace = cellId;
    this->HexFace->SetProperty(this->SelectedFaceProperty);
    if (!this->CurrentHandle)
    {
      this->CurrentHandle = this->HexFace;
    }
  }
  else
  {
    this->HexFace->SetProperty(this->FaceProperty);
    this->CurrentHexFace = -1;
  }
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::HighlightOutline(int highlight)
{
  if (highlight)
  {
    this->HexActor->SetProperty(this->SelectedOutlineProperty);
    this->HexOutline->SetProperty(this->SelectedOutlineProperty);
  }
  else
  {
    this->HexActor->SetProperty(this->OutlineProperty);
    this->HexOutline->SetProperty(this->OutlineProperty);
  }
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::RegisterPickers()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->HandlePicker, this);
  pm->AddPicker(this->HexPicker, this);
}

//------------------------------------------------------------------------------
void vtkTensorRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  double* bounds = this->InitialBounds;
  os << indent << "Initial Bounds: "
     << "(" << bounds[0] << "," << bounds[1] << ") "
     << "(" << bounds[2] << "," << bounds[3] << ") "
     << "(" << bounds[4] << "," << bounds[5] << ")\n";

  if (this->HandleProperty)
  {
    os << indent << "Handle Property: " << this->HandleProperty << "\n";
  }
  else
  {
    os << indent << "Handle Property: (none)\n";
  }
  if (this->SelectedHandleProperty)
  {
    os << indent << "Selected Handle Property: " << this->SelectedHandleProperty << "\n";
  }
  else
  {
    os << indent << "SelectedHandle Property: (none)\n";
  }

  if (this->FaceProperty)
  {
    os << indent << "Face Property: " << this->FaceProperty << "\n";
  }
  else
  {
    os << indent << "Face Property: (none)\n";
  }
  if (this->SelectedFaceProperty)
  {
    os << indent << "Selected Face Property: " << this->SelectedFaceProperty << "\n";
  }
  else
  {
    os << indent << "Selected Face Property: (none)\n";
  }

  if (this->OutlineProperty)
  {
    os << indent << "Outline Property: " << this->OutlineProperty << "\n";
  }
  else
  {
    os << indent << "Outline Property: (none)\n";
  }
  if (this->SelectedOutlineProperty)
  {
    os << indent << "Selected Outline Property: " << this->SelectedOutlineProperty << "\n";
  }
  else
  {
    os << indent << "Selected Outline Property: (none)\n";
  }

  if (this->EllipsoidProperty)
  {
    os << indent << "Ellipsoid Property: " << this->EllipsoidProperty << "\n";
  }
  else
  {
    os << indent << "Ellipsoid Property: (none)\n";
  }

  os << indent << "Snap To Axes: " << (this->SnapToAxes ? "On\n" : "Off\n");

  os << indent << "Outline Face Wires: " << (this->OutlineFaceWires ? "On\n" : "Off\n");
  os << indent << "Outline Cursor Wires: " << (this->OutlineCursorWires ? "On\n" : "Off\n");
  os << indent << "Tensor Ellipsoid: " << (this->TensorEllipsoid ? "On\n" : "Off\n");
  os << indent << "Inside Out: " << (this->InsideOut ? "On\n" : "Off\n");
}
