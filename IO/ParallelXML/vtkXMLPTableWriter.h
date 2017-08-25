/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPTableWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLPTableWriter
 * @brief   Write PVTK XML UnstructuredGrid files.
 *
 * vtkXMLPTableWriter writes the PVTK XML Table
 * file format.  One table input can be written into a
 * parallel file format with any number of pieces spread across files.
 * The standard extension for this writer's file format is "pvtt".
 * This writer uses vtkXMLTableWriter to write the
 * individual piece files.
 *
 * @sa
 * vtkXMLTableWriter
*/

#ifndef vtkXMLPTableWriter_h
#define vtkXMLPTableWriter_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLWriter.h"

class vtkCallbackCommand;
class vtkMultiProcessController;
class vtkTable;
class vtkXMLTableWriter;

class VTKIOPARALLELXML_EXPORT vtkXMLPTableWriter : public vtkXMLWriter
{
public:
  static vtkXMLPTableWriter* New();
  vtkTypeMacro(vtkXMLPTableWriter, vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Get/Set the writer's input.
   */
  vtkTable* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the number of pieces that are being written in parallel.
   */
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);
  //@}

  //@{
  /**
   * Get/Set the range of pieces assigned to this writer.
   */
  vtkSetMacro(StartPiece, int);
  vtkGetMacro(StartPiece, int);
  vtkSetMacro(EndPiece, int);
  vtkGetMacro(EndPiece, int);
  //@}

  //@{
  /**
   * Get/Set the ghost level used for this writer's piece.
   */
  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);
  //@}

  //@{
  /**
   * Get/Set whether to use a subdirectory to store the pieces
   */
  vtkSetMacro(UseSubdirectory, bool);
  vtkGetMacro(UseSubdirectory, bool);
  //@}

  //@{
  /**
   * Get/Set whether the writer should write the summary file that
   * refers to all of the pieces' individual files.
   * This is on by default. Note that only the first process writes
   * the summary file.
   */
  virtual void SetWriteSummaryFile(int flag);
  vtkGetMacro(WriteSummaryFile, int);
  vtkBooleanMacro(WriteSummaryFile, int);
  //@}

  //@{
  /**
   * Controller used to communicate data type of blocks.
   * By default, the global controller is used. If you want another
   * controller to be used, set it with this.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  /**
   * Overridden to handle passing the CONTINUE_EXECUTING() flags to the
   * executive.
   */
  int ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;

protected:
  vtkXMLPTableWriter();
  ~vtkXMLPTableWriter() VTK_OVERRIDE;

  /**
  * see algorithm for more info
  */
  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  const char* GetDataSetName() VTK_OVERRIDE;
  vtkXMLTableWriter* CreateTablePieceWriter();

  vtkXMLWriter* CreatePieceWriter(int index);
  void WritePData(vtkIndent indent);

  /**
   * Overridden to make appropriate piece request from upstream.
   */
  virtual int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);
  /**
  * Override writing method from superclass.
  */
  int WriteInternal() VTK_OVERRIDE;

  /**
  * Collect information between ranks before writing the summary file.
  * This method is called on all ranks while summary file is only written on 1
  * rank (rank 0).
  */
  virtual void PrepareSummaryFile();

  void WritePrimaryElementAttributes(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  int WriteData() VTK_OVERRIDE;
  virtual void WritePPieceAttributes(int index);

  char* CreatePieceFileName(int index, const char* path = nullptr);
  void SplitFileName();
  virtual int WritePiece(int index);

  /**
  * Callback registered with the InternalProgressObserver.
  */
  static void ProgressCallbackFunction(vtkObject*, unsigned long, void*, void*);

  /**
   * Valid at end of WriteInternal to indicate if we're going to continue
   * execution.
   */
  vtkGetMacro(ContinuingExecution, bool);

  void WritePRowData(vtkDataSetAttributes* ds, vtkIndent indent);
  /**
  * Progress callback from internal writer.
  */

  /**
  * The observer to report progress from the internal writer.
  */
  vtkCallbackCommand* InternalProgressObserver;

  vtkMultiProcessController* Controller;

  virtual void ProgressCallback(vtkAlgorithm* w);

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

private:
  vtkXMLPTableWriter(const vtkXMLPTableWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLPTableWriter&) VTK_DELETE_FUNCTION;

  /**
   * Method used to delete all written files.
   */
  void DeleteFiles();

  /**
   * Initializes PieceFileNameExtension.
   */
  void SetupPieceFileNameExtension();

  /**
  * Indicates the piece currently being written.
  */
  int CurrentPiece;

  /**
  * Set in WriteInternal() to request continued execution from the executive to
  * write more pieces.
  */
  bool ContinuingExecution;

  /**
  * Flags used to keep track of which pieces were written out.
  */
  unsigned char* PieceWrittenFlags;
};

#endif
