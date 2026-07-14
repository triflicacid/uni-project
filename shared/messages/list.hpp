#pragma once

#include <functional>
#include <optional>

#include "message.hpp"

namespace message {
  /** @brief An ordered collection of diagnostic messages, accumulated during a compilation stage and printed together. */
  class List {
    std::vector<std::unique_ptr<BasicMessage>> messages;

  public:
    /** @brief Get the number of messages in the list. @return Message count. */
    [[nodiscard]] size_t size() const { return messages.size(); }

    /** @brief Remove all messages. */
    void clear() { messages.clear(); }

    /**
     * @brief Add a message to the list.
     * @param message Message to add; ownership is transferred to the list.
     */
    void add(std::unique_ptr<BasicMessage> message);

    /**
     * @brief Check whether the list contains a message of a given severity.
     * @param level Severity to check for.
     * @return True if at least one message has this severity.
     */
    [[nodiscard]] bool has_message_of(Level level) const;

    /**
     * @brief Call a function on every message in the list.
     * @param func Called with each message, in insertion order.
     */
    void for_each_message(const std::function<void(BasicMessage&)>& func) const;

    /**
     * @brief Call a function on every message meeting a minimum severity.
     * @param func Called with each qualifying message, in insertion order.
     * @param min_level Minimum severity a message must have to be included.
     */
    void for_each_message(const std::function<void(BasicMessage&)>& func, Level min_level) const;

    /**
     * @brief Move all messages from another list into this one.
     * @param other List to merge in; left empty afterwards.
     */
    void add(List& other);
  };

  /**
   * @brief Print every message in a list to a stream, then clear the list.
   * @param list List of messages to print and clear.
   * @param os Stream to print to.
   * @return True if the list contained an error-level message.
   */
  bool print_and_check(List& list, std::ostream& os);
}
