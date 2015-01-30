/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeOpacityTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkOpenGLVolumeOpacityTable_h_
#define vtkOpenGLVolumeOpacityTable_h_

#include <vtkPiecewiseFunction.h>
#include <vtkTextureObject.h>
#include <vtkVolumeMapper.h>

#include <vtk_glew.h>

//----------------------------------------------------------------------------
class vtkOpenGLVolumeOpacityTable
{
public:
  //--------------------------------------------------------------------------
  vtkOpenGLVolumeOpacityTable(int width = 1024)
    {
      this->TextureObject = 0;
      this->LastBlendMode = vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND;
      this->TextureWidth = width;
      this->LastSampleDistance = 1.0;
      this->Table = 0;
      this->LastInterpolation = -1;
      this->LastRange[0] = this->LastRange[1] = 0.0;
    }

  //--------------------------------------------------------------------------
  ~vtkOpenGLVolumeOpacityTable()
    {
      if (this->TextureObject)
        {
        this->TextureObject->Delete();
        this->TextureObject = 0;
        }

      if (this->Table)
        {
        delete[] this->Table;
        this->Table=0;
        }
    }

  // Bind texture.
  //--------------------------------------------------------------------------
  void Bind()
    {
    if (!this->TextureObject)
      {
      return;
      }
    this->TextureObject->Activate();
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
    bool needUpdate = false;
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

    if(scalarOpacity->GetMTime() > this->BuildTime ||
       this->TextureObject->GetMTime() > this->BuildTime ||
       (this->LastBlendMode != blendMode) ||
       (blendMode == vtkVolumeMapper::COMPOSITE_BLEND &&
        this->LastSampleDistance != sampleDistance) ||
       needUpdate || !this->TextureObject->GetHandle())
      {
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

      this->TextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
      this->TextureObject->SetMagnificationFilter(filterValue);
      this->TextureObject->SetMinificationFilter(filterValue);
      this->TextureObject->CreateAlphaFromRaw(this->TextureWidth,
                                              vtkTextureObject::alpha16,
                                              VTK_FLOAT,
                                              this->Table);
      this->LastInterpolation = filterValue;
      this->TextureObject->Activate();
      this->BuildTime.Modified();
      }

    if(this->LastInterpolation != filterValue)
      {
      this->LastInterpolation = filterValue;
      this->TextureObject->SetMagnificationFilter(filterValue);
      this->TextureObject->SetMinificationFilter(filterValue);
      }
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

  //--------------------------------------------------------------------------
  void ReleaseGraphicsResources(vtkWindow *window)
    {
    if (this->TextureObject)
      {
      this->TextureObject->ReleaseGraphicsResources(window);
      this->TextureObject->Delete();
      this->TextureObject = 0;
      }
    }

protected:
  vtkTextureObject * TextureObject;
  int LastBlendMode;
  int TextureWidth;

  double LastSampleDistance;
  vtkTimeStamp BuildTime;
  float *Table;
  int LastInterpolation;
  double LastRange[2];
private:
  vtkOpenGLVolumeOpacityTable(const vtkOpenGLVolumeOpacityTable&);
  vtkOpenGLVolumeOpacityTable& operator=(const vtkOpenGLVolumeOpacityTable&);
};

//----------------------------------------------------------------------------
class vtkOpenGLVolumeOpacityTables
{
public:
  //--------------------------------------------------------------------------
  vtkOpenGLVolumeOpacityTables(unsigned int numberOfTables)
    {
    this->Tables = new vtkOpenGLVolumeOpacityTable[numberOfTables];
    this->NumberOfTables = numberOfTables;
    }

  //--------------------------------------------------------------------------
  ~vtkOpenGLVolumeOpacityTables()
    {
    delete [] this->Tables;
    }

  // brief Get opacity table at a given index.
  //--------------------------------------------------------------------------
  vtkOpenGLVolumeOpacityTable* GetTable(unsigned int i)
    {
    if (i >= this->NumberOfTables)
      {
      return NULL;
      }
    return &this->Tables[i];
    }

  // Get number of opacity tables.
  //--------------------------------------------------------------------------
  unsigned int GetNumberOfTables()
    {
    return this->NumberOfTables;
    }

  //--------------------------------------------------------------------------
  void ReleaseGraphicsResources(vtkWindow *window)
    {
    for (unsigned int i = 0; i <this->NumberOfTables; ++i)
      {
      this->Tables[i].ReleaseGraphicsResources(window);
      }
    }

private:
  unsigned int NumberOfTables;
  vtkOpenGLVolumeOpacityTable *Tables;

  // vtkOpenGLVolumeOpacityTables (Not implemented)
  vtkOpenGLVolumeOpacityTables();

  // vtkOpenGLVolumeOpacityTables (Not implemented)
  vtkOpenGLVolumeOpacityTables(const vtkOpenGLVolumeOpacityTables &other);

  // operator = (Not implemented)
  vtkOpenGLVolumeOpacityTables &operator=(const vtkOpenGLVolumeOpacityTables &other);
};

#endif // vtkOpenGLVolumeOpacityTable_h_
// VTK-HeaderTest-Exclude: vtkOpenGLVolumeOpacityTable.h
