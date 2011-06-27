/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResliceCursorPicker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkResliceCursorPicker.h"
#include "vtkObjectFactory.h"

#include "vtkCommand.h"
#include "vtkMath.h"
#include "vtkPoints.h"
#include "vtkGenericCell.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkLine.h"
#include "vtkPolyData.h"
#include "vtkRenderWindow.h"
#include "vtkResliceCursor.h"
#include "vtkResliceCursorPolyDataAlgorithm.h"
#include "vtkPlane.h"
#include "vtkMatrix4x4.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkResliceCursorPicker);
vtkCxxSetObjectMacro(vtkResliceCursorPicker,
    ResliceCursorAlgorithm, vtkResliceCursorPolyDataAlgorithm);
vtkCxxSetObjectMacro(vtkResliceCursorPicker, TransformMatrix, vtkMatrix4x4 );

//----------------------------------------------------------------------------
vtkResliceCursorPicker::vtkResliceCursorPicker()
{
  // For polydata picking
  this->Cell = vtkGenericCell::New();

  // Miscellaneous ivars
  this->Tolerance = 1e-6;

  this->PickedAxis1 = this->PickedAxis2 = 0;

  this->ResliceCursorAlgorithm = NULL;
  this->TransformMatrix = NULL;
}

//----------------------------------------------------------------------------
vtkResliceCursorPicker::~vtkResliceCursorPicker()
{
  this->Cell->Delete();
  this->SetResliceCursorAlgorithm(NULL);
  this->SetTransformMatrix(NULL);
}

#define sign(x) ((x) >= 0 ? 1 : 0)

//----------------------------------------------------------------------------
int vtkResliceCursorPicker::Pick(double selectionX, double selectionY,
                           double selectionZ, vtkRenderer *renderer)
{
  int i;
  vtkCamera *camera;
  double p1World[4], p2World[4];
  int winSize[2] = {1, 1};
  double x, y;
  double *viewport;
  double cameraPos[4], cameraFP[4];
  double *displayCoords, *worldCoords;
  double *clipRange;
  double ray[3], rayLength;
  double windowLowerLeft[4], windowUpperRight[4];
  double bounds[6], tol;
  double tF, tB;
  double cameraDOP[3];

  bounds[0] = bounds[1] = bounds[2] = bounds[3] = bounds[4] = bounds[5] = 0;

  //  Initialize picking process
  this->Initialize();
  this->Renderer = renderer;
  this->SelectionPoint[0] = selectionX;
  this->SelectionPoint[1] = selectionY;
  this->SelectionPoint[2] = selectionZ;

  // Invoke start pick method if defined
  this->InvokeEvent(vtkCommand::StartPickEvent,NULL);

  if ( renderer == NULL )
    {
    vtkErrorMacro(<<"Must specify renderer!");
    return 0;
    }

  // Get camera focal point and position. Convert to display (screen)
  // coordinates. We need a depth value for z-buffer.
  //
  camera = renderer->GetActiveCamera();
  camera->GetPosition(cameraPos);
  cameraPos[3] = 1.0;
  camera->GetFocalPoint(cameraFP);
  cameraFP[3] = 1.0;

  renderer->SetWorldPoint(cameraFP[0],cameraFP[1],cameraFP[2],cameraFP[3]);
  renderer->WorldToDisplay();
  displayCoords = renderer->GetDisplayPoint();
  selectionZ = displayCoords[2];

  // Convert the selection point into world coordinates.
  //
  renderer->SetDisplayPoint(selectionX, selectionY, selectionZ);
  renderer->DisplayToWorld();
  worldCoords = renderer->GetWorldPoint();
  if ( worldCoords[3] == 0.0 )
    {
    vtkErrorMacro(<<"Bad homogeneous coordinates");
    return 0;
    }
  for (i=0; i < 3; i++)
    {
    this->PickPosition[i] = worldCoords[i] / worldCoords[3];
    }

  //  Compute the ray endpoints.  The ray is along the line running from
  //  the camera position to the selection point, starting where this line
  //  intersects the front clipping plane, and terminating where this
  //  line intersects the back clipping plane.
  for (i=0; i<3; i++)
    {
    ray[i] = this->PickPosition[i] - cameraPos[i];
    }
  for (i=0; i<3; i++)
    {
    cameraDOP[i] = cameraFP[i] - cameraPos[i];
    }

  vtkMath::Normalize(cameraDOP);

  if (( rayLength = vtkMath::Dot(cameraDOP,ray)) == 0.0 )
    {
    vtkWarningMacro("Cannot process points");
    return 0;
    }

  clipRange = camera->GetClippingRange();

  if ( camera->GetParallelProjection() )
    {
    tF = clipRange[0] - rayLength;
    tB = clipRange[1] - rayLength;
    for (i=0; i<3; i++)
      {
      p1World[i] = this->PickPosition[i] + tF*cameraDOP[i];
      p2World[i] = this->PickPosition[i] + tB*cameraDOP[i];
      }
    }
  else
    {
    tF = clipRange[0] / rayLength;
    tB = clipRange[1] / rayLength;
    for (i=0; i<3; i++)
      {
      p1World[i] = cameraPos[i] + tF*ray[i];
      p2World[i] = cameraPos[i] + tB*ray[i];
      }
    }
  p1World[3] = p2World[3] = 1.0;

  // Compute the tolerance in world coordinates.  Do this by
  // determining the world coordinates of the diagonal points of the
  // window, computing the width of the window in world coordinates, and
  // multiplying by the tolerance.
  //
  viewport = renderer->GetViewport();
  if (renderer->GetRenderWindow())
    {
    int *winSizePtr = renderer->GetRenderWindow()->GetSize();
    if (winSizePtr)
      {
      winSize[0] = winSizePtr[0];
      winSize[1] = winSizePtr[1];
      }
    }
  x = winSize[0] * viewport[0];
  y = winSize[1] * viewport[1];
  renderer->SetDisplayPoint(x, y, selectionZ);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(windowLowerLeft);

  x = winSize[0] * viewport[2];
  y = winSize[1] * viewport[3];
  renderer->SetDisplayPoint(x, y, selectionZ);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(windowUpperRight);

  for (tol=0.0,i=0; i<3; i++)
    {
    tol += (windowUpperRight[i] - windowLowerLeft[i]) *
      (windowUpperRight[i] - windowLowerLeft[i]);
    }

  tol = sqrt (tol) * this->Tolerance;

  vtkResliceCursor *rc =
    this->ResliceCursorAlgorithm->GetResliceCursor();
  const int axis1 = this->ResliceCursorAlgorithm->GetAxis1();
  const int axis2 = this->ResliceCursorAlgorithm->GetAxis2();
  const int axis3 = this->ResliceCursorAlgorithm->GetReslicePlaneNormal();

  double center[3];
  rc->GetCenter(center);
  this->PickedCenter = this->IntersectPointWithLine( p1World, p2World,
            center, tol );
  this->PickedAxis1 = this->IntersectPolyDataWithLine( p1World, p2World,
            rc->GetCenterlineAxisPolyData(axis1), tol);
  this->PickedAxis2 = this->IntersectPolyDataWithLine( p1World, p2World,
            rc->GetCenterlineAxisPolyData(axis2), tol);

  if (this->PickedAxis1 || this->PickedAxis2 || this->PickedCenter)
    {
    double t;
    vtkSmartPointer< vtkPlane > plane = vtkSmartPointer< vtkPlane >::New();
    plane->SetOrigin(rc->GetPlane(axis3)->GetOrigin());
    double planeNormal[3] = {0,0,0};
    planeNormal[axis3] = sign(rc->GetPlane(axis3)->GetNormal()[axis3]) * 1;
    plane->SetNormal(planeNormal);
    plane->IntersectWithLine(p1World, p2World, t, this->PickPosition);
    }


  return this->PickedAxis1 + this->PickedAxis2 + this->PickedCenter;
}

//----------------------------------------------------------------------------
int vtkResliceCursorPicker::IntersectPolyDataWithLine(
    double p1[3], double p2[3], vtkPolyData *data, double tol )
{
  vtkIdType numCells = data->GetNumberOfCells();

  for (vtkIdType cellId = 0; cellId < numCells; cellId++)
    {
    double t;
    double x[3];
    double pcoords[3];
    pcoords[0] = pcoords[1] = pcoords[2] = 0;
    int newSubId = -1;
    int numSubIds = 1;

    // This will only loop once unless we need to deal with a strip
    for (int subId = 0; subId < numSubIds; subId++)
      {
      data->GetCell(cellId, this->Cell);

      // Transform the points using any transform matrix that may be set.

      for (int i = 0; i < this->Cell->GetPoints()->GetNumberOfPoints(); i++)
        {
        if (this->TransformMatrix)
          {
          double pIn[4] = {0,0,0,1}, pOut[4];
          this->Cell->GetPoints()->GetPoint(i,pIn);
          this->TransformMatrix->MultiplyPoint(pIn,pOut);
          this->Cell->GetPoints()->SetPoint(i,pOut);
          }
        }

      int cellPicked = 0;
      cellPicked = this->Cell->IntersectWithLine(
                       const_cast<double *>(p1), const_cast<double *>(p2),
                       tol, t, x, pcoords, newSubId);

      if (cellPicked)
        {
        return cellPicked;
        }

      } // if a close cell
    } // for all cells

  return 0;
}

//----------------------------------------------------------------------------
int vtkResliceCursorPicker::IntersectPointWithLine(
    double p1[3], double p2[3], double x[3], double tol )
{

  double X[4] = {x[0], x[1], x[2], 1};
  if (this->TransformMatrix)
    {
    double pIn[4] = {x[0], x[1], x[2], 1};
    this->TransformMatrix->MultiplyPoint(pIn,X);
    }

  int i;
  double ray[3], rayFactor, projXYZ[3];


  for (i=0; i<3; i++)
    {
    ray[i] = p2[i] - p1[i];
    }
  if (( rayFactor = vtkMath::Dot(ray,ray)) == 0.0 )
    {
    return 0;
    }
  //
  //  Project each point onto ray. Determine if point is within tolerance.
  //
  const double t = (ray[0]*(X[0]-p1[0]) +
                    ray[1]*(X[1]-p1[1]) +
                    ray[2]*(X[2]-p1[2])) / rayFactor;

  if ( t >= 0.0 && t <= 1.0 )
    {
    for (i=0; i<3; i++)
      {
      projXYZ[i] = p1[i] + t*ray[i];
      if ( fabs(X[i]-projXYZ[i]) > tol )
        {
        break;
        }
      }

    if ( i > 2 ) // within tolerance
      {
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkResliceCursorPicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PickedAxis1: " << this->PickedAxis1 << endl;
  os << indent << "PickedAxis2: " << this->PickedAxis2 << endl;
  os << indent << "PickedCenter: " << this->PickedCenter << endl;
  // this->PointIds;
  os << indent << "ResliceCursorAlgorithm: " <<
        this->ResliceCursorAlgorithm << "\n";
  if (this->ResliceCursorAlgorithm)
    {
    this->ResliceCursorAlgorithm->PrintSelf(os, indent);
    }
  // this->Cell;
}
