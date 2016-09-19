#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="xdmf3"
readonly ownership="XDMF Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="git://xdmf.org/Xdmf.git"
readonly tag="master"
readonly paths="
CMake
CMakeLists.txt
CTestConfig.cmake
Copyright.txt
Xdmf.dtd
Xdmf.hpp
XdmfAggregate.cpp
XdmfAggregate.hpp
XdmfAttribute.cpp
XdmfAttribute.hpp
XdmfAttributeCenter.cpp
XdmfAttributeCenter.hpp
XdmfAttributeType.cpp
XdmfAttributeType.hpp
XdmfConfig.cmake.in
XdmfCurvilinearGrid.cpp
XdmfCurvilinearGrid.hpp
XdmfDomain.cpp
XdmfDomain.hpp
XdmfGeometry.cpp
XdmfGeometry.hpp
XdmfGeometryType.cpp
XdmfGeometryType.hpp
XdmfGraph.cpp
XdmfGraph.hpp
XdmfGrid.cpp
XdmfGrid.hpp
XdmfGridTemplate.cpp
XdmfGridTemplate.hpp
XdmfGridCollection.cpp
XdmfGridCollection.hpp
XdmfGridCollectionType.cpp
XdmfGridCollectionType.hpp
XdmfGridController.cpp
XdmfGridController.hpp
XdmfItemFactory.cpp
XdmfItemFactory.hpp
XdmfMap.cpp
XdmfMap.hpp
XdmfReader.cpp
XdmfReader.hpp
XdmfRectilinearGrid.cpp
XdmfRectilinearGrid.hpp
XdmfRegularGrid.cpp
XdmfRegularGrid.hpp
XdmfSet.cpp
XdmfSet.hpp
XdmfSetType.cpp
XdmfSetType.hpp
XdmfTemplate.cpp
XdmfTemplate.hpp
XdmfTime.cpp
XdmfTime.hpp
XdmfTopology.cpp
XdmfTopology.hpp
XdmfTopologyType.cpp
XdmfTopologyType.hpp
XdmfUnstructuredGrid.cpp
XdmfUnstructuredGrid.hpp
core/CMakeLists.txt
core/XdmfArray.cpp
core/XdmfArray.hpp
core/XdmfArray.tpp
core/XdmfArrayReference.cpp
core/XdmfArrayReference.hpp
core/XdmfArrayType.cpp
core/XdmfArrayType.hpp
core/XdmfBinaryController.cpp
core/XdmfBinaryController.hpp
core/XdmfConfig.hpp.in
core/XdmfCore.hpp
core/XdmfCoreConfig.hpp.in
core/XdmfCoreItemFactory.cpp
core/XdmfCoreItemFactory.hpp
core/XdmfCoreReader.cpp
core/XdmfCoreReader.hpp
core/XdmfError.cpp
core/XdmfError.hpp
core/XdmfFunction.cpp
core/XdmfFunction.hpp
core/XdmfHDF5Controller.cpp
core/XdmfHDF5Controller.hpp
core/XdmfHDF5Writer.cpp
core/XdmfHDF5Writer.hpp
core/XdmfHeavyDataController.cpp
core/XdmfHeavyDataController.hpp
core/XdmfHeavyDataDescription.cpp
core/XdmfHeavyDataDescription.hpp
core/XdmfHeavyDataWriter.cpp
core/XdmfHeavyDataWriter.hpp
core/XdmfInformation.cpp
core/XdmfInformation.hpp
core/XdmfItem.cpp
core/XdmfItem.hpp
core/XdmfItemProperty.cpp
core/XdmfItemProperty.hpp
core/XdmfPlaceholder.cpp
core/XdmfPlaceholder.hpp
core/XdmfSharedPtr.hpp
core/XdmfSparseMatrix.cpp
core/XdmfSparseMatrix.hpp
core/XdmfSubset.cpp
core/XdmfSubset.hpp
core/XdmfSystemUtils.cpp
core/XdmfSystemUtils.hpp
core/XdmfTIFFController.cpp
core/XdmfTIFFController.hpp
core/XdmfVisitor.cpp
core/XdmfVisitor.hpp
core/XdmfWriter.cpp
core/XdmfWriter.hpp
core/loki
core/dsm/CMakeLists.txt
core/dsm/XdmfDSM.hpp
core/dsm/XdmfDSMBuffer.cpp
core/dsm/XdmfDSMBuffer.hpp
core/dsm/XdmfDSMCommMPI.cpp
core/dsm/XdmfDSMCommMPI.hpp
core/dsm/XdmfDSMDescription.cpp
core/dsm/XdmfDSMDescription.hpp
core/dsm/XdmfDSMDriver.cpp
core/dsm/XdmfDSMDriver.hpp
core/dsm/XdmfDSMItemFactory.cpp
core/dsm/XdmfDSMItemFactory.hpp
core/dsm/XdmfHDF5ControllerDSM.cpp
core/dsm/XdmfHDF5ControllerDSM.hpp
core/dsm/XdmfHDF5WriterDSM.cpp
core/dsm/XdmfHDF5WriterDSM.hpp
"

extract_source () {
    git archive --worktree-attributes --prefix="$name-reduced/" HEAD -- $paths | \
        tar -C "$extractdir" -x
    #prevent VTK from trying to enforce its style rules
    echo "*        -whitespace" > "$extractdir/.gitattributes"
}

. "${BASH_SOURCE%/*}/../update-common.sh"
