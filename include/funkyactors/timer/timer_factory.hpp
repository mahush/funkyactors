#ifndef FUNKYACTORS_TIMER_TIMER_FACTORY_HPP
#define FUNKYACTORS_TIMER_TIMER_FACTORY_HPP

#include <memory>

#include <asio.hpp>

#include "funkyactors/timer/asio_timer_core.hpp"
#include "funkyactors/timer/timer.hpp"
#include "funkyactors/timer/timer_core.hpp"

namespace funkyactors {

/**
 * @brief Factory class for creating timers with Asio backend
 *
 * Holds reference to io_context. The strand is passed per-timer
 * since each actor has its own strand.
 */
class TimerFactory {
 public:
  /**
   * @brief Construct a new Timer Factory
   * @param io The io_context for async operations
   */
  explicit TimerFactory(asio::io_context& io) : io_(io) {}

  /**
   * @brief Create a timer with the specified type
   * @tparam TTimer Timer type (Timer<ElapsedEvent, Command>)
   * @param strand The strand for this timer (typically the actor's strand)
   * @return Shared pointer to the created timer
   */
  template <typename TTimer>
  std::shared_ptr<TTimer> create(asio::strand<asio::io_context::executor_type>& strand) {
    auto timer_core = std::make_shared<AsioTimerCore>(io_, strand);
    return std::make_shared<TTimer>(timer_core);
  }

 private:
  asio::io_context& io_;
};

using TimerFactoryPtr = std::shared_ptr<TimerFactory>;

}  // namespace funkyactors

#endif  // FUNKYACTORS_TIMER_TIMER_FACTORY_HPP
