// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSTLReader
 * @brief   read ASCII or binary stereo lithography files
 *
 * vtkSTLReader is a source object that reads ASCII or binary stereo
 * lithography files (.stl files). The FileName must be specified to
 * vtkSTLReader. The object automatically detects whether the file is
 * ASCII or binary. This reader supports reading streams.
 *
 * .stl files are quite inefficient since they duplicate vertex
 * definitions. By setting the Merging boolean you can control whether the
 * point data is merged after reading. Merging is performed by default,
 * however, merging requires a large amount of temporary storage since a
 * 3D hash table must be constructed.
 *
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * vtkSTLWriter uses little endian byte ordering and swaps bytes on other systems.
 */

#ifndef vtkSTLReader_h
#define vtkSTLReader_h

#include "vtkAbstractPolyDataReader.h"
#include "vtkIOGeometryModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkFloatArray;
class vtkIncrementalPointLocator;
class vtkPoints;
class vtkResourceParser;
class vtkResourceStream;

class VTKIOGEOMETRY_EXPORT vtkSTLReader : public vtkAbstractPolyDataReader
{
public:
  vtkTypeMacro(vtkSTLReader, vtkAbstractPolyDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with default options.
   */
  static vtkSTLReader* New();

  ///@{
  /**
   * Return true if, after a quick check of file header, it looks like the provided file or stream
   * can be read. Return false if it is sure it cannot be read, except if using RelaxedConformance.
   * The stream version may move the stream cursor.
   * Check that the first chars are "solid", if not, assume the file is binary
   * and skip the header of size 80, then read the number of triangles and
   * then check it correspond to the number of triangle defined in the file.
   */
  static bool CanReadFile(VTK_FILEPATH const char* filename);
  static bool CanReadFile(vtkResourceStream* stream);
  ///@}

  /**
   * Overload standard modified time function. If locator is modified,
   * then this object is modified as well.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set to true to support malformed files. Set to false to be strict and reject malformed files.
   * Default is true to match behaviour of VTK <= 9.5.
   */
  vtkSetMacro(RelaxedConformance, bool);
  vtkGetMacro(RelaxedConformance, bool);
  vtkBooleanMacro(RelaxedConformance, bool);
  ///@}

  ///@{
  /**
   * Turn on/off the merging of coincident points to restore neighborhood information.
   * Default is true.
   */
  vtkSetMacro(Merging, vtkTypeBool);
  vtkGetMacro(Merging, vtkTypeBool);
  vtkBooleanMacro(Merging, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off tagging of solids with scalars.
   * Default is false.
   */
  vtkSetMacro(ScalarTags, vtkTypeBool);
  vtkGetMacro(ScalarTags, vtkTypeBool);
  vtkBooleanMacro(ScalarTags, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify a spatial locator for merging points. By
   * default an instance of vtkMergePoints is used.
   */
  void SetLocator(vtkIncrementalPointLocator* locator);
  vtkGetObjectMacro(Locator, vtkIncrementalPointLocator);
  ///@}

  /**
   * Get header string.
   * If an ASCII STL file contains multiple solids then
   * headers are separated by newline character.
   * If a binary STL file is read, the first zero-terminated
   * string is stored in this header, the full header is available
   * by using GetBinaryHeader().
   * \sa GetBinaryHeader()
   */
  vtkGetStringMacro(Header);

  /**
   * Get binary file header string.
   * If ASCII STL file is read then BinaryHeader is not set,
   * and the header can be retrieved using GetHeader() instead.
   * \sa GetHeader()
   */
  vtkGetObjectMacro(BinaryHeader, vtkUnsignedCharArray);

protected:
  vtkSTLReader();
  ~vtkSTLReader() override;

  /**
   * Create default locator. Used to create one when none is specified.
   */
  vtkIncrementalPointLocator* NewDefaultLocator();

  /**
   * Set header string. Internal use only.
   */
  vtkSetStringMacro(Header);
  virtual void SetBinaryHeader(vtkUnsignedCharArray* binaryHeader);

  vtkTypeBool Merging = true;
  vtkTypeBool ScalarTags = false;
  vtkIncrementalPointLocator* Locator = nullptr;
  char* Header = nullptr;
  vtkUnsignedCharArray* BinaryHeader = nullptr;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  bool RelaxedConformance = true;

  vtkSTLReader(const vtkSTLReader&) = delete;
  void operator=(const vtkSTLReader&) = delete;

  bool ReadBinarySTL(vtkResourceStream* stream, vtkPoints*, vtkCellArray*);
  bool ReadASCIISTL(
    vtkResourceParser* parser, vtkPoints*, vtkCellArray*, vtkFloatArray* scalars = nullptr);

  static bool ReadBinaryHeader(vtkResourceStream* stream, vtkUnsignedCharArray* header);
  static bool ReadBinaryTrisField(vtkResourceStream* stream, uint32_t& numTrisField);
  static bool ReadBinaryTrisFile(vtkResourceStream* stream, vtkTypeInt64& numTrisFile);
};

VTK_ABI_NAMESPACE_END
#endif
