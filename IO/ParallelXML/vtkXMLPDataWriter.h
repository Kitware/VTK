/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPDataWriter - Write data in a parallel XML format.
// .SECTION Description
// vtkXMLPDataWriter is the superclass for all XML parallel data set
// writers.  It provides functionality needed for writing parallel
// formats, such as the selection of which writer writes the summary
// file and what range of pieces are assigned to each serial writer.

#ifndef vtkXMLPDataWriter_h
#define vtkXMLPDataWriter_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLWriter.h"

class vtkCallbackCommand;
class vtkMultiProcessController;

class VTKIOPARALLELXML_EXPORT vtkXMLPDataWriter : public vtkXMLWriter
{
public:
  vtkTypeMacro(vtkXMLPDataWriter,vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the number of pieces that are being written in parallel.
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);

  // Description:
  // Get/Set the range of pieces assigned to this writer.
  vtkSetMacro(StartPiece, int);
  vtkGetMacro(StartPiece, int);
  vtkSetMacro(EndPiece, int);
  vtkGetMacro(EndPiece, int);

  // Description:
  // Get/Set the ghost level used for this writer's piece.
  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);

  // Description:
  // Get/Set whether the writer should write the summary file that
  // refers to all of the pieces' individual files.
  // This is on by default. Note that only the first process writes
  // the summary file.
  virtual void SetWriteSummaryFile(int flag);
  vtkGetMacro(WriteSummaryFile, int);
  vtkBooleanMacro(WriteSummaryFile, int);

  // Description:
  // Controller used to communicate data type of blocks.
  // By default, the global controller is used. If you want another
  // controller to be used, set it with this.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

protected:
  vtkXMLPDataWriter();
  ~vtkXMLPDataWriter();

  // Override writing method from superclass.
  virtual int WriteInternal();

  virtual vtkXMLWriter* CreatePieceWriter(int index)=0;

  virtual void WritePrimaryElementAttributes(ostream &os, vtkIndent indent);
  int WriteData();
  virtual void WritePData(vtkIndent indent);
  virtual void WritePPieceAttributes(int index);

  char* CreatePieceFileName(int index, const char* path=0);
  void SplitFileName();
  virtual int WritePieces();
  virtual int WritePiece(int index);

  // Callback registered with the ProgressObserver.
  static void ProgressCallbackFunction(vtkObject*, unsigned long, void*,
                                       void*);
  // Progress callback from internal writer.
  virtual void ProgressCallback(vtkAlgorithm* w);

  int StartPiece;
  int EndPiece;
  int NumberOfPieces;
  int GhostLevel;
  int WriteSummaryFile;

  char* PathName;
  char* FileNameBase;
  char* FileNameExtension;
  char* PieceFileNameExtension;

  // The observer to report progress from the internal writer.
  vtkCallbackCommand* ProgressObserver;

  vtkMultiProcessController* Controller;

private:
  vtkXMLPDataWriter(const vtkXMLPDataWriter&);  // Not implemented.
  void operator=(const vtkXMLPDataWriter&);  // Not implemented.
};

#endif
