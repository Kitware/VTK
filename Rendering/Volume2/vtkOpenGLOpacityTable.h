#ifndef __vtkOpenGLVolumeOpacityTable_h_
#define __vtkOpenGLVolumeOpacityTable_h_

#include <vtkPiecewiseFunction.h>
#include <vtkVolumeMapper.h>

#include <GL/glew.h>
#include <vtkgl.h>

///
/// \brief The vtkOpenGLOpacityTable class
///----------------------------------------------------------------------------
class vtkOpenGLOpacityTable
{
public:
  ///
  /// \brief Constructor.
  ///--------------------------------------------------------------------------
  vtkOpenGLOpacityTable(int width = 1024)
    {
      this->TextureId = 0;
      this->LastBlendMode = vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND;
      this->TextureWidth = width;
      this->TextureHeight = 0;
      this->LastSampleDistance = 1.0;
      this->Table = 0;
      this->Loaded = false;
      this->LastLinearInterpolation = false;
      this->LastRange[0] = this->LastRange[1] = 0.0;
    }

  ///
  /// \brief Destructor.
  ///--------------------------------------------------------------------------
  ~vtkOpenGLOpacityTable()
    {
      if (this->TextureId != 0)
        {
        glDeleteTextures(1, &this->TextureId);
        this->TextureId=0;
        }

      if (this->Table!=0)
        {
        delete[] this->Table;
        this->Table=0;
        }
    }

  ///
  /// \brief Check if opacity transfer function texture is loaded.
  /// \return
  ///--------------------------------------------------------------------------
  bool IsLoaded()
    {
    return this->Loaded;
    }

  ///
  /// \brief Bind texture.
  ///--------------------------------------------------------------------------
  void Bind()
    {
    /// Activate texture 2
    glActiveTexture(GL_TEXTURE2);

    glBindTexture(GL_TEXTURE_1D, this->TextureId);
    }

  ///
  /// \brief Update opacity tranfer function texture.
  /// \param scalarOpacity
  /// \param blendMode
  /// \param sampleDistance
  /// \param range
  /// \param unitDistance
  /// \param linearInterpolation
  ///--------------------------------------------------------------------------
  void Update(vtkPiecewiseFunction* scalarOpacity,
              int blendMode,
              double sampleDistance,
              double range[2],
              double unitDistance,
              bool linearInterpolation)
    {
    /// Activate texture 2
    glActiveTexture(GL_TEXTURE2);


    bool needUpdate=false;
    if(this->TextureId == 0)
      {
      glGenTextures(1,&this->TextureId);
      needUpdate = true;
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
       (this->LastBlendMode != blendMode) ||
       (blendMode == vtkVolumeMapper::COMPOSITE_BLEND &&
        this->LastSampleDistance != sampleDistance) ||
       needUpdate || !this->Loaded)
      {
      this->Loaded = false;
      if(this->Table == 0)
        {
        this->Table = new float[this->TextureWidth];
        }

      scalarOpacity->GetTable(range[0], range[1], this->TextureWidth, this->Table);
      this->LastBlendMode = blendMode;

      /// Correct the opacity array for the spacing between the planes if we
      /// are using a composite blending operation
      /// TODO Fix this code for sample distance in three dimensions
//        if(blendMode == vtkVolumeMapper::COMPOSITE_BLEND)
//          {
//          float* ptr = this->Table;
//          double factor = sampleDistance[0]/unitDistance;
//          int i=0;
//          while(i < this->TextureWidth)
//            {
//            if(*ptr > 0.0001f)
//              {
//              *ptr = static_cast<float>(1.0-pow(1.0-static_cast<double>(*ptr),
//                                        factor));
//              }
//            ++ptr;
//            ++i;
//            }
//          this->LastSampleDistance[0] = sampleDistance[0];
//          this->LastSampleDistance[1] = sampleDistance[1];
//          this->LastSampleDistance[2] = sampleDistance[2];
//          }
//        else if (blendMode==vtkVolumeMapper::ADDITIVE_BLEND)
//          {
//          float* ptr = this->Table;
//          double factor = sampleDistance[0]/unitDistance;
//          int i = 0;
//          while( i < this->TextureWidth)
//            {
//            if(*ptr > 0.0001f)
//              {
//              *ptr = static_cast<float>(static_cast<double>(*ptr)*factor);
//              }
//            ++ptr;
//            ++i;
//            }
//          this->LastSampleDistance[0] = sampleDistance[0];
//          this->LastSampleDistance[1] = sampleDistance[1];
//          this->LastSampleDistance[2] = sampleDistance[2];
//          }

      glTexImage1D(GL_TEXTURE_1D, 0, GL_ALPHA16, this->TextureWidth,
                   this->TextureHeight, GL_ALPHA, GL_FLOAT, this->Table);
      this->Loaded = true;
      this->BuildTime.Modified();
      }

    needUpdate= needUpdate ||
      this->LastLinearInterpolation!=linearInterpolation;
    if(needUpdate)
      {
      this->LastLinearInterpolation = linearInterpolation;
      GLint value = linearInterpolation ? GL_LINEAR : GL_NEAREST;
      glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, value);
      glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, value);
      }
    }

protected:
  GLuint TextureId;
  int LastBlendMode;
  int TextureWidth;
  int TextureHeight;

  double LastSampleDistance;
  vtkTimeStamp BuildTime;
  float *Table;
  bool Loaded;
  bool LastLinearInterpolation;
  double LastRange[2];
private:
  vtkOpenGLOpacityTable(const vtkOpenGLOpacityTable&);
  vtkOpenGLOpacityTable& operator=(const vtkOpenGLOpacityTable&);
};

//-----------------------------------------------------------------------------
///
/// \brief The vtkOpenGLOpacityTables class
///
class vtkOpenGLOpacityTables
{
public:
  ///
  /// \brief Constructor.
  /// \param numberOfTables
  ///--------------------------------------------------------------------------
  vtkOpenGLOpacityTables(unsigned int numberOfTables)
    {
    this->Tables = new vtkOpenGLOpacityTable[numberOfTables];
    this->NumberOfTables = numberOfTables;
    }

  ///
  /// \brief Destructor.
  ///--------------------------------------------------------------------------
  ~vtkOpenGLOpacityTables()
    {
    delete [] this->Tables;
    }

  ///
  /// \brief Get opacity table at a given index.
  /// \param i
  /// \return
  ///--------------------------------------------------------------------------
  vtkOpenGLOpacityTable* GetTable(unsigned int i)
    {
    return &this->Tables[i];
    }

  ///
  /// \brief Get number of tables.
  /// \return
  ///--------------------------------------------------------------------------
  unsigned int GetNumberOfTables()
    {
    return this->NumberOfTables;
    }

private:
  unsigned int NumberOfTables;
  vtkOpenGLOpacityTable *Tables;

  ///
  /// \brief vtkOpenGLOpacityTables (Not implemented)
  ///
  vtkOpenGLOpacityTables();

  ///
  /// \brief vtkOpenGLOpacityTables (Not implemented)
  /// \param other
  ///
  vtkOpenGLOpacityTables(const vtkOpenGLOpacityTables &other);

  ///
  /// \brief operator = (Not implemented)
  /// \param other
  /// \return
  ///
  vtkOpenGLOpacityTables &operator=(const vtkOpenGLOpacityTables &other);
};

#endif // __vtkOpenGLVolumeOpacityTable_h_
