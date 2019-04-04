/*
 * ksp_parser.h
 *
 *  Created on: Nov 28, 2018
 *      Author: noel
 */

#ifndef SRC_KSP_PARSER_H_
#define SRC_KSP_PARSER_H_

#include <vector>
#include <tinyxml2.h>

#include "definitions.h"

void parseKspData (tinyxml2::XMLNode* rootNode, linkContainer_t& links, pathContainer_t& paths, flowContainer_t& flows);

#endif /* SRC_KSP_PARSER_H_ */
