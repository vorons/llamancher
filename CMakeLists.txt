cmake_minimum_required(VERSION 3.25)
project(llamancher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ── Optimisation flags ──────────────────────────────────────────────────────
if(CMAKE_BUILD_TYPE MATCHES "^Release$|^MinSizeRel$")
  add_compile_options(-O2 -flto=auto -fvisibility=hidden -fomit-frame-pointer)
  add_compile_options(-fdata-sections -ffunction-sections)
  add_link_options(-flto=auto -Wl,--gc-sections -Wl,--as-needed)
elseif(CMAKE_BUILD_TYPE MATCHES "^RelWithDebInfo$")
  add_compile_options(-O2 -flto=auto -fvisibility=hidden -g)
  add_compile_options(-fdata-sections -ffunction-sections)
  add_link_options(-flto=auto -Wl,--gc-sections -Wl,--as-needed)
else()
  add_compile_options(-O0 -g0)
endif()

# ── Saucer ──────────────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
  saucer
  GIT_REPOSITORY "https://github.com/saucer/saucer"
  GIT_TAG        v8.0.5
)
set(FETCHCONTENT_QUIET OFF)
FetchContent_MakeAvailable(saucer)

# ── GTK4 (native file dialogs) ─────────────────────────────────────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4>=4.12 IMPORTED_TARGET)

# ── Backend executable ──────────────────────────────────────────────────────
add_executable(llamancher
  backend/main.cpp
  backend/model_manager.cpp
  backend/server_manager.cpp
  backend/settings.cpp
  backend/preset.cpp
)

target_include_directories(llamancher PRIVATE backend)

# ── Frontend build (Vite) ───────────────────────────────────────────────────
find_program(PNPM pnpm)
if(PNPM)
  add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/frontend/dist/index.html"
    COMMAND ${PNPM} install
    COMMAND ${PNPM} run build
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/frontend"
    COMMENT "Building Svelte frontend with Vite …"
  )

  # Embed the built frontend into the binary
  saucer_embed("${CMAKE_SOURCE_DIR}/frontend/dist" TARGET llamancher)
  target_link_libraries(llamancher PRIVATE saucer saucer::embedded PkgConfig::GTK4)
else()
  message(WARNING "pnpm not found — frontend will not be built automatically")
  target_link_libraries(llamancher PRIVATE saucer PkgConfig::GTK4)
endif()
