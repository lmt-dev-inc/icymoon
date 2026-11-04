# PropertyGroup Tests Review

Review of the unit tests of `PropertyGroup` (branch `100-do-not-use-shared_ptr-to-hold-properties`), done on 2026-11-03. It lists refactorings that simplify the existing tests and the ones to come, then the behaviors that are not tested yet. Delete this document once the work is done.

Files concerned:

- [test_property_group.cpp](../../im3e/property/test/unit/test_property_group.cpp): the tests.
- [mock_property_interface.h](../../im3e/property/mock/include/im3e/property/mock_property_interface.h) and [mock_property_interface.cpp](../../im3e/property/mock/src/mock_property_interface.cpp): the mocks. `test_property_group.cpp` is their only user, so they can change freely.
- [property_group.h](../../im3e/property/lib/include/im3e/property/property_group.h) and [property_group.cpp](../../im3e/property/lib/src/property_group.cpp): the class under test.
- [test_property.cpp](../../im3e/property/test/unit/test_property.cpp): the tests of `Property<T>`, used as the reference for naming and structure.

- [Summary](#summary)
- [Refactorings](#refactorings)
  - [R1. Mocks notify through a real `CallbackRegistry`](#r1-mocks-notify-through-a-real-callbackregistry)
  - [R2. One helper to notify the children](#r2-one-helper-to-notify-the-children)
  - [R3. Free functions instead of a stateless fixture](#r3-free-functions-instead-of-a-stateless-fixture)
  - [R4. Align `ConfigEq` with `test_property.cpp`](#r4-align-configeq-with-test_propertycpp)
  - [R5. Rename and extend `constructor_withNullChildren`](#r5-rename-and-extend-constructor_withnullchildren)
  - [R6. Where the helpers live](#r6-where-the-helpers-live)
- [Gaps](#gaps)
  - [Interface](#interface)
  - [Lifetimes](#lifetimes)
  - [Behavior](#behavior)
  - [Not needed: tests with real children](#not-needed-tests-with-real-children)
- [Design decisions](#design-decisions)
  - [Duplicate children throw](#duplicate-children-throw)
  - [Notification when a child is destroyed](#notification-when-a-child-is-destroyed)
- [Order of work](#order-of-work)

## Summary

| Id  | Kind        | Item                                                                                | Priority |
| --- | ----------- | ----------------------------------------------------------------------------------- | -------- |
| R1  | Refactoring | Mocks register callbacks in a real `CallbackRegistry` and expose `notifyChanged()`. | Done     |
| R2  | Refactoring | Replace the loop duplicated in two tests (it also calls `front()` unchecked).       | Done     |
| R3  | Refactoring | Turn the stateless fixture into free functions and use `TEST`.                      | Done     |
| R4  | Refactoring | `ConfigEq`: `Eq` instead of `StrEq`, drop `typename`.                               | Done     |
| R5  | Refactoring | Rename `constructor_withNullChildren`, also cover an empty `weak_ptr`.              | Done     |
| R6  | Refactoring | `LockedElementsAre` became the general `WhenLocked(matcher)` in `test_utils.h`.     | Done     |
| —   | Gap         | `getInterface()` is not tested at all (6 tests).                                    | High     |
| —   | Gap         | Lifetimes: children outliving the group, cancelled registrations (4 tests).         | High     |
| —   | Gap         | Behavior: no children, callback reading the children (2 tests).                     | Medium   |
| —   | Change      | The constructor throws on duplicate children, with its test.                        | Medium   |

## Refactorings

### R1. Mocks notify through a real `CallbackRegistry`

**Problem.** `MockPropertyNode` stores every callback it receives in a `std::vector<std::function<void()>>`, and returns a `CallbackRegistration` built from a token that owns nothing (`createMockToken` in [mock_property_interface.cpp](../../im3e/property/mock/src/mock_property_interface.cpp)). As a result:

- **Registrations have no effect.** Releasing or destroying the registration does not unregister anything, so the tests cannot check that the group keeps its registrations alive, or releases them when destroyed.
- **Lifetime tests would crash.** The stored callbacks capture the group's `Impl` as `this`. Calling one after the group is destroyed is undefined behavior instead of a no-op, so the most important lifetime test ([childOutlivesGroup](#lifetimes)) cannot be written.
- **Notifying is verbose.** Each test has to fetch `getRegisteredOnChange()`, check its size, take `front()` and call it.

**Change.** Give each mock a real `CallbackRegistry<>`, register into it by default, and add `notifyChanged()`:

```cpp
class MockPropertyNode : public IPropertyNode
{
public:
    MockPropertyNode();
    ~MockPropertyNode() override;

    MOCK_METHOD(CallbackRegistration, registerOnChange, (std::function<void()>), (const, override));
    MOCK_METHOD(const std::string&, getName, (), (const, override));

    // Calls the callbacks whose registration is still alive, as a real property does on a change.
    void notifyChanged() { m_callbacks.invokeAll(); }

private:
    mutable CallbackRegistry<> m_callbacks;
};

MockPropertyNode::MockPropertyNode()
{
    ON_CALL(*this, registerOnChange(_)).WillByDefault([this](auto onChange) {
        return m_callbacks.registerCallback(std::move(onChange));
    });
}
```

- `EXPECT_CALL(*mockChild, registerOnChange(_))` keeps working: an `EXPECT_CALL` without an action uses the `ON_CALL` default.
- Do the same for `MockProperty` and `MockPropertyGroup`, and remove `getRegisteredOnChange()`, `m_token` and `createMockToken`. The three mocks then repeat the same constructor body; if that bothers, a small helper class holding the registry and the `ON_CALL` setup can be shared, but three short copies are acceptable.
- Side benefit: a registration made with `registerOnChange({})` now throws `std::invalid_argument` in the mock too, like in the real classes.

### R2. One helper to notify the children

`constructor_withOnChangeCallback` and `registerOnChange` contain the same 7-line loop. It also has a latent bug: it checks `registeredCallbacks.size()` with `EXPECT_THAT`, which does not stop the test, then calls `front()`, which is undefined behavior on an empty vector. It should have been `ASSERT_THAT`.

With R1, the loop becomes:

```cpp
void notifyChanged(const std::vector<std::shared_ptr<MockPropertyNode>>& mockChildren)
{
    for (const auto& mockChild : mockChildren)
    {
        mockChild->notifyChanged();
    }
}
```

and both tests shrink to:

```cpp
EXPECT_CALL(mockOnChange, Call()).Times(mockChildren.size());
notifyChanged(mockChildren);
```

Whether each child registered exactly once is already checked by `constructor` through `expectChildrenRegistered`, so the tests no longer need to check it themselves.

### R3. Free functions instead of a stateless fixture

`PropertyGroupTest` holds no state: its helpers could be free functions in the anonymous namespace, next to `ConfigEq`, and the tests could use `TEST` like `test_property.cpp`. If the fixture stays, remove the redundant `public:` in the `struct` and make the helpers `static`.

A fixture becomes worthwhile if most tests start sharing setup (for example a default set of 3 children and an `onChange` mock). Today the counts vary per test (3 to 6) for no specific reason: a common default of 3 would let the fixture own them, and the few tests that need a specific set can create their own.

### R4. Align `ConfigEq` with `test_property.cpp`

- `Eq(expected.name)` instead of `StrEq(expected.name)`, as `test_property.cpp` does. Both work on `std::string`; `Eq` is the one used elsewhere.
- `using ConfigT = PropertyGroup::Config;` without `typename`: the type is not dependent here (the `typename` is only needed in `test_property.cpp`'s template version).

### R5. Rename and extend `constructor_withNullChildren`

The children of this test are not null, they are expired: rename it `constructor_withExpiredChildren`. Also add a default-constructed `std::weak_ptr<IPropertyNode>{}` to the input, which is a different case (never pointed to anything) handled by the same `lock()` check.

### R6. Where the helpers live

`createMockPropertyNodes` and `toWeakPtrVector` are only used in this file, so they stay there. `LockedElementsAre` was replaced by `WhenLocked(matcher)` in [test_utils.h](../../im3e/test_utils/test_utils.h), since `std::weak_ptr` will be common: it locks every element of a range of `std::weak_ptr` and applies any matcher to the result, e.g. `WhenLocked(ElementsAreArray(mockChildren))` or `WhenLocked(Each(NotNull()))`. It is modelled on GoogleMock's `WhenSorted()`.

## Gaps

Test names follow the existing convention, `method_condition`, and mirror the names used in `test_property.cpp` where the behavior is the same.

### Interface

`getInterface()` is not tested at all. `test_property.cpp` has the equivalent tests for `Property<T>` (`interface`, `interface_registerOnChange`, `interfaceExpiresWithProperty`, `interfaceOutlivesProperty`), which can serve as models.

| Test                                          | Checks                                                                                                                                                                                                                                                                      |
| --------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `interface`                                   | `getInterface().lock()` is not null, `getName()` returns `config.name`, `getChildren()` returns the children (`WhenLocked`).                                                                                                                                                |
| `interface_const`                             | The `const` overload, called through a `const PropertyGroup&`, returns a valid `weak_ptr<const IPropertyGroup>` with the same name and children.                                                                                                                            |
| `interface_registerOnChange`                  | A callback registered through the interface is called when a child changes.                                                                                                                                                                                                 |
| `interface_registerOnChange_withNullCallback` | Registering `{}` through the interface throws `std::invalid_argument`.                                                                                                                                                                                                      |
| `interfaceExpiresWithGroup`                   | The `weak_ptr` expires when the group is destroyed and nobody else holds the interface.                                                                                                                                                                                     |
| `interfaceOutlivesGroup`                      | With the interface held as a `shared_ptr` after the group is destroyed: the `onChange` given to the constructor no longer fires (its registration belonged to the group), a callback registered by the test still fires on a child change, and `getChildren()` still works. |

### Lifetimes

| Test                                   | Checks                                                                                                                                                                                                                             |
| -------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `childOutlivesGroup`                   | Destroy the group (no interface held), then `notifyChanged()` on the children: nothing is called and nothing crashes. This guards the `[this]` capture in `Impl`'s constructor. **Needs R1.** Most important of the list.          |
| `registerOnChange_canBeCancelled`      | After `registration.reset()`, a child change no longer calls the callback. Same as `PropertyTest.registerOnChange_canBeCancelled`.                                                                                                 |
| `getChildren_whenMiddleChildDestroyed` | Destroy a child in the middle of the list (the existing test only drops the last ones with `resize`): the remaining children keep their order.                                                                                     |
| `registerOnChange_afterChildDestroyed` | Destroy a child, then `notifyChanged()` on a remaining one: the group's callbacks still fire once and `getChildren()` is still correct. This goes through the `std::erase_if` in `Impl::_onChange()`, which no test reaches today. |

### Behavior

| Test                                    | Checks                                                                                                                                                                       |
| --------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `constructor_withNoChildren`            | An empty list gives an empty `getChildren()` and a valid interface. `registerOnChange_withNullCallback` builds one but checks nothing about it.                              |
| `registerOnChange_callbackSeesChildren` | Inside the callback, `getChildren()` returns the expected children, like `PropertyTest.registerOnChange_callbackSeesNewValue`: a listener refreshing a UI does exactly this. |

### Not needed: tests with real children

Decided not to add tests using real `Property<T>` or nested `PropertyGroup` children. `PropertyGroup` only uses its children through `IPropertyNode`, so:

- Each side of the `registerOnChange` contract is tested on its own: `Property<T>`'s tests check when a node notifies, the group's tests check what the group does when notified. With R1, the mock notifies like a real node.
- Nested groups are covered by composition: a group notifies its listeners when a child notifies (its own tests), and an inner group is just a child.

Tests with real children would repeat these checks. The goal is simple tests without repetition, through mocks and good coverage of both classes.

## Design decisions

### Duplicate children throw

Giving the same node twice is always a mistake of the caller, so the constructor throws `std::invalid_argument` rather than registering, listing and notifying the node twice. Failing at construction means a group never exists in a bad state, and the exception can be caught when children are chosen at run time. Python bindings would also translate it to a `ValueError` (pybind11 and nanobind both map `std::invalid_argument` this way).

This is an implementation change, not only a test:

- In `PropertyGroup::Impl`'s constructor, compare each live child to the ones already kept, by the address of the locked node. A linear search over `m_children` is enough for the sizes of groups. Use `throwIfFalse<std::invalid_argument>(...)` like `Property<T>`'s constructor.
- Expired and empty `weak_ptr`s are skipped before the check, so several of them are not duplicates.
- Test: `constructor_withDuplicateChildren`, with `EXPECT_THROW`. Also document it in the constructor's doc comment.
- Out of scope: the same node in two different groups of one tree is legitimate, even if an ancestor of both is then notified twice.

### Notification when a child is destroyed

Decided not to add it now. The analysis is recorded in [properties.md](../architecture/properties.md#notify-when-a-child-is-destroyed), to revisit when adding and removing children is designed. Remaining work: say in `PropertyGroup`'s constructor doc comment that a destroyed child disappears from `getChildren()` without notification.

## Order of work

1. ~~R1, the mocks: it is a prerequisite for `childOutlivesGroup` and simplifies R2.~~ Done.
2. ~~R2 to R5 on the existing tests.~~ Done.
3. The gap tests, starting with `childOutlivesGroup` and the interface tests.
4. The duplicate check in the constructor, with `constructor_withDuplicateChildren`.
5. Build and run `test_im3e_property` through the build-and-test skill.
6. Delete this document and its link in [docs/index.md](../index.md).
