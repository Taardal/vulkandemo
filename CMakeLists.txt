cmake_minimum_required(VERSION 3.20.2)
set(CMAKE_CXX_STANDARD 17)

project(vulkandemo)

set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(LIB_DIR ${PROJECT_SOURCE_DIR}/lib)
set(BIN_DIR ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_DIR ${PROJECT_SOURCE_DIR}/cmake)
set(ASSETS_DIR ${PROJECT_SOURCE_DIR}/assets)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE Debug)
endif ()

add_executable(
        ${PROJECT_NAME}
        ${SRC_DIR}/main.cpp
        ${SRC_DIR}/App.cpp
        ${SRC_DIR}/App.h
        ${SRC_DIR}/Assert.h
        ${SRC_DIR}/Environment.h
        ${SRC_DIR}/FileSystem.cpp
        ${SRC_DIR}/FileSystem.h
        ${SRC_DIR}/Log.cpp
        ${SRC_DIR}/Log.h
        ${SRC_DIR}/Vertex.cpp
        ${SRC_DIR}/Vertex.h
        ${SRC_DIR}/Vulkan.cpp
        ${SRC_DIR}/Vulkan.h
        ${SRC_DIR}/VulkanBuffer.cpp
        ${SRC_DIR}/VulkanBuffer.h
        ${SRC_DIR}/VulkanCommandPool.cpp
        ${SRC_DIR}/VulkanCommandPool.h
        ${SRC_DIR}/VulkanCommandBuffer.cpp
        ${SRC_DIR}/VulkanCommandBuffer.h
        ${SRC_DIR}/VulkanDevice.cpp
        ${SRC_DIR}/VulkanDevice.h
        ${SRC_DIR}/VulkanFramebuffer.cpp
        ${SRC_DIR}/VulkanFramebuffer.h
        ${SRC_DIR}/VulkanGraphicsPipeline.cpp
        ${SRC_DIR}/VulkanGraphicsPipeline.h
        ${SRC_DIR}/VulkanPhysicalDevice.cpp
        ${SRC_DIR}/VulkanPhysicalDevice.h
        ${SRC_DIR}/VulkanRenderPass.cpp
        ${SRC_DIR}/VulkanRenderPass.h
        ${SRC_DIR}/VulkanShader.cpp
        ${SRC_DIR}/VulkanShader.h
        ${SRC_DIR}/VulkanSwapChain.cpp
        ${SRC_DIR}/VulkanSwapChain.h
        ${SRC_DIR}/Window.cpp
        ${SRC_DIR}/Window.h
)

set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES
        RUNTIME_OUTPUT_NAME ${PROJECT_NAME}
        RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BIN_DIR}/debug
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BIN_DIR}/release
)

add_custom_command(
        TARGET ${PROJECT_NAME}
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_DIR}/pre_build.cmake
)

add_custom_command(
        TARGET ${PROJECT_NAME}
        PRE_LINK
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_DIR}/pre_link.cmake
)

add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -DBIN_DIR=${BIN_DIR} -DASSETS_DIR=${ASSETS_DIR} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -P ${CMAKE_DIR}/post_build.cmake
)

install(
        TARGETS ${PROJECT_NAME}
        DESTINATION ${BIN_DIR}
)

option(
        BUILD_GLFW_SRC
        "Build GLFW as part of this project instead of using binaries installed on local machine"
        OFF
)
if (${BUILD_GLFW_SRC})
    set(GLFW_DIR ${LIB_DIR}/glfw)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(${GLFW_DIR})
    target_compile_definitions(${PROJECT_NAME} PUBLIC GLFW_INCLUDE_NONE)
    target_include_directories(${PROJECT_NAME} PUBLIC ${GLFW_DIR}/include)
    target_link_libraries(${PROJECT_NAME} glfw ${GLFW_LIBRARIES})
else ()
    set(GLFW_DIR $ENV{GLFW_HOME})
    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${GLFW_DIR})
    find_package(glfw3 REQUIRED)
    target_link_libraries(${PROJECT_NAME} glfw)
endif ()

set(GLM_DIR ${LIB_DIR}/glm)
target_include_directories(${PROJECT_NAME} PUBLIC ${GLM_DIR})

set(SPDLOG_DIR ${LIB_DIR}/spdlog)
target_include_directories(${PROJECT_NAME} PUBLIC ${SPDLOG_DIR}/include)

find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)