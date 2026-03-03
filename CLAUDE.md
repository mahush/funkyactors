# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with the funkyactors library.

## Project Overview

Funkyactors is a minimal, type-safe actor framework built on standalone Asio for in-process concurrent programming. It implements a **pull-based, typed actor model** with:
- Actors communicating through typed Topics (pub-sub channels)
- Messages as plain C++ structs with zero serialization overhead
- Each actor running on its own strand for serialized execution
- Automatic memory management with proper cleanup
- Built-in support for single-shot and periodic timers

## Build and Development Commands

### Build
```bash
# Standard build
cmake -S . -B build
cmake --build build

# With specific compiler
cmake -S . -B build -DCMAKE_CXX_COMPILER=g++-13
cmake --build build
```

### Code Quality
```bash
# Generate compile_commands.json (automatic with current CMakeLists.txt)
cmake -S . -B build

# Format code (uses .clang-format - Google style, 120 cols)
clang-format -i include/funkyactors/*.hpp include/funkyactors/timer/*.hpp src/timer/*.cpp

# Lint (uses .clang-tidy if available)
clang-tidy include/funkyactors/*.hpp src/timer/*.cpp -- -I include -I build/_deps/asio-src/asio/include
```

## Coding Style

### Naming Conventions

**Types** (classes, structs, enums, type aliases): `CamelCase`
```cpp
class Actor { };
struct TimerCommand { };
enum class Command { START, STOP };
using TimerFactoryPtr = std::shared_ptr<TimerFactory>;
```

**Template Parameters**: `TCamelCase` - prefix with `T`, append `Fn` for callables
```cpp
template <typename TMessage, typename TProcessFn>
void processMessages(TMessage& msg, TProcessFn&& fn);
```

**Functions/Methods**: `camelCase`
```cpp
void processInputs();
std::optional<Message> tryTakeMessage();
```

**Member Variables**: `snake_case_` - lowercase with trailing underscore
```cpp
class Actor {
  asio::strand<...> strand_;
  TopicPtr<Msg> topic_;
};
```

**Local Variables & Parameters**: `snake_case` - lowercase, no trailing underscore
```cpp
void subscribe(const TopicPtr<Msg>& topic, size_t queue_size) {
  auto subscription = std::make_shared<Subscription<Msg>>(topic, queue_size);
}
```

**Constants & Enum Values**: `SCREAMING_SNAKE_CASE`
```cpp
constexpr size_t MAX_QUEUE_SIZE = 2;
enum class Command { START_SINGLE_SHOT, START_PERIODIC, CANCEL };
```

**Namespaces**: `snake_case`
```cpp
namespace funkyactors { }
```

**Files**: `snake_case.hpp` / `snake_case.cpp`
```
actor.hpp
timer_factory.hpp
asio_timer_core.cpp
```

**Include Guards**: `SCREAMING_SNAKE_CASE` with full path (NOT #pragma once)
```cpp
#ifndef FUNKYACTORS_ACTOR_HPP
#define FUNKYACTORS_ACTOR_HPP
// ...
#endif  // FUNKYACTORS_ACTOR_HPP
```

**IMPORTANT**: Always use traditional include guards, never `#pragma once`. This ensures compatibility across all compilers and build systems.

### Initialization

**Constructor Initializer Lists:** Always use curly braces `{}` for member initialization in constructor initializer lists. This prevents narrowing conversions.

**Variable Initialization:** For regular variable declarations, use assignment `=` or curly braces `{}`.

### Documentation

**Doxygen-style comments** for public APIs:
```cpp
/**
 * @brief Brief description of what the function does
 *
 * More detailed explanation if needed (optional).
 *
 * @tparam TMessage Type of message this subscription handles
 * @param topic The topic to subscribe to
 * @return Description of return value
 */
template <typename TMessage>
std::shared_ptr<Subscription<TMessage>> create_sub(TopicPtr<TMessage> topic);
```

**Inline comments** for implementation details:
```cpp
// Single-line comment for explanation
void processInputs() {
  // Pull all pending messages
  while (auto msg = sub_->tryTakeMessage()) {
    handleMessage(*msg);
  }
}
```

### Header Organization

**Include Order**: Standard library → Third-party → Local headers
```cpp
#include <memory>      // Standard library
#include <vector>

#include <asio.hpp>    // Third-party

#include "funkyactors/actor.hpp"    // Local headers
#include "funkyactors/topic.hpp"
```

**Include Guards**: Always use traditional include guards (full path in SCREAMING_SNAKE_CASE)
```cpp
#ifndef FUNKYACTORS_TIMER_TIMER_HPP
#define FUNKYACTORS_TIMER_TIMER_HPP
// ...
#endif  // FUNKYACTORS_TIMER_TIMER_HPP
```

**Never use `#pragma once`** - traditional include guards are mandatory for consistency and portability.

**Namespace Closing**: Add comment for clarity
```cpp
namespace funkyactors {
// ...
}  // namespace funkyactors
```

### Class Organization

**Member Access Order**: `public` → `protected` → `private`
```cpp
class MyClass {
 public:
  // Public interface first

 protected:
  // Protected members for derived classes

 private:
  // Private implementation last
};
```

**Group Related Members** with comments:
```cpp
class Topic {
 private:
  // Subscriber management
  std::vector<std::weak_ptr<Subscription<TMessage>>> subscribers_;
  std::mutex subscribers_mutex_;

  // Notification tracking
  std::atomic<bool> is_notifying_{false};
};
```

### Type Aliases

**Define type aliases** for commonly used pointer types to reduce verbosity:
```cpp
using TimerFactoryPtr = std::shared_ptr<TimerFactory>;
template <typename T>
using TopicPtr = std::shared_ptr<Topic<T>>;
template <typename T>
using PublisherPtr = std::shared_ptr<Publisher<T>>;
template <typename T>
using SubscriptionPtr = std::shared_ptr<Subscription<T>>;
```

### Parameter Passing

**Pass by const reference** for read-only complex types:
```cpp
void subscribe(const TopicPtr<TMessage>& topic);
void publish(const TMessage& message);
```

**Pass by value** for:
- Small POD types (int, bool, enum)
- When taking ownership and modifying
```cpp
void setQueueSize(size_t size);
void executeCommand(TimerCommand<TTag> command);
```

**Forward references** for template perfect forwarding:
```cpp
template <typename TFn>
void process(TFn&& fn);
```

### const Correctness

**Mark read-only parameters** as `const&`:
```cpp
void publish(const TMessage& message);
```

**Mark non-mutating methods** as `const`:
```cpp
bool hasInputItems() const;
size_t size() const;
```

**Mark const member variables** when they never change after construction:
```cpp
class Subscription {
  const size_t max_queue_size_;
};
```

### auto Usage

**Use auto** for:
- Complex template return types
- Iterator types
- Lambda captures

```cpp
// Complex template return types
auto subscription = create_sub(topic);

// Iterators
for (auto it = container.begin(); it != container.end(); ++it) { }
// Or better: range-based for
for (const auto& item : container) { }
```

**Avoid auto** when type clarity is important:
```cpp
// Prefer explicit type for clarity
size_t count = getCount();           // Better than: auto count = getCount();
bool is_ready = checkReady();        // Better than: auto is_ready = checkReady();
```

### Internal Linkage in .cpp Files

**Use unnamed (anonymous) namespace** for internal helpers in `.cpp` files:
```cpp
// In asio_timer_core.cpp
namespace {

// Internal helper function
void logTimerEvent(const std::string& message) {
  // ...
}

}  // namespace

// Public functions from .hpp follow below
void AsioTimerCore::schedule_single_shot(...) { ... }
```

This prevents name collisions and keeps internal implementation details from leaking to other translation units.

## Architecture Overview

### Actor-Based Message Passing System

1. **Topic-Based Pub/Sub** (`topic.hpp`, `subscription.hpp`, `publisher.hpp`)
   - Topics are typed message channels (`Topic<TMessage>`)
   - Publishers send messages via `Publisher<TMessage>::publish()`
   - Actors subscribe with `Subscription<TMessage>` and pull messages via `tryTakeMessage()`
   - Thread-safe: multiple actors can publish/subscribe concurrently
   - Uses weak_ptr for automatic cleanup of dead subscribers

2. **Actor Base Class** (`actor.hpp`)
   - CRTP-based `Actor<Derived>` provides factory pattern and subscription lifecycle
   - Each actor has an Asio strand for serialized execution (no explicit locking needed)
   - Factory method: `Actor<T>::create(io, ...)` ensures proper shared_ptr initialization
   - Subscriptions are deferred until after construction (enables `shared_from_this()`)
   - Helper methods: `create_sub()` and `create_pub()` for subscription/publisher creation

3. **Message Processing Flow**
   - Topics notify actors via `asio::post(strand, processInputs())`
   - Actors implement `processInputs()` to pull and process queued messages
   - Only notifies when queue transitions from empty→non-empty (avoids spam)
   - Pull-based model: actors control when to process messages

4. **Timer System** (`timer/*.hpp`)
   - Generic timer interface with Asio backend implementation
   - Supports single-shot and periodic timers
   - Timers implement `InputSource<TElapsedEvent>` for unified processing
   - TimerFactory creates timers with proper strand binding

### Key Patterns

- **Factory Construction**: Always use `Actor<T>::create(io, ...)` instead of constructors directly
- **Strand Isolation**: Each actor runs on its own strand - no manual locking needed
- **Weak References**: Topics hold weak_ptr to actors for automatic cleanup
- **Deferred Initialization**: Subscriptions registered after construction to enable shared_from_this()
- **Pull-Based Processing**: Actors pull messages from subscriptions rather than push callbacks
- **InputSource Abstraction**: Unified interface for messages, timer events, and custom inputs

## Important Implementation Notes

### Actor Creation
Always use the factory pattern for actors:
```cpp
// CORRECT
auto actor = MyActor::create(io, topic1, topic2);

// WRONG - constructor is protected
auto actor = std::make_shared<MyActor>(io, topic1, topic2);
```

### Message Processing Pattern
Actors process messages by pulling from subscriptions:
```cpp
void MyActor::processInputs() {
  while (auto msg = my_sub_->tryTakeMessage()) {
    handleMessage(*msg);
  }
}
```

### Topic Lifecycle
- Topics should be created first and shared among actors
- Use `TopicPtr<Msg>` (shared_ptr) to pass topics around
- Topics are thread-safe - safe to publish from any thread

### Subscription vs Publisher
- **Subscription**: Owned by actor (value or shared_ptr), used to pull messages
- **Publisher**: Lightweight wrapper, used to send messages to topic
- Create with `create_sub()` and `create_pub()` helpers in Actor base class

### Timer Usage
```cpp
// Define timer types
struct MyTimerTag {};
using MyElapsedEvent = TimerElapsedEvent<MyTimerTag>;
using MyCommand = TimerCommand<MyTimerTag>;
using MyTimer = Timer<MyElapsedEvent, MyCommand>;

// In actor constructor
timer_ = create_timer<MyTimer>(timer_factory);

// Start periodic timer
timer_->execute_command(make_periodic_command<MyTimerTag>(std::chrono::seconds(1)));

// Process timer events
while (auto event = timer_->tryTakeElapsedEvent()) {
  handleTimerEvent(*event);
}
```

## Dependencies

- **Standalone Asio 1.30.2**: Bundled via CMake FetchContent
- **C++17**: Required (GCC 11+, Clang 14+)
- **CMake 3.12+**: Build system

## Library Usage

### As a CMake Subdirectory
```cmake
add_subdirectory(path/to/funkyactors)
target_link_libraries(your_target PRIVATE funkyactors)
```

### Via CMake FetchContent
```cmake
include(FetchContent)
FetchContent_Declare(
  funkyactors
  GIT_REPOSITORY https://github.com/yourusername/funkyactors.git
  GIT_TAG main
)
FetchContent_MakeAvailable(funkyactors)

target_link_libraries(your_target PRIVATE funkyactors)
```

## Best Practices

### For Library Maintainers
- **Keep headers clean**: Minimize includes in public headers
- **Use forward declarations** where possible to reduce compile-time dependencies
- **Document public APIs** with Doxygen comments
- **Maintain backward compatibility**: This is a library, breaking changes affect users
- **Test on multiple compilers**: CI tests GCC 11-14, Clang 14-18

### For Contributors
- **Follow the style guide**: Run clang-format before committing
- **Write examples**: Good examples in README.md are worth more than documentation
- **Consider the API surface**: Every public type/function is a commitment
- **Think about performance**: This library is for concurrent systems

## Thread Safety

Actor code runs serialized on its strand - no manual locking needed. Topics are thread-safe for publishing from any thread. Never call actor methods directly - always communicate via topics.
