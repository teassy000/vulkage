#pragma once

template <typename T, size_t Size>
char(*countof_helper(T(&_Array)[Size]))[Size];

#define COUNTOF(array) (sizeof(*countof_helper(array)) + 0)

#define NO_VTABLE __declspec(novtable)


#define KAGE_DELETE_ARRAY(ptr) \
    do { \
            if (ptr != nullptr) { \
                delete[] ptr; \
                ptr = nullptr; \
            } \
    } while (false)


#ifdef _DEBUG

#define KAGE_DEBUG 1

#else

#define KAGE_DEBUG 0

#endif //_DEBUG

