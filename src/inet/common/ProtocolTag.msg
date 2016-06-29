//
// This program is property of its copyright holder. All rights reserved.
//

cplusplus{{
#include "inet/common/Protocol.h"
#include "inet/common/TagBase_m.h"
}}

namespace inet;

cplusplus{{
typedef const Protocol * ProtocolPtr;
}}

class noncobject ProtocolPtr;
class TagBase extends cObject;

class ProtocolTag extends TagBase
{
    ProtocolPtr protocol = nullptr;
}

class ProtocolReq extends ProtocolTag
{
}

class ProtocolInd extends ProtocolTag
{
}
