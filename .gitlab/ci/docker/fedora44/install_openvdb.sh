#!/bin/sh

set -e

readonly openvdb_version="11.0.0"
readonly openvdb_tarball="openvdb-$openvdb_version.tar.gz"
readonly openvdb_sha256sum="6314ff1db057ea90050763e7b7d7ed86d8224fcd42a82cdbb9c515e001b96c74"

readonly openvdb_root="$HOME/openvdb"

readonly openvdb_src="$openvdb_root/src"
readonly openvdb_build="$openvdb_root/build"

mkdir -p "$openvdb_root" \
    "$openvdb_src" "$openvdb_build"
cd "$openvdb_root"

echo "$openvdb_sha256sum  $openvdb_tarball" > openvdb.sha256sum
curl -OL "https://www.paraview.org/files/dependencies/openvdb-11.0.0.tar.gz"
sha256sum --check openvdb.sha256sum

tar -C "$openvdb_src" --strip-components=1 -xf "$openvdb_tarball"

# backport patch from openvdb 13.0.0 to fix build on clang
# source: https://github.com/AcademySoftwareFoundation/openvdb/commit/930c3acb8e0c7c2f1373f3a70dc197f5d04dfe74
cd "$openvdb_src/"
echo "diff --git a/nanovdb/nanovdb/util/GridBuilder.h b/nanovdb/nanovdb/util/GridBuilder.h
index 30385661d0..428215ba65 100644
--- a/nanovdb/nanovdb/util/GridBuilder.h
+++ b/nanovdb/nanovdb/util/GridBuilder.h
@@ -1158,7 +1158,7 @@ struct LeafNode
         ValueIterator& operator=(const ValueIterator&) = default;
         ValueType operator*() const { NANOVDB_ASSERT(*this); return mParent->mValues[mPos];}
         Coord getCoord() const { NANOVDB_ASSERT(*this); return mParent->offsetToGlobalCoord(mPos);}
-        bool isActive() const { NANOVDB_ASSERT(*this); return mParent->isActive(mPos);}
+        bool isActive() const { NANOVDB_ASSERT(*this); return mParent->mValueMask.isOn(mPos);}
         operator bool() const {return mPos < SIZE;}
         ValueIterator& operator++() {++mPos; return *this;}
         ValueIterator operator++(int) {
diff --git a/openvdb/openvdb/tree/NodeManager.h b/openvdb/openvdb/tree/NodeManager.h
index 27a3f82012..1023c00748 100644
--- a/openvdb/openvdb/tree/NodeManager.h
+++ b/openvdb/openvdb/tree/NodeManager.h
@@ -327,7 +327,7 @@ class NodeList
         void operator()(const NodeRange& range) const
         {
             for (typename NodeRange::Iterator it = range.begin(); it; ++it) {
-                OpT::template eval(mNodeOp, it);
+                OpT::eval(mNodeOp, it);
             }
         }
         const NodeOp mNodeOp;
@@ -347,7 +347,7 @@ class NodeList
         void operator()(const NodeRange& range) const
         {
             for (typename NodeRange::Iterator it = range.begin(); it; ++it) {
-                OpT::template eval(mNodeOp, it);
+                OpT::eval(mNodeOp, it);
             }
         }
         const NodeOp& mNodeOp;
@@ -372,7 +372,7 @@ class NodeList
         void operator()(const NodeRange& range)
         {
             for (typename NodeRange::Iterator it = range.begin(); it; ++it) {
-                OpT::template eval(*mNodeOp, it);
+                OpT::eval(*mNodeOp, it);
             }
         }
         void join(const NodeReducer& other)
" | patch -p1

cd "$openvdb_build"

cmake -GNinja "$openvdb_src/" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DCMAKE_INSTALL_LIBDIR=lib64 \
    -DUSE_BLOSC:BOOL=ON \
    -DUSE_ZLIB:BOOL=ON \
    -DUSE_CCACHE:BOOL=OFF \
    -DOPENVDB_BUILD_NANOVDB:BOOL=ON \
    -DUSE_NANOVDB:BOOL=ON \
    -DOPENVDB_CORE_STATIC:BOOL=OFF \
    -DTbb_INCLUDE_DIR:PATH=/usr/include \
    -DTbb_tbb_LIBRARY_RELEASE:PATH=/usr/lib64/libtbb.so \
    -DTbb_tbbmalloc_LIBRARY_RELEASE:PATH=/usr/lib64/libtbbmalloc.so
ninja
cmake --install .

cd
rm -rf "$openvdb_root"
