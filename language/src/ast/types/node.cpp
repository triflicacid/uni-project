#include "node.hpp"

static lang::ast::type::TypeId current_id = 0;

lang::ast::type::Node::Node(): id_(current_id++) {}
