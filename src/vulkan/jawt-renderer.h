#pragma once


#include <jawt_md.h>



class JAWTVulkanRenderer {
public:

    virtual void render(JNIEnv* jni, jobject javaVulkanRenderer, const std::vector<std::vector<glm::dvec2>>& polygonSet,
                        const Triangulation* triangulation) = 0;

    virtual ~JAWTVulkanRenderer() = default;

};

JAWTVulkanRenderer* createVulkanRenderer(JNIEnv*);
void destroyVulkanRenderer(JAWTVulkanRenderer*);