#pragma once

#ifdef TRACY_ENABLE

#include <tracy/Tracy.hpp>
#include "colors.h"

#include "volk.h"
#include <tracy/TracyVulkan.hpp>

#define VKZ_ProfCtxType tracy::VkCtx

#define VKZ_ProfVkContext(_ctx, _phyDevice, _device, _queue, _cmdBuf) do{ \
            _ctx = TracyVkContext(_phyDevice, _device, _queue, _cmdBuf); \
        } while (0)

#define VKZ_ProfDestroyContext(_ctx) do{ \
            TracyVkDestroy(_ctx); \
        } while (0)

#define VKZ_VkZone(_ctx, _cmdBuf, _name) TracyVkZone(_ctx, _cmdBuf, _name) 
#define VKZ_VkZoneC(_ctx, _cmdBuf, _name, _color) TracyVkZoneC(_ctx, _cmdBuf, _name, _color) 
#define VKZ_VkZoneTransient(_ctx, _var, _cmdBuf, _name) TracyVkZoneTransient(_ctx, _var, _cmdBuf, _name, true)

#define VKZ_VkCollect(_ctx, _cmdBuf) TracyVkCollect(_ctx, _cmdBuf)

#define VKZ_ProfAlloc(_ptr, _size) TracyAlloc(_ptr, _size)
#define VKZ_ProfFree(_ptr) TracyFree(_ptr)

#define VKZ_FrameMark FrameMark

#define VKZ_ZoneScoped ZoneScoped
#define VKZ_ZoneScopedC(_color) ZoneScopedC(_color)

#else // ! TRACY_ENABLE

#define VKZ_ProfCtxType void

#define VKZ_ProfVkContext(c, x, y, z, w)

#define VKZ_ProfDestroyContext(c)

#define VKZ_VkZone(c, x, y) 
#define VKZ_VkZoneC(c, x, y, z)
#define VKZ_VkZoneTransient(c, x, y, z)

#define VKZ_VkCollect(c, x)

#define VKZ_ProfAlloc(x, y) 
#define VKZ_ProfFree(x) 

#define VKZ_FrameMark

#define VKZ_ZoneScoped
#define VKZ_ZoneScopedC(x)

#endif // TRACY_ENABLE
