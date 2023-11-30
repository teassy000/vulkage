#pragma once

template <typename T, size_t Size>
char(*countof_helper(T(&_Array)[Size]))[Size];

#define COUNTOF(array) (sizeof(*countof_helper(array)) + 0)
#define NO_VTABLE __declspec(novtable)
#define VKZ_DELETE(ptr) \
    do { \
            if (ptr != nullptr) { \
                delete ptr; \
                ptr = nullptr; \
            } \
    } while (false)

#define VKZ_DELETE_ARRAY(ptr) \
    do { \
            if (ptr != nullptr) { \
                delete[] ptr; \
                ptr = nullptr; \
            } \
    } while (false)