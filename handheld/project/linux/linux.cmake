add_compile_definitions(LINUX)

if(ASAN)
  target_compile_options(mcpe INTERFACE -fsanitize=address,undefined)
  target_link_options(mcpe INTERFACE -fsanitize=address,undefined)
  target_compile_options(mcpe INTERFACE -g -fno-omit-frame-pointer)
endif()

#target_compile_options(mcpe PUBLIC -Wl,--wrap=malloc -Wl,--wrap=free)
#target_link_options(mcpe PUBLIC -Wl,--wrap=malloc -Wl,--wrap=free)

set(GLEW_USE_STATIC_LIBS TRUE)
find_package(OpenGL REQUIRED COMPONENTS OpenGL EGL)
find_package(OpenAL REQUIRED)
find_package(glfw3 REQUIRED)

target_sources(mcpe_client PRIVATE
  ${HANDHELD}/platform/audio/SoundSystemAL.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/gl.c
)

target_include_directories(mcpe INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(mcpe_client PUBLIC
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
