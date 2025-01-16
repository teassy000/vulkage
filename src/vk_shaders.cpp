#include "common.h"
#include "vk_shaders.h"
#include <stdio.h>

#if VK_HEADER_VERSION >= 135
#include <spirv-headers/spirv.h>
#else
#include <vulkan/spirv.h>
#endif
#include "macro.h"


namespace kage { namespace vk
{
    // https://www.khronos.org/registry/spir-v/specs/1.0/SPIRV.pdf
    struct Id
    {
        uint32_t opcode;
        uint32_t typeId;
        uint32_t storageClass;
        uint32_t binding;
        uint32_t set;
        uint32_t constant;
    };


    static VkShaderStageFlagBits getShaderStage(SpvExecutionModel executionModel)
    {
        switch (executionModel)
        {
        case SpvExecutionModelVertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case SpvExecutionModelFragment:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case SpvExecutionModelGLCompute:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        case SpvExecutionModelTaskEXT:
            return VK_SHADER_STAGE_TASK_BIT_EXT;
        case SpvExecutionModelMeshEXT:
            return VK_SHADER_STAGE_MESH_BIT_EXT;

        default:
            assert(!"Unsupported execution model");
            return VkShaderStageFlagBits(0);
        }
    }

    static VkDescriptorType getDescriptorType(SpvOp op)
    {
        switch (op)
        {
        case SpvOpTypeStruct:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case SpvOpTypeImage:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case SpvOpTypeSampler:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
        case SpvOpTypeSampledImage:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        default:
            assert(!"Unknown resource type");
            return VkDescriptorType(0);
        }
    }

    static VkDescriptorType getStorageDescriptorType(SpvStorageClass _sc)
    {
        switch (_sc)
        {
        case SpvStorageClassUniform:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case SpvStorageClassUniformConstant:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case SpvStorageClassStorageBuffer:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        default:
            assert(!"Unknown resource type");
            return VkDescriptorType(0);
        }
    }


    static void parseShader(Shader_vk& shader, const uint32_t* code, uint32_t codeSize)
    {
        assert(code[0] == SpvMagicNumber);

        uint32_t idBound = code[3];

        stl::vector<Id> ids(idBound);

        int localSizeIdX = -1;
        int localSizeIdY = -1;
        int localSizeIdZ = -1;

        const uint32_t* insn = code + 5;

        while (insn != code + codeSize)
        {
            uint16_t opcode = uint16_t(insn[0]);
            uint16_t wordCount = uint16_t(insn[0] >> 16);

            switch (opcode)
            {
            case SpvOpEntryPoint:
            {
                assert(wordCount >= 2);
                shader.stage = getShaderStage(SpvExecutionModel(insn[1]));
            } break;
            case SpvOpExecutionMode:
            {
                assert(wordCount >= 3);
                uint32_t mode = insn[2];

                switch (mode)
                {
                case SpvExecutionModeLocalSize:
                    assert(wordCount == 6);
                    shader.localSizeX = insn[3];
                    shader.localSizeY = insn[4];
                    shader.localSizeZ = insn[5];
                    break;
                }
            } break;
            case SpvOpExecutionModeId:
            {
                assert(wordCount >= 3);
                uint32_t mode = insn[2];

                switch (mode)
                {
                case SpvExecutionModeLocalSizeId:
                    assert(wordCount == 6);
                    localSizeIdX = int(insn[3]);
                    localSizeIdY = int(insn[4]);
                    localSizeIdZ = int(insn[5]);
                    break;
                }
            } break;
            case SpvOpDecorate:
            {
                assert(wordCount >= 3);

                uint32_t id = insn[1];
                assert(id < idBound);

                switch (insn[2])
                {
                case SpvDecorationDescriptorSet:
                    assert(wordCount == 4);
                    ids[id].set = insn[3];
                    break;
                case SpvDecorationBinding:
                    assert(wordCount == 4);
                    ids[id].binding = insn[3];
                    break;
                }
            } break;
            case SpvOpTypeStruct:
            case SpvOpTypeImage:
            case SpvOpTypeSampler:
            case SpvOpTypeSampledImage:
            {
                assert(wordCount >= 2);

                uint32_t id = insn[1];
                assert(id < idBound);

                assert(ids[id].opcode == 0);
                ids[id].opcode = opcode;
            } break;
            case SpvOpTypePointer:
            {
                assert(wordCount == 4);

                uint32_t id = insn[1];
                assert(id < idBound);

                assert(ids[id].opcode == 0);
                ids[id].opcode = opcode;
                ids[id].typeId = insn[3];
                ids[id].storageClass = insn[2];
            } break;
            case SpvOpConstant:
            {
                assert(wordCount >= 4); // we currently only correctly handle 32-bit integer constants

                uint32_t id = insn[2];
                assert(id < idBound);

                assert(ids[id].opcode == 0);
                ids[id].opcode = opcode;
                ids[id].typeId = insn[1];
                ids[id].constant = insn[3]; // note: this is the value, not the id of the constant
            } break;
            case SpvOpVariable:
            {
                assert(wordCount >= 4);

                uint32_t id = insn[2];
                assert(id < idBound);

                assert(ids[id].opcode == 0);
                ids[id].opcode = opcode;
                ids[id].typeId = insn[1];
                ids[id].storageClass = insn[3];
            } break;
            }

            assert(insn + wordCount <= code + codeSize);
            insn += wordCount;
        }

        for (auto& id : ids)
        {
            // reserve set 0 for push descriptors
            if (id.opcode == SpvOpVariable && (id.storageClass == SpvStorageClassUniform || id.storageClass == SpvStorageClassUniformConstant || id.storageClass == SpvStorageClassStorageBuffer) && id.set == 0)
            {
                assert(id.binding < 32);
                assert(ids[id.typeId].opcode == SpvOpTypePointer);

                uint32_t typeKind = ids[ids[id.typeId].typeId].opcode;
                VkDescriptorType resourceType = getDescriptorType(SpvOp(typeKind));

                if (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER == resourceType)
                {
                    resourceType = getStorageDescriptorType((SpvStorageClass)id.storageClass);
                }

                assert((shader.resourceMask & (1 << id.binding)) == 0 || shader.resourceTypes[id.binding] == resourceType);

                shader.resourceTypes[id.binding] = resourceType;
                shader.resourceMask |= 1 << id.binding;
            }

            // set 1 for bind-less descriptor array
            if (id.opcode == SpvOpVariable && id.storageClass == SpvStorageClassUniformConstant && id.set == 1)
            {
                shader.usesBindless = true;
            }

            if (id.opcode == SpvOpVariable && id.storageClass == SpvStorageClassPushConstant)
            {
                shader.usesPushConstants = true;
            }
        }

        if (shader.stage == VK_SHADER_STAGE_COMPUTE_BIT)
        {
            if (localSizeIdX >= 0)
            {
                assert(ids[localSizeIdX].opcode == SpvOpConstant);
                shader.localSizeX = ids[localSizeIdX].constant;
            }

            if (localSizeIdY >= 0)
            {
                assert(ids[localSizeIdY].opcode == SpvOpConstant);
                shader.localSizeY = ids[localSizeIdY].constant;
            }

            if (localSizeIdZ >= 0)
            {
                assert(ids[localSizeIdZ].opcode == SpvOpConstant);
                shader.localSizeZ = ids[localSizeIdZ].constant;
            }

            assert(shader.localSizeX && shader.localSizeY && shader.localSizeZ);
        }
    }


    bool loadShader(Shader_vk& shader, VkDevice device, const char* path)
    {
        FILE* file = fopen(path, "rb");
        if (!file)
            return false;

        fseek(file, 0, SEEK_END);
        long length = ftell(file);
        assert(length >= 0);
        fseek(file, 0, SEEK_SET);

        char* buffer = new char[length];
        assert(buffer);

        size_t rc = fread(buffer, 1, length, file);
        assert(rc = size_t(length));
        fclose(file);

        assert(length % 4 == 0);

        VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer);
        createInfo.codeSize = length;

        VkShaderModule shaderModule = 0;
        VK_CHECK(vkCreateShaderModule(device, &createInfo, 0, &shaderModule));

        parseShader(shader, reinterpret_cast<const uint32_t*>(buffer), length / 4);

        delete[] buffer;

        shader.module = shaderModule;
        return true;
    }

    static VkSpecializationInfo fillSpecializationInfo(stl::vector<VkSpecializationMapEntry>& entries, const stl::vector<int>& constants)
    {
        for (size_t i = 0; i < constants.size(); ++i)
            entries.push_back({ uint32_t(i), uint32_t(i * 4), 4 });

        VkSpecializationInfo result = {};
        result.mapEntryCount = uint32_t(entries.size());
        result.pMapEntries = entries.data();
        result.dataSize = constants.size() * sizeof(int);
        result.pData = constants.data();

        return result;
    }

    VkPipeline createGraphicsPipeline(VkDevice device, VkPipelineCache pipelineCache, VkPipelineLayout layout, const VkPipelineRenderingCreateInfo& renderInfo
        , const stl::vector<Shader_vk>& shaders, VkPipelineVertexInputStateCreateInfo* vtxInputState, const stl::vector<int> constants /*= {}*/, const PipelineConfigs_vk& pipeConfigs /* = {}*/)
    {
        stl::vector<VkSpecializationMapEntry> specializationEntries;
        VkSpecializationInfo specializationInfo = fillSpecializationInfo(specializationEntries, constants);

        VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

        stl::vector<VkPipelineShaderStageCreateInfo> stages;
        for (const Shader_vk& shader : shaders)
        {
            VkPipelineShaderStageCreateInfo stage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
            stage.module = shader.module;
            stage.stage = shader.stage;
            stage.pName = "main";
            stage.pSpecializationInfo = &specializationInfo;

            stages.push_back(stage);
        }

        createInfo.stageCount = uint32_t(stages.size());
        createInfo.pStages = stages.data();

        VkPipelineVertexInputStateCreateInfo defaultVertexInput = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        createInfo.pVertexInputState = (vtxInputState != nullptr) ? vtxInputState : &defaultVertexInput;

        VkPipelineInputAssemblyStateCreateInfo inputAssemply = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        inputAssemply.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        createInfo.pInputAssemblyState = &inputAssemply;

        VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;
        createInfo.pViewportState = &viewportState;

        VkPipelineRasterizationStateCreateInfo rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rasterizationState.lineWidth = 1.f;
        rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizationState.cullMode = pipeConfigs.cullMode;
        createInfo.pRasterizationState = &rasterizationState;

        VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        createInfo.pMultisampleState = &multisampleState;

        VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        depthStencilState.depthTestEnable = pipeConfigs.enableDepthTest;
        depthStencilState.depthWriteEnable = pipeConfigs.enableDepthWrite;
        depthStencilState.depthCompareOp = pipeConfigs.depthCompOp; // all UI element rendered to (z = 1.0);
        depthStencilState.depthBoundsTestEnable = false;
        depthStencilState.minDepthBounds = 0.f;
        depthStencilState.maxDepthBounds = 1.f;
        depthStencilState.stencilTestEnable = false;
        createInfo.pDepthStencilState = &depthStencilState;


        stl::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(renderInfo.colorAttachmentCount);
        for (uint32_t i = 0; i < renderInfo.colorAttachmentCount; ++i)
        {
            VkPipelineColorBlendAttachmentState& colorBlendAttachment = colorBlendAttachments[i];
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = true;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        }

        VkPipelineColorBlendStateCreateInfo colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        colorBlendState.attachmentCount = renderInfo.colorAttachmentCount;
        colorBlendState.pAttachments = colorBlendAttachments.data();
        createInfo.pColorBlendState = &colorBlendState;

        VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

        VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamicState.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
        dynamicState.pDynamicStates = dynamicStates;
        createInfo.pDynamicState = &dynamicState;

        createInfo.layout = layout;
        createInfo.pNext = &renderInfo;

        VkPipeline pipeline = 0;
        VK_CHECK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &createInfo, 0, &pipeline));

        return pipeline;
    }

    VkPipeline createComputePipeline(VkDevice device, VkPipelineCache pipelineCache, VkPipelineLayout layout, const Shader_vk& shader, const stl::vector<int> constants/* = {}*/)
    {
        assert(shader.stage == VK_SHADER_STAGE_COMPUTE_BIT);

        stl::vector<VkSpecializationMapEntry> specializationEntries;
        VkSpecializationInfo specializationInfo = fillSpecializationInfo(specializationEntries, constants);

        VkComputePipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };

        VkPipelineShaderStageCreateInfo stage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        stage.module = shader.module;
        stage.stage = shader.stage;
        stage.pName = "main";
        stage.pSpecializationInfo = &specializationInfo;

        createInfo.stage = stage;
        createInfo.layout = layout;

        VkPipeline pipeline = 0;
        VK_CHECK(vkCreateComputePipelines(device, pipelineCache, 1, &createInfo, 0, &pipeline));

        return pipeline;
    }

    static uint32_t gatherResources(const stl::vector<Shader_vk>& shaders, VkDescriptorType(&resourceTypes)[32])
    {
        uint32_t resourceMask = 0;

        for (const Shader_vk& shader : shaders)
        {
            for (uint32_t i = 0; i < 32; ++i)
            {
                if (shader.resourceMask & (1 << i))
                {
                    if (resourceMask & (1 << i))
                    {
                        assert(resourceTypes[i] == shader.resourceTypes[i]);
                    }
                    else
                    {
                        resourceTypes[i] = shader.resourceTypes[i];
                        resourceMask |= 1 << i;
                    }
                }
            }
        }

        return resourceMask;
    }

    static VkDescriptorUpdateTemplate createDescriptorTemplates(VkDevice device, VkPipelineBindPoint bindingPoint, VkPipelineLayout layout, VkDescriptorSetLayout setLayout, const stl::vector<Shader_vk>& shaders)
    {
        stl::vector<VkDescriptorUpdateTemplateEntry> entries;

        VkDescriptorType resourceTypes[32] = {};
        uint32_t resourceMask = gatherResources(shaders, resourceTypes);

        for (uint32_t i = 0; i < 32; ++i)
        {
            if (resourceMask & (1 << i))
            {
                VkDescriptorUpdateTemplateEntry entry = {};
                entry.dstBinding = i;
                entry.dstArrayElement = 0;
                entry.descriptorCount = 1;
                entry.descriptorType = resourceTypes[i];
                entry.offset = sizeof(DescriptorInfo) * i;
                entry.stride = sizeof(DescriptorInfo);

                entries.push_back(entry);
            }
        }

        VkDescriptorUpdateTemplateCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO };
        createInfo.descriptorUpdateEntryCount = uint32_t(entries.size());
        createInfo.pDescriptorUpdateEntries = entries.data();
        createInfo.templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR;
        createInfo.descriptorSetLayout = setLayout;
        createInfo.pipelineBindPoint = bindingPoint;
        createInfo.pipelineLayout = layout;

        VkDescriptorUpdateTemplate updateTemplate = 0;
        VK_CHECK(vkCreateDescriptorUpdateTemplate(device, &createInfo, 0, &updateTemplate));

        return updateTemplate;
    }


    kage::vk::Program_vk createProgram(VkDevice _device, VkPipelineBindPoint _bindingPoint, const stl::vector<Shader_vk>& _shaders, uint32_t _pushConstantSize /*= 0*/, VkDescriptorSetLayout _arrayLayout /*= 0*/)
    {
        VkShaderStageFlags pushConstantStages = 0;
        for (const Shader_vk& shader : _shaders)
            if (shader.usesPushConstants)
                pushConstantStages |= shader.stage;

        bool useBindless = false;
        VkShaderStageFlags useBindlessStages = 0;
        for (const Shader_vk& shader : _shaders) {
            useBindless |= shader.usesBindless;
            useBindlessStages |= shader.stage;
        }

        assert(!useBindless || _arrayLayout); // TODO: create array layout for shaders that use bind-less

        Program_vk program = {};

        program.setLayout = createDescSetLayout(_device, _shaders);
        assert(program.setLayout);

        program.layout = createPipelineLayout(_device, program.setLayout, _arrayLayout, pushConstantStages, _pushConstantSize);
        assert(program.layout);

        program.updateTemplate = createDescriptorTemplates(_device, _bindingPoint, program.layout, program.setLayout, _shaders);
        assert(program.updateTemplate);

        program.arrayLayout = _arrayLayout;
        program.pushConstantStages = pushConstantStages;
        program.bindPoint = _bindingPoint;
        program.pushConstantSize = _pushConstantSize;

        return program;
    }

    void destroyProgram(VkDevice _device, const Program_vk& _program)
    {
        vkDestroyDescriptorUpdateTemplate(_device, _program.updateTemplate, 0);
        vkDestroyPipelineLayout(_device, _program.layout, 0);
        vkDestroyDescriptorSetLayout(_device, _program.setLayout, 0);

    }

    VkDescriptorSetLayout createDescSetLayout(VkDevice device, const stl::vector<Shader_vk>& shaders)
    {
        stl::vector<VkDescriptorSetLayoutBinding> setBindings;

        VkDescriptorType resourceTypes[32] = {};
        uint32_t resourceMask = gatherResources(shaders, resourceTypes);

        for (uint32_t i = 0; i < 32; ++i)
        {
            if (resourceMask & (1 << i))
            {
                VkDescriptorSetLayoutBinding binding = {};
                binding.binding = i;

                binding.descriptorCount = 1;
                binding.descriptorType = resourceTypes[i];
                binding.stageFlags = 0;
                for (const Shader_vk& shader : shaders)
                    if (shader.resourceMask & (1 << i))
                        binding.stageFlags |= shader.stage;

                setBindings.push_back(binding);
            }
        }

        VkDescriptorSetLayoutCreateInfo setCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        setCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
        setCreateInfo.bindingCount = uint32_t(setBindings.size());
        setCreateInfo.pBindings = setBindings.data();

        VkDescriptorSetLayout setLayout = 0;
        VK_CHECK(vkCreateDescriptorSetLayout(device, &setCreateInfo, 0, &setLayout));

        return setLayout;
    }

    VkPipelineLayout createPipelineLayout(VkDevice _device, VkDescriptorSetLayout _setLayout, VkDescriptorSetLayout _arrayLayout, VkShaderStageFlags _pushConstantStages, size_t _pushConstantSize)
    {
        const VkDescriptorSetLayout layouts[2] = { _setLayout, _arrayLayout };

        VkPipelineLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        createInfo.setLayoutCount = _arrayLayout ? 2 : 1;
        createInfo.pSetLayouts = &layouts[0];

        VkPushConstantRange pushConstantRange = {};
        if (_pushConstantSize)
        {
            pushConstantRange.stageFlags = _pushConstantStages;
            pushConstantRange.size = uint32_t(_pushConstantSize);

            createInfo.pushConstantRangeCount = 1;
            createInfo.pPushConstantRanges = &pushConstantRange;
        }

        VkPipelineLayout layout = 0;
        VK_CHECK(vkCreatePipelineLayout(_device, &createInfo, 0, &layout));

        return layout;
    }


    VkDescriptorPool createDescriptorPool(VkDevice _device)
    {
        VkDescriptorPoolSize poolSizes[] =
        {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kMaxNumOfSamplerDescSets},
        };
         
        VkDescriptorPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolCreateInfo.maxSets = 1;
        poolCreateInfo.poolSizeCount = COUNTOF(poolSizes);
        poolCreateInfo.pPoolSizes = poolSizes;
         
        VkDescriptorPool pool = nullptr;
        VK_CHECK(vkCreateDescriptorPool(_device, &poolCreateInfo, 0, &pool));
         
        return pool;
    }

    VkDescriptorSetLayout createDescArrayLayout(VkDevice _device, VkShaderStageFlags _stages)
    {
        VkDescriptorSetLayoutBinding setBinding = {};
        setBinding.binding = 0;
        setBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        setBinding.descriptorCount = kMaxDescriptorCount;
        setBinding.stageFlags = _stages;
        setBinding.pImmutableSamplers = nullptr;
         
        VkDescriptorBindingFlags bindingFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |    VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
        VkDescriptorSetLayoutBindingFlagsCreateInfo setBindingFlags =  { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
        setBindingFlags.bindingCount = 1;
        setBindingFlags.pBindingFlags = &bindingFlags;
         
        VkDescriptorSetLayoutCreateInfo setLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        setLayoutInfo.pNext = &setBindingFlags;
        setLayoutInfo.bindingCount = 1;
        setLayoutInfo.pBindings = &setBinding;
         
        VkDescriptorSetLayout setLayout = 0;
        VK_CHECK(vkCreateDescriptorSetLayout(_device, &setLayoutInfo, 0, &setLayout));
         
        return setLayout;
    }

    VkDescriptorSet createDescriptorSets(VkDevice _device, VkDescriptorSetLayout _layout , VkDescriptorPool _pool, uint32_t _descCount)
    {
        VkDescriptorSetVariableDescriptorCountAllocateInfo setAllocateCountInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO };
        setAllocateCountInfo.descriptorSetCount = 1;
        setAllocateCountInfo.pDescriptorCounts = &_descCount;

        VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.pNext = &setAllocateCountInfo;
        allocInfo.descriptorPool = _pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &_layout;
        
        VkDescriptorSet set = nullptr;
        VK_CHECK(vkAllocateDescriptorSets(_device, &allocInfo, &set));
        return set;
    }


} // namespace vk
} // namespace kage