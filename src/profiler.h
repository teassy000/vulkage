#ifndef __PROFILER_H__
#define __PROFILER_H__

#ifdef TRACY_ENABLE

#include <tracy/Tracy.hpp>


#define VKZ_ProfAlloc(_ptr, _size) TracyAlloc(_ptr, _size)
#define VKZ_ProfFree(_ptr) TracyFree(_ptr)

#define VKZ_FrameMark FrameMark

#define VKZ_ZoneScoped ZoneScoped
#define VKZ_ZoneScopedC(_color) ZoneScopedC(_color)



#else // ! TRACY_ENABLE

#define VKZ_ProfAlloc(x, y) 
#define VKZ_ProfFree(x) 

#define VKZ_FrameMark

#define VKZ_ZoneScoped
#define VKZ_ZoneScopedC(x)

#endif // TRACY_ENABLE


#endif // __PROFILER_H__
