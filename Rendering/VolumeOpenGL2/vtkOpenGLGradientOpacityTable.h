/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLGradientOpacityTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkOpenGLGradientOpacityTable_h_
#define __vtkOpenGLGradientOpacityTable_h_

#include <vtkPiecewiseFunction.h>
#include <vtkTextureObject.h>
#include <vtkVolumeMapper.h>

#include <vtk_glew.h>

//----------------------------------------------------------------------------
class vtkOpenGLGradientOpacityTable
{
public:
  //--------------------------------------------------------------------------
  vtkOpenGLGradientOpacityTable(int width = 1024)
    {
//      this->TextureId = 0;
      this->TextureObject = 0;
      this->TextureWidth = width;
      this->LastSampleDistance = 1.0;
      this->Table = 0;
//      this->Loaded = false;
//      this->LastLinearInterpolation = false;
      this->LastRange[0] = this->LastRange[1] = 0.0;
    }

  //--------------------------------------------------------------------------
  ~vtkOpenGLGradientOpacityTable()
    {
//      if (this->TextureId != 0)
//        {
//        glDeleteTextures(1, &this->TextureId);
//        this->TextureId=0;
//        }

      if (!this->TextureObject)
        {
        this->TextureObject->UnRegister(0);
        this->TextureObject = 0;
        }

      if (this->Table!=0)
        {
        delete[] this->Table;
        this->Table=0;
        }
    }


//  // Check if opacity transfer function texture is loaded.
//  //--------------------------------------------------------------------------
//  bool IsLoaded()
//    {
//    return this->Loaded;
//    }

  // Bind texture.
  //--------------------------------------------------------------------------
  void Bind()
    {
//    // Activate texture 5
//    glActiveTexture(GL_TEXTURE5);
//    glBindTexture(GL_TEXTURE_1D, this->TextureId);
    if (!this->TextureObject)
      {
      return;
      }
    this->TextureObject->Bind();
    }

  // Update opacity tranfer function texture.
  //--------------------------------------------------------------------------
  void Update(vtkPiecewiseFunction* gradientOpacity,
              double sampleDistance,
              double range[2],
              double vtkNotUsed(unitDistance),
              int filterValue,
              vtkOpenGLRenderWindow* renWin)
    {
//    // Activate texture 5
//    glActiveTexture(GL_TEXTURE5);

    bool needUpdate=false;
//    if(this->TextureId == 0)
//      {
//      glGenTextures(1,&this->TextureId);
//      needUpdate = true;
//      }

    if (!this->TextureObject)
      {
      this->TextureObject = vtkTextureObject::New();
      }

    this->TextureObject->SetContext(renWin);

    if (this->LastRange[0] != range[0] ||
        this->LastRange[1] != range[1])
      {
      this->LastRange[0] = range[0];
      this->LastRange[1] = range[1];
      needUpdate = true;
      }

//    glBindTexture(GL_TEXTURE_1D, this->TextureId);
//    if(needUpdate)
//      {
//      glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S,
//                      GL_CLAMP_TO_EDGE);
//      }

    if(gradientOpacity->GetMTime() > this->BuildTime ||
       this->TextureObject->GetMTime() > this->BuildTime ||
       this->LastSampleDistance != sampleDistance ||
       needUpdate)
      {
//      this->Loaded = false;
      if(this->Table == 0)
        {
        this->Table = new float[this->TextureWidth];
        }

      gradientOpacity->GetTable(0,
                              (this->LastRange[1] - this->LastRange[0]) * 0.25,
                              this->TextureWidth, this->Table);

      this->TextureObject->CreateAlphaFromRaw(this->TextureWidth,
                                              vtkTextureObject::alpha16,
                                              VTK_FLOAT,
                                              this->Table);

      this->TextureObject->Activate();
      this->TextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
      this->TextureObject->SetMagnificationFilter(filterValue);
      this->TextureObject->SetMinificationFilter(filterValue);
//      glTexImage1D(GL_TEXTURE_1D, 0, GL_ALPHA16, this->TextureWidth,
//                   this->TextureHeight, GL_ALPHA, GL_FLOAT, this->Table);
//      this->Loaded = true;
      this->BuildTime.Modified();
      }

//    needUpdate= needUpdate ||
//      this->LastLinearInterpolation!=linearInterpolation;
//    if(needUpdate)
//      {
//      this->LastLinearInterpolation = linearInterpolation;
//      GLint value = linearInterpolation ? GL_LINEAR : GL_NEAREST;
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
//  GLuint TextureId;
  vtkTextureObject* TextureObject;
  int TextureWidth;

  double LastSampleDistance;
  vtkTimeStamp BuildTime;
  float* Table;
//  bool Loaded;
//  bool LastLinearInterpolation;
  double LastRange[2];
private:
  vtkOpenGLGradientOpacityTable(const vtkOpenGLGradientOpacityTable&);
  vtkOpenGLGradientOpacityTable& operator=(const vtkOpenGLGradientOpacityTable&);
};

//-----------------------------------------------------------------------------
class vtkOpenGLGradientOpacityTables
{
public:
  //--------------------------------------------------------------------------
  vtkOpenGLGradientOpacityTables(unsigned int numberOfTables)
    {
    this->Tables = new vtkOpenGLGradientOpacityTable[numberOfTables];
    this->NumberOfTables = numberOfTables;
    }

  //--------------------------------------------------------------------------
  ~vtkOpenGLGradientOpacityTables()
    {
    delete [] this->Tables;
    }

  // Get opacity table at a given index.
  //--------------------------------------------------------------------------
  vtkOpenGLGradientOpacityTable* GetTable(unsigned int i)
    {
    return &this->Tables[i];
    }

  // Get number of tables.
  //--------------------------------------------------------------------------
  unsigned int GetNumberOfTables()
    {
    return this->NumberOfTables;
    }

private:
  unsigned int NumberOfTables;
  vtkOpenGLGradientOpacityTable* Tables;

  // vtkOpenGLGradientOpacityTables (Not implemented)
  vtkOpenGLGradientOpacityTables();

  // vtkOpenGLGradientOpacityTables (Not implemented)
  vtkOpenGLGradientOpacityTables(const vtkOpenGLGradientOpacityTables &other);

  // operator = (Not implemented)
  vtkOpenGLGradientOpacityTables &operator=(const vtkOpenGLGradientOpacityTables &other);
};

#endif // __vtkOpenGLGradientOpacityTable_h_
// VTK-HeaderTest-Exclude: vtkOpenGLGradientOpacityTable.h
