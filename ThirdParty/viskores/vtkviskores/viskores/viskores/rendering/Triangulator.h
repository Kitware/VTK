//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/rendering/raytracing/Triangulator.h>

#include <viskores/Deprecated.h>


struct VISKORES_DEPRECATED(2.2,
                           "viskores/rendering/Triangulator.h header no longer supported. Use "
                           "viskores/rendering/raytracing/Triangulator.h.")
  viskores_deprecated_Triangulator_h_warning
{
};

static viskores_deprecated_Triangulator_h_warning viskores_give_Triangulator_h_warning;

namespace viskores
{
namespace rendering
{

class VISKORES_DEPRECATED(
  2.2,
  "viskores::rendering::Triangulator moved to viskores::rendering::raytracing::Triangulator.")
  Triangulator : public viskores::rendering::raytracing::Triangulator
{
};

}
} // viskores::rendering
