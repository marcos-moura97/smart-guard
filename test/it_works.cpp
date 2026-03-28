#include "CppUTest/TestHarness.h"

TEST_GROUP(FirstTestGroup)
{
};

TEST(FirstTestGroup, FirstTest)
{
   LONGS_EQUAL(2, 1+1);
}

TEST(FirstTestGroup, FirstFail)
{
   FAIL("Fail me!");
}