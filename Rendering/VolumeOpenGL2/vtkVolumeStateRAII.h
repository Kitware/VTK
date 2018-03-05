/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeStateRAII.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkVolumeStateRAII_h
#define vtkVolumeStateRAII_h
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLState.h"

// Only these states can be queries via glIsEnabled:
// http://www.khronos.org/opengles/sdk/docs/man/

class vtkVolumeStateRAII
{
  public:
    vtkVolumeStateRAII(vtkOpenGLState *ostate, bool noOp = false)
      : NoOp(noOp)
    {
      this->State = ostate;

      if (this->NoOp)
      {
        return;
      }

      this->DepthTestEnabled = ostate->GetEnumState(GL_DEPTH_TEST);

      this->BlendEnabled = ostate->GetEnumState(GL_BLEND);

      this->CullFaceEnabled = ostate->GetEnumState(GL_CULL_FACE);
      this->CullFaceMode = ostate->GetCullFace();

      GLboolean depthMaskWrite = GL_TRUE;
      glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskWrite);
      this->DepthMaskEnabled = (depthMaskWrite == GL_TRUE);

      // Enable depth_sampler test
      ostate->glEnable(GL_DEPTH_TEST);

      // Set the over blending function
      // NOTE: It is important to choose GL_ONE vs GL_SRC_ALPHA as our colors
      // will be premultiplied by the alpha value (doing front to back blending)
      ostate->glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);

      ostate->glEnable(GL_BLEND);

      // Enable cull face and set cull face mode
      ostate->glCullFace(GL_BACK);

      ostate->glEnable(GL_CULL_FACE);

      // Disable depth mask writing
      ostate->glDepthMask(GL_FALSE);
    }

    ~vtkVolumeStateRAII()
    {
      glBindVertexArray(0);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

      if (this->NoOp)
      {
        return;
      }

      this->State->glCullFace(this->CullFaceMode);
      this->State->SetEnumState(GL_CULL_FACE, this->CullFaceEnabled);
      this->State->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      // this does not actuallyrestore the state always
      // but a test failsif I change it so either the original
      // test was wrong or it is itended
      if (this->BlendEnabled)
      {
        this->State->glDisable(GL_BLEND);
      }

      this->State->SetEnumState(GL_DEPTH_TEST, this->DepthTestEnabled);

      if (this->DepthMaskEnabled)
      {
        this->State->glDepthMask(GL_TRUE);
      }
    }

private:
  bool NoOp;
  bool DepthTestEnabled;
  bool BlendEnabled;
  bool CullFaceEnabled;
  GLint CullFaceMode;
  bool DepthMaskEnabled;
  vtkOpenGLState *State;
};

#endif // vtkVolumeStateRAII_h
// VTK-HeaderTest-Exclude: vtkVolumeStateRAII.h
