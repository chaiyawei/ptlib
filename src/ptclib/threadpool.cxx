/*
 * threadpool.cxx
 *
 * Generalised Thead Pool functions
 *
 * Portable Windows Library
 *
 * Copyright (C) 2009 Post Increment
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Portions of this code were written with the financial assistance of 
 * Metreos Corporation (http://www.metros.com).
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifdef __GNUC__
#pragma implementation "threadpool.h"
#endif


#include <ptlib.h>
#include <ptclib/threadpool.h>

#define new PNEW


PThreadPoolBase::PThreadPoolBase(unsigned int maxWorkerCount, unsigned int maxWorkUnitCount)
  : m_maxWorkerCount(maxWorkerCount), m_maxWorkUnitCount(maxWorkUnitCount)
{
}

PThreadPoolBase::~PThreadPoolBase()
{
  for (;;) {
    PWaitAndSignal mutex(listMutex);
    if (workers.size() == 0)
      break;

    PThreadPoolWorkerBase * worker = workers[0];
    worker->Shutdown();
    workers.erase(workers.begin());
    StopWorker(worker);
  }
}


PThreadPoolWorkerBase * PThreadPoolBase::AllocateWorker()
{
  // find the worker thread with the minimum number of work units
  // shortcut the search if we find an empty one
  WorkerList_t::iterator minWorker = workers.end();
  size_t minSizeFound = 0x7ffff;
  WorkerList_t::iterator iter;
  for (iter = workers.begin(); iter != workers.end(); ++iter) {
    PThreadPoolWorkerBase & worker = **iter;
    PWaitAndSignal m2(worker.workerMutex);
    if (!worker.shutdown && (worker.GetWorkSize() <= minSizeFound)) {
      minSizeFound = worker.GetWorkSize();
      minWorker = iter;
      if (minSizeFound == 0)
        break;
    }
  }

  // if there is an idle worker, use it
  if (iter != workers.end())
    return *minWorker;

  // if there is a per-worker limit, increase workers in quanta of the max worker count
  // otherwise only allow maximum number of workers
  if (m_maxWorkUnitCount > 0) {
    if (((workers.size() % m_maxWorkerCount) == 0) && (minSizeFound < m_maxWorkUnitCount)) 
      return *minWorker;
  }
  else if ((workers.size() > 0) && (workers.size() <= m_maxWorkerCount))
    return *minWorker;

  // create a new worker thread
  PThreadPoolWorkerBase * worker = CreateWorkerThread();
  worker->Resume();
  workers.push_back(worker);

  return worker;
}


bool PThreadPoolBase::CheckWorker(PThreadPoolWorkerBase * worker)
{
  {
    PWaitAndSignal mutex(listMutex);

    // find worker in list
    WorkerList_t::iterator iter;
    for (iter = workers.begin(); iter != workers.end(); ++iter) {
      if (*iter == worker)
        break;
    }
    if (iter == workers.end())
      return false;

    // if the worker thread has enough work to keep running, leave it alone
    if (worker->GetWorkSize() > 0) 
      return true;

    // but don't shut down the last thread, so we don't have the overhead of starting it up again
    if (workers.size() == 1)
      return true;

    worker->Shutdown();
    workers.erase(iter);
  }

  StopWorker(worker);

  return true;
}


void PThreadPoolBase::StopWorker(PThreadPoolWorkerBase * worker)
{
  // the worker is now finished
  if (!worker->WaitForTermination(10000)) {
    PTRACE(4, "ThreadPool\tWorker did not terminate promptly");
  }
  PTRACE(4, "ThreadPool\tDestroying pool thread");
  delete worker;
}


////////////////////////////////////////////////////////////////

PThreadPoolWorkerBase::PThreadPoolWorkerBase(PThreadPoolBase & _pool)
  : PThread(100, NoAutoDeleteThread, NormalPriority, "Pool")
  , pool(_pool)
  , shutdown(PFalse)
{
}

