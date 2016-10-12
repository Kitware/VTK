/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTIFFReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTIFFReader
 * @brief   read TIFF files
 *
 * vtkTIFFReader is a source object that reads TIFF files.
 * It should be able to read almost any TIFF file
 *
 * @sa
 * vtkTIFFWriter
*/

#ifndef vtkTIFFReader_h
#define vtkTIFFReader_h

#include "vtkImageReader2.h"

class VTKIOIMAGE_EXPORT vtkTIFFReader : public vtkImageReader2
{
public:
  static vtkTIFFReader *New();
  vtkTypeMacro(vtkTIFFReader, vtkImageReader2)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Is the given file name a tiff file?
   */
  virtual int CanReadFile(const char* fname);

  /**
   * Get the file extensions for this format.
   * Returns a string with a space separated list of extensions in
   * the format .extension
   */
  virtual const char* GetFileExtensions()
  {
    return ".tif .tiff";
  }

  /**
   * Return a descriptive name for the file format that might be useful
   * in a GUI.
   */
  virtual const char* GetDescriptiveName()
  {
    return "TIFF";
  }

  /**
   * Set orientation type
   * ORIENTATION_TOPLEFT         1       (row 0 top, col 0 lhs)
   * ORIENTATION_TOPRIGHT        2       (row 0 top, col 0 rhs)
   * ORIENTATION_BOTRIGHT        3       (row 0 bottom, col 0 rhs)
   * ORIENTATION_BOTLEFT         4       (row 0 bottom, col 0 lhs)
   * ORIENTATION_LEFTTOP         5       (row 0 lhs, col 0 top)
   * ORIENTATION_RIGHTTOP        6       (row 0 rhs, col 0 top)
   * ORIENTATION_RIGHTBOT        7       (row 0 rhs, col 0 bottom)
   * ORIENTATION_LEFTBOT         8       (row 0 lhs, col 0 bottom)
   * User need to explicitly include vtk_tiff.h header to have access to those #define
   */
  void SetOrientationType(unsigned int orientationType);
  vtkGetMacro(OrientationType, unsigned int)

  //@{
  /**
   * Get method to check if orientation type is specified.
   */
  vtkGetMacro(OrientationTypeSpecifiedFlag, bool)
  //@}

  //@{
  /**
   * Set/get methods to see if manual origin has been set.
   */
  vtkSetMacro(OriginSpecifiedFlag, bool)
  vtkGetMacro(OriginSpecifiedFlag, bool)
  vtkBooleanMacro(OriginSpecifiedFlag, bool)
  //@}

  //@{
  /**
   * Set/get if the spacing flag has been specified.
   */
  vtkSetMacro(SpacingSpecifiedFlag, bool)
  vtkGetMacro(SpacingSpecifiedFlag, bool)
  vtkBooleanMacro(SpacingSpecifiedFlag, bool)
  //@}

protected:
  vtkTIFFReader();
  ~vtkTIFFReader();

  enum { NOFORMAT, RGB, GRAYSCALE, PALETTE_RGB, PALETTE_GRAYSCALE, OTHER };

  virtual void ExecuteInformation();
  virtual void ExecuteDataWithInformation(vtkDataObject *out, vtkInformation *outInfo);

private:
  vtkTIFFReader(const vtkTIFFReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTIFFReader&) VTK_DELETE_FUNCTION;

  /**
   * Evaluates the image at a single pixel location.
   */
  template<typename T>
  int EvaluateImageAt(T* out, T* in);

  /**
   * Look up color paletter values.
   */
  void GetColor(int index,
                unsigned short *r, unsigned short *g, unsigned short *b);

  // To support Zeiss images
  void ReadTwoSamplesPerPixelImage(void *out,
                                   unsigned int vtkNotUsed(width),
                                   unsigned int height);

  unsigned int GetFormat();

  /**
   * Auxiliary methods used by the reader internally.
   */
  void Initialize();

  /**
   * Internal method, do not use.
   */
  template<typename T>
  void ReadImageInternal(T* buffer);

  /**
   * Reads 3D data from multi-pages tiff.
   */
  template<typename T>
  void ReadVolume(T* buffer);

  /**
   * Reads 3D data from tiled tiff
   */
  void ReadTiles(void* buffer);

  /**
   * Reads a generic image.
   */
  template<typename T>
  void ReadGenericImage(T* out, unsigned int width, unsigned int height);

  /**
   * Dispatch template to determine pixel type and decide on reader actions.
   */
  template <typename T>
  void Process(T *outPtr, int outExtent[6], vtkIdType outIncr[3]);

  /**
   * Second layer of dispatch necessary for some TIFF types.
   */
  template <typename T>
  void Process2(T *outPtr, int *outExt);

  class vtkTIFFReaderInternal;

  unsigned short *ColorRed;
  unsigned short *ColorGreen;
  unsigned short *ColorBlue;
  int TotalColors;
  unsigned int ImageFormat;
  vtkTIFFReaderInternal *InternalImage;
  int OutputExtent[6];
  vtkIdType OutputIncrements[3];
  unsigned int OrientationType;
  bool OrientationTypeSpecifiedFlag;
  bool OriginSpecifiedFlag;
  bool SpacingSpecifiedFlag;
};

#endif
