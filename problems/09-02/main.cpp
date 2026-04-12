#include <gtest/gtest.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <stack>
#include <string>
#include <utility>
#include <vector>

class Tree {
public:
    struct Node {
        int value;
        std::shared_ptr<Node> left;
        std::shared_ptr<Node> right;
        std::weak_ptr<Node> parent;

        explicit Node(const int node_value)
            : value(node_value), left(nullptr), right(nullptr), parent() {
        }

        ~Node() {
            std::cout << "destroy " << value << '\n';
        }
    };

    std::shared_ptr<Node> root;

    Tree() : root(nullptr) {
    }

    void traverse_v1() const {
        if (!root) {
            return;
        }

        std::queue<std::shared_ptr<Node>> nodes;
        nodes.push(root);

        bool is_first = true;
        while (!nodes.empty()) {
            const std::shared_ptr<Node> current = nodes.front();
            nodes.pop();

            if (!is_first) {
                std::cout << ' ';
            }
            std::cout << current->value;
            is_first = false;

            if (current->left) {
                nodes.push(current->left);
            }
            if (current->right) {
                nodes.push(current->right);
            }
        }

        std::cout << '\n';
    }

    void traverse_v2() const {
        if (!root) {
            return;
        }

        std::stack<std::shared_ptr<Node>> nodes;
        nodes.push(root);

        bool is_first = true;
        while (!nodes.empty()) {
            const std::shared_ptr<Node> current = nodes.top();
            nodes.pop();

            if (!is_first) {
                std::cout << ' ';
            }
            std::cout << current->value;
            is_first = false;

            if (current->right) {
                nodes.push(current->right);
            }
            if (current->left) {
                nodes.push(current->left);
            }
        }

        std::cout << '\n';
    }
};

class CoutCapture {
public:
    CoutCapture() : old_buffer_(std::cout.rdbuf(stream_.rdbuf())) {
    }

    CoutCapture(const CoutCapture&) = delete;
    CoutCapture& operator=(const CoutCapture&) = delete;

    ~CoutCapture() {
        Restore();
    }

    std::string RestoreAndRead() {
        Restore();
        return stream_.str();
    }

private:
    void Restore() {
        if (old_buffer_ != nullptr) {
            std::cout.rdbuf(old_buffer_);
            old_buffer_ = nullptr;
        }
    }

    std::ostringstream stream_;
    std::streambuf* old_buffer_;
};

Tree CreateSampleTree() {
    constexpr int kRootValue = 1;
    constexpr int kLeftValue = 2;
    constexpr int kRightValue = 3;
    constexpr int kLeftLeftValue = 4;
    constexpr int kLeftRightValue = 5;
    constexpr int kRightLeftValue = 6;
    constexpr int kRightRightValue = 7;

    Tree tree;

    tree.root = std::make_shared<Tree::Node>(kRootValue);
    tree.root->left = std::make_shared<Tree::Node>(kLeftValue);
    tree.root->right = std::make_shared<Tree::Node>(kRightValue);

    tree.root->left->parent = tree.root;
    tree.root->right->parent = tree.root;

    tree.root->left->left = std::make_shared<Tree::Node>(kLeftLeftValue);
    tree.root->left->right = std::make_shared<Tree::Node>(kLeftRightValue);
    tree.root->right->left = std::make_shared<Tree::Node>(kRightLeftValue);
    tree.root->right->right = std::make_shared<Tree::Node>(kRightRightValue);

    tree.root->left->left->parent = tree.root->left;
    tree.root->left->right->parent = tree.root->left;
    tree.root->right->left->parent = tree.root->right;
    tree.root->right->right->parent = tree.root->right;

    return tree;
}

std::vector<std::weak_ptr<Tree::Node>> CollectWeakNodes(const Tree& tree) {
    return {
        tree.root,
        tree.root->left,
        tree.root->right,
        tree.root->left->left,
        tree.root->left->right,
        tree.root->right->left,
        tree.root->right->right
    };
}

std::vector<int> ParseDestroyedValues(const std::string& text) {
    const std::string prefix = "destroy ";
    std::istringstream input(text);
    std::string line;
    std::vector<int> values;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        EXPECT_TRUE(line.rfind(prefix, 0) == 0);
        values.push_back(std::stoi(line.substr(prefix.size())));
    }

    return values;
}

void DestroySilently(Tree& tree) {
    CoutCapture capture;
    tree.root.reset();
    static_cast<void>(capture.RestoreAndRead());
}

TEST(TreeTest, TraverseBreadthFirstPrintsExpectedOrder) {
    Tree tree = CreateSampleTree();

    CoutCapture capture;
    tree.traverse_v1();
    const std::string output = capture.RestoreAndRead();

    EXPECT_EQ(output, "1 2 3 4 5 6 7\n");

    DestroySilently(tree);
}

TEST(TreeTest, TraverseDepthFirstPrintsExpectedOrder) {
    Tree tree = CreateSampleTree();

    CoutCapture capture;
    tree.traverse_v2();
    const std::string output = capture.RestoreAndRead();

    EXPECT_EQ(output, "1 2 4 5 3 6 7\n");

    DestroySilently(tree);
}

TEST(TreeTest, ParentLinksAreWeakAndDoNotCreateOwnershipCycles) {
    Tree tree = CreateSampleTree();

    ASSERT_NE(tree.root, nullptr);
    ASSERT_NE(tree.root->left, nullptr);
    ASSERT_NE(tree.root->right, nullptr);
    ASSERT_NE(tree.root->left->left, nullptr);
    ASSERT_NE(tree.root->left->right, nullptr);
    ASSERT_NE(tree.root->right->left, nullptr);
    ASSERT_NE(tree.root->right->right, nullptr);

    EXPECT_EQ(tree.root.use_count(), 1);
    EXPECT_EQ(tree.root->left.use_count(), 1);
    EXPECT_EQ(tree.root->right.use_count(), 1);
    EXPECT_EQ(tree.root->left->left.use_count(), 1);
    EXPECT_EQ(tree.root->left->right.use_count(), 1);
    EXPECT_EQ(tree.root->right->left.use_count(), 1);
    EXPECT_EQ(tree.root->right->right.use_count(), 1);

    EXPECT_EQ(tree.root->left->parent.lock(), tree.root);
    EXPECT_EQ(tree.root->right->parent.lock(), tree.root);
    EXPECT_EQ(tree.root->left->left->parent.lock(), tree.root->left);
    EXPECT_EQ(tree.root->left->right->parent.lock(), tree.root->left);
    EXPECT_EQ(tree.root->right->left->parent.lock(), tree.root->right);
    EXPECT_EQ(tree.root->right->right->parent.lock(), tree.root->right);

    DestroySilently(tree);
}

TEST(TreeTest, ResettingRootDestroysAllNodesAndBreaksAllReferences) {
    Tree tree = CreateSampleTree();
    const std::vector<std::weak_ptr<Tree::Node>> weak_nodes = CollectWeakNodes(tree);

    CoutCapture capture;
    tree.root.reset();
    const std::string destruction_output = capture.RestoreAndRead();

    EXPECT_EQ(tree.root, nullptr);

    for (const std::weak_ptr<Tree::Node>& node : weak_nodes) {
        EXPECT_TRUE(node.expired());
    }

    std::vector<int> destroyed_values = ParseDestroyedValues(destruction_output);
    std::sort(destroyed_values.begin(), destroyed_values.end());

    const std::vector<int> expected_values{1, 2, 3, 4, 5, 6, 7};
    EXPECT_EQ(destroyed_values, expected_values);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}