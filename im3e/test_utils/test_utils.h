#pragma once

#include <gmock/gmock.h>

namespace im3e {

using ::testing::_;
using ::testing::AllOf;
using ::testing::AnyNumber;
using ::testing::ByRef;
using ::testing::Const;
using ::testing::ContainerEq;
using ::testing::DoAll;
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
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SetArgPointee;
using ::testing::StrEq;
using ::testing::StrictMock;
using ::testing::Test;
using ::testing::Unused;
using ::testing::Values;
using ::testing::ValuesIn;

}  // namespace im3e