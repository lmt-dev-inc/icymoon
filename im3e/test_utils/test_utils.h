#pragma once

#include <gmock/gmock.h>

#include <filesystem>
#include <memory>
#include <ostream>
#include <ranges>
#include <string_view>
#include <type_traits>
#include <vector>

namespace im3e {

using ::testing::_;
using ::testing::AllOf;
using ::testing::AnyNumber;
using ::testing::ByRef;
using ::testing::Const;
using ::testing::ContainerEq;
using ::testing::DoAll;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Field;
using ::testing::FloatEq;
using ::testing::Ge;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;
using ::testing::IsEmpty;
using ::testing::IsFalse;
using ::testing::IsNull;
using ::testing::IsSubsetOf;
using ::testing::IsSupersetOf;
using ::testing::IsTrue;
using ::testing::Mock;
using ::testing::MockFunction;
using ::testing::Ne;
using ::testing::NiceMock;
using ::testing::NotNull;
using ::testing::Pointee;
using ::testing::Pointwise;
using ::testing::ResultOf;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SetArgPointee;
using ::testing::StrEq;
using ::testing::StrictMock;
using ::testing::Test;
using ::testing::Unused;
using ::testing::Values;
using ::testing::ValuesIn;

/// @brief Generate a file path to save test output e.g. images
/// @details The file path points to the test suite folder within the current test binary folder and the file name is
/// the given name and extension.
auto generateTestOutputFilePath(std::string_view suiteName, std::string_view name, std::string_view extension)
    -> std::filesystem::path;

/// @brief Implementation of WhenLocked(), modelled on GoogleMock's WhenSorted().
template <typename InnerMatcher>
class WhenLockedMatcher
{
public:
    explicit WhenLockedMatcher(InnerMatcher innerMatcher)
      : m_innerMatcher(std::move(innerMatcher))
    {
    }

    template <typename Range>
    operator ::testing::Matcher<Range>() const
    {
        return ::testing::MakeMatcher(new Impl<Range>(m_innerMatcher));
    }

private:
    template <typename Range>
    class Impl : public ::testing::MatcherInterface<Range>
    {
        using WeakPtrT = std::ranges::range_value_t<std::remove_cvref_t<Range>>;
        using LockedT = std::vector<std::shared_ptr<typename WeakPtrT::element_type>>;

    public:
        explicit Impl(const InnerMatcher& innerMatcher)
          : m_innerMatcher(::testing::SafeMatcherCast<const LockedT&>(innerMatcher))
        {
        }

        void DescribeTo(std::ostream* os) const override
        {
            *os << "(when locked) ";
            m_innerMatcher.DescribeTo(os);
        }

        void DescribeNegationTo(std::ostream* os) const override
        {
            *os << "(when locked) ";
            m_innerMatcher.DescribeNegationTo(os);
        }

        bool MatchAndExplain(Range weakPtrs, ::testing::MatchResultListener* listener) const override
        {
            LockedT locked;
            for (const auto& weakPtr : weakPtrs)
            {
                locked.emplace_back(weakPtr.lock());
            }

            if (!listener->IsInterested())
            {
                return m_innerMatcher.Matches(locked);
            }

            *listener << "which is " << ::testing::PrintToString(locked) << " when locked";
            ::testing::StringMatchResultListener innerListener;
            const bool matches = m_innerMatcher.MatchAndExplain(locked, &innerListener);
            if (!innerListener.str().empty())
            {
                *listener << ", " << innerListener.str();
            }
            return matches;
        }

    private:
        ::testing::Matcher<const LockedT&> m_innerMatcher;
    };

    InnerMatcher m_innerMatcher;
};

/// @brief Match a range of std::weak_ptr by locking each element and matching the resulting std::shared_ptr vector.
/// @details Expired elements become null pointers, e.g. WhenLocked(Each(NotNull())) checks that none expired.
/// Usage: EXPECT_THAT(weakPtrs, WhenLocked(ElementsAre(sharedPtr1, sharedPtr2)));
/// @param innerMatcher Matcher applied to the std::vector of locked std::shared_ptr.
template <typename InnerMatcher>
auto WhenLocked(InnerMatcher innerMatcher)
{
    return WhenLockedMatcher<InnerMatcher>(std::move(innerMatcher));
}

}  // namespace im3e