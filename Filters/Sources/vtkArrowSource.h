// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkArrowSource
 * @brief   Appends a cylinder to a cone to form an arrow.
 *
 * vtkArrowSource was intended to be used as the source for a glyph.
 * The shaft base is always at (0,0,0). The arrow tip is always at (1,0,0). If
 * "Invert" is true, then the ends are flipped i.e. tip is at (0,0,0) while
 * base is at (1, 0, 0).
 * The resolution of the cone and shaft can be set and default to 6.
 * The radius of the cone and shaft can be set and default to 0.03 and 0.1.
 * The length of the tip can also be set, and defaults to 0.35.
 */

#ifndef vtkArrowSource_h
#define vtkArrowSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSSOURCES_EXPORT vtkArrowSource : public vtkPolyDataAlgorithm
{
public:
  /**
   * Construct cone with angle of 45 degrees.
   */
  static vtkArrowSource* New();

  vtkTypeMacro(vtkArrowSource, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the length, and radius of the tip.  They default to 0.35 and 0.1
   */
  vtkSetClampMacro(TipLength, double, 0.0, 1.0);
  vtkGetMacro(TipLength, double);
  vtkSetClampMacro(TipRadius, double, 0.0, 10.0);
  vtkGetMacro(TipRadius, double);
  ///@}

  ///@{
  /**
   * Set the resolution of the tip.  The tip behaves the same as a cone.
   * Resolution 1 gives a single triangle, 2 gives two crossed triangles.
   */
  vtkSetClampMacro(TipResolution, int, 1, 128);
  vtkGetMacro(TipResolution, int);
  ///@}

  ///@{
  /**
   * Set the radius of the shaft.  Defaults to 0.03.
   */
  vtkSetClampMacro(ShaftRadius, double, 0.0, 5.0);
  vtkGetMacro(ShaftRadius, double);
  ///@}

  ///@{
  /**
   * Set the resolution of the shaft. Minimum is 3 for a triangular shaft.
   */
  vtkSetClampMacro(ShaftResolution, int, 3, 128);
  vtkGetMacro(ShaftResolution, int);
  ///@}

  ///@{
  /**
   * Inverts the arrow direction. When set to true, base is at (1, 0, 0) while the
   * tip is at (0, 0, 0). The default is false, i.e. base at (0, 0, 0) and the tip
   * at (1, 0, 0).
   */
  vtkBooleanMacro(Invert, bool);
  vtkSetMacro(Invert, bool);
  vtkGetMacro(Invert, bool);
  ///@}

  enum class ArrowOrigins
  {
    Default = 0,
    Center = 1
  };

  ///@{
  /**
   * Sets and Gets the location used for orienting and scaling the arrow.
   * Default is set to Default.
   */
  vtkSetEnumMacro(ArrowOrigin, ArrowOrigins);
  vtkGetEnumMacro(ArrowOrigin, ArrowOrigins);
  ///@}

  void SetArrowOriginToDefault() { this->SetArrowOrigin(ArrowOrigins::Default); }
  void SetArrowOriginToCenter() { this->SetArrowOrigin(ArrowOrigins::Center); }
  std::string GetArrowOriginAsString() const;

protected:
  vtkArrowSource();
  ~vtkArrowSource() override = default;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int TipResolution;
  double TipLength;
  double TipRadius;

  int ShaftResolution;
  double ShaftRadius;
  bool Invert;
  ArrowOrigins ArrowOrigin;

private:
  vtkArrowSource(const vtkArrowSource&) = delete;
  void operator=(const vtkArrowSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
