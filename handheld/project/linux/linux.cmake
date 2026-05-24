add_definitions(-DLINUX)

if(ASAN)
  add_compile_options(-fsanitize=address,undefined)
  add_link_options(-fsanitize=address,undefined)
  add_compile_options(-g -fno-omit-frame-pointer)
endif()

set(GLEW_USE_STATIC_LIBS TRUE)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED COMPONENTS OpenGL EGL)
find_package(OpenAL REQUIRED)
find_package(glfw3 REQUIRED)

target_sources(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/src/gl.c
  ${HANDHELD}/platform/audio/SoundSystemAL.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/../linux/include
)

target_link_libraries(${PROJECT_NAME} PUBLIC
  GLEW::GLEW
  OpenGL::GL
  OpenGL::EGL
  OpenAL::OpenAL
  glfw
)

macro(add_assets GLOB)
  file(GLOB_RECURSE ASSET_FILES 
    RELATIVE "${ASSETS_DIR}" 
    "${ASSETS_DIR}/${GLOB}"
  )
  foreach(asset ${ASSET_FILES})
    set(asset_full "${ASSETS_DIR}/${asset}")
    set(out_full "${CMAKE_CURRENT_BINARY_DIR}/data/${asset}")
    get_filename_component(out_dir ${out_full} DIRECTORY)
    file(COPY ${asset_full} DESTINATION "${out_dir}")
    message(NOTICE "${asset}")
  endforeach()
endmacro()

add_assets(images/terrain.png)
add_assets(images/particles.png)
add_assets(images/armor/*)
add_assets(images/art/*)
add_assets(images/environment/*)
add_assets(images/font/*)
add_assets(images/gui/*)
add_assets(images/item/*)
add_assets(images/mob/*)
add_assets(fonts/*)
add_assets(lang/*)
