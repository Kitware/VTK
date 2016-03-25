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

// Only these states can be queries via glIsEnabled:
// http://www.khronos.org/opengles/sdk/docs/man/

class vtkVolumeStateRAII
  {
  public:
    vtkVolumeStateRAII()
      {
      this->DepthTestEnabled = (glIsEnabled(GL_DEPTH_TEST) != 0);

      this->BlendEnabled = (glIsEnabled(GL_BLEND) != 0);

      this->CullFaceEnabled = (glIsEnabled(GL_CULL_FACE) != 0);

      GLboolean depthMaskWrite = GL_TRUE;
      glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskWrite);
      this->DepthMaskEnabled = (depthMaskWrite == GL_TRUE);

      // Enable depth_sampler test
      if (!this->DepthTestEnabled)
        {
        glEnable(GL_DEPTH_TEST);
        }

      // Set the over blending function
      // NOTE: It is important to choose GL_ONE vs GL_SRC_ALPHA as our colors
      // will be premultiplied by the alpha value (doing front to back blending)
      glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);

      if (!this->BlendEnabled)
        {
        glEnable(GL_BLEND);
        }

      // Enable cull face
      if (!this->CullFaceEnabled)
        {
        glEnable(GL_CULL_FACE);
        }

      // Disable depth mask writing
      if (this->DepthMaskEnabled)
        {
        glDepthMask(GL_FALSE);
        }
      }

    ~vtkVolumeStateRAII()
      {
#ifdef __APPLE__
      if (vtkOpenGLRenderWindow::GetContextSupportsOpenGL32())
#endif
        {
        glBindVertexArray(0);
        }
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

      if (!this->CullFaceEnabled)
        {
        glDisable(GL_CULL_FACE);
        }

      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      if (!this->BlendEnabled)
        {
        glDisable(GL_BLEND);
        }

      if (!this->DepthTestEnabled)
        {
        glDisable(GL_DEPTH_TEST);
        }

      if (this->DepthMaskEnabled)
        {
        glDepthMask(GL_TRUE);
        }
      }

private:
  bool DepthTestEnabled;
  bool BlendEnabled;
  bool CullFaceEnabled;
  bool DepthMaskEnabled;
};

#endif // vtkVolumeStateRAII_h
// VTK-HeaderTest-Exclude: vtkVolumeStateRAII.h
