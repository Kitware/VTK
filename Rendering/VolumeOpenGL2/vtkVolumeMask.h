/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeMask.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkVolumeMask_h_
#define vtkVolumeMask_h_

#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkTextureObject.h>

#include <map> // STL required

//----------------------------------------------------------------------------
class vtkVolumeMask
{
public:
  //--------------------------------------------------------------------------
  vtkVolumeMask()
    {
    this->Texture = NULL;
    this->Loaded = false;
    this->LoadedExtent[0] = VTK_INT_MAX;
    this->LoadedExtent[1] = VTK_INT_MIN;
    this->LoadedExtent[2] = VTK_INT_MAX;
    this->LoadedExtent[3] = VTK_INT_MIN;
    this->LoadedExtent[4] = VTK_INT_MAX;
    this->LoadedExtent[5] = VTK_INT_MIN;
    }

  //--------------------------------------------------------------------------
  ~vtkVolumeMask()
    {
    if (this->Texture)
      {
      this->Texture->Delete();
      this->Texture = 0;
      }
    }

  //--------------------------------------------------------------------------
  vtkTimeStamp GetBuildTime()
    {
    return this->BuildTime;
    }

  //--------------------------------------------------------------------------
  void Bind()
    {
    this->Texture->Activate();
    }

  //--------------------------------------------------------------------------
  void Update(vtkRenderer* ren,
              vtkImageData *input,
              int cellFlag,
              int textureExtent[6],
              int scalarMode,
              int arrayAccessMode,
              int arrayId,
              const char* arrayName,
              vtkIdType maxMemoryInBytes)
    {
      bool needUpdate = false;
      bool modified = false;

      if (!this->Texture)
        {
        this->Texture = vtkTextureObject::New();
        needUpdate = true;
        }

      this->Texture->SetContext(vtkOpenGLRenderWindow::SafeDownCast(
                                  ren->GetRenderWindow()));

      if (!this->Texture->GetHandle())
        {
        needUpdate = true;
        }

      int obsolete = needUpdate || !this->Loaded ||
                     input->GetMTime()>this->BuildTime;
      if(!obsolete)
        {
        obsolete = cellFlag != this->LoadedCellFlag;
        int i = 0;
        while(!obsolete && i<6)
          {
          obsolete = obsolete || this->LoadedExtent[i]>textureExtent[i];
          ++i;
          obsolete = obsolete || this->LoadedExtent[i]<textureExtent[i];
          ++i;
          }
        }

      if(obsolete)
        {
        this->Loaded = false;
        int dim[3];
        input->GetDimensions(dim);

        vtkDataArray *scalars =
          vtkAbstractMapper::GetScalars(input,scalarMode,arrayAccessMode,
                                        arrayId,arrayName,
                                        this->LoadedCellFlag);

        // DONT USE GetScalarType() or GetNumberOfScalarComponents() on
        // ImageData as it deals only with point data...
        int scalarType = scalars->GetDataType();
        if(scalarType != VTK_UNSIGNED_CHAR)
          {
          cout <<"Mask should be VTK_UNSIGNED_CHAR." << endl;
          }
        if(scalars->GetNumberOfComponents()!=1)
          {
          cout << "Mask should be a one-component scalar field." << endl;
          }

        GLint internalFormat = GL_ALPHA8;
        GLenum format = GL_ALPHA;
        GLenum type = GL_UNSIGNED_BYTE;

        // Enough memory?
        int textureSize[3];
        int i = 0;
        while(i < 3)
          {
          textureSize[i] = textureExtent[2*i+1] - textureExtent[2*i] + 1;
          ++i;
          }

        GLint width;
        glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &width);
        this->Loaded = textureSize[0] <= width && textureSize[1] <= width &&
                       textureSize[2] <= width;
        if(this->Loaded)
          {
          // so far, so good but some cards don't report allocation error
          this->Loaded = textureSize[0] * textureSize[1]*
                         textureSize[2] *
                         vtkAbstractArray::GetDataTypeSize(scalarType) *
                         scalars->GetNumberOfComponents() <=
                         maxMemoryInBytes;
          if(this->Loaded)
            {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            if(!(textureExtent[1]-textureExtent[0]+cellFlag==dim[0]))
              {
              glPixelStorei(GL_UNPACK_ROW_LENGTH,dim[0]-cellFlag);
              }
            if(!(textureExtent[3]-textureExtent[2]+cellFlag==dim[1]))
              {
              glPixelStorei(GL_UNPACK_IMAGE_HEIGHT_EXT,
                            dim[1]-cellFlag);
              }
            void* dataPtr = scalars->GetVoidPointer(
                            ((textureExtent[4]*(dim[1]-cellFlag)+textureExtent[2]) *
                            (dim[0]-cellFlag)+textureExtent[0]) *
                            scalars->GetNumberOfComponents());

            this->Texture->SetDataType(type);
            this->Texture->SetFormat(format);
            this->Texture->SetInternalFormat(internalFormat);
            this->Texture->Create3DFromRaw(
              textureSize[0], textureSize[1], textureSize[2],
              1, scalarType, dataPtr);
            this->Texture->Activate();
            this->Texture->SetWrapS(vtkTextureObject::ClampToEdge);
            this->Texture->SetWrapT(vtkTextureObject::ClampToEdge);
            this->Texture->SetWrapR(vtkTextureObject::ClampToEdge);
            this->Texture->SetMagnificationFilter(vtkTextureObject::Nearest);
            this->Texture->SetMinificationFilter(vtkTextureObject::Nearest);
            this->Texture->SetBorderColor(0.0f, 0.0f, 0.0f, 0.0f);

            // Restore the default values.
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            glPixelStorei(GL_UNPACK_IMAGE_HEIGHT_EXT, 0);

            this->LoadedCellFlag = cellFlag;
            i = 0;
            while(i < 6)
              {
              this->LoadedExtent[i] = textureExtent[i];
              ++i;
              }

            double spacing[3];
            double origin[3];
            input->GetSpacing(spacing);
            input->GetOrigin(origin);
            int swapBounds[3];
            swapBounds[0] = (spacing[0] < 0);
            swapBounds[1] = (spacing[1] < 0);
            swapBounds[2] = (spacing[2] < 0);

            if(!this->LoadedCellFlag) // loaded extents represent points
              {
              // slabsPoints[i]=(slabsDataSet[i] - origin[i/2]) / spacing[i/2];
              // in general, x=o+i*spacing.
              // if spacing is positive min extent match the min of the
              // bounding box
              // and the max extent match the max of the bounding box
              // if spacing is negative min extent match the max of the
              // bounding box
              // and the max extent match the min of the bounding box

              // if spacing is negative, we may have to rethink the equation
              // between real point and texture coordinate...
              this->LoadedBounds[0]=origin[0]+
                static_cast<double>(this->LoadedExtent[0+swapBounds[0]])*spacing[0];
              this->LoadedBounds[2]=origin[1]+
                static_cast<double>(this->LoadedExtent[2+swapBounds[1]])*spacing[1];
              this->LoadedBounds[4]=origin[2]+
                static_cast<double>(this->LoadedExtent[4+swapBounds[2]])*spacing[2];
              this->LoadedBounds[1]=origin[0]+
                static_cast<double>(this->LoadedExtent[1-swapBounds[0]])*spacing[0];
              this->LoadedBounds[3]=origin[1]+
                static_cast<double>(this->LoadedExtent[3-swapBounds[1]])*spacing[1];
              this->LoadedBounds[5]=origin[2]+
                static_cast<double>(this->LoadedExtent[5-swapBounds[2]])*spacing[2];

              }
            else // loaded extents represent cells
              {
              int wholeTextureExtent[6];
              input->GetExtent(wholeTextureExtent);
              i=1;
              while(i<6)
                {
                wholeTextureExtent[i]--;
                i+=2;
                }

              i=0;
              while(i<3)
                {
                if(this->LoadedExtent[2*i]==wholeTextureExtent[2*i])
                  {
                  this->LoadedBounds[2*i+swapBounds[i]]=origin[i];
                  }
                else
                  {
                  this->LoadedBounds[2*i+swapBounds[i]]=origin[i]+
                    (static_cast<double>(this->LoadedExtent[2*i])+0.5)*spacing[i];
                  }

                if(this->LoadedExtent[2*i+1]==wholeTextureExtent[2*i+1])
                  {
                  this->LoadedBounds[2*i+1-swapBounds[i]]=origin[i]+
                    (static_cast<double>(this->LoadedExtent[2*i+1])+1.0)*spacing[i];
                  }
                else
                  {
                  this->LoadedBounds[2*i+1-swapBounds[i]]=origin[i]+
                    (static_cast<double>(this->LoadedExtent[2*i+1])+0.5)*spacing[i];
                  }
                ++i;
                }
              }
            modified = true;
            }
          }
        }

      if(modified)
        {
        this->BuildTime.Modified();
        }
    }

  //--------------------------------------------------------------------------
  double* GetLoadedBounds()
    {
    return this->LoadedBounds;
    }

  //--------------------------------------------------------------------------
  vtkIdType* GetLoadedExtent()
    {
    return this->LoadedExtent;
    }

  //--------------------------------------------------------------------------
  int GetLoadedCellFlag()
    {
    return this->LoadedCellFlag;
    }

  //--------------------------------------------------------------------------
  bool IsLoaded()
    {
    return this->Loaded;
    }

  // Get the texture unit
  //--------------------------------------------------------------------------
  int GetTextureUnit(void)
    {
    if (!this->Texture)
      {
      return -1;
      }
    return this->Texture->GetTextureUnit();
    }

  //--------------------------------------------------------------------------
  void ReleaseGraphicsResources(vtkWindow *window)
    {
    if (this->Texture)
      {
      this->Texture->ReleaseGraphicsResources(window);
      this->Texture->Delete();
      this->Texture = 0;
      }
    }


protected:
  vtkTextureObject* Texture;
  vtkTimeStamp BuildTime;

  double LoadedBounds[6];
  vtkIdType LoadedExtent[6];

  int LoadedCellFlag;
  bool Loaded;
};

//----------------------------------------------------------------------------
class vtkMapMaskTextureId
{
public:
  std::map<vtkImageData *,vtkVolumeMask*> Map;
  vtkMapMaskTextureId()
    {
    }
private:
  vtkMapMaskTextureId(const vtkMapMaskTextureId &other);
  vtkMapMaskTextureId &operator=(const vtkMapMaskTextureId &other);
};

#endif // vtkVolumeMask_h_
// VTK-HeaderTest-Exclude: vtkVolumeMask.h
