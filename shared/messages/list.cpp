#include "list.hpp"

namespace message {
    bool List::has_message_of(Level level) const {
        return std::any_of(messages.begin(), messages.end(), [&level](const auto &message) {
            return message->get_level() == level;
        });
    }

    void List::for_each_message(const std::function<void(Message &)> &func) const {
        for (auto &message: messages) {
            func(*message);
        }
    }

    void List::for_each_message(const std::function<void(Message &)> &func, Level min_level) const {
        for (auto &message: messages) {
            if (message->get_level() >= min_level) {
                func(*message);
            }
        }
    }

    void List::add(std::unique_ptr<Message> message) {
        messages.push_back(std::move(message));
    }

    void List::add(List &other) {
        for (auto &msg : other.messages) {
            messages.push_back(std::move(msg));
        }

        other.clear();
    }

    bool print_and_check(List &list, std::ostream &os) {
        list.for_each_message([&](Message &msg) {
            msg.print(os);
        });

        bool is_error = list.has_message_of(Level::Error);

        list.clear();

        return is_error;
    }
}
