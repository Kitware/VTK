/*=========================================================================

Program:   Visualization Toolkit

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*=========================================================================

F3D project
BSD 3-Clause License
See LICENSE

Copyright 2019-2021 Kitware SAS
Copyright 2021-2022 Michael Migliore, Mathieu Westphal
All rights reserved.

=========================================================================*/

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

#include "vtkIOOCCTModule.h" // For export macro

#include <memory> // For std::unique_ptr

class vtkInformationDoubleVectorKey;

class VTKIOOCCT_EXPORT vtkOCCTReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkOCCTReader* New();
  vtkTypeMacro(vtkOCCTReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum class Format : unsigned int
  {
    STEP,
    IGES
  };

  //@{
  /**
   * Set the file format to read.
   * It can be either STEP or IGES.
   * Default is FILE_FORMAT::STEP
   */
  vtkSetEnumMacro(FileFormat, Format);
  //@}

  //@{
  /**
   * Set/Get the linear deflection.
   * This value limits the distance between a curve and the resulting tesselation.
   * Default is 0.1
   */
  vtkGetMacro(LinearDeflection, double);
  vtkSetMacro(LinearDeflection, double);
  //@}

  //@{
  /**
   * Set/Get the angular deflection.
   * This value limits the angle between two subsequent segments.
   * Default is 0.5
   */
  vtkGetMacro(AngularDeflection, double);
  vtkSetMacro(AngularDeflection, double);
  //@}

  //@{
  /**
   * Set/Get relative deflection.
   * Determine if the deflection values are relative to object size.
   * Default is false
   */
  vtkGetMacro(RelativeDeflection, bool);
  vtkSetMacro(RelativeDeflection, bool);
  vtkBooleanMacro(RelativeDeflection, bool);
  //@}

  //@{
  /**
   * Enable/Disable 1D cells read. If enabled, surface boundaries are read.
   * Default is false
   */
  vtkGetMacro(ReadWire, bool);
  vtkSetMacro(ReadWire, bool);
  vtkBooleanMacro(ReadWire, bool);
  //@}

  //@{
  /**
   * Get/Set the file name.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  //@}

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
  Format FileFormat = Format::STEP;
  char* FileName = nullptr;
};

#endif
