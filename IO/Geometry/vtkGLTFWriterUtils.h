// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @brief   Helper functions for writing glTF files
 *
 * vtkGLTFWriterUtils is a helper namespace that contains various functions to help
 * with the parsing and validation of JSON-formatted glTF files.
 * More specifically, these functions add existence and type verifications
 * before extracting Json values.
 * Another function helps check the document's version against supported glTF versions
 */

#ifndef vtkGLTFWriterUtils_h
#define vtkGLTFWriterUtils_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkWrappingHints.h"

#include <vtk_nlohmannjson.h>
#include VTK_NLOHMANN_JSON(json.hpp)
#include <ostream>

VTK_ABI_NAMESPACE_BEGIN
class vtkBase64OutputStream;
class vtkCellArray;
class vtkDataArray;

class VTKIOGEOMETRY_EXPORT vtkGLTFWriterUtils
{
public:
  VTK_WRAPEXCLUDE static void WriteValues(vtkDataArray* ca, std::ostream& myFile);
  VTK_WRAPEXCLUDE static void WriteValues(vtkDataArray* ca, vtkBase64OutputStream* ostr);
  VTK_WRAPEXCLUDE static void WriteBufferAndView(vtkDataArray* inda, const char* fileName,
    bool inlineData, nlohmann::json& buffers, nlohmann::json& bufferViews, int bufferViewTarget);
  VTK_WRAPEXCLUDE static void WriteCellBufferAndView(vtkCellArray* ca, const char* fileName,
    bool inlineData, nlohmann::json& buffers, nlohmann::json& bufferViews);
};

// gltf uses hard coded numbers to represent data types
// they match the definitions from gl.h but for your convenience
// some of the common values we use are listed below to make
// the code more readable without including gl.h

#define GL_BYTE 0x1400
#define GL_UNSIGNED_BYTE 0x1401
#define GL_SHORT 0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406

#define GL_CLAMP_TO_EDGE 0x812F
#define GL_REPEAT 0x2901

#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601

#define ARRAY_BUFFER 34962
#define ELEMENT_ARRAY_BUFFER 34963

const int GLTF_ARRAY_BUFFER = 34962;
const int GLTF_ELEMENT_ARRAY_BUFFER = 34963;

VTK_ABI_NAMESPACE_END
#endif

// VTK-HeaderTest-Exclude: vtkGLTFWriterUtils.h
