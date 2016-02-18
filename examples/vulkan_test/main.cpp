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

constexpr auto kWidth = 800u;
constexpr auto kHeight = 600u;

int main() {
  GLFWwindow* window;

  /* Initialize the library */
  if (!glfwInit())
    return -1;

  /////////////////////////////////
  // query required vulkan extensions
  // for creating a presentable surface
  int requiredExtensionCount;
  const char** requiredExtensionNames =
      glfwGetRequiredInstanceExtensions(&requiredExtensionCount);

  if (!requiredExtensionNames) {
    fmt::print(std::cerr, "glfwGetRequiredInstanceExtensions returned nullptr. "
                          "Cannot create a presentable surface.");
    glfwTerminate();
    return -1;
  }

  for (int i = 0; i < requiredExtensionCount; ++i)
    fmt::print("Extension: {}\n", requiredExtensionNames[i]);

  /////////////////////////////////
  // Create a vulkan instance
  VkInstance instance = nullptr;

  {
    VkInstanceCreateInfo ici = {};
    ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici.enabledExtensionCount = requiredExtensionCount;
    ici.ppEnabledExtensionNames = requiredExtensionNames;
    vkCreateInstance(&ici, nullptr, &instance);
  }

  /////////////////////////////////
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

  /////////////////////////////////
  // Select a physical device
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

  // Note: the physical device must support VK_KHR_swapchain!
  // TODO: check it

  /////////////////////////////////
  // Find which queue families we want
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
        // this queue must support presentation
        if (glfwGetPhysicalDevicePresentationSupport(instance, mainPhysDev,
                                                     i)) {
          queueFamilyIndex = i;
          break;
        }
      }
    }
    if (queueFamilyIndex == queueFamilyPropertyCount) {
      fmt::print(std::cerr, "No universal queue found.\n");
      glfwTerminate();
      return -1;
    }
  }

  // enum extensions
  /*{
    uint32_t numExtensions = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, nullptr);
    std::vector<VkExtensionProperties> props(numExtensions);
    vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions,
                                           props.data());
    for (const auto& p : props) {
      fmt::print("{} (version={})\n", p.extensionName, p.specVersion);
    }
  }*/

  /////////////////////////////////
  // create a vulkan device
  VkDevice device;

  {
	  const char* deviceExts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    float queuePriorities[] = {0.0f};
    VkDeviceQueueCreateInfo qci[] = {
        {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0,
         queueFamilyIndex, 1, queuePriorities}};
    VkDeviceCreateInfo dci = {};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.enabledExtensionCount = 1;
    dci.ppEnabledExtensionNames = deviceExts;
    dci.pQueueCreateInfos = qci;
    dci.queueCreateInfoCount = 1;

    auto err = vkCreateDevice(mainPhysDev, &dci, nullptr, &device);
    fmt::print("vkCreateDevice returned {}\n", err);
  }

  /////////////////////////////////
  // open window
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(kWidth, kHeight, "Hello World", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  /////////////////////////////////
  // create surface
  VkSurfaceKHR surface;
  VkResult err = glfwCreateWindowSurface(instance, window, NULL, &surface);
  if (err != VK_SUCCESS) {
    fmt::print("Could not create a presentable surface\n");
    return -1;
  }

  /////////////////////////////////
  // Query supported swapchain formats
  VkFormat surfaceFormat;
  VkColorSpaceKHR surfaceColorSpace;
  {
    uint32_t surfaceFormatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(mainPhysDev, surface,
                                         &surfaceFormatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        mainPhysDev, surface, &surfaceFormatCount, surfaceFormats.data());
    fmt::print("Supported surface formats:\n");
    for (const auto& p : surfaceFormats) {
      fmt::print("- format={}, colorSpace={}\n", p.format, p.colorSpace);
    }
    assert(surfaceFormatCount != 0);
    // just choose the first one...
    surfaceFormat = surfaceFormats[0].format;
    surfaceColorSpace = surfaceFormats[0].colorSpace;
  }

  /////////////////////////////////
  // create swapchain
  VkSwapchainKHR swapchain;
  {
    uint32_t queueFamilyIndices[] = {queueFamilyIndex};
    VkSwapchainCreateInfoKHR swapCreateInfo{};
    swapCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapCreateInfo.surface = surface;
    swapCreateInfo.minImageCount = 2;
    swapCreateInfo.imageFormat = surfaceFormat;
    swapCreateInfo.imageColorSpace = surfaceColorSpace;
    swapCreateInfo.imageExtent = VkExtent2D{kWidth, kHeight};
    swapCreateInfo.queueFamilyIndexCount = 1;
    swapCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    swapCreateInfo.imageArrayLayers = 1;
	swapCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapCreateInfo.compositeAlpha =
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // whatever that means
    swapCreateInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;       // same
    swapCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // idem
    swapCreateInfo.preTransform =
        VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; // ibidem
	auto err = vkCreateSwapchainKHR(device, &swapCreateInfo, NULL, &swapchain);
	fmt::print("vkCreateSwapchainKHR returned {}\n", err);
  }

  // Again this should be properly enumerated
  /*VkImage images[4];
  uint32_t swapCount;
  vkGetSwapchainImagesKHR(dev, swap, &swapCount, images);

  // Synchronisation is needed here!
  uint32_t currentSwapImage;
  vkAcquireNextImageKHR(dev, swap, UINT64_MAX, presentCompleteSemaphore, NULL,
                        &currentSwapImage);

  // pass appropriate creation info to create view of image
  VkImageView backbufferView;
  vkCreateImageView(dev, &backbufferViewCreateInfo, NULL, &backbufferView);*/

  /*VkQueue queue;
  vkGetDeviceQueue(dev, 0, 0, &queue);*/

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
}
