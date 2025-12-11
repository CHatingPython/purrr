#ifdef _PURRR_BACKEND_VULKAN

#include "purrr/exceptions.hpp"

#include "purrr/vulkan/exceptions.hpp"

#include "purrr/vulkan/program.hpp"
#include "purrr/vulkan/context.hpp"
#include "purrr/vulkan/renderTarget.hpp"

#include "purrr/vulkan/format.hpp"

#include <vector>
#include <array>

namespace purrr::vulkan {

VkShaderStageFlagBits vkShaderType(ShaderType type) {
  switch (type) {
  case ShaderType::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
  case ShaderType::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
  }

  throw Unreachable();
}

VkVertexInputRate vkVertexInputRate(VertexInputRate inputRate) {
  switch (inputRate) {
  case VertexInputRate::Vertex: return VK_VERTEX_INPUT_RATE_VERTEX;
  case VertexInputRate::Instance: return VK_VERTEX_INPUT_RATE_INSTANCE;
  }

  throw Unreachable();
}

VkPrimitiveTopology vkTopology(Topology topology) {
  switch (topology) {
  case Topology::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
  case Topology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  case Topology::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
  case Topology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  case Topology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
  }

  throw Unreachable();
}

VkCullModeFlagBits vkCullMode(CullMode cullMode) {
  switch (cullMode) {
  case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
  case CullMode::Back: return VK_CULL_MODE_BACK_BIT;
  case CullMode::Both: return VK_CULL_MODE_FRONT_AND_BACK;
  }

  throw Unreachable();
}

VkFrontFace vkFrontFace(FrontFace frontFace) {
  switch (frontFace) {
  case FrontFace::Clockwise: return VK_FRONT_FACE_CLOCKWISE;
  case FrontFace::CounterClockwise: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
  }

  throw Unreachable();
}

Shader::Shader(Context *context, const ShaderInfo &info)
  : mContext(context), mStage(vkShaderType(info.type)) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.pNext    = VK_NULL_HANDLE;
  createInfo.flags    = 0;
  createInfo.codeSize = info.codeLength;
  createInfo.pCode    = reinterpret_cast<const uint32_t *>(info.code);

  expectResult(
      "Shader module creation",
      vkCreateShaderModule(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mModule));
}

Shader::~Shader() {
  if (mModule) vkDestroyShaderModule(mContext->getDevice(), mModule, VK_NULL_HANDLE);
}

Program::Program(IRenderTarget *renderTarget, Context *context, const ProgramInfo &info)
  : mRenderTarget(renderTarget), mContext(context) {
  createLayout(info);
  createPipeline(info);
}

Program::~Program() {
  if (mLayout) vkDestroyPipelineLayout(mContext->getDevice(), mLayout, VK_NULL_HANDLE);
  if (mPipeline) vkDestroyPipeline(mContext->getDevice(), mPipeline, VK_NULL_HANDLE);
}

void Program::createLayout(const ProgramInfo &info) {
  std::vector<VkDescriptorSetLayout> layouts(info.slotCount);
  for (uint32_t i = 0; i < info.slotCount; ++i) {
    switch (info.slots[i]) {
    case ProgramSlot::Texture: {
      layouts[i] = mContext->getTextureDescriptorSetLayout();
    } break;
    case ProgramSlot::UniformBuffer: {
      layouts[i] = mContext->getUniformDescriptorSetLayout();
    } break;
    case ProgramSlot::StorageBuffer: {
      layouts[i] = mContext->getStorageDescriptorSetLayout();
    } break;
    }
  }

  VkPipelineLayoutCreateInfo createInfo{};
  createInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  createInfo.pNext                  = VK_NULL_HANDLE;
  createInfo.flags                  = 0;
  createInfo.setLayoutCount         = static_cast<uint32_t>(layouts.size());
  createInfo.pSetLayouts            = layouts.data();
  createInfo.pushConstantRangeCount = 0;
  createInfo.pPushConstantRanges    = VK_NULL_HANDLE;

  expectResult(
      "Pipeline layout creation",
      vkCreatePipelineLayout(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mLayout));
}

void Program::createPipeline(const ProgramInfo &info) {
  std::vector<VkPipelineShaderStageCreateInfo> stages{};
  stages.reserve(info.shaderCount);

  for (size_t i = 0; i < info.shaderCount; ++i) {
    auto shader = info.shaders[i];
    if (shader->api() != Api::Vulkan) throw InvalidUse("Uncompatible shader object");
    auto vkShader = reinterpret_cast<const Shader *>(shader);

    stages.push_back({ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                       VK_NULL_HANDLE,
                       0,
                       vkShader->getStage(),
                       vkShader->getModule(),
                       "main",
                       VK_NULL_HANDLE });
  }

  std::vector<VkVertexInputBindingDescription> vertexBindings{};
  vertexBindings.reserve(info.vertexInfoCount);
  std::vector<VkVertexInputAttributeDescription> vertexAttributes{};

  for (size_t i = 0; i < info.vertexInfoCount; ++i) {
    const auto &vertexInfo = info.vertexInfos[i];

    vertexBindings.push_back({ static_cast<uint32_t>(i), vertexInfo.stride, vkVertexInputRate(vertexInfo.inputRate) });

    vertexAttributes.reserve(vertexAttributes.size() + vertexInfo.attributeCount);
    for (size_t j = 0; j < vertexInfo.attributeCount; ++j) {
      const auto &attribute = vertexInfo.attributes[j];

      vertexAttributes.push_back(
          { static_cast<uint32_t>(j), static_cast<uint32_t>(i), vkFormat(attribute.format), attribute.offset });
    }
  }

  VkPipelineVertexInputStateCreateInfo vertexInputState{};
  vertexInputState.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputState.pNext                           = VK_NULL_HANDLE;
  vertexInputState.flags                           = 0;
  vertexInputState.vertexBindingDescriptionCount   = static_cast<uint32_t>(vertexBindings.size());
  vertexInputState.pVertexBindingDescriptions      = vertexBindings.data();
  vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size());
  vertexInputState.pVertexAttributeDescriptions    = vertexAttributes.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
  inputAssemblyState.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssemblyState.pNext                  = VK_NULL_HANDLE;
  inputAssemblyState.flags                  = 0;
  inputAssemblyState.topology               = vkTopology(info.topology);
  inputAssemblyState.primitiveRestartEnable = VK_FALSE;

  VkPipelineTessellationStateCreateInfo tessellationState{};
  tessellationState.sType              = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
  tessellationState.pNext              = VK_NULL_HANDLE;
  tessellationState.flags              = 0;
  tessellationState.patchControlPoints = 0;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.pNext         = VK_NULL_HANDLE;
  viewportState.flags         = 0;
  viewportState.viewportCount = 1;
  viewportState.pViewports    = VK_NULL_HANDLE;
  viewportState.scissorCount  = 1;
  viewportState.pScissors     = VK_NULL_HANDLE;

  VkPipelineRasterizationStateCreateInfo rasterizationState{};
  rasterizationState.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizationState.pNext                   = VK_NULL_HANDLE;
  rasterizationState.flags                   = 0;
  rasterizationState.depthClampEnable        = VK_FALSE;
  rasterizationState.rasterizerDiscardEnable = VK_FALSE;
  rasterizationState.polygonMode             = VK_POLYGON_MODE_FILL;
  rasterizationState.cullMode                = static_cast<VkCullModeFlags>(vkCullMode(info.cullMode));
  rasterizationState.frontFace               = vkFrontFace(info.frontFace);
  rasterizationState.depthBiasEnable         = VK_FALSE;
  rasterizationState.depthBiasConstantFactor = 1.0f;
  rasterizationState.depthBiasClamp          = VK_FALSE;
  rasterizationState.depthBiasSlopeFactor    = 1.0f;
  rasterizationState.lineWidth               = 1.0f;

  VkPipelineMultisampleStateCreateInfo multisampleState{};
  multisampleState.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampleState.pNext                 = VK_NULL_HANDLE;
  multisampleState.flags                 = 0;
  multisampleState.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
  multisampleState.sampleShadingEnable   = VK_FALSE;
  multisampleState.minSampleShading      = 0.0f;
  multisampleState.pSampleMask           = VK_NULL_HANDLE;
  multisampleState.alphaToCoverageEnable = VK_FALSE;
  multisampleState.alphaToOneEnable      = VK_FALSE;

  VkPipelineDepthStencilStateCreateInfo depthStencilState{};
  depthStencilState.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilState.pNext                 = VK_NULL_HANDLE;
  depthStencilState.flags                 = 0;
  depthStencilState.depthTestEnable       = VK_FALSE;
  depthStencilState.depthWriteEnable      = VK_FALSE;
  depthStencilState.depthCompareOp        = VK_COMPARE_OP_NEVER;
  depthStencilState.depthBoundsTestEnable = VK_FALSE;
  depthStencilState.stencilTestEnable     = VK_FALSE;
  depthStencilState.front                 = {};
  depthStencilState.back                  = {};
  depthStencilState.minDepthBounds        = 0.0f;
  depthStencilState.maxDepthBounds        = 1.0f;

  VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
  colorBlendAttachmentState.blendEnable         = VK_FALSE;
  colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachmentState.colorBlendOp        = VK_BLEND_OP_ADD;
  colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachmentState.alphaBlendOp        = VK_BLEND_OP_ADD;
  colorBlendAttachmentState.colorWriteMask      = static_cast<VkColorComponentFlags>(
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);

  VkPipelineColorBlendStateCreateInfo colorBlendState{};
  colorBlendState.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendState.pNext             = VK_NULL_HANDLE;
  colorBlendState.flags             = 0;
  colorBlendState.logicOpEnable     = VK_FALSE;
  colorBlendState.logicOp           = VK_LOGIC_OP_NO_OP;
  colorBlendState.attachmentCount   = 1;
  colorBlendState.pAttachments      = &colorBlendAttachmentState;
  colorBlendState.blendConstants[0] = 1.0f;
  colorBlendState.blendConstants[1] = 1.0f;
  colorBlendState.blendConstants[2] = 1.0f;
  colorBlendState.blendConstants[3] = 1.0f;

  std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.pNext             = VK_NULL_HANDLE;
  dynamicState.flags             = 0;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates    = dynamicStates.data();

  VkGraphicsPipelineCreateInfo createInfo{};
  createInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  createInfo.pNext               = VK_NULL_HANDLE;
  createInfo.flags               = 0;
  createInfo.stageCount          = static_cast<uint32_t>(stages.size());
  createInfo.pStages             = stages.data();
  createInfo.pVertexInputState   = &vertexInputState;
  createInfo.pInputAssemblyState = &inputAssemblyState;
  createInfo.pTessellationState  = &tessellationState;
  createInfo.pViewportState      = &viewportState;
  createInfo.pRasterizationState = &rasterizationState;
  createInfo.pMultisampleState   = &multisampleState;
  createInfo.pDepthStencilState  = &depthStencilState;
  createInfo.pColorBlendState    = &colorBlendState;
  createInfo.pDynamicState       = &dynamicState;
  createInfo.layout              = mLayout;
  createInfo.renderPass          = mRenderTarget->getRenderPass();
  createInfo.subpass             = 0;
  createInfo.basePipelineHandle  = VK_NULL_HANDLE;
  createInfo.basePipelineIndex   = 0;

  expectResult(
      "Pipeline creation",
      vkCreateGraphicsPipelines(mContext->getDevice(), VK_NULL_HANDLE, 1, &createInfo, VK_NULL_HANDLE, &mPipeline));
}

} // namespace purrr::vulkan

#endif // _PURRR_BACKEND_VULKAN