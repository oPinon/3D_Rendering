cmake_minimum_required (VERSION 2.6)
project (3DRendering)

set (CMAKE_CXX_STANDARD 11)

set(GlewDir "~/Libraries/Glew" CACHE PATH "desc")
set(GlutDir "~/Libraries/Glut" CACHE PATH "desc")

set( InstallDir "${CMAKE_BINARY_DIR}/install" CACHE PATH "" )

if( NOT EXISTS ${GlewDir}/include )
	message( SEND_ERROR "Please specify GlewDir" )
endif()

if( NOT EXISTS ${GlutDir}/include )
	message( SEND_ERROR "Please specify GlutDir" )
endif()

set( SrcDir "${CMAKE_CURRENT_LIST_DIR}/src" )

add_library( 3DRendering
	"${SrcDir}/GlewGlut.h"
	"${SrcDir}/Mesh.h"
	"${SrcDir}/Mesh.cpp"
)

target_include_directories( 3DRendering PUBLIC
	${GlewDir}/include
	${GlutDir}/include
	${SrcDir}
)

target_link_libraries( 3DRendering PUBLIC
	${GlewDir}/lib/Release/x64/glew32.lib
	${GlutDir}/lib/x64/freeglut.lib
)

## Volumetric ##
file( GLOB VolumetricSrc
	"${SrcDir}/Volumetric/*.cpp"
	"${SrcDir}/Volumetric/*.h"
)
add_executable( Volumetric ${VolumetricSrc} )
target_link_libraries( Volumetric 3DRendering )
install( TARGETS Volumetric DESTINATION ${InstallDir}/Volumetric )
file( GLOB ShaderFiles "${SrcDir}/Volumetric/*.glsl" )
install( FILES ${ShaderFiles} DESTINATION ${InstallDir}/Volumetric )

## Lodepng (dependency)
add_library( Lodepng
	${CMAKE_CURRENT_LIST_DIR}/deps/lodepng/lodepng.cpp
	${CMAKE_CURRENT_LIST_DIR}/deps/lodepng/lodepng.h
)
target_include_directories( Lodepng PUBLIC
	${CMAKE_CURRENT_LIST_DIR}/deps/lodepng/
)

## Fractal 3D ##
file( GLOB Fractal3DSrc
	${SrcDir}/Fractal3D/*.h
	${SrcDir}/Fractal3D/*.cpp
)
add_executable( Fractal3D ${Fractal3DSrc} )
target_link_libraries( Fractal3D
	3DRendering
	Lodepng
)
install( TARGETS Fractal3D DESTINATION ${InstallDir}/Fractal3D )
file( GLOB ShaderFiles "${SrcDir}/Fractal3D/*.glsl" )
install( FILES ${ShaderFiles} DESTINATION ${InstallDir}/Fractal3D )

## Hair ## 
file( GLOB HairSrc
	${SrcDir}/Hair/*.h
	${SrcDir}/Hair/*.cpp
)
add_executable( Hair ${HairSrc} )
target_link_libraries( Hair
	3DRendering
	Lodepng
)
install( TARGETS Hair DESTINATION ${InstallDir}/Hair )
file( GLOB ShaderFiles "${SrcDir}/Hair/*.glsl" )
install( FILES ${ShaderFiles} DESTINATION ${InstallDir}/Hair )

## Deferred rendering
file( GLOB DeferredRenderingSrc
	${SrcDir}/DeferredRendering/*.h
	${SrcDir}/DeferredRendering/*.cpp
)
add_executable( DeferredRendering ${DeferredRenderingSrc} )
target_link_libraries( DeferredRendering
	3DRendering
	Lodepng
)
install( TARGETS DeferredRendering DESTINATION ${InstallDir}/DeferredRendering )
file( GLOB ShaderFiles "${SrcDir}/DeferredRendering/*.glsl" )
install( FILES ${ShaderFiles} DESTINATION ${InstallDir}/DeferredRendering )

# HACK / TODO : function for all the similar targets
