#ifndef vtkAvmeshReader_h
#define vtkAvmeshReader_h

#include <string>

#include <vtkMultiBlockDataSetAlgorithm.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>

class AvmeshMetadata;
class AvmeshPatch;
class BinaryFile;

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

  bool ReadMetadata(BinaryFile& fin, AvmeshMetadata& meta) const;

  bool CheckAssumptions(AvmeshMetadata const& meta) const;

  vtkSmartPointer<vtkPoints> ReadVolumeVerts(BinaryFile& fin, int nNodes) const;

  vtkSmartPointer<vtkUnstructuredGrid> AddBlock(
    vtkMultiBlockDataSet* output, const char* blockName) const;

  void Read2DSurfaceConn(BinaryFile& fin, int nbnd, BfaceList& bfaces, bool fileHasNeighbors) const;

  void Read3DSurfaceConn(
    BinaryFile& fin, int ntri, int nquad, BfaceList& bfaces, bool fileHasNeighbors) const;

  void Read2DVolumeConn(BinaryFile& fin, int nquad, int ntri, vtkUnstructuredGrid* ugrid) const;

  void Read3DVolumeConn(
    BinaryFile& fin, int nhex, int ntet, int npri, int npyr, vtkUnstructuredGrid* ugrid) const;

  void Read3DVolumeConnOfType(
    BinaryFile& fin, int etype, int ncell, vtkUnstructuredGrid* ugrid) const;

  int NodesPerCell(int etype) const;

  void BuildBoundaryBlocks(vtkMultiBlockDataSet* output, vtkPoints* volPoints,
    std::vector<AvmeshPatch> const& patches, BfaceList& bfaces) const;

  void BuildSurfaceBlock(int pid, vtkUnstructuredGrid* surfGrid, vtkPoints* volPoints,
    BfaceList::const_iterator faceBegin, BfaceList::const_iterator faceEnd) const;

  // Data ----------------------------------------------------------------------

  std::string FileName;
  bool SurfaceOnly = false;
};

#endif // vtkAvmeshReader_h
