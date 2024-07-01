#pragma once

#ifdef TRACY_ENABLE

#include <tracy/Tracy.hpp>
#include "colors.h"

#include "volk.h"
#include <tracy/TracyVulkan.hpp>

#define KG_ProfCtxType tracy::VkCtx

#define KG_ProfVkContext(_ctx, _phyDevice, _device, _queue, _cmdBuf) do{ \
            _ctx = TracyVkContext(_phyDevice, _device, _queue, _cmdBuf); \
        } while (0)

#define KG_ProfDestroyContext(_ctx) do{ \
            TracyVkDestroy(_ctx); \
        } while (0)

#define KG_VkZone(_ctx, _cmdBuf, _name) TracyVkZone(_ctx, _cmdBuf, _name) 
#define KG_VkZoneC(_ctx, _cmdBuf, _name, _color) TracyVkZoneC(_ctx, _cmdBuf, _name, _color) 
#define KG_VkZoneTransient(_ctx, _var, _cmdBuf, _name) TracyVkZoneTransient(_ctx, _var, _cmdBuf, _name, true)

#define KG_VkCollect(_ctx, _cmdBuf) TracyVkCollect(_ctx, _cmdBuf)

#define KG_ProfAlloc(_ptr, _size) TracyAlloc(_ptr, _size)
#define KG_ProfFree(_ptr) TracyFree(_ptr)

#define KG_FrameMark FrameMark

#define KG_ZoneScoped ZoneScoped
#define KG_ZoneScopedC(_color) ZoneScopedC(_color)
#define KG_ZoneScopedNC(_name, _color) ZoneScopedNC(_name, _color)


#else // ! TRACY_ENABLE

#define KG_ProfCtxType void

#define KG_ProfVkContext(c, x, y, z, w)

#define KG_ProfDestroyContext(c)

#define KG_VkZone(c, x, y) 
#define KG_VkZoneC(c, x, y, z)
#define KG_VkZoneTransient(c, x, y, z)

#define KG_VkCollect(c, x)

#define KG_ProfAlloc(x, y) 
#define KG_ProfFree(x) 

#define KG_FrameMark

#define KG_ZoneScoped
#define KG_ZoneScopedC(x)
#define KG_ZoneScopedNC(x, y)

#endif // TRACY_ENABLE
