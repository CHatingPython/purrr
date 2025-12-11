#ifndef _PURRR_EXCEPTIONS_HPP_
#define _PURRR_EXCEPTIONS_HPP_

#include <exception>
#include <string>

namespace purrr {

using namespace std::string_literals;

class Unreachable : std::exception {
public:
  virtual const char *what() const noexcept override { return "Unreachable"; }
};

class InvalidUse : std::exception {
public:
  InvalidUse()
    : mMessage("Invalid use") {}

  InvalidUse(const char *what)
    : mMessage("Invalid use"s + what) {}

  virtual const char *what() const noexcept override { return mMessage.c_str(); }
private:
  std::string mMessage;
};

} // namespace purrr

#endif // _PURRR_EXCEPTIONS_HPP_