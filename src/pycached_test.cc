#include <gtest/gtest.h>
#include "pycached_impl.cc"

#define init\
  std::vector<std::string> host;\
  host.push_back("182.48.52.238:11211");

TEST(construct, test){
  init;
  Client cl(host);
}

TEST(construct, fail){
  std::vector<std::string> host;
  host.push_back("121.0.0.1:11211");

  ASSERT_THROW(new Client(host), SocketException);
}

TEST(operation, set){
  init;
  Client cl(host);
  cl.set("hoge", "fuge");
}

TEST(operation, nullstring){
  init;
  Client cl(host);
  std::string value("a");
  value[0] = '\0';
  cl.set("hoge", value);
}

TEST(operation, add){
  init;
  Client cl(host);
  int result = cl.add("hoge","auu");
  EXPECT_EQ(result, 0);
}

typedef std::string raw_data;
#define EXPECT_BUFFER_EQ(a,b) EXPECT_TRUE(memcmp(a.data(), b, a.size()) == 0)
TEST(operation, get){
  init;
  Client cl(host);
  cl.set("hoge", "fuge");
  std::string result(cl.get("hoge"));
  EXPECT_BUFFER_EQ(result, "fuge");
}
TEST(operation, get_fail){
  init;
  Client cl(host);
  std::string result(cl.get("o3i24"));
  EXPECT_EQ(result.size(), 0U);
}


TEST(operation, gets){
  init;
  Client cl(host);
  std::string result(cl.gets("hoge"));
  EXPECT_BUFFER_EQ(result, "fuge");
}

TEST(operation, gets_a){
  init;
  Client cl(host);
  cl.set("aaaa", "b");
  std::string result(cl.gets("aaaa"));
  EXPECT_BUFFER_EQ(result, "b");
}

TEST(operation, cas){
  init;
  Client cl(host);
  cl.set("casing", "0");
  EXPECT_BUFFER_EQ(cl.gets("casing"), "0");
  EXPECT_EQ(cl.cas("casing", "1"), true);
  EXPECT_BUFFER_EQ(cl.get("casing"), "1");
}
TEST(string_to_int, test){
#define CHECK(HOGE)\
  EXPECT_EQ(Client::string_to_int( #HOGE ), HOGE ## U)
  CHECK(3242);
#undef CHECK
}
