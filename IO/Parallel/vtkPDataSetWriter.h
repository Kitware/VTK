// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPDataSetWriter
 * @brief   Manages writing pieces of a data set.
 *
 * vtkPDataSetWriter will write a piece of a file, and will also create
 * a metadata file that lists all of the files in a data set.
 */

#ifndef vtkPDataSetWriter_h
#define vtkPDataSetWriter_h

#include "vtkDataSetWriter.h"
#include "vtkIOParallelModule.h" // For export macro

#include <map>    // for keeping track of extents
#include <vector> // for keeping track of extents

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;
class vtkRectilinearGrid;
class vtkStructuredGrid;
class vtkMultiProcessController;

class VTKIOPARALLEL_EXPORT vtkPDataSetWriter : public vtkDataSetWriter
{
public:
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkPDataSetWriter, vtkDataSetWriter);
  static vtkPDataSetWriter* New();

  /**
   * Write the pvtk file and corresponding vtk files.
   */
  int Write() override;

  ///@{
  /**
   * This is how many pieces the whole data set will be divided into.
   */
  void SetNumberOfPieces(int num);
  vtkGetMacro(NumberOfPieces, int);
  ///@}

  ///@{
  /**
   * Extra ghost cells will be written out to each piece file
   * if this value is larger than 0.
   */
  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);
  ///@}

  ///@{
  /**
   * This is the range of pieces that that this writer is
   * responsible for writing.  All pieces must be written
   * by some process.  The process that writes piece 0 also
   * writes the pvtk file that lists all the piece file names.
   */
  vtkSetMacro(StartPiece, int);
  vtkGetMacro(StartPiece, int);
  vtkSetMacro(EndPiece, int);
  vtkGetMacro(EndPiece, int);
  ///@}

  ///@{
  /**
   * This file pattern uses the file name and piece number
   * to construct a file name for the piece file.
   */
  vtkSetFilePathMacro(FilePattern);
  vtkGetFilePathMacro(FilePattern);
  ///@}

  ///@{
  /**
   * This flag determines whether to use absolute paths for the piece files.
   * By default the pieces are put in the main directory, and the piece file
   * names in the meta data pvtk file are relative to this directory.
   * This should make moving the whole lot to another directory, an easier task.
   */
  vtkSetMacro(UseRelativeFileNames, vtkTypeBool);
  vtkGetMacro(UseRelativeFileNames, vtkTypeBool);
  vtkBooleanMacro(UseRelativeFileNames, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Controller used to communicate data type of blocks.
   * By default, the global controller is used. If you want another
   * controller to be used, set it with this.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

protected:
  vtkPDataSetWriter();
  ~vtkPDataSetWriter() override;

  ostream* OpenFile();
  int WriteUnstructuredMetaData(
    vtkDataSet* input, char* root, char* str, size_t strSize, ostream* fptr);
  int WriteImageMetaData(vtkImageData* input, char* root, char* str, size_t strSize, ostream* fptr);
  int WriteRectilinearGridMetaData(
    vtkRectilinearGrid* input, char* root, char* str, size_t strSize, ostream* fptr);
  int WriteStructuredGridMetaData(
    vtkStructuredGrid* input, char* root, char* str, size_t strSize, ostream* fptr);

  int StartPiece;
  int EndPiece;
  int NumberOfPieces;
  int GhostLevel;

  vtkTypeBool UseRelativeFileNames;

  char* FilePattern;

  void DeleteFiles();

  typedef std::map<int, std::vector<int>> ExtentsType;
  ExtentsType Extents;

  vtkMultiProcessController* Controller;

private:
  vtkPDataSetWriter(const vtkPDataSetWriter&) = delete;
  void operator=(const vtkPDataSetWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
