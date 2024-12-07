// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridWriter
 * @brief   Write a cell-grid file.
 *
 * Write a cell-grid object to a file. This is a simple JSON format for debugging purposes.
 */

#ifndef vtkCellGridWriter_h
#define vtkCellGridWriter_h

#include "vtkIOCellGridModule.h" // For export macro.
#include "vtkWriter.h"

// clang-format off
#include <vtk_nlohmannjson.h>        // For API.
#include VTK_NLOHMANN_JSON(json.hpp) // For API.
// clang-format on

VTK_ABI_NAMESPACE_BEGIN

class vtkCellGrid;

class VTKIOCELLGRID_EXPORT vtkCellGridWriter : public vtkWriter
{
public:
  vtkTypeMacro(vtkCellGridWriter, vtkWriter);
  static vtkCellGridWriter* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /// Set/get the filename to write to.
  ///
  /// This must be set before you call Write(). However, it need
  /// not be set before calling ToJSON().
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  ///@}

  ///@{
  /// Get the input cell-grid.
  vtkCellGrid* GetInput();
  vtkCellGrid* GetInput(int port);
  ///@}

  /// Populate the JSON \a data with a representation of the input \a grid.
  bool ToJSON(nlohmann::json& data, vtkCellGrid* grid);

  /// On-disk formats for the cell-grid data.
  ///
  /// This list may be extended in the future.
  enum Format : int
  {
    PlainText,      //!< Human-readable text.
    MessagePack,    //!< Binary message-pack data.
    NumberOfFormats //!< The number of enumerants (not a valid format).
  };

  ///@{
  /// Set/get the format to use when writing to disk.
  vtkSetEnumMacro(FileFormat, Format);
  vtkGetEnumMacro(FileFormat, Format);
  ///@}

protected:
  vtkCellGridWriter();
  ~vtkCellGridWriter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  void WriteData() override;

  char* FileName{ nullptr };
  Format FileFormat{ Format::PlainText };

private:
  vtkCellGridWriter(const vtkCellGridWriter&) = delete;
  void operator=(const vtkCellGridWriter&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif // vtkCellGridWriter_h
