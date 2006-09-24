/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageOrthoPlanes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageOrthoPlanes.h"

#include "vtkObjectFactory.h"
#include "vtkImagePlaneWidget.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkCallbackCommand.h"
#include "vtkImageData.h"
#include "vtkTransform.h"

#include <math.h>

//---------------------------------------------------------------------------

vtkCxxRevisionMacro(vtkImageOrthoPlanes, "1.1"); 
vtkStandardNewMacro(vtkImageOrthoPlanes);

//---------------------------------------------------------------------------
static void vtkImageOrthoPlanesInteractionCallback(
  vtkObject *obj,
  unsigned long,
  void *clientData,
  void *vtkNotUsed(callData))
{
  vtkImagePlaneWidget *currentImagePlane
    = vtkImagePlaneWidget::SafeDownCast(obj);
  vtkImageOrthoPlanes *orthoPlane
    = reinterpret_cast<vtkImageOrthoPlanes *>(clientData);

  orthoPlane->HandlePlaneEvent(currentImagePlane);
}

//-----------------------------------------------------------------------
vtkImageOrthoPlanes::vtkImageOrthoPlanes()
{
  for (int i = 0; i < 3; i++)
    {
    this->Planes[i] = 0;
    this->ObserverTags[i] = 0;

    this->Origin[i][0] = 0.0;
    this->Origin[i][1] = 0.0;
    this->Origin[i][2] = 0.0;
    this->Point1[i][0] = 1.0;
    this->Point1[i][1] = 0.0;
    this->Point1[i][2] = 0.0;
    this->Point2[i][0] = 0.0;
    this->Point2[i][1] = 1.0;
    this->Point2[i][2] = 0.0;
    }

  this->Transform = vtkTransform::New();
}

//-----------------------------------------------------------------------
vtkImageOrthoPlanes::~vtkImageOrthoPlanes()
{
  if (this->Transform)
    {
    this->Transform->Delete();
    }

  for (int i = 0; i < 3; i++)
    {
    if (this->Planes[i])
      {
      this->Planes[i]->RemoveObserver(this->ObserverTags[i]);
      this->Planes[i]->Delete();
      }
    }
}

//---------------------------------------------------------------------------
void vtkImageOrthoPlanes::HandlePlaneEvent(
  vtkImagePlaneWidget *currentImagePlane)
{
  int i = 0;

  // Find out which plane the event came from
  int indexOfModifiedPlane = -1;
  for (i = 0; i < 3; i++)
    {
    if (this->Planes[i] == currentImagePlane)
      {
      indexOfModifiedPlane = i ; 
      break; 
      }
    }

  if (indexOfModifiedPlane == -1)
    {
    vtkGenericWarningMacro("vtkImageOrthoPlanes: Unidentified plane "
                           << currentImagePlane);
    return;
    }

  // Origin, point1, point2
  double p0[3];
  double p1[3];
  double p2[3];

  currentImagePlane->GetOrigin(p0);
  currentImagePlane->GetPoint1(p1);
  currentImagePlane->GetPoint2(p2);

  // Two vectors defining the plane orientation
  double v1[3];
  double v2[3];

  currentImagePlane->GetVector1(v1);
  currentImagePlane->GetVector2(v2);
  double xSize = vtkMath::Norm(v1);
  double ySize = vtkMath::Norm(v2);
  vtkMath::Normalize(v1);
  vtkMath::Normalize(v2);
  
  // Extract the three columns of the current orientation matrix
  double u1[3]; u1[0] = 1; u1[1] = 0; u1[2] = 0;
  double u2[3]; u2[0] = 0; u2[1] = 1; u2[2] = 0;
  double u3[3]; u3[0] = 0; u3[1] = 0; u3[2] = 1;
  
  this->Transform->TransformVector(u1, u1);
  this->Transform->TransformVector(u2, u2);
  this->Transform->TransformVector(u3, u3);
  vtkMath::Normalize(u1);
  vtkMath::Normalize(u2);
  vtkMath::Normalize(u3);

  // Compare these against the plane orientation by calculating dot
  // products.  The closer the dot product is to 1.0, the smaller
  // the difference in orientation.
  double dot1 = 0;
  double dot2 = 0;
  switch (indexOfModifiedPlane)
    {
    case 0:
      dot1 = vtkMath::Dot(v1, u2);
      dot2 = vtkMath::Dot(v2, u3);
      break;
    case 1:
      dot1 = vtkMath::Dot(v1, u3);
      dot2 = vtkMath::Dot(v2, u1);
      break;
    case 2:
      dot1 = vtkMath::Dot(v1, u1);
      dot2 = vtkMath::Dot(v2, u2);
      break;
    default:
      break;
    }
    
  // Use the dot product to determine whether the plane has rotated
  if (fabs(1.0 - dot1) > 1e-8 || fabs(1.0 - dot2) > 1e-8)
    {
    this->HandlePlaneRotation(currentImagePlane, indexOfModifiedPlane);
    return;
    }

  // Check for scale change
  double q0[3];
  double q1[3];
  double q2[3];

  this->Transform->TransformPoint(this->Origin[indexOfModifiedPlane], q0);
  this->Transform->TransformPoint(this->Point1[indexOfModifiedPlane], q1);
  this->Transform->TransformPoint(this->Point2[indexOfModifiedPlane], q2);

  double xSizeOld = sqrt(vtkMath::Distance2BetweenPoints(q0, q1));
  double ySizeOld = sqrt(vtkMath::Distance2BetweenPoints(q0, q2));

  if (fabs((xSize - xSizeOld)/xSizeOld) > 1e-5 ||
      fabs((ySize - ySizeOld)/ySizeOld) > 1e-5)
    {
    this->HandlePlaneScale(currentImagePlane, indexOfModifiedPlane);
    return;
    }

  // Check for translation of plane
  double newCenter[3];
  currentImagePlane->GetCenter(newCenter);
  double oldCenter[3];
  oldCenter[0] = 0.5*(this->Point1[indexOfModifiedPlane][0] +
                      this->Point2[indexOfModifiedPlane][0]);
  oldCenter[1] = 0.5*(this->Point1[indexOfModifiedPlane][1] +
                      this->Point2[indexOfModifiedPlane][1]);
  oldCenter[2] = 0.5*(this->Point1[indexOfModifiedPlane][2] +
                      this->Point2[indexOfModifiedPlane][2]);
  this->Transform->TransformPoint(oldCenter, oldCenter);

  if (sqrt(vtkMath::Distance2BetweenPoints(newCenter, oldCenter)) > 1e-5)
    {
    double vec[3];
    vec[0] = newCenter[0] - oldCenter[0];
    vec[1] = newCenter[1] - oldCenter[1];
    vec[2] = newCenter[2] - oldCenter[2];

    if (fabs(vtkMath::Dot(v1, vec)) < 1e-5 && 
        fabs(vtkMath::Dot(v2, vec)) < 1e-5)
      {
      this->HandlePlanePush(currentImagePlane, indexOfModifiedPlane);
      }
    else
      {
      this->HandlePlaneTranslate(currentImagePlane, indexOfModifiedPlane);
      }
    }
}

//-----------------------------------------------------------------------
void vtkImageOrthoPlanes::HandlePlanePush(
  vtkImagePlaneWidget *currentImagePlane,
  int indexOfModifiedPlane)
{
  int i = indexOfModifiedPlane;
    
  // Get the bounds of the input image data
  vtkImageData *input = 
    vtkImageData::SafeDownCast(currentImagePlane->GetInput());
  if (!input)
    {
    return;
    }

  int extent[6];
  double origin[3];
  double spacing[3];
  input->UpdateInformation();
  input->GetWholeExtent(extent);
  input->GetOrigin(origin);
  input->GetSpacing(spacing);

  double bounds[2];
  bounds[0] = origin[i] + spacing[i]*extent[2*i];
  bounds[1] = origin[i] + spacing[i]*extent[2*i+1];

  // Get the information for the plane
  double center[3];
  currentImagePlane->GetCenter(center);

  this->Transform->GetInverse()->TransformPoint(center, center);

  if (center[i] < bounds[0] || center[i] > bounds[1])
    {
    if (center[i] < bounds[0])
      {
      center[i] = bounds[0];
      }
    if (center[i] > bounds[1])
      {
      center[i] = bounds[1];
      }

    double origin[3];
    double point1[3];
    double point2[3];

    this->Origin[i][i] = center[i];
    this->Point1[i][i] = center[i];
    this->Point2[i][i] = center[i];

    this->Transform->TransformPoint(this->Origin[i], origin);
    this->Transform->TransformPoint(this->Point1[i], point1);
    this->Transform->TransformPoint(this->Point2[i], point2);

    currentImagePlane->SetOrigin(origin);
    currentImagePlane->SetPoint1(point1);
    currentImagePlane->SetPoint2(point2);

    currentImagePlane->UpdatePlacement();
    }
}

//-----------------------------------------------------------------------
void vtkImageOrthoPlanes::HandlePlaneTranslate(
  vtkImagePlaneWidget *currentImagePlane,
  int indexOfModifiedPlane)
{
  // Find out how large the translation is

  double newCenter[3];
  currentImagePlane->GetCenter(newCenter);
  double oldCenter[3];
  oldCenter[0] = 0.5*(this->Point1[indexOfModifiedPlane][0] +
                      this->Point2[indexOfModifiedPlane][0]);
  oldCenter[1] = 0.5*(this->Point1[indexOfModifiedPlane][1] +
                      this->Point2[indexOfModifiedPlane][1]);
  oldCenter[2] = 0.5*(this->Point1[indexOfModifiedPlane][2] +
                      this->Point2[indexOfModifiedPlane][2]);
  this->Transform->TransformPoint(oldCenter, oldCenter);

  double vec[3];
  vec[0] = newCenter[0] - oldCenter[0];
  vec[1] = newCenter[1] - oldCenter[1];
  vec[2] = newCenter[2] - oldCenter[2];  

  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();

  this->Transform->GetMatrix(matrix);

  matrix->SetElement(0, 3, matrix->GetElement(0, 3) + vec[0]);
  matrix->SetElement(1, 3, matrix->GetElement(1, 3) + vec[1]);
  matrix->SetElement(2, 3, matrix->GetElement(2, 3) + vec[2]);

  this->SetTransformMatrix(matrix, currentImagePlane);  

  matrix->Delete();
}

//-----------------------------------------------------------------------
void vtkImageOrthoPlanes::HandlePlaneRotation(
  vtkImagePlaneWidget *currentImagePlane,
  int indexOfModifiedPlane)
{
  int i = 0;

  // Get the current scale
  double scale[3];
  for (i = 0; i < 3; i++)
    {
    double col[3];
    col[0] = col[1] = col[2] = 0.0;
    col[i] = 1.0;
    this->Transform->TransformVector(col, col);
    scale[i] = vtkMath::Norm(col);
    }
    
  // Create a matrix from the plane orientation
  double v1[3];
  double v2[3];
  double v3[3];

  currentImagePlane->GetVector1(v1);
  currentImagePlane->GetVector2(v2);
  vtkMath::Normalize(v1);
  vtkMath::Normalize(v2);
  vtkMath::Cross(v1, v2, v3);

  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  switch (indexOfModifiedPlane)
    {
    case 0:
      for (i = 0; i < 3; i++)
        {
        matrix->SetElement(i, 0, v3[i]*scale[0]);
        matrix->SetElement(i, 1, v1[i]*scale[1]);
        matrix->SetElement(i, 2, v2[i]*scale[2]);
        }
      break;
    case 1:
      for (i = 0; i < 3; i++)
        {
        matrix->SetElement(i, 0, v2[i]*scale[0]);
        matrix->SetElement(i, 1, v3[i]*scale[1]);
        matrix->SetElement(i, 2, v1[i]*scale[2]);
        }
      break;
    case 2:
      for (i = 0; i < 3; i++)
        {
        matrix->SetElement(i, 0, v1[i]*scale[0]);
        matrix->SetElement(i, 1, v2[i]*scale[1]);
        matrix->SetElement(i, 2, v3[i]*scale[2]);
        }
      break;
    default:
      break;
    }
  
  // Get the center of the rotated plane
  double center[3];
  currentImagePlane->GetCenter(center);

  // Grab the previous translation
  double translation[3];
  translation[0] = 0.0;
  translation[1] = 0.0;
  translation[2] = 0.0;

  this->Transform->TransformPoint(translation, translation);
  
  // Invert and multiply by new rotation to get the relative rotation
  vtkTransform *rotationTransform = vtkTransform::New();
  rotationTransform->PostMultiply();
  rotationTransform->Translate(translation);
  this->Transform->GetLinearInverse()->Update();
  rotationTransform->Concatenate(
    this->Transform->GetLinearInverse()->GetMatrix());
  rotationTransform->Concatenate(matrix);
      
  // Make this into a rotation about the center of the modified plane 
  rotationTransform->PreMultiply();
  rotationTransform->Translate(-center[0], -center[1], -center[2]);
  rotationTransform->PostMultiply();
  rotationTransform->Translate(center[0], center[1], center[2]);
  // Apply this rotation to the translation
  rotationTransform->TransformPoint(translation, translation);
  rotationTransform->Delete();

  matrix->SetElement(0, 3, translation[0]); 
  matrix->SetElement(1, 3, translation[1]); 
  matrix->SetElement(2, 3, translation[2]); 
 
  this->SetTransformMatrix(matrix, currentImagePlane);  

  matrix->Delete();
}

//-----------------------------------------------------------------------
void vtkImageOrthoPlanes::HandlePlaneScale(
  vtkImagePlaneWidget *currentImagePlane,
  int indexOfModifiedPlane)
{
  int i = 0;

  // Calculate the new scale
  double p0[3];
  double p1[3];
  double p2[3];

  currentImagePlane->GetOrigin(p0);
  currentImagePlane->GetPoint1(p1);
  currentImagePlane->GetPoint2(p2);
  double xSize = sqrt(vtkMath::Distance2BetweenPoints(p0, p1));
  double ySize = sqrt(vtkMath::Distance2BetweenPoints(p0, p2));

  // Check for previous scale
  double q0[3];
  double q1[3];
  double q2[3];

  this->Transform->TransformPoint(this->Origin[indexOfModifiedPlane], q0);
  this->Transform->TransformPoint(this->Point1[indexOfModifiedPlane], q1);
  this->Transform->TransformPoint(this->Point2[indexOfModifiedPlane], q2);
  double xSizeOld = sqrt(vtkMath::Distance2BetweenPoints(q0, q1));
  double ySizeOld = sqrt(vtkMath::Distance2BetweenPoints(q0, q2));

  // Check for original scale
  double xSizeOrig = sqrt(vtkMath::Distance2BetweenPoints(
                            this->Origin[indexOfModifiedPlane],
                            this->Point1[indexOfModifiedPlane]));
  double ySizeOrig = sqrt(vtkMath::Distance2BetweenPoints(
                            this->Origin[indexOfModifiedPlane],
                            this->Point2[indexOfModifiedPlane]));

  // Get the center for the scale
  double center[3];
  currentImagePlane->GetCenter(center);  

  // Get the previous center
  double oldCenter[3];
  oldCenter[0] = 0.5*(q1[0] + q2[0]);
  oldCenter[1] = 0.5*(q1[1] + q2[1]);
  oldCenter[2] = 0.5*(q1[2] + q2[2]);
  
  // Check whether the center has changed position.  If it has, then
  // the user has grabbed a corner.
  double zScale = 1.0;
  if (sqrt(vtkMath::Distance2BetweenPoints(center, oldCenter)) <= 1e-5)
    {
    zScale = sqrt((xSize/xSizeOld)*(ySize/ySizeOld));
    }

  // Find the absolute scale and the relative change
  double scale[3];
  double relativeScale[3];

  switch (indexOfModifiedPlane)
    {
    case 0:
      scale[1] = xSize/xSizeOrig;
      scale[2] = ySize/ySizeOrig;
      scale[0] = zScale;
      relativeScale[1] = xSize/xSizeOld;
      relativeScale[2] = ySize/ySizeOld;
      relativeScale[0] = zScale;
      break;
    case 1:
      scale[2] = xSize/xSizeOrig;
      scale[0] = ySize/ySizeOrig;
      scale[1] = zScale;
      relativeScale[2] = xSize/xSizeOld;
      relativeScale[0] = ySize/ySizeOld;
      relativeScale[1] = zScale;
      break;
    case 2:
      scale[0] = xSize/xSizeOrig;
      scale[1] = ySize/ySizeOrig;
      scale[2] = zScale;
      relativeScale[0] = xSize/xSizeOld;
      relativeScale[1] = ySize/ySizeOld;
      relativeScale[2] = zScale;
      break;
    }

  // Create a matrix for the relative change
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  for (i = 0; i < 3; i++)
    {
    double col[3];
    col[0] = col[1] = col[2] = 0.0;
    col[i] = 1.0;

    this->Transform->TransformVector(col, col);
    vtkMath::Normalize(col);

    matrix->SetElement(0, i, col[0]*relativeScale[i]);
    matrix->SetElement(1, i, col[1]*relativeScale[i]);
    matrix->SetElement(2, i, col[2]*relativeScale[i]);
    }

  // Grab the previous translation from the transform
  double translation[3];
  translation[0] = 0.0;
  translation[1] = 0.0;
  translation[2] = 0.0;

  this->Transform->TransformPoint(translation, translation);

  // Modify this translation according to the scale
  vtkTransform *transform = vtkTransform::New();
  transform->PostMultiply();
  transform->Translate(-oldCenter[0], -oldCenter[1], -oldCenter[2]);
  transform->Concatenate(matrix);
  transform->Translate(center[0], center[1], center[2]);
  transform->TransformPoint(translation, translation);
  transform->Delete();

  // Create a new matrix that contains the new scale
  for (i = 0; i < 3; i++)
    {
    double col[3];
    col[0] = col[1] = col[2] = 0.0;
    col[i] = 1.0;

    this->Transform->TransformVector(col, col);
    if (i != indexOfModifiedPlane)
      {
      vtkMath::Normalize(col);
      }
    col[0] *= scale[i];
    col[1] *= scale[i];
    col[2] *= scale[i];

    matrix->SetElement(0, i, col[0]);
    matrix->SetElement(1, i, col[1]);
    matrix->SetElement(2, i, col[2]);
    matrix->SetElement(i, 3, translation[i]);
    }

  this->SetTransformMatrix(matrix, currentImagePlane);

  matrix->Delete();
}    

//-----------------------------------------------------------------------
void vtkImageOrthoPlanes::SetTransformMatrix(
  vtkMatrix4x4 *matrix,
  vtkImagePlaneWidget *currentImagePlane)
{
  int i = 0;

  // Calculate the intersection of the planes so that it can be preserved
  double intersectionPoint[3];
  for (i = 0; i < 3; i++)
    {
    double q1[3];
    double q2[3];

    this->Planes[i]->GetPoint1(q1);
    this->Planes[i]->GetPoint2(q2);
      
    this->Transform->GetInverse()->TransformPoint(q1, q1);
    this->Transform->GetInverse()->TransformPoint(q2, q2);

    intersectionPoint[i] = 0.5*(q1[i] + q2[i]);
    }

  // Set the new transform
  this->Transform->Identity();
  this->Transform->Concatenate(matrix);
      
  // Apply this transform to the three planes
  for (i = 0; i < 3; i++)
    {
    vtkImagePlaneWidget *planeWidget = this->Planes[i];
    if (planeWidget != currentImagePlane)
      {
      double origin[3];
      double point1[3];
      double point2[3];

      this->Origin[i][i] = intersectionPoint[i];
      this->Point1[i][i] = intersectionPoint[i];
      this->Point2[i][i] = intersectionPoint[i];
      
      this->Transform->TransformPoint(this->Origin[i], origin);
      this->Transform->TransformPoint(this->Point1[i], point1);
      this->Transform->TransformPoint(this->Point2[i], point2);

      planeWidget->SetOrigin(origin);
      planeWidget->SetPoint1(point1);
      planeWidget->SetPoint2(point2);

      planeWidget->UpdatePlacement();
      }
    }
}

//-----------------------------------------------------------------------
void vtkImageOrthoPlanes::SetPlane(
  int i, vtkImagePlaneWidget *currentImagePlane)
{
  if (i >= 0 && i < 3)
    {
    if (this->Planes[i])
      {
      this->Planes[i]->RemoveObserver(this->ObserverTags[i]);
      this->Planes[i]->Delete();
      }
     
    this->Planes[i] = currentImagePlane;

    if (currentImagePlane == 0)
      {
      return;
      }

    vtkCallbackCommand* callbackCommand = vtkCallbackCommand::New();
    callbackCommand->SetClientData(this);
    callbackCommand->SetCallback(vtkImageOrthoPlanesInteractionCallback);
    this->ObserverTags[i] =
      currentImagePlane->AddObserver(vtkCommand::InteractionEvent,
                                     callbackCommand, 1.0);
    callbackCommand->Delete();

    currentImagePlane->SetPlaneOrientation(i);
    currentImagePlane->RestrictPlaneToVolumeOff();
    currentImagePlane->GetOrigin(this->Origin[i]);
    currentImagePlane->GetPoint1(this->Point1[i]);
    currentImagePlane->GetPoint2(this->Point2[i]);

    currentImagePlane->Register(this);
    }
  else
    {
    vtkErrorMacro("wrong plane index");
    return;
    }

  // Diagnostic: only do this the last time
  if (this->Planes[0] && this->Planes[1] && this->Planes[2])
    {
    this->Transform->Identity();
    }
}

//-----------------------------------------------------------------------
vtkImagePlaneWidget* vtkImageOrthoPlanes::GetPlane(int i)
{
  if (i < 0 || i >= 3)
    {
     vtkErrorMacro("requested invalid plane index");
     return 0;
    }
  else 
    {
    return this->Planes[i];
    }
}

//-----------------------------------------------------------------------
void vtkImageOrthoPlanes::ResetPlanes()
{
  int i = 0;

  this->Transform->Identity();

  double intersection[3];
  double center[3];
  for (i = 0; i < 3; i++)
    {
    this->Planes[i]->GetCenter(center);
    intersection[(i+1)%3] = center[(i+1)%3];
    }
  
  for (i = 0; i < 3; i++)
    {
    this->Origin[i][i] = intersection[i];
    this->Point1[i][i] = intersection[i];
    this->Point2[i][i] = intersection[i];

    this->Planes[i]->SetOrigin(this->Origin[i]);
    this->Planes[i]->SetPoint1(this->Point1[i]);
    this->Planes[i]->SetPoint2(this->Point2[i]);

    this->Planes[i]->UpdatePlacement();
    }

  this->Modified();
}

//-----------------------------------------------------------------------
void vtkImageOrthoPlanes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Transform: " << this->Transform << "\n";
  this->Transform->PrintSelf(os, indent.GetNextIndent());
}




