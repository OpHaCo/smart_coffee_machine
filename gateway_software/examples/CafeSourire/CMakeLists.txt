cmake_minimum_required(VERSION 2.8)
set(CMAKE_PREFIX_PATH ../raspicam/build)
project( CafeSourire )
find_package( OpenCV REQUIRED )

set(CMAKE_BUILD_TYPE Debug)
set(SOURCE client.cpp client.h base64.cpp base64.h smile_rasp.cpp)
add_executable(smile_detect ${SOURCE})

find_path(RPI_TARGET bcm_host.h PATHS /opt/vc/include/ NO_DEFAULT_PATH)
if(RPI_TARGET)
      	MESSAGE(STATUS "Prepare archlinux raspberry target") 
	find_package( raspicam REQUIRED )
	target_link_libraries( smile_detect ${OpenCV_LIBS} ${raspicam_CV_LIBS} mosquittopp)
	add_definitions(-DUSE_RASPICAM)
else()
	MESSAGE(STATUS "Prepare non-raspberry target")
	target_link_libraries( smile_detect ${OpenCV_LIBS} mosquittopp)
endif()



