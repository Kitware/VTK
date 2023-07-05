// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkSmartPointer.h" // For vtkSmartPointer

VTK_ABI_NAMESPACE_BEGIN
class vtkStringArray;
VTK_ABI_NAMESPACE_END

VTK_ABI_NAMESPACE_BEGIN
class VTKIOIMAGE_EXPORT vtkPNGReader : public vtkImageReader2
{
public:
  static vtkPNGReader* New();
  vtkTypeMacro(vtkPNGReader, vtkImageReader2);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Is the given file a PNG file?
   */
  int CanReadFile(VTK_FILEPATH const char* fname) override;

  /**
   * Get the file extensions for this format.
   * Returns a string with a space separated list of extensions in
   * the format .extension
   */
  const char* GetFileExtensions() override { return ".png"; }

  /**
   * Return a descriptive name for the file format that might be useful in a GUI.
   */
  const char* GetDescriptiveName() override { return "PNG"; }

  /**
   * Given a 'key' for the text chunks, fills in 'beginEndIndex'
   * with the begin and end indexes. Values are stored between
   * [begin, end) indexes.
   */
  void GetTextChunks(const char* key, int beginEndIndex[2]);

  ///@{
  /**
   * Returns the text key stored at 'index'.
   */
  const char* GetTextKey(int index);
  vtkStringArray* GetTextKeys();
  ///@}

  ///@{
  /**
   * Returns the text value stored at 'index'. A range of indexes
   * that store values for a certain key can be obtained by calling
   * GetTextChunks.
   */
  const char* GetTextValue(int index);
  vtkStringArray* GetTextValues();
  ///@}

  /**
   * Return the number of text chunks in the PNG file.
   * Note that we don't process compressed or international text entries
   */
  size_t GetNumberOfTextChunks();

  ///@{
  /**
   * Set/Get if data spacing should be calculated from the PNG file.
   * Use default spacing if the PNG file don't have valid pixel per meter parameters.
   * Default is false.
   */
  vtkSetMacro(ReadSpacingFromFile, bool);
  vtkGetMacro(ReadSpacingFromFile, bool);
  vtkBooleanMacro(ReadSpacingFromFile, bool);
  ///@}
protected:
  vtkPNGReader();
  ~vtkPNGReader() override;

  void ExecuteInformation() override;
  void ExecuteDataWithInformation(vtkDataObject* out, vtkInformation* outInfo) override;
  template <class OT>
  void vtkPNGReaderUpdate(vtkImageData* data, OT* outPtr);
  template <class OT>
  void vtkPNGReaderUpdate2(OT* outPtr, int* outExt, vtkIdType* outInc, long pixSize);

private:
  vtkPNGReader(const vtkPNGReader&) = delete;
  void operator=(const vtkPNGReader&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
  bool ReadSpacingFromFile;
};
VTK_ABI_NAMESPACE_END
#endif
