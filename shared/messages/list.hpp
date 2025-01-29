#pragma once

#include <functional>
#include <optional>

#include "message.hpp"

namespace message {
  class List {
    std::vector<std::unique_ptr<BasicMessage>> messages;

  public:
    [[nodiscard]] size_t size() const { return messages.size(); }

    void clear() { messages.clear(); }

    void add(std::unique_ptr<BasicMessage> message);

    [[nodiscard]] bool has_message_of(Level level) const;

    /** Go through each message, calling the given function on it **/
    void for_each_message(const std::function<void(BasicMessage&)>& func) const;

    /** Go through each message, calling the given function on it. Only include messages which meet the minimum level. **/
    void for_each_message(const std::function<void(BasicMessage&)>& func, Level min_level) const;

    /** Merge given list into this. The given list is emptied. */
    void add(List& other);
  };

  /** Handle message list: debug_print messages and empty the list, return if there was an error. */
  bool print_and_check(List& list, std::ostream& os);
}
