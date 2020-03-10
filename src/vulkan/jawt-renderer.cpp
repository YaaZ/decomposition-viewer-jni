#include <jni.h>

#include "../triangulation.h"
#include "jawt-renderer.h"
#include "include.h"
#include "renderer.h"


extern vk::Instance vkInstance;



#if defined(VK_USE_PLATFORM_WIN32_KHR)

#include <Windows.h>
static vk::UniqueSurfaceKHR createSurface(vk::Instance vk, const JAWT_DrawingSurfaceInfo& drawingSurfaceInfo) {
    auto& win32DrawingSurfaceInfo = *((JAWT_Win32DrawingSurfaceInfo*) drawingSurfaceInfo.platformInfo);
    auto hinstance = (HINSTANCE) GetWindowLongPtr(win32DrawingSurfaceInfo.hwnd, GWLP_HINSTANCE);
    return vk.createWin32SurfaceKHRUnique({{}, hinstance, win32DrawingSurfaceInfo.hwnd});
}

#else

static vk::UniqueSurfaceKHR createSurface(vk::Instance vk, const JAWT_DrawingSurfaceInfo& drawingSurfaceInfo) {
    auto& x11DrawingSurfaceInfo = *((JAWT_X11DrawingSurfaceInfo*) drawingSurfaceInfo.platformInfo);
    return vk.createXlibSurfaceKHRUnique({{}, x11DrawingSurfaceInfo.display, x11DrawingSurfaceInfo.drawable});
}

#endif





class JAWTVulkanRendererImpl : public JAWTVulkanRenderer {


    class Lock {

        JAWT_DrawingSurface* const jawtDrawingSurface;
    public:
        JAWT_DrawingSurfaceInfo* jawtDrawingSurfaceInfo;
        bool surfaceChanged = false, boundsChanged = false;

        explicit Lock(JAWT_DrawingSurface* jawtDrawingSurface, JAWT_Rectangle& lastDrawingSurfaceBounds) :
                jawtDrawingSurface(jawtDrawingSurface) {
            jint lock = jawtDrawingSurface->Lock(jawtDrawingSurface);
            if((lock & JAWT_LOCK_ERROR) != 0) throw std::runtime_error("Error locking JAWT drawing surface");
            if((lock & JAWT_LOCK_SURFACE_CHANGED) != 0) {
                surfaceChanged = true;
                boundsChanged = true;
            }
            if((lock & JAWT_LOCK_BOUNDS_CHANGED) != 0) boundsChanged = true;
            // Some JAWT implementations may update existing info and some may not, so we better get new one each frame
            jawtDrawingSurfaceInfo = jawtDrawingSurface->GetDrawingSurfaceInfo(jawtDrawingSurface);
            if (jawtDrawingSurfaceInfo == nullptr) {
                jawtDrawingSurface->Unlock(jawtDrawingSurface);
                throw std::runtime_error("Cannot get JAWT drawing surface info");
            }
            // Some JAWT implementations don't ever return JAWT_LOCK_BOUNDS_CHANGED, so we'll re-check it ourselves
            if(!boundsChanged && (
                    jawtDrawingSurfaceInfo->bounds.x != lastDrawingSurfaceBounds.x ||
                    jawtDrawingSurfaceInfo->bounds.y != lastDrawingSurfaceBounds.y ||
                    jawtDrawingSurfaceInfo->bounds.width != lastDrawingSurfaceBounds.width ||
                    jawtDrawingSurfaceInfo->bounds.height != lastDrawingSurfaceBounds.height
            )) boundsChanged = true;
        };

        ~Lock() {
            jawtDrawingSurface->FreeDrawingSurfaceInfo(jawtDrawingSurfaceInfo);
            jawtDrawingSurface->Unlock(jawtDrawingSurface);
        }

    };


    JAWT jawt {JAWT_VERSION_9};
    JAWT_DrawingSurface* jawtDrawingSurface {nullptr};
    JAWT_Rectangle lastDrawingSurfaceBounds {};

    vk::UniqueSurfaceKHR surface;

    VulkanRenderer renderer {};

public:
    explicit JAWTVulkanRendererImpl(JNIEnv* jni) {
        if(JAWT_GetAWT(jni, &jawt) == JNI_FALSE) throw std::runtime_error("JAWT Not found");
    }

    void render(JNIEnv* jni, jobject javaVulkanRenderer, const std::vector<std::vector<glm::dvec2>>& polygonSet,
                const Triangulation* const triangulation) final {
        bool justRetrievedDrawingSurface = false;
        if(jawtDrawingSurface == nullptr) {
            jawtDrawingSurface = jawt.GetDrawingSurface(jni, javaVulkanRenderer);
            if (jawtDrawingSurface == nullptr) throw std::runtime_error("Cannot get JAWT drawing surface");
            justRetrievedDrawingSurface = true;
        }
        Lock lock(jawtDrawingSurface, lastDrawingSurfaceBounds);
        lastDrawingSurfaceBounds = lock.jawtDrawingSurfaceInfo->bounds;
        if(lock.surfaceChanged || justRetrievedDrawingSurface) {
            renderer = {};
            surface = {};
            surface = createSurface(vkInstance, *lock.jawtDrawingSurfaceInfo);
            renderer = VulkanRenderer(vkInstance, *surface);
        }
        if(lock.boundsChanged || justRetrievedDrawingSurface) renderer.updateSwapchainContext();
        renderer.render(polygonSet, triangulation);
    }

    ~JAWTVulkanRendererImpl() final {
        renderer = {};
        surface = {};
        jawt.FreeDrawingSurface(jawtDrawingSurface);
    }


};


JAWTVulkanRenderer* createVulkanRenderer(JNIEnv* jni) {
    return new JAWTVulkanRendererImpl(jni);
}

void destroyVulkanRenderer(JAWTVulkanRenderer* renderer) {
    delete renderer;
}