// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPLYReader
 * @brief   read Stanford University PLY polygonal file format
 *
 * vtkPLYReader is a source object that reads polygonal data in
 * Stanford University PLY file format (see
 * http://graphics.stanford.edu/data/3Dscanrep). It requires that
 * the elements "vertex" and "face" are defined. The "vertex" element
 * must have the properties "x", "y", and "z". The "face" element must
 * have the property "vertex_indices" defined. Optionally, if the "face"
 * element has the properties "intensity" and/or the triplet "red",
 * "green", "blue", and optionally "alpha"; these are read and added as scalars
 * to the output data.
 * If the "face" element has the property "texcoord" a new TCoords
 * point array is created and points are duplicated if they have 2 or
 * more different texture coordinates. Points are duplicated only if
 * DuplicatePointsForFaceTexture is true (default).
 * This creates a polygonal data that can be textured without
 * artifacts. If unique points are required use a vtkCleanPolyData
 * filter after this reader or use this reader with DuplicatePointsForFaceTexture
 * set to false.
 *
 * This reader supports streaming.
 * Set `ReadFromInputStream` to true and set the `Stream` object to read from a stream.
 *
 * @sa
 * vtkPLYWriter, vtkCleanPolyData
 */

#ifndef vtkPLYReader_h
#define vtkPLYReader_h

#include "vtkAbstractPolyDataReader.h"
#include "vtkIOPLYModule.h"    // For export macro
#include "vtkResourceStream.h" // For vtkResourceStream

VTK_ABI_NAMESPACE_BEGIN
class vtkStringArray;

class VTKIOPLY_EXPORT vtkPLYReader : public vtkAbstractPolyDataReader
{
public:
  vtkTypeMacro(vtkPLYReader, vtkAbstractPolyDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with merging set to true.
   */
  static vtkPLYReader* New();

  /**
   * A simple, non-exhaustive check to see if a file is a valid ply file.
   */
  static int CanReadFile(VTK_FILEPATH const char* filename);

  vtkGetObjectMacro(Comments, vtkStringArray);

  /**
   * Tolerance used to detect different texture coordinates for shared
   * points for faces.
   */
  vtkGetMacro(FaceTextureTolerance, float);
  vtkSetMacro(FaceTextureTolerance, float);

  ///@{
  /**
   * Enable reading from an InputString instead of the default, a file.
   * Note that reading from an input stream would be more flexible (enabling
   * other kind of streams) and possibly more efficient because we don't need
   * to save the whole stream to a string. However a stream interface
   * does not translate well to python and the string interface satisfies
   * our current needs. So we leave the stream interface for future work.
   */
  vtkSetMacro(ReadFromInputString, bool);
  vtkGetMacro(ReadFromInputString, bool);
  vtkBooleanMacro(ReadFromInputString, bool);
  void SetInputString(const std::string& s) { this->InputString = s; }
  ///@}

  ///@{
  /**
   * Enable reading from an InputStream
   * `ReadFromInputStream` has an higher priority than `ReadFromInputString`.
   */
  vtkSetMacro(ReadFromInputStream, bool);
  vtkGetMacro(ReadFromInputStream, bool);
  vtkBooleanMacro(ReadFromInputStream, bool);
  ///@}

  /**
   * If true (default) and the "face" element has the property "texcoord" duplicate
   * face points if they have 2 or more different texture coordinates.
   * Otherwise, each texture coordinate for a face point overwrites previously set
   * texture coordinates for that point.
   */
  vtkGetMacro(DuplicatePointsForFaceTexture, bool);
  vtkSetMacro(DuplicatePointsForFaceTexture, bool);

protected:
  vtkPLYReader();
  ~vtkPLYReader() override;

  vtkStringArray* Comments;
  // Whether this object is reading from a string or a file.
  // Default is 0: read from file.
  bool ReadFromInputString;
  // The input string.
  std::string InputString;

  bool ReadFromInputStream = false;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPLYReader(const vtkPLYReader&) = delete;
  void operator=(const vtkPLYReader&) = delete;

  float FaceTextureTolerance;
  bool DuplicatePointsForFaceTexture;
};

VTK_ABI_NAMESPACE_END
#endif
