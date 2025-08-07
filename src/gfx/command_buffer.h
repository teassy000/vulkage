#pragma once

#include "core/kage_inner.h"
#include "bx/spscqueue.h"

namespace kage
{
    struct CommandBuffer
    {
        CommandBuffer()
            : m_buffer(nullptr)
            , m_pos(0)
            , m_size(0)
            , m_minCapacity(0)
        {
            resize();
            finish();
        }

        ~CommandBuffer()
        {
            bx::free(g_bxAllocator, m_buffer);
        }

        void init(uint32_t _minCapacity)
        {
            m_minCapacity = bx::alignUp(_minCapacity, 1024);
            resize();
        }

        void resize(uint32_t _capacity = 0)
        {
            m_capacity = bx::alignUp(bx::max(_capacity, m_minCapacity), 1024);
            m_buffer = (uint8_t*)bx::realloc(g_bxAllocator, m_buffer, m_capacity);
        }

        void write(const void* _data, uint32_t _size)
        {
            BX_ASSERT(m_size == 0, "Called write outside start/finish (m_size: %d)?", m_size);
            if (m_pos + _size > m_capacity)
            {
                resize(m_capacity + (16 << 10));
            }

            bx::memCopy(&m_buffer[m_pos], _data, _size);
            m_pos += _size;
        }

        template<typename Ty>
        uint32_t write(const Ty& _data)
        {
            uint32_t p = m_pos;
            align(BX_ALIGNOF(Ty));
            uint32_t pad = m_pos - p;
            write(reinterpret_cast<const uint8_t*>(&_data), sizeof(Ty));

            return pad;
        }

        const uint8_t* skip(uint32_t _size)
        {
            BX_ASSERT(m_pos + _size <= m_size
                , "CommandBuffer::skip error (pos: %d-%d, size: %d)."
                , m_pos
                , m_pos + _size
                , m_size
            );
            const uint8_t* result = &m_buffer[m_pos];
            m_pos += _size;
            return result;
        }

        template<typename Ty>
        void skip()
        {
            align(BX_ALIGNOF(Ty));
            skip(sizeof(Ty));
        }

        void align(uint32_t _alignment)
        {
            const uint32_t mask = _alignment - 1;
            const uint32_t pos = (m_pos + mask) & (~mask);
            m_pos = pos;
        }

        uint32_t getPos() const
        {
            return m_pos;
        }

        void reset()
        {
            m_pos = 0;
        }

        void start()
        {
            m_pos = 0;
            m_size = 0;
        }

        void finish()
        {
            m_size = m_pos;
            m_pos = 0;

            if (m_size < m_minCapacity
                && m_capacity != m_minCapacity)
            {
                resize();
            }
        }

        void actPos(uint32_t _pos)
        {
            BX_ASSERT(_pos <= m_size
                , "CommandBuffer::actPos error (pos: %d, m_size: %d)."
                , _pos
                , m_size
            );

            m_pos = _pos;
        }

        const uint8_t* seek(uint32_t _pos, uint32_t _size) const
        {
            BX_ASSERT(_pos + _size <= m_size
                , "CommandBuffer::setPos error (pos: %d, size: %d)."
                , _pos
                , m_size
            );
            const uint8_t* result = &m_buffer[_pos];
            return result;
        }

        uint8_t* m_buffer;
        uint32_t m_size;
        uint32_t m_pos;

        uint32_t m_capacity;
        uint32_t m_minCapacity;
    };

#define ENTRY_IMPLEMENT_COMMAND(_class, _type) \
            _class() : Command(_type) {}

    struct Command
    {
        enum Enum : uint16_t
        {
            cb_reserve,

            create_pass,
            create_image,
            create_buffer,
            create_program,
            create_shader,
            create_sampler,
            create_bindless,

            alias_image,
            alias_buffer,

            set_name,

            update_image,
            update_buffer,

            record,

            record_start,

            record_set_constants,
            record_push_descriptor_set,
            record_bind_descriptor_set,
            record_set_color_attachments,
            record_set_depth_attachment,
            record_set_bindless,
            record_set_viewport,
            record_set_scissor,

            record_set_vertex_buffer,
            record_set_index_buffer,

            record_blit,
            record_copy,
            record_fill_buffer,
            record_clear_attachment,

            record_dispatch,
            record_dispatch_indirect,
            record_draw,
            record_draw_indirect,
            record_draw_indirect_count,
            record_draw_indexed,
            record_draw_indexed_indirect,
            record_draw_indexed_indirect_count,
            record_draw_mesh_task,
            record_draw_mesh_task_indirect,
            record_draw_mesh_task_indirect_count,

            record_end,

            end,

            destroy_pass,
            destroy_image,
            destroy_buffer,
            destroy_program,
            destroy_shader,
            destroy_sampler,
        };

        Command(Enum _cmd)
            : m_cmd(_cmd)
        {
        }

        Enum m_cmd;
    };

    struct CreatePassCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(CreatePassCmd, Command::create_pass);
        PassHandle m_handle;
        PassDesc m_desc;
    };

    struct CreateImageCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(CreateImageCmd, Command::create_image);
        ImageHandle m_handle;
        ImageCreate m_desc;
    };

    struct CreateBufferCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(CreateBufferCmd, Command::create_buffer);
        BufferHandle m_handle;
        BufferDesc m_desc;
        const Memory* m_mem;
        ResourceLifetime m_lt;
    };

    struct CreateProgramCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(CreateProgramCmd, Command::create_program);
        ProgramHandle m_handle;
        uint16_t m_num;
        const Memory* m_mem;
        uint32_t m_sizeConstants;
        BindlessHandle m_bindless;
    };

    struct CreateShaderCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(CreateShaderCmd, Command::create_shader);
        ShaderHandle m_handle;
        String m_path;
    };

    struct CreateSamplerCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(CreateSamplerCmd, Command::create_sampler);
        SamplerHandle m_handle;
        SamplerDesc m_desc;
    };

    struct CreateBindlessResCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(CreateBindlessResCmd, Command::create_bindless);
        BindlessHandle   m_handle;
        BindlessDesc     m_desc;
    };

    struct AliasImageCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(AliasImageCmd, Command::alias_image);
        ImageHandle m_handle;
        ImageHandle m_src;
    };

    struct AliasBufferCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(AliasBufferCmd, Command::alias_buffer);
        BufferHandle m_handle;
        BufferHandle m_src;
    };

    struct SetNameCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(SetNameCmd, Command::set_name);
        Handle m_handle;
        char* m_name;
        int32_t m_len;
    };

    struct UpdateImageCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(UpdateImageCmd, Command::update_image);
        ImageHandle m_handle;
        uint32_t m_width;
        uint32_t m_height;
        uint32_t m_layers;
        const Memory* m_mem;
    };

    struct UpdateBufferCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(UpdateBufferCmd, Command::update_buffer);
        BufferHandle m_handle;
        const Memory* m_mem;
        uint32_t m_offset;
        uint32_t m_size;
    };

    struct RecordCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordCmd, Command::record);
        PassHandle m_pass;
    };

    struct RecordStartCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordStartCmd, Command::record_start);
        PassHandle m_pass;
    };

    struct RecordSetConstantsCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordSetConstantsCmd, Command::record_set_constants);
        const Memory* m_mem;
    };

    struct RecordPushDescriptorSetCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordPushDescriptorSetCmd, Command::record_push_descriptor_set);
        const Memory* m_mem;
    };

    struct RecordBindDescriptorSetCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordBindDescriptorSetCmd, Command::record_bind_descriptor_set);
        const Memory* m_binds;
        const Memory* m_counts;
    };

    struct RecordSetColorAttachmentsCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordSetColorAttachmentsCmd, Command::record_set_color_attachments);
        const Memory* m_mem;
    };

    struct RecordSetDepthAttachmentCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordSetDepthAttachmentCmd, Command::record_set_depth_attachment);
        Attachment m_depthAttachment;
    };

    struct RecordSetBindlessCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordSetBindlessCmd, Command::record_set_bindless);
        BindlessHandle m_bindless;
    };

    struct RecordSetViewportCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordSetViewportCmd, Command::record_set_viewport);
        uint32_t m_x;
        uint32_t m_y;
        uint32_t m_w;
        uint32_t m_h;
    };

    struct RecordSetScissorCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordSetScissorCmd, Command::record_set_scissor);
        uint32_t m_x;
        uint32_t m_y;
        uint32_t m_w;
        uint32_t m_h;
    };

    struct RecordSetVertexBufferCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordSetVertexBufferCmd, Command::record_set_vertex_buffer);
        BufferHandle m_buf;
    };

    struct RecordSetIndexBufferCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordSetIndexBufferCmd, Command::record_set_index_buffer);
        BufferHandle m_buf;
        uint32_t m_offset;
        IndexType m_type;
    };

    struct RecordBlitCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordBlitCmd, Command::record_blit);
        ImageHandle m_dst;
        ImageHandle m_src;
    };

    struct RecordCopyCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordCopyCmd, Command::record_copy);
        ImageHandle m_dst;
        BufferHandle m_src;
    };

    struct RecordFillBufferCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordFillBufferCmd, Command::record_fill_buffer);
        BufferHandle m_buf;
        uint32_t m_val;
    };

    struct RecordClearAttachmentsCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordClearAttachmentsCmd, Command::record_clear_attachment);
        const Memory* m_attachments;
    };

    struct RecordDispatchCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordDispatchCmd, Command::record_dispatch);
        uint32_t m_x;
        uint32_t m_y;
        uint32_t m_z;
    };

    struct RecordDispatchIndirectCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordDispatchIndirectCmd, Command::record_dispatch_indirect);
        BufferHandle m_buf;
        uint32_t m_off;
    };

    struct RecordDrawCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordDrawCmd, Command::record_draw);
        uint32_t m_vtxCnt;
        uint32_t m_instCnt;
        uint32_t m_1stVtx;
        uint32_t m_1stInst;
    };

    struct RecordDrawIndirectCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordDrawIndirectCmd, Command::record_draw_indirect);
        BufferHandle m_buf;
        uint32_t m_off;
        uint32_t m_cnt;
    };

    struct RecordDrawIndirectCountCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordDrawIndirectCountCmd, Command::record_draw_indirect_count);
        uint32_t m_off;
        BufferHandle m_cntBuf;
        uint32_t m_cntOff;
        uint32_t m_maxCnt;
        uint32_t m_stride;
    };

    struct RecordDrawIndexedCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordDrawIndexedCmd, Command::record_draw_indexed);
        uint32_t m_idxCnt;
        uint32_t m_instCnt;
        uint32_t m_1stIdx;
        int32_t m_vtxOff;
        uint32_t m_1stInst;
    };

    struct RecordDrawIndexedIndirectCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordDrawIndexedIndirectCmd, Command::record_draw_indexed_indirect);
        BufferHandle m_indirectBuf;
        uint32_t m_off;
        uint32_t m_cnt;
        uint32_t m_stride;
    };

    struct RecordDrawIndexedIndirectCountCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordDrawIndexedIndirectCountCmd, Command::record_draw_indexed_indirect_count);
        BufferHandle m_indirectBuf;
        uint32_t m_off;
        BufferHandle m_cntBuf;
        uint32_t m_cntOff;
        uint32_t m_maxCnt;
        uint32_t m_stride;
    };

    struct RecordDrawMeshTaskCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordDrawMeshTaskCmd, Command::record_draw_mesh_task);
        
        uint32_t m_x;
        uint32_t m_y;
        uint32_t m_z;
    };

    struct RecordDrawMeshTaskIndirectCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordDrawMeshTaskIndirectCmd, Command::record_draw_mesh_task_indirect);

        BufferHandle m_buf;
        uint32_t m_off;
        uint32_t m_cnt;
        uint32_t m_stride;
    };

    struct RecordDrawMeshTaskIndirectCountCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordDrawMeshTaskIndirectCountCmd, Command::record_draw_mesh_task_indirect_count);
        
        BufferHandle m_indirectBuf;
        uint32_t m_off;
        BufferHandle m_cntBuf;
        uint32_t m_cntOff;
        uint32_t m_maxCnt;
        uint32_t m_stride;
    };

    struct RecordEndCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(RecordEndCmd, Command::record_end);
    };

    struct EndCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(EndCmd, Command::end);
    };

    struct DestroyPassCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(DestroyPassCmd, Command::destroy_pass);
        PassHandle m_pass;
    };

    struct DestroyImageCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(DestroyImageCmd, Command::destroy_image);
        ImageHandle m_img;
    };

    struct DestroyBufferCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(DestroyBufferCmd, Command::destroy_buffer);
        BufferHandle m_buf;
    };

    struct DestroyProgramCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(DestroyProgramCmd, Command::destroy_program);
        ProgramHandle m_prog;
    };

    struct DestroyShaderCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(DestroyShaderCmd, Command::destroy_shader);
        ShaderHandle m_shader;
    };

    struct DestroySamplerCmd : public Command
    {
        ENTRY_IMPLEMENT_COMMAND(DestroySamplerCmd, Command::destroy_sampler);
        SamplerHandle m_sampler;
    };

    struct CommandQueue
    {
        void cmdCreatePass(PassHandle _handle, const PassDesc& _desc)
        {
            CreatePassCmd cmd;
            cmd.m_handle = _handle;
            cmd.m_desc = _desc;

            push(cmd);
        }

        void cmdCreateImage(ImageHandle _handle, const ImageCreate& _desc)
        {
            CreateImageCmd cmd;
            cmd.m_handle = _handle;
            cmd.m_desc = _desc;

            push(cmd);
        }

        void cmdCreateBuffer(BufferHandle _handle, const BufferDesc& _desc, const Memory* _mem = nullptr, ResourceLifetime _lt = ResourceLifetime::transition)
        {
            CreateBufferCmd cmd;
            cmd.m_handle = _handle;
            cmd.m_desc = _desc;
            cmd.m_mem = _mem;
            cmd.m_lt = _lt;

            push(cmd);
        }

        void cmdCreateProgram(ProgramHandle _handle, const Memory* _mem, const uint16_t _shaderNum, const uint32_t _sizePushConstants, BindlessHandle _bindless)
        {
            CreateProgramCmd cmd;
            cmd.m_handle = _handle;
            cmd.m_num = _shaderNum;
            cmd.m_mem = _mem;
            cmd.m_sizeConstants = _sizePushConstants;
            cmd.m_bindless = _bindless;

            push(cmd);
        }

        void cmdCreateShader(ShaderHandle _handle, const String& _path)
        {
            CreateShaderCmd cmd;
            cmd.m_handle = _handle;
            cmd.m_path = _path;

            push(cmd);
        }

        void cmdCreateSampler(SamplerHandle _handle, const SamplerDesc& _desc)
        {
            CreateSamplerCmd cmd;
            cmd.m_handle = _handle;
            cmd.m_desc = _desc;

            push(cmd);
        }

        void cmdCreateBindlessRes(BindlessHandle _handle, const BindlessDesc& _desc)
        {
            CreateBindlessResCmd cmd;
            cmd.m_handle = _handle;
            cmd.m_desc = _desc;

            push(cmd);
        }

        void cmdAliasImage(ImageHandle _handle, ImageHandle _src)
        {
            AliasImageCmd cmd;
            cmd.m_handle = _handle;
            cmd.m_src = _src;

            push(cmd);
        }

        void cmdAliasBuffer(BufferHandle _handle, BufferHandle _src)
        {
            AliasBufferCmd cmd;
            cmd.m_handle = _handle;
            cmd.m_src = _src;

            push(cmd);
        }

        void cmdSetName(Handle _handle, const char* _name, uint32_t _len)
        {
            SetNameCmd cmd;
            cmd.m_handle = _handle;
            cmd.m_len = _len;
            cmd.m_name = (char*)_name;

            push(cmd);
        }

        void cmdUpdateImage(ImageHandle _handle, uint32_t _width, uint32_t _height, uint32_t _layers, const Memory* _mem)
        {
            UpdateImageCmd cmd;
            cmd.m_handle = _handle;
            cmd.m_width = _width;
            cmd.m_height = _height;
            cmd.m_layers = _layers;
            cmd.m_mem = _mem;

            push(cmd);
        }

        void cmdUpdateBuffer(BufferHandle _handle, const Memory* _mem, uint32_t _offset = 0, uint32_t _size = 0)
        {
            UpdateBufferCmd cmd;
            cmd.m_handle = _handle;
            cmd.m_mem = _mem;
            cmd.m_offset = _offset;
            cmd.m_size = _size;

            push(cmd);
        }

        void cmdRecord(PassHandle _pass)
        {
            RecordCmd cmd;
            cmd.m_pass = _pass;

            push(cmd);
        }

        void cmdRecordStart(PassHandle _pass)
        {
            RecordStartCmd cmd;
            cmd.m_pass = _pass;

            push(cmd);
        }

        void cmdRecordSetConstants(const Memory* _mem)
        {
            RecordSetConstantsCmd cmd;
            cmd.m_mem = _mem;

            push(cmd);
        }

        void cmdRecordPushDescriptorSet(const Memory* _mem)
        {
            RecordPushDescriptorSetCmd cmd;
            cmd.m_mem = _mem;

            push(cmd);
        }

        void cmdRecordBindDescriptorSet(const Memory* _binds, const Memory* _counts)
        {
            RecordBindDescriptorSetCmd cmd;
            cmd.m_binds = _binds;
            cmd.m_counts = _counts;
            push(cmd);
        }

        void cmdRecordSetColorAttachments(const Memory* _mem)
        {
            RecordSetColorAttachmentsCmd cmd;
            cmd.m_mem = _mem;
            push(cmd);
        }

        void cmdRecordSetDepthAttachment(const Attachment _depthAtt)
        {
            RecordSetDepthAttachmentCmd cmd;
            cmd.m_depthAttachment = _depthAtt;
            push(cmd);
        }

        void cmdRecordSetBindless(BindlessHandle _bindless)
        {
            RecordSetBindlessCmd cmd;
            cmd.m_bindless = _bindless;
            push(cmd);
        }

        void cmdRecordSetViewport(uint32_t _x, uint32_t _y, uint32_t _w, uint32_t _h)
        {
            RecordSetViewportCmd cmd;
            cmd.m_x = _x;
            cmd.m_y = _y;
            cmd.m_w = _w;
            cmd.m_h = _h;

            push(cmd);
        }

        void cmdRecordSetScissor(uint32_t _x, uint32_t _y, uint32_t _w, uint32_t _h)
        {
            RecordSetScissorCmd cmd;
            cmd.m_x = _x;
            cmd.m_y = _y;
            cmd.m_w = _w;
            cmd.m_h = _h;

            push(cmd);
        }

        void cmdRecordSetVertexBuffer(BufferHandle _handle)
        {
            RecordSetVertexBufferCmd cmd;
            cmd.m_buf = _handle;

            push(cmd);
        }

        void cmdRecordSetIndexBuffer(BufferHandle _handle, uint32_t _offset, IndexType _type)
        {
            RecordSetIndexBufferCmd cmd;
            cmd.m_buf = _handle;
            cmd.m_offset = _offset;
            cmd.m_type = _type;

            push(cmd);
        }

        void cmdRecordBlit(ImageHandle _dst, ImageHandle _src)
        {
            RecordBlitCmd cmd;
            cmd.m_dst = _dst;
            cmd.m_src = _src;

            push(cmd);
        }

        void cmdRecordCopy(ImageHandle _dst, BufferHandle _src)
        {
            RecordCopyCmd cmd;
            cmd.m_dst = _dst;
            cmd.m_src = _src;

            push(cmd);
        }

        void cmdRecordFillBuffer(BufferHandle _handle, uint32_t _v)
        {
            RecordFillBufferCmd cmd;
            cmd.m_buf = _handle;
            cmd.m_val = _v;

            push(cmd);
        }

        void cmdRecordClearAttachments(const Memory* _handles)
        {
            RecordClearAttachmentsCmd cmd;
            cmd.m_attachments = _handles;
            push(cmd);
        }

        void cmdRecordDispatch(uint32_t _numX, uint32_t _numY, uint32_t _numZ)
        {
            RecordDispatchCmd cmd;
            cmd.m_x = _numX;
            cmd.m_y = _numY;
            cmd.m_z = _numZ;

            push(cmd);
        }

        void cmdRecordDispatchIndirect(BufferHandle _handle, uint32_t _offset)
        {
            RecordDispatchIndirectCmd cmd;
            cmd.m_buf = _handle;
            cmd.m_off = _offset;
            push(cmd);
        }

        void cmdRecordDraw(uint32_t _vertexCount, uint32_t _instanceCount, uint32_t _firstVertex, uint32_t _firstInstance)
        {
            RecordDrawCmd cmd;
            cmd.m_vtxCnt = _vertexCount;
            cmd.m_instCnt = _instanceCount;
            cmd.m_1stVtx = _firstVertex;
            cmd.m_1stInst = _firstInstance;

            push(cmd);
        }

        void cmdRecordDrawIndirect(BufferHandle _handle, uint32_t _offset, uint32_t _count)
        {
            RecordDrawIndirectCmd cmd;
            cmd.m_buf = _handle;
            cmd.m_off = _offset;
            cmd.m_cnt = _count;

            push(cmd);
        }

        void cmdRecordDrawIndirectCount(uint32_t _offset, BufferHandle _countBuf, uint32_t _countOffset, uint32_t _maxCount, uint32_t _stride)
        {
            RecordDrawIndirectCountCmd cmd;
            cmd.m_off = _offset;
            cmd.m_cntBuf = _countBuf;
            cmd.m_cntOff = _countOffset;
            cmd.m_maxCnt = _maxCount;
            cmd.m_stride = _stride;

            push(cmd);
        }

        void cmdRecordDrawIndexed(uint32_t _indexCount, uint32_t _instanceCount, uint32_t _firstIndex, int32_t _vertexOffset, uint32_t _firstInstance)
        {
            RecordDrawIndexedCmd cmd;
            cmd.m_idxCnt = _indexCount;
            cmd.m_instCnt = _instanceCount;
            cmd.m_1stIdx = _firstIndex;
            cmd.m_vtxOff = _vertexOffset;
            cmd.m_1stInst = _firstInstance;

            push(cmd);
        }

        void cmdRecordDrawIndexedIndirect(BufferHandle _handle, uint32_t _offset, uint32_t _count, uint32_t _stride)
        {
            RecordDrawIndexedIndirectCmd cmd;
            cmd.m_indirectBuf = _handle;
            cmd.m_off = _offset;
            cmd.m_cnt = _count;
            cmd.m_stride = _stride;

            push(cmd);
        }

        void cmdRecordDrawIndexedIndirectCount(BufferHandle _handle, uint32_t _offset, BufferHandle _countBuf, uint32_t _countOffset, uint32_t _maxCount, uint32_t _stride)
        {
            RecordDrawIndexedIndirectCountCmd cmd;
            cmd.m_indirectBuf = _handle;
            cmd.m_off = _offset;
            cmd.m_cntBuf = _countBuf;
            cmd.m_cntOff = _countOffset;
            cmd.m_maxCnt = _maxCount;
            cmd.m_stride = _stride;

            push(cmd);
        }

        void cmdRecordDrawMeshTask(uint32_t _x, uint32_t _y, uint32_t _z)
        {
            RecordDrawMeshTaskCmd cmd;
            cmd.m_x = _x;
            cmd.m_y = _y;
            cmd.m_z = _z;

            push(cmd);
        }

        void cmdRecordDrawMeshTaskIndirect(BufferHandle _hIndirectBuf, uint32_t _offset, uint32_t _count, uint32_t _stride)
        {
            RecordDrawMeshTaskIndirectCmd cmd;
            cmd.m_buf = _hIndirectBuf;
            cmd.m_off = _offset;
            cmd.m_cnt = _count;
            cmd.m_stride = _stride;

            push(cmd);
        }

        void cmdRecordDrawMeshTaskIndirectCount(BufferHandle _hIndirectBuf, uint32_t _offset, BufferHandle _countBuf, uint32_t _countOffset, uint32_t _maxCount, uint32_t _stride)
        {
            RecordDrawMeshTaskIndirectCountCmd cmd;
            cmd.m_indirectBuf = _hIndirectBuf;
            cmd.m_off = _offset;
            cmd.m_cntBuf = _countBuf;
            cmd.m_cntOff = _countOffset;
            cmd.m_maxCnt = _maxCount;
            cmd.m_stride = _stride;

            push(cmd);
        }

        void cmdRecordEnd()
        {
            RecordEndCmd cmd;

            push(cmd);
        }

        void cmdEnd()
        {
            EndCmd cmd;

            push(cmd);
        }

        void cmdDestroyPass(PassHandle _handle)
        {
            DestroyPassCmd cmd;
            cmd.m_pass = _handle;

            push(cmd);
        }

        void cmdDestroyImage(ImageHandle _handle)
        {
            DestroyImageCmd cmd;
            cmd.m_img = _handle;

            push(cmd);
        }

        void cmdDestroyBuffer(BufferHandle _handle)
        {
            DestroyBufferCmd cmd;
            cmd.m_buf = _handle;

            push(cmd);
        }

        void cmdDestroyProgram(ProgramHandle _handle)
        {
            DestroyProgramCmd cmd;
            cmd.m_prog = _handle;

            push(cmd);
        }

        void cmdDestroyShader(ShaderHandle _handle)
        {
            DestroyShaderCmd cmd;
            cmd.m_shader = _handle;

            push(cmd);
        }

        void cmdDestroySampler(SamplerHandle _handle)
        {
            DestroySamplerCmd cmd;
            cmd.m_sampler = _handle;

            push(cmd);
        }

        // class ops
        CommandQueue()
            : m_cmdIdx(0)
            , m_cb()
        {
            finish();
            m_cps.clear();
        }

        CommandQueue(const CommandQueue&) = delete;

        ~CommandQueue()
        {
            m_cps.clear();
        }

        void push(const CommandQueue& _other, uint32_t _offIdx, uint32_t _count)
        {
            BX_ASSERT( 
                ((0 < _offIdx) && (_offIdx + _count - 1 < (uint32_t)_other.m_cps.size()))
                , "CommandQueue::push error (offIdx: %d, count: %d, other count: %d)." 
                , _offIdx
                , _count
                , _other.m_cps.size()
            );

            const CBPos* root = _other.m_cps.data();
            const CBPos* first = root + _offIdx;
            const CBPos* last = root + _offIdx + _count - 1;

            uint32_t size = last->m_end - first->m_start;
            uint32_t offPos = first->m_start - m_cb.getPos();


            
            // fill cb 
            const uint8_t* data = _other.m_cb.seek(first->m_start, size);
            m_cb.write(data, size);

            // fill cps
            for (uint32_t ii = 0; ii < _count; ++ii)
            {
                CBPos cp = *(first+ii); 
                cp.m_start = first->m_start - offPos;
                cp.m_end = first->m_end - offPos;

                m_cps.emplace_back(cp);
            }
        }

        const Command* poll()
        {
            const Command* cmd = nullptr;
            if (m_cmdIdx < m_cps.size())
            {
                CBPos cp;
                cp = m_cps[m_cmdIdx];

                uint32_t offset = m_cb.getPos();

                m_cb.skip(cp.m_pad);
                cmd = reinterpret_cast<const Command*>(m_cb.skip(cp.m_size));

                uint32_t end = m_cb.getPos();

                BX_ASSERT(end - offset == cp.m_size + cp.m_pad, "read offset mismatched");
                m_cmdIdx++;
            }

            return cmd;
        }

        void init(uint32_t _minCapacity)
        {
            m_cb.init(_minCapacity);
            
            m_cps.clear();
            m_cps.reserve(1024);
            m_cmdIdx = 0;
        }

        void start()
        {
            m_cb.start();

            m_cps.clear();
            m_cps.reserve(1024);

            m_cmdIdx = 0;
        }

        void reset()
        {
            m_cb.reset();
            m_cmdIdx = 0;
        }

        void finish()
        {
            m_cb.finish();

            m_cmdIdx = 0;
        }

        uint32_t getIdx() const
        {
            return (uint32_t)m_cps.size();
        }

        void setIdx(uint32_t _idx)
        {
            BX_ASSERT(_idx < (uint32_t)m_cps.size(), "setIdx out of range!");

            m_cmdIdx = _idx;

            CBPos cp = m_cps[m_cmdIdx];
            m_cb.actPos(cp.m_start);
        }

    private:
        template<typename Ty>
        void push(const Ty& _obj)
        {
            CBPos cmd;
            cmd.m_start = m_cb.getPos();
            cmd.m_pad = m_cb.write(_obj);
            cmd.m_end = m_cb.getPos();
            cmd.m_size = sizeof(Ty);

            m_cps.emplace_back(cmd);
        }

        struct CBPos
        {
            uint32_t m_size{ 0 };
            uint32_t m_start{ 0 };
            uint32_t m_end{ 0 };
            uint32_t m_pad{ 0 };
        };

        uint32_t m_cmdIdx;
        CommandBuffer m_cb;
        stl::vector<CBPos> m_cps;
    };
}
