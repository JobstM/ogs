######################
### Find tools     ###
######################

# Find dot tool from graphviz
FIND_PROGRAM(DOT_TOOL_PATH dot DOC "Dot tool from graphviz")

# Find doxygen
FIND_PACKAGE(Doxygen)

# Find gnu profiler gprof
FIND_PROGRAM(GPROF_PATH gprof DOC "GNU profiler gprof")

FIND_PACKAGE(cppcheck)

######################
### Find libraries ###
######################

# Clang does not have OpenMP support atm, see https://github.com/ufz/ogs/issues/8
IF(NOT COMPILER_IS_CLANG)
	FIND_PACKAGE(OpenMP)
ENDIF () # !clang
IF(OPENMP_FOUND)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
ENDIF()

FIND_PACKAGE(Metis)

## Qt4 library ##
IF(NOT OGS_DONT_USE_QT)
	FIND_PACKAGE( Qt4 4.7)
ENDIF(NOT OGS_DONT_USE_QT)

IF ( QT4_FOUND )
	# Enable more modules
	SET(QT_USE_QTOPENGL TRUE)
	SET(QT_USE_QTSQL TRUE)
	SET(QT_USE_QTTEST TRUE)
	SET(QT_USE_QTXML TRUE)
	SET(QT_USE_QTXMLPATTERNS TRUE)
	INCLUDE( ${QT_USE_FILE} )
	ADD_DEFINITIONS(${QT_DEFINITIONS})
ENDIF (QT4_FOUND )

## pthread ##
SET ( CMAKE_THREAD_PREFER_PTHREAD ON )
FIND_PACKAGE ( Threads )
IF ( CMAKE_USE_PTHREADS_INIT )
	SET (HAVE_PTHREADS TRUE)
	ADD_DEFINITIONS(-DHAVE_PTHREADS)
ENDIF (CMAKE_USE_PTHREADS_INIT )

# blas
FIND_PACKAGE ( BLAS )

# lapack
FIND_PACKAGE ( LAPACK )

## VTK ##
IF (OGS_LIBS_DIR_FOUND)
	SET (VTK_DIR ${PROJECT_SOURCE_DIR}/../Libs/VTK/build)
ENDIF () # OGS_LIBS_DIR_FOUND
IF(NOT OGS_DONT_USE_VTK)
	FIND_PACKAGE( VTK )
ENDIF()
IF(VTK_FOUND)
	ADD_DEFINITIONS(-DVTK_FOUND)
	FIND_PACKAGE(QVTK)
	IF(NOT QVTK_FOUND AND OGS_BUILD_GUI)
		MESSAGE(FATAL_ERROR "QVTK was not found but is required for OGS_BUILD_GUI! On Ubuntu it can be installed via 'sudo apt-get install libvtk5-qt4-dev'")
	ENDIF()
ENDIF()

## NetCDF ##
IF(VTK_FOUND)
	FIND_PATH(VTK_NETCDF_FOUND netcdf.h
		PATHS ${VTK_INCLUDE_DIRS}/vtknetcdf ${VTK_SOURCE_DIR}/Utilities/vtknetcdf
		PATH_SUFFIXES include
		NO_DEFAULT_PATH)
ENDIF()

IF(VTK_NETCDF_FOUND)
	ADD_DEFINITIONS(-DVTK_NETCDF_FOUND)
	INCLUDE_DIRECTORIES(${VTK_NETCDF_FOUND} ${VTK_DIR}/Utilities ${VTK_NETCDF_FOUND}/../..)
ELSE()
	SET(NETCDF_CXX TRUE)
	FIND_PACKAGE(NetCDF)
	IF(NOT NETCDF_FOUND AND OGS_BUILD_GUI)
		MESSAGE(FATAL_ERROR "NetCDF was not found but is required for OGS_BUILD_GUI!")
	ENDIF()
ENDIF()

## geotiff ##
IF(NOT MSVC)
	FIND_PACKAGE( LibTiff )
ENDIF() # NOT MSVC
FIND_PACKAGE( LibGeoTiff )
IF(libgeotiff_FOUND)
	ADD_DEFINITIONS(-Dlibgeotiff_FOUND)
ENDIF() # libgeotiff_FOUND
