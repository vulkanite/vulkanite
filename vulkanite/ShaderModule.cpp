// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/ShaderModule.h"
#include "vulkanite/Context.h"

namespace vkn {

  namespace {

    // Workaround for Slang emitting StoragePushConstant8 capability 
    // for pointers in push constants referencing structs including 
    // 8-bit types.
    bool stripStoragePushConstant8(std::vector<uint32_t> &code)
    {
      if (code.size() < 5) return false;

      std::vector<uint32_t> stripped;
      stripped.reserve(code.size());
      stripped.insert(stripped.end(), code.begin(), code.begin() + 5);

      bool removed = false;
      for (size_t i = 5; i < code.size();) {
        const uint16_t op = uint16_t(code[i] & 0xffffu);
        const uint16_t wordCount = uint16_t(code[i] >> 16);
        if (wordCount == 0 || i + wordCount > code.size())
          return false;

        if (op == 17
            && wordCount == 2
            && code[i + 1] == 4450) {
          removed = true;
        } else {
          stripped.insert(stripped.end(), code.begin() + i,
                          code.begin() + i + wordCount);
        }

        i += wordCount;
      }

      if (removed)
        code.swap(stripped);
      return removed;
    }

  } // anonymous namespace

  ShaderModule::ShaderModule(RTPipeline *pipeline,
                             /*! embedded, precompiled binary spir-v code */
                             const std::vector<uint8_t> &byteCode)
    : Object(pipeline->context),
      pipeline(pipeline)
  {
    LOG_CONSTRUCTION(ShaderModule,this);

    assert(!byteCode.empty() && "empty byte-code in Module constructor!?...");
    int numUIntsForCode = (int)divRoundUp(byteCode.size(),sizeof(uint32_t));
    std::vector<uint32_t> dwordAlignedCode(numUIntsForCode);
    memcpy(dwordAlignedCode.data(),byteCode.data(),byteCode.size());

    if (stripStoragePushConstant8(dwordAlignedCode))
      std::cout << "#vkn: stripped StoragePushConstant8 capability" << std::endl;
      
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = dwordAlignedCode.size()*sizeof(uint32_t);
    createInfo.pCode    = dwordAlignedCode.data();
    VK_CALL(CreateShaderModule(device->handle,
                               &createInfo,
                               nullptr,
                               &this->handle),
            "could not create shader module");
  }
    
  ShaderModule::~ShaderModule()
  {
    vkDestroyShaderModule(device->handle, this->handle, nullptr);

    LOG_DESTRUCTION(ShaderModule,this);
  }

  std::shared_ptr<ShaderProgram> ShaderModule::get(const std::string &entryPoint,
                                                   VkShaderStageFlagBits stage)
  {
    auto it = programs.find(entryPoint);
    if (it != programs.end()) {
      auto locked = it->second.lock();
      if (locked) return locked;
    }
    
    ShaderProgram::SP prog
      = std::make_shared<ShaderProgram>(this,
                                        entryPoint,
                                        stage);
    programs[entryPoint] = prog;
    return prog;
  }
  
} // ::vkn
    
