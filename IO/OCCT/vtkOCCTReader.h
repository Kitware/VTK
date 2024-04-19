// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2019-2021 Kitware SAS
// SPDX-FileCopyrightText: Copyright 2021-2022 Michael Migliore, Mathieu Westphal
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOCCTReader
 * @brief   VTK Reader for STEP and IGES files using OpenCASCADE
 *
 * This reader is based on OpenCASCADE and use XCAF toolkits (TKXDESTEP and TKXDEIGES)
 * if available to read the names and the colors. If not available, TKSTEP and TKIGES are
 * used but no names or colors are read.
 * The quality of the generated mesh is configured using RelativeDeflection, LinearDeflection,
 * and LinearDeflection.
 * Reading 1D cells (wires) is optional.
 *
 */

#ifndef vtkOCCTReader_h
#define vtkOCCTReader_h

#include <vtkMultiBlockDataSetAlgorithm.h>

#include "vtkDeprecation.h"  // For VTK_DEPRECATED_IN_9_3_0
#include "vtkIOOCCTModule.h" // For export macro

#include <memory> // For std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN

class vtkInformationDoubleVectorKey;

class VTKIOOCCT_EXPORT vtkOCCTReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkOCCTReader* New();
  vtkTypeMacro(vtkOCCTReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum Format : unsigned int
  {
    STEP,
    IGES
  };

  ///@{
  /**
   * Set the file format to read.
   * It can be either STEP or IGES.
   * Default is FILE_FORMAT::STEP
   */
  VTK_DEPRECATED_IN_9_3_0("Use SetFormat with unsigned int instead.")
  vtkSetEnumMacro(FileFormat, Format);
  vtkSetClampMacro(FileFormat, unsigned int, Format::STEP, Format::IGES);
  ///@}

  ///@{
  /**
   * Set/Get the linear deflection.
   * This value limits the distance between a curve and the resulting tessellation.
   * Default is 0.1
   */
  vtkGetMacro(LinearDeflection, double);
  vtkSetMacro(LinearDeflection, double);
  ///@}

  ///@{
  /**
   * Set/Get the angular deflection.
   * This value limits the angle between two subsequent segments.
   * Default is 0.5
   */
  vtkGetMacro(AngularDeflection, double);
  vtkSetMacro(AngularDeflection, double);
  ///@}

  ///@{
  /**
   * Set/Get relative deflection.
   * Determine if the deflection values are relative to object size.
   * Default is false
   */
  vtkGetMacro(RelativeDeflection, bool);
  vtkSetMacro(RelativeDeflection, bool);
  vtkBooleanMacro(RelativeDeflection, bool);
  ///@}

  ///@{
  /**
   * Enable/Disable 1D cells read. If enabled, surface boundaries are read.
   * Default is false
   */
  vtkGetMacro(ReadWire, bool);
  vtkSetMacro(ReadWire, bool);
  vtkBooleanMacro(ReadWire, bool);
  ///@}

  ///@{
  /**
   * Get/Set the file name.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

protected:
  vtkOCCTReader();
  ~vtkOCCTReader() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkOCCTReader(const vtkOCCTReader&) = delete;
  void operator=(const vtkOCCTReader&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;

  double LinearDeflection = 0.1;
  double AngularDeflection = 0.5;
  bool RelativeDeflection = false;
  bool ReadWire = false;
  unsigned int FileFormat = Format::STEP;
  char* FileName = nullptr;
};

VTK_ABI_NAMESPACE_END
#endif
