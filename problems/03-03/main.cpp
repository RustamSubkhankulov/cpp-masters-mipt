#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

class List final {
public:
  List() noexcept = default;

  List(const List&) = delete;
  List(List&&) = delete;

  List& operator=(const List&) = delete;
  List& operator=(List&&) = delete;

  ~List() noexcept {
    clear();
  }

  [[nodiscard]] bool empty() const noexcept {
    return size_ == 0uz;
  }

  [[nodiscard]] std::size_t size() const noexcept {
    return size_;
  }

  void show(std::ostream& os) const {
    if (head_ == nullptr) {
      return;
    }

    os << head_->value;
    auto* cur = head_->next;

    while (cur) {
      os << ' ' << cur->value;
      cur = cur->next;
    }
  }

  void push_front(int v) {
    auto* n = new Node(v, head_);

    head_ = n;
    if (tail_ == nullptr) {
      tail_ = n;
    }

    size_ += 1uz;
  }

  void push_back(int v) {
    auto* n = new Node(v, nullptr);

    if (tail_) {
      tail_->next = n;
      tail_ = n;
    } else {
      head_ = tail_ = n;
    }

    size_ += 1uz;
  }

  void pop_front() {
    if (empty()) {
      throw std::out_of_range("pop_front on empty list");
    }

    auto* n = head_;

    head_ = head_->next;
    if (head_ == nullptr) {
      tail_ = nullptr;
    }

    delete n;
    size_ -= 1uz;
  }

  void pop_back() {
    if (empty()) {
      throw std::out_of_range("pop_back on empty list");
    }

    if (head_ == tail_) {
      delete head_;
      head_ = tail_ = nullptr;
      size_ = 0uz;
      return;
    }

    Node* prev = nullptr;
    auto* cur = head_;
    while (cur->next) {
      prev = cur;
      cur = cur->next;
    }

    prev->next = nullptr;
    tail_ = prev;

    delete cur;
    size_ -= 1uz;
  }

  // Returns the value of the current middle node.
  // For even-sized lists, this returns the second middle (upper middle).
  int get() const {
    if (empty()) {
      throw std::out_of_range("get on empty list");
    }

    auto* slow = head_;
    auto* fast = head_;

    while (fast && fast->next) {
      slow = slow->next;
      fast = fast->next->next;
    }

    return slow->value;
  }

private:
  struct Node {
    int value{};
    Node* next{nullptr};
    explicit Node(int v, Node* n = nullptr)
      : value(v)
      , next(n) {}
  };

  Node* head_{nullptr};
  Node* tail_{nullptr};
  std::size_t size_{};

private:
  void clear() noexcept {
    auto* cur = head_;

    while (cur) {
      auto* next = cur->next;
      delete cur;
      cur = next;
    }

    head_ = tail_ = nullptr;
    size_ = 0uz;
  }
};

namespace {

std::string captureShow(const List& lst) {
  std::stringstream ss;
  lst.show(ss);
  return ss.str();
}

TEST(Basic, InitialState) {
  List lst;
  EXPECT_TRUE(lst.empty());
  EXPECT_EQ(lst.size(), 0uz);
  EXPECT_EQ(captureShow(lst), "");
  EXPECT_THROW(lst.get(), std::out_of_range);
  EXPECT_THROW(lst.pop_front(), std::out_of_range);
  EXPECT_THROW(lst.pop_back(), std::out_of_range);
}

TEST(Push, PushFrontSingle) {
  List lst;
  lst.push_front(42);
  EXPECT_FALSE(lst.empty());
  EXPECT_EQ(lst.size(), 1uz);
  EXPECT_EQ(captureShow(lst), "42");
  EXPECT_EQ(lst.get(), 42);
}

TEST(Push, PushBackSingle) {
  List lst;
  lst.push_back(7);
  EXPECT_FALSE(lst.empty());
  EXPECT_EQ(lst.size(), 1uz);
  EXPECT_EQ(captureShow(lst), "7");
  EXPECT_EQ(lst.get(), 7);
}

TEST(Push, PushFBF) {
  List lst;
  lst.push_front(2);
  lst.push_back(3);
  lst.push_front(1);
  EXPECT_FALSE(lst.empty());
  EXPECT_EQ(lst.size(), 3uz);
  EXPECT_EQ(captureShow(lst), "1 2 3");
  EXPECT_EQ(lst.get(), 2);
}

TEST(Push, PushBBF) {
  List lst;
  lst.push_back(10);
  lst.push_back(20);
  lst.push_front(5);
  EXPECT_FALSE(lst.empty());
  EXPECT_EQ(lst.size(), 3uz);
  EXPECT_EQ(captureShow(lst), "5 10 20");
  EXPECT_EQ(lst.get(), 10);
}

TEST(Pop, PopFrontSingle) {
  List lst;
  lst.push_back(1);
  lst.pop_front();
  EXPECT_TRUE(lst.empty());
  EXPECT_EQ(lst.size(), 0uz);
  EXPECT_EQ(captureShow(lst), "");
}

TEST(Pop, PopBackSingle) {
  List lst;
  lst.push_back(1);
  lst.pop_back();
  EXPECT_TRUE(lst.empty());
  EXPECT_EQ(lst.size(), 0uz);
  EXPECT_EQ(captureShow(lst), "");
}

TEST(Pop, PopFrontMultiple) {
  List lst;
  lst.push_back(1);
  lst.push_back(2);
  lst.push_back(3);
  lst.pop_front();
  EXPECT_FALSE(lst.empty());
  EXPECT_EQ(lst.size(), 2uz);
  EXPECT_EQ(captureShow(lst), "2 3");
  EXPECT_EQ(lst.get(), 3); // middle of [2,3] is second middle -> 3
}

TEST(Pop, PopBackMultiple) {
  List lst;
  lst.push_back(1);
  lst.push_back(2);
  lst.push_back(3);
  lst.pop_back();
  EXPECT_FALSE(lst.empty());
  EXPECT_EQ(lst.size(), 2uz);
  EXPECT_EQ(captureShow(lst), "1 2");
  EXPECT_EQ(lst.get(), 2); // second middle in even-length list
}

TEST(Pop, PopBF) {
  List lst;
  lst.push_back(4);
  lst.push_back(5);
  lst.pop_back();
  lst.pop_front();
  EXPECT_TRUE(lst.empty());
  EXPECT_EQ(lst.size(), 0uz);
  EXPECT_THROW(lst.pop_back(), std::out_of_range);
  EXPECT_THROW(lst.pop_front(), std::out_of_range);
}

TEST(Get, MiddleOdd) {
  List lst;
  for (int v : {1, 2, 3, 4, 5}) {
    lst.push_back(v);
  }
  EXPECT_EQ(lst.get(), 3);
}

TEST(Get, MiddleEven) {
  List lst;
  for (int v : {1, 2, 3, 4}) {
    lst.push_back(v);
  }
  EXPECT_EQ(lst.get(), 3);
}

TEST(Get, PushPopMixed) {
  List lst;
  lst.push_front(3);
  lst.push_back(4);
  lst.push_front(2);
  lst.push_front(1);
  lst.push_back(5);
  EXPECT_EQ(lst.get(), 3);
}

TEST(Show, Basic) {
  List lst;
  for (int v : {10, 20, 30}) {
    lst.push_back(v);
  }
  std::string s = captureShow(lst);
  EXPECT_EQ(s, "10 20 30");
}

TEST(Integration, MixedOps) {
  List lst;

  for (int v : {1, 2, 3}) {
    lst.push_back(v);
  }

  EXPECT_EQ(lst.size(), 3uz);
  EXPECT_EQ(captureShow(lst), "1 2 3");
  EXPECT_EQ(lst.get(), 2);

  lst.push_front(0);  // [0,1,2,3]
  lst.push_front(-1); // [-1,0,1,2,3]

  EXPECT_EQ(captureShow(lst), "-1 0 1 2 3");
  EXPECT_EQ(lst.size(), 5uz);
  EXPECT_EQ(lst.get(), 1);

  lst.pop_front();
  lst.pop_back();

  EXPECT_EQ(captureShow(lst), "0 1 2");
  EXPECT_EQ(lst.size(), 3uz);
  EXPECT_EQ(lst.get(), 1);

  lst.push_back(4);
  lst.push_front(-5);

  EXPECT_EQ(captureShow(lst), "-5 0 1 2 4");
  EXPECT_EQ(lst.size(), 5uz);
  EXPECT_EQ(lst.get(), 1);

  lst.pop_front();
  lst.pop_front();
  lst.pop_back();
  lst.pop_back();

  EXPECT_EQ(captureShow(lst), "1");
  EXPECT_EQ(lst.size(), 1uz);
  EXPECT_EQ(lst.get(), 1);

  lst.pop_front();

  EXPECT_TRUE(lst.empty());
  EXPECT_EQ(lst.size(), 0uz);
  EXPECT_EQ(captureShow(lst), "");
  EXPECT_THROW(lst.get(), std::out_of_range);
}

} // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
