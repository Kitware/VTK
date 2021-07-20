/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLMovieSphere.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLMovieSphere
 * @brief   OpenGL MovieSphere, an optimized VR sphere for movies
 *
 * This class is designed to more efficiently convert ffmpeg output to a
 * movie sphere. The vtkSkybox will work, but it does an expensive
 * conversion of the data from YUV to RGB, does an extra copy of the RGB
 * data and the RGB data is twice as large when pushing to the GPU. This
 * class also uses double buffering of textures to help prevent pipeline
 * stalls. In a quick test between the two classes playing a 4K30p video
 * sphere along with VR rendering, decoding etc just switching out the
 * vtkSkybox for this class resulting in CPU usage going from 124 seconds
 * down to 81 seconds. Likewise the frame timings in VR became noticably
 * better which could partially be due to pushing half as much data to the
 * GPU. (YUV420 is half the size of RGB)
 */

#ifndef vtkOpenGLMovieSphere_h
#define vtkOpenGLMovieSphere_h

#include "vtkNew.h" // for ivars
#include "vtkOpenGLSkybox.h"
#include "vtkRenderingFFMPEGOpenGL2Module.h" // For export macro
#include <atomic>                            // for ivars

class vtkFFMPEGVideoSource;
struct vtkFFMPEGVideoSourceVideoCallbackData;
class vtkMutexLock;
class vtkOpenGLActor;
class vtkOpenGLPolyDataMapper;
class vtkTextureObject;

class VTKRENDERINGFFMPEGOPENGL2_EXPORT vtkOpenGLMovieSphere : public vtkOpenGLSkybox
{
public:
  static vtkOpenGLMovieSphere* New();
  vtkTypeMacro(vtkOpenGLMovieSphere, vtkOpenGLSkybox);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Actual MovieSphere render method.
   */
  void Render(vtkRenderer* ren, vtkMapper* mapper) override;

  void SetVideoSource(vtkFFMPEGVideoSource* val);

protected:
  vtkOpenGLMovieSphere();
  ~vtkOpenGLMovieSphere() override;

  void UpdateUniforms(vtkObject*, unsigned long, void*);

  vtkNew<vtkTextureObject> Textures[6];
  int BuildIndex;
  int DrawIndex;
  int YTexture;
  int UTexture;
  int VTexture;

  void VideoCallback(vtkFFMPEGVideoSourceVideoCallbackData const& cbd);

  vtkNew<vtkMutexLock> TextureUpdateMutex;
  unsigned char* TextureData[6];
  int ReadIndex; // access only within mutex
  int WriteIndex;

  std::atomic<int> NewData;
  std::atomic<int> HaveData;

  int Height;
  int Width;
  int UVHeight;
  int UVWidth;

private:
  vtkOpenGLMovieSphere(const vtkOpenGLMovieSphere&) = delete;
  void operator=(const vtkOpenGLMovieSphere&) = delete;
};

#endif
