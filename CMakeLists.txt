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

function( AddProject Folder )
	file( GLOB ${Folder}Src
		"${SrcDir}/${Folder}/*.cpp"
		"${SrcDir}/${Folder}/*.h"
	)
	add_executable( ${Folder} ${${Folder}Src} )
	target_link_libraries( ${Folder} 3DRendering )
	install( TARGETS ${Folder} DESTINATION ${InstallDir}/${Folder} )
	file( GLOB ShaderFiles "${SrcDir}/${Folder}/*.glsl" )
	install( FILES ${ShaderFiles} DESTINATION ${InstallDir}/${Folder} )
endfunction( AddProject )

AddProject( Volumetric False )

## Lodepng (dependency)
add_library( Lodepng
	${CMAKE_CURRENT_LIST_DIR}/deps/lodepng/lodepng.cpp
	${CMAKE_CURRENT_LIST_DIR}/deps/lodepng/lodepng.h
)
target_include_directories( Lodepng PUBLIC
	${CMAKE_CURRENT_LIST_DIR}/deps/lodepng/
)

AddProject( Fractal3D )
target_link_libraries( Fractal3D Lodepng )

AddProject( Hair )

AddProject( DeferredRendering )
