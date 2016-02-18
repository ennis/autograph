#include <fstream>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <extra/image_io/load_image.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <format.h>

const char* getVkPhysicalDeviceTypeString(VkPhysicalDeviceType type) {
  switch (type) {
  case VK_PHYSICAL_DEVICE_TYPE_OTHER:
    return "VK_PHYSICAL_DEVICE_TYPE_OTHER";
  case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
    return "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU";
  case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
    return "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU";
  case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
    return "VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU";
  case VK_PHYSICAL_DEVICE_TYPE_CPU:
    return "VK_PHYSICAL_DEVICE_TYPE_CPU";
  default:
    return "Unknown";
  }
}

int main() {
  GLFWwindow* window;

  /* Initialize the library */
  if (!glfwInit())
    return -1;

  // query vulkan extensions
  int count;
  const char** extensions = glfwGetRequiredInstanceExtensions(&count);

  if (!extensions) {
    fmt::print(std::cerr, "glfwGetRequiredInstanceExtensions returned nullptr. "
                          "Cannot create a presentable surface.");
    glfwTerminate();
    return -1;
  }

  for (int i = 0; i < count; ++i)
	  fmt::print("Extension: {}\n", extensions[i]);
  // Create a vulkan instance
  VkInstance instance = nullptr;

  {
    VkInstanceCreateInfo ici = {};
    ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici.enabledExtensionCount = count;
    ici.ppEnabledExtensionNames = extensions;
    vkCreateInstance(&ici, nullptr, &instance);
  }

  // enumerate physical devices
  uint32_t physDevCount;
  vkEnumeratePhysicalDevices(instance, &physDevCount, nullptr);
  if (!physDevCount) {
    fmt::print(std::cerr, "No vulkan devices available.");
    glfwTerminate();
    return -1;
  }
  std::vector<VkPhysicalDevice> physDevices(physDevCount);
  vkEnumeratePhysicalDevices(instance, &physDevCount, physDevices.data());

  // dump info
  VkPhysicalDevice mainPhysDev;
  VkPhysicalDeviceType mainPhysDevType = VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM;
  fmt::print("Vulkan physical devices: \n");
  for (const auto& physDev : physDevices) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physDev, &props);
    fmt::print("- {} (id={}, type={})\n", props.deviceName, props.deviceID,
               getVkPhysicalDeviceTypeString(props.deviceType));
    VkPhysicalDeviceFeatures features;
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
        mainPhysDevType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      // prefer discrete GPUs
      mainPhysDev = physDev;
      mainPhysDevType = props.deviceType;
    } else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU &&
               mainPhysDevType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      // fallback on integrated
      mainPhysDev = physDev;
      mainPhysDevType = props.deviceType;
    } else if (mainPhysDevType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
               mainPhysDevType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
      // fallback on anything we come across
      mainPhysDev = physDev;
      mainPhysDevType = props.deviceType;
    }
  }

  uint32_t queueFamilyIndex;
  {
    // things
    uint32_t queueFamilyPropertyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
        mainPhysDev, &queueFamilyPropertyCount, nullptr);
    std::vector<VkQueueFamilyProperties> props(queueFamilyPropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(
        mainPhysDev, &queueFamilyPropertyCount, props.data());
    queueFamilyIndex = queueFamilyPropertyCount;
    for (int i = 0; i < queueFamilyPropertyCount; ++i) {
      const auto& p = props[i];
      if (p.queueFlags & VK_QUEUE_GRAPHICS_BIT &&
          p.queueFlags & VK_QUEUE_COMPUTE_BIT &&
          p.queueFlags & VK_QUEUE_TRANSFER_BIT) {
        queueFamilyIndex = i;
        break;
      }
    }
    if (queueFamilyIndex == queueFamilyPropertyCount) {
      fmt::print(std::cerr, "No universal queue found.\n");
      glfwTerminate();
      return -1;
    }
  }

  // open window
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
  if (!window) {
	  glfwTerminate();
	  return -1;
  }

  // create surface
  VkSurfaceKHR surface;
  VkResult err = glfwCreateWindowSurface(instance, window, NULL, &surface);
  if (err != VK_SUCCESS) {
	  fmt::print("Could not create a presentable surface\n");
	  return -1;
  }


  // create a vulkan device
  VkDevice device;

  {
    float queuePriorities[] = {0.0f};
    VkDeviceQueueCreateInfo qci[] = {
        {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0,
         queueFamilyIndex, 1, queuePriorities}};
    VkDeviceCreateInfo dci = {};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.enabledExtensionCount = count;
    dci.ppEnabledExtensionNames = extensions;
    dci.pQueueCreateInfos = qci;
    dci.queueCreateInfoCount = 1;

    auto err = vkCreateDevice(mainPhysDev, &dci, nullptr, &device);
	fmt::print("vkCreateDevice returned {}\n", err);
  }


  /* Loop until the user closes the window */
  while (!glfwWindowShouldClose(window)) {
    /* Render here */

    /* Swap front and back buffers */
    glfwSwapBuffers(window);

    /* Poll for and process events */
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
  return 0;
}
