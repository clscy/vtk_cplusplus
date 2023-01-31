cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(testVtkAffineWidget)

find_package(VTK COMPONENTS
  CommonColor
  CommonCore
  CommonDataModel
  CommonTransforms
  FiltersCore
  FiltersSources
  InteractionStyle
  InteractionWidgets
  RenderingContextOpenGL2
  RenderingCore
  RenderingFreeType
  RenderingGL2PSOpenGL2
  RenderingOpenGL2
  RenderingAnnotation
)


if (NOT VTK_FOUND)
  message(FATAL_ERROR "testVtkAffineWidget: Unable to find the VTK build folder.")
endif()

set(SOURCES
    testVtkAffineWidget.cxx
    vtkClsAffineWidget.cxx
    vtkClsRepresentation2D.cxx
)

set(HEADER
    vtkClsAffineWidget.h
    vtkClsRepresentation2D.h
)

# Prevent a "command line is too long" failure in Windows.
add_executable(testVtkAffineWidget ${SOURCES} ${HEADER})

target_link_libraries(testVtkAffineWidget ${VTK_LIBRARIES}
)
# vtk_module_autoinit is needed
vtk_module_autoinit(
  TARGETS testVtkAffineWidget
  MODULES ${VTK_LIBRARIES}
)