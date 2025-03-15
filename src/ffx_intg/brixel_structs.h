#pragma once

struct BrixelBufDescs
{
    kage::BufferHandle buf;
    uint32_t size;
    uint32_t stride;
};

struct UserResHandle
{
    union
    {
        kage::BufferHandle buf;
        kage::ImageHandle img;
    };

    UserResHandle(kage::BufferHandle _buf)
    {
        buf = _buf;
    }

    UserResHandle(kage::ImageHandle _img)
    {
        img = _img;
    }
};
