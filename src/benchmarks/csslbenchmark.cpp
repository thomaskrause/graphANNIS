#include <celero/Celero.h>

#include <annis/graphstorage/csslprepostorderstorage.h>
#include <cssl/skiplist.h>
#include <cstdlib>

#include <set>

CELERO_MAIN

class BaseFixture : public celero::TestFixture
{
public:
  virtual std::vector<std::pair<int64_t, uint64_t>> getExperimentValues() const override
  {
      std::vector<std::pair<int64_t, uint64_t>> problemSpace;

      problemSpace.push_back({160000, 0});

      return problemSpace;
  }

  virtual void setUp(int64_t experimentValue) override
  {
    for(uint32_t i = 0; i < experimentValue; i++)
    {
      keys.push_back(i+1);
    }

    srand(0);

    uint32_t m = 1000000;
    r_size = keys.size() / 10;

    rkeys.reserve(m);
    for(int i = 0; i < m; i++)
    {
      rkeys.push_back(keys[rand() % keys.size()]);
    }
  }

  std::vector<uint32_t> keys;
  std::vector<uint32_t> rkeys;
  uint32_t r_size;

};

class STLDenseFixture : public BaseFixture
{
public:
  STLDenseFixture()
  {
  }

  /// Before each run, build a vector of random integers.
  virtual void setUp(int64_t experimentValue) override
  {
    BaseFixture::setUp(experimentValue);

    for (auto o : keys)
    {
      container.insert(o);
    }
  }

  std::multiset<uint32_t> container;

};

class CSSLDenseFixture : public BaseFixture
{
public:
  CSSLDenseFixture()
    : slist(nullptr)
  {
  }

  /// Before each run, build a vector of random integers.
  virtual void setUp(int64_t experimentValue) override
  {
    BaseFixture::setUp(experimentValue);

    annis::CSSLPrePostOrderStorage::freeSkiplist(slist);

    slist = createSkipList(9, 5);
    for (auto o : keys)
    {
      insertElement(slist, o);
    }


  }

  virtual ~CSSLDenseFixture()
  {
    annis::CSSLPrePostOrderStorage::freeSkiplist(slist);
  }

  SkipList* slist;


};

BASELINE_F(DenseRangeQuery, STL, STLDenseFixture, 30, 1)
{
  for (auto testKey : rkeys) {
    auto start = container.lower_bound(testKey);
    auto end = container.upper_bound(testKey + r_size);
    end--;
    assert(*start >= testKey && *end <= (testKey + r_size));
  }
}

BENCHMARK_F(DenseRangeQuery, CSSL, CSSLDenseFixture, 30, 1)
{
  for (auto testKey : rkeys) {
    RangeSearchResult res = searchRange(slist, testKey, (testKey + r_size));
    assert(res.start->key >= testKey && res.end->key <= (testKey + r_size));
  }
}
