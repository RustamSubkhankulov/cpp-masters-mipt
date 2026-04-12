#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

template <typename T>
class List
{
private:
    struct Node
    {
        explicit Node(T value_arg)
            : value(std::move(value_arg))
        {
        }

        T value;
        std::shared_ptr<Node> next;
        std::weak_ptr<Node> prev;
    };

public:
    class Iterator
    {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T *;
        using reference = T &;

        Iterator() = default;

        Iterator(std::shared_ptr<Node> node, const List * owner)
            : m_node(std::move(node))
            , m_owner(owner)
        {
        }

        Iterator & operator++()
        {
            m_node = m_node->next;
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator copy = *this;
            ++(*this);
            return copy;
        }

        Iterator & operator--()
        {
            if (m_node)
            {
                m_node = m_node->prev.lock();
            }
            else
            {
                m_node = m_owner->m_tail;
            }

            return *this;
        }

        Iterator operator--(int)
        {
            Iterator copy = *this;
            --(*this);
            return copy;
        }

        reference operator*() const
        {
            return m_node->value;
        }

        pointer operator->() const
        {
            return &m_node->value;
        }

        friend bool operator==(const Iterator & lhs, const Iterator & rhs)
        {
            return lhs.m_node == rhs.m_node && lhs.m_owner == rhs.m_owner;
        }

        friend bool operator!=(const Iterator & lhs, const Iterator & rhs)
        {
            return !(lhs == rhs);
        }

    private:
        std::shared_ptr<Node> m_node;
        const List * m_owner = nullptr;
    };

    void push_back(T value)
    {
        auto node = std::make_shared<Node>(std::move(value));

        if (m_tail)
        {
            node->prev = m_tail;
            m_tail->next = node;
            m_tail = std::move(node);
        }
        else
        {
            m_head = node;
            m_tail = std::move(node);
        }
    }

    Iterator begin()
    {
        return Iterator(m_head, this);
    }

    Iterator end()
    {
        return Iterator(nullptr, this);
    }

private:
    std::shared_ptr<Node> m_head;
    std::shared_ptr<Node> m_tail;
};

namespace
{
template <typename T>
std::vector<T> CollectForward(List<T> & list)
{
    std::vector<T> values;

    for (const auto & value : list)
    {
        values.push_back(value);
    }

    return values;
}

template <typename T>
std::vector<T> CollectBackward(List<T> & list)
{
    std::vector<T> values;
    auto iterator = list.end();

    while (iterator != list.begin())
    {
        --iterator;
        values.push_back(*iterator);
    }

    return values;
}

struct Record
{
    int id = 0;
    std::string name;
};
}  // namespace

TEST(ListTest, PushBackBuildsCorrectForwardOrder)
{
    List<int> list;
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    list.push_back(4);

    const std::vector<int> expected = {1, 2, 3, 4};

    EXPECT_EQ(CollectForward(list), expected);
}

TEST(ListTest, IteratorSupportsPrefixAndPostfixIncrement)
{
    List<int> list;
    list.push_back(10);
    list.push_back(20);
    list.push_back(30);

    auto iterator = list.begin();

    EXPECT_EQ(*iterator, 10);
    EXPECT_EQ(*iterator++, 10);
    EXPECT_EQ(*iterator, 20);
    EXPECT_EQ(*++iterator, 30);
}

TEST(ListTest, IteratorSupportsPrefixAndPostfixDecrement)
{
    List<int> list;
    list.push_back(10);
    list.push_back(20);
    list.push_back(30);

    auto iterator = list.end();

    EXPECT_EQ(*--iterator, 30);
    EXPECT_EQ(*iterator--, 30);
    EXPECT_EQ(*iterator, 20);
    EXPECT_EQ(*--iterator, 10);
}

TEST(ListTest, BackwardTraversalWorksThroughPreviousLinks)
{
    List<int> list;
    list.push_back(5);
    list.push_back(6);
    list.push_back(7);

    const std::vector<int> expected = {7, 6, 5};

    EXPECT_EQ(CollectBackward(list), expected);
}

TEST(ListTest, OperatorArrowProvidesAccessToStoredObject)
{
    List<Record> list;
    list.push_back(Record{7, "node"});

    auto iterator = list.begin();

    EXPECT_EQ(iterator->id, 7);
    EXPECT_EQ(iterator->name, "node");
}

TEST(ListTest, EmptyListHasEqualBeginAndEnd)
{
    List<int> list;

    EXPECT_EQ(list.begin(), list.end());
}

int main(int argc, char ** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
