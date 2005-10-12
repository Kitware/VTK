/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRenderWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"
#include "vtkUnsignedCharArray.h"

#ifndef VTK_IMPLEMENT_MESA_CXX
vtkCxxRevisionMacro(vtkOpenGLRenderWindow, "1.69");
#endif

#define MAX_LIGHTS 8

// Initialize static member that controls global maximum number of multisamples
static int vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples = 8;

void vtkOpenGLRenderWindow::SetGlobalMaximumNumberOfMultiSamples(int val)
{
  if (val == vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples) return;
  vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples = val;
}

int vtkOpenGLRenderWindow::GetGlobalMaximumNumberOfMultiSamples()
{
  return vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples;
}

vtkOpenGLRenderWindow::vtkOpenGLRenderWindow()
{
  this->MultiSamples = vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples;
  this->TextureResourceIds = vtkIdList::New();
  if ( this->WindowName ) 
    delete [] this->WindowName;
  this->WindowName = new char[strlen("Visualization Toolkit - OpenGL")+1];
    strcpy( this->WindowName, "Visualization Toolkit - OpenGL" );
}

// free up memory & close the window
vtkOpenGLRenderWindow::~vtkOpenGLRenderWindow()
{
  this->TextureResourceIds->Delete();
}

// Update system if needed due to stereo rendering.
void vtkOpenGLRenderWindow::StereoUpdate(void)
{
  // if stereo is on and it wasn't before
  if (this->StereoRender && (!this->StereoStatus))
    {
    switch (this->StereoType) 
      {
      case VTK_STEREO_CRYSTAL_EYES:
        // not clear this is supposed to be empty,
        // but it has been that way forever.
        break;
      case VTK_STEREO_RED_BLUE:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_ANAGLYPH:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_DRESDEN:
        this->StereoStatus = 1;
        break;      
      case VTK_STEREO_INTERLACED:
        this->StereoStatus = 1;
      }
    }
  else if ((!this->StereoRender) && this->StereoStatus)
    {
    switch (this->StereoType) 
      {
      case VTK_STEREO_CRYSTAL_EYES:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_RED_BLUE:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_ANAGLYPH:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_DRESDEN:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_INTERLACED:
        this->StereoStatus = 0;
        break;
      }
    }
}


void vtkOpenGLRenderWindow::OpenGLInit()
{
  glMatrixMode( GL_MODELVIEW );
  glDepthFunc( GL_LEQUAL );
  glEnable( GL_DEPTH_TEST );
  glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

  // initialize blending for transparency
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  glEnable(GL_BLEND);

  if (this->PointSmoothing)
    {
    glEnable(GL_POINT_SMOOTH);
    }
  else
    {
    glDisable(GL_POINT_SMOOTH);
    }

  if (this->LineSmoothing)
    {
    glEnable(GL_LINE_SMOOTH);
    }
  else
    {
    glDisable(GL_LINE_SMOOTH);
    }

  if (this->PolygonSmoothing)
    {
    glEnable(GL_POLYGON_SMOOTH);
    }
  else
    {
    glDisable(GL_POLYGON_SMOOTH);
    }

  glEnable(GL_NORMALIZE);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
}


void vtkOpenGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "MultiSamples: " << this->MultiSamples << "\n";
}

int vtkOpenGLRenderWindow::GetDepthBufferSize()
{
  GLint size;

  if ( this->Mapped )
    {
    this->MakeCurrent();
    size = 0;
    glGetIntegerv( GL_DEPTH_BITS, &size );
    return (int) size;
    }
  else
    {
    vtkDebugMacro(<< "Window is not mapped yet!" );
    return 24;
    }
}

unsigned char* vtkOpenGLRenderWindow::GetPixelData(int x1, int y1, 
                                                   int x2, int y2,
                                                   int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }

  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }

  unsigned char *data = 
           new unsigned char[(x_hi - x_low + 1)*(y_hi - y_low + 1)*3];
  this->GetPixelData(x1, y1, x2, y2, front, data);
  return data;
}

int vtkOpenGLRenderWindow::GetPixelData(int x1, int y1,
                                        int x2, int y2,
                                        int front,
                                        vtkUnsignedCharArray* data)
{
  int     y_low, y_hi;
  int     x_low, x_hi;

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }

  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }

  int width  = abs(x_hi - x_low) + 1;
  int height = abs(y_hi - y_low) + 1;
  int size = 3*width*height;

  if ( data->GetMaxId()+1 != size)
    {
    vtkDebugMacro("Resizing array.");
    data->SetNumberOfComponents(3);
    data->SetNumberOfValues(size);
    }
  return this->GetPixelData(x1, y1, x2, y2, front, data->GetPointer(0));
  
}

int vtkOpenGLRenderWindow::GetPixelData(int x1, int y1,
                                        int x2, int y2,
                                        int front, unsigned char* data)
{
  int     y_low, y_hi;
  int     x_low, x_hi;

  // set the current window 
  this->MakeCurrent();

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }

  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }

  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR);

  if (front)
    {
    glReadBuffer(GL_FRONT);
    }
  else
    {
    glReadBuffer(GL_BACK);
    }

  glDisable( GL_SCISSOR_TEST );

#if defined(sparc) && !defined(GL_VERSION_1_2)
  // We need to read the image data one row at a time and convert it
  // from RGBA to RGB to get around a bug in Sun OpenGL 1.1
  long    xloop, yloop;
  unsigned char *buffer;
  unsigned char *p_data = NULL;
  
  buffer = new unsigned char [4*(x_hi - x_low + 1)];
  p_data = data;
  for (yloop = y_low; yloop <= y_hi; yloop++)
    {
    // read in a row of pixels
    glReadPixels(x_low,yloop,(x_hi-x_low+1),1,
                 GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    for (xloop = 0; xloop <= x_hi-x_low; xloop++)
      {
      *p_data = buffer[xloop*4]; p_data++;
      *p_data = buffer[xloop*4+1]; p_data++;
      *p_data = buffer[xloop*4+2]; p_data++;
      }
    }
  
  delete [] buffer;  
#else
  // If the Sun bug is ever fixed, then we could use the following
  // technique which provides a vast speed improvement on the SGI
  
  // Turn of texturing in case it is on - some drivers have a problem
  // getting / setting pixels with texturing enabled.
  glDisable( GL_TEXTURE_2D );

  // Calling pack alignment ensures that we can grab the any size window
  glPixelStorei( GL_PACK_ALIGNMENT, 1 );
  glReadPixels(x_low, y_low, x_hi-x_low+1, y_hi-y_low+1, GL_RGB,
               GL_UNSIGNED_BYTE, data);
#endif

  if (glGetError() != GL_NO_ERROR)
    {
    return VTK_ERROR;
    }
  else
    {
    return VTK_OK;
    }
  
}

int vtkOpenGLRenderWindow::SetPixelData(int x1, int y1, int x2, int y2,
                                        vtkUnsignedCharArray *data, int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;

  if (y1 < y2)
    {

    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }
  
  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }

  int width  = abs(x_hi - x_low) + 1;
  int height = abs(y_hi - y_low) + 1;
  int size = 3*width*height;

  if ( data->GetMaxId()+1 != size)
    {
    vtkErrorMacro("Buffer is of wrong size.");
    return VTK_ERROR;
    }
  return this->SetPixelData(x1, y1, x2, y2, data->GetPointer(0), front);

}

int vtkOpenGLRenderWindow::SetPixelData(int x1, int y1, int x2, int y2,
                                        unsigned char *data, int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;

  // set the current window 
  this->MakeCurrent();

  // Error checking
  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR);

  if (front)
    {
    glDrawBuffer(GL_FRONT);
    }
  else
    {
    glDrawBuffer(GL_BACK);
    }

  if (y1 < y2)
    {

    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }
  
  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }

  glDisable( GL_SCISSOR_TEST );
  glViewport(0, 0, this->Size[0], this->Size[1]);

#if defined(sparc) && !defined(GL_VERSION_1_2)
  // We need to read the image data one row at a time and convert it
  // from RGBA to RGB to get around a bug in Sun OpenGL 1.1
  long    xloop, yloop;
  unsigned char *buffer;
  unsigned char *p_data = NULL;
  
  buffer = new unsigned char [4*(x_hi - x_low + 1)];

  // now write the binary info one row at a time
  glDisable(GL_BLEND);
  p_data = data;
  for (yloop = y_low; yloop <= y_hi; yloop++)
    {
    for (xloop = 0; xloop <= x_hi - x_low; xloop++)
      {
      buffer[xloop*4] = *p_data; p_data++;
      buffer[xloop*4+1] = *p_data; p_data++;
      buffer[xloop*4+2] = *p_data; p_data++;
      buffer[xloop*4+3] = 0xff;
      }
    /* write out a row of pixels */
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    glRasterPos3f( (2.0 * (GLfloat)(x_low) / this->Size[0] - 1),
                   (2.0 * (GLfloat)(yloop) / this->Size[1] - 1),
                   -1.0 );
    glMatrixMode( GL_PROJECTION );
    glPopMatrix();
    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();

    glDrawPixels((x_hi-x_low+1),1, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    }
  
  // This seems to be necessary for the image to show up
  glFlush();  
  
  glEnable(GL_BLEND);
#else
  // If the Sun bug is ever fixed, then we could use the following
  // technique which provides a vast speed improvement on the SGI
  
  // Turn of texturing in case it is on - some drivers have a problem
  // getting / setting pixels with texturing enabled.
  glDisable( GL_TEXTURE_2D );

  // now write the binary info
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();
  glRasterPos3f( (2.0 * (GLfloat)(x_low) / this->Size[0] - 1), 
                 (2.0 * (GLfloat)(y_low) / this->Size[1] - 1),
                 -1.0 );
  glMatrixMode( GL_PROJECTION );
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);
  glDisable(GL_BLEND);
  glDrawPixels((x_hi-x_low+1), (y_hi - y_low + 1),
               GL_RGB, GL_UNSIGNED_BYTE, data);
  glEnable(GL_BLEND);
  
  // This seems to be necessary for the image to show up
  glFlush();  
#endif
  
  if (glGetError() != GL_NO_ERROR)
    {
    return VTK_ERROR;
    }
  else
    {
    return VTK_OK;
    }
}

float* vtkOpenGLRenderWindow::GetRGBAPixelData(int x1, int y1, int x2, int y2,
                                               int front)
{

  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }

  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }

  width  = abs(x_hi - x_low) + 1;
  height = abs(y_hi - y_low) + 1;

  float *data = new float[ (width*height*4) ];
  this->GetRGBAPixelData(x1, y1, x2, y2, front, data);

  return data;

}

int vtkOpenGLRenderWindow::GetRGBAPixelData(int x1, int y1, int x2, int y2,
                                            int front, vtkFloatArray* data)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }

  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }

  width  = abs(x_hi - x_low) + 1;
  height = abs(y_hi - y_low) + 1;

  int size = 4*width*height;
  
  if ( data->GetMaxId()+1 != size)
    {
    vtkDebugMacro("Resizing array.");
    data->SetNumberOfComponents(4);
    data->SetNumberOfValues(size);
    }
  return this->GetRGBAPixelData(x1, y1, x2, y2, front, data->GetPointer(0));

}

int vtkOpenGLRenderWindow::GetRGBAPixelData(int x1, int y1, int x2, int y2,
                                            int front, float* data)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  // set the current window 
  this->MakeCurrent();

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }

  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }

  // Error checking
  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR);

  if (front)
    {
    glReadBuffer(GL_FRONT);
    }
  else
    {
    glReadBuffer(GL_BACK);
    }

  width  = abs(x_hi - x_low) + 1;
  height = abs(y_hi - y_low) + 1;


  // Turn of texturing in case it is on - some drivers have a problem
  // getting / setting pixels with texturing enabled.
  glDisable( GL_TEXTURE_2D );

  glPixelStorei( GL_PACK_ALIGNMENT, 1 );
  glReadPixels( x_low, y_low, width, height, GL_RGBA, GL_FLOAT, data);

  if (glGetError() != GL_NO_ERROR)
    {
    return VTK_ERROR;
    }
  else
    {
    return VTK_OK;
    }
}

void vtkOpenGLRenderWindow::ReleaseRGBAPixelData(float *data) 
{
  delete[] data;
}

int vtkOpenGLRenderWindow::SetRGBAPixelData(int x1, int y1, int x2, int y2,
                                            vtkFloatArray *data, int front,
                                            int blend)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }
  
  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }
  
  width  = abs(x_hi-x_low) + 1;
  height = abs(y_hi-y_low) + 1;

  int size = 4*width*height;
  if ( data->GetMaxId()+1 != size )
    {
    vtkErrorMacro("Buffer is of wrong size.");
    return VTK_ERROR;
    }

  return this->SetRGBAPixelData(x1, y1, x2, y2, data->GetPointer(0), front,
                                blend);
}

int vtkOpenGLRenderWindow::SetRGBAPixelData(int x1, int y1, int x2, int y2,
                                            float *data, int front, int blend)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  // set the current window 
  this->MakeCurrent();

  // Error checking
  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR);

  if (front)
    {
    glDrawBuffer(GL_FRONT);
    }
  else
    {
    glDrawBuffer(GL_BACK);
    }

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }
  
  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }
  
  width  = abs(x_hi-x_low) + 1;
  height = abs(y_hi-y_low) + 1;

  /* write out a row of pixels */
  glDisable( GL_SCISSOR_TEST );
  glViewport(0, 0, this->Size[0], this->Size[1]);
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();
  glRasterPos3f( (2.0 * (GLfloat)(x_low) / this->Size[0] - 1), 
                 (2.0 * (GLfloat)(y_low) / this->Size[1] - 1),
                 -1.0 );
  glMatrixMode( GL_PROJECTION );
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);

  if (!blend)
    {
    glDisable(GL_BLEND);
    glDrawPixels( width, height, GL_RGBA, GL_FLOAT, data);
    glEnable(GL_BLEND);
    }
  else
    {
    glDrawPixels( width, height, GL_RGBA, GL_FLOAT, data);
    }
  
  // This seems to be necessary for the image to show up
  glFlush();  

  if (glGetError() != GL_NO_ERROR)
    {
    return VTK_ERROR;
    }
  else
    {
    return VTK_OK;
    }
}

unsigned char *vtkOpenGLRenderWindow::GetRGBACharPixelData(int x1, int y1, 
                                                           int x2, int y2, 
                                                           int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  if (y1 < y2)
    {
    y_low = y1;
    y_hi  = y2;
    }
  else
    {
    y_low = y2;
    y_hi  = y1;
    }


  if (x1 < x2)
    {
    x_low = x1;
    x_hi  = x2;
    }
  else
    {
    x_low = x2;
    x_hi  = x1;
    }

  width  = abs(x_hi - x_low) + 1;
  height = abs(y_hi - y_low) + 1;

  unsigned char *data = new unsigned char[ (width*height)*4 ];
  this->GetRGBACharPixelData(x1, y1, x2, y2, front, data);

  return data;
}

int vtkOpenGLRenderWindow::GetRGBACharPixelData(int x1, int y1,
                                                int x2, int y2,
                                                int front,
                                                vtkUnsignedCharArray* data)
{
  int     y_low, y_hi;
  int     x_low, x_hi;

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }

  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }

  int width  = abs(x_hi - x_low) + 1;
  int height = abs(y_hi - y_low) + 1;
  int size = 4*width*height;

  if ( data->GetMaxId()+1 != size)
    {
    vtkDebugMacro("Resizing array.");
    data->SetNumberOfComponents(4);
    data->SetNumberOfValues(size);
    }
  return this->GetRGBACharPixelData(x1, y1, x2, y2, front,
                                    data->GetPointer(0));
}

int vtkOpenGLRenderWindow::GetRGBACharPixelData(int x1, int y1,
                                                int x2, int y2,
                                                int front,
                                                unsigned char* data)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;


  // set the current window
  this->MakeCurrent();


  if (y1 < y2)
    {
    y_low = y1;
    y_hi  = y2;
    }
  else
    {
    y_low = y2;
    y_hi  = y1;
    }


  if (x1 < x2)
    {
    x_low = x1;
    x_hi  = x2;
    }
  else
    {
    x_low = x2;
    x_hi  = x1;
    }


  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR);

  if (front)
    {
    glReadBuffer(GL_FRONT);
    }
  else
    {
    glReadBuffer(GL_BACK);
    }

  width  = abs(x_hi - x_low) + 1;
  height = abs(y_hi - y_low) + 1;

  glDisable( GL_SCISSOR_TEST );
  
  // Turn of texturing in case it is on - some drivers have a problem
  // getting / setting pixels with texturing enabled.
  glDisable( GL_TEXTURE_2D );

  glReadPixels( x_low, y_low, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                data);

  if (glGetError() != GL_NO_ERROR)
    {
    return VTK_ERROR;
    }
  else
    {
    return VTK_OK;
    }

}


int vtkOpenGLRenderWindow::SetRGBACharPixelData(int x1,int y1,int x2,int y2,
                                                vtkUnsignedCharArray *data,
                                                int front, int blend)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }
  
  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }
  
  width  = abs(x_hi-x_low) + 1;
  height = abs(y_hi-y_low) + 1;

  int size = 4*width*height;
  if ( data->GetMaxId()+1 != size )
    {
    vtkErrorMacro("Buffer is of wrong size. It is " << data->GetMaxId()+1
                  << ", it should be: " << size);
    return VTK_ERROR;
    }

  return this->SetRGBACharPixelData(x1, y1, x2, y2, data->GetPointer(0),
                                    front, blend);
  
}

int vtkOpenGLRenderWindow::SetRGBACharPixelData(int x1, int y1, int x2,
                                                int y2, unsigned char *data,
                                                int front, int blend)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;


  // set the current window
  this->MakeCurrent();


  // Error checking
  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR);

  if (front)
    {
    glDrawBuffer(GL_FRONT);
    }
  else
    {
    glDrawBuffer(GL_BACK);
    }


  if (y1 < y2)
    {
    y_low = y1;
    y_hi  = y2;
    }
  else
    {
    y_low = y2;
    y_hi  = y1;
    }


  if (x1 < x2)
    {
    x_low = x1;
    x_hi  = x2;
    }
  else
    {
    x_low = x2;
    x_hi  = x1;
    }


  width  = abs(x_hi-x_low) + 1;
  height = abs(y_hi-y_low) + 1;


  /* write out a row of pixels */
  glViewport(0, 0, this->Size[0], this->Size[1]);
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();
  glRasterPos3f( (2.0 * (GLfloat)(x_low) / this->Size[0] - 1),
                 (2.0 * (GLfloat)(y_low) / this->Size[1] - 1),
                 -1.0 );
  glMatrixMode( GL_PROJECTION );
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();


  glDisable( GL_SCISSOR_TEST );
  
  // Turn of texturing in case it is on - some drivers have a problem
  // getting / setting pixels with texturing enabled.
  glDisable( GL_TEXTURE_2D );

  if (!blend)
    {
    glDisable(GL_BLEND);
    glDrawPixels( width, height, GL_RGBA, GL_UNSIGNED_BYTE, 
                  data);
    glEnable(GL_BLEND);
    }
  else
    {
    glDrawPixels( width, height, GL_RGBA, GL_UNSIGNED_BYTE, 
                  data);
    }

  // This seems to be necessary for the image to show up
  glFlush();  

  if (glGetError() != GL_NO_ERROR)
    {
    return VTK_ERROR;
    }
  else
    {
    return VTK_OK;
    }
}


int vtkOpenGLRenderWindow::GetZbufferData( int x1, int y1, int x2, int y2,
                                           float* z_data )
{
  int             y_low;
  int             x_low;
  int             width, height;

  // set the current window 
  this->MakeCurrent();

  if (y1 < y2)
    {
    y_low = y1; 
    }
  else
    {
    y_low = y2; 
    }

  if (x1 < x2)
    {
    x_low = x1; 
    }
  else
    {
    x_low = x2; 
    }

  width =  abs(x2 - x1)+1;
  height = abs(y2 - y1)+1;

  // Error checking
  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR);

  // Turn of texturing in case it is on - some drivers have a problem
  // getting / setting pixels with texturing enabled.
  glDisable( GL_TEXTURE_2D );
  glDisable( GL_SCISSOR_TEST );
  glPixelStorei( GL_PACK_ALIGNMENT, 1 );

  glReadPixels( x_low, y_low, 
                width, height,
                GL_DEPTH_COMPONENT, GL_FLOAT,
                z_data );

  if (glGetError() != GL_NO_ERROR)
    {
    return VTK_ERROR;
    }
  else
    {
    return VTK_OK;
    }
}

float *vtkOpenGLRenderWindow::GetZbufferData( int x1, int y1, int x2, int y2  )
{
  float           *z_data;

  int             width, height;
  width =  abs(x2 - x1)+1;
  height = abs(y2 - y1)+1;

  z_data = new float[width*height];
  this->GetZbufferData(x1, y1, x2, y2, z_data);

  return z_data;
}

int vtkOpenGLRenderWindow::GetZbufferData( int x1, int y1, int x2, int y2,
                                           vtkFloatArray *buffer )
{
  int  width, height;
  width =  abs(x2 - x1)+1;
  height = abs(y2 - y1)+1;
  int size = width*height;
  if ( buffer->GetMaxId()+1 != size)
    {
    vtkDebugMacro("Resizing array.");
    buffer->SetNumberOfComponents(1);
    buffer->SetNumberOfValues(size);
    }
  return this->GetZbufferData(x1, y1, x2, y2, buffer->GetPointer(0));
}

int vtkOpenGLRenderWindow::SetZbufferData( int x1, int y1, int x2, int y2,
                                           vtkFloatArray *buffer )
{
  int width, height;
  width =  abs(x2 - x1)+1;
  height = abs(y2 - y1)+1;
  int size = width*height;
  if ( buffer->GetMaxId()+1 != size )
    {
    vtkErrorMacro("Buffer is of wrong size.");
    return VTK_ERROR;
    }
  return this->SetZbufferData(x1, y1, x2, y2, buffer->GetPointer(0));
}

int vtkOpenGLRenderWindow::SetZbufferData( int x1, int y1, int x2, int y2,
                                           float *buffer )
{
  int             y_low;
  int             x_low;
  int             width, height;

  // set the current window 
  this->MakeCurrent();

  if (y1 < y2)
    {
    y_low = y1; 
    }
  else
    {
    y_low = y2; 
    }

  if (x1 < x2)
    {
    x_low = x1; 
    }
  else
    {
    x_low = x2; 
    }

  width =  abs(x2 - x1)+1;
  height = abs(y2 - y1)+1;

  // Error checking
  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR);

  glViewport(0, 0, this->Size[0], this->Size[1]);
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();
  glRasterPos2f( 2.0 * (GLfloat)(x_low) / this->Size[0] - 1, 
                 2.0 * (GLfloat)(y_low) / this->Size[1] - 1);
  glMatrixMode( GL_PROJECTION );
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  // Turn of texturing in case it is on - some drivers have a problem
  // getting / setting pixels with texturing enabled.
  glDisable( GL_SCISSOR_TEST );
  glDisable( GL_TEXTURE_2D );
  glPixelStorei( GL_PACK_ALIGNMENT, 1 );

  glDrawPixels( width, height, GL_DEPTH_COMPONENT, GL_FLOAT, buffer);

  // This seems to be necessary for the image to show up
  glFlush();  

  if (glGetError() != GL_NO_ERROR)
    {
    return VTK_ERROR;
    }
  else
    {
    return VTK_OK;
    }
}


void vtkOpenGLRenderWindow::RegisterTextureResource (GLuint id)
{
  this->TextureResourceIds->InsertNextId ((int) id);
}


