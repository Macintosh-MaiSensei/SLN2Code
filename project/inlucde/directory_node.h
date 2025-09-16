#pragma once

#include <string>
#include <vector>

class DirectoryNode {
public:
    std::string name;
    std::vector<DirectoryNode> children;
};