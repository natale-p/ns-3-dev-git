/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef MAKE_EVENT_H
#define MAKE_EVENT_H

#include "event-impl.h"
#include <functional>

/**
 * \file
 * \ingroup events
 * ns3::MakeEvent function template.
 */

namespace ns3 {

/**
 * \ingroup events
 * Create EventImpl instance from a callable object.
 *
 * \tparam Ts \deduced Argument types.
 * \param [in] args Callable object and bound arguments, forwarded to std::bind.
 * \returns The constructed EventImpl.
 */
template <typename... Ts>
EventImpl * MakeEvent (Ts&&... args)
{
  class EventMemberImpl : public EventImpl
  {
public:
    EventMemberImpl (Ts&&... args)
      : m_function (std::bind (std::forward<Ts> (args)...))
    {
    }
protected:
    virtual ~EventMemberImpl ()
    {
    }
private:
    virtual void Notify ()
    {
      m_function ();
    }
    std::function<void ()> m_function;
  };
  return new EventMemberImpl (std::forward<Ts> (args)...);
}

} // namespace ns3

#endif /* MAKE_EVENT_H */
