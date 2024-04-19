#ifndef vtkCesium3DTilesHeader_h
#define vtkCesium3DTilesHeader_h

#include "vtkIOCesium3DTilesModule.h" // For export macro
#include <cstdint>

struct Cesium3DTilesHeader
{
  char magic[4];
  uint32_t version;
  uint32_t byteLength;
  uint32_t featureTableJSONByteLength;
  uint32_t featureTableBinaryByteLength;
  uint32_t batchTableJSONByteLength;
  uint32_t batchTableBinaryByteLength;
};

using PNTSHeader = Cesium3DTilesHeader;
using B3DMHeader = Cesium3DTilesHeader;

#endif
