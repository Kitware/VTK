# Put all files into a single directory.
mv -v build/dist/*.whl release/
mv -v build/*-wheel-sdk-*.tar.xz release/
ls release/

# Print the hashes of everything to be uploaded.
sha256sum release/*

# Compute the version number for the upload.
major="$( sed -ne '/set.VTK_MAJOR_VERSION/s/.* \([0-9]*\))/\1/p' CMake/vtkVersion.cmake )"
readonly major
minor="$( sed -ne '/set.VTK_MINOR_VERSION/s/.* \([0-9]*\))/\1/p' CMake/vtkVersion.cmake )"
readonly minor

# Tell the rest of the job about the destination.
RSYNC_DESTINATION="vtk_release/$major.$minor/"
export RSYNC_DESTINATION
