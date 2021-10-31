#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <vector>
#include <fstream>

namespace CustomVulkanUtils {

	// function to read binary data from the compiled shader files
	static std::vector<char> readFile(const std::string& filename) {

		std::ifstream file(filename, std::ios::ate | std::ios::binary); // open file at end to get size of file

		if (!file.is_open()) {
			throw std::runtime_error("failed to open binary shader file!");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize); // read all bytes at once

		file.close();

		return buffer;
	}

	// wrap shader code into VkShaderModule to be used by the graphics pipeline
	VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice& device) {

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderModule;

	}

}