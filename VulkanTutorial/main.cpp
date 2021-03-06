#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>

#include "CustomValidationLayer.h"
#include "PhysicalDeviceUtils.h"
#include "GraphicsPipelineUtils.h"

class HelloTriangleApplication {
public:

    HelloTriangleApplication() {

    }

    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:

    // class members
    GLFWwindow* window;
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    VkInstance instance;
    CustomVulkanUtils::CustomValidationLayer validationLayerManager;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    VkDevice device; // logical device
    VkQueue graphicsQueue; // handle to the logical queue
    VkQueue presentQueue; // handle to the presentation queue between vulkan and windows system

    VkSurfaceKHR surface; // connection between vulkan and window system

    VkSwapchainKHR swapChain; // swap chain that holds rendered images and/ or presents them on screen
    std::vector<VkImage> swapChainImages; // handles of images inside swapChain
    VkFormat swapChainImageFormat; // surface format of the swapChain
    VkExtent2D swapChainExtent; // extent (size of images in pixels) of swapChain

	std::vector<VkImageView> swapChainImageViews; // imageViews that describe how to access the image (2D with Depth or 3D...)

    // validation layers for debugging
    const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
    };

    // needed extensions for the graphics card
    const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif


    // functions
    void initWindow() {
        glfwInit(); // init glfw API

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // do not use OpenGL, create
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // disable window resizing

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr); // init window, store in private window var.
    }

    void initVulkan() {
        createInstance();

        if (enableValidationLayers) {
            validationLayerManager = CustomVulkanUtils::CustomValidationLayer(instance);
            validationLayerManager.setupDebugMessenger();
        }

        createSurface();
        
        physicalDevice = CustomVulkanUtils::pickPhysicalDevice(instance, surface, deviceExtensions);
        device = CustomVulkanUtils::createLogicalDevice(physicalDevice, surface, enableValidationLayers, validationLayers, graphicsQueue, presentQueue, deviceExtensions);
        swapChain = CustomVulkanUtils::createSwapChain(swapChainImages, swapChainImageFormat, swapChainExtent, window, physicalDevice, device, surface);
		CustomVulkanUtils::createImageViews(swapChainImageViews, swapChainImages, swapChainImageFormat, device);

		CustomVulkanUtils::createGraphicsPipeline(device);
    }

    void mainLoop() {
        
        while (!glfwWindowShouldClose(window)) { // endless loop till window closes
            glfwPollEvents(); //check for key events
        }

    }

    void cleanup() {

        if (enableValidationLayers) {
            validationLayerManager.cleanup();
        }

		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}

        vkDestroySwapchainKHR(device, swapChain, nullptr);

        vkDestroyDevice(device, nullptr);

        vkDestroySurfaceKHR(instance, surface, nullptr);

        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }


    //helper functions

    // creat vulkan instance in private member instance
    void createInstance() {

        // see if we can get validation layers running for debugging 
        if (enableValidationLayers && !CustomVulkanUtils::checkValidationLayerSupport(validationLayers)) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        // get the supported extensions
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);

        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()); //query the supported extensions and store in array

        std::cout << "available extensions:\n";

        for (const auto& extension : extensions) {
            std::cout << '\t' << extension.extensionName << '\n';
        }

        // create struct that holds informations about application and store in vulkan instance
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // store information about global extensions and validation layers
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto reqExtensions = getRequiredExtensions(); // get required extensions from glfw
        if (!checkRequiredExtensionsAreSupported(reqExtensions, extensions)) {
            throw std::runtime_error("Not all required extensions are supported!");
        }

        createInfo.enabledExtensionCount = static_cast<uint32_t>(reqExtensions.size());
        createInfo.ppEnabledExtensionNames = reqExtensions.data();

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        // create vulkan instance with specified information and store in private variable
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create instance!");
        }
    }

    void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount); //init vector from char** !

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    // check if all extensions that are required are also supported
    bool checkRequiredExtensionsAreSupported(std::vector<const char*> reqExtensions, std::vector<VkExtensionProperties>& extensions) {
        for (const char* extName : reqExtensions) {
            bool layerFound = false;

            for (const auto& layerProperties : extensions) {
                if (strcmp(extName, layerProperties.extensionName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}