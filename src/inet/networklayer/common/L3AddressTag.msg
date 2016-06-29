//
// This program is property of its copyright holder. All rights reserved.
//

cplusplus{{
#include "inet/common/TagBase_m.h"
#include "inet/networklayer/common/L3Address.h"
}}

namespace inet;

class noncobject L3Address;
class TagBase extends cObject;

class L3AddressTag extends TagBase
{
    L3Address source;
    L3Address destination;
}

class L3AddressReq extends L3AddressTag
{
}

class L3AddressInd extends L3AddressTag
{
}
