#ifndef __PROFILER_H__
#define __PROFILER_H__

#ifdef TRACY_ENABLE

#include <tracy/Tracy.hpp>

#define VKZ_FrameMark FrameMark

#define VKZ_ProfAlloc(_ptr, _size) TracyAlloc(_ptr, _size)
#define VKZ_ProfFree(_ptr) TracyFree(_ptr)

#else // ! TRACY_ENABLE

#define VKZ_FrameMark

#define VKZ_ProfAlloc(_ptr, _size) 
#define VKZ_ProfFree(_ptr) 

#endif // TRACY_ENABLE


#endif // __PROFILER_H__
