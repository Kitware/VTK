/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTIFFReader.h,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTIFFReader - read TIFF files
// .SECTION Description
// vtkTIFFReader is a source object that reads TIFF files.
// It should be able to read almost any TIFF file
//
// .SECTION See Also
// vtkTIFFWriter

#ifndef __vtkTIFFReader_h
#define __vtkTIFFReader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageReader2.h"

//BTX
class vtkTIFFReaderInternal;
//ETX

class VTKIOIMAGE_EXPORT vtkTIFFReader : public vtkImageReader2
{
public:
  static vtkTIFFReader *New();
  vtkTypeMacro(vtkTIFFReader,vtkImageReader2);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Is the given file name a tiff file file?
  virtual int CanReadFile(const char* fname);

  // Description:
  // Get the file extensions for this format.
  // Returns a string with a space separated list of extensions in
  // the format .extension
  virtual const char* GetFileExtensions()
    {
    return ".tif .tiff";
    }

  // Description:
  // Return a descriptive name for the file format that might be useful
  // in a GUI.
  virtual const char* GetDescriptiveName()
    {
    return "TIFF";
    }

  // Description:
  // Auxiliary methods used by the reader internally.
  void InitializeColors();

  // Description:
  // Reads 3D data from multi-pages tiff.
  virtual void ReadVolume(void* buffer);

  // Description:
  // Reads 3D data from tiled tiff
  virtual void ReadTiles(void* buffer);

  // Description:
  // Set orientation type
  // ORIENTATION_TOPLEFT         1       (row 0 top, col 0 lhs)
  // ORIENTATION_TOPRIGHT        2       (row 0 top, col 0 rhs)
  // ORIENTATION_BOTRIGHT        3       (row 0 bottom, col 0 rhs)
  // ORIENTATION_BOTLEFT         4       (row 0 bottom, col 0 lhs)
  // ORIENTATION_LEFTTOP         5       (row 0 lhs, col 0 top)
  // ORIENTATION_RIGHTTOP        6       (row 0 rhs, col 0 top)
  // ORIENTATION_RIGHTBOT        7       (row 0 rhs, col 0 bottom)
  // ORIENTATION_LEFTBOT         8       (row 0 lhs, col 0 bottom)
  // User need to explicitly include vtk_tiff.h header to have access to those #define
  void SetOrientationType( unsigned int orientationType );
  vtkGetMacro( OrientationType, unsigned int );

  // Description:
  // Get method to check if orientation type is specified
  vtkGetMacro( OrientationTypeSpecifiedFlag, bool );

  // Description:
  // Set/get methods to see if manual Origin/Spacing have
  // been set.
  vtkSetMacro( OriginSpecifiedFlag, bool );
  vtkGetMacro( OriginSpecifiedFlag, bool );
  vtkBooleanMacro( OriginSpecifiedFlag, bool );

  // Description:
  //
  vtkSetMacro( SpacingSpecifiedFlag, bool );
  vtkGetMacro( SpacingSpecifiedFlag, bool );
  vtkBooleanMacro( SpacingSpecifiedFlag, bool );

  // Description:
  // Internal method, do not use.
  void ReadImageInternal( void *, void *outPtr,
                          int *outExt, unsigned int size );

protected:
  vtkTIFFReader();
  ~vtkTIFFReader();

  enum { NOFORMAT, RGB, GRAYSCALE, PALETTE_RGB, PALETTE_GRAYSCALE, OTHER };

  vtkTIFFReaderInternal *GetInternalImage() { return this->InternalImage; }

  int EvaluateImageAt( void*, void* );

  void GetColor( int index,
                 unsigned short *r, unsigned short *g, unsigned short *b );

  void ReadGenericImage( void *out,
                         unsigned int vtkNotUsed(width),
                         unsigned int height );

  // To support Zeiss images
  void ReadTwoSamplesPerPixelImage( void *out,
                         unsigned int vtkNotUsed(width),
                         unsigned int height );

  unsigned int  GetFormat();
  virtual void ExecuteInformation();
  virtual void ExecuteDataWithInformation(vtkDataObject *out, vtkInformation *outInfo);

private:
  vtkTIFFReader(const vtkTIFFReader&);  // Not implemented.
  void operator=(const vtkTIFFReader&);  // Not implemented.

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
