/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRGBTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkOpenGLRGBTable_h_
#define __vtkOpenGLRGBTable_h_

#include <vtkColorTransferFunction.h>
#include <vtkTextureObject.h>
#include <vtk_glew.h>

//----------------------------------------------------------------------------
class vtkOpenGLRGBTable
{
public:
  //--------------------------------------------------------------------------
  vtkOpenGLRGBTable()
    {
//    this->Loaded = false;
//    this->LastLinearInterpolation = false;
    this->TextureWidth = 1024;
    this->NumberOfColorComponents = 3;
    this->TextureObject = 0;
//    this->TextureId = 0;
    this->LastRange[0] = this->LastRange[1] = 0;
    this->Table = 0;
    }

  //--------------------------------------------------------------------------
  ~vtkOpenGLRGBTable()
    {
    if (this->TextureObject != 0)
      {
      this->TextureObject->UnRegister(0);
      this->TextureObject = 0;
      }
//    if(this->TextureId!=0)
//      {
//      glDeleteTextures(1,&this->TextureId);
//      this->TextureId=0;
//      }
    if(this->Table!=0)
      {
      delete[] this->Table;
      this->Table=0;
      }
    }

//  // Check if color transfer function texture is loaded.
//  //--------------------------------------------------------------------------
//  bool IsLoaded()
//    {
//    return this->Loaded;
//    }

  // Bind texture.
  //--------------------------------------------------------------------------
//  void Bind(int textureUnit = 1)
  void Bind(void)
    {
    if (!this->TextureObject)
      {
      return;
      }
    this->TextureObject->Activate();
//    // Activate texture 1
//    glActiveTexture(GL_TEXTURE0 + textureUnit);
//    glBindTexture(GL_TEXTURE_1D, this->TextureId);
    }

  // Update color transfer function texture.
  //--------------------------------------------------------------------------
  void Update(vtkColorTransferFunction* scalarRGB,
              double range[2],
              int filterValue,
              vtkOpenGLRenderWindow* renWin)
    {
//    // Activate texture 1
//    glActiveTexture(GL_TEXTURE0 + textureUnit);

    bool needUpdate = false;

    if (!this->TextureObject)
      {
      this->TextureObject = vtkTextureObject::New();
      }
//    if(this->TextureId == 0)
//      {
//      glGenTextures(1, &this->TextureId);
//      needUpdate = true;
//      }

    this->TextureObject->SetContext(renWin);

    if (range[0] != this->LastRange[0] || range[1] != this->LastRange[1])
      {
      this->LastRange[0] = range[0];
      this->LastRange[1] = range[1];
      needUpdate=true;
      }

//    glBindTexture(GL_TEXTURE_1D, this->TextureId);

//    if(needUpdate)
//      {
//      glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S,
//                      GL_CLAMP_TO_EDGE);
//      }

    if (scalarRGB->GetMTime() > this->BuildTime ||
        this->TextureObject->GetMTime() > this->BuildTime ||
        needUpdate)
      {
//    if(scalarRGB->GetMTime() > this->BuildTime || needUpdate || !this->Loaded)
//      {
//      this->Loaded = false;

      // Create table if not created already
      if(this->Table==0)
        {
        this->Table = new float[this->TextureWidth *
          this->NumberOfColorComponents];
        }

      scalarRGB->GetTable(this->LastRange[0], this->LastRange[1],
                          this->TextureWidth, this->Table);
      this->TextureObject->Create1DFromRaw(this->TextureWidth,
                                           this->NumberOfColorComponents,
                                           VTK_FLOAT,
                                           this->Table);
      this->TextureObject->Activate();
      this->TextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
      this->TextureObject->SetMagnificationFilter(filterValue);
      this->TextureObject->SetMinificationFilter(filterValue);

//      glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB16, this->TextureWidth, 0,
//                   GL_RGB, GL_FLOAT, this->Table);

//      this->Loaded = true;
      this->BuildTime.Modified();
      }

//    needUpdate = needUpdate ||
//      this->LastLinearInterpolation!=linearInterpolation;
//    if (needUpdate)
//      {
//      this->LastLinearInterpolation = linearInterpolation;
//      GLint value;
//      if (linearInterpolation)
//        {
//        value = GL_LINEAR;
//        }
//      else
//        {
//        value = GL_NEAREST;
//        }
//      glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, value);
//      glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, value);
//      }
//
//    glActiveTexture(GL_TEXTURE0);
    }

  // Get the texture unit
  //--------------------------------------------------------------------------
  int GetTextureUnit(void)
    {
    if (!this->TextureObject)
      {
      return -1;
      }
    return this->TextureObject->GetTextureUnit();
    }

protected:

//  bool Loaded;
//  bool LastLinearInterpolation;

  int TextureWidth;
  int NumberOfColorComponents;

//  GLuint TextureId;
  vtkTextureObject* TextureObject;

  double LastRange[2];
  float* Table;
  vtkTimeStamp BuildTime;
};

#endif // __vtkOpenGLRGBTable_h_
// VTK-HeaderTest-Exclude: vtkOpenGLRGBTable.h
