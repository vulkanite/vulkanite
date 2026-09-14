// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "vulkanite/ShaderProgram.h"

namespace vkn {

  struct Compute : public ShaderProgram {
    typedef std::shared_ptr<Compute> SP;
      
    Compute(ShaderModule *shaderModule,
            const std::string &entryPoint,
            const size_t size);
    virtual ~Compute();
      
    void launch(const glm::ivec3 numBlocks,
                const void *pArgs);
      
    void buildPipeline();

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline       pipeline = VK_NULL_HANDLE;
    const size_t size;
  };
    
}
