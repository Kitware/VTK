//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
#include "token/json/DeserializationContext.h"

#include "token/json/jsonManager.h"

token_BEGIN_NAMESPACE

DeserializationContext::DeserializationContext(
  std::shared_ptr<token_NAMESPACE::Manager> m,
  const nlohmann::json& j)
  : m_manager(m)
{
  if (m_manager)
  {
    ++m_manager->m_translationDepth;
  }
  from_json(j, m_manager);
}

DeserializationContext::~DeserializationContext()
{
  if (m_manager)
  {
    if ((--m_manager->m_translationDepth) == 0)
    {
      m_manager->m_translation.clear();
    }
  }
}

token_CLOSE_NAMESPACE
