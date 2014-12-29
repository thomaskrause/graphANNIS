#ifndef SEARCHTESTTIGER_H
#define SEARCHTESTTIGER_H

#include "gtest/gtest.h"
#include "db.h"
#include "helper.h"
#include "query.h"
#include "operators/defaultjoins.h"
#include "operators/precedence.h"
#include "annotationsearch.h"
#include "operators/wrapper.h"

#include <vector>

#include <humblelogging/api.h>

using namespace annis;

class SearchTestTiger : public ::testing::Test {
 protected:
  DB db;
  bool loaded;
  SearchTestTiger() {

  }

  virtual ~SearchTestTiger() {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  virtual void SetUp() {

    char* testDataEnv = std::getenv("ANNIS4_TEST_DATA");
    std::string dataDir("data");
    if(testDataEnv != NULL)
    {
      dataDir = testDataEnv;
    }
    bool loadedDB = db.load(dataDir + "/tiger2");
    EXPECT_EQ(true, loadedDB);
  }

  virtual void TearDown() {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }

  // Objects declared here can be used by all tests in the test case for Foo.
};

TEST_F(SearchTestTiger, CatSearch) {

  AnnotationNameSearch search(db, "cat");
  unsigned int counter=0;
  while(search.hasNext())
  {
    Match m = search.next();
    ASSERT_STREQ("cat", db.strings.str(m.anno.name).c_str());
    ASSERT_STREQ("tiger", db.strings.str(m.anno.ns).c_str());
    counter++;
  }

  EXPECT_EQ(373436u, counter);
}

// Should test query
// pos="NN" .2,10 pos="ART"
TEST_F(SearchTestTiger, TokenPrecedence) {

  unsigned int counter=0;

  Query q(db);
  q.addNode(std::make_shared<AnnotationNameSearch>(db, "tiger", "pos", "NN"));
  q.addNode(std::make_shared<AnnotationNameSearch>(db, "tiger", "pos", "ART"));

  q.addOperator(std::make_shared<Precedence>(db, 2, 10), 0, 1);
  while(q.hasNext())
  {
    q.next();
    counter++;
  }

  EXPECT_EQ(179024u, counter);
}

// Should test query
// pos="NN" .2,10 pos="ART" . pos="NN"
TEST_F(SearchTestTiger, TokenPrecedenceThreeNodes) {

  unsigned int counter=0;

  Query q(db);
  q.addNode(std::make_shared<AnnotationNameSearch>(db, "tiger", "pos", "NN"));
  q.addNode(std::make_shared<AnnotationNameSearch>(db, "tiger", "pos", "ART"));
  q.addNode(std::make_shared<AnnotationNameSearch>(db, "tiger", "pos", "NN"));

  q.addOperator(std::make_shared<Precedence>(db, 2, 10), 0, 1);
  q.addOperator(std::make_shared<Precedence>(db), 1, 2);

  while(q.hasNext())
  {
    q.next();
    counter++;
  }

  EXPECT_EQ(114042u, counter);
}

// cat="S" & tok="Bilharziose" & #1 >* #2
TEST_F(SearchTestTiger, BilharzioseSentence)
{
  std::shared_ptr<AnnoIt> n1(std::make_shared<AnnotationNameSearch>(db, "tiger", "cat", "S"));
  std::shared_ptr<AnnoIt> n2(std::make_shared<AnnotationNameSearch>(db, annis_ns, annis_tok, "Bilharziose"));

  unsigned int counter=0;

  const EdgeDB* edbDom = db.getEdgeDB(ComponentType::DOMINANCE, "tiger", "");
  LegacyNestedLoopJoin n1Dom2(edbDom, n1, n2, 1, uintmax);

  for(BinaryMatch m=n1Dom2.next(); m.found; m=n1Dom2.next())
  {
     HL_INFO(logger, (boost::format("Match %1%\t%2%\t%3%\t%4%#%5%\t%6%#%7%")
                      % counter % m.lhs.node % m.rhs.node
                      % db.getNodeDocument(m.lhs.node) % db.getNodeName(m.lhs.node)
                      % db.getNodeDocument(m.rhs.node) % db.getNodeName(m.rhs.node)).str()) ;
    counter++;
  }

  EXPECT_EQ(21u, counter);
}



#endif // SEARCHTESTTIGER_H
