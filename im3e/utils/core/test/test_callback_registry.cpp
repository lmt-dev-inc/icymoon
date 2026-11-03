#include <im3e/utils/core/callback_registry.h>

#include <im3e/test_utils/test_utils.h>

using namespace im3e;

TEST(CallbackRegistrationTest, constructor)
{
    auto token = std::make_shared<int>(5);
    EXPECT_THAT(token.use_count(), Eq(1U));
    {
        auto registration = CallbackRegistration{token};
        EXPECT_THAT(token.use_count(), Eq(2U));
    }
    EXPECT_THAT(token.use_count(), Eq(1U));
}

TEST(CallbackRegistrationTest, defaultConstructor)
{
    auto registration = CallbackRegistration();
    EXPECT_NO_THROW(registration.reset());
}

TEST(CallbackRegistrationTest, constructorWithNullInput)
{
    EXPECT_NO_THROW(CallbackRegistration registration{nullptr});
}

TEST(CallbackRegistrationTest, reset)
{
    auto token = std::make_shared<int>(42);
    EXPECT_THAT(token.use_count(), Eq(1U));

    auto registration = CallbackRegistration(token);
    EXPECT_THAT(token.use_count(), Eq(2U));

    registration.reset();
    EXPECT_THAT(token.use_count(), Eq(1U));
}

TEST(CallbackRegistryTest, constructor)
{
    EXPECT_NO_THROW(CallbackRegistry());
}

TEST(CallbackRegistryTest, registerCallback)
{
    auto registry = CallbackRegistry<int, float, int>();
    std::vector<MockFunction<void(int, float, int)>> mockCallbacks(10U);
    std::vector<CallbackRegistration> registrations;
    for (auto& mockCallback : mockCallbacks)
    {
        registrations.emplace_back(registry.registerCallback(mockCallback.AsStdFunction()));
        EXPECT_CALL(mockCallback, Call(1, 2.0F, 3));
    }
    registry.invokeAll(1, 2.0F, 3);
}

TEST(CallbackRegistryTest, registerCallback_withNull)
{
    auto registry = CallbackRegistry();
    EXPECT_THROW(auto registration = registry.registerCallback({}), std::invalid_argument);
}

TEST(CallbackRegistryTest, canCancelRegistration)
{
    auto registry = CallbackRegistry<float>();
    MockFunction<void(float)> mockCallback;
    {
        auto registration = registry.registerCallback(mockCallback.AsStdFunction());
    }
    // The registration goes out of scope before we invoke the callbacks so the mock callback should not be called:
    EXPECT_CALL(mockCallback, Call(_)).Times(0);
    registry.invokeAll(3.5F);
}

TEST(CallbackRegistryTest, registerFromInvokedCallback)
{
    auto registry = CallbackRegistry();
    MockFunction<void()> mockCallback;
    auto mockRegistration = CallbackRegistration(nullptr);

    // In this first invocation, we use a lambda callback to trigger a callback registration.
    // Since the registration happens after invoke is called, we do not expect the mock callback to be invoked yet.
    {
        EXPECT_CALL(mockCallback, Call()).Times(0);
        auto registration = registry.registerCallback(
            [&] { mockRegistration = registry.registerCallback(mockCallback.AsStdFunction()); });
        registry.invokeAll();
    }

    // This second invocation should now contain the mock callback:
    EXPECT_CALL(mockCallback, Call());
    registry.invokeAll();
}

TEST(CallbackRegistryTest, registrationCancelledDuringInvocation)
{
    auto registry = CallbackRegistry();
    MockFunction<void()> mockCallback;
    auto secondRegistration = CallbackRegistration();
    auto firstRegistration = registry.registerCallback([&] { secondRegistration.reset(); });
    secondRegistration = registry.registerCallback(mockCallback.AsStdFunction());

    EXPECT_CALL(mockCallback, Call()).Times(0);
    registry.invokeAll();
}

TEST(CallbackRegistryTest, callbackCancelsOwnRegistration)
{
    auto registry = CallbackRegistry();
    MockFunction<void()> mockCallback;
    auto registration = CallbackRegistration();
    registration = registry.registerCallback([&] {
        registration.reset();
        // The callback must remain alive until it returns, even though its registration is now destroyed:
        mockCallback.Call();
    });

    EXPECT_CALL(mockCallback, Call()).Times(1);
    registry.invokeAll();
    registry.invokeAll();
}

TEST(CallbackRegistryTest, callbackThrowing)
{
    auto registry = CallbackRegistry();
    MockFunction<void()> mockCallback;
    auto throwingRegistration = registry.registerCallback([] { throw std::runtime_error("Callback failed"); });
    auto mockRegistration = registry.registerCallback(mockCallback.AsStdFunction());

    // The exception interrupts the invocation so the following callbacks are skipped:
    EXPECT_CALL(mockCallback, Call()).Times(0);
    EXPECT_THROW(registry.invokeAll(), std::runtime_error);
    Mock::VerifyAndClearExpectations(&mockCallback);

    // The registry remains usable after the exception:
    throwingRegistration.reset();
    EXPECT_CALL(mockCallback, Call());
    registry.invokeAll();
}

TEST(CallbackRegistryTest, registrationOutlivesRegistry)
{
    auto registration = CallbackRegistration();
    {
        auto registry = CallbackRegistry();
        registration = registry.registerCallback([] {});
    }
    EXPECT_NO_THROW(registration.reset());
}

TEST(CallbackRegistryTest, invokeAllInRegistrationOrder)
{
    auto registry = CallbackRegistry();
    std::vector<MockFunction<void()>> mockCallbacks(5U);
    std::vector<CallbackRegistration> registrations;

    InSequence sequence;
    for (auto& mockCallback : mockCallbacks)
    {
        registrations.emplace_back(registry.registerCallback(mockCallback.AsStdFunction()));
        EXPECT_CALL(mockCallback, Call());
    }
    registry.invokeAll();
}

TEST(CallbackRegistryTest, invokeAllFromInvokedCallback)
{
    auto registry = CallbackRegistry<int>();
    MockFunction<void(int)> mockCallback;
    auto nestingRegistration = registry.registerCallback([&](int depth) {
        if (depth == 0)
        {
            registry.invokeAll(1);
        }
    });
    auto mockRegistration = registry.registerCallback(mockCallback.AsStdFunction());

    // The nested invocation completes before the outer invocation reaches the mock callback:
    InSequence sequence;
    EXPECT_CALL(mockCallback, Call(1));
    EXPECT_CALL(mockCallback, Call(0));
    registry.invokeAll(0);
}
