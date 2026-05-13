// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLPDataObjectWriter
 * @brief   Write data in a parallel XML format.
 *
 * vtkXMLPDataObjectWriter is the superclass for all XML parallel data object
 * writers. It provides functionality needed for writing parallel formats,
 * such as the selection of which writer writes the summary file and what
 * range of pieces are assigned to each serial writer.
 *
 * Typical parallel usage (one writer per MPI rank, each rank writes a
 * single piece, rank 0 also writes the summary `.pvt*` file):
 *
 * @code
 *   // Once per program, on every rank, after MPI_Init:
 *   vtkNew<vtkMPIController> controller;
 *   controller->Initialize();
 *   vtkMultiProcessController::SetGlobalController(controller);
 *
 *   // Per write, on every rank:
 *   vtkNew<vtkXMLPUnstructuredGridWriter> writer;
 *   writer->SetFileName("output.pvtu");
 *   writer->SetInputData(grid);
 *   writer->SetNumberOfPieces(controller->GetNumberOfProcesses());
 *   writer->SetStartPiece(controller->GetLocalProcessId());
 *   writer->SetEndPiece(controller->GetLocalProcessId());
 *   writer->Write();
 * @endcode
 *
 * The three piece settings (NumberOfPieces, StartPiece, EndPiece) are not
 * derived from the controller automatically; callers must set them on every
 * rank. The summary file is written by rank 0 only.
 *
 * If no global vtkMultiProcessController is installed and SetController()
 * has not been called, the writer falls back to single-process behavior,
 * which is rarely what callers want when running under MPI. Either install
 * a global controller (recommended) or pass one explicitly via
 * SetController().
 *
 * @sa
 * vtkXMLDataObjectWriter vtkMPIController
 */

#ifndef vtkXMLPDataObjectWriter_h
#define vtkXMLPDataObjectWriter_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCallbackCommand;
class vtkMultiProcessController;

class VTKIOPARALLELXML_EXPORT vtkXMLPDataObjectWriter : public vtkXMLWriter
{
public:
  vtkTypeMacro(vtkXMLPDataObjectWriter, vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the total number of pieces across all ranks. In a typical MPI
   * setup this is the controller's number of processes; it must be set to
   * the same value on every rank.
   */
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);
  ///@}

  ///@{
  /**
   * Get/Set the inclusive range of pieces this writer is responsible for.
   * Under MPI, each rank typically owns one piece, so both StartPiece and
   * EndPiece are set to the local process id. Indices range from 0 to
   * NumberOfPieces - 1.
   */
  vtkSetMacro(StartPiece, int);
  vtkGetMacro(StartPiece, int);
  vtkSetMacro(EndPiece, int);
  vtkGetMacro(EndPiece, int);
  ///@}

  ///@{
  /**
   * Get/Set the ghost level used for this writer's piece.
   */
  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);
  ///@}

  ///@{
  /**
   * Get/Set whether to use a subdirectory to store the pieces
   */
  vtkSetMacro(UseSubdirectory, bool);
  vtkGetMacro(UseSubdirectory, bool);
  ///@}

  ///@{
  /**
   * Get/Set whether the writer should write the summary file that
   * refers to all of the pieces' individual files.
   * This is on by default. Note that only the first process writes
   * the summary file.
   */
  virtual void SetWriteSummaryFile(int flag);
  vtkGetMacro(WriteSummaryFile, int);
  vtkBooleanMacro(WriteSummaryFile, int);
  ///@}

  ///@{
  /**
   * Set/Get the controller used to coordinate piece assignment, exchange
   * metadata across ranks, and decide which rank writes the summary file.
   *
   * If not set explicitly, the writer uses
   * vtkMultiProcessController::GetGlobalController(). If that is also
   * unset, the writer falls back to single-process behavior, so under MPI
   * a global controller must be installed (or one must be passed here)
   * for parallel writing to work as expected.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  /**
   * Overridden to handle passing the CONTINUE_EXECUTING() flags to the
   * executive.
   */
  vtkTypeBool ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

protected:
  vtkXMLPDataObjectWriter();
  ~vtkXMLPDataObjectWriter() override;

  /**
   * Override writing method from superclass.
   */
  int WriteInternal() override;

  /**
   * Write data from the input dataset. Call WritePData(vtkIndent indent)
   */
  int WriteData() override;

  /**
   * Write Data associated with the input dataset. It needs to be overridden by subclass
   */
  virtual void WritePData(vtkIndent indent) = 0;

  /**
   * Write a piece of the dataset on disk. Called by WritePieceInternal().
   * It needs to be overridden by subclass
   */
  virtual int WritePiece(int index) = 0;

  /**
   * Method called by WriteInternal(). It's used for writing a piece of the dataset.
   * It needs to be overridden by subclass.
   */
  virtual int WritePieceInternal() = 0;

  /**
   * Overridden to make appropriate piece request from upstream.
   */
  virtual int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  /**
   * Collect information between ranks before writing the summary file.
   * This method is called on all ranks while summary file is only written on 1
   * rank (rank 0).
   */
  virtual void PrepareSummaryFile();

  /**
   * Write the attributes of the piece at the given index
   */
  virtual void WritePPieceAttributes(int index);

  ///@{
  /**
   * Methods for creating a filename for each piece in the dataset
   */
  char* CreatePieceFileName(int index, const char* path = nullptr);
  void SplitFileName();
  ///@}

  /**
   * Callback registered with the InternalProgressObserver.
   */
  static void ProgressCallbackFunction(vtkObject*, unsigned long, void*, void*);

  /**
   * Valid at end of WriteInternal to indicate if we're going to continue
   * execution.
   */
  vtkGetMacro(ContinuingExecution, bool);

  /**
   * Get the current piece to write
   */
  vtkGetMacro(CurrentPiece, int);

  /**
   * Progress callback from internal writer.
   */
  virtual void ProgressCallback(vtkAlgorithm* w);

  /**
   * Method used to delete all written files.
   */
  void DeleteFiles();

  /**
   * The observer to report progress from the internal writer.
   */
  vtkCallbackCommand* InternalProgressObserver;

  vtkMultiProcessController* Controller;

  int StartPiece;
  int EndPiece;
  int NumberOfPieces;
  int GhostLevel;
  int WriteSummaryFile;
  bool UseSubdirectory;

  char* PathName;
  char* FileNameBase;
  char* FileNameExtension;
  char* PieceFileNameExtension;

  /**
   * Flags used to keep track of which pieces were written out.
   */
  unsigned char* PieceWrittenFlags;

  /**
   * Initializes PieceFileNameExtension.
   */
  virtual void SetupPieceFileNameExtension();

private:
  vtkXMLPDataObjectWriter(const vtkXMLPDataObjectWriter&) = delete;
  void operator=(const vtkXMLPDataObjectWriter&) = delete;

  /**
   * Indicates the piece currently being written.
   */
  int CurrentPiece;

  /**
   * Set in WriteInternal() to request continued execution from the executive to
   * write more pieces.
   */
  bool ContinuingExecution;
};

VTK_ABI_NAMESPACE_END
#endif
