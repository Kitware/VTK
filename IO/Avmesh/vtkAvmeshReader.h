#ifndef vtkAvmeshReader_h
#define vtkAvmeshReader_h

#include <string>
#include <vtkMultiBlockDataSetAlgorithm.h>

class vtkAvmeshReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkAvmeshReader* New();
  vtkTypeMacro(vtkAvmeshReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetStdStringFromCharMacro(FileName);
  vtkGetCharFromStdStringMacro(FileName);

  int CanReadFile(VTK_FILEPATH const char* filename);

  vtkSetMacro(SurfaceOnly, bool);
  vtkGetMacro(SurfaceOnly, bool);
  vtkBooleanMacro(SurfaceOnly, bool);

protected:
  vtkAvmeshReader();
  ~vtkAvmeshReader() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkAvmeshReader(const vtkAvmeshReader&) = delete;
  void operator=(const vtkAvmeshReader&) = delete;

  std::string FileName;
  bool SurfaceOnly = false;
};

#endif // vtkAvmeshReader_h
