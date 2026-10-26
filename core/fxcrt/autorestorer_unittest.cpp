// Copyright 2018 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/autorestorer.h"

#include <memory>
#include <utility>

#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/pseudo_retainable.h"

TEST(fxcrt, AutoRestorer) {
  int x = 5;
  {
    AutoRestorer<int> restorer(&x);
    x = 6;
    EXPECT_EQ(6, x);
  }
  EXPECT_EQ(5, x);
  {
    AutoRestorer<int> restorer(&x, 7);
    EXPECT_EQ(7, x);
  }
  EXPECT_EQ(5, x);
}

TEST(fxcrt, AutoRestorerNoOp) {
  int x = 5;
  {
    AutoRestorer<int> restorer(&x);
    EXPECT_EQ(5, x);
  }
  EXPECT_EQ(5, x);
}

TEST(fxcrt, AutoRestorerAbandon) {
  int x = 5;
  {
    AutoRestorer<int> restorer(&x, 6);
    EXPECT_EQ(6, x);
    restorer.AbandonRestoration();
  }
  EXPECT_EQ(6, x);
}

TEST(fxcrt, AutoRestorerUnownedPtr) {
  int x = 5;
  int y = 6;
  UnownedPtr<int> ptr(&x);
  {
    AutoRestorer<UnownedPtr<int>> restorer(&ptr);
    ptr = &y;
    EXPECT_EQ(&y, ptr);
  }
  EXPECT_EQ(&x, ptr);
  {
    AutoRestorer<UnownedPtr<int>> restorer(&ptr, &y);
    EXPECT_EQ(&y, ptr);
  }
  EXPECT_EQ(&x, ptr);
}

TEST(fxcrt, AutoRestorerUnownedNoOp) {
  int x = 5;
  UnownedPtr<int> ptr(&x);
  {
    AutoRestorer<UnownedPtr<int>> restorer(&ptr);
    EXPECT_EQ(&x, ptr);
  }
  EXPECT_EQ(&x, ptr);
}

TEST(fxcrt, AutoRestorerUnownedPtrAbandon) {
  int x = 5;
  int y = 6;
  UnownedPtr<int> ptr(&x);
  {
    AutoRestorer<UnownedPtr<int>> restorer(&ptr);
    ptr = &y;
    EXPECT_EQ(&y, ptr);
    restorer.AbandonRestoration();
  }
  EXPECT_EQ(&y, ptr);
}

TEST(fxcrt, AutoRestorerRetainPtr) {
  PseudoRetainable obj1;
  PseudoRetainable obj2;
  {
    RetainPtr<PseudoRetainable> ptr(&obj1);
    EXPECT_EQ(1, obj1.retain_count());
    EXPECT_EQ(0, obj1.release_count());
    {
      AutoRestorer<RetainPtr<PseudoRetainable>> restorer(&ptr);
      // `obj1` is kept alive by restorer in case it need to be restored.
      EXPECT_EQ(2, obj1.retain_count());
      EXPECT_EQ(0, obj1.release_count());
      ptr.Reset(&obj2);
      EXPECT_EQ(&obj2, ptr.Get());

      // `obj1` released by `ptr` assignment, but kept alive by `AutoRestorer`'s
      // `old_value_`.
      EXPECT_TRUE(obj1.alive());
      EXPECT_EQ(2, obj1.retain_count());
      EXPECT_EQ(1, obj1.release_count());

      // `obj2` now kept alive by `ptr`.
      EXPECT_TRUE(obj1.alive());
      EXPECT_EQ(1, obj2.retain_count());
      EXPECT_EQ(0, obj2.release_count());
    }
    EXPECT_EQ(&obj1, ptr.Get());

    // Moving from `AutoRestorer`'s `old_value_` into `ptr` avoided ref churn.
    EXPECT_TRUE(obj1.alive());
    EXPECT_EQ(2, obj1.retain_count());
    EXPECT_EQ(1, obj1.release_count());

    // `obj2` dead.
    EXPECT_FALSE(obj2.alive());
    EXPECT_EQ(1, obj2.retain_count());
    EXPECT_EQ(1, obj2.release_count());
  }
  EXPECT_FALSE(obj1.alive());
  EXPECT_EQ(2, obj1.retain_count());
  EXPECT_EQ(2, obj1.release_count());
  EXPECT_FALSE(obj2.alive());
  EXPECT_EQ(1, obj2.retain_count());
  EXPECT_EQ(1, obj2.release_count());
}

TEST(fxcrt, AutoRestorerRetainPtrCopy) {
  PseudoRetainable obj1;
  PseudoRetainable obj2;
  {
    RetainPtr<PseudoRetainable> ptr1(&obj1);
    RetainPtr<PseudoRetainable> ptr2(&obj2);
    EXPECT_EQ(1, obj1.retain_count());
    EXPECT_EQ(0, obj1.release_count());
    EXPECT_EQ(1, obj2.retain_count());
    EXPECT_EQ(0, obj2.release_count());
    {
      AutoRestorer<RetainPtr<PseudoRetainable>> restorer(&ptr1, ptr2);
      EXPECT_EQ(&obj2, ptr1.Get());
      EXPECT_EQ(&obj2, ptr2.Get());

      // `obj1` was moved into `restorer`'s `old_value_` without ref churn,
      // where it is kept alive until restoration.
      EXPECT_TRUE(obj1.alive());
      EXPECT_EQ(1, obj1.retain_count());
      EXPECT_EQ(0, obj1.release_count());

      // `obj2` was copied into `ptr1`, incrementing its retain count.
      EXPECT_TRUE(obj2.alive());
      EXPECT_EQ(2, obj2.retain_count());
      EXPECT_EQ(0, obj2.release_count());
    }
    EXPECT_EQ(&obj1, ptr1.Get());
    EXPECT_EQ(&obj2, ptr2.Get());

    // `obj1` moved back to `ptr1` on destruction, with zero ref churn.
    EXPECT_TRUE(obj1.alive());
    EXPECT_EQ(1, obj1.retain_count());
    EXPECT_EQ(0, obj1.release_count());

    // `ptr1`'s copy of `obj2` was released when overwritten by restoration.
    EXPECT_TRUE(obj2.alive());
    EXPECT_EQ(2, obj2.retain_count());
    EXPECT_EQ(1, obj2.release_count());
  }
  EXPECT_FALSE(obj1.alive());
  EXPECT_EQ(1, obj1.retain_count());
  EXPECT_EQ(1, obj1.release_count());
  EXPECT_FALSE(obj2.alive());
  EXPECT_EQ(2, obj2.retain_count());
  EXPECT_EQ(2, obj2.release_count());
}

TEST(fxcrt, AutoRestorerRetainPtrMove) {
  PseudoRetainable obj1;
  PseudoRetainable obj2;
  {
    RetainPtr<PseudoRetainable> ptr1(&obj1);
    RetainPtr<PseudoRetainable> ptr2(&obj2);
    EXPECT_EQ(1, obj1.retain_count());
    EXPECT_EQ(0, obj1.release_count());
    EXPECT_EQ(1, obj2.retain_count());
    EXPECT_EQ(0, obj2.release_count());
    {
      AutoRestorer<RetainPtr<PseudoRetainable>> restorer(&ptr1,
                                                         std::move(ptr2));
      EXPECT_EQ(&obj2, ptr1.Get());
      EXPECT_FALSE(ptr2);

      // `obj1` was moved into `restorer`'s `old_value_` without ref churn,
      // where it is kept alive until restoration.
      EXPECT_TRUE(obj1.alive());
      EXPECT_EQ(1, obj1.retain_count());
      EXPECT_EQ(0, obj1.release_count());

      // `ptr2` was moved into `ptr1` without ref churn.
      EXPECT_TRUE(obj2.alive());
      EXPECT_EQ(1, obj2.retain_count());
      EXPECT_EQ(0, obj2.release_count());
    }
    EXPECT_EQ(&obj1, ptr1.Get());
    EXPECT_FALSE(ptr2);

    // `obj1` moved back to `ptr1` on destruction, with zero ref churn.
    EXPECT_TRUE(obj1.alive());
    EXPECT_EQ(1, obj1.retain_count());
    EXPECT_EQ(0, obj1.release_count());

    // `ptr1`'s sole reference to `obj2` was released on restoration, so `obj2`
    // dies.
    EXPECT_FALSE(obj2.alive());
    EXPECT_EQ(1, obj2.retain_count());
    EXPECT_EQ(1, obj2.release_count());
  }
  EXPECT_FALSE(obj1.alive());
  EXPECT_EQ(1, obj1.retain_count());
  EXPECT_EQ(1, obj1.release_count());
  EXPECT_FALSE(obj2.alive());
  EXPECT_EQ(1, obj2.retain_count());
  EXPECT_EQ(1, obj2.release_count());
}

TEST(fxcrt, AutoRestorerRetainPtrNoOp) {
  PseudoRetainable obj1;
  {
    RetainPtr<PseudoRetainable> ptr(&obj1);
    EXPECT_EQ(1, obj1.retain_count());
    EXPECT_EQ(0, obj1.release_count());
    {
      AutoRestorer<RetainPtr<PseudoRetainable>> restorer(&ptr);
      EXPECT_EQ(2, obj1.retain_count());
      EXPECT_EQ(0, obj1.release_count());
    }
    EXPECT_EQ(&obj1, ptr.Get());

    // Self-reassignement avoided ref churn.
    EXPECT_TRUE(obj1.alive());
    EXPECT_EQ(2, obj1.retain_count());
    EXPECT_EQ(1, obj1.release_count());
  }
  EXPECT_FALSE(obj1.alive());
  EXPECT_EQ(2, obj1.retain_count());
  EXPECT_EQ(2, obj1.release_count());
}

TEST(fxcrt, AutoRestorerRetainPtrAbandon) {
  PseudoRetainable obj1;
  PseudoRetainable obj2;
  {
    RetainPtr<PseudoRetainable> ptr(&obj1);
    EXPECT_EQ(1, obj1.retain_count());
    EXPECT_EQ(0, obj1.release_count());
    {
      AutoRestorer<RetainPtr<PseudoRetainable>> restorer(&ptr);
      // `obj1` is kept alive by restorer in case it need to be restored.
      EXPECT_EQ(2, obj1.retain_count());
      EXPECT_EQ(0, obj1.release_count());
      ptr.Reset(&obj2);
      EXPECT_EQ(&obj2, ptr.Get());

      // `obj1` released by `ptr` assignment.
      EXPECT_EQ(2, obj1.retain_count());
      EXPECT_EQ(1, obj1.release_count());

      // `obj2` now kept alive by `ptr`.
      EXPECT_EQ(1, obj2.retain_count());
      EXPECT_EQ(0, obj2.release_count());

      restorer.AbandonRestoration();
    }
    EXPECT_EQ(&obj2, ptr.Get());

    // `obj1` now dead as a result of abandonment.
    EXPECT_FALSE(obj1.alive());
    EXPECT_EQ(2, obj1.retain_count());
    EXPECT_EQ(2, obj1.release_count());

    // `obj2` kept alive by `ptr`.
    EXPECT_TRUE(obj2.alive());
    EXPECT_EQ(1, obj2.retain_count());
    EXPECT_EQ(0, obj2.release_count());
  }
  EXPECT_FALSE(obj1.alive());
  EXPECT_EQ(2, obj1.retain_count());
  EXPECT_EQ(2, obj1.release_count());
  EXPECT_FALSE(obj2.alive());
  EXPECT_EQ(1, obj2.retain_count());
  EXPECT_EQ(1, obj2.release_count());
}

TEST(fxcrt, AutoRestorerMoveOnlyType) {
  auto ptr1 = std::make_unique<int>(5);
  auto ptr2 = std::make_unique<int>(6);
  {
    AutoRestorer<std::unique_ptr<int>> restorer(&ptr1, std::move(ptr2));
    EXPECT_FALSE(ptr2);
    EXPECT_EQ(6, *ptr1);
  }
  EXPECT_EQ(5, *ptr1);
}

TEST(fxcrt, AutoRestorerMoveOnlyTypeAbandon) {
  auto ptr1 = std::make_unique<int>(5);
  auto ptr2 = std::make_unique<int>(6);
  {
    AutoRestorer<std::unique_ptr<int>> restorer(&ptr1, std::move(ptr2));
    EXPECT_FALSE(ptr2);
    EXPECT_EQ(6, *ptr1);
    restorer.AbandonRestoration();
  }
  EXPECT_EQ(6, *ptr1);
}
