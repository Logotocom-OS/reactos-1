
spec2def(dllexport_test_dll1.dll dllexport_test_dll1.spec ADD_IMPORTLIB)
spec2def(dllexport_test_dll2.dll dllexport_test_dll2.spec ADD_IMPORTLIB)

set(baseaddress_dllexport_test_dll1 0x1000000)
add_library(dllexport_test_dll1 SHARED
    dllexport_test_dll1.c
    ${CMAKE_CURRENT_BINARY_DIR}/dllexport_test_dll1.def)
set_module_type(dllexport_test_dll1 win32dll ENTRYPOINT 0)
add_importlibs(dllexport_test_dll1 dllexport_test_dll2)

set(baseaddress_dllexport_test_dll2 0x2000000)
add_library(dllexport_test_dll2 SHARED
    dllexport_test_dll2.c
    ${CMAKE_CURRENT_BINARY_DIR}/dllexport_test_dll2.def)
set_module_type(dllexport_test_dll2 win32dll ENTRYPOINT 0)

add_executable(dllexport_test
    dllexport_test.c)

set_module_type(dllexport_test win32cui)
add_importlibs(dllexport_test dllexport_test_dll1 msvcrt kernel32 ntdll)

if(MSVC)
    add_compile_flags("/Zc:wchar_t-")
endif()

add_library(dllimport_test SHARED
    dllimport_framedyn.cpp)
set_module_type(dllimport_test win32dll ENTRYPOINT 0)
add_importlibs(dllimport_test framedyn)
if(NOT MSVC)
    target_link_libraries(dllimport_test framedynex)
endif()
