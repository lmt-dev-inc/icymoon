#include <im3e/state/state.h>

#include <im3e/test_utils/glm.h>
#include <im3e/test_utils/test_utils.h>

using namespace im3e;

namespace {

struct StateTest : public Test
{
};

}  // namespace

TEST_F(StateTest, workInProgress)
{
    EXPECT_THAT(true, IsFalse());
}