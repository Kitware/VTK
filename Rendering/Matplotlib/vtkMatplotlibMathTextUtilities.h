/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatplotlibMathTextUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMatplotlibMathTextUtilities
 * @brief   Access to MatPlotLib MathText rendering
 *
 * vtkMatplotlibMathTextUtilities provides access to the MatPlotLib MathText
 * implementation.
 *
 * This class is aware of a number of environment variables that can be used to
 * configure and debug python initialization (all are optional):
 * - VTK_MATPLOTLIB_DEBUG: Enable verbose debugging output during initialization
 * of the python environment.
 *
 * This class handles rendering multiline and multicolumn strings into image data.
 * Use '\n' to define a line, and '|' to define a column.
 *
 * This class does not support rendering multiline and multicolumn strings into
 * a vtkPath.
 *
 * Example :
 *
 * str =    "$\\sum_{i=0}^\\infty x_i$ | 2 | 3 | 4 \n"
 *        +  1 | 2 | 3";
 *
 * The vertical space between two lines can be set with vtkTextProperty::SetLineSpacing and
 * vtkTextProperty::SetLineOffset
 *
 * The horizontal space between two cells can be set with vtkTextProperty::SetCellOffset
 *
 * Currently, it is not possible to draw separators between grid cells. Only exterior
 * edges can be drawn with vtkTextProperty::SetFrame
 */

#ifndef vtkMatplotlibMathTextUtilities_h
#define vtkMatplotlibMathTextUtilities_h

#include "vtkMathTextUtilities.h"
#include "vtkRenderingMatplotlibModule.h" // For export macro

#include <vector> // for std::vector

struct _object;
typedef struct _object PyObject;
class vtkSmartPyObject;
class vtkImageData;
class vtkPath;
class vtkPythonInterpreter;
class vtkTextProperty;
struct TextColors;

class VTKRENDERINGMATPLOTLIB_EXPORT vtkMatplotlibMathTextUtilities : public vtkMathTextUtilities
{
public:
  vtkTypeMacro(vtkMatplotlibMathTextUtilities, vtkMathTextUtilities);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkMatplotlibMathTextUtilities* New();

  bool IsAvailable() override;

  /**
   * Given a text property and a string, get the bounding box {xmin, xmax,
   * ymin, ymax} of the rendered string in pixels. The origin of the bounding
   * box is the anchor point described by the horizontal and vertical
   * justification text property variables.
   * Returns true on success, false otherwise.
   */
  bool GetBoundingBox(vtkTextProperty* tprop, const char* str, int dpi, int bbox[4]) override;

  bool GetMetrics(
    vtkTextProperty* tprop, const char* str, int dpi, vtkTextRenderer::Metrics& metrics) override;

  /**
   * Render the given string @a str into the vtkImageData @a image with a
   * resolution of @a dpi. The image is resized automatically. textDims
   * will be overwritten by the pixel width and height of the rendered string.
   * This is useful when ScaleToPowerOfTwo is true, and the image dimensions may
   * not match the dimensions of the rendered text.
   * The origin of the image's extents is aligned with the anchor point
   * described by the text property's vertical and horizontal justification
   * options.
   * This function supports multiline and multicolumn strings.
   */
  bool RenderString(const char* str, vtkImageData* image, vtkTextProperty* tprop, int dpi,
    int textDims[2] = nullptr) override;

  /**
   * Parse the MathText expression in str and fill path with a contour of the
   * glyphs. The origin of the path coordinates is aligned with the anchor point
   * described by the text property's horizontal and vertical justification
   * options.
   * This function does not support multiline and multicolumn strings.
   */
  bool StringToPath(const char* str, vtkPath* path, vtkTextProperty* tprop, int dpi) override;

  ///@{
  /**
   * Set to true if the graphics implementation requires texture image dimensions
   * to be a power of two. Default is true, but this member will be set
   * appropriately when GL is inited.
   */
  void SetScaleToPowerOfTwo(bool val) override;
  bool GetScaleToPowerOfTwo() override;
  ///@}

protected:
  vtkMatplotlibMathTextUtilities();
  ~vtkMatplotlibMathTextUtilities() override;

  bool InitializeMaskParser();
  bool InitializePathParser();
  bool InitializeFontPropertiesClass();

  bool CheckForError();
  bool CheckForError(PyObject* object);

  /**
   * Replace each occurence of strToFind in str by replacementStr.
   * Used to protect escaped pipe before the splitting process, and recover them
   * after.
   */
  void FindAndReplaceInString(
    std::string& str, const std::string& strToFind, const std::string& replacementStr);

  static constexpr const char* PipeProtectString = "VTK_PROTECT_PIPE";

  /**
   * Compute rgba values of the foreground, background and frame
   * of the text property.
   */
  void ComputeTextColors(vtkTextProperty* tprop, TextColors& tcolors);

  /**
   * Modify matplotlib.rcParams to customize math text font.
   */
  bool SetMathTextFont(vtkTextProperty* tprop);

  /**
   * Returns a matplotlib.font_manager.FontProperties PyObject, initialized from
   * the vtkTextProperty tprop.
   */
  PyObject* GetFontProperties(vtkTextProperty* tprop);

  /**
   * Cleanup and destroy any python objects. This is called during destructor as
   * well as when the Python interpreter is finalized. Thus this class must
   * handle the case where the internal python objects disappear between calls.
   */
  void CleanupPythonObjects();

  vtkPythonInterpreter* Interpreter;
  PyObject* MaskParser;
  PyObject* PathParser;
  PyObject* FontPropertiesClass;

  static void GetJustifiedBBox(int rows, int cols, vtkTextProperty* tprop, int bbox[4]);

  // Rotate the 4 2D corner points by the specified angle (degrees) around the
  // origin and calculate the bounding box
  static void RotateCorners(double angleDeg, double corners[4][2], double bbox[4]);

  bool ScaleToPowerOfTwo;
  bool PrepareImageData(vtkImageData* data, int bbox[4]);

private:
  vtkMatplotlibMathTextUtilities(const vtkMatplotlibMathTextUtilities&) = delete;
  void operator=(const vtkMatplotlibMathTextUtilities&) = delete;

  /**
   * Used for runtime checking of matplotlib's mathtext availability.
   * @sa IsAvailable
   */
  enum Availability
  {
    NOT_TESTED = 0,
    AVAILABLE,
    UNAVAILABLE
  };

  /**
   * Function used to check MPL availability and update MPLMathTextAvailable.
   * This will do tests only the first time this method is called. This method
   * is called internally when matplotlib rendering is first needed and is used
   * to implement IsAvailable.
   */
  static Availability CheckMPLAvailability();

  ///@{
  /**
   * Cache the availability of matplotlib in the current python session.
   */
  static Availability MPLMathTextAvailable;
  ///@}

  typedef std::vector<std::vector<std::string>> GridOfStrings;

  /**
   * Parse the string to handle multiline and multicolumn.
   * Divide it in lines (splitted with '\n') and cells (splitted with '|')
   * and store each cell string in strGrid. Also compute the maximum number of cells of all
   * lines to ensure that all lines have the same number of cells.
   */
  bool ParseString(const char* str, GridOfStrings& strGrid, std::size_t& maxNumberOfCells);

  /**
   * Given a grid of string and its corresponding maximum number of cells,
   * text property and dpi, compute the resulting number of rows and cols
   * of the image.
   * Precondition : Matplotlib rendering is available and mask parser is initialized.
   */
  bool ComputeRowsAndCols(const GridOfStrings& strGrid, const std::size_t& maxNumberOfCells,
    vtkTextProperty* tprop, PyObject* pyFontProp, int dpi, std::uint64_t& rows,
    std::uint64_t& cols);

  /**
   * Given a cell string, text property and dpi, call python mathtext to render the cell and store
   * it in list if list is not nullptr, and store in rows and cols the size of the python data.
   * Precondition : Matplotlib rendering is available and mask parser is initialized.
   */
  bool ComputeCellRowsAndCols(const char* cellStr, PyObject* pyFontProp, int dpi,
    std::uint64_t& rows, std::uint64_t& cols, vtkSmartPyObject* list);

  /**
   * Render in the image starting from (rowStart, colStart) to (rowStart + cellRows, colStart +
   * cellCols) a cell of size (pythonRows, pythonCols) with pixels value stored in pythonData. If
   * the python cell size is inferior to the cell size, fill with background color.
   */
  bool RenderOneCell(vtkImageData* image, int bbox[4], const std::int64_t rowStart,
    const std::int64_t colStart, vtkSmartPyObject& pythonData, const std::uint64_t pythonRows,
    const std::uint64_t pythonCols, const std::uint64_t cellRows, const std::uint64_t cellCols,
    vtkTextProperty* tprop, const TextColors& tcolors);
};

#endif
