#pragma once
#include "traits.h"

#define LIST_COUNT_ENTRY(...) + 1
#define LIST_COUNT(list) (0 list(LIST_COUNT_ENTRY))
#define FD_ENUM_DECLARE(name, value) name = value,
#define FD_ENUM_VALUE(name, value) value,
#define FD_ENUM_TO_STRING(name, value) case Type::name: return #name;


#define FD_ENUM_TRAITS(name, type, list)                         \
    template<> struct fd::EnumOf<name>                           \
    {                                                            \
        using Type = name;                                       \
        static constexpr bool value = true;                      \
        static constexpr type fields[] = {list(FD_ENUM_VALUE)};  \
        static constexpr size_t count = sizeof(fields) / sizeof(fields[0]); \
        static constexpr const char *toString(Type v){ switch(v) { list(FD_ENUM_TO_STRING) default: return "Unknown"; } } \
    };

#define FD_ENUM_STRUCT(name, type, list) enum struct name : type { list(FD_ENUM_DECLARE) };

