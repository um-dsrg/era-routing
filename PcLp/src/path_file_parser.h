#ifndef PATH_FILE_PARSER_H
#define PATH_FILE_PARSER_H

#include <vector>
#include <tinyxml2.h>

#include "definitions.h"

void parsePathFile (tinyxml2::XMLNode *rootNode, linkContainer_t &links, pathContainer_t &paths,
                    flowContainer_t &flows);

#endif /* PATH_FILE_PARSER_H */
