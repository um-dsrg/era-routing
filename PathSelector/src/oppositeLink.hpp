#include <map>

#include "definitions.hpp"
#include "boost-graph.hpp"

std::map<id_t, id_t> GenerateOppositeLinkMap (const std::string &lgfPath,
                                              const BoostGraph &boostGraph);
