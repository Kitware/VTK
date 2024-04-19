// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkProStarReader
 * @brief   Reads geometry in proSTAR (STARCD) file format.
 *
 * vtkProStarReader creates an unstructured grid dataset.
 * It reads .cel/.vrt files stored in proSTAR (STARCD) ASCII format.
 *
 * @par Thanks:
 * Reader written by Mark Olesen
 *
 */

#ifndef vtkProStarReader_h
#define vtkProStarReader_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIOGEOMETRY_EXPORT vtkProStarReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkProStarReader* New();
  vtkTypeMacro(vtkProStarReader, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the file name prefix of the cel/vrt files to read.
   * The reader will try to open FileName.cel and FileName.vrt files.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * The proSTAR files are often in millimeters.
   * Specify an alternative scaling factor.
   */
  vtkSetClampMacro(ScaleFactor, double, 0, VTK_DOUBLE_MAX);
  vtkGetMacro(ScaleFactor, double);
  ///@}

  /**
   * The type of material represented by the cell
   */
  enum cellType
  {
    starcdFluidType = 1,
    starcdSolidType = 2,
    starcdBaffleType = 3,
    starcdShellType = 4,
    starcdLineType = 5,
    starcdPointType = 6
  };

  /**
   * The primitive cell shape
   */
  enum shapeType
  {
    starcdPoint = 1,
    starcdLine = 2,
    starcdShell = 3,
    starcdHex = 11,
    starcdPrism = 12,
    starcdTet = 13,
    starcdPyr = 14,
    starcdPoly = 255
  };

protected:
  vtkProStarReader();
  ~vtkProStarReader() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * The name of the file to be read.  If it has a .cel, .vrt, or .inp
   * extension it will be truncated and later appended when reading
   * the appropriate files.  Otherwise those extensions will be appended
   * to FileName when opening the files.
   */
  char* FileName;

  /**
   * The coordinates are multiplied by ScaleFactor when setting them.
   * The default value is 1.
   */
  double ScaleFactor;

private:
  //
  // Internal Classes/Structures
  //
  struct idMapping;

  FILE* OpenFile(const char* ext);

  bool ReadVrtFile(vtkUnstructuredGrid* output, idMapping& pointMapping);
  bool ReadCelFile(vtkUnstructuredGrid* output, const idMapping& pointMapping);

  vtkProStarReader(const vtkProStarReader&) = delete;
  void operator=(const vtkProStarReader&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
