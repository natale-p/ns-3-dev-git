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

#ifdef HAVE_STLAB
#include <stlab/concurrency/future.hpp>
#include <stlab/concurrency/utility.hpp>
#endif

/**
 * \file
 * \ingroup events
 * ns3::MakeEvent function template.
 */

namespace ns3 {

#ifdef HAVE_STLAB
/**
 * \brief Check (at compile time) if T is a stlab::future<void>
 * \tparam T Type to check
 */
template<typename T>
using IsFuture = std::is_convertible<T, stlab::future<void>>;

/**
 * If function returns a stlab::future<void>, then directly run it.
 */
template<typename F, std::enable_if_t<IsFuture<std::result_of_t<F()>>::value>* = nullptr>
void DoInvokeSelect(F& function)
{
  stlab::future<void> ret = function();
  blocking_get (ret);
}

/**
 * If function returns anything else than a stlab::future<void>, then
 * encapsulate it inside a stlab::future<void> that runs now.
 */
template<typename F, std::enable_if_t<!IsFuture<std::result_of_t<F()>>::value>* = nullptr>
void DoInvokeSelect(F& function)
{
  function();
}
#endif // HAVE_STLAB

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
#ifdef HAVE_STLAB
      DoInvokeSelect (m_function);
#else
      m_function ();
#endif
    }
#ifdef HAVE_STLAB
    using result = decltype(std::bind(std::declval<Ts>()...)());

    std::conditional_t<IsFuture<result>::value,
        std::function<stlab::future<void>()>,
        std::function<void()>
    > m_function;
#else
    std::function<void ()> m_function;
#endif
  };
  return new EventMemberImpl (std::forward<Ts> (args)...);
}

} // namespace ns3

#endif /* MAKE_EVENT_H */
