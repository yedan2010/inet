//
// This program is property of its copyright holder. All rights reserved.
//

cplusplus{{
#include "inet/common/TagBase_m.h"
#include "inet/linklayer/common/MACAddress.h"
}}

namespace inet;

class noncobject MACAddress;
class TagBase extends cObject;

class MACAddressTag extends TagBase
{
    MACAddress source;
    MACAddress destination;
}

class MACAddressReq extends MACAddressTag
{
}

class MACAddressInd extends MACAddressTag
{
}
