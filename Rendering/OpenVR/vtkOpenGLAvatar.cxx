/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLAvatar.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLAvatar.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLState.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkOpenGLError.h"
#include "vtkRenderWindow.h"
#include "vtkShaderProgram.h"
#include "vtkTexture.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkAvatarHead.h" // geometry for head
#include "vtkAvatarLeftHand.h" // geometry for hand
#include "vtkAvatarRightHand.h" // geometry for hand
#include "vtkAvatarTorso.h" // geometry for torso
#include "vtkAvatarLeftForeArm.h" // geometry for torso
#include "vtkAvatarRightForeArm.h" // geometry for torso

#include <cmath>

vtkStandardNewMacro(vtkOpenGLAvatar);

vtkOpenGLAvatar::vtkOpenGLAvatar()
{
  vtkNew<vtkXMLPolyDataReader> reader;
  reader->SetReadFromInputString(true);
  reader->SetInputString(std::string(vtkAvatarHead, vtkAvatarHead + sizeof vtkAvatarHead));
  reader->Update();

  this->HeadMapper->SetInputData(reader->GetOutput());
  this->SetMapper(this->HeadMapper);
  this->HeadActor->SetMapper(this->HeadMapper);

  vtkNew<vtkXMLPolyDataReader> reader2;
  reader2->SetReadFromInputString(true);
  reader2->SetInputString(std::string(vtkAvatarLeftHand, vtkAvatarLeftHand + sizeof vtkAvatarLeftHand));
  reader2->Update();
  this->LeftHandMapper->SetInputData(reader2->GetOutput());
  this->LeftHandActor->SetMapper(this->LeftHandMapper);

  vtkNew<vtkXMLPolyDataReader> reader3;
  reader3->SetReadFromInputString(true);
  reader3->SetInputString(std::string(vtkAvatarRightHand, vtkAvatarRightHand + sizeof vtkAvatarRightHand));
  reader3->Update();
  this->RightHandMapper->SetInputData(reader3->GetOutput());
  this->RightHandActor->SetMapper(this->RightHandMapper);

  const unsigned char *models[NUM_BODY] = {
    vtkAvatarTorso,
    vtkAvatarLeftForeArm,
    vtkAvatarRightForeArm,
    nullptr,
    nullptr,
  };
  size_t modelSize[NUM_BODY] = {
    sizeof vtkAvatarTorso,
    sizeof vtkAvatarLeftForeArm,
    sizeof vtkAvatarRightForeArm,
    0,
    0,
  };

  this->GetProperty()->SetDiffuse(0.7);
  this->GetProperty()->SetAmbient(0.3);
  this->GetProperty()->SetSpecular(0.0);
  // link properties, share color.
  this->HeadActor->SetProperty(this->GetProperty());
  this->LeftHandActor->SetProperty(this->GetProperty());
  this->RightHandActor->SetProperty(this->GetProperty());

  for (int i = 0; i < NUM_BODY; ++i) {
    if (!models[i]) continue;
    vtkNew<vtkXMLPolyDataReader> reader4;
    reader4->SetReadFromInputString(true);
    reader4->SetInputString(std::string(models[i], models[i] + modelSize[i]));
    reader4->Update();
    this->BodyMapper[i]->SetInputData(reader4->GetOutput());
    this->BodyActor[i]->SetMapper(this->BodyMapper[i]);

    this->BodyActor[i]->SetProperty(this->GetProperty());
  }

}

vtkOpenGLAvatar::~vtkOpenGLAvatar() = default;

// Actual Avatar render method.
void vtkOpenGLAvatar::Render(vtkRenderer *ren, vtkMapper *mapper)
{
  vtkOpenGLClearErrorMacro();

  this->CalcBody();

  this->HeadActor->SetScale(this->GetScale());
  this->HeadActor->SetPosition(this->HeadPosition);
  this->HeadActor->SetOrientation(this->HeadOrientation);
  this->LeftHandActor->SetScale(this->GetScale());
  this->LeftHandActor->SetPosition(this->LeftHandPosition);
  this->LeftHandActor->SetOrientation(this->LeftHandOrientation);
  this->RightHandActor->SetScale(this->GetScale());
  this->RightHandActor->SetPosition(this->RightHandPosition);
  this->RightHandActor->SetOrientation(this->RightHandOrientation);


  // send a render to the mapper; update pipeline
  mapper->Render(ren, this->HeadActor);
  this->LeftHandMapper->Render(ren, this->LeftHandActor);
  this->RightHandMapper->Render(ren, this->RightHandActor);
  for (int i = 0; i <= RIGHT_FORE; ++i) {
    this->BodyActor[i]->SetScale(this->GetScale());
    this->BodyActor[i]->SetPosition(this->BodyPosition[i]);
    this->BodyActor[i]->SetOrientation(this->BodyOrientation[i]);
    this->BodyMapper[i]->Render(ren, this->BodyActor[i]);
  }
  vtkOpenGLCheckErrorMacro("failed after Render");
}

void vtkOpenGLAvatar::CalcBody()
{
  this->BodyPosition[TORSO][0] = this->HeadPosition[0];
  this->BodyPosition[TORSO][1] = this->HeadPosition[1];
  this->BodyPosition[TORSO][2] = this->HeadPosition[2];

  // keep the head orientation in the direction of the up vector.
  this->BodyOrientation[TORSO][0] = this->HeadOrientation[0] * this->UpVector[0];
  this->BodyOrientation[TORSO][1] = this->HeadOrientation[1] * this->UpVector[1];
  this->BodyOrientation[TORSO][2] = this->HeadOrientation[2] * this->UpVector[2];

  // Initial try - keep forearm rigidly attached to hand.
  this->BodyPosition[LEFT_FORE][0] = this->LeftHandPosition[0];
  this->BodyPosition[LEFT_FORE][1] = this->LeftHandPosition[1];
  this->BodyPosition[LEFT_FORE][2] = this->LeftHandPosition[2];

  this->BodyOrientation[LEFT_FORE][0] = this->LeftHandOrientation[0];
  this->BodyOrientation[LEFT_FORE][1] = this->LeftHandOrientation[1];
  this->BodyOrientation[LEFT_FORE][2] = this->LeftHandOrientation[2];

  this->BodyPosition[RIGHT_FORE][0] = this->RightHandPosition[0];
  this->BodyPosition[RIGHT_FORE][1] = this->RightHandPosition[1];
  this->BodyPosition[RIGHT_FORE][2] = this->RightHandPosition[2];

  this->BodyOrientation[RIGHT_FORE][0] = this->RightHandOrientation[0];
  this->BodyOrientation[RIGHT_FORE][1] = this->RightHandOrientation[1];
  this->BodyOrientation[RIGHT_FORE][2] = this->RightHandOrientation[2];

}

//----------------------------------------------------------------------------
void vtkOpenGLAvatar::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
