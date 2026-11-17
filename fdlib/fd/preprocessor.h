#pragma once

#define FD_ENUM_DECLARE(name, value) name = value,

#define FD_ENUM_TO_STRING(name, value) \
    case Type::name: return #name;

#define FD_ENUM_STRUCT(name, type, list)                \
    enum struct name : type                             \
    {                                                   \
        list(FD_ENUM_DECLARE)                           \
    };                                                  \
                                                        \
    constexpr const char* toString(name value)          \
    {                                                   \
        using Type = name;                              \
        switch (value)                                  \
        {                                               \
            list(FD_ENUM_TO_STRING)                     \
            default: return "Unknown";                  \
        }                                               \
    }


