#include <im3e/utils/core/platform_utils.h>

#include <im3e/test_utils/test_utils.h>

using namespace im3e;

TEST(PlatformUtilsTest, getCurrentExecutableFolder)
{
    const auto folder = getCurrentExecutableFolder();
    EXPECT_THAT(std::filesystem::is_directory(folder), IsTrue());
}
