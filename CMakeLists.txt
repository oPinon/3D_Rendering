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

set( ResourceDir "${CMAKE_CURRENT_LIST_DIR}/deps/resources" )
file( MAKE_DIRECTORY ${ResourceDir} )

function( DownloadResource Url FilePath )
	if( NOT EXISTS ${FilePath} )
		message( STATUS "Downloading ${FilePath} from ${Url}" )
		file( DOWNLOAD ${Url} ${FilePath} SHOW_PROGRESS )
	endif()
endfunction( DownloadResource )

AddProject( Fractal3D )
target_link_libraries( Fractal3D Lodepng )
DownloadResource(
	"https://github.com/hughsk/matcap/raw/master/textures/00030.png"
	"${ResourceDir}/matcap.png"
)
install( FILES ${ResourceDir}/matcap.png DESTINATION ${InstallDir}/Fractal3D/ )

DownloadResource(
	"https://www.dropbox.com/s/lwt2jmnlvgca6kj/suzan.obj?dl=1"
	"${ResourceDir}/suzan.obj"
)

DownloadResource(
	"https://www.dropbox.com/s/5lql5h25kwqsc1n/suzanHair.obj?dl=1"
	"${ResourceDir}/suzanHair.obj"
)

AddProject( Hair )
install( FILES ${ResourceDir}/suzan.obj DESTINATION ${InstallDir}/Hair/ RENAME body.obj )
install( FILES ${ResourceDir}/suzanHair.obj DESTINATION ${InstallDir}/Hair/ RENAME hairLines.obj )

AddProject( DeferredRendering )
install( FILES ${ResourceDir}/suzan.obj DESTINATION ${InstallDir}/DeferredRendering/ )
