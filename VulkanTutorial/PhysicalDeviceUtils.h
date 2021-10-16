#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <vector>
#include <map>
#include <optional> //wrapper that contains no value until we assign something. Can be queried with "has_value()" member function.

namespace CustomVulkanUtils {

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;

		bool isComplete() { // check if the value is set and we thus support the wanted commands
			return graphicsFamily.has_value();
		}
	};

	// check which queue families are supported by the device
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;

		// Logic to find queue family indices to populate struct with
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) { // find atleast one queue family that supports VK_QUEUE_GRAPHICS_BIT (graphics capabilities)
				indices.graphicsFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}

			i++;
		}

		return indices;
	}


	// check physical devices (graphics cards) for compatabilities with our requirements
	bool isDeviceSuitable(VkPhysicalDevice device) {

		// query for device details
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);

		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		//return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		//	deviceFeatures.geometryShader;
		return true;
	}

	// function to return a score of a graphics card with respect to the options it supports
	int rateDeviceSuitability(VkPhysicalDevice device) {
		
		// Query for device details
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);

		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		int score = 0;

		// Discrete GPUs have a significant performance advantage
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			score += 1000;
		}

		// Maximum possible size of textures affects graphics quality
		score += deviceProperties.limits.maxImageDimension2D;

		// Application can't function without geometry shaders
		if (!deviceFeatures.geometryShader) {
			return 0;
		}

		// Ensure the device can process the commands (with its supported queue families) we want to use
		QueueFamilyIndices indices = findQueueFamilies(device);
		if (!indices.isComplete()) {
			return 0;
		}

		return score;
	}


	VkPhysicalDevice pickPhysicalDevice(VkInstance instance) {

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

		// listing available graphics cards
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		// check if any of the listed devices satisfy our requirements
		/*for (const auto& device : devices) {
			if (isDeviceSuitable(device)) {
				physicalDevice = device;
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}*/

		// order the graphics cards according to their value: use ordered map for comparison
		std::multimap<int, VkPhysicalDevice> candidates;

		for (const auto& device : devices) {
			int score = rateDeviceSuitability(device);
			candidates.insert(std::make_pair(score, device));
		}

		// Check if the best candidate is suitable at all
		if (candidates.rbegin()->first > 0) {
			physicalDevice = candidates.rbegin()->second;
		}
		else {
			throw std::runtime_error("failed to find a suitable GPU!");
		}

		return physicalDevice;
	}


	VkDevice createLogicalDevice(VkPhysicalDevice physicalDevice, bool enableValidationLayers, std::vector<const char*> validationLayers, VkQueue& graphicsQueue) {

		VkDevice device;

		// specifying used queues: we only care about the graphic functionalities here (probe for VK_QUEUE_GRAPHICS_BIT in findQueueFamilies) 
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
		queueCreateInfo.queueCount = 1;

		float queuePriority = 1.0f;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		// needed device features, like geometry shaders
		VkPhysicalDeviceFeatures deviceFeatures{};


		// fill main creatInfo structure with previous 2 structs
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.pQueueCreateInfos = &queueCreateInfo;
		createInfo.queueCreateInfoCount = 1;

		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = 0;

		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}

		//retrieve queue handles
		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);

		return device;
	}

}