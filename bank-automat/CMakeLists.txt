cmake_minimum_required(VERSION 3.19)
project(bank-automat LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

qt_standard_project_setup()

qt_add_executable(bank-automat
    WIN32 MACOSX_BUNDLE
    main.cpp
    MainWindow.cpp

    MainWindow.ui
    StartWindow.h StartWindow.cpp StartWindow.ui
    LoginDialog.h LoginDialog.cpp LoginDialog.ui
    MainWindow.h MainWindow.cpp MainWindow.ui
    ApiClient.h ApiClient.cpp


)

target_link_libraries(bank-automat PRIVATE Qt6::Widgets Qt6::Network)

include(GNUInstallDirs)

install(TARGETS bank-automat
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET bank-automat
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
