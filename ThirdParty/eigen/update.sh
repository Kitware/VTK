#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="eigen"
readonly ownership="Eigen Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/eigen.git"
readonly tag="for/vtk-20190613.1-3.3.7"
readonly paths="
Eigen/Cholesky
Eigen/CholmodSupport
Eigen/CMakeLists.txt
Eigen/Core
Eigen/Dense
Eigen/Eigen
Eigen/Eigenvalues
Eigen/Geometry
Eigen/Householder
Eigen/IterativeLinearSolvers
Eigen/Jacobi
Eigen/LU
Eigen/MetisSupport
Eigen/OrderingMethods
Eigen/PardisoSupport
Eigen/PaStiXSupport
Eigen/QR
Eigen/QtAlignedMalloc
Eigen/Sparse
Eigen/SparseCholesky
Eigen/SparseCore
Eigen/SparseLU
Eigen/SparseQR
Eigen/SPQRSupport
Eigen/StdDeque
Eigen/StdList
Eigen/StdVector
Eigen/SuperLUSupport
Eigen/SVD
Eigen/UmfPackSupport
Eigen/src

COPYING.BSD
COPYING.MINPACK
COPYING.MPL2
COPYING.README
README.md
README.kitware.md

CMakeLists.txt
cmake/FindStandardMathLibrary.cmake

.gitattributes
"

extract_source () {
    git_archive
    pushd "$extractdir"
    mv "$name-reduced/Eigen" "$name-reduced/eigen"
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
