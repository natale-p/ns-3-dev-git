/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 Natale Patriciello <natale.patriciello@gmail.com>
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
#include "ns3/simulator.h"
#include "ns3/command-line.h"
#include "ns3/global-value.h"
#include "ns3/string.h"

#include <chrono>
#include <unistd.h>
#include <stlab/concurrency/default_executor.hpp>

using namespace ns3;

/* The mutex for the std::cout object */
static std::mutex coutMutex;

/**
 * \brief A class that have some work that can be deferred.
 */
class MyClass
{
public:
  /**
   * \brief MyClass constructor
   */
  MyClass ();

  /**
   * \brief Init the asynchronous job, and then enqueue it for the async run
   */
  stlab::future<void> InitWork();
  /**
   * \brief Do a very long job.
   * \return The important value of 42.
   */
  int DoWork();
};

MyClass::MyClass ()
{
  // Do nothing
}

stlab::future<void>
MyClass::InitWork()
{
  // A member function should be enqueued using std::bind, because to call
  // it we need the "this" pointer as well.
  stlab::future<int> fut = Simulator::AddJob (std::bind(&MyClass::DoWork, this));

  // Here we can advance other work in parallel. DoWork is running somewhere,
  // or (if the number of threads is 0) is holded until "get" is called over fut.

  // Retrieve the value of the future, and then (hopefully) use it.

  return fut.then([](const int &x) {std::cout << "The answer is " << x << std::endl;});
}

int
MyClass::DoWork ()
{
  auto start = std::chrono::high_resolution_clock::now();

  {
    std::lock_guard<std::mutex> lock (coutMutex);
    std::cout << "Starting job at "
              << Simulator::Now().GetSeconds() << std::endl;
  }

  // Here we "emulate" a work that takes 5 seconds.
  sleep (5);

  {
    std::lock_guard<std::mutex> lock (coutMutex);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end-start;

    std::cout << "Ending job at "
              << Simulator::Now().GetSeconds()
              << ", real time passed: " << diff.count() * 1000
              << " ms" << std::endl;
  }

  // Please note how the simulator time stays fixed. Well, at least until
  // you use the DefaultSimulatorImpl...
  return 42;
}

/**
 * \brief Check if a num is prime, and do some wait in the middle
 * \param num number to check for primality
 * \return true if num is prime, false otherwise
 */
static bool IsPrime(uint64_t num)
{
  bool ret = true;
  auto start = std::chrono::high_resolution_clock::now();

  {
    std::lock_guard<std::mutex> lock (coutMutex);
    std::cout << "Starting job on " << num << " at "
              << Simulator::Now().GetSeconds() << std::endl;
  }

  // We are so efficient in calculating primality that we add 5 seconds of
  // sleeping time.
  sleep (5);


  if (num <= 3)
    {
      ret = (num > 1) ? true : false;
    }
  else if (num % 2 == 0 || num % 3 == 0)
    {
      ret = false;
    }
  else
    {
      for (uint64_t i = 5; i * i <= num; i += 6)
        {
          if (num % i == 0 || num % (i + 2) == 0)
            {
              ret = false;
              break;
            }
        }
    }

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end-start;

  {
    std::lock_guard<std::mutex> lock (coutMutex);
    std::cout << "Ended job on " << num << " at "
              << Simulator::Now().GetSeconds()
              << " execution time " << diff.count() * 1000
              << " ms " << std::endl;
  }

  return ret;
}


static void StartWorking ()
{
  auto start = std::chrono::high_resolution_clock::now();

  // Launch three jobs. Depending on the number of threads, these will run
  // sequentially or in parallel.
  stlab::future<bool> first = Simulator::AddJob (IsPrime, 0xA87b83728);
  stlab::future<bool> second = Simulator::AddJob (IsPrime, 0xA87b837AA);
  stlab::future<bool> third = Simulator::AddJob (IsPrime, 0xA87b837BB);

  // we "emulate" other work that we can do in the main thread
  sleep (2);

  // Take the values. If the number of threads is 0, the work will be done
  // in the following.
  // Waiting just for illustrational purpose
  while (!first.get_try()) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
  while (!second.get_try()) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
  while (!third.get_try()) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
  bool a = first.get_try().value();
  bool b = second.get_try().value();
  bool c = third.get_try().value();

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end-start;

  {
    std::lock_guard<std::mutex> lock (coutMutex);
    std::cout << "the result is " << a
              << " " << b << " " << c
              << ", total running time: " << diff.count() * 1000
              << " ms." << std::endl;
  }
}

int main (int argc, char *argv[])
{
  GlobalValue::Bind ("SimulatorImplementationType",
                     StringValue ("ns3::ConcurrentSimulatorImpl"));
  CommandLine cmd;
  cmd.Parse (argc, argv);

  MyClass c;

  Simulator::Schedule (Seconds (10.0), &StartWorking);
  Simulator::Schedule (Seconds (11.0), &MyClass::InitWork, &c);

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
