/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeShearWarpMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLVolumeShearWarpMapper.h"
#include "vtkMatrix4x4.h"
#include "vtkVolume.h"
#ifndef VTK_IMPLEMENT_MESA_CXX
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#endif
#include "vtkObjectFactory.h"

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"

#ifndef VTK_IMPLEMENT_MESA_CXX
vtkCxxRevisionMacro(vtkOpenGLVolumeShearWarpMapper, "1.4");
vtkStandardNewMacro(vtkOpenGLVolumeShearWarpMapper);
#endif

#ifndef VTK_IMPLEMENT_MESA_CXX
/*
//------------------------------------------------------------------------------
vtkOpenGLVolumeShearWarpMapper* vtkOpenGLVolumeShearWarpMapper::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkOpenGLVolumeShearWarpMapper");
  if(ret)
    {
    return (vtkOpenGLVolumeShearWarpMapper*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkOpenGLVolumeShearWarpMapper;
}
*/
#endif



vtkOpenGLVolumeShearWarpMapper::vtkOpenGLVolumeShearWarpMapper()
{
}

vtkOpenGLVolumeShearWarpMapper::~vtkOpenGLVolumeShearWarpMapper()
{
}

//#define SEPP
#ifdef SEPP
void vtkOpenGLVolumeShearWarpMapper::RenderTexture(vtkRenderer *ren, vtkVolume *vol)
{

  if (this->vtkVolumeShearWarpMapper::IntermixIntersectingGeometry)
    glDisable(GL_DEPTH_TEST);

  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();

  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
//  glLoadIdentity();

  float warp[16];
  int row, col, i=0;

  for (col=0; col<4; ++col)
    for (row=0; row<4; ++row)
      warp[i++] = this->WarpMatrix->Element[row][col];

  glLoadMatrixf(warp);


  glDisable( GL_LIGHTING );
  glEnable( GL_TEXTURE_2D );

  GLuint tempIndex;
  glGenTextures(1, &tempIndex);
  glBindTexture(GL_TEXTURE_2D, tempIndex);

  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, this->ImageWidth, this->ImageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, this->ImageData );
  glColor3f( 1.0, 1.0, 1.0 );

  glDepthMask(0);
  const float ZPOS = 0.0f;        // texture z position. TODO: make zPos changeable and adjust warp matrix accordingly

  float scaleFactor = 1.0;
  
//  if (this->ReverseOrder)
    scaleFactor = 1.0f / (1.0f - (this->CountK-1) * this->Scale);

    
  float px = (float) this->vtkVolumeShearWarpMapper::IntermediateWidth / (float) this->vtkVolumeShearWarpMapper::ImageWidth;
  float py = (float) this->vtkVolumeShearWarpMapper::IntermediateHeight / (float) this->vtkVolumeShearWarpMapper::ImageHeight;

  float center[4] = {vol->GetCenter()[0],vol->GetCenter()[1],vol->GetCenter()[2],1.0};
  float ts[4];
  float ws[4];

  this->ShearMatrix->MultiplyPoint(center,ts);
  ts[0] /= ts[3];
  ts[1] /= ts[3];
  ts[2] /= ts[3];
  ts[3] /= ts[3];

  vtkMatrix4x4 *view = vtkMatrix4x4::New();
  vtkMatrix4x4::Multiply4x4(this->WarpMatrix,this->ViewportMatrix,view);
  view->MultiplyPoint(ts,ws);
  view->Delete();
  ws[0] /= ws[3];
  ws[1] /= ws[3];
  ws[2] /= ws[3];
  ws[3] /= ws[3];
  /*
  b[0] /= b[3];
  b[1] /= b[3];
  b[2] /= b[3];
  b[3] /= b[3];
  */
  float position[4][2];

  cout << "ts: " << ts[0] << " ... " << ts[1] << "\n";
  cout << "ws: " << ws[0] << " ... " << ws[1] << "\n";
  
  position[0][0] = 0;//-b[0];
  position[0][1] = 0;//-b[1];
  
  position[1][0] = position[0][0] + this->ImageWidth;
  position[1][1] = position[0][1];

  position[2][0] = position[0][0] + this->ImageWidth;
  position[2][1] = position[0][1] + this->ImageHeight;

  position[3][0] = position[0][0];           
  position[3][1] = position[0][1] + this->ImageHeight;
  
  glBegin(GL_QUADS);

  glTexCoord2f(0.0f, 0.0f);
  glVertex2fv(position[0]);    // bottom left
//      glVertex3f(0.0f,0.0f,0.0f);

  glTexCoord2f(1.0, 0.0f);
  glVertex2fv(position[1]);    // bottom left
//      glVertex3f(this->IntermediateWidth,0.0f,0.0f);
      
  glTexCoord2f(1.0, 1.0);
  glVertex2fv(position[2]);    // bottom left
//      glVertex3f(this->IntermediateWidth,this->IntermediateHeight,0.0f);
      
  glTexCoord2f(0.0f, 1.0);
  glVertex2fv(position[3]);    // bottom left
//      glVertex3f(0.0f,this->IntermediateHeight,0.0f);
      
  glEnd();

//  glDrawPixels(this->ImageWidth,this->ImageHeight,GL_RGBA,GL_UNSIGNED_BYTE,this->ImageData);

  if (this->vtkVolumeShearWarpMapper::IntermixIntersectingGeometry == 1)
  {
    if (this->IntermediateZBuffer)
    {
    //  glDrawPixels(maxx - minx + 1,maxy - miny + 1,GL_LUMINANCE,GL_FLOAT,this->ZBuffer);
      glDrawPixels(this->IntermediateWidth,this->IntermediateHeight,GL_LUMINANCE,GL_FLOAT,this->IntermediateZBuffer);
//      glDrawPixels(this->ImageWidth,this->ImageHeight,GL_RGBA,GL_UNSIGNED_BYTE,this->ImageData);
      delete [] this->ZBuffer;
      delete [] this->IntermediateZBuffer;
    }
  }

  glDepthMask(1);

  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  glMatrixMode( GL_PROJECTION );
  glPopMatrix();

  glDisable( GL_TEXTURE_2D );
  glEnable( GL_LIGHTING );

  if (this->vtkVolumeShearWarpMapper::IntermixIntersectingGeometry)
    glEnable(GL_DEPTH_TEST);

}
#endif


#define FRANZ
#ifdef FRANZ
void vtkOpenGLVolumeShearWarpMapper::RenderTexture(vtkRenderer *ren, vtkVolume *vol)
{
  float position[4][4];
  float translation[4];

  float px,py,sx,sy;
/*  float w00,w01,w10,w11;
  float w03,w30,w31,w13,w33;
  float pc;
  float depthVal;
  
  ren->SetWorldPoint( vol->GetCenter()[0],
                      vol->GetCenter()[1],
                      vol->GetCenter()[2],
                      1.0 );
  depthVal = ren->GetViewPoint()[2];

  int *renWinSize   =  ren->GetRenderWindow()->GetSize();
  float *viewport   =  ren->GetViewport();

  // The coefficients of the 2D warp matrix
  w00 = WarpMatrix->Element[0][0];
  w01 = WarpMatrix->Element[0][1];
  w10 = WarpMatrix->Element[1][0];
  w11 = WarpMatrix->Element[1][1];

  w03 = 0.0f;//WarpMatrix->Element[0][3];
  w13 = 0.0f;//WarpMatrix->Element[1][3];
  w30 = WarpMatrix->Element[3][0];
  w31 = WarpMatrix->Element[3][1];
  w33 = WarpMatrix->Element[3][3];  
*/
  float scaleFactor = 1.0f / (1.0f - (this->CountK-1) * this->Scale);

  px = (float) this->vtkVolumeShearWarpMapper::IntermediateWidth / (float) this->vtkVolumeShearWarpMapper::ImageWidth;
  py = (float) this->vtkVolumeShearWarpMapper::IntermediateHeight / (float) this->vtkVolumeShearWarpMapper::ImageHeight;
/*
  // Warp the edges of the polygon
  pc = 1.0f;//0.0f * w30 + 0.0f * w31 + w33;
  position[0][0] = (0.0f * w00 + 0.0f * w01 + w03) / pc;
  position[0][1] = (0.0f * w10 + 0.0f * w11 + w13) / pc;

  pc = 1.0f;//px * w30 + 0.0f * w31 + w33;
  position[1][0] = (px * w00 + 0.0f * w01 + w03) / pc;
  position[1][1] = (px * w10 + 0.0f * w11 + w13) / pc;

  pc = 1.0f;//px * w30 + py * w31 + w33;
  position[2][0] = (px * w00 + py * w01 + w03) / pc;
  position[2][1] = (px * w10 + py * w11 + w13) / pc;

  pc = 1.0f;//0.0f * w30 + py * w31 + w33;
  position[3][0] = (0.0f * w00 + py * w01 + w03) / pc;
  position[3][1] = (0.0f * w10 + py * w11 + w13) / pc;

  // Compute the translation of the polygon
  pc = 1.0f;//px * 0.5f * w30 + py * 0.5f * w31 + w33;
  translation[0] = (px * 0.5f * w00 + py * 0.5f * w01 + w03) / pc;
  translation[1] = (px * 0.5f * w10 + py * 0.5f * w11 + w13) / pc;
  */
  
  position[0][0] = 0.0f;
  position[0][1] = 0.0f;
  position[0][2] = 0.0f;
  position[0][3] = 1.0f;
  
  position[1][0] = px;
  position[1][1] = 0.0f;
  position[1][2] = 0.0f;
  position[1][3] = 1.0f;
  
  position[2][0] = px;
  position[2][1] = py;
  position[2][2] = 0.0f;
  position[2][3] = 1.0f;

  position[3][0] = 0.0f;
  position[3][1] = py;
  position[3][2] = 0.0f;
  position[3][3] = 1.0f;

  translation[0] = 0.5f*px;
  translation[1] = 0.5f*py;
  translation[2] = 0.0;
  translation[3] = 1.0;

  this->WarpMatrix->MultiplyPoint(position[0],position[0]);
  this->WarpMatrix->MultiplyPoint(position[1],position[1]);
  this->WarpMatrix->MultiplyPoint(position[2],position[2]);
  this->WarpMatrix->MultiplyPoint(position[3],position[3]);
  this->WarpMatrix->MultiplyPoint(translation,translation);

  /*
  position[0][0] /= position[0][3];
  position[0][1] /= position[0][3];
  position[0][2] /= position[0][3];
  position[0][3] /= position[0][3];

  position[1][0] /= position[1][3];
  position[1][1] /= position[1][3];
  position[1][2] /= position[1][3];
  position[1][3] /= position[1][3];

  position[2][0] /= position[2][3];
  position[2][1] /= position[2][3];
  position[2][2] /= position[2][3];
  position[2][3] /= position[2][3];
  
  position[3][0] /= position[3][3];
  position[3][1] /= position[3][3];
  position[3][2] /= position[3][3];
  position[3][3] /= position[3][3];

  translation[0] /= translation[3];
  translation[1] /= translation[3];
  translation[2] /= translation[3];
  translation[3] /= translation[3];
  */

  
  sx =  (float) this->vtkVolumeShearWarpMapper::ImageSampleDistance *
        (float) this->vtkVolumeShearWarpMapper::ImageWidth /
        (float) this->vtkVolumeShearWarpMapper::ImageViewportSize[0] * 2.0f;
  sy =  (float) this->vtkVolumeShearWarpMapper::ImageSampleDistance *
        (float) this->vtkVolumeShearWarpMapper::ImageHeight /
        (float) this->vtkVolumeShearWarpMapper::ImageViewportSize[1] * 2.0f;

  float a[4] = {vol->GetCenter()[0],vol->GetCenter()[1],vol->GetCenter()[2],1.0f};
  float b[4];
  this->PerspectiveMatrix->MultiplyPoint(a,b);

  /*
  b[0] /= b[3];
  b[1] /= b[3];
  b[2] /= b[3];
  b[3] /= b[3];
  */

  if (this->vtkVolumeShearWarpMapper::IntermixIntersectingGeometry)
    glDisable(GL_DEPTH_TEST);
  
  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();

  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();

  glDisable( GL_LIGHTING );
  glEnable( GL_TEXTURE_2D );

  GLuint tempIndex;
  glGenTextures(1, &tempIndex);
  glBindTexture(GL_TEXTURE_2D, tempIndex);

  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, this->ImageWidth, this->ImageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, this->ImageData );
  glColor3f( 1.0, 1.0, 1.0 );                                                                                                   

  glDepthMask(0);

  // Draw the polygin
  glBegin(GL_QUADS);
  
  glTexCoord2f(0.0f,0.0f);
  glVertex4f(b[0] + sx*(position[0][0]-translation[0]),b[1] + sy*(position[0][1]-translation[1]),b[2] + (position[0][2] - translation[2]),b[3] + (position[0][3] - translation[3]));
  
  glTexCoord2f(px,0.0f);
//  glTexCoord2f(1.0f,0.0f);
  glVertex4f(b[0] + sx*(position[1][0]-translation[0]),b[1] + sy*(position[1][1]-translation[1]),b[2] + (position[1][2] - translation[2]),b[3] + (position[1][3] - translation[3]));
  
  glTexCoord2f(px,py);
//  glTexCoord2f(1.0f,1.0f);
  glVertex4f(b[0] + sx*(position[2][0]-translation[0]),b[1] + sy*(position[2][1]-translation[1]),b[2] + (position[2][2] - translation[2]),b[3] + (position[2][3] - translation[3]));

  glTexCoord2f(0.0f,py);
//  glTexCoord2f(0.0f,1.0f);
  glVertex4f(b[0] + sx*(position[3][0]-translation[0]),b[1] + sy*(position[3][1]-translation[1]),b[2] + (position[3][2] - translation[2]),b[3] + (position[3][3] - translation[3]));

  glEnd();

  if (this->Debug == 1)
    glDrawPixels(this->ImageWidth,this->ImageHeight,GL_RGBA,GL_UNSIGNED_BYTE,this->ImageData);
  
  if (this->vtkVolumeShearWarpMapper::IntermixIntersectingGeometry == 1)
  {
    if (this->IntermediateZBuffer)
    {
      if (this->Debug == 2)      
        glDrawPixels(this->ZBufferSize[0],this->ZBufferSize[1],GL_LUMINANCE,GL_FLOAT,this->ZBuffer);
      else if (this->Debug == 3)
        glDrawPixels(this->ImageSampleDistance*this->IntermediateWidth,this->ImageSampleDistance*this->IntermediateHeight,GL_LUMINANCE,GL_FLOAT,this->IntermediateZBuffer);

      delete [] this->ZBuffer;
      delete [] this->IntermediateZBuffer;
    }
  }

  glDepthMask(1);

  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  glMatrixMode( GL_PROJECTION );
  glPopMatrix(); 

  glDisable( GL_TEXTURE_2D );
  glEnable( GL_LIGHTING );

  if (this->vtkVolumeShearWarpMapper::IntermixIntersectingGeometry)
    glEnable(GL_DEPTH_TEST);
  
}
#endif

// Print the vtkOpenGLVolumeShearWarpMapper
void vtkOpenGLVolumeShearWarpMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

