/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProjectedTetrahedraMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkVolumeStateRAII_h
#define __vtkVolumeStateRAII_h

// Only these states can be queries via glIsEnabled:
// http://www.khronos.org/opengles/sdk/docs/man/

class vtkVolumeStateRAII
  {
  public:
    vtkVolumeStateRAII()
      {
      this->DepthTestEnabled = glIsEnabled(GL_DEPTH_TEST);

      this->BlendEnabled = glIsEnabled(GL_BLEND);

      this->CullFaceEnabled = glIsEnabled(GL_CULL_FACE);

      // Enable texture 1D and 3D as we are using it
      // for transfer functions and m_volume data
      glEnable(GL_TEXTURE_1D);
      glEnable(GL_TEXTURE_2D);
      glEnable(GL_TEXTURE_3D);

      // Enable depth_sampler test
      if (!this->DepthTestEnabled)
        {
        std::cerr << "enabling depth test" << std::endl;
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
      }

    ~vtkVolumeStateRAII()
      {
      glBindVertexArray(0);

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

      glActiveTexture(GL_TEXTURE0);

      glDisable(GL_TEXTURE_3D);
      glDisable(GL_TEXTURE_2D);
      glDisable(GL_TEXTURE_1D);
      }

private:
  bool DepthTestEnabled;
  bool BlendEnabled;
  bool CullFaceEnabled;
};

#endif // __vtkVolumeStateRAII_h
// VTK-HeaderTest-Exclude: vtkVolumeStateRAII.h
