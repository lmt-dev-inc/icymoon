#include <im3e/utils/core/path_utils.h>

#include <im3e/test_utils/test_utils.h>

using namespace im3e;

namespace {

// "café/高程.tif", spelled with escapes so that the test doesn't depend on the source and execution encodings
const auto Utf8Path = std::string{"caf\xC3\xA9/\xE9\xAB\x98\xE7\xA8\x8B.tif"};
const auto Utf8PathU8 = std::u8string{u8"café/高程.tif"};

}  // namespace

TEST(PathUtilsTest, toUtf8)
{
    EXPECT_THAT(path::toUtf8(std::filesystem::path{Utf8PathU8}), Eq(Utf8Path));
}

TEST(PathUtilsTest, fromUtf8)
{
    EXPECT_THAT(path::fromUtf8(Utf8Path), Eq(std::filesystem::path{Utf8PathU8}));
}

TEST(PathUtilsTest, utf8RoundTrip)
{
    const auto filePath = path::fromUtf8(Utf8Path);
    EXPECT_THAT(filePath.filename(), Eq(std::filesystem::path{u8"高程.tif"}));
    EXPECT_THAT(path::toUtf8(filePath), Eq(Utf8Path));
}
