/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeRGBTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkOpenGLVolumeRGBTable_h
#define vtkOpenGLVolumeRGBTable_h

#include <vtkObjectFactory.h>
#include <vtkColorTransferFunction.h>
#include <vtkTextureObject.h>
#include <vtk_glew.h>
#include <vtkMath.h>


//----------------------------------------------------------------------------
class vtkOpenGLVolumeRGBTable : public vtkObject
{
public:

  static vtkOpenGLVolumeRGBTable* New();

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

  // Update color transfer function texture.
  //--------------------------------------------------------------------------
  void Update(vtkColorTransferFunction* scalarRGB,
              double range[2],
              int filterValue,
              vtkOpenGLRenderWindow* renWin)
  {
    bool needUpdate = false;

    if (!this->TextureObject)
    {
      this->TextureObject = vtkTextureObject::New();
    }

    this->TextureObject->SetContext(renWin);

    if (range[0] != this->LastRange[0] || range[1] != this->LastRange[1])
    {
      this->LastRange[0] = range[0];
      this->LastRange[1] = range[1];
      needUpdate = true;
    }

    if (scalarRGB->GetMTime() > this->BuildTime ||
        this->TextureObject->GetMTime() > this->BuildTime ||
        needUpdate || !this->TextureObject->GetHandle())
    {
      int const idealW = scalarRGB->EstimateMinNumberOfSamples(this->LastRange[0],
        this->LastRange[1]);
      int const newWidth = this->GetMaximumSupportedTextureWidth(renWin, idealW);

      if(this->Table == NULL || this->TextureWidth != newWidth)
      {
        this->TextureWidth = newWidth;
        delete [] this->Table;
        this->Table = new float[this->TextureWidth *
          this->NumberOfColorComponents];
      }

      scalarRGB->GetTable(this->LastRange[0], this->LastRange[1],
                          this->TextureWidth, this->Table);
      this->TextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
      this->TextureObject->SetWrapT(vtkTextureObject::ClampToEdge);
      this->TextureObject->SetMagnificationFilter(filterValue);
      this->TextureObject->SetMinificationFilter(filterValue);
      this->TextureObject->Create2DFromRaw(this->TextureWidth, 1,
                                           this->NumberOfColorComponents,
                                           VTK_FLOAT,
                                           this->Table);
      this->LastInterpolation = filterValue;
      this->BuildTime.Modified();
    }

    if (this->LastInterpolation != filterValue)
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
      this->TextureObject = 0;
    }
  }

protected:

  //--------------------------------------------------------------------------
  vtkOpenGLVolumeRGBTable()
  {
    this->TextureWidth = 1024;
    this->NumberOfColorComponents = 3;
    this->TextureObject = NULL;
    this->LastInterpolation = -1;
    this->LastRange[0] = this->LastRange[1] = 0;
    this->Table = NULL;
  }

  //--------------------------------------------------------------------------
  ~vtkOpenGLVolumeRGBTable() VTK_OVERRIDE
  {
    if (this->TextureObject)
    {
      this->TextureObject->Delete();
      this->TextureObject = NULL;
    }

    delete[] this->Table;
  }


  int TextureWidth;
  int NumberOfColorComponents;

  vtkTextureObject* TextureObject;

  int LastInterpolation;
  double LastRange[2];
  float* Table;
  vtkTimeStamp BuildTime;

private:
  vtkOpenGLVolumeRGBTable(const vtkOpenGLVolumeRGBTable&)
    VTK_DELETE_FUNCTION;
  vtkOpenGLVolumeRGBTable& operator=(const vtkOpenGLVolumeRGBTable&)
    VTK_DELETE_FUNCTION;
};

vtkStandardNewMacro(vtkOpenGLVolumeRGBTable);


////////////////////////////////////////////////////////////////////////////////
class vtkOpenGLVolumeRGBTables
{
public:
  //--------------------------------------------------------------------------
  vtkOpenGLVolumeRGBTables(unsigned int numberOfTables)
  {
    this->Tables.reserve(static_cast<size_t>(numberOfTables));

    for (unsigned int i = 0; i < numberOfTables; i++)
    {
      vtkOpenGLVolumeRGBTable* table = vtkOpenGLVolumeRGBTable::New();
      this->Tables.push_back(table);
    }
  }

  //--------------------------------------------------------------------------
  ~vtkOpenGLVolumeRGBTables()
  {
    size_t const size = this->Tables.size();
    for (size_t i = 0; i < size; i++)
    {
      this->Tables[i]->Delete();
    }
  }

  // brief Get opacity table at a given index.
  //--------------------------------------------------------------------------
  vtkOpenGLVolumeRGBTable* GetTable(unsigned int i)
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
  std::vector<vtkOpenGLVolumeRGBTable*> Tables;

  vtkOpenGLVolumeRGBTables() VTK_DELETE_FUNCTION;

  vtkOpenGLVolumeRGBTables(const vtkOpenGLVolumeRGBTables &other) VTK_DELETE_FUNCTION;

  vtkOpenGLVolumeRGBTables &operator=(const vtkOpenGLVolumeRGBTables &other) VTK_DELETE_FUNCTION;
};

#endif // vtkOpenGLVolumeRGBTable_h
// VTK-HeaderTest-Exclude: vtkOpenGLVolumeRGBTable.h
