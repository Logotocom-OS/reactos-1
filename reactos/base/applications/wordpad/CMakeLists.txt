
remove_definitions(-D_WIN32_IE=0x600)

add_definitions(-D__ROS_LONG64__)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)

list(APPEND SOURCE
    olecallback.c
    print.c
    registry.c
    wordpad.c
    rsrc.rc)

add_executable(wordpad ${SOURCE})

set_module_type(wordpad win32gui)
target_link_libraries(wordpad wine uuid)
add_importlibs(wordpad comdlg32 ole32 shell32 user32 gdi32 advapi32 comctl32 msvcrt kernel32 ntdll)
add_cd_file(TARGET wordpad DESTINATION reactos/system32 FOR all)
