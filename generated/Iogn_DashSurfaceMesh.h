// Copyright(C) 1999-2025 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_Beam2.h"
#include "Ioss_CodeTypes.h"
#include "Ioss_Hex8.h"
#include "Ioss_Shell4.h"
#include "generated/Iogn_GeneratedMesh.h" // for GeneratedMesh
#include <cstddef>                        // for size_t
#include <cstdint>                        // for int64_t
#include <exception>                      // for exception
#include <string>                         // for string
#include <utility>                        // for pair
#include <vector>                         // for vector

#include "iogn_export.h"
#include "vtk_ioss_mangle.h"

namespace Iogn {

  enum { INVALID = -1, NUMBER_OF_SURFACES = 2, SPATIAL_DIMENSION = 3, NUM_NODES_PER_QUAD_FACE = 4 };

  struct IOGN_EXPORT SharedNode
  {
    SharedNode() = default;
    int nodeId{-1};
    int procId{-1};
  };

  enum Topology { Beam2 = 2, Shell4 = 4, Hex8 = 8 };

  IOSS_NODISCARD inline std::string getTopologyName(Topology topology)
  {
    switch (topology) {
    case Shell4: return {Ioss::Shell4::name};
    case Hex8: return {Ioss::Hex8::name};
    case Beam2: return {Ioss::Beam2::name};
    }
    throw std::exception();
  }

  struct IOGN_EXPORT ExodusData
  {
    std::vector<double>                 coordinates{};
    const std::vector<std::vector<int>> elementBlockConnectivity;
    const std::vector<int>              globalNumberOfElementsInBlock{};
    const std::vector<int>              localNumberOfElementsInBlock{};
    const std::vector<Topology>         blockTopologicalData{};

    const int globalNumberOfNodes{0};

    const std::vector<int> globalIdsOfLocalElements{};
    const std::vector<int> globalIdsOfLocalNodes{};

    std::vector<SharedNode> sharedNodes{};

    // A sideset' is basically an exodus sideset.  A
    // sideset has a list of elements and a corresponding local
    // element side (1-based) The side id is: side_id =
    // 10*element_id + local_side_number This assumes that all
    // sides in a sideset are boundary sides.
    std::vector<std::vector<int>> sidesetConnectivity;
    std::vector<Ioss::NameList>   sidesetTouchingBlocks;

    ExodusData() = delete;
    ExodusData(std::vector<double> coords, std::vector<std::vector<int>> elemBlockConnectivity,
               std::vector<int> globalNumOfElemsInBlock, std::vector<int> localNumOfElemsInBlock,
               std::vector<Topology> blockTopoData, int globalNumNodes,
               std::vector<int> globalIdsOfLocalElems, std::vector<int> globalIdsLocalNodes,
               std::vector<std::vector<int>> sidesetConn   = std::vector<std::vector<int>>(),
               std::vector<Ioss::NameList>   sidesetBlocks = std::vector<Ioss::NameList>())
        : coordinates(std::move(coords)),
          elementBlockConnectivity(std::move(elemBlockConnectivity)),
          globalNumberOfElementsInBlock(std::move(globalNumOfElemsInBlock)),
          localNumberOfElementsInBlock(std::move(localNumOfElemsInBlock)),
          blockTopologicalData(std::move(blockTopoData)), globalNumberOfNodes(globalNumNodes),
          globalIdsOfLocalElements(std::move(globalIdsOfLocalElems)),
          globalIdsOfLocalNodes(std::move(globalIdsLocalNodes)),
          sidesetConnectivity(std::move(sidesetConn)),
          sidesetTouchingBlocks(std::move(sidesetBlocks))
    {
    }
  };

  struct IOGN_EXPORT DashSurfaceData
  {
    const std::vector<double> coordinates{};
    const std::vector<int>    surfaceAConnectivity{};
    const std::vector<int>    surfaceBConnectivity{};

    int globalNumberOfNodes{};
    int globalNumberOfElements{};

    int globalNumberOfElementsSurface1{};
    int globalNumberOfElementsSurface2{};

    std::vector<int> globalIdsOfLocalElements{};
    std::vector<int> globalIdsOfLocalNodes{};

    std::vector<SharedNode> sharedNodes{};

    DashSurfaceData(std::vector<double> coords, std::vector<int> connectivity1,
                    std::vector<int> connectivity2)
        : coordinates(std::move(coords)), surfaceAConnectivity(std::move(connectivity1)),
          surfaceBConnectivity(std::move(connectivity2))
    {
      this->setSerialDefaults();
    }

  private:
    void setSerialDefaults()
    {
      globalNumberOfNodes = coordinates.size() / SPATIAL_DIMENSION;

      globalNumberOfElementsSurface1 = surfaceBConnectivity.size() / NUM_NODES_PER_QUAD_FACE;
      globalNumberOfElementsSurface2 = surfaceAConnectivity.size() / NUM_NODES_PER_QUAD_FACE;
      globalNumberOfElements = globalNumberOfElementsSurface1 + globalNumberOfElementsSurface2;

      globalIdsOfLocalElements.resize(globalNumberOfElements);
      globalIdsOfLocalNodes.resize(globalNumberOfNodes);

      for (size_t i = 0; i < globalIdsOfLocalElements.size(); i++) {
        globalIdsOfLocalElements[i] = i + 1;
      }

      for (size_t i = 0; i < globalIdsOfLocalNodes.size(); i++) {
        globalIdsOfLocalNodes[i] = i + 1;
      }
    }
  };

  class IOGN_EXPORT DashSurfaceMesh : public GeneratedMesh
  {
  public:
    explicit DashSurfaceMesh(DashSurfaceData &dashSurfaceData) : mDashSurfaceData(dashSurfaceData)
    {
    }

    IOSS_NODISCARD int64_t node_count() const override;
    IOSS_NODISCARD int64_t node_count_proc() const override;

    IOSS_NODISCARD int64_t element_count() const override;
    IOSS_NODISCARD int64_t element_count(int64_t surfaceNumber) const override;
    IOSS_NODISCARD int64_t element_count_proc() const override;
    IOSS_NODISCARD int64_t element_count_proc(int64_t block_number) const override;

    IOSS_NODISCARD int block_count() const override;

    IOSS_NODISCARD int     nodeset_count() const override;
    IOSS_NODISCARD int64_t nodeset_node_count_proc(int64_t id) const override;

    IOSS_NODISCARD int     sideset_count() const override;
    IOSS_NODISCARD int64_t sideset_side_count_proc(int64_t id) const override;

    IOSS_NODISCARD int64_t communication_node_count_proc() const override;

    void coordinates(double *coord) const override;
    void coordinates(std::vector<double> &coord) const override;
    void coordinates(int component, std::vector<double> &xyz) const override;
    void coordinates(int component, double *xyz) const override;
    void coordinates(std::vector<double> &x, std::vector<double> &y,
                     std::vector<double> &z) const override;

    void connectivity(int64_t block_number, int *connect) const override;

    IOSS_NODISCARD std::pair<std::string, int> topology_type(int64_t block_number) const override;

    void sideset_elem_sides(int64_t setId, std::vector<int64_t> &elem_sides) const override;

    void nodeset_nodes(int64_t nset_id, std::vector<int64_t> &nodes) const override;

    void node_communication_map(std::vector<int64_t> &map, std::vector<int> &proc) override;

    void node_map(std::vector<int> &map) const override;
    void node_map(std::vector<int64_t> &map) const override;

    void element_map(int64_t block_number, std::vector<int> &map) const override;
    void element_map(int64_t block_number, std::vector<int64_t> &map) const override;
    void element_map(std::vector<int64_t> &map) const override;
    void element_map(std::vector<int> &map) const override;

  private:
    IOSS_NODISCARD std::string get_sideset_topology() const override;

    DashSurfaceData mDashSurfaceData;
  };

  class IOGN_EXPORT ExodusMesh : public GeneratedMesh
  {
  public:
    explicit ExodusMesh(const ExodusData &exodusData);

    IOSS_NODISCARD int64_t node_count() const override;
    IOSS_NODISCARD int64_t node_count_proc() const override;

    IOSS_NODISCARD int64_t element_count() const override;
    IOSS_NODISCARD int64_t element_count(int64_t blockNumber) const override;
    IOSS_NODISCARD int64_t element_count_proc() const override;
    IOSS_NODISCARD int64_t element_count_proc(int64_t blockNumber) const override;

    IOSS_NODISCARD int block_count() const override;

    IOSS_NODISCARD int     nodeset_count() const override;
    IOSS_NODISCARD int64_t nodeset_node_count_proc(int64_t id) const override;

    IOSS_NODISCARD int     sideset_count() const override;
    IOSS_NODISCARD int64_t sideset_side_count_proc(int64_t id) const override;

    IOSS_NODISCARD int64_t communication_node_count_proc() const override;

    void coordinates(double *coord) const override;
    void coordinates(std::vector<double> &coord) const override;
    void coordinates(int component, std::vector<double> &xyz) const override;
    void coordinates(int component, double *xyz) const override;
    void coordinates(std::vector<double> &x, std::vector<double> &y,
                     std::vector<double> &z) const override;

    void connectivity(int64_t blockNumber, int *connectivityForBlock) const override;

    IOSS_NODISCARD std::pair<std::string, int> topology_type(int64_t blockNumber) const override;

    void sideset_elem_sides(int64_t setId, std::vector<int64_t> &elem_sides) const override;

    IOSS_NODISCARD Ioss::NameList sideset_touching_blocks(int64_t setId) const override;

    void nodeset_nodes(int64_t nset_id, std::vector<int64_t> &nodes) const override;

    void node_communication_map(std::vector<int64_t> &map, std::vector<int> &proc) override;

    void node_map(std::vector<int> &map) const override;
    void node_map(std::vector<int64_t> &map) const override;

    void element_map(int64_t blockNumber, std::vector<int> &map) const override;
    void element_map(int64_t blockNumber, std::vector<int64_t> &map) const override;
    void element_map(std::vector<int64_t> &map) const override;
    void element_map(std::vector<int> &map) const override;

  private:
    IOSS_NODISCARD std::string get_sideset_topology() const override;

    int64_t mGlobalNumberOfElements;
    int64_t mLocalNumberOfElements;

    const ExodusData    &mExodusData;
    std::vector<int64_t> mElementOffsetForBlock;
  };
} // namespace Iogn
