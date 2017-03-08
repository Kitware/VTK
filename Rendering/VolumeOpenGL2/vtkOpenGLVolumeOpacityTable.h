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

#ifndef vtkOpenGLVolumeOpacityTable_h
#define vtkOpenGLVolumeOpacityTable_h

#include <vector>

#include <vtkObjectFactory.h>
#include <vtkPiecewiseFunction.h>
#include <vtkTextureObject.h>
#include <vtkVolumeMapper.h>
#include <vtk_glew.h>
#include <vtkMath.h>


//----------------------------------------------------------------------------
class vtkOpenGLVolumeOpacityTable : public vtkObject
{
public:

  static vtkOpenGLVolumeOpacityTable* New();

  // Activate texture.
  //--------------------------------------------------------------------------
  void Activate()
  {
    if (!this->TextureObject)
    {
      return;
    }
    this->TextureObject->Activate();
  }

  // Deactivate texture.
  //--------------------------------------------------------------------------
  void Deactivate()
  {
    if (!this->TextureObject)
    {
      return;
    }
    this->TextureObject->Deactivate();
  }

  // Update opacity transfer function texture.
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
      int const idealW = scalarOpacity->EstimateMinNumberOfSamples(this->LastRange[0],
        this->LastRange[1]);
      int const newWidth = this->GetMaximumSupportedTextureWidth(renWin, idealW);

      if(this->Table == NULL || this->TextureWidth != newWidth)
      {
        this->TextureWidth = newWidth;
        delete [] this->Table;
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
      this->TextureObject->Create2DFromRaw(this->TextureWidth, 1, 1,
                                              VTK_FLOAT,
                                              this->Table);
      this->LastInterpolation = filterValue;
      this->BuildTime.Modified();
    }

    if(this->LastInterpolation != filterValue)
    {
      this->LastInterpolation = filterValue;
      this->TextureObject->SetMagnificationFilter(filterValue);
      this->TextureObject->SetMinificationFilter(filterValue);
    }
  }

  //--------------------------------------------------------------------------
  inline int GetMaximumSupportedTextureWidth(vtkOpenGLRenderWindow* renWin,
    int idealWidth)
  {
    if (!this->TextureObject)
    {
      vtkErrorMacro("vtkTextureObject not initialized!");
      return -1;
    }

    // Try to match the next power of two.
    idealWidth = vtkMath::NearestPowerOfTwo(idealWidth);
    int const maxWidth = this->TextureObject->GetMaximumTextureSize(renWin);
    if (maxWidth < 0)
    {
      vtkErrorMacro("Failed to query max texture size! using default 1024.");
      return 1024;
    }

    if (maxWidth >= idealWidth)
    {
      idealWidth = vtkMath::Max(1024, idealWidth);
      return idealWidth;
    }

    vtkWarningMacro("This OpenGL implementation does not support the required "
      "texture size of " << idealWidth << ". Falling back to maximum allowed, "
      << maxWidth << "." << "This may cause an incorrect color table mapping.");

    return maxWidth;
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

  //--------------------------------------------------------------------------
  vtkOpenGLVolumeOpacityTable(int width = 1024)
  {
      this->TextureObject = NULL;
      this->LastBlendMode = vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND;
      this->TextureWidth = width;
      this->LastSampleDistance = 1.0;
      this->Table = NULL;
      this->LastInterpolation = -1;
      this->LastRange[0] = this->LastRange[1] = 0.0;
  }

  //--------------------------------------------------------------------------
  ~vtkOpenGLVolumeOpacityTable() VTK_OVERRIDE
  {
      if (this->TextureObject)
      {
        this->TextureObject->Delete();
        this->TextureObject = NULL;
      }

      delete[] this->Table;
  }


  vtkTextureObject * TextureObject;
  int LastBlendMode;
  int TextureWidth;

  double LastSampleDistance;
  vtkTimeStamp BuildTime;
  float *Table;
  int LastInterpolation;
  double LastRange[2];

private:
  vtkOpenGLVolumeOpacityTable(const vtkOpenGLVolumeOpacityTable&)
    VTK_DELETE_FUNCTION;
  vtkOpenGLVolumeOpacityTable& operator=(const vtkOpenGLVolumeOpacityTable&)
    VTK_DELETE_FUNCTION;
};

vtkStandardNewMacro(vtkOpenGLVolumeOpacityTable);


////////////////////////////////////////////////////////////////////////////////
class vtkOpenGLVolumeOpacityTables
{
public:
  //--------------------------------------------------------------------------
  vtkOpenGLVolumeOpacityTables(unsigned int numberOfTables)
  {
    this->Tables.reserve(static_cast<size_t>(numberOfTables));

    for (unsigned int i = 0; i < numberOfTables; i++)
    {
      vtkOpenGLVolumeOpacityTable* table = vtkOpenGLVolumeOpacityTable::New();
      this->Tables.push_back(table);
    }
  }

  //--------------------------------------------------------------------------
  ~vtkOpenGLVolumeOpacityTables()
  {
    size_t const size = this->Tables.size();
    for (size_t i = 0; i < size; i++)
    {
      this->Tables[i]->Delete();
    }
  }

  // brief Get opacity table at a given index.
  //--------------------------------------------------------------------------
  vtkOpenGLVolumeOpacityTable* GetTable(unsigned int i)
  {
    if (i >= this->Tables.size())
    {
      return NULL;
    }
    return this->Tables[i];
  }

  // Get number of opacity tables.
  //--------------------------------------------------------------------------
  size_t GetNumberOfTables()
  {
    return this->Tables.size();
  }

  //--------------------------------------------------------------------------
  void ReleaseGraphicsResources(vtkWindow *window)
  {
    size_t const size = this->Tables.size();
    for (size_t i = 0; i < size; ++i)
    {
      this->Tables[i]->ReleaseGraphicsResources(window);
    }
  }

private:
  std::vector<vtkOpenGLVolumeOpacityTable*> Tables;

  vtkOpenGLVolumeOpacityTables() VTK_DELETE_FUNCTION;

  vtkOpenGLVolumeOpacityTables(const vtkOpenGLVolumeOpacityTables &other) VTK_DELETE_FUNCTION;

  vtkOpenGLVolumeOpacityTables &operator=(const vtkOpenGLVolumeOpacityTables &other) VTK_DELETE_FUNCTION;
};

#endif // vtkOpenGLVolumeOpacityTable_h
// VTK-HeaderTest-Exclude: vtkOpenGLVolumeOpacityTable.h
