#include <im3e/utils/path.h>

#include <im3e/test_utils/test_utils.h>

using namespace im3e;

TEST(PathTest, constructor_default)
{
    const Path path;
    EXPECT_THAT(path.isRoot(), IsTrue());
    EXPECT_THAT(path.getName(), StrEq(""));
    EXPECT_THAT(path.getParent(), Eq(path));
    EXPECT_THAT(path.toString(), StrEq("/"));
    EXPECT_THAT(path, Eq(Path("/")));
}

TEST(PathTest, constructor)
{
    const Path path("/camera/view");
    EXPECT_THAT(path.isRoot(), IsFalse());
    EXPECT_THAT(path.getName(), Eq("view"));
    EXPECT_THAT(path.getParent(), Eq(Path("/camera")));
    EXPECT_THAT(path.toString(), Eq("/camera/view"));
}

TEST(PathTest, constructor_withEmptyComponents)
{
    EXPECT_THAT(Path("//camera//view/"), Eq(Path("/camera/view")));
}

TEST(PathTest, constructor_withInvalidPaths)
{
    EXPECT_THROW(Path("camera"), std::invalid_argument);
    EXPECT_THROW(Path(""), std::invalid_argument);
    EXPECT_THROW(Path("/camera") / "", std::invalid_argument);
    EXPECT_THROW(Path("/camera") / "view/position", std::invalid_argument);
}

TEST(PathTest, appendOperator)
{
    EXPECT_THAT(Path("/camera") / "view", Eq(Path("/camera/view")));
    EXPECT_THAT(Path() / "camera", Eq(Path("/camera")));
}
