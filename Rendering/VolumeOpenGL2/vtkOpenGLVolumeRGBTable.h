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

#ifndef vtkOpenGLVolumeRGBTable_h_
#define vtkOpenGLVolumeRGBTable_h_

#include <vtkColorTransferFunction.h>
#include <vtkTextureObject.h>
#include <vtk_glew.h>

//----------------------------------------------------------------------------
class vtkOpenGLVolumeRGBTable
{
public:
  //--------------------------------------------------------------------------
  vtkOpenGLVolumeRGBTable()
    {
    this->TextureWidth = 1024;
    this->NumberOfColorComponents = 3;
    this->TextureObject = 0;
    this->LastInterpolation = -1;
    this->LastRange[0] = this->LastRange[1] = 0;
    this->Table = 0;
    }

  //--------------------------------------------------------------------------
  ~vtkOpenGLVolumeRGBTable()
    {
    if (this->TextureObject)
      {
      this->TextureObject->Delete();
      this->TextureObject = 0;
      }
    if(this->Table)
      {
      delete[] this->Table;
      this->Table=0;
      }
    }

  // Bind texture.
  //--------------------------------------------------------------------------
  void Bind(void)
    {
    if (!this->TextureObject)
      {
      return;
      }
    this->TextureObject->Activate();
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
      // Create table if not created already
      if(this->Table==0)
        {
        this->Table = new float[this->TextureWidth *
          this->NumberOfColorComponents];
        }

      scalarRGB->GetTable(this->LastRange[0], this->LastRange[1],
                          this->TextureWidth, this->Table);
      this->TextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
      this->TextureObject->SetMagnificationFilter(filterValue);
      this->TextureObject->SetMinificationFilter(filterValue);
      this->TextureObject->Create1DFromRaw(this->TextureWidth,
                                           this->NumberOfColorComponents,
                                           VTK_FLOAT,
                                           this->Table);
      this->LastInterpolation = filterValue;
      this->TextureObject->Activate();
      this->BuildTime.Modified();
      }

    if (this->LastInterpolation != filterValue)
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
  int TextureWidth;
  int NumberOfColorComponents;

  vtkTextureObject* TextureObject;

  int LastInterpolation;
  double LastRange[2];
  float* Table;
  vtkTimeStamp BuildTime;
};

//----------------------------------------------------------------------------
class vtkOpenGLVolumeRGBTables
{
public:
  //--------------------------------------------------------------------------
  vtkOpenGLVolumeRGBTables(unsigned int numberOfTables)
    {
    this->Tables = new vtkOpenGLVolumeRGBTable[numberOfTables];
    this->NumberOfTables = numberOfTables;
    }

  //--------------------------------------------------------------------------
  ~vtkOpenGLVolumeRGBTables()
    {
    delete [] this->Tables;
    }

  // brief Get opacity table at a given index.
  //--------------------------------------------------------------------------
  vtkOpenGLVolumeRGBTable* GetTable(unsigned int i)
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
  vtkOpenGLVolumeRGBTable* Tables;

  // vtkOpenGLVolumeRGBTables (Not implemented)
  vtkOpenGLVolumeRGBTables();

  // vtkOpenGLVolumeRGBTables (Not implemented)
  vtkOpenGLVolumeRGBTables(const vtkOpenGLVolumeRGBTables &other);

  // operator = (Not implemented)
  vtkOpenGLVolumeRGBTables &operator=(const vtkOpenGLVolumeRGBTables &other);
};

#endif // vtkOpenGLVolumeRGBTable_h_
// VTK-HeaderTest-Exclude: vtkOpenGLVolumeRGBTable.h
