#ifndef FUNKYACTORS_PUBLISHER_HPP
#define FUNKYACTORS_PUBLISHER_HPP

#include <memory>

namespace funkyactors {

template <typename TMessage>
class Topic;

template <typename TMessage>
using TopicPtr = std::shared_ptr<Topic<TMessage>>;

// Publisher interface for sending messages to a topic
// Provides symmetric API to Subscription
template <typename TMessage>
class Publisher {
 public:
  explicit Publisher(TopicPtr<TMessage> topic) : topic_(topic) {}

  // Publish a message to the topic
  // Takes by value to support both copy (lvalue) and move (rvalue) semantics
  void publish(TMessage msg) { topic_->publish(std::move(msg)); }

 private:
  TopicPtr<TMessage> topic_;
};

// Convenience type alias for shared_ptr<Publisher<TMessage>>
template <typename TMessage>
using PublisherPtr = std::shared_ptr<Publisher<TMessage>>;

}  // namespace funkyactors

#endif  // FUNKYACTORS_PUBLISHER_HPP
