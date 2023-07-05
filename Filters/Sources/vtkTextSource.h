// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTextSource
 * @brief   create polygonal text
 *
 * vtkTextSource converts a text string into polygons.  This way you can
 * insert text into your renderings. It uses the 9x15 font from X Windows.
 * You can specify if you want the background to be drawn or not. The
 * characters are formed by scan converting the raster font into
 * quadrilaterals. Colors are assigned to the letters using scalar data.
 * To set the color of the characters with the source's actor property, set
 * BackingOff on the text source and ScalarVisibilityOff on the associated
 * vtkPolyDataMapper. Then, the color can be set using the associated actor's
 * property.
 *
 * vtkVectorText generates higher quality polygonal representations of
 * characters.
 *
 * @sa
 * vtkVectorText
 */

#ifndef vtkTextSource_h
#define vtkTextSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSSOURCES_EXPORT vtkTextSource : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkTextSource, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with no string set and backing enabled.
   */
  static vtkTextSource* New();

  ///@{
  /**
   * Set/Get the text to be drawn.
   */
  vtkSetStringMacro(Text);
  vtkGetStringMacro(Text);
  ///@}

  ///@{
  /**
   * Controls whether or not a background is drawn with the text.
   */
  vtkSetMacro(Backing, vtkTypeBool);
  vtkGetMacro(Backing, vtkTypeBool);
  vtkBooleanMacro(Backing, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the foreground color. Default is white (1,1,1). ALpha is always 1.
   */
  vtkSetVector3Macro(ForegroundColor, double);
  vtkGetVectorMacro(ForegroundColor, double, 3);
  ///@}

  ///@{
  /**
   * Set/Get the background color. Default is black (0,0,0). Alpha is always 1.
   */
  vtkSetVector3Macro(BackgroundColor, double);
  vtkGetVectorMacro(BackgroundColor, double, 3);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output points.
   * vtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * vtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

protected:
  vtkTextSource();
  ~vtkTextSource() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  char* Text;
  vtkTypeBool Backing;
  double ForegroundColor[4];
  double BackgroundColor[4];
  int OutputPointsPrecision;

private:
  vtkTextSource(const vtkTextSource&) = delete;
  void operator=(const vtkTextSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
