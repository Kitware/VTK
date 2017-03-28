/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLContextDevice2DPrivate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkOpenGL2ContextDevice2DPrivate
 * @brief   Private class with storage and
 * utility functions for the vtkOpenGLContextDevice2D.
 *
 *
 * This class is for internal use only, it should not be included from anything
 * outside of the vtkCharts kit. It provides a shared private class that can be
 * used by vtkOpenGLContextDevice2D and derived classes.
 *
 * @warning
 * Internal use only.
 *
 * @sa
 * vtkOpenGLContextDevice2D vtkOpenGL2ContextDevice2D
*/

#ifndef vtkOpenGLContextDevice2DPrivate_h
#define vtkOpenGLContextDevice2DPrivate_h

#include "vtkOpenGLContextDevice2D.h"

#include "vtkAbstractMapper.h"
#include "vtkColor.h"
#include "vtkFreeTypeTools.h"
#include "vtkTextProperty.h"
#include "vtkTextRenderer.h"
#include "vtkTexture.h"
#include "vtkStdString.h"
#include "vtkUnicodeString.h"

#include <algorithm>
#include <list>
#include <utility>

// .NAME vtkTextureImageCache - store vtkTexture and vtkImageData identified by
// a unique key.
// .SECTION Description
// Creating and initializing a texture can be time consuming,
// vtkTextureImageCache offers the ability to reuse them as much as possible.
template <class Key>
class vtkTextureImageCache
{
public:
  struct CacheData
  {
    vtkSmartPointer<vtkImageData> ImageData;
    vtkSmartPointer<vtkTexture>   Texture;
    // Use to generate texture coordinates. Computing this is as expensive as
    // rendering the texture, so we cache it.
    vtkTextRenderer::Metrics Metrics;
  };

  //@{
  /**
   * CacheElement associates a unique key to some cache.
   */
  struct CacheElement: public std::pair<Key, CacheData>
  {
    // Default constructor
    CacheElement()
      : std::pair<Key, CacheData>(Key(), CacheData()){}
    // Construct a partial CacheElement with no CacheData
    // This can be used for temporary CacheElement used to search a given
    // key into the cache list.
    CacheElement(const Key& key)
      : std::pair<Key, CacheData>(key, CacheData()){}
    // Standard constructor of CacheElement
    CacheElement(const Key& key, const CacheData& cacheData)
      : std::pair<Key, CacheData>(key, cacheData){}
    // Operator tuned to be used when searching into the cache list using
    // std::find()
    bool operator==(const CacheElement& other)const
    {
      // Here we cheat and make the comparison only on the key, this allows
      // us to use std::find() to search for a given key.
      return this->first == other.first;
    }
  };
  //@}

  /**
   * Construct a texture image cache with a maximum number of texture of 50.
   */
  vtkTextureImageCache()
  {
    this->MaxSize = 50;
  }

  /**
   * Search the cache list to see if a given key already exists. Returns true
   * if the key is found, false otherwise.
   */
  bool IsKeyInCache(const Key& key)const
  {
    return std::find(this->Cache.begin(), this->Cache.end(), key) != this->Cache.end();
  }

  /**
   * Return the cache associated to a key. If the key doesn't exist yet in the
   * cache list, create a new cache.
   * The returned cache is moved at the beginning of the cache list for faster
   * search next time. The most use cache is faster to be searched.
   */
  CacheData& GetCacheData(const Key& key);

  //@{
  /**
   * Release all the OpenGL Pixel Buffer Object(PBO) associated with the
   * textures of the cache list.
   */
  void ReleaseGraphicsResources(vtkWindow* window)
  {
    typename std::list<CacheElement >::iterator it;
    for (it = this->Cache.begin(); it != this->Cache.end(); ++it)
    {
      it->second.Texture->ReleaseGraphicsResources(window);
    }
  }
  //@}

protected:
  //@{
  /**
   * Add a new cache entry into the cache list. Enforce the MaxSize size of the
   * list by removing the least used cache if needed.
   */
  CacheData& AddCacheData(const Key& key, const CacheData& cacheData)
  {
    assert(!this->IsKeyInCache(key));
    if (this->Cache.size() >= this->MaxSize)
    {
      this->Cache.pop_back();
    }
    this->Cache.push_front(CacheElement(key, cacheData));
    return this->Cache.begin()->second;
  }
  //@}

  /**
   * List of a pair of key and cache data.
   */
  std::list<CacheElement > Cache;
  //@{
  /**
   * Maximum size the cache list can be.
   */
  size_t MaxSize;
};
  //@}

template<class Key>
typename vtkTextureImageCache<Key>::CacheData& vtkTextureImageCache<Key>
::GetCacheData(const Key& key)
{
  typename std::list<CacheElement>::iterator it =
    std::find(this->Cache.begin(), this->Cache.end(), CacheElement(key));
  if (it != this->Cache.end())
  {
    return it->second;
  }
  CacheData cacheData;
  cacheData.ImageData = vtkSmartPointer<vtkImageData>::New();
  cacheData.Texture = vtkSmartPointer<vtkTexture>::New();
  cacheData.Texture->SetInputData(cacheData.ImageData);
  return this->AddCacheData(key, cacheData);
}

// .NAME TextPropertyKey - unique key for a vtkTextProperty and text
// .SECTION Description
// Uniquely describe a pair of vtkTextProperty and text.
template <class StringType>
struct TextPropertyKey
{
  //@{
  /**
   * Transform a text property into an unsigned long
   */
  static vtkTypeUInt32 GetIdFromTextProperty(vtkTextProperty* tprop)
  {
    size_t id;

    vtkFreeTypeTools *ftt = vtkFreeTypeTools::GetInstance();
    ftt->MapTextPropertyToId(tprop, &id);

    // The hash is really a uint32 that gets cast to a size_t in
    // MapTextPropertyToId, so this possible truncation is safe.
    // Yay legacy APIs.
    vtkTypeUInt32 hash = static_cast<vtkTypeUInt32>(id);

    // Ensure that the above implementation assumption still holds. If it
    // doesn't we'll need to rework this cache class a bit.
    assert("Hash is really a uint32" && static_cast<size_t>(hash) == id);

    // Since we cache the text metrics (which includes orientation and alignment
    // info), we'll need to store the alignment options, since
    // MapTextPropertyToId intentionally ignores these:
    int tmp = tprop->GetJustification();
    hash = vtkFreeTypeTools::HashBuffer(&tmp, sizeof(int), hash);
    tmp = tprop->GetVerticalJustification();
    hash = vtkFreeTypeTools::HashBuffer(&tmp, sizeof(int), hash);

    return hash;
  }
  //@}

  //@{
  /**
   * Creates a TextPropertyKey.
   */
  TextPropertyKey(vtkTextProperty* textProperty, const StringType& text,
                  int dpi)
  {
    this->TextPropertyId = GetIdFromTextProperty(textProperty);
    this->FontSize = textProperty->GetFontSize();
    double color[3];
    textProperty->GetColor(color);
    this->Color.Set(static_cast<unsigned char>(color[0] * 255),
                    static_cast<unsigned char>(color[1] * 255),
                    static_cast<unsigned char>(color[2] * 255),
                    static_cast<unsigned char>(textProperty->GetOpacity() * 255));
    this->Text = text;
    this->DPI = dpi;
  }
  //@}

  /**
   * Compares two TextPropertyKeys with each other. Returns true if they are
   * identical: same text and text property
   */
  bool operator==(const TextPropertyKey& other)const
  {
    return this->TextPropertyId == other.TextPropertyId &&
      this->FontSize == other.FontSize &&
      this->Text == other.Text &&
      this->Color[0] == other.Color[0] &&
      this->Color[1] == other.Color[1] &&
      this->Color[2] == other.Color[2] &&
      this->Color[3] == other.Color[3] &&
      this->DPI == other.DPI;
  }

  unsigned short FontSize;
  vtkColor4ub Color;
  // States in the function not to use more than 32 bits - int works fine here.
  vtkTypeUInt32 TextPropertyId;
  StringType Text;
  int DPI;
};

typedef TextPropertyKey<vtkStdString> UTF8TextPropertyKey;
typedef TextPropertyKey<vtkUnicodeString> UTF16TextPropertyKey;

class vtkOpenGLContextDevice2D::Private
{
public:
  Private()
  {
    this->Texture = NULL;
    this->TextureProperties = vtkContextDevice2D::Linear |
        vtkContextDevice2D::Stretch;
    this->SpriteTexture = NULL;
    this->SavedDepthTest = GL_TRUE;
    this->SavedAlphaTest = GL_TRUE;
    this->SavedStencilTest = GL_TRUE;
    this->SavedBlend = GL_TRUE;
    this->SavedDrawBuffer = 0;
    this->SavedClearColor[0] = this->SavedClearColor[1] =
                               this->SavedClearColor[2] =
                               this->SavedClearColor[3] = 0.0f;
    this->TextCounter = 0;
    this->GLExtensionsLoaded = true;
    this->GLSL = true;
    this->PowerOfTwoTextures = false;
  }

  ~Private()
  {
    if (this->Texture)
    {
      this->Texture->Delete();
      this->Texture = NULL;
    }
    if (this->SpriteTexture)
    {
      this->SpriteTexture->Delete();
      this->SpriteTexture = NULL;
    }
  }

  void SaveGLState(bool colorBuffer = false)
  {
    this->SavedDepthTest = glIsEnabled(GL_DEPTH_TEST);

    if (colorBuffer)
    {
      this->SavedAlphaTest = glIsEnabled(GL_ALPHA_TEST);
      this->SavedStencilTest = glIsEnabled(GL_STENCIL_TEST);
      this->SavedBlend = glIsEnabled(GL_BLEND);
      glGetFloatv(GL_COLOR_CLEAR_VALUE, this->SavedClearColor);
      glGetIntegerv(GL_DRAW_BUFFER, &this->SavedDrawBuffer);
    }
  }

  void RestoreGLState(bool colorBuffer = false)
  {
    this->SetGLCapability(GL_DEPTH_TEST, this->SavedDepthTest);

    if (colorBuffer)
    {
      this->SetGLCapability(GL_ALPHA_TEST, this->SavedAlphaTest);
      this->SetGLCapability(GL_STENCIL_TEST, this->SavedStencilTest);
      this->SetGLCapability(GL_BLEND, this->SavedBlend);

      if(this->SavedDrawBuffer != GL_BACK_LEFT)
      {
        glDrawBuffer(this->SavedDrawBuffer);
      }

      int i = 0;
      bool colorDiffer = false;
      while(!colorDiffer && i < 4)
      {
        colorDiffer=this->SavedClearColor[i++] != 0.0;
      }
      if(colorDiffer)
      {
        glClearColor(this->SavedClearColor[0],
                     this->SavedClearColor[1],
                     this->SavedClearColor[2],
                     this->SavedClearColor[3]);
      }
    }
  }

  void SetGLCapability(GLenum capability, GLboolean state)
  {
    if (state)
    {
      glEnable(capability);
    }
    else
    {
      glDisable(capability);
    }
  }

  float* TexCoords(float* f, int n)
  {
    float* texCoord = new float[2*n];
    float minX = f[0]; float minY = f[1];
    float maxX = f[0]; float maxY = f[1];
    float* fptr = f;
    for(int i = 0; i < n; ++i)
    {
      minX = fptr[0] < minX ? fptr[0] : minX;
      maxX = fptr[0] > maxX ? fptr[0] : maxX;
      minY = fptr[1] < minY ? fptr[1] : minY;
      maxY = fptr[1] > maxY ? fptr[1] : maxY;
      fptr+=2;
    }
    fptr = f;
    if (this->TextureProperties & vtkContextDevice2D::Repeat)
    {
      const double* textureBounds = this->Texture->GetInput()->GetBounds();
      float rangeX = (textureBounds[1] - textureBounds[0]) ?
        textureBounds[1] - textureBounds[0] : 1.;
      float rangeY = (textureBounds[3] - textureBounds[2]) ?
        textureBounds[3] - textureBounds[2] : 1.;
      for (int i = 0; i < n; ++i)
      {
        texCoord[i*2] = (fptr[0]-minX) / rangeX;
        texCoord[i*2+1] = (fptr[1]-minY) / rangeY;
        fptr+=2;
      }
    }
    else // this->TextureProperties & vtkContextDevice2D::Stretch
    {
      float rangeX = (maxX - minX)? maxX - minX : 1.f;
      float rangeY = (maxY - minY)? maxY - minY : 1.f;
      for (int i = 0; i < n; ++i)
      {
        texCoord[i*2] = (fptr[0]-minX)/rangeX;
        texCoord[i*2+1] = (fptr[1]-minY)/rangeY;
        fptr+=2;
      }
    }
    return texCoord;
  }

  vtkVector2i FindPowerOfTwo(const vtkVector2i& size)
  {
    vtkVector2i pow2(1, 1);
    for (int i = 0; i < 2; ++i)
    {
      while (pow2[i] < size[i])
      {
        pow2[i] *= 2;
      }
    }
    return pow2;
  }

  GLuint TextureFromImage(vtkImageData *image, vtkVector2f& texCoords)
  {
    if (image->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
      vtkGenericWarningMacro("Invalid image format: expected unsigned char.");
      return 0;
    }
    int bytesPerPixel = image->GetNumberOfScalarComponents();
    int size[3];
    image->GetDimensions(size);
    vtkVector2i newImg = this->FindPowerOfTwo(vtkVector2i(size[0], size[1]));

    for (int i = 0; i < 2; ++i)
    {
      texCoords[i] = size[i] / float(newImg[i]);
    }

    unsigned char *dataPtr =
        new unsigned char[newImg[0] * newImg[1] * bytesPerPixel];
    unsigned char *origPtr =
        static_cast<unsigned char*>(image->GetScalarPointer());

    for (int i = 0; i < newImg[0]; ++i)
    {
      for (int j = 0; j < newImg[1]; ++j)
      {
        for (int k = 0; k < bytesPerPixel; ++k)
        {
          if (i < size[0] && j < size[1])
          {
            dataPtr[i * bytesPerPixel + j * newImg[0] * bytesPerPixel + k] =
                origPtr[i * bytesPerPixel + j * size[0] * bytesPerPixel + k];
          }
          else
          {
            dataPtr[i * bytesPerPixel + j * newImg[0] * bytesPerPixel + k] =
                k == 3 ? 0 : 255;
          }
        }
      }
    }

    GLuint tmpIndex(0);
    GLint glFormat = bytesPerPixel == 3 ? GL_RGB : GL_RGBA;
    GLint glInternalFormat = bytesPerPixel == 3 ? GL_RGB8 : GL_RGBA8;

    glGenTextures(1, &tmpIndex);
    glBindTexture(GL_TEXTURE_2D, tmpIndex);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                     GL_CLAMP_TO_EDGE );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                     GL_CLAMP_TO_EDGE );

    glTexImage2D(GL_TEXTURE_2D, 0 , glInternalFormat,
                 newImg[0], newImg[1], 0, glFormat,
                 GL_UNSIGNED_BYTE, static_cast<const GLvoid *>(dataPtr));
    delete [] dataPtr;
    return tmpIndex;
  }

  GLuint TextureFromImage(vtkImageData *image)
  {
    if (image->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
      cout << "Error = not an unsigned char..." << endl;
      return 0;
    }
    int bytesPerPixel = image->GetNumberOfScalarComponents();
    int size[3];
    image->GetDimensions(size);

    unsigned char *dataPtr =
        static_cast<unsigned char*>(image->GetScalarPointer());
    GLuint tmpIndex(0);
    GLint glFormat = bytesPerPixel == 3 ? GL_RGB : GL_RGBA;
    GLint glInternalFormat = bytesPerPixel == 3 ? GL_RGB8 : GL_RGBA8;

    glGenTextures(1, &tmpIndex);
    glBindTexture(GL_TEXTURE_2D, tmpIndex);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                     GL_CLAMP_TO_EDGE );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                     GL_CLAMP_TO_EDGE );

    glTexImage2D(GL_TEXTURE_2D, 0 , glInternalFormat,
                 size[0], size[1], 0, glFormat,
                 GL_UNSIGNED_BYTE, static_cast<const GLvoid *>(dataPtr));
    return tmpIndex;
  }

  vtkTexture *Texture;
  unsigned int TextureProperties;
  vtkTexture *SpriteTexture;
  // Store the previous GL state so that we can restore it when complete
  GLboolean SavedDepthTest;
  GLboolean SavedAlphaTest;
  GLboolean SavedStencilTest;
  GLboolean SavedBlend;
  GLint SavedDrawBuffer;
  GLfloat SavedClearColor[4];

  int TextCounter;
  vtkVector2i Dim;
  vtkVector2i Offset;
  bool GLExtensionsLoaded;
  bool GLSL;
  bool PowerOfTwoTextures;

  //@{
  /**
   * Cache for text images. Generating texture for strings is expensive,
   * we cache the textures here for a faster reuse.
   */
  mutable vtkTextureImageCache<UTF16TextPropertyKey> TextTextureCache;
  mutable vtkTextureImageCache<UTF8TextPropertyKey> MathTextTextureCache;
};
  //@}

///////////////////////////////////////////////////////////////////////////////////
/**
 * @class   vtkOpenGL2ContextDevice2DCellArrayHelper
 * @brief   Private class with storage and utility functions for the
 * vtkOpenGLContextDevice2D.
 *
 * This class is for internal use only, it should not be included from anything
 * outside of the vtkCharts kit. It provides a shared private class that can be
 * used by vtkOpenGLContextDevice2D and derived classes.
 *
 * The helper class is used to directly render each of the vtkCellArray instances
 * contained in a vtkPolyData object instance without the use of an external mapper.
 *
 * @warning Currently only renders two types of vtkPolyData primitives; Lines and
 * Polygons.
 *
 * @warning Internal use only.
 *
 * @sa vtkOpenGL2ContextDevice2D
*/
class vtkOpenGLContextDevice2D::CellArrayHelper
{

public:
  enum CellType
  {
    LINE = 1,
    POLYGON
    //TRIANGLE_STRIPS
  };

  CellArrayHelper (vtkOpenGLContextDevice2D* device)
  : Device(device)
  , Points(NULL)
  , PointIds(NULL)
  , Colors(NULL)
  , NumPointsCell(0)
  {
  };

  /**
   *  Draw primitives as specified by cellType.
   */
  void Draw (int cellType, vtkCellArray* cellArray, vtkPoints* points, float x,
    float y, float scale, int scalarMode, vtkUnsignedCharArray* colors = NULL)
  {
    this->Points = points;
    this->Colors = colors;
    this->CellColors->SetNumberOfComponents(colors->GetNumberOfComponents());

      switch (cellType)
      {
        case LINE:
          this->DrawLines(cellArray, scalarMode, x, y, scale);
          break;

        case POLYGON:
          this->DrawPolygons(cellArray, scalarMode, x, y, scale);
          break;
      }
  };

private:
  CellArrayHelper(const CellArrayHelper&) VTK_DELETE_FUNCTION;
  void operator=(const CellArrayHelper&) VTK_DELETE_FUNCTION;

  /**
   * Cache points and colors of the current cell in arrays.
   */
  void MapCurrentCell (float const posX, float const posY, float const scale,
    vtkIdType cellId, int scalarMode)
  {
     this->CellPoints.reserve(this->NumPointsCell * 2); /* 2 components */
     this->CellColors->SetNumberOfTuples(this->NumPointsCell); /* RGBA */
     for (int i = 0; i < this->NumPointsCell; i++)
     {
       double point[3];
       this->Points->GetPoint(this->PointIds[i], point);

       // Only 2D meshes are supported
       float const x = static_cast<float>(point[0]) + posX;
       float const y = static_cast<float>(point[1]) + posY;
       this->CellPoints.push_back(x * scale);
       this->CellPoints.push_back(y * scale);

       // Grab specific point / cell colors
       vtkIdType mappedColorId = VTK_SCALAR_MODE_USE_POINT_DATA;
       switch (scalarMode)
       {
         case VTK_SCALAR_MODE_USE_POINT_DATA:
           mappedColorId = this->PointIds[i];
           break;
         case VTK_SCALAR_MODE_USE_CELL_DATA:
           mappedColorId = cellId;
           break;
         default:
           std::cerr << "Scalar mode not supported!" << std::endl;
           break;
       }

       this->CellColors->SetTuple(i, mappedColorId, this->Colors);
     }
  };

  /**
   * Batch all of the line primitives in an array and draw them using
   * ContextDevice2D::DrawLines. The batched array is cached and only reloaded if
   * the vtkCellArray has changed.
   */
  void DrawLines(vtkCellArray* cellArray, int scalarMode, float const x,
    float const y, float const scale)
  {
    if (cellArray->GetMTime() > this->LinesLoadingTime)
    {
      this->Lines.clear();
      this->LineColors->Reset();

      // Pre-allocate batched array
      vtkIdType const numVertices = cellArray->GetNumberOfCells() * 2;// points/line
      this->Lines.reserve(numVertices * 2); // components
      this->LineColors->SetNumberOfComponents(this->Colors->GetNumberOfComponents());
      this->LineColors->SetNumberOfTuples(numVertices);

      vtkIdType cellId = 0;
      vtkIdType vertOffset = 0;
      for (cellArray->InitTraversal(); cellArray->GetNextCell(this->NumPointsCell,
        this->PointIds); cellId++)
      {
        this->MapCurrentCell(x, y, scale, cellId, scalarMode);

        // Accumulate the current cell in the batched array
        for (int i = 0; i < this->NumPointsCell; i++)
        {
          this->Lines.push_back(this->CellPoints[2 * i]);
          this->Lines.push_back(this->CellPoints[2 * i + 1]);

          double* color4 = this->CellColors->GetTuple(i);
          this->LineColors->InsertTuple4(vertOffset + i, color4[0], color4[1], color4[2],
            color4[3]);
        }

        vertOffset += this->NumPointsCell;
        this->CellColors->Reset();
        this->CellPoints.clear();
      }

      this->LinesLoadingTime.Modified();
    }

    this->Device->DrawLines(&this->Lines[0], this->Lines.size() / 2,
      static_cast<unsigned char*>(this->LineColors->GetVoidPointer(0)),
      this->LineColors->GetNumberOfComponents());
  };

  /**
   * Pre-computes the total number of polygon vertices after converted into triangles.
   * vertices to pre-allocate the batch arrays.
   */
  vtkIdType GetCountTriangleVertices(vtkCellArray* cellArray)
  {
    vtkIdType cellId = 0;
    vtkIdType numTriVert = 0;
    for (cellArray->InitTraversal(); cellArray->GetNextCell(this->NumPointsCell,
      this->PointIds); cellId++)
    {
      numTriVert += 3 * (this->NumPointsCell - 2);
    };

    return numTriVert;
  };

  /**
   * Convert all of the polygon primitives into triangles and draw them as a batch using
   * ContextDevice2D::DrawTriangles. The batched array is cached and only reloaded if
   * the vtkCellArray has changed.
   */
  void DrawPolygons(vtkCellArray* cellArray, int scalarMode, float const x,
    float const y, float const scale)
  {
    if (cellArray->GetMTime() > this->PolygonsLoadingTime)
    {
      this->PolyTri.clear();
      this->PolyColors->Reset();

      // Pre-allocate batched array
      vtkIdType const totalTriVert = this->GetCountTriangleVertices(cellArray);
      this->PolyTri.reserve(totalTriVert * 2); // components
      this->PolyColors->SetNumberOfComponents(this->Colors->GetNumberOfComponents());
      this->PolyColors->SetNumberOfTuples(totalTriVert);

      // Traverse polygons and convert to triangles
      vtkIdType cellId = 0;
      vtkIdType vertOffset = 0;
      this->PolyColors->SetNumberOfComponents(this->Colors->GetNumberOfComponents());
      for (cellArray->InitTraversal(); cellArray->GetNextCell(this->NumPointsCell,
        this->PointIds); cellId++)
      {
        this->MapCurrentCell(x, y, scale, cellId, scalarMode);

        // Convert current cell (polygon) to triangles
        for (int i = 0; i < this->NumPointsCell - 2; i++)
        {
          this->PolyTri.push_back(this->CellPoints[0]);
          this->PolyTri.push_back(this->CellPoints[1]);
          this->PolyTri.push_back(this->CellPoints[i * 2 + 2]);
          this->PolyTri.push_back(this->CellPoints[i * 2 + 3]);
          this->PolyTri.push_back(this->CellPoints[i * 2 + 4]);
          this->PolyTri.push_back(this->CellPoints[i * 2 + 5]);

          // Insert triangle vertex color
          vtkIdType const triangOffset = vertOffset + 3 * i;
          double* color4 = this->CellColors->GetTuple(0);
          this->PolyColors->InsertTuple4(triangOffset, color4[0], color4[1],
            color4[2], color4[3]);

          color4 = this->CellColors->GetTuple(i + 1);
          this->PolyColors->InsertTuple4(triangOffset + 1, color4[0], color4[1],
            color4[2], color4[3]);

          color4 = this->CellColors->GetTuple(i + 2);
          this->PolyColors->InsertTuple4(triangOffset + 2, color4[0], color4[1],
            color4[2], color4[3]);
        }

        vertOffset += 3 * (this->NumPointsCell - 2); // Triangle verts current cell
        this->CellColors->Reset();
        this->CellPoints.clear();
      }

      this->PolygonsLoadingTime.Modified();
    }

    this->Device->CoreDrawTriangles(this->PolyTri,
      static_cast<unsigned char*>(this->PolyColors->GetVoidPointer(0)), 4);
  };

  vtkOpenGLContextDevice2D* Device;

  vtkPoints* Points;
  vtkIdType* PointIds;
  vtkUnsignedCharArray* Colors;

  //@{
  /**
   *  Current vtkPolyData cell.
   */
  vtkIdType NumPointsCell;
  std::vector<float> CellPoints;
  vtkNew<vtkUnsignedCharArray> CellColors;
  //@}

  //@{
  /**
   *  Cached polygon primitives (as triangles).
   */
  std::vector<float> PolyTri;
  vtkNew<vtkUnsignedCharArray> PolyColors;
  vtkTimeStamp PolygonsLoadingTime;
  //@}

  //@{
  /**
   *  Cached line primitives.
   */
  std::vector<float> Lines;
  vtkNew<vtkUnsignedCharArray> LineColors;
  vtkTimeStamp LinesLoadingTime;
  //@}
};
#endif // VTKOPENGLCONTEXTDEVICE2DPRIVATE_H
// VTK-HeaderTest-Exclude: vtkOpenGLContextDevice2DPrivate.h
