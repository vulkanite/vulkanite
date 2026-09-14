// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#if 0
#include "vulkanite/driver/Instance.h"

namespace vkn {

  Instance::Instance()
  {
    uint32_t numGPUs;

    PING;
    VkValidationFeaturesEXT validationFeatures{
      VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT
    };
    validationFeatures.pDisabledValidationFeatures = nullptr;
    validationFeatures.disabledValidationFeatureCount = 0;
      
    PING;
    VkValidationFeatureEnableEXT enabled[] = {
      VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT
    };
    validationFeatures.pEnabledValidationFeatures = enabled;
    validationFeatures.enabledValidationFeatureCount = 1;

    static VkApplicationInfo appInfo;
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "vulkanite";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "vulkanite";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;
    appInfo.pNext = VK_NULL_HANDLE;

    // ==================================================================
      

    std::vector<VkBaseOutStructure*> pNext_chain;
    PING;
    VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = {};
    PING;
    if (1/*info.use_debug_messenger*/) {
      messengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
      messengerCreateInfo.pNext = nullptr;
      messengerCreateInfo.messageSeverity =
        // info.debug_message_severity;
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
      messengerCreateInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | 
        VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
      messengerCreateInfo.pfnUserCallback = default_debug_callback;
      messengerCreateInfo.pUserData = nullptr;//info.debug_user_data_pointer;
      pNext_chain.push_back(reinterpret_cast<VkBaseOutStructure*>(&messengerCreateInfo));
    }
      
    PING;
    // VkValidationFeaturesEXT features{};
    if (1) {//info.enabled_validation_features.size() != 0 || info.disabled_validation_features.size()) {
      // features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
      // features.pNext = nullptr;
      // features.enabledValidationFeatureCount = static_cast<uint32_t>(info.enabled_validation_features.size());
      // features.pEnabledValidationFeatures = info.enabled_validation_features.data();
      // features.disabledValidationFeatureCount = static_cast<uint32_t>(info.disabled_validation_features.size());
      // features.pDisabledValidationFeatures = info.disabled_validation_features.data();
      pNext_chain.push_back(reinterpret_cast<VkBaseOutStructure*>(&validationFeatures));
    }
      
    PING;
    // VkValidationFlagsEXT checks{};
    // if (1){//info.disabled_validation_checks.size() != 0) {
    //   checks.sType = VK_STRUCTURE_TYPE_VALIDATION_FLAGS_EXT;
    //   checks.pNext = nullptr;
    //   checks.disabledValidationCheckCount = static_cast<uint32_t>(info.disabled_validation_checks.size());
    //   checks.pDisabledValidationChecks = info.disabled_validation_checks.data();
    //   pNext_chain.push_back(reinterpret_cast<VkBaseOutStructure*>(&checks));
    // }
      
    PING;
    std::vector<VkLayerSettingEXT> layer_settings;
    VkLayerSettingsCreateInfoEXT layer_settings_ci{};
    if (layer_settings.size() > 0) {
      layer_settings_ci.sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT;
      layer_settings_ci.pNext = nullptr;
      layer_settings_ci.settingCount = static_cast<uint32_t>(layer_settings.size());
      layer_settings_ci.pSettings = layer_settings.data();
      pNext_chain.push_back(reinterpret_cast<VkBaseOutStructure*>(&layer_settings_ci));
    }

    // ==================================================================

    PING;
    for (int i=1;i<pNext_chain.size();i++)
      pNext_chain[i-1]->pNext = pNext_chain[i];
                  
    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.pNext = pNext_chain[0];//&validationFeatures;

    VkAllocationCallbacks* allocationCallbacks = nullptr;
      
    std::vector<const char*> extensions = {
      VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
      VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
    };
    instanceCreateInfo.flags = (VkInstanceCreateFlags)0;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
    for (int attempt=0;attempt<2;attempt++) {
      static const char *layers[] =
        {"VK_LAYER_KHRONOS_validation"};
      if (attempt == 0) {
        instanceCreateInfo.enabledLayerCount   = 1;
        instanceCreateInfo.ppEnabledLayerNames = layers;
      } else {
        instanceCreateInfo.enabledLayerCount   = 0;
        instanceCreateInfo.ppEnabledLayerNames = nullptr;
      }
      VkResult rc = vkCreateInstance(&instanceCreateInfo, allocationCallbacks, &handle);
      if (rc == VK_SUCCESS) break;
      if (attempt == 0) {
        std::cout << "#vkn: WARNING - could not enable validation layer "
                  << "when creating vulkan driver instance" << std::endl;
      } else {
        throw std::runtime_error("could not create driver instance");
      }
    }
    VK_CALL(EnumeratePhysicalDevices(handle, &numGPUs, nullptr),
            "could not enumerate physical devices driver instance (step 1)");
    physicalHandles.resize(numGPUs);
    VK_CALL(EnumeratePhysicalDevices(handle, &numGPUs, physicalHandles.data()),
            "could not enumerate physical devices in driver instance (step 2)");
  }

  Instance::~Instance()
  {
    vkDestroyInstance(handle, nullptr);
  }
    
} // ::vkn
    
#endif
