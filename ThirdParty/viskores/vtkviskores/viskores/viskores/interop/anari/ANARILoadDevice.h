//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_interop_anari_ANARILoadDevice_h
#define viskores_interop_anari_ANARILoadDevice_h

#include <viskores/interop/anari/ViskoresANARITypes.h>
#include <viskores/interop/anari/viskores_anari_export.h>

namespace viskores
{
namespace interop
{
namespace anari
{

/// @brief Loads an ANARI device from the library of the provided name.
///
/// Given the name of an ANARI device library, attempts to find the associated
/// library (named `anari_library_` plus the provided `libraryName` with the
/// standard shared object library prefix and extension for the current system),
/// opens it, and returns an ANARI device object. This is a convenience function
/// that essentially uses `anari::loadLibrary` to open the library and
/// `anari::newDevice` to create the device.
///
/// An additional feature of this method is that if you provide the string
/// `viskores` for `libraryName`, this function will be able to load the device
/// without having to find the device on the system. Typically, to find the
/// library you need an environment variable such as `LD_LIBRARY_PATH` or an
/// internal library rpath pointing to the directory containing the library. For
/// the "viskores" library, the device will be loaded directly regardless of its
/// location.
///
/// This method will also link the ANARI device's logging to the Viskores logging
/// so the ANARI logging can be controlled through Viskores' configuration.
///
/// @param libraryName The name of the library to load.
/// @return An open ANARI device or `nullptr` if the library cannot be opened.
VISKORES_ANARI_EXPORT VISKORES_CONT anari_cpp::Device ANARILoadDevice(
  const std::string& libraryName);

}
}
} // namespace viskores::interop::anari


#endif //viskores_interop_anari_ANARILoadDevice_h
