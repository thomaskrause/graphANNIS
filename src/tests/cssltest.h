/*
   Copyright 2017 Thomas Krause <thomaskrause@posteo.de>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#pragma once

#include "gtest/gtest.h"

#include <annis/db.h>
#include <annis/query.h>
#include <annis/graphstorage/csslprepostorderstorage.h>
#include <annis/annosearch/exactannovaluesearch.h>

#include <memory>

extern "C" {
  #include <cssl/skiplist.h>
}

using namespace annis;

class CSSLTest : public ::testing::Test {
protected:
  DB db;
  nodeid_t startNode;

  CSSLPrePostOrderStorage storageCSSL;

  CSSLTest() {
  }

  virtual ~CSSLTest() {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  virtual void SetUp() {
    char* testDataEnv = std::getenv("ANNIS4_TEST_DATA");
    std::string dataDir("data");
    if (testDataEnv != NULL) {
      dataDir = testDataEnv;
    }
    bool loadedDB = db.load(dataDir + "/GUM", true);
    EXPECT_EQ(true, loadedDB);

    std::shared_ptr<const ReadableGraphStorage> orig =
        db.edges.getGraphStorage(ComponentType::DOMINANCE, "const", "");
    ASSERT_TRUE((bool) orig);
    storageCSSL.copy(db, *orig);

    Query q(db);
    q.addNode(std::make_shared<ExactAnnoValueSearch>(db, "cat", "ROOT"));
    if(q.next())
    {
      startNode = q.getCurrent()[0].node;
    }

  }

  virtual void TearDown() {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }

  // Objects declared here can be used by all tests in the test case for Foo.
};

TEST_F(CSSLTest, FindConnectedSimple) {

  SkipList* testList = createSkipList(3, 5);

  std::vector<uint32_t> keys;

  for(uint32_t i=0; i < 100; i++) {
    keys.push_back(i);
    insertElement(testList, i);
  }

  // execute a test search
  RangeSearchResult result = searchRange(testList, 50, 55);

  ASSERT_EQ(5, result.count);

  ASSERT_GE(result.startIdx, 0);
  ASSERT_GE(result.endIdx, 0);
  ASSERT_LT(result.startIdx, keys.size());
  ASSERT_LT(result.endIdx, keys.size());

  EXPECT_EQ(50, keys[result.startIdx]);
  EXPECT_EQ(55, keys[result.endIdx]);

}

TEST_F(CSSLTest, FindConnected) {

  std::unique_ptr<EdgeIterator> it = storageCSSL.findConnected(startNode, 1, 1000);
  unsigned int counter = 0;
  while(it->next().first)
  {
    counter++;
  }
  EXPECT_EQ(19u, counter);
}
