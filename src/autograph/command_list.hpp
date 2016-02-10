#ifndef COMMAND_LIST_HPP
#define COMMAND_LIST_HPP

#include "texture.hpp"

namespace ag {

template <typename D> class CommandList {
public:
  CommandList(D& backend_, typename D::CommandListHandle handle_)
  {}

private:
};
}

#endif // !COMMAND_LIST_HPP
