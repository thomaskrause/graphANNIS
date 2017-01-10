#pragma once

#include <annis/types.h>
#include <annis/iterators.h>

#include <annis/util/sharedqueue.h>
#include <annis/util/threadpool.h>

#include <boost/lockfree/queue.hpp>
#include <thread>
#include <mutex>
#include <atomic>

#include <list>
#include <vector>

namespace annis
{

class Operator;


class ThreadNestedLoop : public Iterator
{
public:
  struct MatchPair
  {
    Match lhs;
    Match rhs;
  };


public:
  ThreadNestedLoop(
            std::shared_ptr<Operator> op,
            std::shared_ptr<Iterator> lhs, std::shared_ptr<Iterator> rhs,
            size_t lhsIdx, size_t rhsIdx,
            bool leftIsOuter,
            size_t numOfTasks,
            std::shared_ptr<ThreadPool> threadPool = std::shared_ptr<ThreadPool>());

  virtual bool next(std::vector<Match>& tuple) override;
  virtual void reset() override;

  virtual ~ThreadNestedLoop();
private:

  std::shared_ptr<Operator> op;

  std::shared_ptr<Iterator> outer;
  bool firstOuterFinished;

  std::shared_ptr<Iterator> inner;
  std::deque<std::vector<Match>> innerCache;
  std::deque<std::vector<Match>>::const_iterator itInnerCache;


  const bool leftIsOuter;

  std::atomic_bool runBackgroundThreads;
  size_t activeBackgroundTasks;
  std::mutex mutex_activeBackgroundTasks;

  const size_t numOfTasks;
  std::shared_ptr<ThreadPool> threadPool;

  std::unique_ptr<SharedQueue<std::vector<Match>>> results;
  std::function<void()> fetchLoop;

  std::deque<std::future<void>> taskList;

  std::mutex mutex_fetch;
  bool initialized;
  std::vector<Match> currentOuter;

private:

  bool nextTuple(std::vector<Match>& matchOuter, std::vector<Match>& matchInner);

  bool fetchNextInner(std::vector<Match>& matchInner)
  {
    if(firstOuterFinished)
    {
      if(itInnerCache != innerCache.end())
      {
        matchInner = *itInnerCache;
        itInnerCache++;
        return true;
      }
      else
      {
        return false;
      }
    }
    else
    {
      bool hasNext = inner->next(matchInner);
      if(hasNext)
      {
        innerCache.push_back(matchInner);
      }
      return hasNext;
    }
  }



};
}
