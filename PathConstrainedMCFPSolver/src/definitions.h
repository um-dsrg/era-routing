/*
 * definitions.h
 *
 *  Created on: Nov 26, 2018
 *      Author: noel
 */

#ifndef SRC_DEFINITIONS_H_
#define SRC_DEFINITIONS_H_

#include <cstdint>
#include <map>
#include <vector>

#include "link.h"
#include "flow_details.h"

typedef uint32_t id_t;
typedef std::map<id_t, Link> linkContainer_t;
typedef std::vector<Flow> flowContainer_t;
typedef std::vector<Path> pathContainer_t;

#endif /* SRC_DEFINITIONS_H_ */
