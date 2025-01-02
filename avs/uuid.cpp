#include "uuid.h"

#ifdef _WIN32
#include <rpc.h>
#elif defined __linux__
#include <uuid/uuid.h>
#endif

std::string uuid4() {
    char uuid_str[37];
    uuid_str[36] = '\0';
#ifdef _WIN32
    UUID uuid;
    UuidCreate(&uuid);
    UuidToString(&uuid, (unsigned char**)&uuid_str);
#elif defined __linux__
    uuid_t uuid;
    uuid_generate(uuid);
    uuid_unparse_lower(uuid, uuid_str);
#endif
    return uuid_str;
}
