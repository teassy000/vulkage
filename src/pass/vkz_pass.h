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

