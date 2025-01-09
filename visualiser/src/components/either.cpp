#include "either.hpp"

#include <ftxui/component/event.hpp>

namespace ftxui {

/// @brief Display one child or another based on a Boolean flag.
/// @param child_f the component to decorate if flag=false.
/// @param child_t the component to decorate if flag=true.
/// @param flag a function returning the Boolean flag.
/// @ingroup component
Component Either(Component child_f, Component child_t, std::function<bool()> flag) {
    class Impl : public ComponentBase {
    public:
      explicit Impl(std::function<bool()> flag) : flag_(std::move(flag)) {}

    private:
      Component Get() const { return children_[flag_()]; }

      Element Render() override {
        return Get()->Render();
      }
      bool Focusable() const override {
        return Get()->Focusable();
      }
      bool OnEvent(Event event) override {
        return Get()->OnEvent(event);
      }

      std::function<bool()> flag_;
    };

    auto either = Make<Impl>(std::move(flag));
    either->Add(std::move(child_f));
    either->Add(std::move(child_t));
    return either;
}

/// @brief Display one child or another based on a Boolean flag.
/// @param child_f the component to decorate if flag=false.
/// @param child_t the component to decorate if flag=true.
/// @param show reference to the Boolean flag.
/// @ingroup component
Component Either(Component child_f, Component child_t, const bool* flag) {
  return Either(std::move(child_f), std::move(child_t), [flag] { return *flag; });
}

}
