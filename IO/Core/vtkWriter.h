/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWriter
 * @brief   abstract class to write data to file(s)
 *
 * vtkWriter is an abstract class for mapper objects that write their data
 * to disk (or into a communications port). All writers respond to Write()
 * method. This method insures that there is input and input is up to date.
 *
 * @warning
 * Every subclass of vtkWriter must implement a WriteData() method. Most likely
 * will have to create SetInput() method as well.
 *
 * @sa
 * vtkXMLDataSetWriter vtkDataSetWriter vtkImageWriter vtkMCubesWriter
*/

#ifndef vtkWriter_h
#define vtkWriter_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkAlgorithm.h"

class vtkDataObject;

#define VTK_ASCII 1
#define VTK_BINARY 2

class VTKIOCORE_EXPORT vtkWriter : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkWriter,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Write data to output. Method executes subclasses WriteData() method, as
   * well as StartMethod() and EndMethod() methods.
   * Returns 1 on success and 0 on failure.
   */
  virtual int Write();

  /**
   * Encode the string so that the reader will not have problems.
   * The resulting string is up to three times the size of the input
   * string.  doublePercent indicates whether to output a double '%' before
   * escaped characters so the string may be used as a printf format string.
   */
  void EncodeString(char* resname, const char* name, bool doublePercent);

  /**
   * Encode the string so that the reader will not have problems.
   * The resulting string is up to three times the size of the input
   * string.  Write the string to the output stream.
   * doublePercent indicates whether to output a double '%' before
   * escaped characters so the string may be used as a printf format string.
   */
  void EncodeWriteString(ostream* out, const char* name, bool doublePercent);

  //@{
  /**
   * Set/get the input to this writer.
   */
  void SetInputData(vtkDataObject *input);
  void SetInputData(int index, vtkDataObject *input);
  //@}

  vtkDataObject *GetInput();
  vtkDataObject *GetInput(int port);

protected:
  vtkWriter();
  ~vtkWriter();

  virtual int ProcessRequest(vtkInformation *request,
                             vtkInformationVector **inputVector,
                             vtkInformationVector *outputVector);
  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

  virtual void WriteData() = 0; //internal method subclasses must respond to
  vtkTimeStamp WriteTime;
private:
  vtkWriter(const vtkWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkWriter&) VTK_DELETE_FUNCTION;
};

#endif
