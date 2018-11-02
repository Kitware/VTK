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

#include <cmath>

vtkStandardNewMacro(vtkOpenGLAvatar);


vtkOpenGLAvatar::vtkOpenGLAvatar()
{
  vtkNew<vtkXMLPolyDataReader> reader;
  reader->SetReadFromInputString(true);
  reader->SetInputString(vtkAvatarHead);
  reader->Update();

  this->HeadMapper->SetInputData(reader->GetOutput());
  this->SetMapper(this->HeadMapper);
  this->HeadActor->SetMapper(this->HeadMapper);

  vtkNew<vtkXMLPolyDataReader> reader2;
  reader2->SetReadFromInputString(true);
  reader2->SetInputString(vtkAvatarLeftHand);
  reader2->Update();
  this->LeftHandMapper->SetInputData(reader2->GetOutput());
  this->LeftHandActor->SetMapper(this->LeftHandMapper);

  vtkNew<vtkXMLPolyDataReader> reader3;
  reader3->SetReadFromInputString(true);
  reader3->SetInputString(vtkAvatarRightHand);
  reader3->Update();
  this->RightHandMapper->SetInputData(reader3->GetOutput());
  this->RightHandActor->SetMapper(this->RightHandMapper);

  this->GetProperty()->SetDiffuse(0.7);
  this->GetProperty()->SetAmbient(0.3);
  this->GetProperty()->SetSpecular(0.0);
  // link properties, share color.
  this->HeadActor->SetProperty(this->GetProperty());
  this->LeftHandActor->SetProperty(this->GetProperty());
  this->RightHandActor->SetProperty(this->GetProperty());
}

vtkOpenGLAvatar::~vtkOpenGLAvatar() = default;

// Actual Avatar render method.
void vtkOpenGLAvatar::Render(vtkRenderer *ren, vtkMapper *mapper)
{
  vtkOpenGLClearErrorMacro();

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

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//----------------------------------------------------------------------------
void vtkOpenGLAvatar::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
