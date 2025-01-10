#include "boolean.hpp"
#include "either.hpp"

namespace ftxui {

/// @brief Display `true` or `false` depending an a Boolean flag.
/// @param flag the Boolean flag.
Element boolean(bool flag) {
  return flag
    ? text("true") | color(Color::GreenLight)
    : text("false") | color(Color::Red);
}

/// @brief Display `true` or `false` depending an the returned Boolean.
/// @param flag a function returning the Boolean flag.
/// @ingroup component
Component Boolean(std::function<bool()> flag) {
    class Impl : public ComponentBase {
    public:
      explicit Impl(std::function<bool()> flag) : flag_(std::move(flag)) {}

    private:
      Element Render() override {
        return flag_() ? true_ : false_;
      }

      std::function<bool()> flag_;
      Element false_ = boolean(false);
      Element true_ = boolean(true);
    };

    return Make<Impl>(std::move(flag));
}

/// @brief Display `true` or `false` depending an a Boolean flag.
/// @param flag reference to the Boolean flag.
/// @ingroup component
Component Boolean(const bool* flag) {
  return Boolean([flag] { return *flag; });
}

}
