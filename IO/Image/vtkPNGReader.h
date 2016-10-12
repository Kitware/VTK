/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNGReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPNGReader
 * @brief   read PNG files
 *
 * vtkPNGReader is a source object that reads PNG files.
 * It should be able to read most any PNG file
 *
 * @sa
 * vtkPNGWriter
*/

#ifndef vtkPNGReader_h
#define vtkPNGReader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageReader2.h"

class VTKIOIMAGE_EXPORT vtkPNGReader : public vtkImageReader2
{
public:
  static vtkPNGReader *New();
  vtkTypeMacro(vtkPNGReader,vtkImageReader2);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Is the given file a PNG file?
   */
  virtual int CanReadFile(const char* fname);

  /**
   * Get the file extensions for this format.
   * Returns a string with a space separated list of extensions in
   * the format .extension
   */
  virtual const char* GetFileExtensions()
  {
      return ".png";
  }

  /**
   * Return a descriptive name for the file format that might be useful in a GUI.
   */
  virtual const char* GetDescriptiveName()
  {
      return "PNG";
  }

  /**
   * Given a 'key' for the text chunks, fills in 'beginEndIndex'
   * with the begin and end indexes. Values are stored between
   * [begin, end) indexes.
   */
  void GetTextChunks(const char* key, int beginEndIndex[2]);
  /**
   * Returns the text key stored at 'index'.
   */
  const char* GetTextKey(int index);
  /**
   * Returns the text value stored at 'index'. A range of indexes
   * that store values for a certain key can be obtained by calling
   * GetTextChunks.
   */
  const char* GetTextValue(int index);
  /**
   * Return the number of text chunks in the PNG file.
   * Note that we don't process compressed or international text entries
   */
  size_t GetNumberOfTextChunks();

protected:
  vtkPNGReader();
  ~vtkPNGReader();

  virtual void ExecuteInformation();
  virtual void ExecuteDataWithInformation(vtkDataObject *out, vtkInformation *outInfo);
  template <class OT>
    void vtkPNGReaderUpdate(vtkImageData *data, OT *outPtr);
  template <class OT>
    void vtkPNGReaderUpdate2(
      OT *outPtr, int *outExt, vtkIdType *outInc, long pixSize);


private:
  vtkPNGReader(const vtkPNGReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPNGReader&) VTK_DELETE_FUNCTION;

  class vtkInternals;
  vtkInternals* Internals;
};
#endif
