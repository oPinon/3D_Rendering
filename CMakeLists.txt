cmake_minimum_required (VERSION 2.6)
project (3DRendering)

set (CMAKE_CXX_STANDARD 11)

set(GlewIDir "/usr/include" CACHE PATH "desc")
set(GlutIDir "/usr/include" CACHE PATH "desc")

set( InstallDir "${CMAKE_BINARY_DIR}/install" CACHE PATH "" )

function( CheckExists Var )
	if( NOT EXISTS ${${Var}} )
		message( SEND_ERROR "Please specify ${Var}" )
	endif()
endfunction( CheckExists )

CheckExists( GlewIDir )
CheckExists( GlutIDir )

set( SrcDir "${CMAKE_CURRENT_LIST_DIR}/src" )

add_library( 3DRendering
	"${SrcDir}/GlewGlut.h"
	"${SrcDir}/Vec.h"
	"${SrcDir}/Mesh.h"
	"${SrcDir}/Mesh.cpp"
)

target_include_directories( 3DRendering PUBLIC
	${GlewIDir}
	${GlutIDir}
	${SrcDir}
)

set( Plsfx "so" )
if( WIN32 )
	set( Plsfx "lib" )
endif()

set( GlewLibPath "/usr/lib/glew32.${Plsfx}" CACHE FILEPATH "" )
CheckExists( GlewLibPath )
set( GlutLibPath "/usr/lib/freeglut.${Plsfx}" CACHE FILEPATH "" )
CheckExists( GlutLibPath )

target_link_libraries( 3DRendering PUBLIC
	${GlewLibPath}
	${GlutLibPath}
)

if( NOT WIN32 )
	set( GlLibPath "/usr/lib/libGL.${Plsfx}" CACHE FILEPATH "" )
	CheckExists( GlLibPath )
	set( GluLibPath "/usr/lib/libGLU.${Plsfx}" CACHE FILEPATH "" )
	CheckExists( GluLibPath )
	target_link_libraries( 3DRendering PUBLIC
		${GlLibPath}
		${GlutLibPath}
	)
endif()

function( AddProject Folder )
	file( GLOB ${Folder}Src
		"${SrcDir}/${Folder}/*.cpp"
		"${SrcDir}/${Folder}/*.h"
	)
	add_executable( ${Folder} ${${Folder}Src} )
	target_link_libraries( ${Folder} 3DRendering )
	install( TARGETS ${Folder} DESTINATION ${InstallDir}/${Folder} )
	file( GLOB ShaderFiles "${SrcDir}/${Folder}/*.glsl" )
	target_sources( ${Folder} PRIVATE "${ShaderFiles}" )
	install( FILES ${ShaderFiles} DESTINATION ${InstallDir}/${Folder} )
	set_target_properties( ${Folder} PROPERTIES # HACK ?
		VS_DEBUGGER_WORKING_DIRECTORY ${InstallDir}/${Folder} )
endfunction( AddProject )

AddProject( Volumetric )

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

AddProject( 2DTiles )

AddProject( ParticleLandscape )
target_link_libraries( ParticleLandscape Lodepng )
install( FILES ${ResourceDir}/particleLandscape.png DESTINATION ${InstallDir}/ParticleLandscape/ )
