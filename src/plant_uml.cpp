// class to generate plant uml for call graph
// Date: 2021/05/10

#include "plant_uml.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cassert>
#include <regex>
#include <cctype>
#include <functional>
#include <memory>
#include <utility>
#include <stdexcept>
#include <type_traits>
#include <tuple>
#include <optional>
#include <variant>
#include <typeinfo>
#include <typeindex>
#include <type_traits>

using namespace std;
class PlantUML {
public:
    PlantUML() = default;
    ~PlantUML() = default;
    void generate(const std::string& file_name, const std::vector<std::string>& call_graph) {
        std::ofstream ofs(file_name);
        ofs << "@startuml" << std::endl;
        for (const auto& str : call_graph) {
            ofs << str << std::endl;
        }
        ofs << "@enduml" << std::endl;
    }
};
