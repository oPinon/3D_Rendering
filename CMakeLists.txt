
cmake_minimum_required (VERSION 2.6)
project (3DRendering)

set(GlewDir "~/Libraries/Glew" CACHE PATH "desc")
set(GlutDir "~/Libraries/Glut" CACHE PATH "desc")

set (CMAKE_CXX_STANDARD 11)
link_directories( ${GlewDir}/lib ${GlutDir}/lib/x64 )
set(GlewLib "${GlewDir}/lib/Release/x64/glew32.lib" CACHE FILEPATH "desc")

include_directories( "${GlewDir}/include" "${GlutDir}/include" ${CMAKE_CURRENT_LIST_DIR}/src )
add_executable( 3DRendering ${CMAKE_CURRENT_LIST_DIR}/Volumetric/Volumetric/Main.cpp )

foreach( ShaderFile vert fragBack fragFront fragFront_Uniform )
	file( COPY ${CMAKE_CURRENT_LIST_DIR}/Volumetric/Volumetric/${ShaderFile}.glsl DESTINATION ${CMAKE_BINARY_DIR}/ )
endforeach( ShaderFile )

target_link_libraries( 3DRendering ${GlewLib} freeglut )
