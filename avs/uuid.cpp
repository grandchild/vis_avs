#include "uuid.h"

#ifdef _WIN32
#include <rpc.h>
#elif defined __linux__
#include <uuid/uuid.h>
#endif

std::string uuid4() {
#ifdef _WIN32
    RPC_CSTR uuid_str;
    UUID uuid;
    UuidCreate(&uuid);
    UuidToString(&uuid, &uuid_str);
    std::string result((char*)uuid_str);
    RpcStringFree(&uuid_str);
    return result;
#elif defined __linux__
    char uuid_str[37];
    uuid_str[36] = '\0';
    uuid_t uuid;
    uuid_generate(uuid);
    uuid_unparse_lower(uuid, uuid_str);
    return uuid_str;
#endif
}
