set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1")

include("${VITASDK}/share/vita.cmake" REQUIRED)

add_compile_definitions(__VITA__)

set(VITA_APP_NAME "Minecraft PE")
set(VITA_TITLEID  "MCPE00000")
set(VITA_VERSION  "00.61")

set(VITA_ICON0 ${CMAKE_CURRENT_LIST_DIR}/sce_sys/icon0.png)
set(VITA_PIC0 ${CMAKE_CURRENT_LIST_DIR}/sce_sys/pic0.png)
set(VITA_BG ${CMAKE_CURRENT_LIST_DIR}/sce_sys/livearea/contents/bg.png)
set(VITA_GATE ${CMAKE_CURRENT_LIST_DIR}/sce_sys/livearea/contents/gate.png)

if(DEMO)
  set(VITA_APP_NAME "Minecraft PE Demo")
  set(VITA_TITLEID  "MCPEDEMO0")
  set(VITA_ICON0 ${CMAKE_CURRENT_LIST_DIR}/sce_sys/icon0_demo.png)
  set(VITA_BG ${CMAKE_CURRENT_LIST_DIR}/sce_sys/livearea/contents/bg_demo.png)
  set(VITA_GATE ${CMAKE_CURRENT_LIST_DIR}/sce_sys/livearea/contents/gate_demo.png)
endif()


target_sources(${PROJECT_NAME} PRIVATE
  ${HANDHELD}/platform/audio/SoundSystemVita.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/lib
)

target_link_libraries(${PROJECT_NAME} PUBLIC
  IMGEGL_stub_weak
  gpu_es4_ext_stub
  GLESv1_CM_stub_weak

  SceTouch_stub
  SceCtrl_stub
  SceDisplay_stub
  SceAudio_stub
  SceNgsUser_stub
  ScePower_stub
  SceNet_stub
  SceAppMgr_stub
  SceNetCtl_stub
  SceNpManager_stub
  SceRegistryMgr_stub
  SceIme_stub

  raknet
  png
  z
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME} UNSAFE)

set(VPK_ARGS "")
macro(add_assets GLOB)
  file(GLOB_RECURSE ASSET_FILES 
    RELATIVE "${ASSETS_DIR}" 
    "${ASSETS_DIR}/${GLOB}"
  )
  foreach(asset ${ASSET_FILES})
    set(asset_full "${ASSETS_DIR}/${asset}")
    list(APPEND VPK_ARGS "FILE" "${asset_full}" "data/${asset}")
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

vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  ${VPK_ARGS}

  FILE ${CMAKE_CURRENT_LIST_DIR}/module/libGLESv2.suprx                        module/libGLESv2.suprx
  FILE ${CMAKE_CURRENT_LIST_DIR}/module/libgpu_es4_ext.suprx                   module/libgpu_es4_ext.suprx
  FILE ${CMAKE_CURRENT_LIST_DIR}/module/libIMGEGL.suprx                        module/libIMGEGL.suprx
  FILE ${CMAKE_CURRENT_LIST_DIR}/module/libpvrPSP2_WSEGL.suprx                 module/libpvrPSP2_WSEGL.suprx
  FILE ${CMAKE_CURRENT_LIST_DIR}/module/libc.suprx                             sce_module/libc.suprx
  FILE ${CMAKE_CURRENT_LIST_DIR}/module/libfios2.suprx                         sce_module/libfios2.suprx
  FILE ${VITA_ICON0}                                                           sce_sys/icon0.png
  FILE ${VITA_PIC0}                                                            sce_sys/pic0.png
  FILE ${VITA_BG}                                                              sce_sys/livearea/contents/bg.png
  FILE ${VITA_GATE}                                                            sce_sys/livearea/contents/gate.png
  FILE ${CMAKE_CURRENT_LIST_DIR}/sce_sys/livearea/contents/template.xml        sce_sys/livearea/contents/template.xml
)
