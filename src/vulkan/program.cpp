#include "purrr/vulkan/exceptions.hpp"

#include "purrr/vulkan/program.hpp"
#include "purrr/vulkan/context.hpp"
#include "purrr/vulkan/window.hpp"

#include "purrr/vulkan/format.hpp"

#include <stdexcept>
#include <vector>
#include <array>

namespace purrr::vulkan {

VkShaderStageFlagBits vkShaderType(ShaderType type) {
  switch (type) {
  case ShaderType::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
  case ShaderType::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
  }

  throw std::runtime_error("Unreachable");
}

VkVertexInputRate vkVertexInputRate(VertexInputRate inputRate) {
  switch (inputRate) {
  case VertexInputRate::Vertex: return VK_VERTEX_INPUT_RATE_VERTEX;
  case VertexInputRate::Instance: return VK_VERTEX_INPUT_RATE_INSTANCE;
  }

  throw std::runtime_error("Unreachable");
}

VkPrimitiveTopology vkTopology(Topology topology) {
  switch (topology) {
  case Topology::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
  case Topology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  case Topology::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
  case Topology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  case Topology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
  }

  throw std::runtime_error("Unreachable");
}

VkCullModeFlagBits vkCullMode(CullMode cullMode) {
  switch (cullMode) {
  case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
  case CullMode::Back: return VK_CULL_MODE_BACK_BIT;
  case CullMode::Both: return VK_CULL_MODE_FRONT_AND_BACK;
  }

  throw std::runtime_error("Unreachable");
}

VkFrontFace vkFrontFace(FrontFace frontFace) {
  switch (frontFace) {
  case FrontFace::Clockwise: return VK_FRONT_FACE_CLOCKWISE;
  case FrontFace::CounterClockwise: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
  }

  throw std::runtime_error("Unreachable");
}

Shader::Shader(Context *context, const ShaderInfo &info)
  : mContext(context), mStage(vkShaderType(info.type)) {
  auto createInfo = VkShaderModuleCreateInfo{ .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                              .pNext    = VK_NULL_HANDLE,
                                              .flags    = 0,
                                              .codeSize = info.codeLength,
                                              .pCode    = reinterpret_cast<const uint32_t *>(info.code) };

  expectResult(
      "Shader module creation",
      vkCreateShaderModule(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mModule));
}

Shader::~Shader() {
  if (mModule) vkDestroyShaderModule(mContext->getDevice(), mModule, VK_NULL_HANDLE);
}

Program::Program(Window *window, Context *context, const ProgramInfo &info)
  : mWindow(window), mContext(context) {
  createLayout(info);
  createPipeline(info);
}

Program::~Program() {
  if (mLayout) vkDestroyPipelineLayout(mContext->getDevice(), mLayout, VK_NULL_HANDLE);
  if (mPipeline) vkDestroyPipeline(mContext->getDevice(), mPipeline, VK_NULL_HANDLE);
}

void Program::createLayout(const ProgramInfo &info) {
  auto createInfo = VkPipelineLayoutCreateInfo{ .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                                                .pNext                  = VK_NULL_HANDLE,
                                                .flags                  = 0,
                                                .setLayoutCount         = 0,
                                                .pSetLayouts            = VK_NULL_HANDLE,
                                                .pushConstantRangeCount = 0,
                                                .pPushConstantRanges    = VK_NULL_HANDLE };

  expectResult(
      "Pipeline layout creation",
      vkCreatePipelineLayout(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mLayout));
}

void Program::createPipeline(const ProgramInfo &info) {
  auto stages = std::vector<VkPipelineShaderStageCreateInfo>(info.shaderCount);
  for (size_t i = 0; i < info.shaderCount; ++i) {
    auto shader = info.shaders[i];
    if (shader->api() != Api::Vulkan) throw std::runtime_error("Uncompatible shader object");
    auto vkShader = reinterpret_cast<const Shader *>(shader);

    stages[i] = VkPipelineShaderStageCreateInfo{ .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                 .pNext  = VK_NULL_HANDLE,
                                                 .flags  = 0,
                                                 .stage  = vkShader->getStage(),
                                                 .module = vkShader->getModule(),
                                                 .pName  = "main",
                                                 .pSpecializationInfo = VK_NULL_HANDLE };
  }

  auto vertexBindings   = std::vector<VkVertexInputBindingDescription>(info.vertexInfoCount);
  auto vertexAttributes = std::vector<VkVertexInputAttributeDescription>();
  for (size_t i = 0; i < info.vertexInfoCount; ++i) {
    const auto &vertexInfo = info.vertexInfos[i];

    vertexBindings[i] = VkVertexInputBindingDescription{ .binding   = static_cast<uint32_t>(i),
                                                         .stride    = vertexInfo.stride,
                                                         .inputRate = vkVertexInputRate(vertexInfo.inputRate) };

    for (size_t j = 0; j < vertexInfo.attributeCount; ++j) {
      const auto &attribute = vertexInfo.attributes[j];

      vertexAttributes.push_back(VkVertexInputAttributeDescription{ .location = static_cast<uint32_t>(j),
                                                                    .binding  = static_cast<uint32_t>(i),
                                                                    .format   = vkFormat(attribute.format),
                                                                    .offset   = attribute.offset });
    }
  }

  auto vertexInputState = VkPipelineVertexInputStateCreateInfo{
    .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .pNext                           = VK_NULL_HANDLE,
    .flags                           = 0,
    .vertexBindingDescriptionCount   = static_cast<uint32_t>(vertexBindings.size()),
    .pVertexBindingDescriptions      = vertexBindings.data(),
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size()),
    .pVertexAttributeDescriptions    = vertexAttributes.data()
  };

  auto inputAssemblyState =
      VkPipelineInputAssemblyStateCreateInfo{ .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                                              .pNext    = VK_NULL_HANDLE,
                                              .flags    = 0,
                                              .topology = vkTopology(info.topology),
                                              .primitiveRestartEnable = VK_FALSE };

  auto tessellationState =
      VkPipelineTessellationStateCreateInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
                                             .pNext = VK_NULL_HANDLE,
                                             .flags = 0,
                                             .patchControlPoints = 0 };

  auto viewportState =
      VkPipelineViewportStateCreateInfo{ .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                                         .pNext         = VK_NULL_HANDLE,
                                         .flags         = 0,
                                         .viewportCount = 1,
                                         .pViewports    = VK_NULL_HANDLE,
                                         .scissorCount  = 1,
                                         .pScissors     = VK_NULL_HANDLE };

  auto rasterizationState =
      VkPipelineRasterizationStateCreateInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                                              .pNext = VK_NULL_HANDLE,
                                              .flags = 0,
                                              .depthClampEnable        = VK_FALSE,
                                              .rasterizerDiscardEnable = VK_FALSE,
                                              .polygonMode             = VK_POLYGON_MODE_FILL,
                                              .cullMode                = (VkCullModeFlags)vkCullMode(info.cullMode),
                                              .frontFace               = vkFrontFace(info.frontFace),
                                              .depthBiasEnable         = VK_FALSE,
                                              .depthBiasConstantFactor = 1.0f,
                                              .depthBiasClamp          = VK_FALSE,
                                              .depthBiasSlopeFactor    = 1.0f,
                                              .lineWidth               = 1.0f };

  auto multisampleState =
      VkPipelineMultisampleStateCreateInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                                            .pNext = VK_NULL_HANDLE,
                                            .flags = 0,
                                            .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
                                            .sampleShadingEnable   = VK_FALSE,
                                            .minSampleShading      = 0.0f,
                                            .pSampleMask           = VK_NULL_HANDLE,
                                            .alphaToCoverageEnable = VK_FALSE,
                                            .alphaToOneEnable      = VK_FALSE };

  auto depthStencilState =
      VkPipelineDepthStencilStateCreateInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                                             .pNext = VK_NULL_HANDLE,
                                             .flags = 0,
                                             .depthTestEnable       = VK_FALSE,
                                             .depthWriteEnable      = VK_FALSE,
                                             .depthCompareOp        = VK_COMPARE_OP_NEVER,
                                             .depthBoundsTestEnable = VK_FALSE,
                                             .stencilTestEnable     = VK_FALSE,
                                             .front                 = VkStencilOpState{},
                                             .back                  = VkStencilOpState{},
                                             .minDepthBounds        = 0.0f,
                                             .maxDepthBounds        = 1.0f };

  auto colorBlendAttachmentState =
      VkPipelineColorBlendAttachmentState{ .blendEnable         = VK_FALSE,
                                           .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                                           .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                                           .colorBlendOp        = VK_BLEND_OP_ADD,
                                           .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                                           .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                                           .alphaBlendOp        = VK_BLEND_OP_ADD,
                                           .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT };

  auto colorBlendState =
      VkPipelineColorBlendStateCreateInfo{ .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                                           .pNext           = VK_NULL_HANDLE,
                                           .flags           = 0,
                                           .logicOpEnable   = VK_FALSE,
                                           .logicOp         = VK_LOGIC_OP_NO_OP,
                                           .attachmentCount = 1,
                                           .pAttachments    = &colorBlendAttachmentState,
                                           .blendConstants  = { 1.0f, 1.0f, 1.0f, 1.0f } };

  auto dynamicStates = std::array<VkDynamicState, 2>({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });

  auto dynamicState =
      VkPipelineDynamicStateCreateInfo{ .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                                        .pNext             = VK_NULL_HANDLE,
                                        .flags             = 0,
                                        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
                                        .pDynamicStates    = dynamicStates.data() };

  auto createInfo = VkGraphicsPipelineCreateInfo{ .sType             = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                                                  .pNext             = VK_NULL_HANDLE,
                                                  .flags             = 0,
                                                  .stageCount        = static_cast<uint32_t>(stages.size()),
                                                  .pStages           = stages.data(),
                                                  .pVertexInputState = &vertexInputState,
                                                  .pInputAssemblyState = &inputAssemblyState,
                                                  .pTessellationState  = &tessellationState,
                                                  .pViewportState      = &viewportState,
                                                  .pRasterizationState = &rasterizationState,
                                                  .pMultisampleState   = &multisampleState,
                                                  .pDepthStencilState  = &depthStencilState,
                                                  .pColorBlendState    = &colorBlendState,
                                                  .pDynamicState       = &dynamicState,
                                                  .layout              = mLayout,
                                                  .renderPass          = mWindow->getRenderPass(),
                                                  .subpass             = 0,
                                                  .basePipelineHandle  = VK_NULL_HANDLE,
                                                  .basePipelineIndex   = 0 };

  expectResult(
      "Pipeline creation",
      vkCreateGraphicsPipelines(mContext->getDevice(), VK_NULL_HANDLE, 1, &createInfo, VK_NULL_HANDLE, &mPipeline));
}

} // namespace purrr::vulkan