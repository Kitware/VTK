/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkX3DExporterWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkX3DExporterWriter - X3D Exporter Writer
// .SECTION Description
// vtkX3DExporterWriter is the definition for
// classes that implement a encoding for the
// X3D exporter

#ifndef vtkX3DExporterWriter_h
#define vtkX3DExporterWriter_h

#include "vtkIOExportModule.h" // For export macro
#include "vtkObject.h"

// Forward declarations
class vtkDataArray;
class vtkUnsignedCharArray;
class vtkCellArray;

class VTKIOEXPORT_EXPORT vtkX3DExporterWriter : public vtkObject
{
public:
  vtkTypeMacro(vtkX3DExporterWriter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Opens the file specified with file
  // returns 1 if successful otherwise 0
  virtual int OpenFile(const char* file) = 0;

  // Description:
  // Init data support to be a stream instead of a file
  virtual int OpenStream() = 0;

  // Description:
  // Enable writing to an OutputString instead of the default, a file.
  vtkSetMacro(WriteToOutputString,int);
  vtkGetMacro(WriteToOutputString,int);
  vtkBooleanMacro(WriteToOutputString,int);

  // Description:
  // When WriteToOutputString in on, then a string is allocated, written to,
  // and can be retrieved with these methods.  The string is deleted during
  // the next call to write ...
  vtkGetMacro(OutputStringLength, int);
  vtkGetStringMacro(OutputString);
  unsigned char *GetBinaryOutputString()
    {
      return reinterpret_cast<unsigned char *>(this->OutputString);
    }

  // Description:
  // This convenience method returns the string, sets the IVAR to NULL,
  // so that the user is responsible for deleting the string.
  // I am not sure what the name should be, so it may change in the future.
  char *RegisterAndGetOutputString();

  // Closes the file if open
  virtual void CloseFile() = 0;
  // Flush can be called optionally after some operations to
  // flush the buffer to the filestream. A writer not necessarily
  // implements this function
  virtual void Flush() {}

  // Description:
  // Starts a document and sets all necessary informations,
  // i.e. the header of the implemented encoding
  virtual void StartDocument() = 0;

  // Description:
  // Ends a document and sets all necessary informations
  // or necessary bytes to finish the encoding correctly
  virtual void EndDocument() = 0;

  // Description:
  // Starts/ends a new X3D node specified via nodeID. The list of
  // nodeIds can be found in vtkX3DExportWriterSymbols.h. The EndNode
  // function closes the last open node. So there must be
  // corresponding Start/EndNode() calls for every node
  virtual void StartNode(int nodeID) = 0;
  virtual void EndNode() = 0;

  // Description:
  // Sets the field specified with attributeID
  // of the active node to the given value.
  // The type of the field is SFString and MFString
  //virtual void SetField(int attributeID, const std::string &value) = 0;
  virtual void SetField(int attributeID, const char* value, bool mfstring = false) = 0;

  // Description:
  // Sets the field specified with attributeID
  // of the active node to the given value.
  // The type of the field is SFInt32
  virtual void SetField(int attributeID, int) = 0;

  // Description:
  // Sets the field specified with attributeID
  // of the active node to the given value.
  // The type of the field is SFFloat
  virtual void SetField(int attributeID, float) = 0;

  // Description:
  // Sets the field specified with attributeID
  // of the active node to the given value.
  // The type of the field is SFDouble
  virtual void SetField(int attributeID, double) = 0;

  // Description:
  // Sets the field specified with attributeID
  // of the active node to the given value.
  // The type of the field is SFBool
  virtual void SetField(int attributeID, bool) = 0;

  // Description:
  // Sets the field specified with attributeID
  // of the active node to the given value.
  // The type of the field is specified with type
  // Supported types: SFVEC3F, SFCOLOR, SFROTATION
  virtual void SetField(int attributeID, int type, const double* a) = 0;

  // Description:
  // Sets the field specified with attributeID
  // of the active node to the given value.
  // The type of the field is specified with type
  // Supported types: MFVEC3F, MFVEC2F
  virtual void SetField(int attributeID, int type, vtkDataArray* a) = 0;

  // Description:
  // Sets the field specified with attributeID
  // of the active node to the given value.
  // The type of the field is specified with type
  // Supported types: MFCOLOR
  virtual void SetField(int attributeID, const double* values, size_t size) = 0;

    // Description:
  // Sets the field specified with attributeID
  // of the active node to the given value.
  // The type of the field is specified with type
  // It is possible to specify that the field is an
  // image for optimized formating or compression
  // Supported types: MFINT32, SFIMAGE
  virtual void SetField(int attributeID, const int* values, size_t size, bool image = false) = 0;

  // Description:
  // Sets the field specified with attributeID
  // of the active node to the given value.
  // The type of the field is specified with type
  // Supported types: MFString
  //virtual void SetField(int attributeID, int type, std::string) = 0;
protected:
  vtkX3DExporterWriter();
  ~vtkX3DExporterWriter();

  char *OutputString;
  int OutputStringLength;
  int WriteToOutputString;

private:
  vtkX3DExporterWriter(const vtkX3DExporterWriter&); // Not implemented.
  void operator=(const vtkX3DExporterWriter&); // Not implemented.
};
#endif

