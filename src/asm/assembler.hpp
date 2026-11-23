#pragma once

#include <string>
#include <vector>
#include "../memory.hpp"
#include "token.hpp"

bool assemble(const Token* head, std::vector<Byte>& out, std::string& error);