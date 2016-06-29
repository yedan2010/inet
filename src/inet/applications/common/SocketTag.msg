//
// This program is property of its copyright holder. All rights reserved.
//

cplusplus{{
#include "inet/common/TagBase_m.h"
}}

namespace inet;

class TagBase extends cObject;

class SocketTag extends TagBase
{
    int socketId;
}

class SocketReq extends SocketTag
{
}

class SocketInd extends SocketTag
{
}
