// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkNamedColors
 * @brief   A class holding colors and their names.
 *
 * For a web page showcasing VTK Named Colors and their RGB values, see:
 * <a
 * href="https://htmlpreview.github.io/?https://github.com/Kitware/vtk-examples/blob/gh-pages/VTKNamedColorPatches.html">VTKNamedColorPatches</a>;
 * <a
 * href="https://kitware.github.io/vtk-examples/site/Python/Visualization/NamedColorPatches/">NamedColorPatches</a>
 * was used to generate this table.
 *
 * Color names are case insensitive and are stored as lower-case names
 * along with a 4-element array whose elements are red, green, blue and alpha,
 * in that order, corresponding to the RGBA value of the color.
 *
 * It is assumed that if the RGBA values are unsigned char then each element
 * lies in the range 0...255 and if the RGBA values are double then each
 * element lies in the range 0...1.
 *
 * The colors and names are those in <a href="https://en.wikipedia.org/wiki/Web_colors">Web
 * colors</a> that are derived from the CSS3 specification: <a
 * href="https://www.w3.org/TR/css-color-3/">CSS Color Module Level 3</a> In this table
 * common synonyms such as cyan/aqua and magenta/fuchsia are also included.
 *
 * Also included in this class are names and colors taken from
 * <em>Wrapping/Python/vtkmodules/util/colors.py</em> that were originally taken from
 * <em>Wrapping/Tcl/vtktesting/colors.tcl</em> (no longer in the VTK source files - deleted
 * 06-Dec-2017).
 *
 * Web colors and names in <a href="https://en.wikipedia.org/wiki/Web_colors">Web colors</a> take
 * precedence over those in <em>colors.py</em>. One consequence of this
 * is that while <em>colors.py</em> specifies green as equivalent to
 * (0,255,0), the web color standard defines it as (0,128,0).
 *
 * The \a SetColor methods will overwrite existing colors if the name of the
 * color being set matches an existing color. Note that ColorExists() can be
 * used to test for existence of the color being set.
 *
 * In the case of the \a GetColor methods returning doubles, alternative versions,
 * identified by the letters RGB in the names, are provided.
 * These get functions return just the red, green and blue components of
 * a color.
 *
 * The class also provides methods for defining a color through an HTML color
 * string. The following formats are supported:
 *
 * - \#RGB                 (3-digit hexadecimal number, where #4F2 is a shortcut for #44FF22)
 * - \#RRGGBB              (6-digit hexadecimal number)
 * - rgb(r, g, b)          (where r, g, b are in 0..255 or percentage values)
 * - rgba(r, g, b, a)      (where r, g, b, are in 0..255 or percentage values, a is in 0.0..1.0)
 * - a CSS3 color name     (e.g. "steelblue")
 */

#ifndef vtkNamedColors_h
#define vtkNamedColors_h

#include "vtkColor.h"             // Needed for vtkColor[34]ub
#include "vtkCommonColorModule.h" // For export macro
#include "vtkObject.h"
#include "vtkStdString.h"     // Needed for arguments
#include "vtkStringArray.h"   // For returning color names
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkNamedColorsDataStore;
class vtkColorStringParser;

class VTKCOMMONCOLOR_EXPORT VTK_MARSHALAUTO vtkNamedColors : public vtkObject
{
public:
  vtkTypeMacro(vtkNamedColors, vtkObject);

  /**
   * Methods invoked by print to print information about the object
   * including superclasses. Typically not called by the user
   * (use Print() instead) but used in the hierarchical print
   * process to combine the output of several classes.
   */
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Create a new vtkNamedColors object.
   */
  static vtkNamedColors* New();

  /**
   * Get the number of colors.
   */
  int GetNumberOfColors();

  /**
   * Reset the colors in the color map to the original colors.
   * Any colors inserted by the user will be lost.
   */
  void ResetColors();

  /**
   * Return true if the color exists.
   */
  bool ColorExists(const vtkStdString& name);

  /**
   * Get the color by name.
   * The name is treated as being case-insensitive.
   * The color is returned as a vtkColor4ub class.
   * The color black is returned if the color is not found.
   */
  vtkColor4ub GetColor4ub(const vtkStdString& name);

  /**
   * Get the color by name.
   * The name is treated as being case-insensitive.
   * The color is returned as four unsigned char variables:
   * red, green, blue, alpha. The range of each element is 0...255.
   * The color black is returned if the color is not found.
   */
  void GetColor(const vtkStdString& name, unsigned char& r, unsigned char& g, unsigned char& b,
    unsigned char& a);

  /**
   * Get the color by name.
   * The name is treated as being case-insensitive.
   * The color is returned as an unsigned char array:
   * [red, green, blue, alpha]. The range of each element is 0...255.
   * The color black is returned if the color is not found.
   */
  void GetColor(const vtkStdString& name, unsigned char rgba[4]);

  /**
   * Get the color by name.
   * The name is treated as being case-insensitive.
   * The color is returned as a vtkColor4ub class.
   * The color black is returned if the color is not found.
   */
  void GetColor(const vtkStdString& name, vtkColor4ub& rgba);

  /**
   * Get the color by name.
   * The name is treated as being case-insensitive.
   * The color is returned as a vtkColor4d class.
   * The color black is returned if the color is not found.
   */
  vtkColor4d GetColor4d(const vtkStdString& name);

  /**
   * Get the color by name.
   * The name is treated as being case-insensitive.
   * The color is returned as four double variables:
   * red, green, blue, alpha. The range of each element is 0...1.
   * The color black is returned if the color is not found.
   */
  void GetColor(const vtkStdString& name, double& r, double& g, double& b, double& a);

  /**
   * Get the color by name.
   * The name is treated as being case-insensitive.
   * The color is returned as a double array:
   * [red, green, blue, alpha]. The range of each element is 0...1.
   * The color black is returned if the color is not found.
   */
  void GetColor(const vtkStdString& name, double rgba[4]);

  /**
   * Get the color by name.
   * The name is treated as being case-insensitive.
   * The color is returned as a vtkColor4d class.
   * The color black is returned if the color is not found.
   */
  void GetColor(const vtkStdString& name, vtkColor4d& rgba);

  /**
   * Get the color by name.
   * The name is treated as being case-insensitive.
   * The color is returned as a vtkColor3ub class.
   * The color black is returned if the color is not found.
   */
  vtkColor3ub GetColor3ub(const vtkStdString& name);

  /**
   * Get the color by name.
   * The name is treated as being case-insensitive.
   * The color is returned as a vtkColor3d class.
   * The color black is returned if the color is not found.
   */
  vtkColor3d GetColor3d(const vtkStdString& name);

  /**
   * Get the color by name.
   * The name is treated as being case-insensitive.
   * The color is returned as three double variables:
   * red, green, blue. The range of each element is 0...1.
   * The color black is returned if the color is not found.
   */
  void GetColor(const vtkStdString& name, double& r, double& g, double& b);

  /**
   * Get the color by name.
   * The name is treated as being case-insensitive.
   * The color is returned as a double array:
   * [red, green, blue]. The range of each element is 0...1.
   * The color black is returned if the color is not found.
   */
  void GetColorRGB(const vtkStdString& name, double rgb[3]);

  /**
   * Get the color by name.
   * The name is treated as being case-insensitive.
   * The color is returned as a vtkColor3ub class.
   * The color black is returned if the color is not found.
   */
  void GetColor(const vtkStdString& name, vtkColor3ub& rgb);

  /**
   * Get the color by name.
   * The name is treated as being case-insensitive.
   * The color is returned as a vtkColor3d class.
   * The color black is returned if the color is not found.
   */
  void GetColor(const vtkStdString& name, vtkColor3d& rgb);

  /**
   * Set the color by name.
   * The name is treated as being case-insensitive.
   * The range of each color is 0...255.
   * No color is set if the name is empty.
   */
  virtual void SetColor(const vtkStdString& name, const unsigned char& r, const unsigned char& g,
    const unsigned char& b, const unsigned char& a = 255);

  /**
   * Set the color by name.
   * The name is treated as being case-insensitive.
   * The range of each color is 0...1.
   * No color is set if the name is empty.
   */
  virtual void SetColor(const vtkStdString& name, const double& r, const double& g, const double& b,
    const double& a = 1);

  /**
   * Set the color by name.
   * The name is treated as being case-insensitive.
   * The color is an unsigned char array:
   * [red, green, blue, alpha]. The range of each element is 0...255.
   * The user must ensure that the color array size is 4.
   * No color is set if the name is empty.
   */
  virtual void SetColor(const vtkStdString& name, const unsigned char rgba[4]);

  /**
   * Set the color by name.
   * The name is treated as being case-insensitive.
   * The color is a vtkColor4ub class.
   * No color is set if the name is empty.
   */
  virtual void SetColor(const vtkStdString& name, const vtkColor4ub& rgba);

  /**
   * Set the color by name.
   * The name is treated as being case-insensitive.
   * The color is a vtkColor3ub class.
   * No color is set if the name is empty.
   */
  virtual void SetColor(const vtkStdString& name, const vtkColor3ub& rgb);

  /**
   * Set the color by name.
   * The name is treated as being case-insensitive.
   * The color is a double array:
   * [red, green, blue, alpha]. The range of each element is 0...1.
   * No color is set if the name is empty.
   */
  virtual void SetColor(const vtkStdString& name, const double rgba[4]);

  /**
   * Set the color by name.
   * The name is treated as being case-insensitive.
   * The color is a vtkColor4d class.
   * No color is set if the name is empty.
   */
  virtual void SetColor(const vtkStdString& name, const vtkColor4d& rgba);

  /**
   * Set the color by name.
   * The name is treated as being case-insensitive.
   * The color is a vtkColor3d class.
   * No color is set if the name is empty.
   */
  virtual void SetColor(const vtkStdString& name, const vtkColor3d& rgb);

  /**
   * Remove the color by name.
   * The name is treated as being case-insensitive.
   * Examples for parsing are provided in:
   * TestNamedColors.cxx and TestNamedColorsIntegration.py
   */
  void RemoveColor(const vtkStdString& name);

  /**
   * Return a string of color names with each name
   * delimited by a line feed.
   * This is easily parsed by the user into whatever
   * data structure they require.
   * Examples for parsing are provided in:
   * TestNamedColors.cxx and TestNamedColorsIntegration.py
   */
  vtkStdString GetColorNames();

  /**
   * Return a string array of color names.
   */
  void GetColorNames(vtkStringArray* colorNames);

  /**
   * Return a string of synonyms such as
   * cyan/aqua and magenta/fuchsia.
   * The string is formatted such that a single line feed delimits
   * each color in the synonym and a double line feed delimits each
   * synonym.
   * Warning this could take a long time for very large color maps.
   * This is easily parsed by the user into whatever
   * data structure they require.
   */
  vtkStdString GetSynonyms();

  /**
   * Return a vtkColor4ub instance from an HTML color string in any of
   * the following formats:
   * - \#RGB
   * - \#RRGGBB
   * - rgb(r, g, b)
   * - rgba(r, g, b, a)
   * - a CSS3 color name, e.g. "steelblue"
   * If the string argument defines a color using one of the above formats
   * the method returns the successfully parsed color else returns a color
   * equal to `rgba(0, 0, 0, 0)`.
   */
  vtkColor4ub HTMLColorToRGBA(const vtkStdString& colorString);

  /**
   * Return a vtkColor3ub instance from an HTML color string in any of
   * the following formats:
   * - \#RGB
   * - \#RRGGBB
   * - rgb(r, g, b)
   * - rgba(r, g, b, a)
   * - a CSS3 color name, e.g. "steelblue"
   * If the string argument defines a color using one of the above formats
   * the method returns the successfully parsed color else returns the color
   * black.
   */
  vtkColor3ub HTMLColorToRGB(const vtkStdString& colorString);

  /**
   * Given a vtkColor3ub instance as input color return a valid HTML color
   * string in the `#RRGGBB` format.
   */
  vtkStdString RGBToHTMLColor(const vtkColor3ub& rgb);

  /**
   * Given a vtkColor4ub instance as input color return a valid HTML color
   * string in the `rgba(r, g, b, a)` format.
   */
  vtkStdString RGBAToHTMLColor(const vtkColor4ub& rgba);

  /**
   * Set the color by name.
   * The name is treated as being case-insensitive.
   * The color must be a valid HTML color string.
   * No color is set if the name is empty or if `htmlString` is not a valid
   * HTML color string.
   */
  void SetColor(const vtkStdString& name, const vtkStdString& htmlString);

protected:
  vtkNamedColors();
  ~vtkNamedColors() override;

private:
  ///@{
  /**
   * The implementation of the color map and other required methods.
   */
  vtkNamedColorsDataStore* Colors;
  vtkColorStringParser* Parser;
  ///@}

  vtkNamedColors(const vtkNamedColors&) = delete;
  void operator=(const vtkNamedColors&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkNamedColors_h */
