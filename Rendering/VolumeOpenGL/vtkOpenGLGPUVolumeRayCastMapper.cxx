/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLGPUVolumeRayCastMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLGPUVolumeRayCastMapper.h"

#include "vtkObjectFactory.h"
#include "vtkVolume.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkImageData.h"

#include "vtkTimerLog.h"

#include "vtkVolumeProperty.h"
#include "vtkColorTransferFunction.h"
#include "vtkPiecewiseFunction.h"

#include "vtkOpenGLExtensionManager.h"
#include "vtkgl.h"

#include "vtkOpenGL.h"

#include <math.h>

#include <string>
#include <map>
#include <vector>
#include <cassert>

#include "vtkClipDataSet.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGeometryFilter.h"
#include "vtkMath.h"
#include "vtkPlane.h"
#include "vtkPlaneCollection.h"
#include "vtkPlanes.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPoints.h"
#include "vtkTriangle.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVoxel.h"

#include "vtkClipConvexPolyData.h"
#include "vtkClipPolyData.h"
#include "vtkDensifyPolyData.h"

#include "vtkImageResample.h"

#include <sstream>
#include <stdlib.h> // qsort()

#include "vtkDataSetTriangleFilter.h"

#include "vtkAbstractArray.h" // required if compiled against VTK 5.0

#include "vtkTessellatedBoxSource.h"
#include "vtkCleanPolyData.h"

#include "vtkCommand.h" // for VolumeMapperRender{Start|End|Progress}Event
#include "vtkPerlinNoise.h"

#include <vtksys/ios/sstream>
#include "vtkStdString.h"

#include "vtkShaderProgram2.h"
#include "vtkShader2.h"
#include "vtkUniformVariables.h"
#include "vtkShader2Collection.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLError.h"

// Uncomment the following line to debug Snow Leopard
//#define APPLE_SNOW_LEOPARD_BUG

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class vtkUnsupportedRequiredExtensionsStringStream
{
public:
  std::ostringstream Stream;
  vtkUnsupportedRequiredExtensionsStringStream()
    {
    }
private:
  // undefined copy constructor.
  vtkUnsupportedRequiredExtensionsStringStream(const vtkUnsupportedRequiredExtensionsStringStream &other);
  // undefined assignment operator.
  vtkUnsupportedRequiredExtensionsStringStream &operator=(const vtkUnsupportedRequiredExtensionsStringStream &other);
};

class vtkMapDataArrayTextureId
{
public:
  std::map<vtkImageData *,vtkKWScalarField *> Map;
  vtkMapDataArrayTextureId()
    {
    }
private:
  // undefined copy constructor.
  vtkMapDataArrayTextureId(const vtkMapDataArrayTextureId &other);
  // undefined assignment operator.
  vtkMapDataArrayTextureId &operator=(const vtkMapDataArrayTextureId &other);
};

class vtkMapMaskTextureId
{
public:
  std::map<vtkImageData *,vtkKWMask *> Map;
  vtkMapMaskTextureId()
    {
    }
private:
  // undefined copy constructor.
  vtkMapMaskTextureId(const vtkMapMaskTextureId &other);
  // undefined assignment operator.
  vtkMapMaskTextureId &operator=(const vtkMapMaskTextureId &other);
};

//-----------------------------------------------------------------------------


extern const char *vtkGPUVolumeRayCastMapper_CompositeFS;
extern const char *vtkGPUVolumeRayCastMapper_CompositeCroppingFS;
extern const char *vtkGPUVolumeRayCastMapper_CompositeNoCroppingFS;
extern const char *vtkGPUVolumeRayCastMapper_HeaderFS;
extern const char *vtkGPUVolumeRayCastMapper_MIPFS;
extern const char *vtkGPUVolumeRayCastMapper_MIPBinaryMaskFS;
extern const char *vtkGPUVolumeRayCastMapper_MIPFourDependentFS;
extern const char *vtkGPUVolumeRayCastMapper_MIPFourDependentCroppingFS;
extern const char *vtkGPUVolumeRayCastMapper_MIPFourDependentNoCroppingFS;
extern const char *vtkGPUVolumeRayCastMapper_MIPCroppingFS;
extern const char *vtkGPUVolumeRayCastMapper_MIPNoCroppingFS;
extern const char *vtkGPUVolumeRayCastMapper_ParallelProjectionFS;
extern const char *vtkGPUVolumeRayCastMapper_PerspectiveProjectionFS;
extern const char *vtkGPUVolumeRayCastMapper_ScaleBiasFS;
extern const char *vtkGPUVolumeRayCastMapper_MinIPFS;
extern const char *vtkGPUVolumeRayCastMapper_MinIPBinaryMaskFS;
extern const char *vtkGPUVolumeRayCastMapper_MinIPFourDependentFS;
extern const char *vtkGPUVolumeRayCastMapper_MinIPFourDependentCroppingFS;
extern const char *vtkGPUVolumeRayCastMapper_MinIPFourDependentNoCroppingFS;
extern const char *vtkGPUVolumeRayCastMapper_MinIPCroppingFS;
extern const char *vtkGPUVolumeRayCastMapper_MinIPNoCroppingFS;
extern const char *vtkGPUVolumeRayCastMapper_CompositeMaskFS;
extern const char *vtkGPUVolumeRayCastMapper_CompositeBinaryMaskFS;
extern const char *vtkGPUVolumeRayCastMapper_NoShadeFS;
extern const char *vtkGPUVolumeRayCastMapper_ShadeFS;
extern const char *vtkGPUVolumeRayCastMapper_OneComponentFS;
extern const char *vtkGPUVolumeRayCastMapper_FourComponentsFS;
extern const char *vtkGPUVolumeRayCastMapper_AdditiveFS;
extern const char *vtkGPUVolumeRayCastMapper_AdditiveCroppingFS;
extern const char *vtkGPUVolumeRayCastMapper_AdditiveNoCroppingFS;

enum
{
  vtkOpenGLGPUVolumeRayCastMapperProjectionNotInitialized=-1, // not init
  vtkOpenGLGPUVolumeRayCastMapperProjectionPerspective=0, // false
  vtkOpenGLGPUVolumeRayCastMapperProjectionParallel=1 // true
};

enum
{
  vtkOpenGLGPUVolumeRayCastMapperMethodNotInitialized,
  vtkOpenGLGPUVolumeRayCastMapperMethodMIP,
  vtkOpenGLGPUVolumeRayCastMapperMethodMIPBinaryMask,
  vtkOpenGLGPUVolumeRayCastMapperMethodMIPFourDependent,
  vtkOpenGLGPUVolumeRayCastMapperMethodComposite,
  vtkOpenGLGPUVolumeRayCastMapperMethodMinIP,
  vtkOpenGLGPUVolumeRayCastMapperMethodMinIPBinaryMask,
  vtkOpenGLGPUVolumeRayCastMapperMethodMinIPFourDependent,
  vtkOpenGLGPUVolumeRayCastMapperMethodCompositeMask,
  vtkOpenGLGPUVolumeRayCastMapperMethodCompositeBinaryMask,
  vtkOpenGLGPUVolumeRayCastMapperMethodAdditive
};

// component implementation
enum
{
  vtkOpenGLGPUVolumeRayCastMapperComponentNotInitialized=-1, // not init
  vtkOpenGLGPUVolumeRayCastMapperComponentOne=0, // false
  vtkOpenGLGPUVolumeRayCastMapperComponentFour=1, // true
  vtkOpenGLGPUVolumeRayCastMapperComponentNotUsed=2 // when not composite
};

// Shade implementation
enum
{
  vtkOpenGLGPUVolumeRayCastMapperShadeNotInitialized=-1, // not init
  vtkOpenGLGPUVolumeRayCastMapperShadeNo=0, // false
  vtkOpenGLGPUVolumeRayCastMapperShadeYes=1, // true
  vtkOpenGLGPUVolumeRayCastMapperShadeNotUsed=2 // when not composite
};


// Cropping implementation
enum
{
  vtkOpenGLGPUVolumeRayCastMapperCroppingNotInitialized,
  vtkOpenGLGPUVolumeRayCastMapperCompositeCropping,
  vtkOpenGLGPUVolumeRayCastMapperCompositeNoCropping,
  vtkOpenGLGPUVolumeRayCastMapperMIPCropping,
  vtkOpenGLGPUVolumeRayCastMapperMIPNoCropping,
  vtkOpenGLGPUVolumeRayCastMapperMIPFourDependentCropping,
  vtkOpenGLGPUVolumeRayCastMapperMIPFourDependentNoCropping,
  vtkOpenGLGPUVolumeRayCastMapperMinIPCropping,
  vtkOpenGLGPUVolumeRayCastMapperMinIPNoCropping,
  vtkOpenGLGPUVolumeRayCastMapperMinIPFourDependentCropping,
  vtkOpenGLGPUVolumeRayCastMapperMinIPFourDependentNoCropping,
  vtkOpenGLGPUVolumeRayCastMapperAdditiveCropping,
  vtkOpenGLGPUVolumeRayCastMapperAdditiveNoCropping,
};

enum
{
  vtkOpenGLGPUVolumeRayCastMapperTextureObjectDepthMap=0, // 2d texture
  vtkOpenGLGPUVolumeRayCastMapperTextureObjectFrameBufferLeftFront // 2d texture
};

const int vtkOpenGLGPUVolumeRayCastMapperNumberOfTextureObjects=vtkOpenGLGPUVolumeRayCastMapperTextureObjectFrameBufferLeftFront+2;

const int vtkOpenGLGPUVolumeRayCastMapperOpacityTableSize=1024; //power of two

vtkStandardNewMacro(vtkOpenGLGPUVolumeRayCastMapper);

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class vtkOpacityTable
{
public:
  vtkOpacityTable()
    {
      this->TextureId=0;
      this->LastBlendMode=vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND;
      this->LastSampleDistance=1.0;
      this->Table=0;
      this->Loaded=false;
      this->LastLinearInterpolation=false;
      this->LastRange[0] = this->LastRange[1] = 0.0;
    }

  ~vtkOpacityTable()
    {
      if(this->TextureId!=0)
        {
        glDeleteTextures(1,&this->TextureId);
        vtkOpenGLStaticCheckErrorMacro("failed at glDeleteTextures");
        this->TextureId=0;
        }
      if(this->Table!=0)
        {
        delete[] this->Table;
        this->Table=0;
        }
    }

  bool IsLoaded()
    {
      return this->Loaded;
    }

  void Bind()
    {
      assert("pre: uptodate" && this->Loaded);
      glBindTexture(GL_TEXTURE_1D,this->TextureId);
      vtkOpenGLStaticCheckErrorMacro("failed at glBindtexture");
    }

  // \pre the active texture is set to TEXTURE2
  void Update(vtkPiecewiseFunction *scalarOpacity,
              int blendMode,
              double sampleDistance,
              double range[2],
              double unitDistance,
              bool linearInterpolation)
    {
      assert("pre: scalarOpacity_exists" && scalarOpacity!=0);
      vtkOpenGLClearErrorMacro();

      bool needUpdate=false;
      if(this->TextureId==0)
        {
        glGenTextures(1,&this->TextureId);
        needUpdate=true;
        }
      if (this->LastRange[0] != range[0] ||
        this->LastRange[1] != range[1])
        {
        needUpdate = true;
        this->LastRange[0] = range[0];
        this->LastRange[1] = range[1];
        }
      glBindTexture(GL_TEXTURE_1D,this->TextureId);
      if(needUpdate)
        {
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S,
                        vtkgl::CLAMP_TO_EDGE);
        }

      if(scalarOpacity->GetMTime() > this->BuildTime ||
         (this->LastBlendMode!=blendMode)
         || (blendMode==vtkVolumeMapper::COMPOSITE_BLEND &&
             this->LastSampleDistance!=sampleDistance)
         || needUpdate || !this->Loaded)
        {
        this->Loaded=false;
        if(this->Table==0)
          {
          this->Table=
            new float[vtkOpenGLGPUVolumeRayCastMapperOpacityTableSize];
          }

        scalarOpacity->GetTable(range[0],range[1],
                                vtkOpenGLGPUVolumeRayCastMapperOpacityTableSize,
                                this->Table);

        this->LastBlendMode=blendMode;

        // Correct the opacity array for the spacing between the planes if we
        // are using a composite blending operation
        if(blendMode==vtkVolumeMapper::COMPOSITE_BLEND)
          {
          float *ptr=this->Table;
          double factor=sampleDistance/unitDistance;
          int i=0;
          while(i<vtkOpenGLGPUVolumeRayCastMapperOpacityTableSize)
            {
            if(*ptr>0.0001f)
              {
              *ptr=static_cast<float>(1.0-pow(1.0-static_cast<double>(*ptr),
                                              factor));
              }
            ++ptr;
            ++i;
            }
          this->LastSampleDistance=sampleDistance;
          }
        else if (blendMode==vtkVolumeMapper::ADDITIVE_BLEND)
          {
          float *ptr=this->Table;
          double factor=sampleDistance/unitDistance;
          int i=0;
          while(i<vtkOpenGLGPUVolumeRayCastMapperOpacityTableSize)
            {
            if(*ptr>0.0001f)
              {
              *ptr=static_cast<float>(static_cast<double>(*ptr)*factor);
              }
            ++ptr;
            ++i;
            }
          this->LastSampleDistance=sampleDistance;
          }

        glTexImage1D(GL_TEXTURE_1D,0,GL_ALPHA16,
                     vtkOpenGLGPUVolumeRayCastMapperOpacityTableSize,0,
                     GL_ALPHA,GL_FLOAT,this->Table);
        vtkOpenGLStaticCheckErrorMacro("1d opacity texture is too large");
        this->Loaded=true;
        this->BuildTime.Modified();
        }

      needUpdate=needUpdate ||
        this->LastLinearInterpolation!=linearInterpolation;
      if(needUpdate)
        {
        this->LastLinearInterpolation=linearInterpolation;
        GLint value;
        if(linearInterpolation)
          {
          value=GL_LINEAR;
          }
        else
          {
          value=GL_NEAREST;
          }
        glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MIN_FILTER,value);
        glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MAG_FILTER,value);
        }
    vtkOpenGLStaticCheckErrorMacro("failed after Update");
    }
protected:
  GLuint TextureId;
  int LastBlendMode;
  double LastSampleDistance;
  vtkTimeStamp BuildTime;
  float *Table;
  bool Loaded;
  bool LastLinearInterpolation;
  double LastRange[2];
private:
  vtkOpacityTable(const vtkOpacityTable&);
  vtkOpacityTable& operator=(const vtkOpacityTable&);
};

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class vtkOpacityTables
{
public:
  vtkOpacityTables(unsigned int numberOfTables)
    {
    this->Tables = new vtkOpacityTable[numberOfTables];
    this->NumberOfTables = numberOfTables;
    }
  ~vtkOpacityTables()
    {
    delete [] this->Tables;
    }
  vtkOpacityTable* GetTable(unsigned int i)
    {
    return &this->Tables[i];
    }
  unsigned int GetNumberOfTables()
    {
    return this->NumberOfTables;
    }
private:
  unsigned int NumberOfTables;
  vtkOpacityTable *Tables;
  // undefined default constructor.
  vtkOpacityTables();
  // undefined copy constructor.
  vtkOpacityTables(const vtkOpacityTables &other);
  // undefined assignment operator.
  vtkOpacityTables &operator=(const vtkOpacityTables &other);
};

//-----------------------------------------------------------------------------
class vtkRGBTable
{
public:
  vtkRGBTable()
    {
      this->TextureId=0;
      this->Table=0;
      this->Loaded=false;
      this->LastLinearInterpolation=false;
      this->LastRange[0] = this->LastRange[1] = 0;
    }

  ~vtkRGBTable()
    {
      if(this->TextureId!=0)
        {
        glDeleteTextures(1,&this->TextureId);
        vtkOpenGLStaticCheckErrorMacro("failed at glDeleteTextures");
        this->TextureId=0;
        }
      if(this->Table!=0)
        {
        delete[] this->Table;
        this->Table=0;
        }
    }

  bool IsLoaded()
    {
      return this->Loaded;
    }

  void Bind()
    {
      assert("pre: uptodate" && this->Loaded);
      glBindTexture(GL_TEXTURE_1D,this->TextureId);
      vtkOpenGLStaticCheckErrorMacro("failed at glBindTexture");
    }

  // \pre the active texture is set properly. (default color,
  // mask1, mask2,..)
  void Update(vtkColorTransferFunction *scalarRGB,
              double range[2],
              bool linearInterpolation)
    {
      assert("pre: scalarRGB_exists" && scalarRGB!=0);
      vtkOpenGLClearErrorMacro();

      bool needUpdate=false;
      if(this->TextureId==0)
        {
        glGenTextures(1,&this->TextureId);
        needUpdate=true;
        }
      if (range[0] != this->LastRange[0] || range[1] != this->LastRange[1])
        {
        needUpdate=true;
        }
      glBindTexture(GL_TEXTURE_1D,this->TextureId);
      if(needUpdate)
        {
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S,
                        vtkgl::CLAMP_TO_EDGE);
        }
      if(scalarRGB->GetMTime() > this->BuildTime
         || needUpdate || !this->Loaded)
        {
        this->Loaded=false;
        if(this->Table==0)
          {
          this->Table=
            new float[vtkOpenGLGPUVolumeRayCastMapperOpacityTableSize*3];
          }

        scalarRGB->GetTable(range[0],range[1],
                            vtkOpenGLGPUVolumeRayCastMapperOpacityTableSize,
                            this->Table);

        glTexImage1D(GL_TEXTURE_1D,0,GL_RGB16,
                     vtkOpenGLGPUVolumeRayCastMapperOpacityTableSize,0,
                     GL_RGB,GL_FLOAT,this->Table);
        vtkOpenGLStaticCheckErrorMacro("1d RGB texture is too large");
        this->Loaded=true;
        this->BuildTime.Modified();
        this->LastRange[0] = range[0];
        this->LastRange[1] = range[1];
        }

      needUpdate=needUpdate ||
        this->LastLinearInterpolation!=linearInterpolation;
      if(needUpdate)
        {
        this->LastLinearInterpolation=linearInterpolation;
        GLint value;
        if(linearInterpolation)
          {
          value=GL_LINEAR;
          }
        else
          {
          value=GL_NEAREST;
          }
        glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MIN_FILTER,value);
        glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MAG_FILTER,value);
        }
    vtkOpenGLStaticCheckErrorMacro("failed after Update");
    }
protected:
  GLuint TextureId;
  vtkTimeStamp BuildTime;
  float *Table;
  bool Loaded;
  bool LastLinearInterpolation;
  double LastRange[2];
};

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class vtkKWScalarField
{
public:
  vtkKWScalarField()
    {
      this->TextureId=0;
      this->Loaded=false;
      this->Supports_GL_ARB_texture_float=false;
      this->LoadedTableRange[0]=0.0;
      this->LoadedTableRange[1]=1.0;
      this->LoadedExtent[0]=VTK_INT_MAX;
      this->LoadedExtent[1]=VTK_INT_MIN;
      this->LoadedExtent[2]=VTK_INT_MAX;
      this->LoadedExtent[3]=VTK_INT_MIN;
      this->LoadedExtent[4]=VTK_INT_MAX;
      this->LoadedExtent[5]=VTK_INT_MIN;
    }
  ~vtkKWScalarField()
    {
      if(this->TextureId!=0)
        {
        glDeleteTextures(1,&this->TextureId);
        vtkOpenGLStaticCheckErrorMacro("failed at glDeleteTextures");
        this->TextureId=0;
        }
    }

  vtkTimeStamp GetBuildTime()
    {
      return this->BuildTime;
    }

  void Bind()
    {
      assert("pre: uptodate" && this->Loaded);
      glBindTexture(vtkgl::TEXTURE_3D,this->TextureId);
      vtkOpenGLStaticCheckErrorMacro("failed at glBindTexture");
    }

  void Update(vtkImageData *input,
              int cellFlag,
              int textureExtent[6],
              int scalarMode,
              int arrayAccessMode,
              int arrayId,
              const char *arrayName,
              bool linearInterpolation,
              double tableRange[2],
              vtkIdType maxMemoryInBytes)
    {
      vtkOpenGLClearErrorMacro();

      bool needUpdate=false;
      bool modified=false;
      if(this->TextureId==0)
        {
        glGenTextures(1,&this->TextureId);
        needUpdate=true;
        }
      glBindTexture(vtkgl::TEXTURE_3D,this->TextureId);

      int obsolete=needUpdate || !this->Loaded || input->GetMTime()>this->BuildTime;
      if(!obsolete)
        {
        obsolete=cellFlag!=this->LoadedCellFlag;
        int i=0;
        while(!obsolete && i<6)
          {
          obsolete=obsolete || this->LoadedExtent[i]>textureExtent[i];
          ++i;
          obsolete=obsolete || this->LoadedExtent[i]<textureExtent[i];
          ++i;
          }
        }

      if(!obsolete)
        {
        obsolete=this->LoadedTableRange[0]!=tableRange[0] ||
          this->LoadedTableRange[1]!=tableRange[1];
        }

      if(obsolete)
        {
        this->Loaded=false;
        int dim[3];
        input->GetDimensions(dim);

        GLint internalFormat=0;
        GLenum format=0;
        GLenum type=0;
        // shift then scale: y:=(x+shift)*scale
        double shift=0.0;
        double scale=1.0;
        int needTypeConversion=0;
        vtkDataArray *sliceArray=0;

        vtkDataArray *scalars=
          vtkAbstractMapper::GetScalars(input,scalarMode,arrayAccessMode,
                                        arrayId,arrayName,
                                        this->LoadedCellFlag);

        // DONT USE GetScalarType() or GetNumberOfScalarComponents() on
        // ImageData as it deals only with point data...

        int scalarType=scalars->GetDataType();
        if(scalars->GetNumberOfComponents()==4)
          {
          // this is RGBA, unsigned char only
          internalFormat=GL_RGBA16;
          format=GL_RGBA;
          type=GL_UNSIGNED_BYTE;
          }
        else
          {
          // input->GetNumberOfScalarComponents()==1
          switch(scalarType)
            {
            case VTK_FLOAT:
              if(this->Supports_GL_ARB_texture_float)
                {
                internalFormat=vtkgl::INTENSITY16F_ARB;
                }
              else
                {
                internalFormat=GL_INTENSITY16;
                }
              format=GL_RED;
              type=GL_FLOAT;
              shift=-tableRange[0];
              scale=1/(tableRange[1]-tableRange[0]);
              break;
            case VTK_UNSIGNED_CHAR:
              internalFormat=GL_INTENSITY8;
              format=GL_RED;
              type=GL_UNSIGNED_BYTE;
              shift=-tableRange[0]/VTK_UNSIGNED_CHAR_MAX;
              scale=
                VTK_UNSIGNED_CHAR_MAX/(tableRange[1]-tableRange[0]);
              break;
            case VTK_SIGNED_CHAR:
              internalFormat=GL_INTENSITY8;
              format=GL_RED;
              type=GL_BYTE;
              shift=-(2*tableRange[0]+1)/VTK_UNSIGNED_CHAR_MAX;
              scale=VTK_SIGNED_CHAR_MAX/(tableRange[1]-tableRange[0]);
              break;
            case VTK_CHAR:
              // not supported
              assert("check: impossible case" && 0);
              break;
            case VTK_BIT:
              // not supported
              assert("check: impossible case" && 0);
              break;
            case VTK_ID_TYPE:
              // not supported
              assert("check: impossible case" && 0);
              break;
            case VTK_INT:
              internalFormat=GL_INTENSITY16;
              format=GL_RED;
              type=GL_INT;

              shift=-(2*tableRange[0]+1)/VTK_UNSIGNED_INT_MAX;
              scale=VTK_INT_MAX/(tableRange[1]-tableRange[0]);
              break;
            case VTK_DOUBLE:
            case VTK___INT64:
            case VTK_LONG:
            case VTK_LONG_LONG:
            case VTK_UNSIGNED___INT64:
            case VTK_UNSIGNED_LONG:
            case VTK_UNSIGNED_LONG_LONG:
              needTypeConversion=1; // to float
              if(this->Supports_GL_ARB_texture_float)
                {
                internalFormat=vtkgl::INTENSITY16F_ARB;
                }
              else
                {
                internalFormat=GL_INTENSITY16;
                }
              format=GL_RED;
              type=GL_FLOAT;
              shift=-tableRange[0];
              scale=1/(tableRange[1]-tableRange[0]);
              sliceArray=vtkFloatArray::New();
              break;
            case VTK_SHORT:
              internalFormat=GL_INTENSITY16;
              format=GL_RED;
              type=GL_SHORT;

              shift=-(2*tableRange[0]+1)/VTK_UNSIGNED_SHORT_MAX;
              scale=VTK_SHORT_MAX/(tableRange[1]-tableRange[0]);
              break;
            case VTK_STRING:
              // not supported
              assert("check: impossible case" && 0);
              break;
            case VTK_UNSIGNED_SHORT:
              internalFormat=GL_INTENSITY16;
              format=GL_RED;
              type=GL_UNSIGNED_SHORT;

              shift=-tableRange[0]/VTK_UNSIGNED_SHORT_MAX;
              scale=
                VTK_UNSIGNED_SHORT_MAX/(tableRange[1]-tableRange[0]);
              break;
            case VTK_UNSIGNED_INT:
              internalFormat=GL_INTENSITY16;
              format=GL_RED;
              type=GL_UNSIGNED_INT;

              shift=-tableRange[0]/VTK_UNSIGNED_INT_MAX;
              scale=VTK_UNSIGNED_INT_MAX/(tableRange[1]-tableRange[0]);
              break;
            default:
              assert("check: impossible case" && 0);
              break;
            }
          }

        // Enough memory?
        int textureSize[3];
        int i=0;
        while(i<3)
          {
          textureSize[i]=textureExtent[2*i+1]-textureExtent[2*i]+1;
          ++i;
          }

        GLint width;
        glGetIntegerv(vtkgl::MAX_3D_TEXTURE_SIZE,&width);
        this->Loaded=textureSize[0]<=width && textureSize[1]<=width
          && textureSize[2]<=width;
        if(this->Loaded)
          {
          // so far, so good. the texture size is theorically small enough
          // for OpenGL

          vtkgl::TexImage3D(vtkgl::PROXY_TEXTURE_3D,0,internalFormat,
                            textureSize[0],textureSize[1],textureSize[2],0,
                            format,type,0);
          glGetTexLevelParameteriv(vtkgl::PROXY_TEXTURE_3D,0,GL_TEXTURE_WIDTH,
                                   &width);

          this->Loaded=width!=0;
          if(this->Loaded)
            {
            // so far, so good but some cards always succeed with a proxy texture
            // let's try to actually allocate..

            vtkgl::TexImage3D(vtkgl::TEXTURE_3D,0,internalFormat,textureSize[0],
                              textureSize[1],textureSize[2],0,format,type,0);
            GLenum errorCode=glGetError();
            this->Loaded=errorCode!=GL_OUT_OF_MEMORY;
            if(this->Loaded)
              {
              // so far, so good, actual allocation succeeded.
              if(errorCode!=GL_NO_ERROR)
                {
                cout<<"after try to load the texture";
                cout<<" ERROR (x"<<hex<<errorCode<<") "<<dec;
                cout<<vtkOpenGLGPUVolumeRayCastMapper::OpenGLErrorMessage(static_cast<unsigned int>(errorCode));
                cout<<endl;
                }
              // so far, so good but some cards don't report allocation error
              this->Loaded=textureSize[0]*textureSize[1]*
                textureSize[2]*vtkAbstractArray::GetDataTypeSize(scalarType)*
                scalars->GetNumberOfComponents()<=maxMemoryInBytes;
              if(this->Loaded)
                {
                // OK, we consider the allocation above succeeded...
                // If it actually didn't the only to fix it for the user
                // is to decrease the value of this->MaxMemoryInBytes.

                // enough memory! We can load the scalars!

                double bias=shift*scale;

                // we don't clamp to edge because for the computation of the
                // gradient on the border we need some external value.
                glTexParameterf(vtkgl::TEXTURE_3D,vtkgl::TEXTURE_WRAP_R,vtkgl::CLAMP_TO_EDGE);
                glTexParameterf(vtkgl::TEXTURE_3D,GL_TEXTURE_WRAP_S,vtkgl::CLAMP_TO_EDGE);
                glTexParameterf(vtkgl::TEXTURE_3D,GL_TEXTURE_WRAP_T,vtkgl::CLAMP_TO_EDGE);

                GLfloat borderColor[4]={0.0,0.0,0.0,0.0};

                glTexParameterfv(vtkgl::TEXTURE_3D,GL_TEXTURE_BORDER_COLOR, borderColor);

                if(needTypeConversion)
                  {
                  // Convert and send to the GPU, z-slice by z-slice.
                  // Allocate memory on the GPU (NULL data pointer with the right
                  // dimensions)
                  // Here we are assuming that GL_ARB_texture_non_power_of_two is
                  // available
                  glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

                  // memory allocation is already done.

                  // Send the slices:
                  // allocate CPU memory for a slice.
                  sliceArray->SetNumberOfComponents(1); // FB TODO CHECK THAT
                  sliceArray->SetNumberOfTuples(textureSize[0]*textureSize[1]);

                  void *slicePtr=sliceArray->GetVoidPointer(0);
                  int k=0;
                  int kInc=(dim[0]-cellFlag)*(dim[1]-cellFlag);
                  int kOffset=(textureExtent[4]*(dim[1]-cellFlag)
                               +textureExtent[2])*(dim[0]-cellFlag)
                    +textureExtent[0];
                  while(k<textureSize[2])
                    {
                    int j=0;
                    int jOffset=0;
                    int jDestOffset=0;
                    while(j<textureSize[1])
                      {
                      i=0;
                      while(i<textureSize[0])
                        {
                        sliceArray->SetTuple1(jDestOffset+i,
                                              (scalars->GetTuple1(kOffset+jOffset
                                                                  +i)
                                               +shift)*scale);
                        ++i;
                        }
                      ++j;
                      jOffset+=dim[0]-cellFlag;
                      jDestOffset+=textureSize[0];
                      }

                    // Here we are assuming that GL_ARB_texture_non_power_of_two is
                    // available
                    vtkgl::TexSubImage3D(vtkgl::TEXTURE_3D, 0,
                                         0,0,k,
                                         textureSize[0],textureSize[1],
                                         1, // depth is 1, not 0!
                                         format,type, slicePtr);
                    ++k;
                    kOffset+=kInc;
                    }
                  sliceArray->Delete();
                  }
                else
                  {
                  // One chunk of data to the GPU.
                  // It works for the whole volume or for a subvolume.
                  // Here we are assuming that GL_ARB_texture_non_power_of_two is
                  // available

                  //  make sure any previous OpenGL call is executed and will not
                  // be disturbed by our PixelTransfer value
                  glFinish();
                  glPixelTransferf(GL_RED_SCALE,static_cast<GLfloat>(scale));
                  glPixelTransferf(GL_RED_BIAS,static_cast<GLfloat>(bias));
                  glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

                  if(!(textureExtent[1]-textureExtent[0]+cellFlag==dim[0]))
                    {
                    glPixelStorei(GL_UNPACK_ROW_LENGTH,dim[0]-cellFlag);
                    }
                  if(!(textureExtent[3]-textureExtent[2]+cellFlag==dim[1]))
                    {
                    glPixelStorei(vtkgl::UNPACK_IMAGE_HEIGHT_EXT,
                                  dim[1]-cellFlag);
                    }
                  void *dataPtr=scalars->GetVoidPointer(
                    ((textureExtent[4]*(dim[1]-cellFlag)+textureExtent[2])
                     *(dim[0]-cellFlag)+textureExtent[0])
                    *scalars->GetNumberOfComponents());

                  if(1) // !this->SupportsPixelBufferObjects)
                    {
                    vtkgl::TexImage3D(vtkgl::TEXTURE_3D, 0, internalFormat,
                                      textureSize[0],textureSize[1],textureSize[2],
                                      0,format,type,dataPtr);
                    }
                  else
                    {
                    GLuint pbo=0;
                    vtkgl::GenBuffers(1,&pbo);
                    vtkOpenGLStaticCheckErrorMacro("genbuffer");
                    vtkgl::BindBuffer(vtkgl::PIXEL_UNPACK_BUFFER,pbo);
                    vtkOpenGLStaticCheckErrorMacro("binbuffer");
                    vtkgl::GLsizeiptr texSize=
                      textureSize[0]*textureSize[1]*textureSize[2]*
                      vtkAbstractArray::GetDataTypeSize(scalarType)*
                      scalars->GetNumberOfComponents();
                    vtkgl::BufferData(vtkgl::PIXEL_UNPACK_BUFFER,texSize,dataPtr,
                                      vtkgl::STREAM_DRAW);
                    vtkOpenGLStaticCheckErrorMacro("bufferdata");
                    vtkgl::TexImage3D(vtkgl::TEXTURE_3D, 0, internalFormat,
                                      textureSize[0],textureSize[1],textureSize[2],
                                      0,format,type,0);
                    vtkOpenGLStaticCheckErrorMacro("teximage3d");
                    vtkgl::BindBuffer(vtkgl::PIXEL_UNPACK_BUFFER,0);
                    vtkOpenGLStaticCheckErrorMacro("bindbuffer to 0");
                    vtkgl::DeleteBuffers(1,&pbo);
                    }
                  vtkOpenGLStaticCheckErrorMacro("3d texture is too large2");
                  // make sure TexImage3D is executed with our PixelTransfer mode
                  glFinish();
                  // Restore the default values.
                  glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
                  glPixelStorei(vtkgl::UNPACK_IMAGE_HEIGHT_EXT,0);
                  glPixelTransferf(GL_RED_SCALE,1.0);
                  glPixelTransferf(GL_RED_BIAS,0.0);
                  }
                this->LoadedCellFlag=cellFlag;
                i=0;
                while(i<6)
                  {
                  this->LoadedExtent[i]=textureExtent[i];
                  ++i;
                  }

                double spacing[3];
                double origin[3];
                input->GetSpacing(spacing);
                input->GetOrigin(origin);
                int swapBounds[3];
                swapBounds[0]=(spacing[0]<0);
                swapBounds[1]=(spacing[1]<0);
                swapBounds[2]=(spacing[2]<0);

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
                this->LoadedTableRange[0]=tableRange[0];
                this->LoadedTableRange[1]=tableRange[1];
                modified=true;
                } // if enough memory
              else
                {
                }
              } //load fail with out of memory
            else
              {
              }
            } // proxy ok
          else
            { // proxy failed
            }
          }
        else
          {
          // out of therical limitationa
          }
        } // if obsolete

      if(this->Loaded &&
         (needUpdate || modified ||
          linearInterpolation!=this->LinearInterpolation))
        {
        this->LinearInterpolation=linearInterpolation;
        if(this->LinearInterpolation)
          {
          glTexParameterf(vtkgl::TEXTURE_3D,GL_TEXTURE_MIN_FILTER,
                          GL_LINEAR);
          glTexParameterf(vtkgl::TEXTURE_3D,GL_TEXTURE_MAG_FILTER,
                          GL_LINEAR);
          }
        else
          {
          glTexParameterf(vtkgl::TEXTURE_3D,GL_TEXTURE_MIN_FILTER,
                          GL_NEAREST );
          glTexParameterf(vtkgl::TEXTURE_3D,GL_TEXTURE_MAG_FILTER,
                          GL_NEAREST );
          }
        modified=true;
        }
      if(modified)
        {
        this->BuildTime.Modified();
        }
    vtkOpenGLStaticCheckErrorMacro("failed after Update");
    }

  double *GetLoadedBounds()
    {
      assert("pre: loaded" && this->Loaded);
      return this->LoadedBounds;
    }

  vtkIdType *GetLoadedExtent()
    {
      assert("pre: loaded" && this->Loaded);
      return this->LoadedExtent;
    }

  int GetLoadedCellFlag()
    {
      assert("pre: loaded" && this->Loaded);
      return this->LoadedCellFlag;
    }

  bool IsLoaded()
    {
      return this->Loaded;
    }

  bool GetSupports_GL_ARB_texture_float()
    {
      return this->Supports_GL_ARB_texture_float;
    }

  void SetSupports_GL_ARB_texture_float(bool value)
    {
      this->Supports_GL_ARB_texture_float=value;
    }

protected:
  GLuint TextureId;
  vtkTimeStamp BuildTime;
  double LoadedBounds[6];
  vtkIdType LoadedExtent[6];
  int LoadedCellFlag;
  bool Loaded;
  bool LinearInterpolation;
  bool Supports_GL_ARB_texture_float;
  double LoadedTableRange[2];
};


//-----------------------------------------------------------------------------
class vtkKWMask
{
public:
  vtkKWMask()
    {
      this->TextureId=0;
      this->Loaded=false;
      this->LoadedExtent[0]=VTK_INT_MAX;
      this->LoadedExtent[1]=VTK_INT_MIN;
      this->LoadedExtent[2]=VTK_INT_MAX;
      this->LoadedExtent[3]=VTK_INT_MIN;
      this->LoadedExtent[4]=VTK_INT_MAX;
      this->LoadedExtent[5]=VTK_INT_MIN;
    }
  ~vtkKWMask()
    {
      if(this->TextureId!=0)
        {
        glDeleteTextures(1,&this->TextureId);
        vtkOpenGLStaticCheckErrorMacro("failed at glDeleteTextures");
        this->TextureId=0;
        }
    }

  vtkTimeStamp GetBuildTime()
    {
      return this->BuildTime;
    }

  // \pre vtkgl::ActiveTexture(vtkgl::TEXTURE7) has to be called first.
  void Bind()
    {
      assert("pre: uptodate" && this->Loaded);
      glBindTexture(vtkgl::TEXTURE_3D,this->TextureId);
      vtkOpenGLStaticCheckErrorMacro("failed at glBindTexture");
    }

  // \pre vtkgl::ActiveTexture(vtkgl::TEXTURE7) has to be called first.
  void Update(vtkImageData *input,
              int cellFlag,
              int textureExtent[6],
              int scalarMode,
              int arrayAccessMode,
              int arrayId,
              const char *arrayName,
              vtkIdType maxMemoryInBytes)
    {
      vtkOpenGLClearErrorMacro();

      bool needUpdate=false;
      bool modified=false;
      if(this->TextureId==0)
        {
        glGenTextures(1,&this->TextureId);
        needUpdate=true;
        }
      glBindTexture(vtkgl::TEXTURE_3D,this->TextureId);

      int obsolete=needUpdate || !this->Loaded
        || input->GetMTime()>this->BuildTime;
      if(!obsolete)
        {
        obsolete=cellFlag!=this->LoadedCellFlag;
        int i=0;
        while(!obsolete && i<6)
          {
          obsolete=obsolete || this->LoadedExtent[i]>textureExtent[i];
          ++i;
          obsolete=obsolete || this->LoadedExtent[i]<textureExtent[i];
          ++i;
          }
        }

      if(obsolete)
        {
        this->Loaded=false;
        int dim[3];
        input->GetDimensions(dim);

        vtkDataArray *scalars=
          vtkAbstractMapper::GetScalars(input,scalarMode,arrayAccessMode,
                                        arrayId,arrayName,
                                        this->LoadedCellFlag);

        // DONT USE GetScalarType() or GetNumberOfScalarComponents() on
        // ImageData as it deals only with point data...

        int scalarType=scalars->GetDataType();
        if(scalarType!=VTK_UNSIGNED_CHAR)
          {
          cout <<"mask should be VTK_UNSIGNED_CHAR." << endl;
          }
        if(scalars->GetNumberOfComponents()!=1)
          {
          cout <<"mask should be a one-component scalar field." << endl;
          }

        GLint internalFormat=GL_ALPHA8;
        GLenum format=GL_ALPHA;
        GLenum type=GL_UNSIGNED_BYTE;

        // Enough memory?
        int textureSize[3];
        int i=0;
        while(i<3)
          {
          textureSize[i]=textureExtent[2*i+1]-textureExtent[2*i]+1;
          ++i;
          }

        GLint width;
        glGetIntegerv(vtkgl::MAX_3D_TEXTURE_SIZE,&width);
        this->Loaded=textureSize[0]<=width && textureSize[1]<=width
          && textureSize[2]<=width;
        if(this->Loaded)
          {
          // so far, so good. the texture size is theorically small enough
          // for OpenGL

          vtkgl::TexImage3D(vtkgl::PROXY_TEXTURE_3D,0,internalFormat,
                            textureSize[0],textureSize[1],textureSize[2],0,
                            format,type,0);
          glGetTexLevelParameteriv(vtkgl::PROXY_TEXTURE_3D,0,GL_TEXTURE_WIDTH,
                                   &width);

          this->Loaded=width!=0;
          if(this->Loaded)
            {
            // so far, so good but some cards always succeed with a proxy texture
            // let's try to actually allocate..

            vtkgl::TexImage3D(vtkgl::TEXTURE_3D,0,internalFormat,textureSize[0],
                              textureSize[1],textureSize[2],0,format,type,0);
            GLenum errorCode=glGetError();
            this->Loaded=errorCode!=GL_OUT_OF_MEMORY;
            if(this->Loaded)
              {
              // so far, so good, actual allocation succeeded.
              if(errorCode!=GL_NO_ERROR)
                {
                cout<<"after try to load the texture";
                cout<<" ERROR (x"<<hex<<errorCode<<") "<<dec;
                cout<<vtkOpenGLGPUVolumeRayCastMapper::OpenGLErrorMessage(static_cast<unsigned int>(errorCode));
                cout<<endl;
                }
              // so far, so good but some cards don't report allocation error
              this->Loaded=textureSize[0]*textureSize[1]*
                textureSize[2]*vtkAbstractArray::GetDataTypeSize(scalarType)*
                scalars->GetNumberOfComponents()<=maxMemoryInBytes;
              if(this->Loaded)
                {
                // OK, we consider the allocation above succeeded...
                // If it actually didn't the only to fix it for the user
                // is to decrease the value of this->MaxMemoryInBytes.

                // enough memory! We can load the scalars!

                // we don't clamp to edge because for the computation of the
                // gradient on the border we need some external value.
                glTexParameterf(vtkgl::TEXTURE_3D,vtkgl::TEXTURE_WRAP_R,vtkgl::CLAMP_TO_EDGE);
                glTexParameterf(vtkgl::TEXTURE_3D,GL_TEXTURE_WRAP_S,vtkgl::CLAMP_TO_EDGE);
                glTexParameterf(vtkgl::TEXTURE_3D,GL_TEXTURE_WRAP_T,vtkgl::CLAMP_TO_EDGE);

                GLfloat borderColor[4]={0.0,0.0,0.0,0.0};

                glTexParameterfv(vtkgl::TEXTURE_3D,GL_TEXTURE_BORDER_COLOR, borderColor);

                glPixelTransferf(GL_ALPHA_SCALE,1.0);
                glPixelTransferf(GL_ALPHA_BIAS,0.0);
                glPixelStorei(GL_UNPACK_ALIGNMENT,1);

                if(!(textureExtent[1]-textureExtent[0]+cellFlag==dim[0]))
                  {
                  glPixelStorei(GL_UNPACK_ROW_LENGTH,dim[0]-cellFlag);
                  }
                if(!(textureExtent[3]-textureExtent[2]+cellFlag==dim[1]))
                  {
                  glPixelStorei(vtkgl::UNPACK_IMAGE_HEIGHT_EXT,
                                dim[1]-cellFlag);
                  }
                void *dataPtr=scalars->GetVoidPointer(
                  ((textureExtent[4]*(dim[1]-cellFlag)+textureExtent[2])
                   *(dim[0]-cellFlag)+textureExtent[0])
                  *scalars->GetNumberOfComponents());

                vtkgl::TexImage3D(vtkgl::TEXTURE_3D, 0, internalFormat,
                                  textureSize[0],textureSize[1],textureSize[2],
                                  0,format,type,dataPtr);

                // Restore the default values.
                glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
                glPixelStorei(vtkgl::UNPACK_IMAGE_HEIGHT_EXT,0);
                glPixelTransferf(GL_ALPHA_SCALE,1.0);
                glPixelTransferf(GL_ALPHA_BIAS,0.0);

                this->LoadedCellFlag=cellFlag;
                i=0;
                while(i<6)
                  {
                  this->LoadedExtent[i]=textureExtent[i];
                  ++i;
                  }

                double spacing[3];
                double origin[3];
                input->GetSpacing(spacing);
                input->GetOrigin(origin);
                int swapBounds[3];
                swapBounds[0]=(spacing[0]<0);
                swapBounds[1]=(spacing[1]<0);
                swapBounds[2]=(spacing[2]<0);

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
              else
                {
                }
              } //load fail with out of memory
            else
              {
              }
            } // proxy ok
          else
            { // proxy failed
            }
          }
        else
          {
          // out of therical limitationa
          }
        } // if obsolete

      if(this->Loaded && (needUpdate || modified))
        {
        glTexParameterf(vtkgl::TEXTURE_3D,GL_TEXTURE_MIN_FILTER,
                        GL_NEAREST );
        glTexParameterf(vtkgl::TEXTURE_3D,GL_TEXTURE_MAG_FILTER,
                        GL_NEAREST );
        modified=true;
        }
      if(modified)
        {
        this->BuildTime.Modified();
        }
      vtkOpenGLStaticCheckErrorMacro("failed after Update");
    }

  double *GetLoadedBounds()
    {
      assert("pre: loaded" && this->Loaded);
      return this->LoadedBounds;
    }

  vtkIdType *GetLoadedExtent()
    {
      assert("pre: loaded" && this->Loaded);
      return this->LoadedExtent;
    }

  int GetLoadedCellFlag()
    {
      assert("pre: loaded" && this->Loaded);
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


//-----------------------------------------------------------------------------
// Display the status of the current framebuffer on the standard output.
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::CheckFrameBufferStatus()
{
  GLenum status;
  status = vtkgl::CheckFramebufferStatusEXT(vtkgl::FRAMEBUFFER_EXT);
  switch(status)
    {
    case 0:
      cout << "call to vtkgl::CheckFramebufferStatusEXT generates an error."
           << endl;
      break;
    case vtkgl::FRAMEBUFFER_COMPLETE_EXT:
      break;
    case vtkgl::FRAMEBUFFER_UNSUPPORTED_EXT:
      cout << "framebuffer is unsupported" << endl;
      break;
    case vtkgl::FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
      cout << "framebuffer has an attachment error"<<endl;
      break;
    case vtkgl::FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
      cout << "framebuffer has a missing attachment"<<endl;
      break;
    case vtkgl::FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
      cout << "framebuffer has bad dimensions"<<endl;
      break;
    case vtkgl::FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
      cout << "framebuffer has bad formats"<<endl;
      break;
    case vtkgl::FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
      cout << "framebuffer has bad draw buffer"<<endl;
      break;
    case vtkgl::FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
      cout << "framebuffer has bad read buffer"<<endl;
      break;
    default:
      cout << "Unknown framebuffer status=0x" << hex<< status << dec << endl;
    }
  // DO NOT REMOVE THE FOLLOWING COMMENTED LINE. FOR DEBUGGING PURPOSE.
#ifdef APPLE_SNOW_LEOPARD_BUG
  this->DisplayFrameBufferAttachments();
  this->DisplayReadAndDrawBuffers();
#endif
}

//-----------------------------------------------------------------------------
vtkStdString vtkOpenGLGPUVolumeRayCastMapper::BufferToString(int buffer)
{
  vtkStdString result;
  vtksys_ios::ostringstream ost;

  GLint size;

  GLint b=static_cast<GLint>(buffer);
  switch(b)
    {
    case GL_NONE:
      ost << "GL_NONE";
      break;
    case GL_FRONT_LEFT:
      ost << "GL_FRONT_LEFT";
      break;
    case GL_FRONT_RIGHT:
      ost << "GL_FRONT_RIGHT";
      break;
    case GL_BACK_LEFT:
      ost << "GL_BACK_LEFT";
      break;
    case GL_BACK_RIGHT:
      ost << "GL_BACK_RIGHT";
      break;
    case GL_FRONT:
      ost << "GL_FRONT";
      break;
    case GL_BACK:
      ost << "GL_BACK";
      break;
    case GL_LEFT:
      ost << "GL_LEFT";
      break;
    case GL_RIGHT:
      ost << "GL_RIGHT";
      break;
    case GL_FRONT_AND_BACK:
      ost << "GL_FRONT_AND_BACK";
      break;
    default:
      glGetIntegerv(GL_AUX_BUFFERS,&size);
      if(buffer>=GL_AUX0 && buffer<(GL_AUX0+size))
        {
        ost << "GL_AUX" << (buffer-GL_AUX0);
        }
      else
        {
        glGetIntegerv(vtkgl::MAX_COLOR_ATTACHMENTS_EXT,&size);
        if(static_cast<GLuint>(buffer)>=vtkgl::COLOR_ATTACHMENT0_EXT &&
           static_cast<GLuint>(buffer)<
           (vtkgl::COLOR_ATTACHMENT0_EXT+static_cast<GLuint>(size)))
          {
          ost << "GL_COLOR_ATTACHMENT"
              << (static_cast<GLuint>(buffer)-vtkgl::COLOR_ATTACHMENT0_EXT)
              << "_EXT";
          }
        else
          {
          ost << "unknown color buffer type=0x"<<hex<<buffer<<dec;
          }
        }
      break;
    }

  result=ost.str();
  return result;
}

// ----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::DisplayReadAndDrawBuffers()
{
  GLint value;
  glGetIntegerv(vtkgl::MAX_DRAW_BUFFERS,&value);
  GLenum max=static_cast<GLenum>(value);

  vtkStdString s;
  GLenum i=0;
  while(i<max)
    {
    glGetIntegerv(vtkgl::DRAW_BUFFER0+i,&value);
    s=this->BufferToString(static_cast<int>(value));
    cout << "draw buffer " << i << "=" << s << endl;
    ++i;
    }

  glGetIntegerv(GL_READ_BUFFER,&value);
  s=this->BufferToString(static_cast<int>(value));
  cout << "read buffer=" << s << endl;
}

// ----------------------------------------------------------------------------
// Description:
// Display all the attachments of the current framebuffer object.
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::DisplayFrameBufferAttachments()
{
  GLint framebufferBinding;
  glGetIntegerv(vtkgl::FRAMEBUFFER_BINDING_EXT,&framebufferBinding);
  this->PrintError("after getting FRAMEBUFFER_BINDING_EXT");
  if(framebufferBinding==0)
    {
    cout<<"Current framebuffer is bind to the system one"<<endl;
    }
  else
    {
    cout<<"Current framebuffer is bind to framebuffer object "
        <<framebufferBinding<<endl;

    GLint value;
    glGetIntegerv(vtkgl::MAX_COLOR_ATTACHMENTS_EXT,&value);
    GLenum maxColorAttachments=static_cast<GLenum>(value);
    this->PrintError("after getting MAX_COLOR_ATTACHMENTS_EXT");
    GLenum i=0;
    while(i<maxColorAttachments)
      {
      cout<<"color attachement "<<i<<":"<<endl;
      this->DisplayFrameBufferAttachment(vtkgl::COLOR_ATTACHMENT0_EXT+i);
      ++i;
      }
    cout<<"depth attachement :"<<endl;
    this->DisplayFrameBufferAttachment(vtkgl::DEPTH_ATTACHMENT_EXT);
    cout<<"stencil attachement :"<<endl;
    this->DisplayFrameBufferAttachment(vtkgl::STENCIL_ATTACHMENT_EXT);
    }
}

// ----------------------------------------------------------------------------
// Description:
// Display a given attachment for the current framebuffer object.
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::DisplayFrameBufferAttachment(
  unsigned int uattachment)
{
  GLenum attachment=static_cast<GLenum>(uattachment);

  GLint params;
  vtkgl::GetFramebufferAttachmentParameterivEXT(
    vtkgl::FRAMEBUFFER_EXT,attachment,
    vtkgl::FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT,&params);

  this->PrintError("after getting FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT");
  switch(params)
    {
    case GL_NONE:
      cout<<" this attachment is empty"<<endl;
      break;
    case GL_TEXTURE:
      vtkgl::GetFramebufferAttachmentParameterivEXT(
        vtkgl::FRAMEBUFFER_EXT,attachment,
        vtkgl::FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT,&params);
      this->PrintError("after getting FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT");
      cout<<" this attachment is a texture with name: "<<params<<endl;
      vtkgl::GetFramebufferAttachmentParameterivEXT(
        vtkgl::FRAMEBUFFER_EXT,attachment,
        vtkgl::FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT,&params);
      this->PrintError(
        "after getting FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT");
      cout<<" its mipmap level is: "<<params<<endl;
      vtkgl::GetFramebufferAttachmentParameterivEXT(
        vtkgl::FRAMEBUFFER_EXT,attachment,
        vtkgl::FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT,&params);
       this->PrintError(
         "after getting FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT");
      if(params==0)
        {
        cout<<" this is not a cube map texture."<<endl;
        }
      else
        {
        cout<<" this is a cube map texture and the image is contained in face "
            <<params<<endl;
        }
       vtkgl::GetFramebufferAttachmentParameterivEXT(
         vtkgl::FRAMEBUFFER_EXT,attachment,
         vtkgl::FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT,&params);
        this->PrintError(
          "after getting FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT");
      if(params==0)
        {
        cout<<" this is not 3D texture."<<endl;
        }
      else
        {
        cout<<" this is a 3D texture and the zoffset of the attached image is "
            <<params<<endl;
        }
      break;
    case vtkgl::RENDERBUFFER_EXT:
      cout<<" this attachment is a renderbuffer"<<endl;
      vtkgl::GetFramebufferAttachmentParameterivEXT(
        vtkgl::FRAMEBUFFER_EXT,attachment,
        vtkgl::FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT,&params);
      this->PrintError("after getting FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT");
      cout<<" this attachment is a renderbuffer with name: "<<params<<endl;

      vtkgl::BindRenderbufferEXT(vtkgl::RENDERBUFFER_EXT,
                                 static_cast<GLuint>(params));
      this->PrintError(
        "after getting binding the current RENDERBUFFER_EXT to params");

      vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT,
                                           vtkgl::RENDERBUFFER_WIDTH_EXT,
                                           &params);
      this->PrintError("after getting RENDERBUFFER_WIDTH_EXT");
      cout<<" renderbuffer width="<<params<<endl;
      vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT,
                                           vtkgl::RENDERBUFFER_HEIGHT_EXT,
                                           &params);
      this->PrintError("after getting RENDERBUFFER_HEIGHT_EXT");
      cout<<" renderbuffer height="<<params<<endl;
      vtkgl::GetRenderbufferParameterivEXT(
        vtkgl::RENDERBUFFER_EXT,vtkgl::RENDERBUFFER_INTERNAL_FORMAT_EXT,
        &params);
      this->PrintError("after getting RENDERBUFFER_INTERNAL_FORMAT_EXT");

      cout<<" renderbuffer internal format=0x"<< hex<<params<<dec<<endl;

      vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT,
                                           vtkgl::RENDERBUFFER_RED_SIZE_EXT,
                                           &params);
      this->PrintError("after getting RENDERBUFFER_RED_SIZE_EXT");
      cout<<" renderbuffer actual resolution for the red component="<<params
          <<endl;
      vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT,
                                           vtkgl::RENDERBUFFER_GREEN_SIZE_EXT,
                                           &params);
      this->PrintError("after getting RENDERBUFFER_GREEN_SIZE_EXT");
      cout<<" renderbuffer actual resolution for the green component="<<params
          <<endl;
      vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT,
                                           vtkgl::RENDERBUFFER_BLUE_SIZE_EXT,
                                           &params);
      this->PrintError("after getting RENDERBUFFER_BLUE_SIZE_EXT");
      cout<<" renderbuffer actual resolution for the blue component="<<params
          <<endl;
      vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT,
                                           vtkgl::RENDERBUFFER_ALPHA_SIZE_EXT,
                                           &params);
      this->PrintError("after getting RENDERBUFFER_ALPHA_SIZE_EXT");
      cout<<" renderbuffer actual resolution for the alpha component="<<params
          <<endl;
      vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT,
                                           vtkgl::RENDERBUFFER_DEPTH_SIZE_EXT,
                                           &params);
      this->PrintError("after getting RENDERBUFFER_DEPTH_SIZE_EXT");
      cout<<" renderbuffer actual resolution for the depth component="<<params
          <<endl;
      vtkgl::GetRenderbufferParameterivEXT(
        vtkgl::RENDERBUFFER_EXT,vtkgl::RENDERBUFFER_STENCIL_SIZE_EXT,&params);
      this->PrintError("after getting RENDERBUFFER_STENCIL_SIZE_EXT");
      cout<<" renderbuffer actual resolution for the stencil component="
          <<params<<endl;
      break;
    default:
      cout<<" unexcepted value."<<endl;
      break;
    }
}

//-----------------------------------------------------------------------------
// Return a string matching the OpenGL errorCode. The returned string will
// not be null.
//-----------------------------------------------------------------------------
const char *vtkOpenGLGPUVolumeRayCastMapper::OpenGLErrorMessage(
  unsigned int errorCode)
{
  const char *result;
  switch(static_cast<GLenum>(errorCode))
    {
    case GL_NO_ERROR:
      result="No error";
      break;
    case GL_INVALID_ENUM:
      result="Invalid enum";
      break;
    case GL_INVALID_VALUE:
      result="Invalid value";
      break;
    case GL_INVALID_OPERATION:
      result="Invalid operation";
      break;
    case GL_STACK_OVERFLOW:
      result="stack overflow";
      break;
    case GL_STACK_UNDERFLOW:
      result="stack underflow";
      break;
    case GL_OUT_OF_MEMORY:
      result="out of memory";
      break;
    case vtkgl::TABLE_TOO_LARGE:
      // GL_ARB_imaging
      result="Table too large";
      break;
    case vtkgl::INVALID_FRAMEBUFFER_OPERATION_EXT:
      // GL_EXT_framebuffer_object, 310
      result="invalid framebuffer operation ext";
      break;
    case vtkgl::TEXTURE_TOO_LARGE_EXT:
      // GL_EXT_texture
      result="Texture too large";
      break;
    default:
      result="unknown error";
    }
  assert("post: result_exists" && result!=0);
  return result;
}

//-----------------------------------------------------------------------------
// Display headerMessage on the standard output and the last OpenGL error
// message if any.
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::PrintError(const char *headerMessage)
{
  GLenum errorCode=glGetError();
  if(errorCode!=GL_NO_ERROR)
    {
    if ( headerMessage )
      {
      cout<<headerMessage;
      }
    cout<<" ERROR (x"<<hex<<errorCode<<") "<<dec;
    cout<<OpenGLErrorMessage(static_cast<unsigned int>(errorCode));
    cout<<endl;
    }
}

//-----------------------------------------------------------------------------
// Construct a new vtkOpenGLGPUVolumeRayCastMapper with default values
//-----------------------------------------------------------------------------
vtkOpenGLGPUVolumeRayCastMapper::vtkOpenGLGPUVolumeRayCastMapper()
{
  this->UnsupportedRequiredExtensions=0;
  this->OpenGLObjectsCreated=0;
  this->LoadExtensionsSucceeded=0;
  this->NumberOfFrameBuffers=0;

  // up to 2 frame buffer 2D textures (left/right)
  // 1 dataset 3D texture
  // 1 colormap 1D texture
  // 1 opacitymap 1d texture
  // 1 grabbed depth buffer 2d texture
  int i=0;
  while(i<vtkOpenGLGPUVolumeRayCastMapperNumberOfTextureObjects)
    {
    this->TextureObjects[i]=0;
    ++i;
    }

  this->DepthRenderBufferObject=0;
  this->FrameBufferObject=0;

  for ( int j = 0; j < 8; j++ )
    {
    for (i = 0; i < 3; i++ )
      {
      this->BoundingBox[j][i] = 0.0;
      }
    }

  this->LastSize[0]=0;
  this->LastSize[1]=0;

  this->ReductionFactor = 1.0;

  this->Supports_GL_ARB_texture_float=0;
  this->SupportsPixelBufferObjects=0;

  i=0;
  while(i<3)
    {
    this->TempMatrix[i]=vtkMatrix4x4::New();
    ++i;
    }

  this->ErrorLine=0;
  this->ErrorColumn=0;
  this->ErrorString=0;

  this->LastParallelProjection=
    vtkOpenGLGPUVolumeRayCastMapperProjectionNotInitialized;
  this->LastRayCastMethod=
    vtkOpenGLGPUVolumeRayCastMapperMethodNotInitialized;
  this->LastCroppingMode=
    vtkOpenGLGPUVolumeRayCastMapperCroppingNotInitialized;
  this->LastComponent=
    vtkOpenGLGPUVolumeRayCastMapperComponentNotInitialized;
  this->LastShade=vtkOpenGLGPUVolumeRayCastMapperShadeNotInitialized;

  this->ClippedBoundingBox = NULL;

  this->SmallInput = NULL;

  this->MaxValueFrameBuffer=0;
  this->MaxValueFrameBuffer2=0;
  this->ReducedSize[0]=0;
  this->ReducedSize[1]=0;

  this->NumberOfCroppingRegions=0;

  this->PolyDataBoundingBox=0;
  this->Planes=0;
  this->NearPlane=0;
  this->Clip=0;
  this->Densify=0;
  this->InvVolumeMatrix=vtkMatrix4x4::New();

  this->SavedFrameBuffer=0;

  this->BoxSource=0;

  this->NoiseTexture=0;
  this->NoiseTextureSize=0;
  this->NoiseTextureId=0;

  this->IgnoreSampleDistancePerPixel=true;

  this->ScalarsTextures=new vtkMapDataArrayTextureId;
  this->MaskTextures=new vtkMapMaskTextureId;

  this->RGBTable=0;
  this->Mask1RGBTable=0;
  this->Mask2RGBTable=0;
  this->OpacityTables=0;

  this->CurrentScalar=0;
  this->CurrentMask=0;

  this->ActualSampleDistance=1.0;
  this->LastProgressEventTime=0.0; // date in seconds

  this->PreserveOrientation=true;

  this->Program=0;
  this->Main=0;
  this->Projection=0;
  this->Trace=0;
  this->CroppingShader=0;
  this->Component=0;
  this->Shade=0;
  this->ScaleBiasProgram=0;
}

//-----------------------------------------------------------------------------
// Destruct a vtkOpenGLGPUVolumeRayCastMapper - clean up any memory used
//-----------------------------------------------------------------------------
vtkOpenGLGPUVolumeRayCastMapper::~vtkOpenGLGPUVolumeRayCastMapper()
{
  if(this->UnsupportedRequiredExtensions!=0)
    {
    delete this->UnsupportedRequiredExtensions;
    this->UnsupportedRequiredExtensions=0;
    }
  int i=0;
  while(i<3)
    {
    this->TempMatrix[i]->Delete();
    this->TempMatrix[i]=0;
    ++i;
    }

  if(this->ErrorString!=0)
    {
    delete[] this->ErrorString;
    this->ErrorString=0;
    }

  if ( this->SmallInput )
    {
    this->SmallInput->UnRegister(this);
    }

  if(this->PolyDataBoundingBox!=0)
    {
    this->PolyDataBoundingBox->UnRegister(this);
    this->PolyDataBoundingBox=0;
    }
  if(this->Planes!=0)
    {
    this->Planes->UnRegister(this);
    this->Planes=0;
    }
  if(this->NearPlane!=0)
    {
    this->NearPlane->UnRegister(this);
    this->NearPlane=0;
    }
  if(this->Clip!=0)
    {
    this->Clip->UnRegister(this);
    this->Clip=0;
    }
  if(this->Densify!=0)
    {
    this->Densify->UnRegister(this);
    this->Densify=0;
    }

  if(this->BoxSource!=0)
    {
    this->BoxSource->UnRegister(this);
    this->BoxSource=0;
    }
  this->InvVolumeMatrix->UnRegister(this);
  this->InvVolumeMatrix=0;

  if(this->NoiseTexture!=0)
    {
    delete[] this->NoiseTexture;
    this->NoiseTexture=0;
    this->NoiseTextureSize=0;
    }

  if(this->ScalarsTextures!=0)
    {
    delete this->ScalarsTextures;
    this->ScalarsTextures=0;
    }

  if(this->MaskTextures!=0)
    {
    delete this->MaskTextures;
    this->MaskTextures=0;
    }

  if(this->Program!=0)
    {
    this->Program->Delete();
    }
  if(this->Main!=0)
    {
    this->Main->Delete();
    }
  if(this->Projection!=0)
    {
    this->Projection->Delete();
    }
  if(this->Trace!=0)
    {
    this->Trace->Delete();
    }
  if(this->CroppingShader!=0)
    {
    this->CroppingShader->Delete();
    }
  if(this->Component!=0)
    {
    this->Component->Delete();
    }
  if(this->Shade!=0)
    {
    this->Shade->Delete();
    }
  if(this->ScaleBiasProgram!=0)
    {
    this->ScaleBiasProgram->Delete();
    }
}

//-----------------------------------------------------------------------------
// Based on hardware and properties, we may or may not be able to
// render using 3D texture mapping. This indicates if 3D texture
// mapping is supported by the hardware, and if the other extensions
// necessary to support the specific properties are available.
//
//-----------------------------------------------------------------------------
int vtkOpenGLGPUVolumeRayCastMapper::IsRenderSupported(
  vtkRenderWindow *window,
  vtkVolumeProperty *vtkNotUsed(property))
{
  window->MakeCurrent();
  if(!this->LoadExtensionsSucceeded)
    {
    this->LoadExtensions(window);
    }
  if(!this->LoadExtensionsSucceeded)
    {
    vtkDebugMacro(
      "The following OpenGL extensions are required but not supported: "
      << (this->UnsupportedRequiredExtensions->Stream.str()).c_str());
    return 0;
    }
  return 1;
}

//-----------------------------------------------------------------------------
// Return if the required OpenGL extension `extensionName' is supported.
// If not, its name is added to the string of unsupported but required
// extensions.
// \pre extensions_exist: extensions!=0
// \pre extensionName_exists: extensionName!=0
//-----------------------------------------------------------------------------
int vtkOpenGLGPUVolumeRayCastMapper::TestRequiredExtension(
  vtkOpenGLExtensionManager *extensions,
  const char *extensionName)
{
  assert("pre: extensions_exist" && extensions!=0);
  assert("pre: extensionName_exists" && extensionName!=0);
  int result=extensions->ExtensionSupported(extensionName);

  if(!result)
    {
    if(this->LoadExtensionsSucceeded)
      {
      this->UnsupportedRequiredExtensions->Stream<<extensionName;
      this->LoadExtensionsSucceeded=0;
      }
    else
      {
      this->UnsupportedRequiredExtensions->Stream<<", "<<extensionName;
      }
    }
  return result;
}

//-----------------------------------------------------------------------------
// Attempt to load required and optional OpenGL extensions for the current
// context window. Variable LoadExtensionsSucceeded is set if all required
// extensions has been loaded. In addition, variable
// Supports_GL_ARB_texture_float is set if this extension has been loaded.
//
// Pre-conditions:
//   - window != NULL
//
// Post-conditions:
//   - this->LoadExtensionsSucceeded will be set to 0 or 1
//   - this->UnsupportedRequiredExtensions will have a message indicating
//     any failure codes
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::LoadExtensions(
  vtkRenderWindow *window)
{
  // We may already have a string stream for the unsupported extensions
  // from the last time this method was called. If so, delete it.
  if(this->UnsupportedRequiredExtensions!=0)
    {
    delete this->UnsupportedRequiredExtensions;
    }

  // Create a string stream to hold the unsupported extensions so we can
  // report something meaningful back
  this->UnsupportedRequiredExtensions =
    new vtkUnsupportedRequiredExtensionsStringStream;

  // It does not work on Apple OS X Snow Leopard with nVidia.
  // There is a bug in the OpenGL driver with an error in the
  // Cg compiler about an infinite loop.
#ifndef APPLE_SNOW_LEOPARD_BUG
 #ifdef __APPLE__
  this->UnsupportedRequiredExtensions->Stream<<
    " Disabled on Apple OS X Snow Leopard with nVidia.";
  this->LoadExtensionsSucceeded=0;
  return;
 #endif
#endif

  // Assume success
  this->LoadExtensionsSucceeded=1;

  // get the extension manager
  vtkOpenGLRenderWindow *context = vtkOpenGLRenderWindow::SafeDownCast(window);
  if (!context)
    {
    this->UnsupportedRequiredExtensions->Stream<<
      " Disabled because context is not a vtkOpenGLRederWindow.";
    this->LoadExtensionsSucceeded=0;
    return;
    }
  vtkOpenGLExtensionManager *extensions = context->GetExtensionManager();

  // mesa notes:
  // 8.0.0 -- missing some required extensions
  // 8.0.5 -- tests pass but there are invalid enum opengl errors reported (mesa bug)
  // 9.1.3 & 9.1.4 w/ OS Mesa -- GPURayCastCompositeShadeMask fails (mesa bug?) test disabled
  // 9.2.0 w/llvmpipe -- tests pass cleanly
  if ( (extensions->DriverIsMesa()
    && !(extensions->DriverGLRendererIsOSMesa() && extensions->DriverVersionAtLeast(9)))
    && !extensions->GetIgnoreDriverBugs("Mesa FBO bugs"))
    {
    this->UnsupportedRequiredExtensions->Stream<<
      " Disabled because of Mesa FBO bugs.";
    this->LoadExtensionsSucceeded=0;
    }

  // GL_ARB_draw_buffers requires OpenGL 1.3, so we must have OpenGL 1.3
  // We don't need to check for some extensions that become part of OpenGL
  // core after 1.3. Among them:
  //   - texture_3d is in core OpenGL since 1.2
  //   - texture_edge_clamp is in core OpenGL since 1.2
  //     (GL_SGIS_texture_edge_clamp or GL_EXT_texture_edge_clamp (nVidia) )
  //   -  multitexture is in core OpenGL since 1.3

  int supports_GL_1_3=extensions->ExtensionSupported("GL_VERSION_1_3");
  int supports_GL_2_0=0;

  // No 1.3 support - give up
  if(!supports_GL_1_3)
    {
    this->LoadExtensionsSucceeded=0;
    this->UnsupportedRequiredExtensions->Stream<<
      " OpenGL 1.3 is required but not supported";
    return;
    }

  // Check for 2.0 support
  supports_GL_2_0=extensions->ExtensionSupported("GL_VERSION_2_0");

  // Some extensions that are supported in 2.0, but if we don't
  // have 2.0 we'll need to check further
  int supports_shading_language_100     = 1;
  int supports_shader_objects           = 1;
  int supports_fragment_shader          = 1;
  int supports_texture_non_power_of_two = 1;
  int supports_draw_buffers             = 1;
  if(!supports_GL_2_0)
    {
    supports_shading_language_100=
      extensions->ExtensionSupported("GL_ARB_shading_language_100");
    supports_shader_objects=
      extensions->ExtensionSupported("GL_ARB_shader_objects");
    supports_fragment_shader=
      extensions->ExtensionSupported("GL_ARB_fragment_shader");
    supports_texture_non_power_of_two=
      extensions->ExtensionSupported("GL_ARB_texture_non_power_of_two");
    supports_draw_buffers=
      extensions->ExtensionSupported("GL_ARB_draw_buffers");
    }

  // We have to check for framebuffer objects
  int supports_GL_EXT_framebuffer_object=
    extensions->ExtensionSupported("GL_EXT_framebuffer_object" );

  // Find out if we have OpenGL 1.4 support
  int supports_GL_1_4=extensions->ExtensionSupported("GL_VERSION_1_4");

  // Find out if we have the depth texture ARB extension
  int supports_GL_ARB_depth_texture=
    extensions->ExtensionSupported("GL_ARB_depth_texture");

  // Depth textures are support if we either have OpenGL 1.4
  // or if the depth texture ARB extension is supported
  int supports_depth_texture =
    supports_GL_1_4 || supports_GL_ARB_depth_texture;

  // Now start adding messages to the UnsupportedRequiredExtensions string
  // Log message if shading language 100 is not supported
  if(!supports_shading_language_100)
    {
    this->UnsupportedRequiredExtensions->Stream<<
      " shading_language_100 (or OpenGL 2.0) is required but not supported";
    this->LoadExtensionsSucceeded=0;
    }
  else
    {
    // We can query the GLSL version, we need >=1.20
    const char *glsl_version=
      reinterpret_cast<const char *>(glGetString(vtkgl::SHADING_LANGUAGE_VERSION));
    int glslMajor, glslMinor;
    vtksys_ios::istringstream ist(glsl_version);
    ist >> glslMajor;
    char c;
    ist.get(c); // '.'
    ist >> glslMinor;
//sscanf(version, "%d.%d", &glslMajor, &glslMinor);
    if(glslMajor<1 || (glslMajor==1 && glslMinor<20))
      {
      this->LoadExtensionsSucceeded=0;
      }
    }

  // Log message if shader objects are not supported
  if(!supports_shader_objects)
    {
    this->UnsupportedRequiredExtensions->Stream<<
      " shader_objects (or OpenGL 2.0) is required but not supported";
    this->LoadExtensionsSucceeded=0;
    }

  // Log message if fragment shaders are not supported
  if(!supports_fragment_shader)
    {
    this->UnsupportedRequiredExtensions->Stream<<
      " fragment_shader (or OpenGL 2.0) is required but not supported";
    this->LoadExtensionsSucceeded=0;
    }

  // Log message if non power of two textures are not supported
  if(!supports_texture_non_power_of_two)
    {
    this->UnsupportedRequiredExtensions->Stream<<
      " texture_non_power_of_two (or OpenGL 2.0) is required but not "
                                               << "supported";
    this->LoadExtensionsSucceeded=0;
    }

  // Log message if draw buffers are not supported
  if(!supports_draw_buffers)
    {
    this->UnsupportedRequiredExtensions->Stream<<
      " draw_buffers (or OpenGL 2.0) is required but not supported";
    this->LoadExtensionsSucceeded=0;
    }

  // Log message if depth textures are not supported
  if(!supports_depth_texture)
    {
    this->UnsupportedRequiredExtensions->Stream<<
      " depth_texture (or OpenGL 1.4) is required but not supported";
    this->LoadExtensionsSucceeded=0;
    }

  // Log message if framebuffer objects are not supported
  if(!supports_GL_EXT_framebuffer_object)
    {
    this->UnsupportedRequiredExtensions->Stream<<
      " framebuffer_object is required but not supported";
    this->LoadExtensionsSucceeded=0;
    }

  // Have we succeeded so far? If not, just return.
  if(!this->LoadExtensionsSucceeded)
    {
    return;
    }

  // Now start loading the extensions
  // First load all 1.2 and 1.3 extensions (we know we
  // support at least up to 1.3)
  extensions->LoadExtension("GL_VERSION_1_2");
  extensions->LoadExtension("GL_VERSION_1_3");

  // Load the 2.0 extensions if supported
  if(supports_GL_2_0)
    {
    extensions->LoadExtension("GL_VERSION_2_0");
    }
  // Otherwise, we'll need to specifically load the
  // shader objects, fragment shader, and draw buffers
  // extensions
  else
    {
    extensions->LoadCorePromotedExtension("GL_ARB_shader_objects");
    extensions->LoadCorePromotedExtension("GL_ARB_fragment_shader");
    extensions->LoadCorePromotedExtension("GL_ARB_draw_buffers");
    }

  // Load the framebuffer object extension
  extensions->LoadExtension("GL_EXT_framebuffer_object");

  // Optional extension (does not fail if not present)
  // Load it if supported which will allow us to store
  // textures as floats
  this->Supports_GL_ARB_texture_float=
    extensions->ExtensionSupported("GL_ARB_texture_float" );
  if(this->Supports_GL_ARB_texture_float)
    {
    extensions->LoadExtension( "GL_ARB_texture_float" );
    }

  // Optional extension (does not fail if not present)
  // Used to minimize memory footprint when loading large 3D textures
  // of scalars.
  // VBO or 1.5 is required by PBO or 2.1
  int supports_GL_1_5=extensions->ExtensionSupported("GL_VERSION_1_5");
  int supports_vertex_buffer_object=supports_GL_1_5 ||
    extensions->ExtensionSupported("GL_ARB_vertex_buffer_object");
  int supports_GL_2_1=extensions->ExtensionSupported("GL_VERSION_2_1");
  this->SupportsPixelBufferObjects=supports_vertex_buffer_object &&
    (supports_GL_2_1 ||
     extensions->ExtensionSupported("GL_ARB_pixel_buffer_object"));

  if(this->SupportsPixelBufferObjects)
    {
    if(supports_GL_1_5)
      {
      extensions->LoadExtension("GL_VERSION_1_5");
      }
    else
      {
      extensions->LoadCorePromotedExtension("GL_ARB_vertex_buffer_object");
      }
    if(supports_GL_2_1)
      {
      extensions->LoadExtension("GL_VERSION_2_1");
      }
    else
      {
      extensions->LoadCorePromotedExtension("GL_ARB_pixel_buffer_object");
      }
    }

  // Ultimate test. Some old cards support OpenGL 2.0 but not while
  // statements in a fragment shader (example: nVidia GeForce FX 5200)
  // It does not fail when compiling each shader source but at linking
  // stage because the parser underneath only check for syntax during
  // compilation and the actual native code generation happens during
  // the linking stage.
  this->NumberOfCroppingRegions=1;
  this->BuildProgram(window,1,
                     vtkOpenGLGPUVolumeRayCastMapperMethodComposite,
                     vtkOpenGLGPUVolumeRayCastMapperShadeNo,
                     vtkOpenGLGPUVolumeRayCastMapperComponentOne);

  this->Program->SetPrintErrors(false);
  this->Program->Build();
  this->Program->SetPrintErrors(true);

  if(this->Program->GetLastBuildStatus()!=VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    this->LoadExtensionsSucceeded=0;
    this->UnsupportedRequiredExtensions->Stream<<
      " this card does not support while statements in fragment shaders.";
    }

  this->Program->ReleaseGraphicsResources();
  if(this->LastComponent!=
     vtkOpenGLGPUVolumeRayCastMapperComponentNotInitialized)
    {
    this->Program->GetShaders()->RemoveItem(this->Component);
    }
  if(this->LastShade!=
     vtkOpenGLGPUVolumeRayCastMapperShadeNotInitialized)
    {
    this->Program->GetShaders()->RemoveItem(this->Shade);
    }

  this->LastParallelProjection=
    vtkOpenGLGPUVolumeRayCastMapperProjectionNotInitialized;
  this->LastRayCastMethod=
    vtkOpenGLGPUVolumeRayCastMapperMethodNotInitialized;
  this->LastCroppingMode=
    vtkOpenGLGPUVolumeRayCastMapperCroppingNotInitialized;
  this->LastComponent=
    vtkOpenGLGPUVolumeRayCastMapperComponentNotInitialized;
  this->LastShade=vtkOpenGLGPUVolumeRayCastMapperShadeNotInitialized;
}

//-----------------------------------------------------------------------------
// Delete OpenGL objects.
// \post done: this->OpenGLObjectsCreated==0
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::ReleaseGraphicsResources(
  vtkWindow *window)
{
  if(this->OpenGLObjectsCreated)
    {
    window->MakeCurrent();
    vtkOpenGLClearErrorMacro();
    this->LastSize[0]=0;
    this->LastSize[1]=0;
    GLuint frameBufferObject=static_cast<GLuint>(this->FrameBufferObject);
    vtkgl::DeleteFramebuffersEXT(1,&frameBufferObject);
    GLuint depthRenderBufferObject=
      static_cast<GLuint>(this->DepthRenderBufferObject);
    vtkgl::DeleteRenderbuffersEXT(1,&depthRenderBufferObject);
    GLuint textureObjects[vtkOpenGLGPUVolumeRayCastMapperNumberOfTextureObjects];
    int i=0;
    while(i<(vtkOpenGLGPUVolumeRayCastMapperTextureObjectFrameBufferLeftFront+this->NumberOfFrameBuffers))
      {
      textureObjects[i]=static_cast<GLuint>(this->TextureObjects[i]);
      ++i;
      }
    glDeleteTextures(vtkOpenGLGPUVolumeRayCastMapperTextureObjectFrameBufferLeftFront+this->NumberOfFrameBuffers,textureObjects);

    if(this->MaxValueFrameBuffer!=0)
      {
      GLuint maxValueFrameBuffer=
        static_cast<GLuint>(this->MaxValueFrameBuffer);
      glDeleteTextures(1,&maxValueFrameBuffer);
      this->MaxValueFrameBuffer=0;
      }
    if(this->MaxValueFrameBuffer2!=0)
      {
      GLuint maxValueFrameBuffer2=
        static_cast<GLuint>(this->MaxValueFrameBuffer2);
      glDeleteTextures(1,&maxValueFrameBuffer2);
      this->MaxValueFrameBuffer2=0;
      }
    this->LastParallelProjection=
      vtkOpenGLGPUVolumeRayCastMapperProjectionNotInitialized;
    this->LastRayCastMethod=
      vtkOpenGLGPUVolumeRayCastMapperMethodNotInitialized;
    this->LastCroppingMode=
      vtkOpenGLGPUVolumeRayCastMapperCroppingNotInitialized;
    this->LastComponent=
      vtkOpenGLGPUVolumeRayCastMapperComponentNotInitialized;
    this->LastShade=vtkOpenGLGPUVolumeRayCastMapperShadeNotInitialized;
    this->OpenGLObjectsCreated=0;
    vtkOpenGLCheckErrorMacro("failed during ReleaseGraphicsResources");
    }

  if(this->NoiseTextureId!=0)
    {
    window->MakeCurrent();
    vtkOpenGLClearErrorMacro();
    GLuint noiseTextureObjects=static_cast<GLuint>(this->NoiseTextureId);
    glDeleteTextures(1,&noiseTextureObjects);
    this->NoiseTextureId=0;
    vtkOpenGLCheckErrorMacro("failed during ReleaseGraphicsResources");
    }

  if(this->ScalarsTextures!=0)
    {
    if(!this->ScalarsTextures->Map.empty())
      {
      std::map<vtkImageData *,vtkKWScalarField *>::iterator it=this->ScalarsTextures->Map.begin();
      while(it!=this->ScalarsTextures->Map.end())
        {
        vtkKWScalarField *texture=(*it).second;
        delete texture;
        ++it;
        }
      this->ScalarsTextures->Map.clear();
      }
    }

  if(this->MaskTextures!=0)
    {
    if(!this->MaskTextures->Map.empty())
      {
      std::map<vtkImageData *,vtkKWMask *>::iterator it=this->MaskTextures->Map.begin();
      while(it!=this->MaskTextures->Map.end())
        {
        vtkKWMask *texture=(*it).second;
        delete texture;
        ++it;
        }
      this->MaskTextures->Map.clear();
      }
    }

  if(this->RGBTable!=0)
    {
    delete this->RGBTable;
    this->RGBTable=0;
    }

  if(this->Mask1RGBTable!=0)
    {
    delete this->Mask1RGBTable;
    this->Mask1RGBTable=0;
    }

  if(this->Mask2RGBTable!=0)
    {
    delete this->Mask2RGBTable;
    this->Mask2RGBTable=0;
    }

  if(this->OpacityTables!=0)
    {
    delete this->OpacityTables;
    this->OpacityTables=0;
    }

  if(this->Program!=0)
    {
    this->Program->ReleaseGraphicsResources();
    }
   if(this->Main!=0)
    {
    this->Main->ReleaseGraphicsResources();
    }
  if(this->Projection!=0)
    {
    this->Projection->ReleaseGraphicsResources();
    }
  if(this->Trace!=0)
    {
    this->Trace->ReleaseGraphicsResources();
    }
  if(this->CroppingShader!=0)
    {
    this->CroppingShader->ReleaseGraphicsResources();
    }
  if(this->Component!=0)
    {
    this->Component->ReleaseGraphicsResources();
    }
  if(this->Shade!=0)
    {
    this->Shade->ReleaseGraphicsResources();
    }
  if(this->ScaleBiasProgram!=0)
    {
    this->ScaleBiasProgram->ReleaseGraphicsResources();
    }
}

//-----------------------------------------------------------------------------
// Create OpenGL objects such as textures, buffers and fragment program Ids.
// It only registers Ids, there is no actual initialization of textures or
// fragment program.
//
// Pre-conditions:
// This method assumes that this->LoadedExtensionsSucceeded is 1.
//
// Post-conditions:
// When this method completes successfully, this->OpenGLObjectsCreated
// will be 1.
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::CreateOpenGLObjects(vtkRenderer *ren)
{
  vtkOpenGLClearErrorMacro();

  GLint value;
  glGetIntegerv(vtkgl::FRAMEBUFFER_BINDING_EXT,&value);
  GLuint savedFrameBuffer=static_cast<GLuint>(value);

  if ( !this->OpenGLObjectsCreated )
    {
    // Initialize only if the objects haven't been created yet.

    // We need only two color buffers (ping-pong)
    this->NumberOfFrameBuffers=2;

    GLuint frameBufferObject;
    GLuint depthRenderBufferObject;


    // TODO: clean this up!
    // 2*Frame buffers(2d textures)+colorMap (1d texture) +dataset (3d texture)
    // + opacitymap (1d texture) + grabbed depthMap (2d texture)
    GLuint textureObjects[vtkOpenGLGPUVolumeRayCastMapperNumberOfTextureObjects];

    // Create the various objects we will need - one frame buffer
    // which will contain a render buffer for depth and a texture
    // for color.
    vtkgl::GenFramebuffersEXT(1, &frameBufferObject); // color
    vtkgl::GenRenderbuffersEXT(1, &depthRenderBufferObject); // depth
    int i=0;
    while(i<( vtkOpenGLGPUVolumeRayCastMapperTextureObjectFrameBufferLeftFront+this->NumberOfFrameBuffers))
      {
      textureObjects[i]=0;
      ++i;
      }

    // Frame buffers(2d textures)+colorMap (1d texture) +dataset (3d texture)
    // + opacity (1d texture)+grabbed depth buffer (2d texture)
    glGenTextures(vtkOpenGLGPUVolumeRayCastMapperTextureObjectFrameBufferLeftFront+this->NumberOfFrameBuffers,textureObjects);
    // Color buffers
    vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT,frameBufferObject);
    i = 0;
    while(i < this->NumberOfFrameBuffers)
      {
      glBindTexture(GL_TEXTURE_2D,textureObjects[vtkOpenGLGPUVolumeRayCastMapperTextureObjectFrameBufferLeftFront+i]);
      ++i;
      }

    // Save GL objects by static casting to standard C types. GL* types
    // are not allowed in VTK header files.
    this->FrameBufferObject=static_cast<unsigned int>(frameBufferObject);
    this->DepthRenderBufferObject=static_cast<unsigned int>(depthRenderBufferObject);
    i=0;
    while(i<(vtkOpenGLGPUVolumeRayCastMapperTextureObjectFrameBufferLeftFront+this->NumberOfFrameBuffers))
      {
      this->TextureObjects[i]=static_cast<unsigned int>(textureObjects[i]);
      ++i;
      }

    this->OpenGLObjectsCreated=1;

    } // endif OpenGLObjectsCreated

  int size[2];
  int i=0;

  ren->GetTiledSize(&size[0],&size[1]);
  this->SizeChanged = this->LastSize[0]!=size[0] || this->LastSize[1]!=size[1];

  GLenum errorCode=glGetError();
  while(i <this->NumberOfFrameBuffers && errorCode==GL_NO_ERROR)
    {
    if (this->SizeChanged)
      {
      glBindTexture(GL_TEXTURE_2D,static_cast<GLuint>(this->TextureObjects[vtkOpenGLGPUVolumeRayCastMapperTextureObjectFrameBufferLeftFront+i]));
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, vtkgl::CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, vtkgl::CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      // Here we are assuming that GL_ARB_texture_non_power_of_two is available
      if(this->Supports_GL_ARB_texture_float)
        {
        glTexImage2D(GL_TEXTURE_2D,0,vtkgl::RGBA16F_ARB,size[0],size[1],
                     0, GL_RGBA, GL_FLOAT, NULL );
        }
      else
        {
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16,size[0],size[1],
                     0, GL_RGBA, GL_FLOAT, NULL );
        }
      }

    errorCode=glGetError();
    ++i;
    }

  if (this->SizeChanged)
    {
    if(errorCode==GL_NO_ERROR)
      {
      // grabbed depth buffer
      glBindTexture(GL_TEXTURE_2D,static_cast<GLuint>(this->TextureObjects[vtkOpenGLGPUVolumeRayCastMapperTextureObjectDepthMap]));
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, vtkgl::CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, vtkgl::CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
      glTexParameteri(GL_TEXTURE_2D, vtkgl::DEPTH_TEXTURE_MODE, GL_LUMINANCE);
      glTexImage2D(GL_TEXTURE_2D, 0, vtkgl::DEPTH_COMPONENT32, size[0],size[1],
                   0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL );

      // Set up the depth render buffer

      GLint savedFrameBuffer2;
      glGetIntegerv(vtkgl::FRAMEBUFFER_BINDING_EXT,&savedFrameBuffer2);
      vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT,
          static_cast<GLuint>(this->FrameBufferObject));
      vtkgl::BindRenderbufferEXT(
        vtkgl::RENDERBUFFER_EXT,
        static_cast<GLuint>(this->DepthRenderBufferObject));
      vtkgl::RenderbufferStorageEXT(vtkgl::RENDERBUFFER_EXT,
                                    vtkgl::DEPTH_COMPONENT24,size[0],size[1]);
      vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT,
                static_cast<GLuint>(savedFrameBuffer2));
      errorCode=glGetError();
      if(errorCode==GL_NO_ERROR)
        {
        this->LastSize[0]=size[0];
        this->LastSize[1]=size[1];
        }
      }
    }


  // Bind the Frame buffer object and then attach 2D texture to the FBO
  vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT,
          static_cast<GLuint>(this->FrameBufferObject));
  vtkgl::FramebufferTexture2DEXT(vtkgl::FRAMEBUFFER_EXT,
                                 vtkgl::COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,
                                 static_cast<GLuint>(this->TextureObjects[vtkOpenGLGPUVolumeRayCastMapperTextureObjectFrameBufferLeftFront]),
                                 0);

  // Depth buffer
  vtkgl::BindRenderbufferEXT(vtkgl::RENDERBUFFER_EXT,
                             static_cast<GLuint>(this->DepthRenderBufferObject));

  vtkgl::FramebufferRenderbufferEXT(vtkgl::FRAMEBUFFER_EXT,
                                    vtkgl::DEPTH_ATTACHMENT_EXT,
                                    vtkgl::RENDERBUFFER_EXT,
                                    static_cast<GLuint>(this->DepthRenderBufferObject));

  // Restore default frame buffer.
  vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT,savedFrameBuffer);

  vtkOpenGLCheckErrorMacro("failed after CreateOpenGLObjects");
}


//-----------------------------------------------------------------------------
// Allocate memory on the GPU for the framebuffers according to the size of
// the window or reallocate if the size has changed. Return true if
// allocation succeeded.
// \pre ren_exists: ren!=0
// \pre opengl_objects_created: this->OpenGLObjectsCreated
// \post right_size: LastSize[]=window size.
//-----------------------------------------------------------------------------
int vtkOpenGLGPUVolumeRayCastMapper::AllocateFrameBuffers(vtkRenderer *ren)
{
  assert("pre: ren_exists" && ren!=0);
  assert("pre: opengl_objects_created" && this->OpenGLObjectsCreated);

  vtkOpenGLClearErrorMacro();

  int result=1;
  int size[2];
  ren->GetTiledSize(&size[0],&size[1]);

  const bool accumulativeBlendMode =
    (this->BlendMode==vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND ||
     this->BlendMode==vtkGPUVolumeRayCastMapper::MINIMUM_INTENSITY_BLEND ||
     this->BlendMode==vtkGPUVolumeRayCastMapper::ADDITIVE_BLEND);

  const bool needNewMaxValueBuffer =
    (this->MaxValueFrameBuffer == 0) && accumulativeBlendMode;

  GLint value;
  glGetIntegerv(vtkgl::FRAMEBUFFER_BINDING_EXT,&value);
  GLuint savedFrameBuffer=static_cast<GLuint>(value);

  if(needNewMaxValueBuffer)
    {
    // blend mode changed and need max value buffer.

    // create and bind second color buffer (we use only the red component
    // to store the max scalar). We can't use a one component color buffer
    // because all color buffer have to have the same format.

    // max scalar frame buffer
    GLuint maxValueFrameBuffer;
    glGenTextures(1,&maxValueFrameBuffer);
    this->MaxValueFrameBuffer=
      static_cast<unsigned int>(maxValueFrameBuffer);

    // max scalar frame buffer2
    GLuint maxValueFrameBuffer2;
    glGenTextures(1,&maxValueFrameBuffer2);
    this->MaxValueFrameBuffer2=
      static_cast<unsigned int>(maxValueFrameBuffer2);
    }
  else
    {
    if (this->MaxValueFrameBuffer && !accumulativeBlendMode)
      {
      // blend mode changed and does not need max value buffer anymore.

      vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT,
                               static_cast<GLuint>(this->FrameBufferObject));
      vtkgl::FramebufferTexture2DEXT(vtkgl::FRAMEBUFFER_EXT,
                                     vtkgl::COLOR_ATTACHMENT0_EXT+1,
                                     GL_TEXTURE_2D,0,0); // not scalar buffer
      vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT,savedFrameBuffer);

      GLuint maxValueFrameBuffer=
        static_cast<GLuint>(this->MaxValueFrameBuffer);
      glDeleteTextures(1,&maxValueFrameBuffer);
      this->MaxValueFrameBuffer=0;

      GLuint maxValueFrameBuffer2=
        static_cast<GLuint>(this->MaxValueFrameBuffer2);
      glDeleteTextures(1,&maxValueFrameBuffer2);
      this->MaxValueFrameBuffer2=0;
      }
    }

  if (accumulativeBlendMode && (this->SizeChanged || needNewMaxValueBuffer))
    {
    // max scalar frame buffer
    GLuint maxValueFrameBuffer=static_cast<GLuint>(this->MaxValueFrameBuffer);
    glBindTexture(GL_TEXTURE_2D,maxValueFrameBuffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, vtkgl::CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, vtkgl::CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // Here we are assuming that GL_ARB_texture_non_power_of_two is available
    if(this->Supports_GL_ARB_texture_float)
      {
      glTexImage2D(GL_TEXTURE_2D,0,vtkgl::RGBA16F_ARB,size[0],size[1],
                   0, GL_RGBA, GL_FLOAT, NULL );
      }
    else
      {
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16,size[0],size[1],
                   0, GL_RGBA, GL_FLOAT, NULL );
      }


    // Bind and attach here (after the size has been set, or ATI will cry),
    // then restore default buffer.
    vtkgl::BindFramebufferEXT(
      vtkgl::FRAMEBUFFER_EXT,static_cast<GLuint>(this->FrameBufferObject));
    vtkgl::FramebufferTexture2DEXT(vtkgl::FRAMEBUFFER_EXT,
                                   vtkgl::COLOR_ATTACHMENT0_EXT+1,
                                   GL_TEXTURE_2D,maxValueFrameBuffer,0);
    vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT,
                              static_cast<GLuint>(savedFrameBuffer));


    // max scalar frame buffer 2
    GLuint maxValueFrameBuffer2=static_cast<GLuint>(this->MaxValueFrameBuffer2);
    glBindTexture(GL_TEXTURE_2D,maxValueFrameBuffer2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, vtkgl::CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, vtkgl::CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // Here we are assuming that GL_ARB_texture_non_power_of_two is available
    if(this->Supports_GL_ARB_texture_float)
      {
      glTexImage2D(GL_TEXTURE_2D,0,vtkgl::RGBA16F_ARB,size[0],size[1],
                   0, GL_RGBA, GL_FLOAT, NULL );
      }
    else
      {
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16,size[0],size[1],
                   0, GL_RGBA, GL_FLOAT, NULL );
      }
    }

  vtkOpenGLCheckErrorMacro("failed after AllocateFrameBuffers");
  return result;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::GetTextureFormat(
  vtkImageData *input,
  unsigned int *internalFormat,
  unsigned int *format,
  unsigned int *type,
  int *componentSize)
{
  *internalFormat=0;
  *format=0;
  *type=0;
  *componentSize=0;

  vtkDataArray *scalars=this->GetScalars(input,this->ScalarMode,
                                         this->ArrayAccessMode,
                                         this->ArrayId,
                                         this->ArrayName,
                                         this->CellFlag);

  int scalarType=scalars->GetDataType();
  int components=scalars->GetNumberOfComponents();
  *componentSize=vtkAbstractArray::GetDataTypeSize(scalarType)*components;

  if(components==4)
    {
    // this is RGBA, unsigned char only
    *internalFormat=GL_RGBA16;
    *format=GL_RGBA;
    *type=GL_UNSIGNED_BYTE;
    }
  else
    {
    // components==1
    switch(scalarType)
      {
      case VTK_FLOAT:
        if(this->Supports_GL_ARB_texture_float)
          {
          *internalFormat=vtkgl::INTENSITY16F_ARB;
          }
        else
          {
          *internalFormat=GL_INTENSITY16;
          }
        *format=GL_RED;
        *type=GL_FLOAT;
        break;
      case VTK_UNSIGNED_CHAR:
        *internalFormat=GL_INTENSITY8;
        *format=GL_RED;
        *type=GL_UNSIGNED_BYTE;
        break;
      case VTK_SIGNED_CHAR:
        *internalFormat=GL_INTENSITY8;
        *format=GL_RED;
        *type=GL_BYTE;
        break;
      case VTK_CHAR:
        // not supported
        assert("check: impossible case" && 0);
        break;
      case VTK_BIT:
        // not supported
        assert("check: impossible case" && 0);
        break;
      case VTK_ID_TYPE:
        // not supported
        assert("check: impossible case" && 0);
        break;
      case VTK_INT:
        *internalFormat=GL_INTENSITY16;
        *format=GL_RED;
        *type=GL_INT;
        break;
      case VTK_DOUBLE:
      case VTK___INT64:
      case VTK_LONG:
      case VTK_LONG_LONG:
      case VTK_UNSIGNED___INT64:
      case VTK_UNSIGNED_LONG:
      case VTK_UNSIGNED_LONG_LONG:
        if(this->Supports_GL_ARB_texture_float)
          {
          *internalFormat=vtkgl::INTENSITY16F_ARB;
          }
        else
          {
          *internalFormat=GL_INTENSITY16;
          }
        *format=GL_RED;
        *type=GL_FLOAT;
        break;
      case VTK_SHORT:
        *internalFormat=GL_INTENSITY16;
        *format=GL_RED;
        *type=GL_SHORT;
        break;
      case VTK_STRING:
        // not supported
        assert("check: impossible case" && 0);
        break;
      case VTK_UNSIGNED_SHORT:
        *internalFormat=GL_INTENSITY16;
        *format=GL_RED;
        *type=GL_UNSIGNED_SHORT;
        break;
      case VTK_UNSIGNED_INT:
        *internalFormat=GL_INTENSITY16;
        *format=GL_RED;
        *type=GL_UNSIGNED_INT;
        break;
      default:
        assert("check: impossible case" && 0);
        break;
      }
    }
}

//-----------------------------------------------------------------------------
// Assuming the textureSize[3] is less of equal to the maximum size of an
// OpenGL 3D texture, try to see if the texture can fit on the card.
//-----------------------------------------------------------------------------
bool vtkOpenGLGPUVolumeRayCastMapper::TestLoadingScalar(
  unsigned int internalFormat,
  unsigned int format,
  unsigned int type,
  int textureSize[3],
  int componentSize)
{
  // componentSize=vtkAbstractArray::GetDataTypeSize(scalarType)*input->GetNumberOfScalarComponents()

  bool result;

  vtkgl::TexImage3D(vtkgl::PROXY_TEXTURE_3D,0,
                    static_cast<GLint>(internalFormat),
                    textureSize[0],textureSize[1],textureSize[2],0,
                    format,
                    type,0);
  GLint width;
  glGetTexLevelParameteriv(vtkgl::PROXY_TEXTURE_3D,0,GL_TEXTURE_WIDTH,
                           &width);

  result=width!=0;
  if(result)
    {
     // so far, so good but some cards always succeed with a proxy texture
    // let's try to actually allocate..
    vtkgl::TexImage3D(vtkgl::TEXTURE_3D,0,static_cast<GLint>(internalFormat),
                      textureSize[0],
                      textureSize[1],textureSize[2],0,
                      format,
                      type,0);
    GLenum errorCode=glGetError();
    result=errorCode!=GL_OUT_OF_MEMORY;
    if(result)
      {
      if(errorCode!=GL_NO_ERROR)
        {
        cout<<"after try to load the texture";
        cout<<" ERROR (x"<<hex<<errorCode<<") "<<dec;
        cout<<OpenGLErrorMessage(static_cast<unsigned int>(errorCode));
        cout<<endl;
        }
      // so far, so good but some cards don't report allocation error
      result=textureSize[0]*textureSize[1]*textureSize[2]*componentSize
        <=static_cast<float>(this->MaxMemoryInBytes)*this->MaxMemoryFraction;
      }
    }
  return result;
}

//-----------------------------------------------------------------------------
// Load the scalar field (one or four component scalar field), cell or point
// based for a given subextent of the whole extent (can be the whole extent)
// as a 3D texture on the GPU.
// Extents are expressed in point if the cell flag is false or in cells of
// the cell flag is true.
// It returns true if it succeeded, false if there is not enough memory on
// the GPU.
// If succeeded, it updates the LoadedExtent, LoadedBounds, LoadedCellFlag
// and LoadedTime. It also succeed if the scalar field is already loaded
// (ie since last load, input has not changed and cell flag has not changed
// and requested texture extents are enclosed in the loaded extent).
// \pre input_exists: input!=0
// \pre valid_point_extent: (this->CellFlag ||
//                           (textureExtent[0]<textureExtent[1] &&
//                            textureExtent[2]<textureExtent[3] &&
//                            textureExtent[4]<textureExtent[5])))
// \pre valid_cell_extent: (!this->CellFlag ||
//                          (textureExtent[0]<=textureExtent[1] &&
//                           textureExtent[2]<=textureExtent[3] &&
//                           textureExtent[4]<=textureExtent[5])))
//-----------------------------------------------------------------------------
int vtkOpenGLGPUVolumeRayCastMapper::LoadScalarField(vtkImageData *input,
                                                        vtkImageData *maskInput,
                                                       int textureExtent[6],
                                                       vtkVolume *volume)
{
  assert("pre: input_exists" && input!=0);
  assert("pre: valid_point_extent" && (this->CellFlag ||
                                       (textureExtent[0]<textureExtent[1] &&
                                        textureExtent[2]<textureExtent[3] &&
                                        textureExtent[4]<textureExtent[5])));
  assert("pre: valid_cell_extent" && (!this->CellFlag ||
                                      (textureExtent[0]<=textureExtent[1] &&
                                       textureExtent[2]<=textureExtent[3] &&
                                       textureExtent[4]<=textureExtent[5])));

  vtkOpenGLClearErrorMacro();

  int result=1; // succeeded

  // make sure we rebind our texture object to texture0 even if we don't have
  // to load the data themselves because the binding might be changed by
  // another mapper between two rendering calls.

  vtkgl::ActiveTexture(vtkgl::TEXTURE0);

  // Find the texture.
  std::map<vtkImageData *,vtkKWScalarField *>::iterator it=
    this->ScalarsTextures->Map.find(input);


  vtkKWScalarField *texture;
  if(it==this->ScalarsTextures->Map.end())
    {
    texture=new vtkKWScalarField;
    this->ScalarsTextures->Map[input]=texture;
    texture->SetSupports_GL_ARB_texture_float(this->Supports_GL_ARB_texture_float==1);
    }
  else
    {
    texture=(*it).second;
    }

  texture->Update(input,this->CellFlag,textureExtent,this->ScalarMode,
                  this->ArrayAccessMode,
                  this->ArrayId,
                  this->ArrayName,
                  volume->GetProperty()->GetInterpolationType()
                  ==VTK_LINEAR_INTERPOLATION,
                  this->TableRange,
                  static_cast<vtkIdType>(static_cast<float>(this->MaxMemoryInBytes)*this->MaxMemoryFraction));

  result=texture->IsLoaded();
  this->CurrentScalar=texture;


  // Mask
  if(maskInput!=0)
    {
    vtkgl::ActiveTexture(vtkgl::TEXTURE7);

    // Find the texture.
    std::map<vtkImageData *,vtkKWMask *>::iterator it2=
      this->MaskTextures->Map.find(maskInput);


    vtkKWMask *mask;
    if(it2==this->MaskTextures->Map.end())
      {
      mask=new vtkKWMask;
      this->MaskTextures->Map[maskInput]=mask;
      }
    else
      {
      mask=(*it2).second;
      }

    mask->Update(maskInput,this->CellFlag,textureExtent,this->ScalarMode,
                 this->ArrayAccessMode,
                 this->ArrayId,
                 this->ArrayName,
                 static_cast<vtkIdType>(static_cast<float>(this->MaxMemoryInBytes)*this->MaxMemoryFraction));

    result=result && mask->IsLoaded();
    this->CurrentMask=mask;
    vtkgl::ActiveTexture(vtkgl::TEXTURE0);
    }

  vtkOpenGLCheckErrorMacro("failed after LoadScalarField");

  return result;
}

//-----------------------------------------------------------------------------
// Allocate memory and load color table on the GPU or
// reload it if the transfer function changed.
// \pre vol_exists: vol!=0
// \pre valid_numberOfScalarComponents: numberOfScalarComponents==1 || numberOfScalarComponents==4
//-----------------------------------------------------------------------------
int vtkOpenGLGPUVolumeRayCastMapper::UpdateColorTransferFunction(
  vtkVolume *vol,
  int numberOfScalarComponents)
{
  assert("pre: vol_exists" && vol!=0);
  assert("pre: valid_numberOfScalarComponents" &&
         (numberOfScalarComponents==1 || numberOfScalarComponents==4));

  vtkOpenGLClearErrorMacro();

  // Build the colormap in a 1D texture.

  // 1D RGB-texture=mapping from scalar values to color values
  // build the table

  if(numberOfScalarComponents==1)
    {
    vtkVolumeProperty *volumeProperty=vol->GetProperty();
    vtkColorTransferFunction *colorTransferFunction=volumeProperty->GetRGBTransferFunction(0);

    vtkgl::ActiveTexture(vtkgl::TEXTURE1);

    this->RGBTable->Update(
      colorTransferFunction,this->TableRange,
      volumeProperty->GetInterpolationType()==VTK_LINEAR_INTERPOLATION);
    // Restore default
    vtkgl::ActiveTexture( vtkgl::TEXTURE0);
    }

  if (this->MaskInput != 0 && this->MaskType == LabelMapMaskType)
    {
    vtkVolumeProperty *volumeProperty=vol->GetProperty();
    vtkColorTransferFunction *c=volumeProperty->GetRGBTransferFunction(1);

    vtkgl::ActiveTexture(vtkgl::TEXTURE8);
    this->Mask1RGBTable->Update(c,this->TableRange,false);

    c=volumeProperty->GetRGBTransferFunction(2);
    vtkgl::ActiveTexture(vtkgl::TEXTURE9);
    this->Mask2RGBTable->Update(c,this->TableRange,false);

     // Restore default
    vtkgl::ActiveTexture( vtkgl::TEXTURE0);
    }

  vtkOpenGLCheckErrorMacro("failed after UpdateColorTransferFunction");
  return 1;
}

//-----------------------------------------------------------------------------
// Allocate memory and load opacity table on the GPU or
// reload it if the transfert function changed.
// \pre vol_exists: vol!=0
// \pre valid_numberOfScalarComponents: numberOfScalarComponents==1 || numberOfScalarComponents==4
//-----------------------------------------------------------------------------
int vtkOpenGLGPUVolumeRayCastMapper::UpdateOpacityTransferFunction(
  vtkVolume *vol,
  int numberOfScalarComponents,
  unsigned int level)
{
  assert("pre: vol_exists" && vol!=0);
  assert("pre: valid_numberOfScalarComponents" &&
         (numberOfScalarComponents==1 || numberOfScalarComponents==4));

  vtkOpenGLClearErrorMacro();

  (void)numberOfScalarComponents; // remove warning in release mode.

  vtkVolumeProperty *volumeProperty=vol->GetProperty();
  vtkPiecewiseFunction *scalarOpacity=volumeProperty->GetScalarOpacity();

  vtkgl::ActiveTexture( vtkgl::TEXTURE2); //stay here
  this->OpacityTables->GetTable(level)->Update(
    scalarOpacity,this->BlendMode,
    this->ActualSampleDistance,
    this->TableRange,
    volumeProperty->GetScalarOpacityUnitDistance(0),
    volumeProperty->GetInterpolationType()==VTK_LINEAR_INTERPOLATION);
  // Restore default active texture
  vtkgl::ActiveTexture( vtkgl::TEXTURE0);

  vtkOpenGLCheckErrorMacro("failed after UpdateOpacityTransferFunction");

  return 1;
}

//-----------------------------------------------------------------------------
// Prepare rendering in the offscreen framebuffer.
// \pre ren_exists: ren!=0
// \pre vol_exists: vol!=0
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::SetupRender(vtkRenderer *ren,
                                                    vtkVolume *vol)
{
  assert("pre: ren_exists" && ren!=0);
  assert("pre: vol_exists" && vol!=0);

  vtkOpenGLClearErrorMacro();

  int  lowerLeft[2];
  int usize, vsize;

  ren->GetTiledSizeAndOrigin(&usize,&vsize,lowerLeft,lowerLeft+1);

  usize = static_cast<int>(usize*this->ReductionFactor);
  vsize = static_cast<int>(vsize*this->ReductionFactor);

  this->ReducedSize[0]=usize;
  this->ReducedSize[1]=vsize;

  // the FBO has the size of the renderer (not the renderwindow),
  // we always starts at 0,0.
  glViewport(0,0, usize, vsize);
  glEnable( GL_SCISSOR_TEST ); // scissor on the FBO, on the reduced part.
  glScissor(0,0, usize, vsize);
  glClearColor(0.0, 0.0, 0.0, 0.0); // maxvalue is 1

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  //double aspect[2];
  //ren->ComputeAspect();
  //ren->GetAspect(aspect);
  //double aspect2[2];
  //ren->vtkViewport::ComputeAspect();
  //ren->vtkViewport::GetAspect(aspect2);
  //double aspectModification = aspect[0]*aspect2[1]/(aspect[1]*aspect2[0]);

  //vtkCamera *cam = ren->GetActiveCamera();
  //glMatrixMode( GL_PROJECTION);
  //if(usize && vsize)
  //  {
  //  this->TempMatrix[0]->DeepCopy(cam->GetProjectionTransformMatrix(
  //                                  aspectModification*usize/vsize, -1,1));
  //  this->TempMatrix[0]->Transpose();
  //  glLoadMatrixd(this->TempMatrix[0]->Element[0]);
  //  }
  //else
  //  {
  //  glLoadIdentity();

  //  }

  // push the model view matrix onto the stack, make sure we
  // adjust the mode first
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  this->TempMatrix[0]->DeepCopy(vol->GetMatrix());
  this->TempMatrix[0]->Transpose();

  // insert camera view transformation
  glMultMatrixd(this->TempMatrix[0]->Element[0]);
  glShadeModel(GL_SMOOTH);
  glDisable( GL_LIGHTING);
  glEnable (GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND); // very important, otherwise the first image looks dark.

  vtkOpenGLCheckErrorMacro("failed after SetupRender");
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::DebugDisplayBox(vtkPolyData *box)
{
  vtkPoints *points=box->GetPoints();
  vtkCellArray *polys=box->GetPolys();
  cout<<"npts="<<points->GetNumberOfPoints()<<endl;
  int pointId=0;
  while(pointId<points->GetNumberOfPoints())
    {
    double coords[3];
    points->GetPoint(pointId,coords);
    cout<<"pointId="<<pointId<<endl;
    int i=0;
    while(i<3)
      {
      cout<<" "<<coords[i];
      ++i;
      }
    cout<<endl;
    ++pointId;
    }
  vtkIdType npts=0;
  vtkIdType *pts=0;
  cout<<"ncells="<<polys->GetNumberOfCells()<<endl;
  int cellId=0;
  polys->InitTraversal();
  while(cellId<polys->GetNumberOfCells())
    {
    polys->GetNextCell(npts,pts);
    cout<<"cellId="<<cellId<<" npts="<<npts<<endl;
    vtkIdType i=0;
    while(i<npts)
      {
      cout<<pts[i]<<" ";
      ++i;
      }
    cout<<endl;
    ++cellId;
    }
}

//-----------------------------------------------------------------------------
// Clip the bounding box with the clipping planes and near and
// far planes. Grab the output polydata for later rendering.
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::ClipBoundingBox(vtkRenderer *ren,
                                                        double worldBounds[6],
                                                        vtkVolume *vol)
{
  // Pass camera through inverse volume matrix
  // so that we are in the same coordinate system
  vol->GetMatrix( this->InvVolumeMatrix );
  this->InvVolumeMatrix->Invert();
  // Normals should be transformed using the transpose of the
  // invert of InvVolumeMatrix.
  vtkMatrix4x4::Transpose(vol->GetMatrix(),this->TempMatrix[0]);

  if(this->BoxSource==0)
    {
    this->BoxSource=vtkTessellatedBoxSource::New();
    }
  this->BoxSource->SetBounds(worldBounds);
  this->BoxSource->SetLevel(0);
  this->BoxSource->QuadsOn();

  if(this->Planes==0)
    {
    this->Planes=vtkPlaneCollection::New();
    }
  this->Planes->RemoveAllItems();

  vtkCamera *cam = ren->GetActiveCamera();
  double camWorldRange[2];
  double camWorldPos[4];
  double camFocalWorldPoint[4];
  double camWorldDirection[4];
  double camPos[4];
  double camPlaneNormal[4];

  cam->GetPosition(camWorldPos);
  camWorldPos[3] = 1.0;
  this->InvVolumeMatrix->MultiplyPoint( camWorldPos, camPos );
  if ( camPos[3] )
    {
    camPos[0] /= camPos[3];
    camPos[1] /= camPos[3];
    camPos[2] /= camPos[3];
    }

  cam->GetFocalPoint(camFocalWorldPoint);
  camFocalWorldPoint[3]=1.0;

  // The range (near/far) must also be transformed
  // into the local coordinate system.
  camWorldDirection[0] = camFocalWorldPoint[0] - camWorldPos[0];
  camWorldDirection[1] = camFocalWorldPoint[1] - camWorldPos[1];
  camWorldDirection[2] = camFocalWorldPoint[2] - camWorldPos[2];
  camWorldDirection[3] = 1.0;

  // Compute the normalized near plane normal
  this->TempMatrix[0]->MultiplyPoint( camWorldDirection, camPlaneNormal );

  vtkMath::Normalize(camWorldDirection);
  vtkMath::Normalize(camPlaneNormal);

  double camNearWorldPoint[4];
  double camFarWorldPoint[4];
  double camNearPoint[4];
  double camFarPoint[4];

  cam->GetClippingRange(camWorldRange);
  camNearWorldPoint[0] = camWorldPos[0] + camWorldRange[0]*camWorldDirection[0];
  camNearWorldPoint[1] = camWorldPos[1] + camWorldRange[0]*camWorldDirection[1];
  camNearWorldPoint[2] = camWorldPos[2] + camWorldRange[0]*camWorldDirection[2];
  camNearWorldPoint[3] = 1.;

  camFarWorldPoint[0] = camWorldPos[0] + camWorldRange[1]*camWorldDirection[0];
  camFarWorldPoint[1] = camWorldPos[1] + camWorldRange[1]*camWorldDirection[1];
  camFarWorldPoint[2] = camWorldPos[2] + camWorldRange[1]*camWorldDirection[2];
  camFarWorldPoint[3] = 1.;

  this->InvVolumeMatrix->MultiplyPoint( camNearWorldPoint, camNearPoint );
  if (camNearPoint[3]!=0.0)
    {
    camNearPoint[0] /= camNearPoint[3];
    camNearPoint[1] /= camNearPoint[3];
    camNearPoint[2] /= camNearPoint[3];
    }

  this->InvVolumeMatrix->MultiplyPoint( camFarWorldPoint, camFarPoint );
  if (camFarPoint[3]!=0.0)
    {
    camFarPoint[0] /= camFarPoint[3];
    camFarPoint[1] /= camFarPoint[3];
    camFarPoint[2] /= camFarPoint[3];
    }


  if(this->NearPlane==0)
    {
    this->NearPlane= vtkPlane::New();
    }

  // We add an offset to the near plane to avoid hardware clipping of the
  // near plane due to floating-point precision.
  // camPlaneNormal is a unit vector, if the offset is larger than the
  // distance between near and far point, it will not work, in this case we
  // pick a fraction of the near-far distance.
  double distNearFar=
    sqrt(vtkMath::Distance2BetweenPoints(camNearPoint,camFarPoint));
  double offset=0.001; // some arbitrary small value.
  if(offset>=distNearFar)
    {
    offset=distNearFar/1000.0;
    }

  camNearPoint[0]+=camPlaneNormal[0]*offset;
  camNearPoint[1]+=camPlaneNormal[1]*offset;
  camNearPoint[2]+=camPlaneNormal[2]*offset;



  this->NearPlane->SetOrigin( camNearPoint );
  this->NearPlane->SetNormal( camPlaneNormal );
  this->Planes->AddItem(this->NearPlane);

  if ( this->ClippingPlanes )
    {
    this->ClippingPlanes->InitTraversal();
    vtkPlane *plane;
    while ( (plane = this->ClippingPlanes->GetNextItem()) )
      {
      // Planes are in world coordinates, we need to
      // convert them in local coordinates
      double planeOrigin[4], planeNormal[4];//, planeP1[4];
      plane->GetOrigin(planeOrigin);
      planeOrigin[3] = 1.;
      plane->GetNormal(planeNormal);
      planeNormal[3] = 1.;
      this->InvVolumeMatrix->MultiplyPoint(planeOrigin, planeOrigin);
      if( planeOrigin[3])
        {
        planeOrigin[0] /= planeOrigin[3];
        planeOrigin[1] /= planeOrigin[3];
        planeOrigin[2] /= planeOrigin[3];
        }
      this->TempMatrix[0]->MultiplyPoint(planeNormal, planeNormal);
      vtkMath::Normalize(planeNormal);
      vtkPlane* localPlane = vtkPlane::New();
      localPlane->SetOrigin(planeOrigin);
      localPlane->SetNormal(planeNormal);
      this->Planes->AddItem(localPlane);
      localPlane->Delete();
      }
    }

  if(this->Clip==0)
    {
    this->Clip=vtkClipConvexPolyData::New();
    this->Clip->SetInputConnection(this->BoxSource->GetOutputPort());
    this->Clip->SetPlanes( this->Planes );
    }

  this->Clip->Update();

  if(this->Densify==0)
    {
    this->Densify=vtkDensifyPolyData::New();
    this->Densify->SetInputConnection(this->Clip->GetOutputPort());
    this->Densify->SetNumberOfSubdivisions(2);
    }
  this->Densify->Update();
  this->ClippedBoundingBox = this->Densify->GetOutput();
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int vtkOpenGLGPUVolumeRayCastMapper::RenderClippedBoundingBox(
  int tcoordFlag,
  size_t currentBlock,
  size_t numberOfBlocks,
  vtkRenderWindow *renWin )
{
  assert("pre: valid_currentBlock" && currentBlock<numberOfBlocks);

  vtkOpenGLClearErrorMacro();

  vtkPoints *points = this->ClippedBoundingBox->GetPoints();
  vtkCellArray *polys = this->ClippedBoundingBox->GetPolys();

  vtkIdType npts;
  vtkIdType *pts;

  vtkIdType i, j;

  double center[3] = {0,0,0};

  // First compute center point
  npts = points->GetNumberOfPoints();
  for ( i = 0; i < npts; i++ )
    {
    double pt[3];
    points->GetPoint( i, pt );
    for ( j = 0; j < 3; j++ )
      {
      center[j] += pt[j];
      }
    }

  if(npts>0)
    {
    center[0]/=static_cast<double>(npts);
    center[1]/=static_cast<double>(npts);
    center[2]/=static_cast<double>(npts);
    }

  double *loadedBounds=0;
  vtkIdType *loadedExtent=0;

  if ( tcoordFlag )
    {
    loadedBounds=this->CurrentScalar->GetLoadedBounds();
    loadedExtent=this->CurrentScalar->GetLoadedExtent();
    }

  double *spacing=this->GetInput()->GetSpacing();
  double spacingSign[3];
  i=0;
  while(i<3)
    {
    if(spacing[i]<0)
      {
      spacingSign[i]=-1.0;
      }
    else
      {
      spacingSign[i]=1.0;
      }
    ++i;
    }

  // make it double for the ratio of the progress.
  int polyId=0;
  double polyCount=static_cast<double>(polys->GetNumberOfCells());
  polys->InitTraversal();
  int abort=0;
  while ( !abort && polys->GetNextCell(npts, pts) )
    {
    vtkIdType start, end, inc;

    // Need to have at least a triangle
    if ( npts > 2 )
      {
      // Check the cross product of the first two
      // vectors dotted with the vector from the
      // center to the second point. Is it positive or
      // negative?

      double p1[3], p2[3], p3[3];
      double v3[3], v4[3];

      points->GetPoint(pts[0], p1 );
      points->GetPoint(pts[1], p2 );
      points->GetPoint(pts[2], p3 );

      vtkTriangle::ComputeNormal(p1, p2, p3, v3);

      v4[0] = p2[0] - center[0];
      v4[1] = p2[1] - center[1];
      v4[2] = p2[2] - center[2];
      vtkMath::Normalize(v4);

      double dot = vtkMath::Dot( v3, v4 );
      if(!this->PreserveOrientation)
        {
        dot=-dot;
        }
      if(dot >= -0.000001)
        {
        start = 0;
        end = npts;
        inc = 1;
        }
      else
        {
        start = npts-1;
        end = -1;
        inc = -1;
        }

      glBegin( GL_TRIANGLE_FAN ); // GL_POLYGON -> GL_TRIANGLE_FAN

      double vert[3];
      double tcoord[3];
      for ( i = start; i != end; i += inc )
        {
        points->GetPoint(pts[i], vert);
        if ( tcoordFlag )
          {
          for ( j = 0; j < 3; j++ )
            {
            // loaded bounds take both cell data and point date cases into
            // account
            if(this->CellFlag) // texcoords between 0 and 1. More complex
              // depends on the loaded texture
              {
              tcoord[j] = spacingSign[j]*(vert[j] - loadedBounds[j*2]) /
                (loadedBounds[j*2+1] - loadedBounds[j*2]);
              }
            else // texcoords between 1/2N and 1-1/2N.
              {
              double tmp; // between 0 and 1
              tmp = spacingSign[j]*(vert[j] - loadedBounds[j*2]) /
                (loadedBounds[j*2+1] - loadedBounds[j*2]);
              double delta=static_cast<double>(
                loadedExtent[j*2+1]-loadedExtent[j*2]+1);
              tcoord[j]=(tmp*(delta-1)+0.5)/delta;
              }
            }
          vtkgl::MultiTexCoord3dv(vtkgl::TEXTURE0, tcoord);
          }
        glVertex3dv(vert);
        }
      glEnd();

      }
    if(tcoordFlag)
      {
      // otherwise, we are rendering back face to initialize the zbuffer.
      if (!this->GeneratingCanonicalView && this->ReportProgress)
        {
        glFinish();
        // Only invoke an event at most one every second.
        double currentTime=vtkTimerLog::GetUniversalTime();
        if(currentTime - this->LastProgressEventTime > 1.0)
          {
          double progress=(static_cast<double>(currentBlock)+polyId/polyCount)/
            static_cast<double>(numberOfBlocks);
          this->InvokeEvent(vtkCommand::VolumeMapperRenderProgressEvent,
                            &progress);
          renWin->MakeCurrent();
          this->LastProgressEventTime = currentTime;
          }
        }
      abort=renWin->CheckAbortStatus();
      }
    ++polyId;
    }

  vtkOpenGLCheckErrorMacro("failed after RenderClippedBoundingBox");
  return abort;
}

// ----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::CopyFBOToTexture()
{
  vtkOpenGLClearErrorMacro();
  // in OpenGL copy texture to texture does not exist but
  // framebuffer to texture exists (and our FB is an FBO).
  // we have to copy and not just to switch color textures because the
  // colorbuffer has to accumulate color or values step after step.
  // Switching would not work because two different steps can draw different
  // polygons that don't overlap
  vtkgl::ActiveTexture(vtkgl::TEXTURE4);
  glBindTexture(
    GL_TEXTURE_2D,
    this->TextureObjects[
      vtkOpenGLGPUVolumeRayCastMapperTextureObjectFrameBufferLeftFront+1]);

  glReadBuffer(vtkgl::COLOR_ATTACHMENT0_EXT);
  glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,this->ReducedSize[0],
                      this->ReducedSize[1]);
  if(this->BlendMode==vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND
     || this->BlendMode==vtkGPUVolumeRayCastMapper::MINIMUM_INTENSITY_BLEND
     || this->BlendMode==vtkGPUVolumeRayCastMapper::ADDITIVE_BLEND)
    {
    vtkgl::ActiveTexture(vtkgl::TEXTURE5);
    glBindTexture(GL_TEXTURE_2D,this->MaxValueFrameBuffer2);
    glReadBuffer(vtkgl::COLOR_ATTACHMENT0_EXT+1);
    glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,this->ReducedSize[0],
                      this->ReducedSize[1]);
    }
  vtkgl::ActiveTexture(vtkgl::TEXTURE0);
  // reset the readbuffer to keep os mesa happy
  // during CheckFrameBufferStatus
  glReadBuffer(vtkgl::COLOR_ATTACHMENT0_EXT);
  vtkOpenGLCheckErrorMacro("failed after CopyFBOToTexture");
}

//-----------------------------------------------------------------------------
// Restore OpenGL state after rendering of the dataset.
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::CleanupRender()
{
  glPopMatrix();
  glDisable(GL_CULL_FACE);
  vtkOpenGLCheckErrorMacro("failed after CleanupRender");
}

//-----------------------------------------------------------------------------
// Build the fragment shader program that scale and bias a texture
// for window/level purpose.
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::BuildScaleBiasProgram(vtkRenderWindow *w)
{
  if(this->ScaleBiasProgram==0)
    {
    this->ScaleBiasProgram=vtkShaderProgram2::New();
    this->ScaleBiasProgram->SetContext(
      static_cast<vtkOpenGLRenderWindow *>(w));
    vtkShader2Collection *shaders=this->ScaleBiasProgram->GetShaders();

    vtkShader2 *s=vtkShader2::New();
    s->SetType(VTK_SHADER_TYPE_FRAGMENT);
    s->SetSourceCode(vtkGPUVolumeRayCastMapper_ScaleBiasFS);
    shaders->AddItem(s);
    s->Delete();
    }
}

//-----------------------------------------------------------------------------
// Render the offscreen buffer to the screen.
// \pre ren_exists: ren!=0
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::RenderTextureToScreen(vtkRenderer *ren)
{
  assert("pre: ren_exists" && ren!=0);

  vtkOpenGLClearErrorMacro();

  if ( this->GeneratingCanonicalView )
    {
    // We just need to copy of the data, not render it
    glBindTexture(GL_TEXTURE_2D,
                  this->TextureObjects[
                    vtkOpenGLGPUVolumeRayCastMapperTextureObjectFrameBufferLeftFront]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    glPixelStorei( GL_PACK_ALIGNMENT, 1 );

    unsigned char *outPtr = static_cast<unsigned char *>(this->CanonicalViewImageData->GetScalarPointer());
    glGetTexImage( GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, outPtr );
    return;
    }

  int  lowerLeft[2];
  int usize, vsize;
  ren->GetTiledSizeAndOrigin(&usize,&vsize,lowerLeft,lowerLeft+1);
  glViewport(lowerLeft[0],lowerLeft[1], usize, vsize);
  glEnable( GL_SCISSOR_TEST );
  glScissor(lowerLeft[0],lowerLeft[1], usize, vsize);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0.0, usize, 0.0, vsize, -1.0, 1.0 );
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glBindTexture(GL_TEXTURE_2D,
                this->TextureObjects[vtkOpenGLGPUVolumeRayCastMapperTextureObjectFrameBufferLeftFront]);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glEnable(GL_BLEND);
  glBlendFunc( GL_ONE,GL_ONE_MINUS_SRC_ALPHA);

  // As we use replace mode, we don't need to set the color value.

  glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);

  glDisable(GL_DEPTH_TEST);

  double xOffset = 1.0 / usize;
  double yOffset = 1.0 / vsize;

  glDepthMask(GL_FALSE);

  double scale=1.0/this->FinalColorWindow;
  double bias=0.5-this->FinalColorLevel/this->FinalColorWindow;

  if(scale!=1.0 || bias!=0.0)
    {
    this->BuildScaleBiasProgram(ren->GetRenderWindow());
    vtkUniformVariables *v=this->ScaleBiasProgram->GetUniformVariables();
    int ivalue=0;
    v->SetUniformi("frameBufferTexture",1,&ivalue);
    float fvalue=static_cast<float>(scale);
    v->SetUniformf("scale",1,&fvalue);
    fvalue=static_cast<float>(bias);
    v->SetUniformf("bias",1,&fvalue);
    this->ScaleBiasProgram->Use();
    }
  else
    {
    glEnable(GL_TEXTURE_2D); // fixed pipeline
    }

  glBegin(GL_QUADS);
  glTexCoord2f(static_cast<GLfloat>(xOffset),static_cast<GLfloat>(yOffset));
  glVertex2f(0.0,0.0);
  glTexCoord2f(static_cast<GLfloat>(this->ReductionFactor-xOffset),
               static_cast<GLfloat>(yOffset));
  glVertex2f(static_cast<GLfloat>(usize),0.0);
  glTexCoord2f(static_cast<GLfloat>(this->ReductionFactor-xOffset),
               static_cast<GLfloat>(this->ReductionFactor-yOffset));
  glVertex2f(static_cast<GLfloat>(usize),static_cast<GLfloat>(vsize));
  glTexCoord2f(static_cast<GLfloat>(xOffset),
               static_cast<GLfloat>(this->ReductionFactor-yOffset));
  glVertex2f(0.0,static_cast<GLfloat>(vsize));
  glEnd();

  // Restore the default mode. Used in overlay.
  glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);

  if(scale!=1.0 || bias!=0.0)
    {
    this->ScaleBiasProgram->Restore();
    }
  else
    {
    glDisable(GL_TEXTURE_2D);
    }

  glDepthMask(GL_TRUE);

  glDisable(GL_BLEND);

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  vtkOpenGLCheckErrorMacro("failed after RenderTextureToScreen");
}

//-----------------------------------------------------------------------------
// Update the reduction factor of the render viewport (this->ReductionFactor)
// according to the time spent in seconds to render the previous frame
// (this->TimeToDraw) and a time in seconds allocated to render the next
// frame (allocatedTime).
// \pre valid_current_reduction_range: this->ReductionFactor>0.0 && this->ReductionFactor<=1.0
// \pre positive_TimeToDraw: this->TimeToDraw>=0.0
// \pre positive_time: allocatedTime>0.0
// \post valid_new_reduction_range: this->ReductionFactor>0.0 && this->ReductionFactor<=1.0
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::ComputeReductionFactor(
  double allocatedTime)
{
  assert("pre: valid_current_reduction_range" && this->ReductionFactor>0.0
         && this->ReductionFactor<=1.0);
  assert("pre: positive_TimeToDraw" && this->TimeToDraw>=0.0);
  assert("pre: positive_time" && allocatedTime>0.0);

  if ( this->GeneratingCanonicalView )
    {
    this->ReductionFactor = 1.0;
    return;
    }

  if ( !this->AutoAdjustSampleDistances )
    {
    this->ReductionFactor = 1.0 / this->ImageSampleDistance;
    return;
    }

  if ( this->TimeToDraw )
    {
    double oldFactor = this->ReductionFactor;

    double timeToDraw;
    if (allocatedTime < 1.0)
      {
      timeToDraw = this->SmallTimeToDraw;
      if ( timeToDraw == 0.0 )
        {
        timeToDraw = this->BigTimeToDraw/3.0;
        }
      }
    else
      {
      timeToDraw = this->BigTimeToDraw;
      }

    if ( timeToDraw == 0.0 )
      {
      timeToDraw = 10.0;
      }

    double fullTime = timeToDraw / this->ReductionFactor;
    double newFactor = allocatedTime / fullTime;

    if ( oldFactor == 1.0 ||
         newFactor / oldFactor > 1.3 ||
         newFactor / oldFactor < .95 )
      {

      this->ReductionFactor = (newFactor+oldFactor)/2.0;

      this->ReductionFactor = (this->ReductionFactor > 5.0)?(1.00):(this->ReductionFactor);
      this->ReductionFactor = (this->ReductionFactor > 1.0)?(0.99):(this->ReductionFactor);
      this->ReductionFactor = (this->ReductionFactor < 0.1)?(0.10):(this->ReductionFactor);

      if ( 1.0/this->ReductionFactor > this->MaximumImageSampleDistance )
        {
        this->ReductionFactor = 1.0 / this->MaximumImageSampleDistance;
        }
      if ( 1.0/this->ReductionFactor < this->MinimumImageSampleDistance )
        {
        this->ReductionFactor = 1.0 / this->MinimumImageSampleDistance;
        }
      }
    }
  else
    {
    this->ReductionFactor = 1.0;
    }

  assert("post: valid_new_reduction_range" && this->ReductionFactor>0.0
         && this->ReductionFactor<=1.0);
}

//-----------------------------------------------------------------------------
// Rendering initialization including making the context current, loading
// necessary extensions, allocating frame buffers, updating transfer function,
// computing clipping regions, and building the fragment shader.
//
// Pre-conditions:
//   - ren != NULL
//   - vol != NULL
//   - ren->GetRenderWindow() != NULL
//   - 1 <= numberOfScalarComponents <= 4
//   - numberOfLevels >= 1
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::PreRender(vtkRenderer *ren,
                                                vtkVolume *vol,
                                                double datasetBounds[6],
                                                double scalarRange[2],
                                                int numberOfScalarComponents,
                                                unsigned int numberOfLevels)
{
  // make sure our window is the current OpenGL context.
  ren->GetRenderWindow()->MakeCurrent();
  vtkOpenGLClearErrorMacro();

  // If we haven't already succeeded in loading the extensions,
  // try to load them
  if(!this->LoadExtensionsSucceeded)
    {
    this->LoadExtensions(ren->GetRenderWindow());
    }

  // If we can't load the necessary extensions, provide
  // feedback on why it failed.
  if(!this->LoadExtensionsSucceeded)
    {
    vtkErrorMacro(
      "Rendering failed because the following OpenGL extensions "
      "are required but not supported: " <<
      (this->UnsupportedRequiredExtensions->Stream.str()).c_str());
    return;
    }

  // Create the OpenGL object that we need
  this->CreateOpenGLObjects(ren);

  // Compute the reduction factor that may be necessary to get
  // the interactive rendering rate that we want
  this->ComputeReductionFactor(vol->GetAllocatedRenderTime());

  // Allocate the frame buffers
  if(!this->AllocateFrameBuffers(ren))
    {
    vtkErrorMacro("Not enough GPU memory to create a framebuffer.");
    return;
    }

  // Save the scalar range - this is what we will use for the range
  // of the transfer functions
  this->TableRange[0]=scalarRange[0];
  this->TableRange[1]=scalarRange[1];


  if(this->RGBTable==0)
    {
    this->RGBTable=new vtkRGBTable;
    }

  if (this->MaskInput != 0 && this->MaskType == LabelMapMaskType)
    {
    if(this->Mask1RGBTable==0)
      {
      this->Mask1RGBTable=new vtkRGBTable;
      }
    if(this->Mask2RGBTable==0)
      {
      this->Mask2RGBTable=new vtkRGBTable;
      }
    }

   // Update the color transfer function
  this->UpdateColorTransferFunction(vol,numberOfScalarComponents);

  // Update the noise texture that will be used to jitter rays to
  // reduce alliasing artifacts
  this->UpdateNoiseTexture();

  // We are going to change the blending mode and blending function -
  // so lets push here so we can pop later
  glPushAttrib(GL_COLOR_BUFFER_BIT);

  // If this is the canonical view - we don't want to intermix so we'll just
  // start by clearing the z buffer.
  if ( this->GeneratingCanonicalView )
    {
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

  // See if the volume transform is orientation-preserving
  vtkMatrix4x4 *m=vol->GetMatrix();
  double det=vtkMath::Determinant3x3(
    m->GetElement(0,0),m->GetElement(0,1),m->GetElement(0,2),
    m->GetElement(1,0),m->GetElement(1,1),m->GetElement(1,2),
    m->GetElement(2,0),m->GetElement(2,1),m->GetElement(2,2));

  this->PreserveOrientation=det>0;

  // If we have clipping planes, render the back faces of the clipped
  // bounding box of the whole dataset to set the zbuffer.
  if(this->ClippingPlanes && this->ClippingPlanes->GetNumberOfItems()!=0)
    {
    // push the model view matrix onto the stack, make sure we
    // adjust the mode first
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    this->TempMatrix[0]->DeepCopy(vol->GetMatrix());
    this->TempMatrix[0]->Transpose();
    glMultMatrixd(this->TempMatrix[0]->Element[0]);
    this->ClipBoundingBox(ren,datasetBounds,vol);
    glEnable (GL_CULL_FACE);
    glCullFace (GL_FRONT);
    glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
    glDisable(GL_ALPHA_TEST);
    this->RenderClippedBoundingBox(0,0,1,ren->GetRenderWindow());
    glDisable (GL_CULL_FACE);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    //glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    }
  // Check if everything is OK
  this->CheckFrameBufferStatus();

  // Intermixed geometry: Grab the depth buffer into a texture

  int size[2];
  int lowerLeft[2];
  ren->GetTiledSizeAndOrigin(size,size+1,lowerLeft,lowerLeft+1);

  vtkgl::ActiveTexture(vtkgl::TEXTURE3);
  glBindTexture(GL_TEXTURE_2D,
                static_cast<GLuint>(
                  this->TextureObjects[
                    vtkOpenGLGPUVolumeRayCastMapperTextureObjectDepthMap]));
  glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,lowerLeft[0],lowerLeft[1],size[0],
                      size[1]);


  vtkgl::ActiveTexture(vtkgl::TEXTURE0);

  int parallelProjection=ren->GetActiveCamera()->GetParallelProjection();

  // initialize variables to prevent compiler warnings.
  int rayCastMethod=vtkOpenGLGPUVolumeRayCastMapperMethodMIP;
  int shadeMethod=vtkOpenGLGPUVolumeRayCastMapperShadeNotUsed;
  int componentMethod=vtkOpenGLGPUVolumeRayCastMapperComponentNotUsed;

  switch(this->BlendMode)
    {
    case vtkVolumeMapper::COMPOSITE_BLEND:
      switch(numberOfScalarComponents)
        {
        case 1:
          componentMethod=vtkOpenGLGPUVolumeRayCastMapperComponentOne;
          break;
        case 4:
          componentMethod=vtkOpenGLGPUVolumeRayCastMapperComponentFour;
          break;
        default:
          assert("check: impossible case" && false);
          break;
        }

      // If we are using a mask to limit the volume rendering to or blend using a
      // label map mask..
      rayCastMethod = this->MaskInput ?
         (this->MaskType == vtkGPUVolumeRayCastMapper::BinaryMaskType ?
              vtkOpenGLGPUVolumeRayCastMapperMethodCompositeBinaryMask :
              vtkOpenGLGPUVolumeRayCastMapperMethodCompositeMask) :
            vtkOpenGLGPUVolumeRayCastMapperMethodComposite;

      if ( vol->GetProperty()->GetShade() )
        {
        shadeMethod=vtkOpenGLGPUVolumeRayCastMapperShadeYes;
        assert("check: only_1_component_todo" && numberOfScalarComponents==1);
        }
      else
        {
        shadeMethod=vtkOpenGLGPUVolumeRayCastMapperShadeNo;
        //cout<<"program is composite"<<endl;
        }
      break;
    case vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND:
      shadeMethod=vtkOpenGLGPUVolumeRayCastMapperShadeNotUsed;
      componentMethod=vtkOpenGLGPUVolumeRayCastMapperComponentNotUsed;
      switch(numberOfScalarComponents)
        {
        case 1:
          rayCastMethod= (this->MaskInput && this->MaskType ==
            vtkGPUVolumeRayCastMapper::BinaryMaskType) ?
              vtkOpenGLGPUVolumeRayCastMapperMethodMIPBinaryMask :
              vtkOpenGLGPUVolumeRayCastMapperMethodMIP;
          break;
        case 4:
          rayCastMethod=
            vtkOpenGLGPUVolumeRayCastMapperMethodMIPFourDependent;
          break;
        default:
          assert("check: impossible case" && false);
          break;
        }
      break;
    case vtkGPUVolumeRayCastMapper::MINIMUM_INTENSITY_BLEND:
      shadeMethod=vtkOpenGLGPUVolumeRayCastMapperShadeNotUsed;
      componentMethod=vtkOpenGLGPUVolumeRayCastMapperComponentNotUsed;
      switch(numberOfScalarComponents)
        {
        case 1:
          rayCastMethod= (this->MaskInput && this->MaskType ==
            vtkGPUVolumeRayCastMapper::BinaryMaskType) ?
              vtkOpenGLGPUVolumeRayCastMapperMethodMinIPBinaryMask :
              vtkOpenGLGPUVolumeRayCastMapperMethodMinIP;
          break;
        case 4:
          rayCastMethod=
            vtkOpenGLGPUVolumeRayCastMapperMethodMinIPFourDependent;
          break;
        default:
          assert("check: impossible case" && false);
          break;
        }
      break;
    case vtkGPUVolumeRayCastMapper::ADDITIVE_BLEND:
      shadeMethod=vtkOpenGLGPUVolumeRayCastMapperShadeNotUsed;
      componentMethod=vtkOpenGLGPUVolumeRayCastMapperComponentNotUsed;
      switch(numberOfScalarComponents)
        {
        case 1:
          rayCastMethod=vtkOpenGLGPUVolumeRayCastMapperMethodAdditive;
          break;
        default:
          assert("check: impossible case" && false);
          break;
        }
      break;
    default:
      assert("check: impossible case" && 0);
      rayCastMethod=0;
      break;
    }

  this->ComputeNumberOfCroppingRegions(); // TODO AMR vs single block
  if(this->AMRMode)
    {
    NumberOfCroppingRegions=2; // >1, means use do compositing between blocks
    }
  this->BuildProgram(ren->GetRenderWindow(),
                     parallelProjection,rayCastMethod,shadeMethod,
                     componentMethod);

#ifdef APPLE_SNOW_LEOPARD_BUG
  this->Program->Build();
#endif

  vtkUniformVariables *v=this->Program->GetUniformVariables();

  // for active texture 0, dataset

  if(numberOfScalarComponents==1)
    {
    // colortable
    vtkgl::ActiveTexture(vtkgl::TEXTURE1);
    this->RGBTable->Bind();

     if (this->MaskInput != 0 && this->MaskType == LabelMapMaskType)
       {
       vtkgl::ActiveTexture(vtkgl::TEXTURE8);
       this->Mask1RGBTable->Bind();
       vtkgl::ActiveTexture(vtkgl::TEXTURE9);
       this->Mask2RGBTable->Bind();
       }
    }

  float fvalue[2];
  int ivalue=0;
  v->SetUniformi("dataSetTexture",1,&ivalue);

  if(this->MaskInput!=0)
    {
    // Make the mask texture available on texture unit 7
    ivalue=7;
    v->SetUniformi("maskTexture",1,&ivalue);
    }

  if(numberOfScalarComponents==1 &&
     this->BlendMode!=vtkGPUVolumeRayCastMapper::ADDITIVE_BLEND)
    {
    ivalue=1;
    v->SetUniformi("colorTexture",1,&ivalue);

    if (this->MaskInput != 0 && this->MaskType == LabelMapMaskType)
      {
      ivalue=8;
      v->SetUniformi("mask1ColorTexture",1,&ivalue);

      ivalue=9;
      v->SetUniformi("mask2ColorTexture",1,&ivalue);

      fvalue[0]=static_cast<float>(this->MaskBlendFactor);
      v->SetUniformf("maskBlendFactor",1,fvalue);
      }

    }

  ivalue=2;
  v->SetUniformi("opacityTexture",1,&ivalue);

  // depthtexture
  vtkgl::ActiveTexture(vtkgl::TEXTURE3);
  glBindTexture(GL_TEXTURE_2D,static_cast<GLuint>(this->TextureObjects[vtkOpenGLGPUVolumeRayCastMapperTextureObjectDepthMap]));

  ivalue=3;
  v->SetUniformi("depthTexture",1,&ivalue);

  // noise texture
  vtkgl::ActiveTexture(vtkgl::TEXTURE6);
  glBindTexture(GL_TEXTURE_2D,static_cast<GLuint>(this->NoiseTextureId));

  ivalue=6;
  v->SetUniformi("noiseTexture",1,&ivalue);

  this->CheckFrameBufferStatus();

#ifdef APPLE_SNOW_LEOPARD_BUG
  this->Program->SendUniforms();
  cout << "BEFORE isValid0"  << endl;
  if(!this->Program->IsValid())
    {
    cout <<this->Program->GetLastValidateLog() << endl;
    this->Program->PrintActiveUniformVariablesOnCout();
    v->Print(cout);
    }
  cout << "AFTER isValid0"  << endl;
#endif

  if(this->NumberOfCroppingRegions>1)
    {
    // framebuffer texture
    if(rayCastMethod!=vtkOpenGLGPUVolumeRayCastMapperMethodMIP
       && rayCastMethod!=vtkOpenGLGPUVolumeRayCastMapperMethodMinIP
       && rayCastMethod!=vtkOpenGLGPUVolumeRayCastMapperMethodAdditive)
      {
      vtkgl::ActiveTexture(vtkgl::TEXTURE4);
      glBindTexture(GL_TEXTURE_2D,static_cast<GLuint>(this->TextureObjects[vtkOpenGLGPUVolumeRayCastMapperTextureObjectFrameBufferLeftFront]));

      ivalue=4;
      v->SetUniformi("frameBufferTexture",1,&ivalue);
      }
    this->CheckFrameBufferStatus();

#ifdef APPLE_SNOW_LEOPARD_BUG
    this->Program->SendUniforms();
    cout << "BEFORE isValid1"  << endl;
    if(!this->Program->IsValid())
      {
      cout <<this->Program->GetLastValidateLog() << endl;
      this->Program->PrintActiveUniformVariablesOnCout();
      v->Print(cout);
      }
    cout << "AFTER isValid1"  << endl;
#endif

    // max scalar value framebuffer texture
    if(this->BlendMode==vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND
       || this->BlendMode==vtkGPUVolumeRayCastMapper::MINIMUM_INTENSITY_BLEND
       || this->BlendMode==vtkGPUVolumeRayCastMapper::ADDITIVE_BLEND)
      {
      vtkgl::ActiveTexture(vtkgl::TEXTURE5);
      glBindTexture(GL_TEXTURE_2D,static_cast<GLuint>(this->MaxValueFrameBuffer2));

      ivalue=5;
      v->SetUniformi("scalarBufferTexture",1,&ivalue);
      }
    }

  this->CheckFrameBufferStatus();

#ifdef APPLE_SNOW_LEOPARD_BUG
  this->Program->SendUniforms();
  cout << "BEFORE isValid2"  << endl;
  if(!this->Program->IsValid())
    {
    cout <<this->Program->GetLastValidateLog() << endl;
    this->Program->PrintActiveUniformVariablesOnCout();
    v->Print(cout);
    }
  cout << "AFTER isValid2"  << endl;
#endif


  fvalue[0]=static_cast<float>(lowerLeft[0]);
  fvalue[1]=static_cast<float>(lowerLeft[1]);
  v->SetUniformf("windowLowerLeftCorner",2,fvalue);

  fvalue[0]=static_cast<float>(1.0/size[0]);
  fvalue[1]=static_cast<float>(1.0/size[1]);
  v->SetUniformf("invOriginalWindowSize",2,fvalue);

  size[0] = static_cast<int>(size[0]*this->ReductionFactor);
  size[1] = static_cast<int>(size[1]*this->ReductionFactor);

  fvalue[0]=static_cast<float>(1.0/size[0]);
  fvalue[1]=static_cast<float>(1.0/size[1]);
  v->SetUniformf("invWindowSize",2,fvalue);
  vtkOpenGLCheckErrorMacro("after uniforms for textures");


  this->CheckFrameBufferStatus();

  GLint savedFrameBuffer;
  glGetIntegerv(vtkgl::FRAMEBUFFER_BINDING_EXT,&savedFrameBuffer);
  this->SavedFrameBuffer=static_cast<unsigned int>(savedFrameBuffer);

  vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT,
                            static_cast<GLuint>(this->FrameBufferObject));

  GLenum buffer[4];
  buffer[0] = vtkgl::COLOR_ATTACHMENT0_EXT;
  if(this->NumberOfCroppingRegions>1 &&
     this->BlendMode==vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND)
    {
    // max scalar frame buffer
    buffer[1] = vtkgl::COLOR_ATTACHMENT1_EXT;
    }
  else
    {
    buffer[1] = GL_NONE;
    }

  vtkgl::DrawBuffers(2,buffer);

  this->CheckFrameBufferStatus();

  // Use by the composite+shade program
  double shininess=vol->GetProperty()->GetSpecularPower();
  if(shininess>128.0)
    {
    shininess=128.0; // upper limit for the OpenGL shininess.
    }
  glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,static_cast<GLfloat>(shininess));

  glDisable(GL_COLOR_MATERIAL); // other mapper may have enable that.

  GLfloat values[4];
  values[3]=1.0;

  values[0]=0.0;
  values[1]=values[0];
  values[2]=values[0];
  glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,values);

  values[0]=static_cast<float>(vol->GetProperty()->GetAmbient());
  values[1]=values[0];
  values[2]=values[0];
  glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,values);

  values[0]=static_cast<float>(vol->GetProperty()->GetDiffuse());
  values[1]=values[0];
  values[2]=values[0];
  glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,values);
  values[0]=static_cast<float>(vol->GetProperty()->GetSpecular());
  values[1]=values[0];
  values[2]=values[0];
  glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,values);

//  cout << "pingpong=" << this->PingPongFlag << endl;

  // To initialize the second color buffer
  vtkgl::FramebufferTexture2DEXT(vtkgl::FRAMEBUFFER_EXT,
                                 vtkgl::COLOR_ATTACHMENT0_EXT,
                                 GL_TEXTURE_2D,
                                 this->TextureObjects[vtkOpenGLGPUVolumeRayCastMapperTextureObjectFrameBufferLeftFront],
                                 0);

  vtkgl::FramebufferTexture2DEXT(vtkgl::FRAMEBUFFER_EXT,
                                 vtkgl::COLOR_ATTACHMENT0_EXT+1,
                                 GL_TEXTURE_2D,
                                 this->TextureObjects[vtkOpenGLGPUVolumeRayCastMapperTextureObjectFrameBufferLeftFront+1],
                                 0);
  buffer[0] = vtkgl::COLOR_ATTACHMENT0_EXT;
  buffer[1] = vtkgl::COLOR_ATTACHMENT1_EXT;
  vtkgl::DrawBuffers(2,buffer);

//  cout << "check before setup" << endl;
  this->CheckFrameBufferStatus();
  this->SetupRender(ren,vol);

  // restore in case of composite with no cropping or streaming.
  buffer[0] = vtkgl::COLOR_ATTACHMENT0_EXT;
  buffer[1] = GL_NONE;
  vtkgl::DrawBuffers(2,buffer);
  vtkgl::FramebufferTexture2DEXT(vtkgl::FRAMEBUFFER_EXT,
                                 vtkgl::COLOR_ATTACHMENT0_EXT+1,
                                 GL_TEXTURE_2D,0,0);
//  cout << "check after color init" << endl;
  this->CheckFrameBufferStatus();

  if(this->NumberOfCroppingRegions>1 &&
     (this->BlendMode==vtkGPUVolumeRayCastMapper::MINIMUM_INTENSITY_BLEND
      || this->BlendMode==vtkGPUVolumeRayCastMapper::MAXIMUM_INTENSITY_BLEND
      || this->BlendMode==vtkGPUVolumeRayCastMapper::ADDITIVE_BLEND))
    {
//    cout << "this->MaxValueFrameBuffer="<< this->MaxValueFrameBuffer <<endl;
//    cout << "this->MaxValueFrameBuffer2="<< this->MaxValueFrameBuffer2 <<endl;

    vtkgl::FramebufferTexture2DEXT(vtkgl::FRAMEBUFFER_EXT,
                                   vtkgl::COLOR_ATTACHMENT0_EXT,
                                   GL_TEXTURE_2D,
                                   this->MaxValueFrameBuffer,0);

    vtkgl::FramebufferTexture2DEXT(vtkgl::FRAMEBUFFER_EXT,
                                   vtkgl::COLOR_ATTACHMENT0_EXT+1,
                                   GL_TEXTURE_2D,
                                   this->MaxValueFrameBuffer2,0);

    buffer[0] = vtkgl::COLOR_ATTACHMENT0_EXT;
    buffer[1] = vtkgl::COLOR_ATTACHMENT1_EXT;
    vtkgl::DrawBuffers(2,buffer);

    if(this->BlendMode==vtkGPUVolumeRayCastMapper::MINIMUM_INTENSITY_BLEND)
      {
      glClearColor(1.0, 0.0, 0.0, 0.0);
      }
    else
      {
      // for MAXIMUM_INTENSITY_BLEND and for ADDITIVE_BLEND
      glClearColor(0.0, 0.0, 0.0, 0.0);
      }
//    cout << "check before clear on max" << endl;
    this->CheckFrameBufferStatus();
    glClear(GL_COLOR_BUFFER_BIT);
    }

  if(this->NumberOfCroppingRegions>1)
    {
    // color buffer target in the color attachement 0
    vtkgl::FramebufferTexture2DEXT(vtkgl::FRAMEBUFFER_EXT,
                                   vtkgl::COLOR_ATTACHMENT0_EXT,
                                   GL_TEXTURE_2D,
                                   this->TextureObjects[vtkOpenGLGPUVolumeRayCastMapperTextureObjectFrameBufferLeftFront],
                                   0);

    // color buffer input is on texture unit 4.
    vtkgl::ActiveTexture(vtkgl::TEXTURE4);
    glBindTexture(GL_TEXTURE_2D,this->TextureObjects[vtkOpenGLGPUVolumeRayCastMapperTextureObjectFrameBufferLeftFront+1]);

    if(this->BlendMode==vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND
       || this->BlendMode==vtkGPUVolumeRayCastMapper::MINIMUM_INTENSITY_BLEND
       || this->BlendMode==vtkGPUVolumeRayCastMapper::ADDITIVE_BLEND )
      {
      // max buffer target in the color attachment 1
      vtkgl::FramebufferTexture2DEXT(vtkgl::FRAMEBUFFER_EXT,
                                     vtkgl::COLOR_ATTACHMENT0_EXT+1,
                                     GL_TEXTURE_2D,
                                     this->MaxValueFrameBuffer,0);

      // max buffer input is on texture unit 5.
      vtkgl::ActiveTexture(vtkgl::TEXTURE5);
      glBindTexture(GL_TEXTURE_2D,this->MaxValueFrameBuffer2);
      }
    vtkgl::ActiveTexture(vtkgl::TEXTURE0);
    }

  this->CheckFrameBufferStatus();

  if(this->OpacityTables!=0 &&
     this->OpacityTables->GetNumberOfTables()!=numberOfLevels)
    {
    delete this->OpacityTables;
    this->OpacityTables=0;
    }
  if(this->OpacityTables==0)
    {
    this->OpacityTables=new vtkOpacityTables(numberOfLevels);
    }

  this->Program->Use();

  // debug code
  // DO NOT REMOVE the following commented line
  if(!this->Program->IsValid())
    {
    vtkErrorMacro(<<this->Program->GetLastValidateLog());
//    this->Program->PrintActiveUniformVariablesOnCout();
    }

  glCullFace (GL_BACK);
  // otherwise, we are rendering back face to initialize the zbuffer.


  if(!this->GeneratingCanonicalView && this->ReportProgress)
    {
    // initialize the time to avoid a progress event at the beginning.
    this->LastProgressEventTime=vtkTimerLog::GetUniversalTime();
    }

  vtkOpenGLCheckErrorMacro("failed after PreRender");
}

//-----------------------------------------------------------------------------
// Compute how each axis of a cell is projected on the viewport in pixel.
// This requires to have information about the camera and about the volume.
// It set the value of IgnoreSampleDistancePerPixel to true in case of
// degenerated case (axes aligned with the view).
//-----------------------------------------------------------------------------
double vtkOpenGLGPUVolumeRayCastMapper::ComputeMinimalSampleDistancePerPixel(
  vtkRenderer *renderer,
  vtkVolume *volume)
{
  // For each of the 3 directions of a cell, compute the step in z
  // (world coordinate, not eye/camera coordinate)
  // to go to the next pixel in x.
  // Same for the next pixel in y.
  // Keep the minimum of both zstep
  // Then keep the minimum for the 3 directions.

  // in case of either the numerator or the denominator of each ratio is 0.
  this->IgnoreSampleDistancePerPixel=true;
  double result=0.0;

  vtkMatrix4x4 *worldToDataset=volume->GetMatrix();
  vtkCamera *camera=renderer->GetActiveCamera();
  vtkMatrix4x4 *eyeToWorld=camera->GetViewTransformMatrix();
  vtkMatrix4x4 *eyeToDataset=vtkMatrix4x4::New();
  vtkMatrix4x4::Multiply4x4(eyeToWorld,worldToDataset,eyeToDataset);

  int usize;
  int vsize;
  renderer->GetTiledSize(&usize,&vsize);
  vtkMatrix4x4 *viewportToEye=camera->GetProjectionTransformMatrix(
    usize/static_cast<double>(vsize),0.0,1.0);

  double volBounds[6];
  this->GetInput()->GetBounds(volBounds);
  int dims[3];
  this->GetInput()->GetDimensions(dims);

  double v0[4];
  v0[0]=volBounds[0];
  v0[1]=volBounds[2];
  v0[2]=volBounds[4];
  v0[3]=1.0;

  double w0[4];
  eyeToDataset->MultiplyPoint(v0,w0);

  double z0;

  if(w0[3]!=0.0)
    {
    z0=w0[2]/w0[3];
    }
  else
    {
    z0=0.0;
    vtkGenericWarningMacro( "eyeToWorld transformation has some projective component." );
    }

  double p0[4];
  viewportToEye->MultiplyPoint(w0,p0);
  p0[0]/=p0[3];
  p0[1]/=p0[3];
  p0[2]/=p0[3];

  bool inFrustum=p0[0]>=-1.0 && p0[0]<=1.0 && p0[1]>=-1.0 && p0[1]<=1.0 && p0[2]>=-1.0 && p0[2]<=1.0;

  if(inFrustum)
    {
    int dim=0;
    while(dim<3)
      {
      double v1[4];
      int coord=0;
      while(coord<3)
        {
        if(coord==dim)
          {
          v1[coord]=volBounds[2*coord+1];
          }
        else
          {
          v1[coord]=volBounds[2*coord]; // same as v0[coord];
          }
        ++coord;
        }
      v1[3]=1.0;

      double w1[4];
      eyeToDataset->MultiplyPoint(v1,w1);
      double z1;

      if(w1[3]!=0.0)
        {
        z1=w1[2]/w1[3];
        }
      else
        {
        z1=0.0;
        vtkGenericWarningMacro( "eyeToWorld transformation has some projective component." );
        }


      double p1[4];
      viewportToEye->MultiplyPoint(w1,p1);
      p1[0]/=p1[3];
      p1[1]/=p1[3];
      p1[2]/=p1[3];

      inFrustum=p1[0]>=-1.0 && p1[0]<=1.0 && p1[1]>=-1.0 && p1[1]<=1.0 && p1[2]>=-1.0 && p1[2]<=1.0;

      if(inFrustum)
        {
        double dx=fabs(p1[0]-p0[0]);
        double dy=fabs(p1[1]-p0[1]);
        double dz=fabs(z1-z0);
        dz=dz/(dims[dim]-1);
        dx=dx/(dims[dim]-1)*usize;
        dy=dy/(dims[dim]-1)*vsize;

        if(dz!=0.0)
          {
          if(dx!=0.0)
            {
            double d=dz/dx;
            if(!this->IgnoreSampleDistancePerPixel)
              {
              if(result>d)
                {
                result=d;
                }
              }
            else
              {
              this->IgnoreSampleDistancePerPixel=false;
              result=d;
              }
            }

          if(dy!=0.0)
            {
            double d=dz/dy;
            if(!this->IgnoreSampleDistancePerPixel)
              {
              if(result>d)
                {
                result=d;
                }
              }
            else
              {
              this->IgnoreSampleDistancePerPixel=false;
              result=d;
              }
            }

          }
        }
      ++dim;
      }
    }

  eyeToDataset->Delete();

  if(this->IgnoreSampleDistancePerPixel)
    {
      //    cout<<"ignore SampleDistancePerPixel"<<endl;
    }
  else
    {
      //cout<<"SampleDistancePerPixel="<<result<<endl;
    }

  return result;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::RenderBlock(vtkRenderer *ren,
                                                    vtkVolume *vol,
                                                    unsigned int level)
{
  vtkImageData *input = this->GetInput();

  if(!this->AutoAdjustSampleDistances)
    {
    this->ActualSampleDistance=this->SampleDistance;
    }
  else
    {
    double datasetSpacing[3]; // from input. same for each block in streaming.
    // different for each block in AMR. Given in dataset space.

    input->GetSpacing(datasetSpacing);

    vtkMatrix4x4 *worldToDataset=vol->GetMatrix();
    double minWorldSpacing=VTK_DOUBLE_MAX;
    int i=0;
    while(i<3)
      {
      double tmp=worldToDataset->GetElement(0,i);
      double tmp2=tmp*tmp;
      tmp=worldToDataset->GetElement(1,i);
      tmp2+=tmp*tmp;
      tmp=worldToDataset->GetElement(2,i);

      // we use fabs() in case the spacing is negative.
      double worldSpacing=fabs(datasetSpacing[i]*sqrt(tmp2+tmp*tmp));
      if(worldSpacing<minWorldSpacing)
        {
        minWorldSpacing=worldSpacing;
        }
      ++i;
      }
    // minWorldSpacing is the optimal sample distance in world space.
    // To go faster (reduceFactor<1.0), we multiply this distance
    // by 1/reduceFactor.

    this->ActualSampleDistance=static_cast<float>(minWorldSpacing);

    if ( this->ReductionFactor < 1.0 )
      {
      this->ActualSampleDistance /= static_cast<GLfloat>(this->ReductionFactor*0.5);
      }
    }

  // As the opacity table table depend on the samplingdistance per block,
  // it has to be recomputed if the sample distance changed between blocks
  // of different size/level.

  vtkDataArray *scalars=this->GetScalars(input,this->ScalarMode,
                                         this->ArrayAccessMode,
                                         this->ArrayId,
                                         this->ArrayName,
                                         this->CellFlag);

  this->UpdateOpacityTransferFunction(vol,
                                      scalars->GetNumberOfComponents(),
                                      level);

  // opacitytable
  vtkgl::ActiveTexture(vtkgl::TEXTURE2);
  this->OpacityTables->GetTable(level)->Bind();
  vtkgl::ActiveTexture(vtkgl::TEXTURE0);

  vtkOpenGLCheckErrorMacro("after uniforms for projection and shade");

  // debug code
  // DO NOT REMOVE the following commented line
//  this->ValidateProgram();

  if(!this->Cropping)
    {
    this->RenderWholeVolume(ren,vol);
    }
  else
    {
    this->ClipCroppingRegionPlanes();
    this->RenderRegions(ren,vol);
    }
  vtkOpenGLCheckErrorMacro("after render");
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::PostRender(
  vtkRenderer *ren,
  int numberOfScalarComponents)
{
  vtkOpenGLClearErrorMacro();
  if(this->NumberOfCroppingRegions>1)
    {
    if(this->BlendMode==vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND
      || this->BlendMode==vtkGPUVolumeRayCastMapper::MINIMUM_INTENSITY_BLEND
      || this->BlendMode==vtkGPUVolumeRayCastMapper::ADDITIVE_BLEND)
      {
      vtkgl::ActiveTexture( vtkgl::TEXTURE5 );
      glBindTexture(GL_TEXTURE_2D,0);
      }

    if(this->LastRayCastMethod!=vtkOpenGLGPUVolumeRayCastMapperMethodMIP
       && this->LastRayCastMethod!=vtkOpenGLGPUVolumeRayCastMapperMethodMinIP
       && this->LastRayCastMethod!=vtkOpenGLGPUVolumeRayCastMapperMethodAdditive)
      {
      vtkgl::ActiveTexture( vtkgl::TEXTURE4 );
      glBindTexture(GL_TEXTURE_2D,0);
      }
    }

  // noisetexture
  vtkgl::ActiveTexture(vtkgl::TEXTURE6);
  glBindTexture(GL_TEXTURE_2D,0);

  // depthtexture
  vtkgl::ActiveTexture(vtkgl::TEXTURE3);
  glBindTexture(GL_TEXTURE_2D,0);

  // opacity
  vtkgl::ActiveTexture(vtkgl::TEXTURE2);
  glBindTexture(GL_TEXTURE_1D,0);

  if(numberOfScalarComponents==1)
    {
    vtkgl::ActiveTexture(vtkgl::TEXTURE1);
    glBindTexture(GL_TEXTURE_1D,0);
    }


  // mask, if any
  if(this->MaskInput!=0)
    {
    vtkgl::ActiveTexture(vtkgl::TEXTURE7);
    glBindTexture(vtkgl::TEXTURE_3D_EXT,0);
    }

  // back to active texture 0.
  vtkgl::ActiveTexture(vtkgl::TEXTURE0);
  glBindTexture(vtkgl::TEXTURE_3D_EXT,0);

  this->Program->Restore();

  this->CleanupRender();

  vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT,
                            static_cast<GLuint>(this->SavedFrameBuffer));
  this->SavedFrameBuffer=0;

  // Undo the viewport change we made to reduce resolution
  int size[2];
  int lowerLeft[2];
  ren->GetTiledSizeAndOrigin(size,size+1,lowerLeft,lowerLeft+1);
  glViewport(lowerLeft[0],lowerLeft[1], size[0], size[1]);
  glEnable( GL_SCISSOR_TEST );
  glScissor(lowerLeft[0],lowerLeft[1], size[0], size[1]);

  // Render the texture to the screen - this copies the offscreen buffer
  // onto the screen as a texture mapped polygon
  this->RenderTextureToScreen(ren);

  glEnable(GL_DEPTH_TEST);

  glPopAttrib(); // restore the blending mode and function

  glFinish();

  vtkOpenGLCheckErrorMacro("failed after PostRender");
}

//-----------------------------------------------------------------------------
// The main render method called from the superclass
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::GPURender(vtkRenderer *ren,
                                                  vtkVolume *vol)
{
  // We've already checked that we have input
  vtkImageData *input = this->GetTransformedInput();

  // Get the bounds of this data
  double bounds[6];
  this->GetBounds(bounds);

  // Get the scalar range. First we have to get the scalars.
  double range[2];
  vtkDataArray *scalars=this->GetScalars(input,this->ScalarMode,
                                         this->ArrayAccessMode,
                                         this->ArrayId,this->ArrayName,
                                         this->CellFlag);

  // How many components are there?
  int numberOfScalarComponents=scalars->GetNumberOfComponents();

  // If it is just one, then get the range from the scalars
  if(numberOfScalarComponents==1)
    {
    // Warning: here, we ignore the blank cells.
    scalars->GetRange(range);
    }
  // If it is 3, then use the 4th component's range since that is
  // the component that will be passed through the scalar opacity
  // transfer function to look up opacity
  else
    {
    // Note that we've already checked data type and we know this is
    // unsigned char
    scalars->GetRange(range,3);
    }

  // The rendering work has been broken into 3 stages to support AMR
  // volume rendering in blocks. Here we are simply rendering the
  // whole volume as one block. Note that if the volume is too big
  // to fix into texture memory, it will be streamed through in the
  // RenderBlock method.

  this->PreRender(ren,vol,bounds,range,numberOfScalarComponents,1);
  if(this->LoadExtensionsSucceeded)
    {
    this->RenderBlock(ren,vol,0);
    this->PostRender(ren,numberOfScalarComponents);
    }

  // If this isn't a canonical view render, then update the progress to
  // 1 because we are done.
  if (!this->GeneratingCanonicalView )
    {
    double progress=1.0;
    this->InvokeEvent(vtkCommand::VolumeMapperRenderProgressEvent,&progress);
    ren->GetRenderWindow()->MakeCurrent();
    }
}


//-----------------------------------------------------------------------------
// Render a the whole volume.
// \pre this->ProgramShader!=0 and is linked.
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::RenderWholeVolume(vtkRenderer *ren,
                                                          vtkVolume *vol)
{
  double volBounds[6];
  this->GetTransformedInput()->GetBounds(volBounds);
  this->RenderSubVolume(ren,volBounds,vol);
}


//-----------------------------------------------------------------------------
// Sort regions from front to back.
//-----------------------------------------------------------------------------
class vtkRegionDistance2
{
public:
  size_t Id; // 0<=Id<27
  // square distance between camera center to region center: >=0
  double Distance2;
};

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C" int vtkRegionComparisonFunction(const void *x,
                                           const void *y)
{
  double dx=static_cast<const vtkRegionDistance2 *>(x)->Distance2;
  double dy=static_cast<const vtkRegionDistance2 *>(y)->Distance2;

  int result;
  if(dx<dy)
    {
    result=-1;
    }
  else
    {
    if(dx>dy)
      {
      result=1;
      }
    else
      {
      result=0;
      }
    }
  return result;
}

//-----------------------------------------------------------------------------
// Render a subvolume.
// \pre this->ProgramShader!=0 and is linked.
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::RenderRegions(vtkRenderer *ren,
                                                      vtkVolume *vol)
{
  double bounds[27][6];
  double distance2[27];

  double camPos[4];
  ren->GetActiveCamera()->GetPosition(camPos);

  double volBounds[6];
  this->GetInput()->GetBounds(volBounds);


  // Pass camera through inverse volume matrix
  // so that we are in the same coordinate system
  vol->GetMatrix( this->InvVolumeMatrix );
  camPos[3] = 1.0;
  this->InvVolumeMatrix->Invert();
  this->InvVolumeMatrix->MultiplyPoint( camPos, camPos );
  if ( camPos[3] )
    {
    camPos[0] /= camPos[3];
    camPos[1] /= camPos[3];
    camPos[2] /= camPos[3];
    }

  // These are the region limits for x (first four), y (next four) and
  // z (last four). The first region limit is the lower bound for
  // that axis, the next two are the region planes along that axis, and
  // the final one in the upper bound for that axis.
  double limit[12];
  size_t i;
  for ( i = 0; i < 3; i++ )
    {
    limit[i*4  ] = volBounds[i*2];
    limit[i*4+1] = this->ClippedCroppingRegionPlanes[i*2];
    limit[i*4+2] = this->ClippedCroppingRegionPlanes[i*2+1];
    limit[i*4+3] = volBounds[i*2+1];
    }

  // For each of the 27 possible regions, find out if it is enabled,
  // and if so, compute the bounds and the distance from the camera
  // to the center of the region.
  size_t numRegions = 0;
  size_t region;
  for ( region = 0; region < 27; region++ )
    {
    int regionFlag = 1<<region;

    if ( this->CroppingRegionFlags & regionFlag )
      {
      // what is the coordinate in the 3x3x3 grid
      size_t loc[3];
      loc[0] = region%3;
      loc[1] = (region/3)%3;
      loc[2] = (region/9)%3;

      // make sure the cropping region is not empty NEW
      // otherwise, we skip the region.
      if((limit[loc[0]]!=limit[loc[0]+1])
         && (limit[loc[1]+4]!=limit[loc[1]+5])
         && (limit[loc[2]+8]!=limit[loc[2]+9]))
        {
        // compute the bounds and center
        double center[3];
        for ( i = 0; i < 3; i++ )
          {
          bounds[numRegions][i*2  ] = limit[4*i+loc[i]];
          bounds[numRegions][i*2+1] = limit[4*i+loc[i]+1];
          center[i]=(bounds[numRegions][i*2]+bounds[numRegions][i*2+1])*0.5;
          }

        // compute the distance squared to the center
        distance2[numRegions] =
          (camPos[0]-center[0])*(camPos[0]-center[0]) +
          (camPos[1]-center[1])*(camPos[1]-center[1]) +
          (camPos[2]-center[2])*(camPos[2]-center[2]);

        // we've added one region
        numRegions++;
        }
      }
    }
  vtkRegionDistance2 regions[27];

  i=0;
  while(i<numRegions)
    {
    regions[i].Id=i;
    regions[i].Distance2=distance2[i];
    ++i;
    }
  qsort(regions,numRegions,sizeof(vtkRegionDistance2),
        vtkRegionComparisonFunction);

  // loop over all regions we need to render
  int abort=0;
  i=0;
  while(!abort && i < numRegions)
    {
//    cout<<"i="<<i<<" regions[i].Id="<<regions[i].Id<<endl;
    abort=this->RenderSubVolume(ren,bounds[regions[i].Id],vol);
    ++i;
    }

}

//-----------------------------------------------------------------------------
// Computethe number of cropping regions. (0 is no cropping).
// \post positive_NumberOfCroppingRegions: this->NumberOfCroppingRegions>=0
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::ComputeNumberOfCroppingRegions()
{
  this->NumberOfCroppingRegions=0;
  if(this->Cropping)
    {
    // For each of the 27 possible regions, find out if it is enabled;
    for ( int region = 0; region < 27; region++ )
      {
      int regionFlag = 1<<region;

      if ( this->CroppingRegionFlags & regionFlag )
        {
        // we've added one region
        ++this->NumberOfCroppingRegions;
        }
      }
    }
  this->NumberOfCroppingRegions=2; // FB: DEBUG
  assert("post: positive_NumberOfCroppingRegions" &&
         this->NumberOfCroppingRegions>=0);
}

//-----------------------------------------------------------------------------
// slabsDataSet are position of the slabs in dataset coordinates.
// slabsPoints are position of the slabs in points coordinates.
// For instance, slabsDataSet[0] is the position of the plane bounding the slab
// on the left of x axis of the dataset. slabsPoints[0]=0.3 means that
// this plane lies between point 0 and point 1 along the x-axis.
// There is no clamping/clipping according to the dataset bounds so,
// slabsPoints can be negative or excess the number of points along the
// corresponding axis.
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::SlabsFromDatasetToIndex(
  double slabsDataSet[6],
  double slabsPoints[6])
{
  double *spacing=this->GetInput()->GetSpacing();
  double origin[3];

  // take spacing sign into account
  double *bds = this->GetInput()->GetBounds();
  origin[0] = bds[0];
  origin[1] = bds[2];
  origin[2] = bds[4];

  int i=0;
  while(i<6)
    {
    slabsPoints[i]=(slabsDataSet[i] - origin[i/2]) / spacing[i/2];
    ++i;
    }
}

//-----------------------------------------------------------------------------
// slabsDataSet are position of the slabs in dataset coordinates.
// slabsPoints are position of the slabs in points coordinates.
// For instance, slabsDataSet[0] is the position of the plane bounding the slab
// on the left of x axis of the dataset. slabsPoints[0]=0.3 means that
// this plane lies between point 0 and point 1 along the x-axis.
// There is no clamping/clipping according to the dataset bounds so,
// slabsPoints can be negative or excess the number of points along the
// corresponding axis.
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::SlabsFromIndexToDataset(
  double slabsPoints[6],
  double slabsDataSet[6])
{
  double *spacing=this->GetInput()->GetSpacing();
  double origin[3];

  // take spacing sign into account
  double *bds = this->GetInput()->GetBounds();
  origin[0] = bds[0];
  origin[1] = bds[2];
  origin[2] = bds[4];

  int i=0;
  while(i<6)
    {
    slabsDataSet[i]=slabsPoints[i]*spacing[i/2]+origin[i/2];
    ++i;
    }
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class vtkStreamBlock
{
public:
  double Bounds[6];
  double Extent[6];
};

//-----------------------------------------------------------------------------
// Render a subvolume. bounds are in world coordinates.
// \pre this->ProgramShader!=0 and is linked.
//-----------------------------------------------------------------------------
int vtkOpenGLGPUVolumeRayCastMapper::RenderSubVolume(vtkRenderer *ren,
                                                       double bounds[6],
                                                       vtkVolume *volume)
{
  vtkOpenGLClearErrorMacro();

  // Time to load scalar field
  size_t i;
  int wholeTextureExtent[6];
  this->GetTransformedInput()->GetExtent(wholeTextureExtent);
  if(this->CellFlag)
    {
    i=1;
    while(i<6)
      {
      wholeTextureExtent[i]--;
      i+=2;
      }
    }

  // 1. Found out the extent of the subvolume
  double realExtent[6];
  int subvolumeTextureExtent[6];

  this->SlabsFromDatasetToIndex(bounds,realExtent);

  if(this->CellFlag) // 3D texture are celldata
    {
    // texture extents are expressed in cells in this case
    i=0;
    while(i<6)
      {
      subvolumeTextureExtent[i]=vtkMath::Floor(realExtent[i]-0.5);
      ++i;
      subvolumeTextureExtent[i]=vtkMath::Floor(realExtent[i]-0.5)+1;
      ++i;
      }
    }
  else
    {
    // texture extents are expressed in points in this case
    i=0;
    while(i<6)
      {
      subvolumeTextureExtent[i]=vtkMath::Floor(realExtent[i]);
      ++i;
      subvolumeTextureExtent[i]=vtkMath::Floor(realExtent[i])+1; // used to not have +1
      ++i;
      }
    }

  i=0;
  while(i<6)
    {
    assert("check: wholeTextureExtent" && wholeTextureExtent[i]==0);
    if(subvolumeTextureExtent[i]<wholeTextureExtent[i])
      {
      subvolumeTextureExtent[i]=wholeTextureExtent[i];
      }
    ++i;
    if(subvolumeTextureExtent[i]>wholeTextureExtent[i])
      {
      subvolumeTextureExtent[i]=wholeTextureExtent[i];
      }
    ++i;
    }

  assert("check: subvolume_inside_wholevolume" &&
         subvolumeTextureExtent[0]>=wholeTextureExtent[0]
         && subvolumeTextureExtent[1]<=wholeTextureExtent[1]
         && subvolumeTextureExtent[2]>=wholeTextureExtent[2]
         && subvolumeTextureExtent[3]<=wholeTextureExtent[3]
         && subvolumeTextureExtent[4]>=wholeTextureExtent[4]
         && subvolumeTextureExtent[5]<=wholeTextureExtent[5]);

  // 2. Is this subvolume already on the GPU?
  //    ie are the extent of the subvolume inside the loaded extent?


  // Find the texture (and mask).
  std::map<vtkImageData *,vtkKWScalarField *>::iterator it=
    this->ScalarsTextures->Map.find(this->GetTransformedInput());
  vtkKWScalarField *texture;
  if(it==this->ScalarsTextures->Map.end())
    {
    texture=0;
    }
  else
    {
    texture=(*it).second;
    }

  vtkKWMask *mask=0;
  if(this->MaskInput!=0)
    {
    std::map<vtkImageData *,vtkKWMask *>::iterator it2=
      this->MaskTextures->Map.find(this->MaskInput);
    if(it2==this->MaskTextures->Map.end())
      {
      mask=0;
      }
    else
      {
      mask=(*it2).second;
      }
    }

  int loaded =
    texture!=0 &&
    texture->IsLoaded() &&
    this->GetTransformedInput()->GetMTime()<=texture->GetBuildTime() &&
    (this->GetMaskInput() ? this->GetMaskInput()->GetMTime() <= texture->GetBuildTime() : true) &&
    texture->GetLoadedCellFlag()==this->CellFlag;


  vtkIdType *loadedExtent;

  if(loaded)
    {
    loadedExtent=texture->GetLoadedExtent();
    i=0;
    while(loaded && i<6)
      {
      loaded=loaded && loadedExtent[i]<=subvolumeTextureExtent[i];
      ++i;
      loaded=loaded && loadedExtent[i]>=subvolumeTextureExtent[i];
      ++i;
      }
    }

  if(loaded)
    {
    this->CurrentScalar=texture;
    vtkgl::ActiveTexture(vtkgl::TEXTURE0);
    this->CurrentScalar->Bind();

    vtkgl::ActiveTexture(vtkgl::TEXTURE7);
    this->CurrentMask=mask;
    if(this->CurrentMask!=0)
      {
      this->CurrentMask->Bind();
      }
    }

  if(!loaded)
    {
    // 3. Not loaded: try to load the whole dataset
    if(!this->LoadScalarField(this->GetTransformedInput(),this->MaskInput,wholeTextureExtent,volume))
      {
      // 4. loading the whole dataset failed: try to load the subvolume
      if(!this->LoadScalarField(this->GetTransformedInput(),this->MaskInput, subvolumeTextureExtent,
                                volume))
        {
        // 5. loading the subvolume failed: stream the subvolume
        // 5.1 do zslabs first, if too large then cut with x or y with the
        // largest dimension. order of zlabs depends on sign of spacing[2]

        unsigned int internalFormat;
        unsigned int format;
        unsigned int type;
        int componentSize;
        this->GetTextureFormat(this->GetInput(),&internalFormat,&format,&type,
                               &componentSize);

        // Enough memory?
        int originalTextureSize[3];
        int textureSize[3];
        i=0;
        while(i<3)
          {
          textureSize[i]=subvolumeTextureExtent[2*i+1]-subvolumeTextureExtent[2*i]+1;
          originalTextureSize[i]=textureSize[i];
          ++i;
          }

        // Make sure loading did not fail because of theorical limits

        GLint width;
        glGetIntegerv(vtkgl::MAX_3D_TEXTURE_SIZE,&width);

        int clippedXY=0;
        int clippedZ=0;

        if(textureSize[0]>width)
          {
          textureSize[0]=width;
          clippedXY=1;
          }
        if(textureSize[1]>width)
          {
          textureSize[1]=width;
          clippedXY=1;
          }
        if(textureSize[2]>width)
          {
          textureSize[2]=width;
          clippedZ=1;
          }

        int minSize;
        if(this->CellFlag)
          {
          minSize=1;
          }
        else
          {
          minSize=2;
          }

        if(clippedXY)
          {
          // We cannot expect to first divide as z-slabs because it is already
          // clipped in another dimension. From now, just divide in the largest
          // dimension.
          bool foundSize=false;
          while(!foundSize && textureSize[0]>=minSize
                && textureSize[1]>=minSize)
            {
            foundSize=this->TestLoadingScalar(internalFormat,format,type,
                                              textureSize,componentSize);
            if(!foundSize)
              {
              int maxDim=0;
              if(textureSize[1]>textureSize[0])
                {
                maxDim=1;
                }
              if(textureSize[2]>textureSize[maxDim])
                {
                maxDim=2;
                }
              textureSize[maxDim]>>=1; // /=2
              }
            }
          }
        else
          {
          // we are in cropping mode, it will be slow anyway. the case we want
          // to optimize is stream the all scalar field. With that in mine,
          // it is better to first try to send z-slabs. If even a minimal
          // z-slab is too big, we have to divide by x or y dimensions. In
          // this case, it will be slow and we can choose to keep blocks as
          // square as possible by dividing by the largest dimension at each
          // iteration.

          if(!clippedZ)
            {
            // we start by subdividing only if we did not already clipped
            // the z dimension according to the theorical limits.
            textureSize[2]>>=1; // /=2
            }

          bool foundSize=false;
          while(!foundSize && textureSize[2]>=minSize)
            {
            foundSize=this->TestLoadingScalar(internalFormat,format,type,
                                              textureSize,componentSize);
            if(!foundSize)
              {
              textureSize[2]>>=1; // /=2
              }
            }
          if(!foundSize)
            {
            textureSize[2]=minSize;
            if(textureSize[0]>textureSize[1])
              {
              textureSize[0]>>=1; // /=2
              }
            else
              {
              textureSize[1]>>=1; // /=2
              }
            while(!foundSize && textureSize[0]>=minSize
                  && textureSize[1]>=minSize)
              {
              foundSize=this->TestLoadingScalar(internalFormat,format,type,
                                                textureSize,componentSize);
              if(!foundSize)
                {
                if(textureSize[0]>textureSize[1])
                  {
                  textureSize[0]>>=1; // /=2
                  }
                else
                  {
                  textureSize[1]>>=1; // /=2
                  }
                }
              }
            }
          if(!foundSize)
            {
            vtkErrorMacro(
              <<"No memory left on the GPU even for a minimal block.");
            return 1; // abort
            }
          }

        // except for the last bound.
        // front to back ordering

        // Pass camera through inverse volume matrix
        // so that we are in the same coordinate system
        double camPos[4];
        vtkCamera *cam = ren->GetActiveCamera();
        cam->GetPosition(camPos);
        volume->GetMatrix( this->InvVolumeMatrix );
        camPos[3] = 1.0;
        this->InvVolumeMatrix->Invert();
        this->InvVolumeMatrix->MultiplyPoint( camPos, camPos );
        if ( camPos[3] )
          {
          camPos[0] /= camPos[3];
          camPos[1] /= camPos[3];
          camPos[2] /= camPos[3];
          }


        // 5.2 iterate of each stream of the subvolume and render it:

        // point scalar: on the first block, the first point is not shared

        // blockExtent is always expressed in point, not in texture
        // extent.
        size_t remainder[3];
        i=0;
        while(i<3)
          {
          remainder[i]=static_cast<size_t>(
            (originalTextureSize[i]-textureSize[i])%(textureSize[i]-1));
          if(remainder[i]>0)
            {
            remainder[i]=1;
            }
          ++i;
          }

        size_t counts[3];

        counts[0]=static_cast<size_t>((originalTextureSize[0]-textureSize[0])
                                      /(textureSize[0]-1));
        counts[0]+=remainder[0]+1;
        counts[1]=static_cast<size_t>((originalTextureSize[1]-textureSize[1])
                                      /(textureSize[1]-1));
        counts[1]+=remainder[1]+1;
        counts[2]=static_cast<size_t>((originalTextureSize[2]-textureSize[2])
                                      /(textureSize[2]-1));
        counts[2]+=remainder[2]+1;

        size_t count=counts[0]*counts[1]*counts[2];

        double blockExtent[6];
        vtkStreamBlock *blocks=new vtkStreamBlock[count];
        vtkRegionDistance2 *sortedBlocks=new vtkRegionDistance2[count];

        // iterate over z,y,x
        size_t blockId=0;

        size_t zIndex=0;
        blockExtent[4]=realExtent[4];
        blockExtent[5]=vtkMath::Floor(blockExtent[4])+textureSize[2];
        if(!this->CellFlag)
          {
          blockExtent[5]--;
          }
        if(blockExtent[5]>realExtent[5])
          {
          blockExtent[5]=realExtent[5];
          }
        while(zIndex<counts[2])
          {

          blockExtent[2]=realExtent[2];
          blockExtent[3]=vtkMath::Floor(blockExtent[2])+textureSize[1];
          if(!this->CellFlag)
            {
            blockExtent[3]--;
            }
          if(blockExtent[3]>realExtent[3])
            {
            blockExtent[3]=realExtent[3];
            }
          size_t yIndex=0;
          while(yIndex<counts[1])
            {

            blockExtent[0]=realExtent[0];
            blockExtent[1]=vtkMath::Floor(blockExtent[0])+textureSize[0];
            if(!this->CellFlag)
              {
              blockExtent[1]--;
              }
            if(blockExtent[1]>realExtent[1])
              {
              blockExtent[1]=realExtent[1];
              }
            size_t xIndex=0;
            while(xIndex<counts[0])
              {
              assert("check: valid_blockId" && blockId<count);
              // save blockExtent to the list of blocks
              double blockBounds[6];
              this->SlabsFromIndexToDataset(blockExtent,blockBounds);

              // compute the bounds and center
              double center[3];
              i=0;
              while(i<3)
                {
                center[i]=(blockBounds[i*2]+blockBounds[i*2+1])*0.5;
                ++i;
                }

              // compute the distance squared to the center
              double distance2=(camPos[0]-center[0])*(camPos[0]-center[0])+
                (camPos[1]-center[1])*(camPos[1]-center[1]) +
                (camPos[2]-center[2])*(camPos[2]-center[2]);

              i=0;
              while(i<6)
                {
                blocks[blockId].Bounds[i]=blockBounds[i];
                blocks[blockId].Extent[i]=blockExtent[i];
                ++i;
                }

              sortedBlocks[blockId].Id=blockId;
              sortedBlocks[blockId].Distance2=distance2;

              ++blockId;

              blockExtent[0]=blockExtent[1];
              blockExtent[1]=blockExtent[0]+textureSize[0];
              if(!this->CellFlag)
                {
                blockExtent[1]--;
                }
              if(blockExtent[1]>realExtent[1])
                {
                blockExtent[1]=realExtent[1];
                }
              ++xIndex;
              } // while x

            blockExtent[2]=blockExtent[3];
            blockExtent[3]=blockExtent[2]+textureSize[1];
            if(!this->CellFlag)
              {
              blockExtent[3]--;
              }
            if(blockExtent[3]>realExtent[3])
              {
              blockExtent[3]=realExtent[3];
              }
            ++yIndex;
            } // while y


          blockExtent[4]=blockExtent[5];
          blockExtent[5]=blockExtent[4]+textureSize[2];
          if(!this->CellFlag)
            {
            blockExtent[5]--;
            }
          if(blockExtent[5]>realExtent[5])
            {
            blockExtent[5]=realExtent[5];
            }
          ++zIndex;
          } // while z

        assert("check: valid_number_of_blocks" && blockId==count);

        qsort(sortedBlocks,static_cast<size_t>(count),
              sizeof(vtkRegionDistance2),
              vtkRegionComparisonFunction);

        // loop over all blocks we need to render
        i=0;
        int abort=0;
        while(!abort && i < count) // 1) //count)
          {
          size_t k=sortedBlocks[i].Id;

          int blockTextureExtent[6];
          int j;
          if(this->CellFlag) // 3D texture are celldata
            {
            // texture extents are expressed in cells in this case
            j=0;
            while(j<6)
              {
              blockTextureExtent[j]=vtkMath::Floor(blocks[k].Extent[j]);
              ++j;
              }
            }
          else
            {
            // texture extents are expressed in points in this case
            j=0;
            while(j<6)
              {
              blockTextureExtent[j]=vtkMath::Floor(blocks[k].Extent[j]);
              ++j;
              blockTextureExtent[j]=vtkMath::Floor(blocks[k].Extent[j]);
              if(blockTextureExtent[j]<blocks[k].Extent[j])
                {
                ++blockTextureExtent[j];
                }
              ++j;
              }
            }

          // Load the block
          if(!this->LoadScalarField(this->GetInput(),this->MaskInput, blockTextureExtent,
                                    volume))
            {
            cout<<"Loading the streamed block FAILED!!!!!"<<endl;
            }

          loadedExtent=this->CurrentScalar->GetLoadedExtent();

          float lowBounds[3];
          float highBounds[3];
          if(!this->CurrentScalar->GetLoadedCellFlag()) // points
            {
            j=0;
            while(j<3)
              {
              double delta=
                static_cast<double>(loadedExtent[j*2+1]-loadedExtent[j*2]);
              lowBounds[j]=static_cast<float>((blocks[k].Extent[j*2]-static_cast<double>(loadedExtent[j*2]))/delta);
              highBounds[j]=static_cast<float>((blocks[k].Extent[j*2+1]-static_cast<double>(loadedExtent[j*2]))/delta);
              ++j;
              }
            }
          else // cells
            {
            j=0;
            while(j<3)
              {
              double delta=
                static_cast<double>(loadedExtent[j*2+1]-loadedExtent[j*2]);
              lowBounds[j]=static_cast<float>((blocks[k].Extent[j*2]-0.5-static_cast<double>(loadedExtent[j*2]))/delta);
              highBounds[j]=static_cast<float>((blocks[k].Extent[j*2+1]-0.5-static_cast<double>(loadedExtent[j*2]))/delta);
              ++j;
              }
            }




          // bounds have to be normalized. There are used in the shader
          // as bounds to a value used to sample a texture.

          assert("check: positive_low_bounds0" && lowBounds[0]>=0.0);
          assert("check: positive_low_bounds1" && lowBounds[1]>=0.0);
          assert("check: positive_low_bounds2" && lowBounds[2]>=0.0);

          assert("check: increasing_bounds0" && lowBounds[0]<=highBounds[0]);
          assert("check: increasing_bounds1" && lowBounds[1]<=highBounds[1]);
          assert("check: increasing_bounds2" && lowBounds[2]<=highBounds[2]);
          assert("check: high_bounds0_less_than1" && highBounds[0]<=1.0);
          assert("check: high_bounds1_less_than1" && highBounds[1]<=1.0);
          assert("check: high_bounds2_less_than1" && highBounds[2]<=1.0);

          vtkUniformVariables *v=this->Program->GetUniformVariables();
          v->SetUniformf("lowBounds",3,lowBounds);
          v->SetUniformf("highBounds",3,highBounds);

          // other sub-volume rendering code
          this->LoadProjectionParameters(ren,volume);
          this->ClipBoundingBox(ren,blocks[k].Bounds,volume);

          this->Program->SendUniforms();
          abort=this->RenderClippedBoundingBox(1,i,count,ren->GetRenderWindow());
          if (!abort)
            {
            this->CopyFBOToTexture();
            }

          ++i;
          }

        delete[] blocks;
        delete[] sortedBlocks;
        return abort;
        }
      }
    }

  loadedExtent=this->CurrentScalar->GetLoadedExtent();

  //   low bounds and high bounds are in texture coordinates.
  float lowBounds[3];
  float highBounds[3];
  if(!this->CurrentScalar->GetLoadedCellFlag()) // points
    {
    i=0;
    while(i<3)
      {
      double delta=
        static_cast<double>(loadedExtent[i*2+1]-loadedExtent[i*2]+1);
      lowBounds[i]=static_cast<float>((realExtent[i*2]+0.5-static_cast<double>(loadedExtent[i*2]))/delta);
      highBounds[i]=static_cast<float>((realExtent[i*2+1]+0.5-static_cast<double>(loadedExtent[i*2]))/delta);
      ++i;
      }
    }
  else // cells
    {
    i=0;
    while(i<3)
      {
      double delta=
        static_cast<double>(loadedExtent[i*2+1]-loadedExtent[i*2]+1);

      // this->LoadedExtent[i*2]==0, texcoord starts at 0, if realExtent==0
      // otherwise, texcoord start at 1/2N
      // this->LoadedExtent[i*2]==wholeTextureExtent[i*2+1], texcoord stops at 1, if realExtent==wholeTextureExtent[i*2+1]+1
      // otherwise it stop at 1-1/2N
      // N is the number of texels in the loadedtexture not the number of
      // texels in the whole texture.

      lowBounds[i]=static_cast<float>((realExtent[i*2]-static_cast<double>(loadedExtent[i*2]))/delta);
      highBounds[i]=static_cast<float>((realExtent[i*2+1]-static_cast<double>(loadedExtent[i*2]))/delta);
      ++i;
      }
    }

  assert("check: positive_low_bounds0" && lowBounds[0]>=0.0);
  assert("check: positive_low_bounds1" && lowBounds[1]>=0.0);
  assert("check: positive_low_bounds2" && lowBounds[2]>=0.0);

  assert("check: increasing_bounds0" && lowBounds[0]<=highBounds[0]);
  assert("check: increasing_bounds1" && lowBounds[1]<=highBounds[1]);
  assert("check: increasing_bounds2" && lowBounds[2]<=highBounds[2]);
  assert("check: high_bounds0_less_than1" && highBounds[0]<=1.0);
  assert("check: high_bounds1_less_than1" && highBounds[1]<=1.0);
  assert("check: high_bounds2_less_than1" && highBounds[2]<=1.0);

  vtkUniformVariables *v=this->Program->GetUniformVariables();
  v->SetUniformf("lowBounds",3,lowBounds);
  v->SetUniformf("highBounds",3,highBounds);

  // other sub-volume rendering code
  this->LoadProjectionParameters(ren,volume);
  this->ClipBoundingBox(ren,bounds,volume);
  this->Program->SendUniforms();
#ifdef APPLE_SNOW_LEOPARD_BUG
  if(!this->Program->IsValid())
    {
    cout << "line " << __LINE__ << " " <<
      this->Program->GetLastValidateLog() << endl;
    }
  this->Program->PrintActiveUniformVariablesOnCout();
#endif
  int abort=this->RenderClippedBoundingBox(1,0,1,ren->GetRenderWindow());
  if (!abort)
    {
    this->CopyFBOToTexture();
    }
  vtkOpenGLCheckErrorMacro("failed after RenderSubVolume");
  return abort;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::LoadProjectionParameters(
  vtkRenderer *ren,
  vtkVolume *vol)
{
  vtkMatrix4x4 *worldToDataset=vol->GetMatrix();
  vtkMatrix4x4 *datasetToWorld=this->TempMatrix[0];
  vtkMatrix4x4::Invert(worldToDataset,datasetToWorld);

  double *bounds=this->CurrentScalar->GetLoadedBounds();

  double delta[3];
  int i=0;
  while(i<3)
    {
    delta[i]=bounds[2*i+1]-bounds[2*i];
    ++i;
    }

  // worldToTexture matrix is needed

  // Compute change-of-coordinate matrix from world space to texture space.
  vtkMatrix4x4 *worldToTexture=this->TempMatrix[2];
  vtkMatrix4x4 *datasetToTexture=this->TempMatrix[1];

  // Set the matrix
  datasetToTexture->Zero();
  datasetToTexture->SetElement(0,0,delta[0]);
  datasetToTexture->SetElement(1,1,delta[1]);
  datasetToTexture->SetElement(2,2,delta[2]);
  datasetToTexture->SetElement(3,3,1.0);
  datasetToTexture->SetElement(0,3,bounds[0]);
  datasetToTexture->SetElement(1,3,bounds[2]);
  datasetToTexture->SetElement(2,3,bounds[4]);

  // worldToTexture=worldToDataSet*dataSetToTexture
  vtkMatrix4x4::Multiply4x4(worldToDataset,datasetToTexture,worldToTexture);

  // NEW
  int parallelProjection=ren->GetActiveCamera()->GetParallelProjection();

//  cout << "actualSampleDistance=" << this->ActualSampleDistance << endl;

  vtkUniformVariables *v=this->Program->GetUniformVariables();
  float fvalues[3];

  if(parallelProjection)
    {
    // Unit vector of the direction of projection in world space.
    double dirWorld[4];
    double dir[4];
    ren->GetActiveCamera()->GetDirectionOfProjection(dirWorld);
    dirWorld[3]=0.0;

    // direction in dataset space.
    datasetToWorld->MultiplyPoint(dirWorld,dir);

    // incremental vector:
    // direction in texture space times sample distance in world space.
    i=0;
    while(i<3)
      {
      dir[i]=dir[i]*this->ActualSampleDistance/delta[i];
      fvalues[i]=static_cast<float>(dir[i]);
      ++i;
      }
    v->SetUniformf("parallelRayDirection",3,fvalues);
    //cout<<"rayDir="<<dir[0]<<","<<dir[1]<<","<<dir[2]<<endl;
    }
  else
    {
    // Perspective projection

    // Compute camera position in texture coordinates
    // Position of the center of the camera in world frame
    double cameraPosWorld[4];
    // Position of the center of the camera in the dataset frame
    // (the transform of the volume is taken into account)
    double cameraPosDataset[4];
    // Position of the center of the camera in the texture frame
    // the coordinates are translated and rescaled
    double cameraPosTexture[4];

    ren->GetActiveCamera()->GetPosition(cameraPosWorld);
    cameraPosWorld[3]=1.0; // we use homogeneous coordinates.

    datasetToWorld->MultiplyPoint(cameraPosWorld,cameraPosDataset);

    // From homogeneous to cartesian coordinates.
    if(cameraPosDataset[3]!=1.0)
      {
      double ratio=1/cameraPosDataset[3];
      cameraPosDataset[0]*=ratio;
      cameraPosDataset[1]*=ratio;
      cameraPosDataset[2]*=ratio;
      }

    double *spacing=this->GetInput()->GetSpacing();
    double spacingSign[3];
    i=0;
    while(i<3)
      {
      if(spacing[i]<0)
        {
        spacingSign[i]=-1.0;
        }
      else
        {
        spacingSign[i]=1.0;
        }
      ++i;
      }

    if(this->CellFlag)
      {
      i=0;
      while(i<3)
        {
        cameraPosTexture[i] = spacingSign[i]*(cameraPosDataset[i]-bounds[i*2])/delta[i];
        ++i;
        }
      }
    else
      {
      // Initial fix by APGX (Gianluca Arcidiacono)
      // http://www.vtk.org/pipermail/vtkusers/2010-August/110978.html
      // VTKEdge Bug 8549

      vtkIdType *loadedExtent=this->CurrentScalar->GetLoadedExtent();
      i=0;
      while(i<3)
        {
        double tmp; // between 0 and 1
        tmp = spacingSign[i]*(cameraPosDataset[i] - bounds[i*2]) / delta[i];
        double delta2=static_cast<double>(
          loadedExtent[i*2+1]-loadedExtent[i*2]+1);
        cameraPosTexture[i]=(tmp*(delta2-1)+0.5)/delta2;
        ++i;
        }
      }

    // Only make sense for the vectorial part of the homogeneous matrix.
    // coefMatrix=transposeWorldToTexture*worldToTexture
    // we re-cycle the datasetToWorld pointer with a different name
    vtkMatrix4x4 *transposeWorldToTexture=this->TempMatrix[1];
    // transposeWorldToTexture={^t}worldToTexture
    vtkMatrix4x4::Transpose(worldToTexture,transposeWorldToTexture);

    vtkMatrix4x4 *coefMatrix=this->TempMatrix[1];
    vtkMatrix4x4::Multiply4x4(transposeWorldToTexture,worldToTexture,
                              coefMatrix);

    fvalues[0]=static_cast<float>(cameraPosTexture[0]);
    fvalues[1]=static_cast<float>(cameraPosTexture[1]);
    fvalues[2]=static_cast<float>(cameraPosTexture[2]);
    v->SetUniformf("cameraPosition",3,fvalues);

    fvalues[0]=this->ActualSampleDistance;
    v->SetUniformf("sampleDistance",1,fvalues);

    fvalues[0]=static_cast<float>(coefMatrix->GetElement(0,0));
    fvalues[1]=static_cast<float>(coefMatrix->GetElement(1,1));
    fvalues[2]=static_cast<float>(coefMatrix->GetElement(2,2));
    v->SetUniformf("matrix1",3,fvalues);

    fvalues[0]=static_cast<float>(2*coefMatrix->GetElement(0,1));
    fvalues[1]=static_cast<float>(2*coefMatrix->GetElement(1,2));
    fvalues[2]=static_cast<float>(2*coefMatrix->GetElement(0,2));
    v->SetUniformf("matrix2",3,fvalues);
    }

  // Change-of-coordinate matrix from Eye space to texture space.
  vtkMatrix4x4 *eyeToTexture=this->TempMatrix[1];
  vtkMatrix4x4 *eyeToWorld=ren->GetActiveCamera()->GetViewTransformMatrix();

  vtkMatrix4x4::Multiply4x4(eyeToWorld,worldToTexture,eyeToTexture);

  GLfloat matrix[16];// used sometimes as 3x3, sometimes as 4x4.
  int index;
  int column;
  int row;

  int shadeMethod=this->LastShade;

  if(shadeMethod==vtkOpenGLGPUVolumeRayCastMapperShadeYes)
    {
    index=0;
    column=0;
    while(column<3)
      {
      row=0;
      while(row<3)
        {
//        cout << "index=" << index << " row*4+column=" << row*4+column << endl;
        matrix[index]=static_cast<GLfloat>(eyeToTexture->Element[row][column]);
        ++index;
        ++row;
        }
      ++column;
      }
    v->SetUniformMatrix("eyeToTexture3",3,3,matrix);

    index=0;
    column=0;
    while(column<4)
      {
      row=0;
      while(row<4)
        {
//        cout << "index=" << index << " row*4+column=" << row*4+column << endl;
        matrix[index]=static_cast<GLfloat>(eyeToTexture->Element[row][column]);
        ++index;
        ++row;
        }
      ++column;
      }
    v->SetUniformMatrix("eyeToTexture4",4,4,matrix);
    }

  eyeToTexture->Invert();

  index=0;
  column=0;
  while(column<4)
    {
    row=0;
    while(row<4)
      {
//      cout << "index=" << index << " row*4+column=" << row*4+column << endl;
      matrix[index]=static_cast<GLfloat>(eyeToTexture->Element[row][column]);
      ++index;
      ++row;
      }
    ++column;
    }

  v->SetUniformMatrix("textureToEye",4,4,matrix);

  if(shadeMethod==vtkOpenGLGPUVolumeRayCastMapperShadeYes)
    {
    eyeToTexture->Transpose();

    index=0;
    column=0;
    while(column<3)
      {
      row=0;
      while(row<3)
        {
//        cout << "index=" << index << " row*4+column=" << row*4+column << endl;
        matrix[index]=static_cast<GLfloat>(eyeToTexture->Element[row][column]);
        ++index;
        ++row;
        }
      ++column;
      }
    v->SetUniformMatrix("transposeTextureToEye",3,3,matrix);

    float cellScale[3]; // 1/(2*Step)
    float cellStep[3]; // Step

    vtkIdType *loadedExtent=this->CurrentScalar->GetLoadedExtent();
    cellScale[0]=static_cast<float>(static_cast<double>(
                                      loadedExtent[1]-loadedExtent[0])*0.5);
    cellScale[1]=static_cast<float>(static_cast<double>(
                                      loadedExtent[3]-loadedExtent[2])*0.5);
    cellScale[2]=static_cast<float>(static_cast<double>(
                                      loadedExtent[5]-loadedExtent[4])*0.5);
    cellStep[0]=static_cast<float>(1.0/static_cast<double>(
                                     loadedExtent[1]-loadedExtent[0]));
    cellStep[1]=static_cast<float>(1.0/static_cast<double>(
                                     loadedExtent[3]-loadedExtent[2]));
    cellStep[2]=static_cast<float>(1.0/static_cast<double>(
                                     loadedExtent[5]-loadedExtent[4]));

    v->SetUniformf("cellScale",3,cellScale);
    v->SetUniformf("cellStep",3,cellStep);
    }
}

//-----------------------------------------------------------------------------
// Concatenate the header string, projection type code and method to the
// final fragment code in this->FragmentCode.
// \pre valid_raycastMethod: raycastMethod>=
//    vtkOpenGLGPUVolumeRayCastMapperMethodMIP &&
//    raycastMethod<=vtkOpenGLGPUVolumeRayCastMapperMethodMinIPFourDependent
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::BuildProgram(vtkRenderWindow *w,
                                                   int parallelProjection,
                                                   int raycastMethod,
                                                   int shadeMethod,
                                                   int componentMethod)
{

  assert("pre: valid_raycastMethod" &&
         raycastMethod>= vtkOpenGLGPUVolumeRayCastMapperMethodMIP
         && raycastMethod<=vtkOpenGLGPUVolumeRayCastMapperMethodAdditive);

  if(this->Program==0)
    {
    this->Program=vtkShaderProgram2::New();
    this->Program->SetContext(static_cast<vtkOpenGLRenderWindow *>(w));
    }

  vtkShader2Collection *shaders=this->Program->GetShaders();

  if(this->Main==0)
    {
    this->Main=vtkShader2::New();
    this->Main->SetType(VTK_SHADER_TYPE_FRAGMENT);
    this->Main->SetSourceCode(vtkGPUVolumeRayCastMapper_HeaderFS);
//      this->Main->SetSourceCode(vtkGPUVolumeRayCastMapper_DebugFS);
    shaders->AddItem(this->Main);
    }
  if(this->Projection==0)
    {
    this->Projection=vtkShader2::New();
    this->Projection->SetType(VTK_SHADER_TYPE_FRAGMENT);
    // SourceCode is postponed.
    shaders->AddItem(this->Projection);
    }
  if(this->Trace==0)
    {
    this->Trace=vtkShader2::New();
    this->Trace->SetType(VTK_SHADER_TYPE_FRAGMENT);
    // SourceCode is postponed.
    shaders->AddItem(this->Trace);
    }
  if(this->CroppingShader==0)
    {
    this->CroppingShader=vtkShader2::New();
    this->CroppingShader->SetType(VTK_SHADER_TYPE_FRAGMENT);
    // SourceCode is postponed.
    shaders->AddItem(this->CroppingShader);
    }
  if(this->Component==0)
    {
    this->Component=vtkShader2::New();
    this->Component->SetType(VTK_SHADER_TYPE_FRAGMENT);
    // SourceCode is postponed.
    // addition to collection is postponed.
    }
  if(this->Shade==0)
    {
    this->Shade=vtkShader2::New();
    this->Shade->SetType(VTK_SHADER_TYPE_FRAGMENT);
    // SourceCode is postponed.
    // addition to collection is postponed.
    }

//  cout<<"projection="<<parallelProjection<<endl;
//  cout<<"method="<<raycastMethod<<endl;
  if(parallelProjection!=this->LastParallelProjection)
    {
    this->LastParallelProjection=parallelProjection;
    // update projection
    const char *projectionCode;
    if(parallelProjection)
      {
      projectionCode=vtkGPUVolumeRayCastMapper_ParallelProjectionFS;
      }
    else
      {
      projectionCode=vtkGPUVolumeRayCastMapper_PerspectiveProjectionFS;
      }
    this->Projection->SetSourceCode(projectionCode);
    }

  if(raycastMethod!=this->LastRayCastMethod)
    {
    this->LastRayCastMethod=raycastMethod;
    // update tracing method
    const char *methodCode;
    switch(raycastMethod)
      {
      case vtkOpenGLGPUVolumeRayCastMapperMethodMIP:
        methodCode=vtkGPUVolumeRayCastMapper_MIPFS;
        break;
      case vtkOpenGLGPUVolumeRayCastMapperMethodMIPBinaryMask:
        methodCode=vtkGPUVolumeRayCastMapper_MIPBinaryMaskFS;
        break;
      case vtkOpenGLGPUVolumeRayCastMapperMethodMIPFourDependent:
        methodCode=vtkGPUVolumeRayCastMapper_MIPFourDependentFS;
        break;
      case vtkOpenGLGPUVolumeRayCastMapperMethodComposite:
        methodCode=vtkGPUVolumeRayCastMapper_CompositeFS;
        break;
      case vtkOpenGLGPUVolumeRayCastMapperMethodCompositeMask:
        methodCode=vtkGPUVolumeRayCastMapper_CompositeMaskFS;
        break;
      case vtkOpenGLGPUVolumeRayCastMapperMethodCompositeBinaryMask:
        methodCode=vtkGPUVolumeRayCastMapper_CompositeBinaryMaskFS;
        break;
      case vtkOpenGLGPUVolumeRayCastMapperMethodMinIP:
        methodCode=vtkGPUVolumeRayCastMapper_MinIPFS;
        break;
      case vtkOpenGLGPUVolumeRayCastMapperMethodMinIPBinaryMask:
        methodCode=vtkGPUVolumeRayCastMapper_MinIPBinaryMaskFS;
        break;
      case vtkOpenGLGPUVolumeRayCastMapperMethodMinIPFourDependent:
        methodCode=vtkGPUVolumeRayCastMapper_MinIPFourDependentFS;
        break;
      case vtkOpenGLGPUVolumeRayCastMapperMethodAdditive:
        methodCode=vtkGPUVolumeRayCastMapper_AdditiveFS;
        break;
      default:
        assert("check: impossible case" && 0);
        methodCode=0; // to avoid warning
        break;
      }
    this->Trace->SetSourceCode(methodCode);
    }

  // update cropping method
  int croppingMode;
  switch(raycastMethod)
    {
    case vtkOpenGLGPUVolumeRayCastMapperMethodMIP:
    case vtkOpenGLGPUVolumeRayCastMapperMethodMIPBinaryMask:
      if(this->NumberOfCroppingRegions>1)
        {
        croppingMode=vtkOpenGLGPUVolumeRayCastMapperMIPCropping;
        }
      else
        {
        croppingMode=vtkOpenGLGPUVolumeRayCastMapperMIPNoCropping;
        }
      break;
    case vtkOpenGLGPUVolumeRayCastMapperMethodMIPFourDependent:
      if(this->NumberOfCroppingRegions>1)
        {
        croppingMode=vtkOpenGLGPUVolumeRayCastMapperMIPFourDependentCropping;
        }
      else
        {
        croppingMode=
          vtkOpenGLGPUVolumeRayCastMapperMIPFourDependentNoCropping;
        }
      break;
    case vtkOpenGLGPUVolumeRayCastMapperMethodMinIP:
    case vtkOpenGLGPUVolumeRayCastMapperMethodMinIPBinaryMask:
      if(this->NumberOfCroppingRegions>1)
        {
        croppingMode=vtkOpenGLGPUVolumeRayCastMapperMinIPCropping;
        }
      else
        {
        croppingMode=vtkOpenGLGPUVolumeRayCastMapperMinIPNoCropping;
        }
      break;
    case vtkOpenGLGPUVolumeRayCastMapperMethodMinIPFourDependent:
      if(this->NumberOfCroppingRegions>1)
        {
        croppingMode=vtkOpenGLGPUVolumeRayCastMapperMinIPFourDependentCropping;
        }
      else
        {
        croppingMode=
          vtkOpenGLGPUVolumeRayCastMapperMinIPFourDependentNoCropping;
        }
      break;
    case vtkOpenGLGPUVolumeRayCastMapperMethodAdditive:
      if(this->NumberOfCroppingRegions>1)
        {
        croppingMode=vtkOpenGLGPUVolumeRayCastMapperAdditiveCropping;
        }
      else
        {
        croppingMode=vtkOpenGLGPUVolumeRayCastMapperAdditiveNoCropping;
        }
      break;
    default:
      if(this->NumberOfCroppingRegions>1)
        {
        croppingMode=vtkOpenGLGPUVolumeRayCastMapperCompositeCropping;
        }
      else
        {
        croppingMode=vtkOpenGLGPUVolumeRayCastMapperCompositeNoCropping;
        }
      break;
    }

//  cout<<"croppingMode="<<croppingMode<<endl;
  if(croppingMode!=this->LastCroppingMode)
    {
    this->LastCroppingMode=croppingMode;
    const char *croppingCode;
    switch(croppingMode)
      {
      case vtkOpenGLGPUVolumeRayCastMapperMIPCropping:
        croppingCode=vtkGPUVolumeRayCastMapper_MIPCroppingFS;
        break;
      case vtkOpenGLGPUVolumeRayCastMapperMIPNoCropping:
        croppingCode=vtkGPUVolumeRayCastMapper_MIPNoCroppingFS;
        break;
      case vtkOpenGLGPUVolumeRayCastMapperMIPFourDependentCropping:
        croppingCode=vtkGPUVolumeRayCastMapper_MIPFourDependentCroppingFS;
        break;
      case vtkOpenGLGPUVolumeRayCastMapperMIPFourDependentNoCropping:
        croppingCode=vtkGPUVolumeRayCastMapper_MIPFourDependentNoCroppingFS;
        break;
      case vtkOpenGLGPUVolumeRayCastMapperCompositeCropping:
        croppingCode=vtkGPUVolumeRayCastMapper_CompositeCroppingFS;
        break;
      case vtkOpenGLGPUVolumeRayCastMapperCompositeNoCropping:
        croppingCode=vtkGPUVolumeRayCastMapper_CompositeNoCroppingFS;
        break;
      case vtkOpenGLGPUVolumeRayCastMapperMinIPCropping:
        croppingCode=vtkGPUVolumeRayCastMapper_MinIPCroppingFS;
        break;
      case vtkOpenGLGPUVolumeRayCastMapperMinIPNoCropping:
        croppingCode=vtkGPUVolumeRayCastMapper_MinIPNoCroppingFS;
        break;
      case vtkOpenGLGPUVolumeRayCastMapperMinIPFourDependentCropping:
        croppingCode=vtkGPUVolumeRayCastMapper_MinIPFourDependentCroppingFS;
        break;
      case vtkOpenGLGPUVolumeRayCastMapperMinIPFourDependentNoCropping:
        croppingCode=vtkGPUVolumeRayCastMapper_MinIPFourDependentNoCroppingFS;
        break;
      case vtkOpenGLGPUVolumeRayCastMapperAdditiveCropping:
        croppingCode=vtkGPUVolumeRayCastMapper_AdditiveCroppingFS;
        break;
      case vtkOpenGLGPUVolumeRayCastMapperAdditiveNoCropping:
        croppingCode=vtkGPUVolumeRayCastMapper_AdditiveNoCroppingFS;
        break;
      default:
        assert("check: impossible case" && 0);
        croppingCode=0; // to avoid warning
        break;
      }
    this->CroppingShader->SetSourceCode(croppingCode);
    }

  if(componentMethod!=this->LastComponent)
    {
    if(shadeMethod==vtkOpenGLGPUVolumeRayCastMapperComponentNotUsed)
      {
      if(this->LastComponent!=
         vtkOpenGLGPUVolumeRayCastMapperComponentNotInitialized)
        {
        shaders->RemoveItem(this->Component);
        }
      }
    else
      {
      if(this->LastComponent==
         vtkOpenGLGPUVolumeRayCastMapperComponentNotInitialized ||
         this->LastComponent==
         vtkOpenGLGPUVolumeRayCastMapperComponentNotUsed)
        {
        shaders->AddItem(this->Component);
        }
      const char *componentCode;
      if(componentMethod==vtkOpenGLGPUVolumeRayCastMapperComponentOne)
        {
        componentCode=vtkGPUVolumeRayCastMapper_OneComponentFS;
        }
      else
        {
        componentCode=vtkGPUVolumeRayCastMapper_FourComponentsFS;
        }
      this->Component->SetSourceCode(componentCode);
      }
    this->LastComponent=componentMethod;
    }

  if(shadeMethod!=this->LastShade)
    {
    if(shadeMethod==vtkOpenGLGPUVolumeRayCastMapperShadeNotUsed)
      {
      if(this->LastShade!=
         vtkOpenGLGPUVolumeRayCastMapperShadeNotInitialized)
        {
        shaders->RemoveItem(this->Shade);
        }
      }
    else
      {
      if(this->LastShade==vtkOpenGLGPUVolumeRayCastMapperShadeNotInitialized
         || this->LastShade==vtkOpenGLGPUVolumeRayCastMapperShadeNotUsed)
        {
        shaders->AddItem(this->Shade);
        }
      const char *shadeCode;
      if(shadeMethod==vtkOpenGLGPUVolumeRayCastMapperShadeYes)
        {
        shadeCode=vtkGPUVolumeRayCastMapper_ShadeFS;
        }
      else
        {
        shadeCode=vtkGPUVolumeRayCastMapper_NoShadeFS;
        }
      this->Shade->SetSourceCode(shadeCode);
      }
    this->LastShade=shadeMethod;
    }
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
const char *vtkOpenGLGPUVolumeRayCastMapper::GetEnabledString(
  unsigned char value)
{
  if(value)
    {
    return "enabled";
    }
  else
    {
    return "disabled";
    }
}

//-----------------------------------------------------------------------------
// Display current OpenGL state
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::GetOpenGLState()
{
  cout<<"lighting:"<<this->GetEnabledString(glIsEnabled(GL_LIGHTING))<<endl;
  cout<<"lighting:"<<this->GetEnabledString(glIsEnabled(GL_LIGHTING))<<endl;

  // save current active texture.
  GLint value;
  glGetIntegerv(vtkgl::ACTIVE_TEXTURE,&value);
  GLenum activeTexture=static_cast<GLenum>(value);
  cout<<"active texture is "<<(activeTexture-vtkgl::TEXTURE0)<<endl;

  // iterative over all textures.

  GLenum texture=vtkgl::TEXTURE0;
  while(texture<vtkgl::TEXTURE6)
    {
    vtkgl::ActiveTexture(texture);
    cout<<"texture"<<texture-vtkgl::TEXTURE0<<endl;
    cout<<"1d:"<<GetEnabledString(glIsEnabled(GL_TEXTURE_1D))<<endl;
    cout<<"2d:"<<GetEnabledString(glIsEnabled(GL_TEXTURE_2D))<<endl;
    cout<<"3d:"<<GetEnabledString(glIsEnabled(vtkgl::TEXTURE_3D_EXT))<<endl;
    glGetIntegerv(GL_TEXTURE_BINDING_1D,&value);
    cout<<"binding 1d:"<<value<<endl;
    glGetIntegerv(GL_TEXTURE_BINDING_2D,&value);
    cout<<"binding 2d:"<<value<<endl;
    glGetIntegerv(vtkgl::TEXTURE_BINDING_3D,&value);
    cout<<"binding 3d:"<<value<<endl;
    ++texture;
    }

  // restore current active texture
  vtkgl::ActiveTexture(activeTexture);
  vtkOpenGLClearErrorMacro();
}

//-----------------------------------------------------------------------------
// Return the current OpenGL state about lighting.
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::GetLightingStatus()
{
  //cout<<"lighting: ";
  GLboolean flag=glIsEnabled(GL_LIGHTING);
  if(flag)
    {
    cout<<"enabled"<<endl;
    }
  else
    {
    cout<<"disabled"<<endl;
    }
  GLint value;
  glGetIntegerv(GL_MAX_LIGHTS,&value);
  cout<<"number of lights supported by this GPU:"<<value<<endl;

  float values[4];
  glGetFloatv(GL_LIGHT_MODEL_AMBIENT,values);

  cout<<"light model ambient="<<values[0]<<","<<values[1]<<","<<values[2]
      <<","<<values[3]<<endl;

  unsigned int i=0;
  unsigned int c=static_cast<unsigned int>(value);

  cout<<"light\t| status\t| ambient\t| diffuse\t| specular\t| position\t| spot direction\t| spot exponent\t| spot cutoff\t| k0\t| k1\t| k2"<<endl;

  while(i<c)
    {
    cout<<i<<"\t| ";
    glIsEnabled(GL_LIGHT0+i);
    if(flag)
      {
      cout<<"enabled";
      }
    else
      {
      cout<<"disabled";
      }

    glGetLightfv(GL_LIGHT0+i,GL_AMBIENT,values);
    cout<<"\t| ("<<values[0]<<","<<values[1]<<","<<values[2]<<","<<values[3];
    glGetLightfv(GL_LIGHT0+i,GL_DIFFUSE,values);
    cout<<")\t| ("<<values[0]<<","<<values[1]<<","<<values[2]<<","<<values[3];
    glGetLightfv(GL_LIGHT0+i,GL_SPECULAR,values);
    cout<<")\t| ("<<values[0]<<","<<values[1]<<","<<values[2]<<","<<values[3];
    glGetLightfv(GL_LIGHT0+i,GL_POSITION,values);
    cout<<")\t| ("<<values[0]<<","<<values[1]<<","<<values[2]<<","<<values[3];
    glGetLightfv(GL_LIGHT0+i,GL_SPOT_DIRECTION,values);
    cout<<")\t| ("<<values[0]<<","<<values[1]<<","<<values[2];
    glGetLightfv(GL_LIGHT0+i,GL_SPOT_EXPONENT,values);
    cout<<")\t| "<<values[0];
    glGetLightfv(GL_LIGHT0+i,GL_SPOT_CUTOFF,values);
    cout<<"\t| "<<values[0];
    glGetLightfv(GL_LIGHT0+i,GL_CONSTANT_ATTENUATION,values);
    cout<<"\t| "<<values[0];
    glGetLightfv(GL_LIGHT0+i,GL_LINEAR_ATTENUATION,values);
    cout<<"\t| "<<values[0];
    glGetLightfv(GL_LIGHT0+i,GL_QUADRATIC_ATTENUATION,values);
    cout<<"\t| "<<values[0]<<endl;
    ++i;
    }

  cout<<"color material=";
  flag=glIsEnabled(GL_COLOR_MATERIAL);
  if(flag)
    {
    cout<<"enabled"<<endl;
    }
  else
    {
    cout<<"disabled"<<endl;
    }

  cout<<"color material face=";
  GLint ivalue[4];
  glGetIntegerv(GL_COLOR_MATERIAL_FACE,ivalue); // 2.14.3
  switch(ivalue[0])
    {
    case GL_FRONT_AND_BACK:
      cout<<"GL_FRONT_AND_BACK";
      break;
    case GL_FRONT:
      cout<<"GL_FRONT";
      break;
    case GL_BACK:
      cout<<"GL_BACK";
      break;
    default:
      cout<<"unknown value="<<ivalue[0]<<endl;
      break;
    }

  cout<<"color material parameter=";
  glGetIntegerv(GL_COLOR_MATERIAL_PARAMETER,ivalue); // 2.14.3
  switch(ivalue[0])
    {
    case GL_AMBIENT_AND_DIFFUSE:
      cout<<"GL_AMBIENT_AND_DIFFUSE";
      break;
    case GL_AMBIENT:
      cout<<"GL_AMBIENT";
      break;
    case GL_DIFFUSE:
      cout<<"GL_DIFFUSE";
      break;
    case GL_EMISSION:
      cout<<"GL_EMISSION";
      break;
    case GL_SPECULAR:
      cout<<"GL_SPECULAR";
      break;
    default:
      cout<<"unknown value="<<ivalue[0]<<endl;
      break;
    }

  GLfloat fcolor[4];
  glGetMaterialfv(GL_FRONT,GL_EMISSION,fcolor);
  cout<<"front emission="<<fcolor[0]<<" "<<fcolor[1]<<" "<<fcolor[2]<<" "<<fcolor[3]<<endl;
  glGetMaterialfv(GL_FRONT,GL_AMBIENT,fcolor);
  cout<<"front ambient="<<fcolor[0]<<" "<<fcolor[1]<<" "<<fcolor[2]<<" "<<fcolor[3]<<endl;
  glGetMaterialfv(GL_FRONT,GL_DIFFUSE,fcolor);
  cout<<"front diffuse="<<fcolor[0]<<" "<<fcolor[1]<<" "<<fcolor[2]<<" "<<fcolor[3]<<endl;
  glGetMaterialfv(GL_FRONT,GL_SPECULAR,fcolor);
  cout<<"front specular="<<fcolor[0]<<" "<<fcolor[1]<<" "<<fcolor[2]<<" "<<fcolor[3]<<endl;
}


//-----------------------------------------------------------------------------
// Compute y=2^n such that x<=y.
// \pre positive_x: x>=0
// \post valid_result: result>=x
//-----------------------------------------------------------------------------
int vtkOpenGLGPUVolumeRayCastMapper::PowerOfTwoGreaterOrEqual(int x)
{
  assert("pre: positive_x" && x>=0);

  int result=1;
  while(result<x)
    {
    result<<=1; // *2
    }
  assert("post: valid_result" && result>=x);
  return result;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::UpdateNoiseTexture()
{
  vtkOpenGLClearErrorMacro();

  if(this->NoiseTextureId==0)
    {
    GLuint noiseTextureObject;
    glGenTextures(1,&noiseTextureObject);
    this->NoiseTextureId=static_cast<unsigned int>(noiseTextureObject);
    vtkgl::ActiveTexture(vtkgl::TEXTURE6);
    glBindTexture(GL_TEXTURE_2D,noiseTextureObject);

    GLsizei size=128; // 1024; // Power of two value
    GLint maxSize;
    const float factor=0.1f;
//    const float factor=1.0f;
    const float amplitude=0.5f*factor; // something positive.
    // amplitude=0.5. noise between -0.5 +0.5. add some +0.5 shift.

    glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maxSize);
    if(size>maxSize)
      {
      size=maxSize;
      }
    if(this->NoiseTexture!=0 && this->NoiseTextureSize!=size)
      {
      delete[] this->NoiseTexture;
      this->NoiseTexture=0;
      }
    if(this->NoiseTexture==0)
      {
      this->NoiseTexture=new float[size*size];
      this->NoiseTextureSize=size;
      vtkPerlinNoise *noiseGenerator=vtkPerlinNoise::New();
      noiseGenerator->SetFrequency(size,1.0,1.0);
      noiseGenerator->SetPhase(0.0,0.0,0.0);
      noiseGenerator->SetAmplitude(amplitude);
      int j=0;
      while(j<size)
        {
        int i=0;
        while(i<size)
          {
          this->NoiseTexture[j*size+i]=0.0; //amplitude+static_cast<float>(noiseGenerator->EvaluateFunction(i,j,0.0));
          ++i;
          }
        ++j;
        }
      noiseGenerator->Delete();
      }
    glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE,size,size,0,GL_RED,GL_FLOAT,
                 this->NoiseTexture);

    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    GLfloat borderColor[4]={0.0,0.0,0.0,0.0};
    glTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR,borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    vtkgl::ActiveTexture(vtkgl::TEXTURE0);
    }

  vtkOpenGLCheckErrorMacro("failed after UpdateNoiseTexture");
}

// ----------------------------------------------------------------------------
// Description:
// Return how much the dataset has to be reduced in each dimension to
// fit on the GPU. If the value is 1.0, there is no need to reduce the
// dataset.
// \pre the calling thread has a current OpenGL context.
// \pre mapper_supported: IsRenderSupported(renderer->GetRenderWindow(),0)
// The computation is based on hardware limits (3D texture indexable size)
// and MaxMemoryInBytes.
// \post valid_i_ratio: ratio[0]>0 && ratio[0]<=1.0
// \post valid_j_ratio: ratio[1]>0 && ratio[1]<=1.0
// \post valid_k_ratio: ratio[2]>0 && ratio[2]<=1.0
void vtkOpenGLGPUVolumeRayCastMapper::GetReductionRatio(double ratio[3])
{
  // Compute texture size
  int i;
  int wholeTextureExtent[6];
  this->GetInput()->GetExtent(wholeTextureExtent);

  // To ensure this->CellFlag is initialized.
  vtkDataArray *scalars=this->GetScalars(this->GetInput(),this->ScalarMode,
                                         this->ArrayAccessMode,
                                         this->ArrayId,
                                         this->ArrayName,
                                         this->CellFlag);

  if(this->CellFlag) // if we deal with cell data
    {
    i=1;
    while(i<6)
      {
      wholeTextureExtent[i]--;
      i+=2;
      }
    }

  // Indexable hardware limits
  GLint maxSize;
  glGetIntegerv(vtkgl::MAX_3D_TEXTURE_SIZE,&maxSize);

  vtkIdType rTextureSize[3];
  double dMaxSize=static_cast<double>(maxSize);
  i=0;
  while(i<3)
    {
    double textureSize=wholeTextureExtent[2*i+1]-wholeTextureExtent[2*i]+1;
    if(textureSize>maxSize)
      {
      ratio[i]=dMaxSize/textureSize;
      }
    else
      {
      ratio[i]=1.0; // no reduction
      }
    rTextureSize[i]=static_cast<vtkIdType>(floor(textureSize*ratio[i]));
    ++i;
    }

  // Data memory limits.
  int scalarType=scalars->GetDataType();

  vtkIdType size=rTextureSize[0]*rTextureSize[1]*rTextureSize[2]
    *vtkAbstractArray::GetDataTypeSize(scalarType)
    *scalars->GetNumberOfComponents();

  if(size>static_cast<double>(this->MaxMemoryInBytes)
     *static_cast<double>(this->MaxMemoryFraction))
    {
    double r=static_cast<double>(this->MaxMemoryInBytes)
      *static_cast<double>(this->MaxMemoryFraction)/static_cast<double>(size);
    double r3=pow(r,1.0/3.0);
    // try the keep reduction ratio uniform to avoid artifacts.
    bool reduced[3];
    i=0;
    int count=0;
    while(i<3)
      {
      vtkIdType newSize=static_cast<vtkIdType>(
        floor(static_cast<double>(rTextureSize[i])*r3));
      reduced[i]=newSize>=1;
      if(reduced[i])
        {
        ++count;
        }
      ++i;
      }

    if(count<3) // some axis cannot be reduced
      {
      double r2=sqrt(r);
      count=0;
      i=0;
      while(i<3)
        {
        if(reduced[i])
          {
          vtkIdType newSize=static_cast<vtkIdType>(
            floor(static_cast<double>(rTextureSize[i])*r2));
          reduced[i]=newSize>=1;
          if(reduced[i])
            {
            ++count;
            }
          }
        ++i;
        }
      if(count<2) // we can only reduce one axis
        {
        i=0;
        while(i<3)
          {
          if(reduced[i])
            {
            ratio[i]*=r;
            }
          ++i;
          }
        }
      else // we can reduce two axes
        {
        i=0;
        while(i<3)
          {
          if(reduced[i])
            {
            ratio[i]*=r2;
            }
          ++i;
          }
        }
      }
    else // we can reduce all three axes
      {
      i=0;
      while(i<3)
        {
        ratio[i]*=r3;
        ++i;
        }
      }
    }

  assert("post: valid_i_ratio" && ratio[0]>0 && ratio[0]<=1.0);
  assert("post: valid_j_ratio" && ratio[1]>0 && ratio[1]<=1.0);
  assert("post: valid_k_ratio" && ratio[2]>0 && ratio[2]<=1.0);
}

//-----------------------------------------------------------------------------
// Standard print method
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::PrintSelf(ostream& os,
                                                  vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
