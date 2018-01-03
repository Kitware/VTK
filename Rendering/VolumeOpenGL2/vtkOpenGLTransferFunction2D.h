/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLTransferFunction2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkOpenGLTransferFunction2D_h
#define vtkOpenGLTransferFunction2D_h
#ifndef __VTK_WRAP__

#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkImageResize.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkTextureObject.h>
#include <vtk_glew.h>


/**
 * \brief 2D Transfer function container.
 *
 * Manages the texture fetched by the fragment shader when TransferFunction2D
 * mode is active. Update() assumes the vtkImageData instance used as source
 * is of type VTK_FLOAT and has 4 components (vtkVolumeProperty ensures this
 * is the case when the function is set).
 *
 * \sa vtkVolumeProperty::SetTransferFunction2D
 */
class vtkOpenGLTransferFunction2D : public vtkObject
{
public:

  static vtkOpenGLTransferFunction2D* New();

  //--------------------------------------------------------------------------
  void Activate()
  {
    if (!this->TextureObject)
    {
      return;
    }
    this->TextureObject->Activate();
  };

  //--------------------------------------------------------------------------
  void Deactivate()
  {
    if (!this->TextureObject)
    {
      return;
    }
    this->TextureObject->Deactivate();
  };

  //--------------------------------------------------------------------------
  void Update(vtkImageData* transfer2D, int interpolation,
    vtkOpenGLRenderWindow* renWin)
  {
    if (!this->TextureObject)
    {
      this->TextureObject = vtkTextureObject::New();
    }
    this->TextureObject->SetContext(renWin);

    // Reload texture
    if (transfer2D->GetMTime() > this->BuildTime ||
      this->TextureObject->GetMTime() > this->BuildTime ||
      !this->TextureObject->GetHandle())
    {
      int* dims = transfer2D->GetDimensions();
      int const width = this->GetMaximumSupportedTextureWidth(renWin, dims[0]);
      int const height = this->GetMaximumSupportedTextureWidth(renWin, dims[1]);

      // Resample if there is a size restriction
      void* data = transfer2D->GetPointData()->GetScalars()->GetVoidPointer(0);
      if (dims[0] != width || dims[1] != height)
      {
        this->ResizeFilter->SetInputData(transfer2D);
        this->ResizeFilter->SetResizeMethodToOutputDimensions();
        this->ResizeFilter->SetOutputDimensions(width, height, 1);
        this->ResizeFilter->Update();
        data = this->ResizeFilter->GetOutput()->GetPointData()->GetScalars(
          )->GetVoidPointer(0);
      }

      this->TextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
      this->TextureObject->SetWrapT(vtkTextureObject::ClampToEdge);
      this->TextureObject->SetMagnificationFilter(interpolation);
      this->TextureObject->SetMinificationFilter(interpolation);
      this->TextureObject->Create2DFromRaw(width, height, 4, VTK_FLOAT,
        data);
      this->LastInterpolation = interpolation;
      this->BuildTime.Modified();
    }

    // Update filtering
    if (this->LastInterpolation != interpolation)
    {
      this->LastInterpolation = interpolation;
      this->TextureObject->SetMagnificationFilter(interpolation);
      this->TextureObject->SetMinificationFilter(interpolation);
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
      return 256;
    }

    if (maxWidth >= idealWidth)
    {
      idealWidth = vtkMath::Max(256, idealWidth);
      return idealWidth;
    }

    vtkWarningMacro("This OpenGL implementation does not support the required "
      "texture size of " << idealWidth << ", falling back to maximum allowed, "
      << maxWidth << "." << "This may cause an incorrect color table mapping.");

    return maxWidth;
  }

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
  vtkOpenGLTransferFunction2D()
  : vtkObject()
  , TextureObject(nullptr)
  , LastInterpolation(-1)
  {
  }

  //--------------------------------------------------------------------------
  ~vtkOpenGLTransferFunction2D() override
  {
    if (this->TextureObject)
    {
      this->TextureObject->Delete();
      this->TextureObject = nullptr;
    }
  }

  vtkNew<vtkImageResize> ResizeFilter;
  vtkTextureObject* TextureObject;
  int LastInterpolation;
  vtkTimeStamp BuildTime;

private:
  vtkOpenGLTransferFunction2D(
    const vtkOpenGLTransferFunction2D&) = delete;
  vtkOpenGLTransferFunction2D& operator=(
    const vtkOpenGLTransferFunction2D&) = delete;
};

////////////////////////////////////////////////////////////////////////////////

/**
 *  \brief Container for a set of TransferFunction2D instances.
 *
 *  Used as a convenience class to instantiate functions for each component.
 *
 *  \note This class will be merged with other VolumeOpenGL2/vtk*Tables to reduce
 *  code duplication.
 *
 *  \sa vtkOpenGLVolumeRGBTables
 */
class vtkOpenGLTransferFunctions2D : public vtkObject
{
public:
  static vtkOpenGLTransferFunctions2D* New();

  //--------------------------------------------------------------------------
  void Create(unsigned int numberOfTables)
  {
    this->Tables.reserve(static_cast<size_t>(numberOfTables));

    for (unsigned int i = 0; i < numberOfTables; i++)
    {
      vtkOpenGLTransferFunction2D* table = vtkOpenGLTransferFunction2D::New();
      this->Tables.push_back(table);
    }
  }

  //--------------------------------------------------------------------------
  ~vtkOpenGLTransferFunctions2D()
  {
    size_t const size = this->Tables.size();
    for (size_t i = 0; i < size; i++)
    {
      this->Tables[i]->Delete();
    }
  }

  //--------------------------------------------------------------------------
  vtkOpenGLTransferFunction2D* GetTable(unsigned int i)
  {
    if (i >= this->Tables.size())
    {
      return nullptr;
    }
    return this->Tables[i];
  }

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
  vtkOpenGLTransferFunctions2D() = default;

private:
  vtkOpenGLTransferFunctions2D(
    const vtkOpenGLTransferFunctions2D& other) = delete;
  vtkOpenGLTransferFunctions2D& operator=(
    const vtkOpenGLTransferFunctions2D& other) = delete;

  std::vector<vtkOpenGLTransferFunction2D*> Tables;
};

#endif
#endif // vtkOpenGLTransferFunction2D_h
// VTK-HeaderTest-Exclude: vtkOpenGLTransferFunction2D.h
