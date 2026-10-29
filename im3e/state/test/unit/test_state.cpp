#include <im3e/state/state.h>

#include <im3e/test_utils/glm.h>
#include <im3e/test_utils/test_utils.h>
#include <im3e/utils/core/throw_utils.h>

#include <fmt/format.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <compare>
#include <limits>
#include <numbers>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

using namespace im3e;

// TODO: move to the im3e_state library once the API settles.
namespace im3e::state {

/// @brief Absolute path of an entity in the state tree, e.g. "/camera/view".
/// Unlike std::filesystem::path, the separator is always "/" whatever the OS, and ":" has no special meaning.
class Path
{
public:
    /// @brief Root path: "/".
    Path() = default;
    Path(const char* pPath)
      : Path(std::string_view{pPath})
    {
    }
    Path(std::string_view path)
    {
        throwIfFalse<std::invalid_argument>(path.starts_with('/'),
                                            fmt::format("State path must be absolute: \"{}\"", path));
        for (auto component : std::views::split(path, '/'))
        {
            if (!component.empty())
            {
                m_components.emplace_back(component.begin(), component.end());
            }
        }
    }

    [[nodiscard]] auto operator/(std::string_view name) const -> Path
    {
        throwIfFalse<std::invalid_argument>(!name.empty() && name.find('/') == std::string_view::npos,
                                            fmt::format("Invalid state path component: \"{}\"", name));
        auto path = *this;
        path.m_components.emplace_back(name);
        return path;
    }

    [[nodiscard]] auto operator<=>(const Path&) const = default;

    [[nodiscard]] auto isRoot() const -> bool { return m_components.empty(); }

    /// @brief Last component of the path, empty for the root.
    [[nodiscard]] auto getName() const -> std::string_view
    {
        return isRoot() ? std::string_view{} : std::string_view{m_components.back()};
    }

    /// @brief Parent of the path. The parent of the root is the root.
    [[nodiscard]] auto getParent() const -> Path
    {
        auto parent = *this;
        if (!parent.isRoot())
        {
            parent.m_components.pop_back();
        }
        return parent;
    }

    [[nodiscard]] auto toString() const -> std::string
    {
        if (isRoot())
        {
            return "/";
        }
        std::string str;
        for (const auto& rComponent : m_components)
        {
            str += '/';
            str += rComponent;
        }
        return str;
    }

private:
    std::vector<std::string> m_components;
};

}  // namespace im3e::state

namespace {

// Objects owning an entity are not copyable, since state::Entity is not: they have an identity in the state tree,
// like a QObject. They are movable, and their state is copied through snapshots instead.
class ViewTransform
{
public:
    explicit ViewTransform(state::Path path)
      : m_entity(std::move(path), state::EntityConfig{
                                      .name = "View Transform",
                                      .properties = {Position, Direction, Up, Right, Matrix},
                                  })
    {
        _updateMatrix(m_entity);
    }

    void setPosition(const glm::vec3& rPosition) { m_entity.set(Position, rPosition); }
    void setDirection(const glm::vec3& rDirection, const glm::vec3& rUp)
    {
        // Listeners and onChange callbacks are notified once, when the transaction ends, instead of once per property:
        auto transaction = m_entity.startTransaction();

        const auto direction = glm::normalize(rDirection);
        m_entity.set(Direction, direction);

        // Update right from the given direction and up:
        const auto right = glm::normalize(glm::cross(direction, rUp));
        m_entity.set(Right, right);

        // Ensure that up is orthogonal to direction:
        m_entity.set(Up, glm::normalize(glm::cross(right, direction)));
    }

    [[nodiscard]] auto getPosition() const -> const glm::vec3& { return m_entity.get(Position); }
    [[nodiscard]] auto getDirection() const -> const glm::vec3& { return m_entity.get(Direction); }
    [[nodiscard]] auto getUp() const -> const glm::vec3& { return m_entity.get(Up); }
    [[nodiscard]] auto getRight() const -> const glm::vec3& { return m_entity.get(Right); }
    [[nodiscard]] auto getMatrix() const -> const glm::mat4& { return m_entity.get(Matrix); }
    [[nodiscard]] auto getEntity() const -> const state::Entity& { return m_entity; }

private:
    // Callbacks are given the entity that changed, so they never capture `this` and moves need no re-binding:
    static void _updateMatrix(state::Entity& rEntity)
    {
        const auto& rPosition = rEntity.get(Position);
        rEntity.set(Matrix, glm::lookAt(rPosition, rPosition + rEntity.get(Direction), rEntity.get(Up)));
    }

    // Keys describe the properties once for all instances. Typed access requires the key, and keys are private, so
    // only ViewTransform can write to its read-only properties. Generic access (GUI, scripts...) honors `access`.
    static inline const state::PropertyKey<glm::vec3> Position{{
        .id = "position",
        .name = "Position",
        .description = "Camera position in world space",
        .defaultValue = glm::vec3{0.0F, 0.0F, 0.0F},
        .onChange = &ViewTransform::_updateMatrix,
    }};
    static inline const state::PropertyKey<glm::vec3> Direction{{
        .id = "direction",
        .name = "Direction",
        .description = "Camera direction in world space",
        .defaultValue = glm::vec3{0.0F, 0.0F, -1.0F},
        .access = state::Access::ReadOnly,
        .onChange = &ViewTransform::_updateMatrix,
    }};
    static inline const state::PropertyKey<glm::vec3> Up{{
        .id = "up",
        .name = "Up",
        .description = "Camera up vector in world space",
        .defaultValue = glm::vec3{0.0F, 1.0F, 0.0F},
        .access = state::Access::ReadOnly,
        .onChange = &ViewTransform::_updateMatrix,
    }};
    static inline const state::PropertyKey<glm::vec3> Right{{
        .id = "right",
        .name = "Right",
        .description = "Camera right vector in world space",
        .defaultValue = glm::vec3{1.0F, 0.0F, 0.0F},
        .access = state::Access::ReadOnly,
        .onChange = &ViewTransform::_updateMatrix,
    }};
    static inline const state::PropertyKey<glm::mat4> Matrix{{
        .id = "matrix",
        .name = "Matrix",
        .description = "View matrix, transforming world space to view space",
        .defaultValue = glm::mat4{1.0F},
        .access = state::Access::ReadOnly,
        .onChange = &ViewTransform::_updateMatrix,
    }};

    // Proxy to the entity's node in the current registry: the node is added at the given path on construction, and
    // removed on destruction.
    state::Entity m_entity;
};

class PerspectiveProjection
{
public:
    explicit PerspectiveProjection(state::Path path)
      : m_entity(std::move(path), state::EntityConfig{
                                      .name = "Perspective Projection",
                                      .properties = {FovY, AspectRatio, Near, Far, Matrix},
                                  })
    {
        _updateMatrix(m_entity);
    }

    void setFovY(float fovY) { m_entity.set(FovY, fovY); }
    void setAspectRatio(float aspectRatio) { m_entity.set(AspectRatio, aspectRatio); }
    void setNear(float near) { m_entity.set(Near, near); }
    void setFar(float far) { m_entity.set(Far, far); }

    [[nodiscard]] auto getFovY() const -> float { return m_entity.get(FovY); }
    [[nodiscard]] auto getAspectRatio() const -> float { return m_entity.get(AspectRatio); }
    [[nodiscard]] auto getNear() const -> float { return m_entity.get(Near); }
    [[nodiscard]] auto getFar() const -> float { return m_entity.get(Far); }
    [[nodiscard]] auto getMatrix() const -> const glm::mat4& { return m_entity.get(Matrix); }
    [[nodiscard]] auto getEntity() const -> const state::Entity& { return m_entity; }

private:
    static void _updateMatrix(state::Entity& rEntity)
    {
        rEntity.set(Matrix,
                    glm::perspective(rEntity.get(FovY), rEntity.get(AspectRatio), rEntity.get(Near), rEntity.get(Far)));
    }

    static inline const state::PropertyKey<float> FovY{{
        .id = "fovY",
        .name = "FOV Y",
        .description = "Vertical Field of View",
        .defaultValue = std::numbers::pi_v<float> / 3.0F,
        .minValue = std::numbers::pi_v<float> / 180.0F,
        .maxValue = std::numbers::pi_v<float> * 179.0F / 180.0F,
        .onChange = &PerspectiveProjection::_updateMatrix,
    }};
    // Read-only because it follows the size of the render panel:
    static inline const state::PropertyKey<float> AspectRatio{{
        .id = "aspectRatio",
        .name = "Aspect Ratio",
        .description = "Aspect Ratio of the perspective projection",
        .defaultValue = 1.0F,
        .minValue = 0.0F,
        .access = state::Access::ReadOnly,
        .onChange = &PerspectiveProjection::_updateMatrix,
    }};
    static inline const state::PropertyKey<float> Near{{
        .id = "near",
        .name = "Near",
        .description = "Distance from the eye to the near plane of the perspective projection",
        .defaultValue = 0.1F,
        .minValue = std::numeric_limits<float>::epsilon(),
        .onChange = &PerspectiveProjection::_updateMatrix,
    }};
    static inline const state::PropertyKey<float> Far{{
        .id = "far",
        .name = "Far",
        .description = "Distance from the eye to the far plane of the perspective projection",
        .defaultValue = 10'000.0F,
        .minValue = std::numeric_limits<float>::epsilon(),
        .onChange = &PerspectiveProjection::_updateMatrix,
    }};
    static inline const state::PropertyKey<glm::mat4> Matrix{{
        .id = "matrix",
        .name = "Matrix",
        .description = "Projection matrix, transforming view space to clip space",
        .defaultValue = glm::mat4{1.0F},
        .access = state::Access::ReadOnly,
    }};

    state::Entity m_entity;
};

class Camera
{
public:
    // Children are created at paths under the camera's. The registry links them to the camera's node whatever the
    // order they are created in, creating missing intermediate nodes on demand:
    explicit Camera(state::Path path)
      : m_entity(path, state::EntityConfig{
                           .name = "Camera",
                           .properties = {ViewProjectionMatrix},
                           .onChildChange = &Camera::_updateViewProjectionMatrix,
                       })
      , m_projection(path / "perspectiveProjection")
      , m_view(path / "viewTransform")
    {
        _updateViewProjectionMatrix(m_entity);
    }

    // Changes made through these references reach the camera through the registry, which knows the parent of each
    // entity, so the camera does not need to give callbacks to its children:
    auto getProjection() -> PerspectiveProjection& { return m_projection; }
    auto getView() -> ViewTransform& { return m_view; }

    [[nodiscard]] auto getProjection() const -> const PerspectiveProjection& { return m_projection; }
    [[nodiscard]] auto getView() const -> const ViewTransform& { return m_view; }
    [[nodiscard]] auto getViewProjectionMatrix() const -> const glm::mat4&
    {
        return m_entity.get(ViewProjectionMatrix);
    }
    [[nodiscard]] auto getEntity() const -> const state::Entity& { return m_entity; }

private:
    // Callbacks only get the entity, not the Camera, so they cannot call m_view.getMatrix(). Children's values are
    // read through the generic API instead, since the children's keys are private:
    static void _updateViewProjectionMatrix(state::Entity& rEntity)
    {
        const auto& rProjectionMatrix = rEntity.getChild("perspectiveProjection").get<glm::mat4>("matrix");
        const auto& rViewMatrix = rEntity.getChild("viewTransform").get<glm::mat4>("matrix");
        rEntity.set(ViewProjectionMatrix, rProjectionMatrix * rViewMatrix);
    }

    static inline const state::PropertyKey<glm::mat4> ViewProjectionMatrix{{
        .id = "viewProjectionMatrix",
        .name = "View Projection Matrix",
        .description = "Transforms world space to clip space",
        .defaultValue = glm::mat4{1.0F},
        .access = state::Access::ReadOnly,
    }};

    state::Entity m_entity;
    PerspectiveProjection m_projection;
    ViewTransform m_view;
};

struct StateTest : public Test
{
    // Replaces the registry returned by state::getRegistry() for the duration of each test:
    state::ScopedRegistry m_scopedRegistry;
};

}  // namespace

TEST(PathTest, constructFromString)
{
    const state::Path path("/camera/view");

    EXPECT_THAT(path.toString(), Eq("/camera/view"));
    EXPECT_THAT(path.getName(), Eq("view"));
    EXPECT_THAT(path.getParent(), Eq(state::Path("/camera")));
}

TEST(PathTest, root)
{
    const state::Path root;

    EXPECT_THAT(root.isRoot(), IsTrue());
    EXPECT_THAT(root.toString(), Eq("/"));
    EXPECT_THAT(state::Path("/"), Eq(root));
    EXPECT_THAT(root.getParent(), Eq(root));
}

TEST(PathTest, append)
{
    EXPECT_THAT(state::Path("/camera") / "view", Eq(state::Path("/camera/view")));
}

TEST(PathTest, invalidPathsThrow)
{
    EXPECT_THROW(state::Path("camera"), std::invalid_argument);
    EXPECT_THROW(state::Path("/camera") / "", std::invalid_argument);
    EXPECT_THROW(state::Path("/camera") / "view/position", std::invalid_argument);
}

TEST_F(StateTest, setPositionUpdatesMatrix)
{
    ViewTransform view("/view");

    const auto position = glm::vec3{1.0F, 2.0F, 3.0F};
    view.setPosition(position);

    EXPECT_THAT(view.getPosition(), Eq(position));
    EXPECT_THAT(view.getMatrix(), Eq(glm::lookAt(position, position + view.getDirection(), view.getUp())));
}

TEST_F(StateTest, entityIsAddedToAndRemovedFromRegistry)
{
    {
        ViewTransform view("/view");
        EXPECT_THAT(state::getRegistry().findEntity("/view"), NotNull());
    }
    EXPECT_THAT(state::getRegistry().findEntity("/view"), IsNull());
}

TEST_F(StateTest, genericAccessHonorsReadOnly)
{
    ViewTransform view("/view");

    // How a GUI or a script would access the state, without knowing ViewTransform:
    auto* pEntity = state::getRegistry().findEntity("/view");
    ASSERT_THAT(pEntity, NotNull());

    const auto position = glm::vec3{1.0F, 2.0F, 3.0F};
    pEntity->setAnyValue("position", position);
    EXPECT_THAT(view.getPosition(), Eq(position));

    EXPECT_THROW(pEntity->setAnyValue("direction", glm::vec3{1.0F, 0.0F, 0.0F}), std::runtime_error);
}

TEST_F(StateTest, takeSnapshot)
{
    ViewTransform view("/view");
    view.setPosition(glm::vec3{1.0F, 2.0F, 3.0F});

    const auto snapshot = state::getRegistry().takeSnapshot();

    // The snapshot is a copy of the values: later changes do not affect it.
    view.setPosition(glm::vec3{4.0F, 5.0F, 6.0F});
    EXPECT_THAT(snapshot.getValue<glm::vec3>("/view", "position"), Eq(glm::vec3{1.0F, 2.0F, 3.0F}));
}

TEST_F(StateTest, cameraBuildsTreeFromItsMembers)
{
    Camera camera("/camera");

    EXPECT_THAT(state::getRegistry().findEntity("/camera"), NotNull());
    EXPECT_THAT(state::getRegistry().findEntity("/camera/perspectiveProjection"), NotNull());
    EXPECT_THAT(state::getRegistry().findEntity("/camera/viewTransform"), NotNull());
}

TEST_F(StateTest, childChangeUpdatesCamera)
{
    Camera camera("/camera");

    camera.getView().setPosition(glm::vec3{1.0F, 2.0F, 3.0F});
    camera.getProjection().setFovY(std::numbers::pi_v<float> / 4.0F);

    EXPECT_THAT(camera.getViewProjectionMatrix(),
                Eq(camera.getProjection().getMatrix() * camera.getView().getMatrix()));
}
