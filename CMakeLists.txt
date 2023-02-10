cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(testVtkBoxWidget)

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
  message(FATAL_ERROR "testVtkBoxWidget: Unable to find the VTK build folder.")
endif()

set(SOURCES
    testVtkBoxWidget.cxx
    # vtkClsAffineWidget.cxx
    # vtkClsRepresentation2D.cxx
    vtkWxxBoxWidget2.cxx
    vtkWxxBoxRepresentation.cxx
    # psiPlanBoxWidget2.cxx
    # psiPlanBoxRepresentation.cxx
)

set(HEADER
    # vtkClsAffineWidget.h
    # vtkClsRepresentation2D.h
    vtkWxxBoxWidget2.h
    vtkWxxBoxRepresentation.h
    # psiPlanBoxWidget2.h
    # psiPlanBoxRepresentation.h
)

# Prevent a "command line is too long" failure in Windows.
add_executable(testVtkBoxWidget ${SOURCES} ${HEADER})

target_link_libraries(testVtkBoxWidget ${VTK_LIBRARIES}
)
# vtk_module_autoinit is needed
vtk_module_autoinit(
  TARGETS testVtkBoxWidget
  MODULES ${VTK_LIBRARIES}
)