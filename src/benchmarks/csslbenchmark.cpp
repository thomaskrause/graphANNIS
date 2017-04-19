#include <celero/Celero.h>

#include <annis/graphstorage/csslprepostorderstorage.h>
#include <cssl/skiplist.h>

#include <cstdlib>

#include <set>
#include <google/btree_set.h>

CELERO_MAIN

class BaseFixture : public celero::TestFixture
{
public:

  BaseFixture(bool dense)
    : dense(dense)
  {

  }

  virtual std::vector<std::pair<int64_t, uint64_t>> getExperimentValues() const override
  {
      std::vector<std::pair<int64_t, uint64_t>> problemSpace;

      problemSpace.push_back({16000, 0});
      problemSpace.push_back({160000, 0});
      problemSpace.push_back({1600000, 0});
//      problemSpace.push_back({16000000, 0});

      return problemSpace;
  }

  virtual void setUp(int64_t experimentValue) override
  {

    srand(0);

    if(dense)
    {
      for(uint32_t i = 0; i < experimentValue; i++)
      {
        keys.push_back(i+1);
      }
    }
    else
    {
      for(uint32_t i = 0; i < experimentValue; i++)
      {
        keys.push_back((rand() % (INT_MAX/2-1)) + 1);
      }
      std::sort(keys.begin(), keys.end());
    }


    uint32_t m = 1000000;
    r_size = keys.size() / 100;

    rkeys.reserve(m);
    for(int i = 0; i < m; i++)
    {
      rkeys.push_back(keys[rand() % keys.size()]);
    }
  }

  const bool dense;

  std::vector<uint32_t> keys;
  std::vector<uint32_t> rkeys;
  uint32_t r_size;

};

class STLFixture : public BaseFixture
{
public:
  STLFixture(bool dense) : BaseFixture(dense)
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

class BTreeFixture : public BaseFixture
{
public:
  BTreeFixture(bool dense) : BaseFixture(dense)
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

  btree::btree_multiset<uint32_t> container;

};

class CSSLFixture : public BaseFixture
{
public:
  CSSLFixture(bool dense)
    : BaseFixture(dense), slist(nullptr)
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

  virtual ~CSSLFixture()
  {
    annis::CSSLPrePostOrderStorage::freeSkiplist(slist);
  }

  SkipList* slist;
};


class CSSLDenseFixture : public CSSLFixture
{
public:
  CSSLDenseFixture() : CSSLFixture(true) {}
};

class BTreeDenseFixture : public BTreeFixture
{
public:
  BTreeDenseFixture() : BTreeFixture(true) {}
};

class STLDenseFixture : public STLFixture
{
public:
  STLDenseFixture() : STLFixture(true) {}
};

class CSSLSparseFixture : public CSSLFixture
{
public:
  CSSLSparseFixture() : CSSLFixture(false) {}
};

class BTreeSpareFixture : public BTreeFixture
{
public:
  BTreeSpareFixture() : BTreeFixture(false) {}
};

class STLSparseFixture : public STLFixture
{
public:
  STLSparseFixture() : STLFixture(false) {}
};

BASELINE_F(DenseRangeQuery, STL, STLDenseFixture, 10, 1)
{
  for (auto testKey : rkeys) {
    auto start = container.lower_bound(testKey);
    auto end = container.upper_bound(testKey + r_size);
    end--;
    assert(*start >= testKey && *end <= (testKey + r_size));
  }
}

BASELINE_F(SparseRangeQuery, STL, STLSparseFixture, 10, 1)
{
  for (auto testKey : rkeys) {
    auto start = container.lower_bound(testKey);
    auto end = container.upper_bound(testKey + r_size);
    end--;
    assert(*start >= testKey && *end <= (testKey + r_size));
  }
}

BENCHMARK_F(DenseRangeQuery, BTree, BTreeDenseFixture, 10, 1)
{
  for (auto testKey : rkeys)
  {
    auto start = container.lower_bound(testKey);
    auto end = container.upper_bound(testKey + r_size);
    end--;
    assert(*start >= testKey && *end <= (testKey + r_size));
  }
}

BENCHMARK_F(DenseRangeQuery, CSSL, CSSLDenseFixture, 10, 1)
{
  for (auto testKey : rkeys) {
    RangeSearchResult res = searchRange(slist, testKey, (testKey + r_size));
    assert(res.start->key >= testKey && res.end->key <= (testKey + r_size));
  }
}

BENCHMARK_F(SparseRangeQuery, BTree, BTreeSpareFixture, 10, 1)
{
  for (auto testKey : rkeys)
  {
    auto start = container.lower_bound(testKey);
    auto end = container.upper_bound(testKey + r_size);
    end--;
    assert(*start >= testKey && *end <= (testKey + r_size));
  }
}

BENCHMARK_F(SparseRangeQuery, CSSL, CSSLSparseFixture, 10, 1)
{
  for (auto testKey : rkeys) {
    RangeSearchResult res = searchRange(slist, testKey, (testKey + r_size));
    assert(res.start->key >= testKey && res.end->key <= (testKey + r_size));
  }
}
