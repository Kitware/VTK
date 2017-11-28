/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeGradientOpacityTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkOpenGLVolumeGradientOpacityTable_h
#define vtkOpenGLVolumeGradientOpacityTable_h
#ifndef __VTK_WRAP__

#include <vector>

#include <vtkObjectFactory.h>
#include <vtkPiecewiseFunction.h>
#include <vtkTextureObject.h>
#include <vtkVolumeMapper.h>
#include <vtkMath.h>
#include <vtk_glew.h>


//----------------------------------------------------------------------------
class vtkOpenGLVolumeGradientOpacityTable : public vtkObject
{
public:

  static vtkOpenGLVolumeGradientOpacityTable* New();

  // activate texture.
  //--------------------------------------------------------------------------
  void Activate()
  {
    if (!this->TextureObject)
    {
      return;
    }
    this->TextureObject->Activate();
  }

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
  void Update(vtkPiecewiseFunction* gradientOpacity,
              double sampleDistance,
              double range[2],
              double vtkNotUsed(unitDistance),
              int filterValue,
              vtkOpenGLRenderWindow* renWin)
  {
    bool needUpdate=false;

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

    if(gradientOpacity->GetMTime() > this->BuildTime ||
       this->TextureObject->GetMTime() > this->BuildTime ||
       this->LastSampleDistance != sampleDistance ||
       needUpdate || !this->TextureObject->GetHandle())
    {
      int const idealW = gradientOpacity->EstimateMinNumberOfSamples(this->LastRange[0],
        this->LastRange[1]);
      int const newWidth = this->GetMaximumSupportedTextureWidth(renWin, idealW);

      if(this->Table == nullptr || this->TextureWidth != newWidth)
      {
        this->TextureWidth = newWidth;
        delete [] this->Table;
        this->Table = new float[this->TextureWidth];
      }

      gradientOpacity->GetTable(0,
                                (this->LastRange[1] - this->LastRange[0]) * 0.25,
                                this->TextureWidth, this->Table);

      this->TextureObject->Create2DFromRaw(this->TextureWidth, 1, 1,
                                           VTK_FLOAT,
                                           this->Table);

      this->TextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
      this->TextureObject->SetMagnificationFilter(filterValue);
      this->TextureObject->SetMinificationFilter(filterValue);
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
      "texture size of " << idealWidth << ", falling back to maximum allowed, "
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
      this->TextureObject = nullptr;
    }
  }

protected:
  //--------------------------------------------------------------------------
  vtkOpenGLVolumeGradientOpacityTable(int width = 1024)
  {
      this->TextureObject = nullptr;
      this->TextureWidth = width;
      this->LastSampleDistance = 1.0;
      this->Table = nullptr;
      this->LastInterpolation = -1;
      this->LastRange[0] = this->LastRange[1] = 0.0;
  }

  //--------------------------------------------------------------------------
  ~vtkOpenGLVolumeGradientOpacityTable() override
  {
      if (this->TextureObject)
      {
        this->TextureObject->Delete();
        this->TextureObject = nullptr;
      }

      delete[] this->Table;
  }

  vtkTextureObject* TextureObject;
  int TextureWidth;

  double LastSampleDistance;
  vtkTimeStamp BuildTime;
  float* Table;
  int LastInterpolation;
  double LastRange[2];

private:
  vtkOpenGLVolumeGradientOpacityTable(
      const vtkOpenGLVolumeGradientOpacityTable &) = delete;
  vtkOpenGLVolumeGradientOpacityTable& operator=(
    const vtkOpenGLVolumeGradientOpacityTable&) = delete;
};

////////////////////////////////////////////////////////////////////////////////
class vtkOpenGLVolumeGradientOpacityTables : public vtkObject
{
public:
  static vtkOpenGLVolumeGradientOpacityTables* New();

  //--------------------------------------------------------------------------
  void Create(unsigned int numberOfTables)
  {
    this->Tables.reserve(static_cast<size_t>(numberOfTables));

    for (unsigned int i = 0; i < numberOfTables; i++)
    {
      vtkOpenGLVolumeGradientOpacityTable* table =
        vtkOpenGLVolumeGradientOpacityTable::New();
      this->Tables.push_back(table);
    }
  }

  //--------------------------------------------------------------------------
  ~vtkOpenGLVolumeGradientOpacityTables()
  {
    size_t const size = this->Tables.size();
    for (size_t i = 0; i < size; i++)
    {
      this->Tables[i]->Delete();
    }
  }

  // Get opacity table at a given index.
  //--------------------------------------------------------------------------
  vtkOpenGLVolumeGradientOpacityTable* GetTable(unsigned int i)
  {
    if (i >= this->Tables.size())
    {
      return nullptr;
    }
    return this->Tables[i];
  }

  // Get number of tables.
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
protected:
  vtkOpenGLVolumeGradientOpacityTables() = default;

private:
  std::vector<vtkOpenGLVolumeGradientOpacityTable*> Tables;

  vtkOpenGLVolumeGradientOpacityTables(
    const vtkOpenGLVolumeGradientOpacityTables &other) = delete;
  vtkOpenGLVolumeGradientOpacityTables &operator=(
    const vtkOpenGLVolumeGradientOpacityTables &other) = delete;
};

#endif
#endif // vtkOpenGLVolumeGradientOpacityTable_h
// VTK-HeaderTest-Exclude: vtkOpenGLVolumeGradientOpacityTable.h
