#include <mtf/coroutine/simple.hpp>

#include <context/stack.hpp>

#include <wheels/test/test_framework.hpp>

#include <memory>
#include <thread>
#include <chrono>

//////////////////////////////////////////////////////////////////////

struct TreeNode;

using TreeNodePtr = std::shared_ptr<TreeNode>;

struct TreeNode {
  TreeNodePtr left_;
  TreeNodePtr right_;

  TreeNode(TreeNodePtr left = nullptr, TreeNodePtr right = nullptr)
      : left_(std::move(left)), right_(std::move(right)) {
  }

  static TreeNodePtr Create(TreeNodePtr left, TreeNodePtr right) {
    return std::make_shared<TreeNode>(std::move(left), std::move(right));
  }

  static TreeNodePtr CreateLeaf() {
    return std::make_shared<TreeNode>();
  }
};

//////////////////////////////////////////////////////////////////////

using Coroutine = mtf::coroutine::SimpleCoroutine;

//////////////////////////////////////////////////////////////////////

#include <wheels/support/mmap_allocation.hpp>

TEST_SUITE(Coroutine) {
  SIMPLE_TEST(JustWorks) {
    Coroutine foo([&]() {
      // 1
      Coroutine::Suspend();
      // 2
    });

    ASSERT_FALSE(foo.IsCompleted());
    foo.Resume();
    foo.Resume();
    ASSERT_TRUE(foo.IsCompleted());

    // ASSERT_THROW(foo.Resume(), CoroutineCompleted)
  }

  SIMPLE_TEST(Interleaving) {
    int step = 0;

    Coroutine first([&]() {
      ASSERT_EQ(step, 0);
      step = 1;
      Coroutine::Suspend();
      ASSERT_EQ(step, 2);
      step = 3;
    });

    Coroutine second([&]() {
      ASSERT_EQ(step, 1);
      step = 2;
      Coroutine::Suspend();
      ASSERT_EQ(step, 3);
      step = 4;
    });

    first.Resume();
    second.Resume();

    ASSERT_EQ(step, 2);

    first.Resume();
    second.Resume();

    ASSERT_TRUE(first.IsCompleted());
    ASSERT_TRUE(second.IsCompleted());

    ASSERT_EQ(step, 4);
  }

  SIMPLE_TEST(Threads) {
    size_t steps = 0;

    Coroutine coroutine([&steps]() {
      std::cout << "Step" << std::endl;
      ++steps;
      Coroutine::Suspend();
      std::cout << "Step" << std::endl;
      ++steps;
      Coroutine::Suspend();
      std::cout << "Step" << std::endl;
      ++steps;
    });

    for (size_t i = 0; i < 3; ++i) {
      std::thread t([&]() {
        coroutine.Resume();
      });
      t.join();
    }

    ASSERT_EQ(steps, 3);
  }

  void TreeWalk(TreeNodePtr node) {
    if (node->left_) {
      TreeWalk(node->left_);
    }
    Coroutine::Suspend();
    if (node->right_) {
      TreeWalk(node->right_);
    }
  }

  SIMPLE_TEST(TreeWalk) {
    TreeNodePtr root = TreeNode::Create(
        TreeNode::CreateLeaf(),
        TreeNode::Create(
            TreeNode::Create(TreeNode::CreateLeaf(), TreeNode::CreateLeaf()),
            TreeNode::CreateLeaf()));

    Coroutine walker([&root]() {
      TreeWalk(root);
    });

    size_t node_count = 0;

    while (true) {
      walker.Resume();
      if (walker.IsCompleted()) {
        break;
      }
      ++node_count;
    }

    ASSERT_EQ(node_count, 7);
  }

  SIMPLE_TEST(Pipeline) {
    const size_t kSteps = 123;

    size_t inner_step_count = 0;

    auto inner_routine = [&]() {
      for (size_t i = 0; i < kSteps; ++i) {
        ++inner_step_count;
        Coroutine::Suspend();
      }
    };

    auto outer_routine = [&]() {
      Coroutine inner(inner_routine);
      while (!inner.IsCompleted()) {
        inner.Resume();
        Coroutine::Suspend();
      }
    };

    Coroutine outer(outer_routine);
    while (!outer.IsCompleted()) {
      outer.Resume();
    }

    ASSERT_EQ(inner_step_count, kSteps);
  }

  /*
  SIMPLE_TEST(NotInCoroutine) {
    ASSERT_THROW(Coroutine::Suspend(), coroutine::NotInCoroutine)
  }
  */

  SIMPLE_TEST(Exception) {
    auto foo_routine = [&]() {
      Coroutine::Suspend();
      throw std::runtime_error("Test exception");
    };

    Coroutine foo(foo_routine);

    ASSERT_FALSE(foo.IsCompleted());
    foo.Resume();
    ASSERT_THROW(foo.Resume(), std::runtime_error);
    ASSERT_TRUE(foo.IsCompleted());
  }

  struct MyException {};

  SIMPLE_TEST(NestedException1) {
    auto bar_routine = [&]() {
      throw MyException();
    };

    auto foo_routine = [&]() {
      Coroutine bar(bar_routine);
      ASSERT_THROW(bar.Resume(), MyException);
    };

    Coroutine foo(foo_routine);
    foo.Resume();
  }

  SIMPLE_TEST(NestedException2) {
    auto bar_routine = [&]() {
      throw MyException();
    };

    auto foo_routine = [&]() {
      Coroutine bar(bar_routine);
      bar.Resume();
    };

    Coroutine foo(foo_routine);
    ASSERT_THROW(foo.Resume(), MyException);
  }

  SIMPLE_TEST(ExceptionsHard) {
    int score = 0;

    Coroutine a([&]() {
      Coroutine b([]() {
        throw 1;
      });
      try {
        b.Resume();
      } catch (int) {
        ++score;
        Coroutine::Suspend();
        throw;
      }
    });

    a.Resume();

    std::thread t([&]() {
      try {
        a.Resume();
      } catch (int) {
        ++score;
      }
    });
    t.join();

    ASSERT_EQ(score, 2);
  }

  SIMPLE_TEST(Leak) {
    auto shared_ptr = std::make_shared<int>(42);
    std::weak_ptr<int> weak_ptr = shared_ptr;

    {
      auto routine = [ptr = std::move(shared_ptr)]() {};
      Coroutine co(routine);
      co.Resume();
    }

    ASSERT_FALSE(weak_ptr.lock());
  }
}
