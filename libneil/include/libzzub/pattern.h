#pragma once

#include <vector>
#include <string>

namespace zzub {

struct pattern {
    typedef std::vector<int> column;
    typedef std::vector<column> track;
    typedef std::vector<track> group;

    std::vector<group> groups;
    std::string name;
    int rows;

    pattern() {
        rows = 0;
    }
};

}