//
// This program is property of its copyright holder. All rights reserved.
// 

cplusplus{{
#include "inet/common/TagBase_m.h"
}}

namespace inet;

class TagBase extends cObject;

class InterfaceTag extends TagBase
{
    int interfaceId = -1;
}

class InterfaceReq extends InterfaceTag
{
}

class InterfaceInd extends InterfaceTag
{
}
