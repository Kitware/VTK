/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkX3DExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkX3DExporter
 * @brief   create an x3d file
 *
 * vtkX3DExporter is a render window exporter which writes out the renderered
 * scene into an X3D file. X3D is an XML-based format for representation
 * 3D scenes (similar to VRML). Check out http://www.web3d.org/x3d/ for more
 * details.
 * @par Thanks:
 * X3DExporter is contributed by Christophe Mouton at EDF.
*/

#ifndef vtkX3DExporter_h
#define vtkX3DExporter_h

#include "vtkIOExportModule.h" // For export macro
#include "vtkExporter.h"

class vtkActor;
class vtkActor2D;
class vtkDataArray;
class vtkLight;
class vtkPoints;
class vtkPolyData;
class vtkRenderer;
class vtkUnsignedCharArray;
class vtkX3DExporterWriter;

class VTKIOEXPORT_EXPORT vtkX3DExporter : public vtkExporter
{
public:
  static vtkX3DExporter *New();
  vtkTypeMacro(vtkX3DExporter,vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get the output file name.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Specify the Speed of navigation. Default is 4.
   */
  vtkSetMacro(Speed,double);
  vtkGetMacro(Speed,double);
  //@}

  //@{
  /**
   * Turn on binary mode
   */
  vtkSetClampMacro(Binary, vtkTypeBool, 0, 1);
  vtkBooleanMacro(Binary, vtkTypeBool);
  vtkGetMacro(Binary, vtkTypeBool);
  //@}

  //@{
  /**
   * In binary mode use fastest instead of best compression
   */
  vtkSetClampMacro(Fastest, vtkTypeBool, 0, 1);
  vtkBooleanMacro(Fastest, vtkTypeBool);
  vtkGetMacro(Fastest, vtkTypeBool);
  //@}

  //@{
  /**
   * Enable writing to an OutputString instead of the default, a file.
   */
  vtkSetMacro(WriteToOutputString,vtkTypeBool);
  vtkGetMacro(WriteToOutputString,vtkTypeBool);
  vtkBooleanMacro(WriteToOutputString,vtkTypeBool);
  //@}

  //@{
  /**
   * When WriteToOutputString in on, then a string is allocated, written to,
   * and can be retrieved with these methods.  The string is deleted during
   * the next call to write ...
   */
  vtkGetMacro(OutputStringLength, int);
  vtkGetStringMacro(OutputString);
  unsigned char *GetBinaryOutputString()
  {
      return reinterpret_cast<unsigned char *>(this->OutputString);
  }
  //@}

  /**
   * This convenience method returns the string, sets the IVAR to nullptr,
   * so that the user is responsible for deleting the string.
   * I am not sure what the name should be, so it may change in the future.
   */
  char *RegisterAndGetOutputString();

protected:
  vtkX3DExporter();
  ~vtkX3DExporter() override;

  // Stream management
  vtkTypeBool WriteToOutputString;
  char *OutputString;
  int OutputStringLength;

  /**
   * Write data to output.
   */
  void WriteData() override;

  void WriteALight(vtkLight *aLight, vtkX3DExporterWriter* writer);
  void WriteAnActor(vtkActor *anActor, vtkX3DExporterWriter* writer,
    int index);
  void WriteAPiece(vtkPolyData* piece, vtkActor *anActor, vtkX3DExporterWriter* writer, int index);
  void WritePointData(vtkPoints *points, vtkDataArray *normals,
    vtkDataArray *tcoords, vtkUnsignedCharArray *colors,
    vtkX3DExporterWriter* writer, int index);
  void WriteATextActor2D(vtkActor2D *anTextActor2D,
    vtkX3DExporterWriter* writer);
  void WriteATexture(vtkActor *anActor, vtkX3DExporterWriter* writer);
  void WriteAnAppearance(vtkActor *anActor, bool writeEmissiveColor, vtkX3DExporterWriter* writer);

  // Called to give subclasses a chance to write additional nodes to the file.
  // Default implementation does nothing.
  virtual void WriteAdditionalNodes(vtkX3DExporterWriter* vtkNotUsed(writer)) {}

  int HasHeadLight(vtkRenderer* ren);

  char *FileName;
  double Speed;
  vtkTypeBool Binary;
  vtkTypeBool Fastest;

private:

  vtkX3DExporter(const vtkX3DExporter&) = delete;
  void operator=(const vtkX3DExporter&) = delete;
};


#endif
