
set(libraries glad glfw assimp imgui itugl ${APPLE_LIBRARIES})

file(GLOB_RECURSE target_inc "*.h" )
file(GLOB_RECURSE target_src "*.cpp" )

file(GLOB_RECURSE shaders "*.vert" "*.frag" "*.geom" "*.glsl")
source_group("Shaders" FILES ${shaders})

add_executable(${TARGETNAME} ${target_inc} ${target_src} ${shaders})
target_link_libraries(${TARGETNAME} ${libraries})
add_custom_command(TARGET ${TARGETNAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:${TARGETNAME}>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/models"
        "$<TARGET_FILE_DIR:${TARGETNAME}>/models"