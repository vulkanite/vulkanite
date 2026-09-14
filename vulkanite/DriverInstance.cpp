// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#include "vulkanite/DriverInstance.h"
#include "vulkanite/PhysicalDevice.h"

namespace vkn {
  const char *toString(VkDebugUtilsMessageSeverityFlagBitsEXT s);
  const char *toString(VkDebugUtilsMessageTypeFlagsEXT s);
  inline VKAPI_ATTR VkBool32
  VKAPI_CALL default_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                    void*)
  {
    auto ms = toString(messageSeverity);
    auto mt = toString(messageType);
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
      printf("VKN [%s: %s] - %s\n%s\n", ms, mt, pCallbackData->pMessageIdName, pCallbackData->pMessage);
    } else {
      printf("VKN [%s: %s]\n%s\n", ms, mt, pCallbackData->pMessage);
    }

    return VK_FALSE; // Applications must return false here (Except Validation, if return true, will skip calling to driver)
  }
    
  
  namespace driver {
    
    Instance::Instance()
    {
      VkValidationFeatureEnableEXT enabled[]
        = {VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT};
      
      // validationFeatures.pEnabledValidationFeatures = enabled;
      // validationFeatures.enabledValidationFeatureCount = 1;

      VkApplicationInfo appInfo;
      appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      appInfo.pApplicationName = "vulkanite";
      appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
      appInfo.pEngineName = "vulkanite";
      appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
      appInfo.apiVersion = VK_API_VERSION_1_2;
      appInfo.pNext = VK_NULL_HANDLE;

      // ==================================================================
      
#if 1
      VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = {};
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
      messengerCreateInfo.pUserData = nullptr;
#endif

      std::vector<const char *> instanceExtensions;
      instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
      
      VkValidationFeaturesEXT validationFeatures
        {VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
      validationFeatures.disabledValidationFeatureCount = 0;
      validationFeatures.enabledValidationFeatureCount = 1;
      validationFeatures.pDisabledValidationFeatures = nullptr;
      validationFeatures.pEnabledValidationFeatures = enabled;
      
      
      VkInstanceCreateInfo instanceCreateInfo{};
      instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      instanceCreateInfo.pApplicationInfo = &appInfo;
      
      const char* layerNames[1] = {"VK_LAYER_KHRONOS_validation"};
      instanceCreateInfo.ppEnabledLayerNames = &layerNames[0];
      instanceCreateInfo.enabledLayerCount = 1;
#if VKN_VALIDATION_ENABLED
      instanceCreateInfo.pNext = &validationFeatures;
#else
      instanceCreateInfo.pNext = validationFeatures.pNext;
#endif
      instanceCreateInfo.enabledExtensionCount = (uint32_t) instanceExtensions.size();
      instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

      VkAllocationCallbacks* allocationCallbacks = nullptr;

      std::vector<const char*> extensions = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
      };
      instanceCreateInfo.flags = (VkInstanceCreateFlags)0;
      instanceCreateInfo.pApplicationInfo = &appInfo;
      instanceCreateInfo.enabledExtensionCount
        = static_cast<uint32_t>(extensions.size());
      instanceCreateInfo.ppEnabledExtensionNames
        = extensions.data();
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
    }

    Instance::~Instance()
    {
      std::cout << "#vkn: ================================================================== " << std::endl;
      std::cout << "#vkn: driver instance is dying!" << std::endl;
      std::cout << "#vkn: ================================================================== " << std::endl;
      vkDestroyInstance(handle, nullptr);
    }
    
  } // ::vkn::driver

  const char *toString(VkDebugUtilsMessageSeverityFlagBitsEXT s)
  {
    switch (s) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      return "VERBOSE";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      return "ERROR";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      return "WARNING";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      return "INFO";
    default:
      return "UNKNOWN";
    }
  }

  const char *toString(VkDebugUtilsMessageTypeFlagsEXT s)
  {
    if (s == 7) return "General | Validation | Performance";
    if (s == 6) return "Validation | Performance";
    if (s == 5) return "General | Performance";
    if (s == 4 /*VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT*/) return "Performance";
    if (s == 3) return "General | Validation";
    if (s == 2 /*VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT*/) return "Validation";
    if (s == 1 /*VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT*/) return "General";
    return "Unknown";
  }
    
  DriverInstance::DriverInstance(VKNDeviceType deviceType)    
  {
    createDriverHandle();

    std::vector<PhysicalDevice::SP> physicalDevices = findDevices(deviceType);
    for (auto phys : physicalDevices) {
      devices.push_back(std::make_shared<LogicalDevice>(phys));
    }
  }

  void DriverInstance::terminate()
  {
    for (auto &dev : devices) {
      dev->terminate();
    }
    devices = {};
    assert(this->selfReference);
    this->selfReference.reset();
  }
  
  void DriverInstance::createSelfReference()
  { this->selfReference = shared_from_this(); }

  DriverInstance::~DriverInstance()
  {
    assert(devices.empty());
    
    if (driverHandle.use_count() != 1) {
      std::cerr << "#vkn: driverinstance wants to die, but seems somebody else "
                << "is still holding a reference to the VkInstance!?" << std::endl;
      exit(1);
    }
    driverHandle.reset();
  }


  
  void DriverInstance::createDriverHandle()
  {
    assert(!driverHandle);
    driverHandle = std::make_shared<driver::Instance>();
  }
  
  /*! create a list of matching physical devices matchign the
    specified type(s) that can do ray tracing */
  std::vector<PhysicalDevice::SP> DriverInstance::findDevices(VKNDeviceType deviceType)
  {
    uint32_t numGPUs = 0;
    VK_CALL(EnumeratePhysicalDevices(getHandle(), &numGPUs, nullptr),
            "could not enumerate physical devices driver instance (step 1)");
    if (numGPUs == 0)
      throw std::runtime_error("could not find _any_ vulkan GPUs!?");

    std::vector<VkPhysicalDevice> physicalHandles(numGPUs);
    VK_CALL(EnumeratePhysicalDevices(getHandle(), &numGPUs, physicalHandles.data()),
            "could not enumerate physical devices in driver instance (step 2)");
    
    std::vector<PhysicalDevice::SP> physicalDevices;
    for (uint32_t vulkanID = 0; vulkanID < numGPUs; vulkanID++) {
      auto ph = physicalHandles[vulkanID];
      PhysicalDevice::SP pd
        = std::make_shared<PhysicalDevice>(driverHandle,vulkanID,ph);
      if (!pd->canDoRayTracing())
        continue;
      physicalDevices.push_back(pd);
    }

    std::vector<PhysicalDevice::SP> matching;
    if (deviceType & VKN_DEVICE_TYPE_DISCRETE)
      for (auto dev : physicalDevices)
        if (dev->type == PhysicalDevice::DiscreteGPU)
          matching.push_back(dev);
    
    if (deviceType & VKN_DEVICE_TYPE_INTEGRATED)
      for (auto dev : physicalDevices)
        if (dev->type == PhysicalDevice::IntegratedGPU)
          matching.push_back(dev);
    
    if (deviceType & VKN_DEVICE_TYPE_VIRTUAL)
      for (auto dev : physicalDevices)
        if (dev->type == PhysicalDevice::VirtualGPU)
          matching.push_back(dev);
    
    return matching;
  }
  
  
} // ::vkn
    
