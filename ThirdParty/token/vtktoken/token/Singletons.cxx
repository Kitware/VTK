#include "token/Singletons.h"

token_BEGIN_NAMESPACE
namespace {

TypeContainer s_singletons;

} // anonymous namespace

TypeContainer& singletons()
{
  return s_singletons;
}

token_CLOSE_NAMESPACE
