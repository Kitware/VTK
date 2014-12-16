/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLOpacityTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkOpenGLVolumeOpacityTable_h_
#define __vtkOpenGLVolumeOpacityTable_h_

#include <vtkPiecewiseFunction.h>
#include <vtkTextureObject.h>
#include <vtkVolumeMapper.h>

#include <vtk_glew.h>

//----------------------------------------------------------------------------
class vtkOpenGLOpacityTable
{
public:
  //--------------------------------------------------------------------------
  vtkOpenGLOpacityTable(int width = 1024)
    {
//      this->TextureId = 0;
      this->TextureObject = 0;
      this->LastBlendMode = vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND;
      this->TextureWidth = width;
      this->LastSampleDistance = 1.0;
      this->Table = 0;
//      this->Loaded = false;
//      this->LastLinearInterpolation = false;
      this->LastRange[0] = this->LastRange[1] = 0.0;
    }

  //--------------------------------------------------------------------------
  ~vtkOpenGLOpacityTable()
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
//    // Activate texture 2
//    glActiveTexture(GL_TEXTURE2);
//    glBindTexture(GL_TEXTURE_1D, this->TextureId);
    if (!this->TextureObject)
      {
      return;
      }
    this->TextureObject->Bind();
    }

  // Update opacity tranfer function texture.
  //--------------------------------------------------------------------------
  void Update(vtkPiecewiseFunction* scalarOpacity,
              int blendMode,
              double sampleDistance,
              double range[2],
              double unitDistance,
              int filterValue,
              vtkOpenGLRenderWindow* renWin)
    {
//    // Activate texture 2
//    glActiveTexture(GL_TEXTURE2);

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

//    glBindTexture(GL_TEXTURE_1D,this->TextureId);
//    if(needUpdate)
//      {
//      glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S,
//                      GL_CLAMP_TO_EDGE);
//      }

    if(scalarOpacity->GetMTime() > this->BuildTime ||
       this->TextureObject->GetMTime() > this->BuildTime ||
       (this->LastBlendMode != blendMode) ||
       (blendMode == vtkVolumeMapper::COMPOSITE_BLEND &&
        this->LastSampleDistance != sampleDistance) ||
       needUpdate)
      {
//      this->Loaded = false;
      if(this->Table == 0)
        {
        this->Table = new float[this->TextureWidth];
        }

      scalarOpacity->GetTable(this->LastRange[0],
                              this->LastRange[1],
                              this->TextureWidth,
                              this->Table);
      this->LastBlendMode = blendMode;

      // Correct the opacity array for the spacing between the planes if we
      // are using a composite blending operation
      // TODO Fix this code for sample distance in three dimensions
        if(blendMode == vtkVolumeMapper::COMPOSITE_BLEND)
          {
          float* ptr = this->Table;
          double factor = sampleDistance/unitDistance;
          int i=0;
          while(i < this->TextureWidth)
            {
            if(*ptr > 0.0001f)
              {
              *ptr = static_cast<float>(1.0-pow(1.0-static_cast<double>(*ptr),
                                        factor));
              }
            ++ptr;
            ++i;
            }
          this->LastSampleDistance = sampleDistance;
          }
        else if (blendMode==vtkVolumeMapper::ADDITIVE_BLEND)
          {
          float* ptr = this->Table;
          double factor = sampleDistance/unitDistance;
          int i = 0;
          while( i < this->TextureWidth)
            {
            if(*ptr > 0.0001f)
              {
              *ptr = static_cast<float>(static_cast<double>(*ptr)*factor);
              }
            ++ptr;
            ++i;
            }
          this->LastSampleDistance = sampleDistance;
          }

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
  vtkTextureObject * TextureObject;
  int LastBlendMode;
  int TextureWidth;

  double LastSampleDistance;
  vtkTimeStamp BuildTime;
  float *Table;
//  bool Loaded;
//  bool LastLinearInterpolation;
  double LastRange[2];
private:
  vtkOpenGLOpacityTable(const vtkOpenGLOpacityTable&);
  vtkOpenGLOpacityTable& operator=(const vtkOpenGLOpacityTable&);
};

//----------------------------------------------------------------------------
class vtkOpenGLOpacityTables
{
public:
  //--------------------------------------------------------------------------
  vtkOpenGLOpacityTables(unsigned int numberOfTables)
    {
    this->Tables = new vtkOpenGLOpacityTable[numberOfTables];
    this->NumberOfTables = numberOfTables;
    }

  //--------------------------------------------------------------------------
  ~vtkOpenGLOpacityTables()
    {
    delete [] this->Tables;
    }

  // brief Get opacity table at a given index.
  //--------------------------------------------------------------------------
  vtkOpenGLOpacityTable* GetTable(unsigned int i)
    {
    return &this->Tables[i];
    }

  // Get number of opacity tables.
  //--------------------------------------------------------------------------
  unsigned int GetNumberOfTables()
    {
    return this->NumberOfTables;
    }

private:
  unsigned int NumberOfTables;
  vtkOpenGLOpacityTable *Tables;

  // vtkOpenGLOpacityTables (Not implemented)
  vtkOpenGLOpacityTables();

  // vtkOpenGLOpacityTables (Not implemented)
  vtkOpenGLOpacityTables(const vtkOpenGLOpacityTables &other);

  // operator = (Not implemented)
  vtkOpenGLOpacityTables &operator=(const vtkOpenGLOpacityTables &other);
};

#endif // __vtkOpenGLVolumeOpacityTable_h_
// VTK-HeaderTest-Exclude: vtkOpenGLOpacityTable.h
