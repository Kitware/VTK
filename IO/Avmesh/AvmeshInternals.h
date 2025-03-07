#ifndef AvmeshInternals_h
#define AvmeshInternals_h

#include <stdexcept>
#include <string>

class vtkMultiBlockDataSet;

void ReadAvmesh(vtkMultiBlockDataSet* output, std::string fname, bool SurfaceOnly);

class AvmeshError : public std::runtime_error
{
public:
  AvmeshError(std::string msg);
};

#endif // AvmeshInternals_h
