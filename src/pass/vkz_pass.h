#pragma once

#include "core/kage.h"
#include "core/kage_math.h"
#include "demo_structs.h"

using BufUsage      = kage::BufferUsageFlagBits::Enum;
using Stage         = kage::PipelineStageFlagBits::Enum;
using Access        = kage::AccessFlagBits::Enum;
using BindingAccess = kage::BindingAccess;
using LoadOp        = kage::AttachmentLoadOp;
using StoreOp       = kage::AttachmentStoreOp;
using Aspect        = kage::ImageAspectFlagBits::Enum;
using ImgUsage      = kage::ImageUsageFlagBits::Enum;


// get pass name based on culling stage and pass type
void inline getPassName(std::string& _out, const char* _baseName, const RenderStage _stage = RenderStage::count, const RenderPipeline _pass = RenderPipeline::count)
{
    _out = _baseName;

    const char* stageStr[static_cast<uint8_t>(RenderStage::count)] = { "_early", "_late", "_alpha" };
    const char* passStr[static_cast<uint8_t>(RenderPipeline::count)] = { "_normal", "_task", "_compute" };

    if (_stage != RenderStage::count)
        _out += stageStr[static_cast<uint8_t>(_stage)];

    if (_pass != RenderPipeline::count)
        _out += passStr[static_cast<uint8_t>(_pass)];
}