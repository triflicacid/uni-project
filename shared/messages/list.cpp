#include "list.hpp"

bool message::List::has_message_of(Level level) const {
  return std::any_of(messages.begin(), messages.end(), [&level](const auto& message) {
    return message->get_level() == level;
  });
}

void message::List::for_each_message(const std::function<void(BasicMessage&)>& func) const {
  for (auto& message: messages) {
    func(*message);
  }
}

void message::List::for_each_message(const std::function<void(BasicMessage&)>& func, Level min_level) const {
  for (auto& message: messages) {
    if (message->get_level() >= min_level) {
      func(*message);
    }
  }
}

void message::List::add(std::unique_ptr<BasicMessage> message) {
  messages.push_back(std::move(message));
}

void message::List::add(List& other) {
  for (auto& msg: other.messages) {
    messages.push_back(std::move(msg));
  }

  other.clear();
}

bool message::print_and_check(List& list, std::ostream& os) {
  list.for_each_message([&](BasicMessage& msg) {
    msg.print(os);
  });

  bool is_error = list.has_message_of(Level::Error);

  list.clear();

  return is_error;
}
