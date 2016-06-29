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
    ProtocolPtr protocol @tostring("->info()") = nullptr; // specifies the protocol (internal class with an artificial identifier)
}

//
// Determines the destination protocol of the message/packet (within a network node)
// when it's sent from one protocol to another.
//
class ProtocolReq extends ProtocolTag
{
}

//
// Specifies the source protocol of the message/packet (within a network node)
// when it's sent from one protocol to another.
//
class ProtocolInd extends ProtocolTag
{
}
