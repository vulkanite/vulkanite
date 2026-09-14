// SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ingo Wald
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "vulkanite/ShaderProgram.h"
#include "vulkanite/DescriptorPool.h"

namespace vkn {
    
  struct Context;
  struct GeomType;
  struct Geom;
  struct RayGenType;
  struct RayGen;
  struct MissProgType;
  struct MissProg;
  struct Compute;
  struct GeomAccel;//TrianglesGeomAccel;
  struct Sampler;
  struct Image;
  
  template<typename ProgType>
  struct ListOfPrograms {
    void insert(ProgType *type) { active.insert(type); }
    void remove(ProgType *type) { active.erase(type); }
    std::set<ProgType *> active;
  };
  
  template<typename T>
  struct ListOf {
    inline int insert(T *me)
    {
      if (freeIDs.empty()) {
        int ID = elements.size();
        elements.push_back(me);
        return ID;
      } else {
        int ID = freeIDs.top();
        freeIDs.pop();
        elements[ID] = me;
        return ID;
      }
    }
    inline void release(int ID)
    {
      freeIDs.push(ID);
      elements[ID] = 0;
    }
    
    std::stack<int> freeIDs;
    std::vector<T *> elements;
  };


  template<typename T>
  struct IndexedListOf {
    int  track(T t);
    void forget(int ID);
    
    std::vector<T>  data;
    std::stack<int> freeIDs;
    int maxUsedID    = 0;
  };

  template<typename T>
  int  IndexedListOf<T>::track(T t)
  {
    if (freeIDs.empty()) 
      { int ID = data.size(); data.push_back(t); return ID; }
    else
      { int ID = freeIDs.top(); freeIDs.pop(); data[ID] = t; return ID; }
  }
  
  template<typename T>
  void IndexedListOf<T>::forget(int ID)
  { freeIDs.push(ID); data[ID] = 0; }

  template<typename VkHandle, typename Type>
  struct DSHelper {

    DSHelper(Context *context);
    ~DSHelper() {
      destroy();
    }
    void update();
    void destroy() {
      descriptorSet.free();
      pool.destroy();
    }
    void createLayout();
    void createDS();
    std::string toString();
    
    int  insert(Type *t);
    void remove(int ID);

    DescriptorPool        pool;
    DescriptorSetLayout   layout;
    DescriptorSet         descriptorSet;
    int dsSize = 0;
    IndexedListOf<Type *> objects;
    bool dirty = true;
    Context *const context;
  };
  
  
  struct RTPipeline {
    typedef std::shared_ptr<RTPipeline> SP;
    
    RTPipeline(Context *context);
    ~RTPipeline();
    
    void build();

    void track(ShaderProgram *program);
    void forget(ShaderProgram *program);
    void track(RayGenType *rgt);
    void forget(RayGenType *rgt);
    void track(MissProgType *rgt);
    void forget(MissProgType *rgt);
    void track(GeomType  *gt);
    void forget(GeomType *gt);
    void track(GeomAccel  *accel);
    void forget(GeomAccel *accel);
    
    std::set<ShaderProgram *>      programs;
    std::set<RayGenType *>         rayGenTypes;
    std::set<MissProgType *>       missProgTypes;
    std::set<GeomType *>           geomTypes;
    std::set<GeomAccel *> geomAccels;

    ListOf<RayGen> rayGens;
    ListOf<MissProg> missProgs;
    
    void gatherPrograms(std::vector<VkPipelineShaderStageCreateInfo> &shaderStages,
                        std::vector<VkRayTracingShaderGroupCreateInfoKHR> &shaderGroups);
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR properties;
    std::vector<uint8_t> shaderHandleStorage;
    /*! for now, hardcoded - should eventually be set to whatever
      max data size of any push constant type has been created */
    int maxPushConstantSize = 128;
      
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline       pipeline = VK_NULL_HANDLE;
    VkDescriptorSetLayout rayGenDescriptorSetLayout = VK_NULL_HANDLE;

    DSHelper<VkSampler,Sampler> samplers;
    DSHelper<VkImage,Image> image2Ds;
    
    void writeHeader(uint8_t *where, int shaderGroupIndex);
    
    Context *context;
    LogicalDevice::SP device;
  };

} // ::vkn


