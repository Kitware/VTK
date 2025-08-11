## Add CMake Option for HDF5 Thread Safety

You can now enable HDF5 thread safety in the build by setting the new `VTK_USE_HDF5_THREAD_SAFETY` CMake option. This allows VTK to use HDF5 in multi-threaded environments, allowing you to load multiple HDF5 files concurrently.
