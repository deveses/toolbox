set(SOURCES 
    include/SPSCData.h
    src/SPSCData.cpp
)

add_library(ToolboxLib OBJECT ${SOURCES})

target_include_directories(ToolboxLib PUBLIC include)
target_compile_features(ToolboxLib PUBLIC cxx_std_17)
target_compile_options(ToolboxLib PRIVATE -fsanitize=thread)
target_link_options(ToolboxLib PRIVATE -fsanitize=thread)