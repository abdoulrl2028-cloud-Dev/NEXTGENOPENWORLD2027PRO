# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)

project("nextgenopenworld2027pro")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT NG_THIRD_PARTY)
    set(NG_THIRD_PARTY "${CMAKE_CURRENT_SOURCE_DIR}/../../../../third_party")
endif()

# Creates your game shared library. The name must be the same as the
# one used for loading in your Kotlin/Java or AndroidManifest.txt files.
add_library(nextgenopenworld2027pro SHARED
        main.cpp
        AndroidOut.cpp
        Renderer.cpp
        Shader.cpp
        TextureAsset.cpp
        Utility.cpp
        core/AssetFile.cpp
        graphics/Camera.cpp
        graphics/EnemyCrowd.cpp
        graphics/GltfLoader.cpp
        graphics/GlslProgram.cpp
        graphics/SceneShader.cpp
        graphics/SkyPass.cpp
        graphics/StaticMesh.cpp
        ui/BitmapFont.cpp
        ui/MainMenuScene.cpp
        ui/MenuAudio.cpp
        ui/SceneManager.cpp
        ui/SettingsStore.cpp
        ui/TextureLoader.cpp
        ui/UIButton.cpp
        ui/UiCanvas.cpp)

target_include_directories(nextgenopenworld2027pro PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${NG_THIRD_PARTY})

# Searches for a package provided by the game activity dependency
find_package(game-activity REQUIRED CONFIG)

# Forces the linker to keep the JNI entry point for GameActivity
set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -u Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")

# Configure libraries CMake uses to link your target library.
target_link_libraries(nextgenopenworld2027pro
        # The game activity
        game-activity::game-activity_static

        # EGL and other dependent libraries required for drawing
        # and interacting with Android system
        EGL
        GLESv3
        jnigraphics
        android
        aaudio
        log)