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

#include <map> // STL required

//----------------------------------------------------------------------------
class vtkVolumeMask
{
public:
  vtkVolumeMask()
    {
      this->TextureId = 0;
      this->Loaded = false;
      this->LoadedExtent[0] = VTK_INT_MAX;
      this->LoadedExtent[1] = VTK_INT_MIN;
      this->LoadedExtent[2] = VTK_INT_MAX;
      this->LoadedExtent[3] = VTK_INT_MIN;
      this->LoadedExtent[4] = VTK_INT_MAX;
      this->LoadedExtent[5] = VTK_INT_MIN;
    }

  ~vtkVolumeMask()
    {
      if(this->TextureId != 0)
        {
        glDeleteTextures(1, &this->TextureId);
        this->TextureId = 0;
        }
    }

  vtkTimeStamp GetBuildTime()
    {
    return this->BuildTime;
    }

  void Bind()
    {
      // Activate texture 6
      glActiveTexture(GL_TEXTURE6);
      glBindTexture(GL_TEXTURE_3D, this->TextureId);
    }

  void Update(vtkImageData *input,
              int cellFlag,
              int textureExtent[6],
              int scalarMode,
              int arrayAccessMode,
              int arrayId,
              const char* arrayName,
              vtkIdType maxMemoryInBytes)
    {
      glActiveTexture(GL_TEXTURE6);

      bool needUpdate = false;
      bool modified = false;
      if(this->TextureId == 0)
        {
        glGenTextures(1, &this->TextureId);
        needUpdate = true;
        }
      glBindTexture(GL_TEXTURE_3D,this->TextureId);

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
        int i=0;
        while(i<3)
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
          // So far, so good but some cards always succeed with a proxy texture
          // let's try to actually allocate..
          glTexImage3D(GL_TEXTURE_3D, 0, internalFormat, textureSize[0],
                       textureSize[1], textureSize[2], 0, format, type, 0);
          GLenum errorCode = glGetError();
          this->Loaded = errorCode!= GL_OUT_OF_MEMORY;
          if(this->Loaded)
            {
            // so far, so good, actual allocation succeeded.
            if(errorCode != GL_NO_ERROR)
              {
              cout << "After try to load the texture";
              cout << "ERROR (x"<<hex<<errorCode<<") " << dec;
              cout << endl;
              }
            // so far, so good but some cards don't report allocation error
            this->Loaded = textureSize[0] * textureSize[1]*
                           textureSize[2]*vtkAbstractArray::GetDataTypeSize(scalarType)*
            scalars->GetNumberOfComponents() <= maxMemoryInBytes;
            if(this->Loaded)
              {
              // OK, we consider the allocation above succeeded...
              // If it actually didn't the only to fix it for the user
              // is to decrease the value of this->MaxMemoryInBytes.

              // enough memory! We can load the scalars!

              // we don't clamp to edge because for the computation of the
              // gradient on the border we need some external value.
              glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
              glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
              glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

              GLfloat borderColor[4]={0.0,0.0,0.0,0.0};
              glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, borderColor);

              glPixelTransferf(GL_ALPHA_SCALE, 1.0);
              glPixelTransferf(GL_ALPHA_BIAS, 0.0);
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

              glTexImage3D(GL_TEXTURE_3D, 0, internalFormat,
                           textureSize[0], textureSize[1], textureSize[2],
                           0, format, type, dataPtr);

              // Restore the default values.
              glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
              glPixelStorei(GL_UNPACK_IMAGE_HEIGHT_EXT,0);
              glPixelTransferf(GL_ALPHA_SCALE,1.0);
              glPixelTransferf(GL_ALPHA_BIAS,0.0);

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
              modified=true;
              } // if enough memory
            } //load fail with out of memory
          }
        }

      if(this->Loaded && (needUpdate || modified))
        {
        glTexParameterf(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,
                        GL_NEAREST );
        glTexParameterf(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,
                        GL_NEAREST );
        modified=true;
        }
      if(modified)
        {
        this->BuildTime.Modified();
        }
      glActiveTexture(GL_TEXTURE0);
    }

  double* GetLoadedBounds()
    {
    return this->LoadedBounds;
    }

  vtkIdType* GetLoadedExtent()
    {
    return this->LoadedExtent;
    }

  int GetLoadedCellFlag()
    {
    return this->LoadedCellFlag;
    }

  bool IsLoaded()
    {
    return this->Loaded;
    }

protected:
  GLuint TextureId;
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
