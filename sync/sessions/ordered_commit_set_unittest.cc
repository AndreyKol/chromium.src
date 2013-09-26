// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sync/sessions/ordered_commit_set.h"
#include "sync/test/engine/test_id_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

using std::vector;

namespace syncer {
namespace sessions {
namespace {

class OrderedCommitSetTest : public testing::Test {
 public:
  OrderedCommitSetTest() {}
 protected:
  TestIdFactory ids_;
};

TEST_F(OrderedCommitSetTest, Insertions) {
  vector<int64> expected;
  for (int64 i = 0; i < 8; i++)
    expected.push_back(i);

  OrderedCommitSet commit_set1, commit_set2;
  commit_set1.AddCommitItem(expected[0], BOOKMARKS);
  commit_set1.AddCommitItem(expected[1], BOOKMARKS);
  commit_set1.AddCommitItem(expected[2], PREFERENCES);
  // Duplicates should be dropped.
  commit_set1.AddCommitItem(expected[2], PREFERENCES);
  commit_set1.AddCommitItem(expected[3], SESSIONS);
  commit_set1.AddCommitItem(expected[4], SESSIONS);
  commit_set2.AddCommitItem(expected[7], AUTOFILL);
  commit_set2.AddCommitItem(expected[6], AUTOFILL);
  commit_set2.AddCommitItem(expected[5], AUTOFILL);
  // Add something in set1 to set2, which should get dropped by AppendReverse.
  commit_set2.AddCommitItem(expected[0], BOOKMARKS);
  commit_set1.AppendReverse(commit_set2);

  EXPECT_EQ(8U, commit_set1.Size());

  // First, we should verify the projections are correct. Second, we want to
  // do the same verification after truncating by 1. Next, try truncating
  // the set to a size of 4, so that the DB projection is wiped out and
  // PASSIVE has one element removed.  Finally, truncate to 1 so only UI is
  // remaining.
  std::vector<size_t> sizes;
  sizes.push_back(8);
  sizes.push_back(7);
  sizes.push_back(4);
  sizes.push_back(1);
  for (std::vector<size_t>::iterator it = sizes.begin();
       it != sizes.end(); ++it) {
    commit_set1.Truncate(*it);
    size_t expected_size = *it;

    SCOPED_TRACE(::testing::Message("Iteration size = ") << *it);
    std::vector<int64> all_ids = commit_set1.GetAllCommitHandles();
    EXPECT_EQ(expected_size, all_ids.size());
    for (size_t i = 0; i < expected_size; i++) {
      EXPECT_TRUE(expected[i] == all_ids[i]);
      EXPECT_TRUE(expected[i] == commit_set1.GetCommitHandleAt(i));
    }
  }
}

TEST_F(OrderedCommitSetTest, HasBookmarkCommitId) {
  OrderedCommitSet commit_set;

  commit_set.AddCommitItem(0, AUTOFILL);
  commit_set.AddCommitItem(1, SESSIONS);
  EXPECT_FALSE(commit_set.HasBookmarkCommitId());

  commit_set.AddCommitItem(2, PREFERENCES);
  commit_set.AddCommitItem(3, PREFERENCES);
  EXPECT_FALSE(commit_set.HasBookmarkCommitId());

  commit_set.AddCommitItem(4, BOOKMARKS);
  EXPECT_TRUE(commit_set.HasBookmarkCommitId());

  commit_set.Truncate(4);
  EXPECT_FALSE(commit_set.HasBookmarkCommitId());
}

TEST_F(OrderedCommitSetTest, AddAndRemoveEntries) {
  OrderedCommitSet commit_set;

  ASSERT_TRUE(commit_set.Empty());

  commit_set.AddCommitItem(0, AUTOFILL);
  ASSERT_EQ(static_cast<size_t>(1), commit_set.Size());

  commit_set.Clear();
  ASSERT_TRUE(commit_set.Empty());
}

}  // namespace
}  // namespace sessions
}  // namespace syncer
