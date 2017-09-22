/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfMap.hpp                                                         */
/*                                                                           */
/*  Author:                                                                  */
/*     Kenneth Leiter                                                        */
/*     kenneth.leiter@arl.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2011 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/

#ifndef XDMFMAP_HPP_
#define XDMFMAP_HPP_

// C Compatible Includes
#include "Xdmf.hpp"
#include "XdmfItem.hpp"

#ifdef __cplusplus

// Forward Declarations
class XdmfArray;
class XdmfAttribute;
class XdmfHeavyDataController;

// Includes

#include <set>

/**
 * @brief Boundary communicator map for partitioned spatial
 * collections.
 *
 * Provides mechanism for mapping nodes across partition
 * boundaries. Each partitioned grid contains its own map, mapping its
 * own nodes to all other nodes in the global system.
 *
 * There are two methods for constructing XdmfMaps:
 *
 * Calling New() with no parameters will construct an empty map. The
 * map can be filled manually with subsequent insert commands.
 *
 * Calling New(const std::vector<shared_ptr<XdmfAttribute> > &
 * globalNodeIds) will construct XdmfMaps for each grid in an entire
 * global system. Each entry in the vector contains the globalNodeIds
 * for that partition. The constructor accepts global node ids for
 * each partition to construct the proper XdmfMaps.
 */
class XDMF_EXPORT XdmfMap : public XdmfItem {

public:

  typedef int node_id;
  typedef int task_id;
  typedef std::map<node_id, std::set<node_id> > node_id_map;

  /**
   * Create a new XdmfMap.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfMap.cpp
   * @skipline //#initialization
   * @until //#initialization
   *
   * Python
   *
   * @dontinclude XdmfExampleMap.py
   * @skipline #//initialization
   * @until #//initialization
   *
   * @return    Constructed XdmfMap.
   */
  static shared_ptr<XdmfMap> New();

  /**
   * Create XdmfMaps for each grid in a domain decomposed mesh. Each
   * entry in the globalNodeIds vector contains the global node ids
   * for that partition.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfMap.cpp
   * @skipline //#initializationnode
   * @until //#initializationnode
   *
   * Python
   *
   * @dontinclude XdmfExampleMap.py
   * @skipline #//initializationnode
   * @until #//initializationnode
   *
   * @param     globalNodeIds   A vector of attributes containing globalNodeId
   *                            values for each partition to be mapped.
   *
   * @return                    Constructed XdmfMaps for each partition. The
   *                            size of the vector will be the same as the
   *                            globalNodeIds vector.
   */
  static std::vector<shared_ptr<XdmfMap> >
  New(const std::vector<shared_ptr<XdmfAttribute> > & globalNodeIds);

  virtual ~XdmfMap();

  LOKI_DEFINE_VISITABLE(XdmfMap, XdmfItem)
  static const std::string ItemTag;

  std::map<std::string, std::string> getItemProperties() const;

  /**
   * Get stored boundary communicator map.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfMap.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getMap
   * @until //#getMap
   *
   * Python
   *
   * @dontinclude XdmfExampleMap.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getMap
   * @until #//getMap
   *
   * @return    Stored boundary communicator map.
   */
  std::map<task_id, node_id_map> getMap() const;

  /**
   * Get name of boundary communicator map.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfMap.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setName
   * @until //#setName
   * @skipline //#getName
   * @until //#getName
   *
   * Python
   *
   * @dontinclude XdmfExampleMap.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setName
   * @until #//setName
   * @skipline #//getName
   * @until #//getName
   *
   * @return    Name of boundary communicator map.
   */
  std::string getName() const;

  /**
   * Given a remote task id return a map of local node ids to remote
   * node ids
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfMap.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#getRemoteNodeIds
   * @until //#getRemoteNodeIds
   *
   * Python
   *
   * @dontinclude XdmfExampleMap.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//getRemoteNodeIds
   * @until #//getRemoteNodeIds
   *
   * @param     remoteTaskId    Task id to retrieve mapping for.
   *
   * @return                    A map of local node ids to a vector of
   *                            remote node ids on remoteTaskId.
   */
  node_id_map getRemoteNodeIds(const task_id remoteTaskId);

  std::string getItemTag() const;

  using XdmfItem::insert;

  /**
   * Insert a new entry in map.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfMap.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#inserttuple
   * @until //#inserttuple
   *
   * Python
   *
   * @dontinclude XdmfExampleMap.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//inserttuple
   * @until #//inserttuple
   *
   * @param     remoteTaskId            task id where the remoteLoalNodeId is
   *                                    located.
   * @param     localNodeId             The node id of the node being mapped.
   * @param     remoteLocalNodeId       A node id on the remoteTaskId that the
   *                                    localNodeId is mapped to.
   */
  void insert(const task_id  remoteTaskId,
              const node_id  localNodeId,
              const node_id  remoteLocalNodeId);

  /**
   * Returns whether the map is initialized (contains values in
   * memory).
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfMap.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#isInitialized
   * @until //#isInitialized
   *
   * Python
   *
   * @dontinclude XdmfExampleMap.py
   * @skipline //#initialization
   * @until //#initialization
   * @skipline #//isInitialized
   * @until #//isInitialized
   *
   * @return    bool true if map contains values in memory.
   */
  bool isInitialized() const;

  /**
   * Read data from disk into memory.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfMap.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#isInitialized
   * @until //#isInitialized
   *
   * Python
   *
   * @dontinclude XdmfExampleMap.py
   * @skipline //#initialization
   * @until //#initialization
   * @skipline #//isInitialized
   * @until #//isInitialized
   */
  void read();

  /**
   * Release all data held in memory. The heavy data remain attached.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfMap.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#release
   * @until //#release
   *
   * Python
   *
   * @dontinclude XdmfExampleMap.py
   * @skipline //#initialization
   * @until //#initialization
   * @skipline #//release
   * @until #//release
   */
  void release();

  /**
   * Set the heavy data controllers for this map.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfMap.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setHeavyDataController
   * @until //#setHeavyDataController
   *
   * Python
   *
   * @dontinclude XdmfExampleMap.py
   * @skipline //#initialization
   * @until //#initialization
   * @skipline #//setHeavyDataController
   * @until #//setHeavyDataController
   *
   * @param     remoteTaskIdsControllers        A vector of XdmfHeavyDataControllers 
   *                                            to the remote task ids dataset.
   * @param     localNodeIdsControllers         A vector of XdmfHeavyDataControllers
   *                                            to the local node ids dataset.
   * @param     remoteLocalNodeIdsControllers   A vector of XdmfHeavyDataControllers
   *                                            to the remote local node ids dataset.
   */
  void
  setHeavyDataControllers(std::vector<shared_ptr<XdmfHeavyDataController> > remoteTaskIdsControllers,
                          std::vector<shared_ptr<XdmfHeavyDataController> > localNodeIdsControllers,
                          std::vector<shared_ptr<XdmfHeavyDataController> > remoteLocalNodeIdsControllers);

  /**
   * Set the boundary communicator map.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfMap.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setMap
   * @until //#setMap
   *
   * Python
   *
   * @dontinclude XdmfExampleMap.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setMap
   * @until #//setMap
   *
   * @param     map     The boundary communicator map to store.
   */
  void setMap(std::map<task_id, node_id_map> map);

  /**
   * Set the name of the boundary communicator map.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfMap.cpp
   * @skipline //#initialization
   * @until //#initialization
   * @skipline //#setName
   * @until //#setName
   *
   * Python
   *
   * @dontinclude XdmfExampleMap.py
   * @skipline #//initialization
   * @until #//initialization
   * @skipline #//setName
   * @until #//setName
   *
   * @param     name    The name of the boundary communicator map to set.
   */
  void setName(const std::string & name);

  void traverse(const shared_ptr<XdmfBaseVisitor> visitor);

protected:

  XdmfMap();

  virtual void
  populateItem(const std::map<std::string, std::string> & itemProperties,
               const std::vector<shared_ptr<XdmfItem> > & childItems,
               const XdmfCoreReader * const reader);

private:

  XdmfMap(const XdmfMap & map);  // Not implemented.
  void operator=(const XdmfMap & map);  // Not implemented.

  std::vector<shared_ptr<XdmfHeavyDataController> > mLocalNodeIdsControllers;
  // remoteTaskId | localNodeId | remoteLocalNodeId
  std::map<task_id, node_id_map > mMap;
  std::string mName;
  std::vector<shared_ptr<XdmfHeavyDataController> > mRemoteLocalNodeIdsControllers;
  std::vector<shared_ptr<XdmfHeavyDataController> > mRemoteTaskIdsControllers;

};

#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers go here

struct XDMFMAP; // Simply as a typedef to ensure correct typing
typedef struct XDMFMAP XDMFMAP;

XDMF_EXPORT XDMFMAP * XdmfMapNew();

XDMF_EXPORT XDMFMAP ** XdmfMapNewFromIdVector(int ** globalNodeIds, int * numIdsOnNode, int numIds);

XDMF_EXPORT char * XdmfMapGetName(XDMFMAP * map);

XDMF_EXPORT void XdmfMapInsert(XDMFMAP * map, int remoteTaskId, int localNodeId, int remoteLocalNodeId);

XDMF_EXPORT int XdmfMapIsInitialized(XDMFMAP * map);

XDMF_EXPORT void XdmfMapRead(XDMFMAP * map, int * status);

XDMF_EXPORT void XdmfMapRelease(XDMFMAP * map);

XDMF_EXPORT int * XdmfMapRetrieveLocalNodeIds(XDMFMAP * map, int remoteTaskId);

XDMF_EXPORT int XdmfMapRetrieveNumberLocalNodeIds(XDMFMAP * map, int remoteTaskId);

XDMF_EXPORT int XdmfMapRetrieveNumberRemoteTaskIds(XDMFMAP * map);

XDMF_EXPORT int XdmfMapRetrieveNumberRemoteNodeIds(XDMFMAP * map, int remoteTaskId, int localNodeId);

XDMF_EXPORT int * XdmfMapRetrieveRemoteTaskIds(XDMFMAP * map);

XDMF_EXPORT int * XdmfMapRetrieveRemoteNodeIds(XDMFMAP * map, int remoteTaskId, int localNodeId);

XDMF_EXPORT void XdmfMapSetHeavyDataControllers(XDMFMAP * map,
                                                XDMFHEAVYDATACONTROLLER ** remoteTaskControllers,
                                                int numRemoteTaskControllers,
                                                XDMFHEAVYDATACONTROLLER ** localNodeControllers,
                                                int numberLocalNodeControllers,
                                                XDMFHEAVYDATACONTROLLER ** remoteLocalNodeControllers,
                                                int numRemoteLocalNodeControllers,
                                                int passControl,
                                                int * status);

XDMF_EXPORT void XdmfMapSetName(XDMFMAP * map, char * newName);

XDMF_ITEM_C_CHILD_DECLARE(XdmfMap, XDMFMAP, XDMF)

#ifdef __cplusplus
}
#endif

#endif /* XDMFMAP_HPP_ */
