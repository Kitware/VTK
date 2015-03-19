/*=========================================================================
 *
=========================================================================*/
// .NAME vtkOBJImporter - import polydata + textures + actors
//                        from Wavefront .obj/.mtl files
// .SECTION Description


#ifndef __vtkOBJPolydataProcessor_h
#define __vtkOBJPolydataProcessor_h

#include "vtkSmartPointer.h"
#include "vtkIOImportModule.h" // For export macro
#include "vtkImporter.h"

class vtkRenderWindow;
class vtkRenderer;
class vtkPolydata;
class vtkOBJPolydataProcessor;

//! @note{updated by peter karasev, 2015 to read texture coordinates + material properties}
class VTKIOIMPORT_EXPORT vtkOBJImporter : public vtkImporter
{
public:
  static vtkOBJImporter *New();

  vtkTypeMacro(vtkOBJImporter,vtkImporter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the name of the file to read.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileNameMTL);
  vtkGetStringMacro(FileNameMTL);

protected:
  vtkOBJImporter();
  ~vtkOBJImporter();

  virtual int  ImportBegin() /*override*/;
  virtual void ImportEnd () /*override*/;
  virtual void ReadData() /* override */;

  vtkSmartPointer<vtkOBJPolydataProcessor>   Impl;
  char* FileName;
  char* FileNameMTL;

private:
  vtkOBJImporter(const vtkOBJImporter&);  // Not implemented.
  void operator=(const vtkOBJImporter&);  // Not implemented.
};



#endif
