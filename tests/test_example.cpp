#include <gtest/gtest.h>

#include <asio.hpp>
#include <atomic>
#include <chrono>
#include <thread>

#include "funkyactors/actor.hpp"
#include "funkyactors/timer/timer.hpp"
#include "funkyactors/timer/timer_factory.hpp"
#include "funkyactors/topic.hpp"

using namespace funkyactors;

// Message definitions
struct Request {
  int id;
};

struct Response {
  int id;
  std::string result;
};

// Timer types for producer
struct ProducerTag {};
using ProducerCommand = TimerCommand<ProducerTag>;
using ProducerElapsed = TimerElapsedEvent<ProducerTag>;
using ProducerTimer = Timer<ProducerElapsed, ProducerCommand>;

// Producer actor (timer-driven)
class Producer : public Actor<Producer> {
 public:
  Producer(ActorContext ctx, TopicPtr<Request> request_topic, TimerFactoryPtr timer_factory)
      : Actor(ctx), request_pub_(create_pub(request_topic)), timer_(create_timer<ProducerTimer>(timer_factory)) {
    // Start periodic timer (100ms intervals for faster test)
    timer_->execute_command(make_periodic_command<ProducerTag>(std::chrono::milliseconds(100)));
  }

  void processInputs() override {
    // Process timer ticks
    while (auto event = timer_->tryTakeElapsedEvent()) {
      produce();
    }
  }

  int getNextId() const { return next_id_; }

 private:
  void produce() { request_pub_->publish(Request{next_id_++}); }

  PublisherPtr<Request> request_pub_;
  std::shared_ptr<ProducerTimer> timer_;
  int next_id_ = 0;
};

// Worker actor
class Worker : public Actor<Worker> {
 public:
  Worker(ActorContext ctx, TopicPtr<Request> request_topic, TopicPtr<Response> response_topic)
      : Actor(ctx), request_sub_(create_sub(request_topic)), response_pub_(create_pub(response_topic)) {}

  void processInputs() override {
    while (auto msg = request_sub_->tryTakeMessage()) {
      // Process request
      std::string result = "Processed " + std::to_string(msg->id);
      response_pub_->publish(Response{msg->id, result});
      processed_count_++;
    }
  }

  int getProcessedCount() const { return processed_count_; }

 private:
  SubscriptionPtr<Request> request_sub_;
  PublisherPtr<Response> response_pub_;
  int processed_count_ = 0;
};

// Collector actor to verify responses
class Collector : public Actor<Collector> {
 public:
  Collector(ActorContext ctx, TopicPtr<Response> response_topic)
      : Actor(ctx), response_sub_(create_sub(response_topic)) {}

  void processInputs() override {
    while (auto msg = response_sub_->tryTakeMessage()) {
      responses_.push_back(*msg);
    }
  }

  const std::vector<Response>& getResponses() const { return responses_; }

 private:
  SubscriptionPtr<Response> response_sub_;
  std::vector<Response> responses_;
};

// Test the producer-consumer example from README
TEST(ActorTest, ProducerConsumerExample) {
  asio::io_context io;

  // Create shared resources
  auto request_topic = std::make_shared<Topic<Request>>();
  auto response_topic = std::make_shared<Topic<Response>>();
  auto timer_factory = std::make_shared<TimerFactory>(io);

  // Create actors
  auto producer = Producer::create(io, request_topic, timer_factory);
  auto worker = Worker::create(io, request_topic, response_topic);
  auto collector = Collector::create(io, response_topic);

  // Run event loop in background thread
  std::atomic<bool> running{true};
  std::thread io_thread([&io, &running]() {
    while (running) {
      io.run_for(std::chrono::milliseconds(10));
    }
  });

  // Let it run for a bit to process some messages
  std::this_thread::sleep_for(std::chrono::milliseconds(350));

  // Stop the event loop
  running = false;
  io_thread.join();

  // Verify that messages were produced and consumed
  EXPECT_GT(producer->getNextId(), 0) << "Producer should have produced messages";
  EXPECT_GT(worker->getProcessedCount(), 0) << "Worker should have processed messages";
  EXPECT_GT(collector->getResponses().size(), 0) << "Collector should have received responses";

  // Verify response format
  if (!collector->getResponses().empty()) {
    const auto& first_response = collector->getResponses()[0];
    EXPECT_EQ(first_response.result, "Processed " + std::to_string(first_response.id));
  }
}

// Test basic topic pub/sub
TEST(ActorTest, TopicPubSub) {
  asio::io_context io;

  struct TestMsg {
    int value;
  };

  auto topic = std::make_shared<Topic<TestMsg>>();
  auto pub = std::make_shared<Publisher<TestMsg>>(topic);

  // Simple subscriber actor
  class TestActor : public Actor<TestActor> {
   public:
    TestActor(ActorContext ctx, TopicPtr<TestMsg> msg_topic) : Actor(ctx), sub_(create_sub(msg_topic)) {}

    void processInputs() override {
      while (auto msg = sub_->tryTakeMessage()) {
        messages_.push_back(msg->value);
      }
    }

    const std::vector<int>& getMessages() const { return messages_; }

   private:
    SubscriptionPtr<TestMsg> sub_;
    std::vector<int> messages_;
  };

  auto actor = TestActor::create(io, topic);

  // Publish some messages
  pub->publish(TestMsg{1});
  pub->publish(TestMsg{2});
  pub->publish(TestMsg{3});

  // Process messages
  io.run();

  // Verify
  const auto& messages = actor->getMessages();
  ASSERT_EQ(messages.size(), 3);
  EXPECT_EQ(messages[0], 1);
  EXPECT_EQ(messages[1], 2);
  EXPECT_EQ(messages[2], 3);
}
