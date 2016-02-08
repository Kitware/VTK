/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOsprayTestInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Description
// A common interactor style for the ospray tests.

#include "vtkOsprayTestInteractor.h"
#include "vtkObjectFactory.h"

#include "vtkOpenGLRenderer.h"
#include "vtkOsprayRendererNode.h"
#include "vtkRenderPass.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOsprayTestInteractor);

//----------------------------------------------------------------------------
vtkOsprayTestInteractor::vtkOsprayTestInteractor()
{
  this->SetPipelineControlPoints(NULL,NULL,NULL);
  this->VisibleActor = -1;
}

//----------------------------------------------------------------------------
void vtkOsprayTestInteractor::SetPipelineControlPoints
  (vtkOpenGLRenderer *g,
   vtkRenderPass *_O,
   vtkRenderPass *_G)
{
  this->GLRenderer = g;
  this->O = _O;
  this->G = _G;
}

//----------------------------------------------------------------------------
void vtkOsprayTestInteractor::OnKeyPress()
{
  if (this->GLRenderer == NULL)
    {
    return;
    }

  // Get the keypress
  vtkRenderWindowInteractor *rwi = this->Interactor;
  std::string key = rwi->GetKeySym();

  if(key == "c")
    {
    vtkRenderPass * current = this->GLRenderer->GetPass();
    if (current == this->G)
      {
      cerr << "OSPRAY rendering" << this->O << endl;
      this->GLRenderer->SetPass(this->O);
      this->GLRenderer->GetRenderWindow()->Render();
      }
    else if (current == this->O)
      {
      cerr << "GL rendering" << this->G << endl;
      this->GLRenderer->SetPass(this->G);
      this->GLRenderer->GetRenderWindow()->Render();
      }
    }

  if(key == "n")
    {
    vtkActorCollection * actors = this->GLRenderer->GetActors();

    this->VisibleActor++;
    cerr << "VISIBLE " << this->VisibleActor << " : ";
    if (this->VisibleActor == actors->GetNumberOfItems())
      {
      this->VisibleActor = -1;
      }
    for (int i = 0; i < actors->GetNumberOfItems(); i++)
      {
      if (this->VisibleActor == -1 || this->VisibleActor == i)
        {
        //cerr << names[i] << " ";
        vtkActor::SafeDownCast(actors->GetItemAsObject(i))->
          SetVisibility(1);
        }
      else
        {
        vtkActor::SafeDownCast(actors->GetItemAsObject(i))->
              SetVisibility(0);
        }
      }
    cerr << endl;
    this->GLRenderer->ResetCamera();
    this->GLRenderer->GetRenderWindow()->Render();
    }

  if(key == "P")
    {
    vtkOsprayRendererNode::maxframes=
      vtkOsprayRendererNode::maxframes+=4;
    if (vtkOsprayRendererNode::maxframes>64)
      {
      vtkOsprayRendererNode::maxframes=64;
      }
    cerr << "MF" << vtkOsprayRendererNode::maxframes << endl;
    this->GLRenderer->GetRenderWindow()->Render();
    }

  if(key == "p")
    {
    if (vtkOsprayRendererNode::maxframes>1)
      {
      vtkOsprayRendererNode::maxframes=
        vtkOsprayRendererNode::maxframes/2;
      }
    cerr << "MF" << vtkOsprayRendererNode::maxframes << endl;
    this->GLRenderer->GetRenderWindow()->Render();
    }

  if(key == "s")
    {
    cerr << "change shadows" << endl;
    if (vtkOsprayRendererNode::doshadows)
      {
      vtkOsprayRendererNode::doshadows=0;
      }
    else
      {
      vtkOsprayRendererNode::doshadows=1;
      }
    this->GLRenderer->GetRenderWindow()->Render();
    }

  if(key == "2")
    {
    cerr << "change SPP" << endl;
    vtkOsprayRendererNode::spp++;
    this->GLRenderer->GetRenderWindow()->Render();
    }
  if(key == "1")
    {
    cerr << "change SPP" << endl;
    vtkOsprayRendererNode::spp=0;
    this->GLRenderer->GetRenderWindow()->Render();
    }

  // Forward events
  vtkInteractorStyleTrackballCamera::OnKeyPress();
}
