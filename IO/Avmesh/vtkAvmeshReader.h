#ifndef vtkAvmeshReader_h
#define vtkAvmeshReader_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"
#include <string>

class AvmeshMetadata;
class AvmeshPatch;

typedef std::array<int, 5> Bface; // enough to hold a quad plus a patch ID
typedef std::vector<Bface> BfaceList;

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

  bool ReadMetadata(std::ifstream& fin, AvmeshMetadata& meta) const;

  bool CheckAssumptions(AvmeshMetadata const& meta) const;

  vtkSmartPointer<vtkPoints> ReadVolumeVerts(std::ifstream& fin, int nNodes) const;

  vtkSmartPointer<vtkUnstructuredGrid> AddBlock(
    vtkMultiBlockDataSet* output, const char* blockName) const;

  void Read2DSurfaceConn(
    std::ifstream& fin, int nbnd, BfaceList& bfaces, bool fileHasNeighbors) const;

  void Read3DSurfaceConn(
    std::ifstream& fin, int ntri, int nquad, BfaceList& bfaces, bool fileHasNeighbors) const;

  void Read2DVolumeConn(std::ifstream& fin, int nquad, int ntri, vtkUnstructuredGrid* ugrid) const;

  void Read3DVolumeConn(
    std::ifstream& fin, int nhex, int ntet, int npri, int npyr, vtkUnstructuredGrid* ugrid) const;

  void Read3DVolumeConnOfType(
    std::ifstream& fin, int etype, int ncell, vtkUnstructuredGrid* ugrid) const;

  int NodesPerCell(int etype) const;

  void BuildBoundaryBlocks(vtkMultiBlockDataSet* output, vtkPoints* volPoints,
    std::vector<AvmeshPatch> const& patches, BfaceList& bfaces) const;

  void BuildSurfaceBlock(int pid, vtkUnstructuredGrid* surfGrid, vtkPoints* volPoints,
    BfaceList::const_iterator faceBegin, BfaceList::const_iterator faceEnd) const;

  // Binary I/O helpers --------------------------------------------------------

  void ReadString(std::ifstream& fin, char* str, int length = 128) const;
  void ReadVarString(std::ifstream& fin, std::string& str) const;
  int ReadInt(std::ifstream& fin) const;
  double ReadDouble(std::ifstream& fin) const;

  template <typename T>
  void ReadArray(std::ifstream& fin, T* arr, size_t nitems) const
  {
    fin.read((char*)arr, nitems * sizeof(T)); // FIXME byteswapping
  }

  // Data ----------------------------------------------------------------------

  std::string FileName;
  bool SurfaceOnly = false;
};

#endif // vtkAvmeshReader_h
