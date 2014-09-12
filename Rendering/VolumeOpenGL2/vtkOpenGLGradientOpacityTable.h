#ifndef __vtkOpenGLGradientOpacityTable_h_
#define __vtkOpenGLGradientOpacityTable_h_

#include <vtkPiecewiseFunction.h>
#include <vtkVolumeMapper.h>

#include <GL/glew.h>
#include <vtkgl.h>

//
// The vtkOpenGLGradientOpacityTable class
//----------------------------------------------------------------------------
class vtkOpenGLGradientOpacityTable
{
public:
  //--------------------------------------------------------------------------
  vtkOpenGLGradientOpacityTable(int width = 1024)
    {
      this->TextureId = 0;
      this->TextureWidth = width;
      this->TextureHeight = 0;
      this->LastSampleDistance = 1.0;
      this->Table = 0;
      this->Loaded = false;
      this->LastLinearInterpolation = false;
      this->LastRange[0] = this->LastRange[1] = 0.0;
    }

  //--------------------------------------------------------------------------
  ~vtkOpenGLGradientOpacityTable()
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


  // Check if opacity transfer function texture is loaded.
  //--------------------------------------------------------------------------
  bool IsLoaded()
    {
    return this->Loaded;
    }

  // Bind texture.
  //--------------------------------------------------------------------------
  void Bind()
    {
    // Activate texture 5
    glActiveTexture(GL_TEXTURE5);

    glBindTexture(GL_TEXTURE_1D, this->TextureId);
    }

  // Update opacity tranfer function texture.
  // \param gradientOpacity
  // \param blendMode
  // \param sampleDistance
  // \param range
  // \param unitDistance
  // \param linearInterpolation
  //--------------------------------------------------------------------------
  void Update(vtkPiecewiseFunction* gradientOpacity,
              double sampleDistance,
              double range[2],
              double vtkNotUsed(unitDistance),
              bool linearInterpolation)
    {
    // Activate texture 5
    glActiveTexture(GL_TEXTURE5);

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
                      GL_CLAMP_TO_EDGE);
      }

    if(gradientOpacity->GetMTime() > this->BuildTime ||
       this->LastSampleDistance != sampleDistance ||
       needUpdate || !this->Loaded)
      {
      this->Loaded = false;
      if(this->Table == 0)
        {
        this->Table = new float[this->TextureWidth];
        }

      gradientOpacity->GetTable(0, (range[1] - range[0]) * 0.25,
        this->TextureWidth, this->Table);

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
  int TextureWidth;
  int TextureHeight;

  double LastSampleDistance;
  vtkTimeStamp BuildTime;
  float* Table;
  bool Loaded;
  bool LastLinearInterpolation;
  double LastRange[2];
private:
  vtkOpenGLGradientOpacityTable(const vtkOpenGLGradientOpacityTable&);
  vtkOpenGLGradientOpacityTable& operator=(const vtkOpenGLGradientOpacityTable&);
};

// The vtkOpenGLGradientOpacityTables class
//-----------------------------------------------------------------------------
class vtkOpenGLGradientOpacityTables
{
public:
  // Constructor
  //--------------------------------------------------------------------------
  vtkOpenGLGradientOpacityTables(unsigned int numberOfTables)
    {
    this->Tables = new vtkOpenGLGradientOpacityTable[numberOfTables];
    this->NumberOfTables = numberOfTables;
    }

  // Destructor
  //--------------------------------------------------------------------------
  ~vtkOpenGLGradientOpacityTables()
    {
    delete [] this->Tables;
    }

  // Get opacity table at a given index.
  //--------------------------------------------------------------------------
  vtkOpenGLGradientOpacityTable* GetTable(unsigned int i)
    {
    return &this->Tables[i];
    }

  // Get number of tables.
  //--------------------------------------------------------------------------
  unsigned int GetNumberOfTables()
    {
    return this->NumberOfTables;
    }

private:
  unsigned int NumberOfTables;
  vtkOpenGLGradientOpacityTable* Tables;

  // vtkOpenGLGradientOpacityTables (Not implemented)
  vtkOpenGLGradientOpacityTables();

  // vtkOpenGLGradientOpacityTables (Not implemented)
  vtkOpenGLGradientOpacityTables(const vtkOpenGLGradientOpacityTables &other);

  // operator = (Not implemented)
  vtkOpenGLGradientOpacityTables &operator=(const vtkOpenGLGradientOpacityTables &other);
};

#endif // __vtkOpenGLGradientOpacityTable_h_
