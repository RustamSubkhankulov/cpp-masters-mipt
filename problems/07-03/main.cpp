#include <gtest/gtest.h>

#include <ios>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <string_view>
#include <utility>

/*
Normal branching points in the original function:
1. The operator `||` creates short-circuit branching:
   - if `person.grade() == 10` is true, `person.salary()` is not evaluated;
   - otherwise `person.salary()` is evaluated.
2. The `if` statement selects one of two normal paths:
   - `save(Status::success, person.id())`
   - `save(Status::failure, person.id())`

Potential exception points in the original function:
1. `std::cout << "test : "` may throw if the stream is configured to throw
   or if its buffer fails.
2. `person.name()` may throw.
3. `<< person.name()` may throw.
4. `<< '\n'` may throw.
5. `person.grade()` may throw.
6. `person.salary()` may throw when the left operand of `||` is false.
7. `person.id()` may throw in the selected branch.
8. `save(...)` may throw.
9. If `grade()` or `salary()` return user-defined comparable types instead of
   arithmetic values, `==` and `>` may also throw.
*/

enum class Status
{
    success,
    failure
};

enum class ThrowPoint
{
    none,
    name,
    grade,
    salary,
    id
};

consteval int ExcellentGrade()
{
    return 10;
}

consteval int HighSalaryThreshold()
{
    return 1000000;
}

consteval int SaveThrowsId()
{
    return -1;
}

class NameError final : public std::runtime_error
{
public:
    NameError() : std::runtime_error("person.name() failed")
    {
    }
};

class GradeError final : public std::runtime_error
{
public:
    GradeError() : std::runtime_error("person.grade() failed")
    {
    }
};

class SalaryError final : public std::runtime_error
{
public:
    SalaryError() : std::runtime_error("person.salary() failed")
    {
    }
};

class IdError final : public std::runtime_error
{
public:
    IdError() : std::runtime_error("person.id() failed")
    {
    }
};

class SaveError final : public std::runtime_error
{
public:
    SaveError() : std::runtime_error("save() failed")
    {
    }
};

class Person final
{
public:
    Person(std::string name, int grade, int salary, int id, ThrowPoint throw_point = ThrowPoint::none)
        : name_(std::move(name)),
          grade_(grade),
          salary_(salary),
          id_(id),
          throw_point_(throw_point)
    {
    }

    [[nodiscard]] std::string const & name() const
    {
        if (throw_point_ == ThrowPoint::name)
        {
            throw NameError();
        }
        return name_;
    }

    [[nodiscard]] int grade() const
    {
        if (throw_point_ == ThrowPoint::grade)
        {
            throw GradeError();
        }
        return grade_;
    }

    [[nodiscard]] int salary() const
    {
        if (throw_point_ == ThrowPoint::salary)
        {
            throw SalaryError();
        }
        return salary_;
    }

    [[nodiscard]] int id() const
    {
        if (throw_point_ == ThrowPoint::id)
        {
            throw IdError();
        }
        return id_;
    }

private:
    std::string name_;
    int grade_;
    int salary_;
    int id_;
    ThrowPoint throw_point_;
};

[[nodiscard]] std::string_view ToString(Status status)
{
    switch (status)
    {
    case Status::success:
        return "success";
    case Status::failure:
        return "failure";
    }

    throw std::logic_error("unreachable status");
}

void save(Status status, int id)
{
    // Extra output is used only to make the selected branch visible in tests.
    std::cout << "save(" << ToString(status) << ", " << id << ")\n";

    if (id == SaveThrowsId())
    {
        throw SaveError();
    }
}

void test(Person const & person)
{
    std::cout << "test : " << person.name() << '\n';

    if (person.grade() == ExcellentGrade() || person.salary() > HighSalaryThreshold())
    {
        save(Status::success, person.id());
    }
    else
    {
        save(Status::failure, person.id());
    }
}

class ScopedCoutRedirect final
{
public:
    explicit ScopedCoutRedirect(std::streambuf * new_buffer)
        : old_buffer_(std::cout.rdbuf(new_buffer))
    {
    }

    ScopedCoutRedirect(ScopedCoutRedirect const &) = delete;
    ScopedCoutRedirect & operator=(ScopedCoutRedirect const &) = delete;

    ~ScopedCoutRedirect() noexcept
    {
        std::cout.rdbuf(old_buffer_);
    }

private:
    std::streambuf * old_buffer_;
};

class ScopedCoutExceptionMask final
{
public:
    explicit ScopedCoutExceptionMask(std::ios_base::iostate new_mask)
        : old_mask_(std::cout.exceptions())
    {
        std::cout.clear();
        std::cout.exceptions(new_mask);
    }

    ScopedCoutExceptionMask(ScopedCoutExceptionMask const &) = delete;
    ScopedCoutExceptionMask & operator=(ScopedCoutExceptionMask const &) = delete;

    ~ScopedCoutExceptionMask() noexcept
    {
        try
        {
            std::cout.exceptions(std::ios_base::goodbit);
            std::cout.clear();
            std::cout.exceptions(old_mask_);
        }
        catch (...)
        {
            std::cout.exceptions(std::ios_base::goodbit);
            std::cout.clear();
        }
    }

private:
    std::ios_base::iostate old_mask_;
};

class ThrowingStreambuf final : public std::streambuf
{
protected:
    std::streamsize xsputn(char const *, std::streamsize) override
    {
        throw std::runtime_error("stream write failed");
    }

    int_type overflow(int_type) override
    {
        throw std::runtime_error("stream write failed");
    }
};

template <class Action>
[[nodiscard]] std::string CaptureCout(Action && action)
{
    std::ostringstream buffer;
    ScopedCoutRedirect redirect(buffer.rdbuf());
    std::forward<Action>(action)();
    return buffer.str();
}

[[nodiscard]] std::string DemonstrateSuccessByGrade()
{
    Person const person("Alice", ExcellentGrade(), 500000, 1);
    return CaptureCout([&person]() { test(person); });
}

[[nodiscard]] std::string DemonstrateSuccessBySalary()
{
    Person const person("Bob", 9, HighSalaryThreshold() + 1, 2);
    return CaptureCout([&person]() { test(person); });
}

[[nodiscard]] std::string DemonstrateFailureBranch()
{
    Person const person("Carol", 9, HighSalaryThreshold(), 3);
    return CaptureCout([&person]() { test(person); });
}

[[nodiscard]] std::string DemonstrateShortCircuitAvoidsSalary()
{
    Person const person("Dave", ExcellentGrade(), 0, 4, ThrowPoint::salary);
    return CaptureCout([&person]() { test(person); });
}

void DemonstrateNameException()
{
    Person const person("Eve", ExcellentGrade(), 1, 5, ThrowPoint::name);
    test(person);
}

void DemonstrateGradeException()
{
    Person const person("Frank", ExcellentGrade(), 1, 6, ThrowPoint::grade);
    test(person);
}

void DemonstrateSalaryException()
{
    Person const person("Grace", 9, HighSalaryThreshold() + 1, 7, ThrowPoint::salary);
    test(person);
}

void DemonstrateIdException()
{
    Person const person("Heidi", ExcellentGrade(), 1, 8, ThrowPoint::id);
    test(person);
}

void DemonstrateSaveException()
{
    Person const person("Ivan", ExcellentGrade(), 1, SaveThrowsId());
    test(person);
}

void DemonstrateOutputException()
{
    ThrowingStreambuf throwing_buffer;
    ScopedCoutExceptionMask exception_mask(std::ios_base::badbit);
    ScopedCoutRedirect redirect(&throwing_buffer);

    Person const person("Judy", ExcellentGrade(), 1, 9);
    test(person);
}

TEST(TestFunctionAnalysis, SuccessBranchMayBeChosenByGrade)
{
    EXPECT_EQ(
        DemonstrateSuccessByGrade(),
        std::string("test : Alice\nsave(success, 1)\n"));
}

TEST(TestFunctionAnalysis, SuccessBranchMayBeChosenBySalary)
{
    EXPECT_EQ(
        DemonstrateSuccessBySalary(),
        std::string("test : Bob\nsave(success, 2)\n"));
}

TEST(TestFunctionAnalysis, FailureBranchMayBeChosen)
{
    EXPECT_EQ(
        DemonstrateFailureBranch(),
        std::string("test : Carol\nsave(failure, 3)\n"));
}

TEST(TestFunctionAnalysis, ShortCircuitMaySkipSalaryEvaluation)
{
    EXPECT_EQ(
        DemonstrateShortCircuitAvoidsSalary(),
        std::string("test : Dave\nsave(success, 4)\n"));
}

TEST(TestFunctionAnalysis, NameCallMayThrow)
{
    EXPECT_THROW(DemonstrateNameException(), NameError);
}

TEST(TestFunctionAnalysis, GradeCallMayThrow)
{
    EXPECT_THROW(DemonstrateGradeException(), GradeError);
}

TEST(TestFunctionAnalysis, SalaryCallMayThrowWhenLeftOperandIsFalse)
{
    EXPECT_THROW(DemonstrateSalaryException(), SalaryError);
}

TEST(TestFunctionAnalysis, IdCallMayThrowInSelectedBranch)
{
    EXPECT_THROW(DemonstrateIdException(), IdError);
}

TEST(TestFunctionAnalysis, SaveMayThrow)
{
    EXPECT_THROW(DemonstrateSaveException(), SaveError);
}

TEST(TestFunctionAnalysis, OutputOperationsMayThrow)
{
    EXPECT_ANY_THROW(DemonstrateOutputException());
}

int main(int argc, char ** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
