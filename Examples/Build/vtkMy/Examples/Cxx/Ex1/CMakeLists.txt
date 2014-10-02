#
# Add the executable
#

add_executable(vtkmyEx1 vtkmyEx1.cxx)
target_link_libraries(vtkmyEx1 vtkmyUnsorted vtkmyCommon vtkmyImaging)
