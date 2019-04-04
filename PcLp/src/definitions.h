/*
 * definitions.h
 *
 *  Created on: Nov 26, 2018
 *      Author: noel
 */

#ifndef SRC_DEFINITIONS_H_
#define SRC_DEFINITIONS_H_

#include <cstdint>
#include <memory>
#include <map>
#include <vector>

#include "link.h"
#include "path.h"
#include "flow.h"

typedef uint32_t id_t;

typedef std::map<id_t, std::unique_ptr<Link>> linkContainer_t;
typedef std::vector<std::unique_ptr<Path>> pathContainer_t;

class Flow; // Forward declaration
typedef std::vector<std::unique_ptr<Flow>> flowContainer_t;

#endif /* SRC_DEFINITIONS_H_ */
