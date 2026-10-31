set(NIUMA_LEGACY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/legacy_v2")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(NIUMA_LEGACY_CORE_SOURCES
    legacy_v2/application/achievement/AchievementUseCase.cpp
    legacy_v2/application/maintenance/MainWindowMaintenanceUseCase.cpp
    legacy_v2/application/maintenance/StartupUseCase.cpp
    legacy_v2/application/maintenance/ScanImportUseCase.cpp
    legacy_v2/application/recyclebin/RecycleBinUseCase.cpp
    legacy_v2/application/settings/SettingsUseCase.cpp
    legacy_v2/application/stats/StatsUseCase.cpp
    legacy_v2/application/reference/ReferenceCommandUseCase.cpp
    legacy_v2/application/reference/ReferenceQueryUseCase.cpp
    legacy_v2/application/task/TaskCommandUseCase.cpp
    legacy_v2/application/periodic/PeriodicTaskUseCase.cpp
    legacy_v2/application/home/HomeCommandUseCase.cpp
    legacy_v2/application/home/HomeDashboardUseCase.cpp
    legacy_v2/application/task/TaskDetailUseCase.cpp
    legacy_v2/application/task/TaskFormUseCase.cpp
    legacy_v2/application/task/TaskQueryUseCase.cpp
    legacy_v2/database/AchievementDao.cpp
    legacy_v2/database/DatabaseManager.cpp
    legacy_v2/database/HandlerDao.cpp
    legacy_v2/database/OperationLogDao.cpp
    legacy_v2/database/PeriodicTaskDao.cpp
    legacy_v2/database/ReferenceMaterialDao.cpp
    legacy_v2/database/SettingsDao.cpp
    legacy_v2/database/SourceDictDao.cpp
    legacy_v2/database/TagDictDao.cpp
    legacy_v2/database/TaskDao.cpp
    legacy_v2/database/TaskRemarkDao.cpp
    legacy_v2/models/ExportConfig.cpp
    legacy_v2/models/PeriodicTask.cpp
    legacy_v2/models/Task.cpp
    legacy_v2/models/TaskFilter.cpp
    legacy_v2/models/TaskTableModel.cpp
    legacy_v2/service/BackupService.cpp
    legacy_v2/service/FileService.cpp
    legacy_v2/service/FocusService.cpp
    legacy_v2/service/PeriodicTaskService.cpp
    legacy_v2/service/ReferenceInfoFile.cpp
    legacy_v2/service/ScanMatchService.cpp
    legacy_v2/service/TaskService.cpp
    legacy_v2/service/TaskInfoFile.cpp
    legacy_v2/ui/AchievementDialog.cpp
    legacy_v2/ui/AchievementView.cpp
    legacy_v2/ui/ColorPalette.cpp
    legacy_v2/ui/HomeView.cpp
    legacy_v2/ui/MainWindow.cpp
    legacy_v2/ui/PeriodicTaskCreateDialog.cpp
    legacy_v2/ui/PeriodicTaskView.cpp
    legacy_v2/ui/RecycleBinView.cpp
    legacy_v2/ui/ReferenceCreateDialog.cpp
    legacy_v2/ui/ReferenceDetailDialog.cpp
    legacy_v2/ui/ReferenceEditDialog.cpp
    legacy_v2/ui/ReferenceView.cpp
    legacy_v2/ui/ScanImportDialog.cpp
    legacy_v2/ui/AppTheme.cpp
    legacy_v2/ui/SettingsView.cpp
    legacy_v2/ui/StatsView.cpp
    legacy_v2/ui/TaskCreateDialog.cpp
    legacy_v2/ui/TaskDetailView.cpp
    legacy_v2/ui/TaskEditDialog.cpp
    legacy_v2/ui/TaskListView.cpp
    legacy_v2/ui/widgets/DragDropArea.cpp
    legacy_v2/ui/widgets/HeatmapWidget.cpp
    legacy_v2/ui/widgets/PaginationWidget.cpp
    legacy_v2/ui/widgets/SidebarDelegate.cpp
    legacy_v2/ui/widgets/StyledMessageBox.cpp
    legacy_v2/ui/widgets/TaskItemDelegate.cpp
    legacy_v2/ui/widgets/TitleBar.cpp
    legacy_v2/ui/widgets/ToastWidget.cpp
    legacy_v2/ui/widgets/TrendChart.cpp
    legacy_v2/utils/AmountUtil.cpp
    legacy_v2/utils/DateUtil.cpp
    legacy_v2/utils/FileUtil.cpp
    legacy_v2/utils/Logger.cpp
    legacy_v2/utils/PathUtil.cpp
    legacy_v2/utils/TaskNumberGenerator.cpp
    legacy_v2/utils/WordExport.cpp)

set(NIUMA_LEGACY_BACKEND_SOURCES
    legacy_v2/application/achievement/AchievementUseCase.cpp
    legacy_v2/application/maintenance/MainWindowMaintenanceUseCase.cpp
    legacy_v2/application/maintenance/StartupUseCase.cpp
    legacy_v2/application/maintenance/ScanImportUseCase.cpp
    legacy_v2/application/recyclebin/RecycleBinUseCase.cpp
    legacy_v2/application/settings/SettingsUseCase.cpp
    legacy_v2/application/stats/StatsUseCase.cpp
    legacy_v2/application/reference/ReferenceCommandUseCase.cpp
    legacy_v2/application/reference/ReferenceQueryUseCase.cpp
    legacy_v2/application/task/TaskCommandUseCase.cpp
    legacy_v2/application/periodic/PeriodicTaskUseCase.cpp
    legacy_v2/application/home/HomeCommandUseCase.cpp
    legacy_v2/application/home/HomeDashboardUseCase.cpp
    legacy_v2/application/task/TaskDetailUseCase.cpp
    legacy_v2/application/task/TaskFormUseCase.cpp
    legacy_v2/application/task/TaskQueryUseCase.cpp
    legacy_v2/database/AchievementDao.cpp
    legacy_v2/database/DatabaseManager.cpp
    legacy_v2/database/HandlerDao.cpp
    legacy_v2/database/OperationLogDao.cpp
    legacy_v2/database/PeriodicTaskDao.cpp
    legacy_v2/database/ReferenceMaterialDao.cpp
    legacy_v2/database/SettingsDao.cpp
    legacy_v2/database/SourceDictDao.cpp
    legacy_v2/database/TagDictDao.cpp
    legacy_v2/database/TaskDao.cpp
    legacy_v2/database/TaskRemarkDao.cpp
    legacy_v2/models/ExportConfig.cpp
    legacy_v2/models/PeriodicTask.cpp
    legacy_v2/models/Task.cpp
    legacy_v2/models/TaskFilter.cpp
    legacy_v2/service/BackupService.cpp
    legacy_v2/service/FileService.cpp
    legacy_v2/service/FocusService.cpp
    legacy_v2/service/PeriodicTaskService.cpp
    legacy_v2/service/ReferenceInfoFile.cpp
    legacy_v2/service/ScanMatchService.cpp
    legacy_v2/service/TaskService.cpp
    legacy_v2/service/TaskInfoFile.cpp
    legacy_v2/utils/AmountUtil.cpp
    legacy_v2/utils/DateUtil.cpp
    legacy_v2/utils/FileUtil.cpp
    legacy_v2/utils/Logger.cpp
    legacy_v2/utils/PathUtil.cpp
    legacy_v2/utils/TaskNumberGenerator.cpp
    legacy_v2/utils/WordExport.cpp)

find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets Sql Concurrent Svg)
if(WIN32)
    add_library(NiumaV3WindowsChromeSupport STATIC legacy_v2/ui/widgets/WindowsChrome.cpp)
    target_link_libraries(NiumaV3WindowsChromeSupport PRIVATE Qt5::Widgets Qt5::GuiPrivate)
    target_compile_definitions(NiumaV3WindowsChromeSupport PRIVATE WINVER=0x0601 _WIN32_WINNT=0x0601)
endif()
add_executable(NiumaV3IconAssetTool tools/generate_app_icon.cpp)
target_link_libraries(NiumaV3IconAssetTool PRIVATE Qt5::Svg Qt5::Gui)
set(NIUMA_GENERATED_ICON "${CMAKE_CURRENT_BINARY_DIR}/generated/app-icon.ico")
add_custom_command(OUTPUT "${NIUMA_GENERATED_ICON}"
    COMMAND NiumaV3IconAssetTool "${NIUMA_LEGACY_ROOT}/resources/icons/app-icon.svg" "${NIUMA_GENERATED_ICON}"
    DEPENDS NiumaV3IconAssetTool legacy_v2/resources/icons/app-icon.svg
    VERBATIM)
add_custom_target(NiumaV3ProductIcon DEPENDS "${NIUMA_GENERATED_ICON}")
set_source_files_properties(legacy_v2/resources/app.rc PROPERTIES OBJECT_DEPENDS "${NIUMA_GENERATED_ICON}")

add_executable(NiumaV3LegacyBaseline WIN32
    legacy_v2/src/main.cpp
    ${NIUMA_LEGACY_CORE_SOURCES}
    legacy_v2/resources/resources.qrc
    legacy_v2/resources/app.rc)

target_link_libraries(NiumaV3LegacyBaseline PRIVATE Qt5::Widgets Qt5::Sql Qt5::Concurrent)
set_target_properties(NiumaV3LegacyBaseline PROPERTIES OUTPUT_NAME Niuma)
if(WIN32)
    # Keep new source calls within the Windows 7 API surface; newer APIs must
    # be resolved dynamically and have a working fallback.
    target_compile_definitions(NiumaV3LegacyBaseline PRIVATE WINVER=0x0601 _WIN32_WINNT=0x0601)
    if(MINGW)
        target_link_options(NiumaV3LegacyBaseline PRIVATE -Wl,--major-subsystem-version,6,--minor-subsystem-version,1)
    endif()
endif()
add_dependencies(NiumaV3LegacyBaseline NiumaV3ProductIcon)
target_compile_definitions(NiumaV3LegacyBaseline PRIVATE NIUMA_APP_ICON_PATH="${NIUMA_GENERATED_ICON}")
target_include_directories(NiumaV3LegacyBaseline PRIVATE
    "${NIUMA_LEGACY_ROOT}"
    "${NIUMA_LEGACY_ROOT}/src"
    "${NIUMA_LEGACY_ROOT}/ui"
    "${NIUMA_LEGACY_ROOT}/ui/widgets"
    "${NIUMA_LEGACY_ROOT}/database"
    "${NIUMA_LEGACY_ROOT}/service"
    "${NIUMA_LEGACY_ROOT}/models"
    "${NIUMA_LEGACY_ROOT}/utils")

add_executable(NiumaV3Phase3StagingTest
    legacy_v2/tests/phase3_e2e_test.cpp
    ${NIUMA_LEGACY_BACKEND_SOURCES})

add_executable(NiumaV3StagingPathTest
    tests/staging_path_test.cpp
    ${NIUMA_LEGACY_BACKEND_SOURCES})

add_executable(NiumaV3ShellContractTest
    tests/shell_contract_test.cpp
    ${NIUMA_LEGACY_CORE_SOURCES}
    legacy_v2/resources/resources.qrc)

add_executable(NiumaV3HomeContractTest
    tests/home_contract_test.cpp
    ${NIUMA_LEGACY_CORE_SOURCES}
    legacy_v2/resources/resources.qrc)

add_executable(NiumaV3TaskListContractTest
    tests/task_list_contract_test.cpp
    ${NIUMA_LEGACY_CORE_SOURCES}
    legacy_v2/resources/resources.qrc)

add_executable(NiumaV3TaskDialogContractTest
    tests/task_dialog_contract_test.cpp
    ${NIUMA_LEGACY_CORE_SOURCES}
    legacy_v2/resources/resources.qrc)

add_executable(NiumaV3TaskApplicationContractTest
    tests/task_application_contract_test.cpp
    ${NIUMA_LEGACY_BACKEND_SOURCES})

add_executable(NiumaV3PeriodicTaskContractTest
    tests/periodic_task_contract_test.cpp
    ${NIUMA_LEGACY_CORE_SOURCES}
    legacy_v2/resources/resources.qrc)

add_executable(NiumaV3ReferenceContractTest
    tests/reference_contract_test.cpp
    ${NIUMA_LEGACY_CORE_SOURCES}
    legacy_v2/resources/resources.qrc)

add_executable(NiumaV3AchievementContractTest
    tests/achievement_contract_test.cpp
    ${NIUMA_LEGACY_CORE_SOURCES}
    legacy_v2/resources/resources.qrc)

add_executable(NiumaV3StatsContractTest
    tests/stats_contract_test.cpp
    ${NIUMA_LEGACY_CORE_SOURCES}
    legacy_v2/resources/resources.qrc)

add_executable(NiumaV3RecycleBinContractTest
    tests/recycle_bin_contract_test.cpp
    ${NIUMA_LEGACY_CORE_SOURCES}
    legacy_v2/resources/resources.qrc)

add_executable(NiumaV3SettingsContractTest
    tests/settings_contract_test.cpp
    ${NIUMA_LEGACY_CORE_SOURCES}
    legacy_v2/resources/resources.qrc)

add_executable(NiumaV3SettingsMaintenanceContractTest
    tests/settings_maintenance_contract_test.cpp
    ${NIUMA_LEGACY_CORE_SOURCES}
    legacy_v2/resources/resources.qrc)

add_executable(NiumaV3StyledMessageBoxContractTest
    tests/styled_message_box_contract_test.cpp
    legacy_v2/ui/widgets/StyledMessageBox.cpp)

add_executable(NiumaV3NativeVisualCaptureContractTest
    tests/native_visual_capture_contract_test.cpp
    ${NIUMA_LEGACY_CORE_SOURCES}
    legacy_v2/resources/resources.qrc)

target_link_libraries(NiumaV3Phase3StagingTest PRIVATE Qt5::Widgets Qt5::Sql Qt5::Concurrent)
target_include_directories(NiumaV3Phase3StagingTest PRIVATE
    "${NIUMA_LEGACY_ROOT}"
    "${NIUMA_LEGACY_ROOT}/database"
    "${NIUMA_LEGACY_ROOT}/service"
    "${NIUMA_LEGACY_ROOT}/models"
    "${NIUMA_LEGACY_ROOT}/utils")

target_link_libraries(NiumaV3StagingPathTest PRIVATE Qt5::Widgets Qt5::Sql Qt5::Concurrent)
target_link_libraries(NiumaV3ShellContractTest PRIVATE Qt5::Widgets Qt5::Sql Qt5::Concurrent)
target_link_libraries(NiumaV3HomeContractTest PRIVATE Qt5::Widgets Qt5::Sql Qt5::Concurrent)
target_link_libraries(NiumaV3TaskListContractTest PRIVATE Qt5::Widgets Qt5::Sql Qt5::Concurrent)
target_link_libraries(NiumaV3TaskDialogContractTest PRIVATE Qt5::Widgets Qt5::Sql Qt5::Concurrent)
target_link_libraries(NiumaV3TaskApplicationContractTest PRIVATE Qt5::Widgets Qt5::Sql Qt5::Concurrent)
target_link_libraries(NiumaV3PeriodicTaskContractTest PRIVATE Qt5::Widgets Qt5::Sql Qt5::Concurrent)
target_link_libraries(NiumaV3ReferenceContractTest PRIVATE Qt5::Widgets Qt5::Sql Qt5::Concurrent)
target_link_libraries(NiumaV3AchievementContractTest PRIVATE Qt5::Widgets Qt5::Sql Qt5::Concurrent)
target_link_libraries(NiumaV3StatsContractTest PRIVATE Qt5::Widgets Qt5::Sql Qt5::Concurrent)
target_link_libraries(NiumaV3RecycleBinContractTest PRIVATE Qt5::Widgets Qt5::Sql Qt5::Concurrent)
target_link_libraries(NiumaV3SettingsContractTest PRIVATE Qt5::Widgets Qt5::Sql Qt5::Concurrent)
target_link_libraries(NiumaV3SettingsMaintenanceContractTest PRIVATE Qt5::Widgets Qt5::Sql Qt5::Concurrent)
target_link_libraries(NiumaV3StyledMessageBoxContractTest PRIVATE Qt5::Widgets)
target_link_libraries(NiumaV3NativeVisualCaptureContractTest PRIVATE Qt5::Widgets Qt5::Sql Qt5::Concurrent)
target_include_directories(NiumaV3StagingPathTest PRIVATE
    "${NIUMA_LEGACY_ROOT}"
    "${NIUMA_LEGACY_ROOT}/database"
    "${NIUMA_LEGACY_ROOT}/service"
    "${NIUMA_LEGACY_ROOT}/models"
    "${NIUMA_LEGACY_ROOT}/utils")

target_include_directories(NiumaV3ShellContractTest PRIVATE
    "${NIUMA_LEGACY_ROOT}"
    "${NIUMA_LEGACY_ROOT}/src"
    "${NIUMA_LEGACY_ROOT}/ui"
    "${NIUMA_LEGACY_ROOT}/ui/widgets"
    "${NIUMA_LEGACY_ROOT}/database"
    "${NIUMA_LEGACY_ROOT}/service"
    "${NIUMA_LEGACY_ROOT}/models"
    "${NIUMA_LEGACY_ROOT}/utils")

target_include_directories(NiumaV3HomeContractTest PRIVATE
    "${NIUMA_LEGACY_ROOT}"
    "${NIUMA_LEGACY_ROOT}/src"
    "${NIUMA_LEGACY_ROOT}/ui"
    "${NIUMA_LEGACY_ROOT}/ui/widgets"
    "${NIUMA_LEGACY_ROOT}/database"
    "${NIUMA_LEGACY_ROOT}/service"
    "${NIUMA_LEGACY_ROOT}/models"
    "${NIUMA_LEGACY_ROOT}/utils")

target_include_directories(NiumaV3TaskListContractTest PRIVATE
    "${NIUMA_LEGACY_ROOT}"
    "${NIUMA_LEGACY_ROOT}/src"
    "${NIUMA_LEGACY_ROOT}/ui"
    "${NIUMA_LEGACY_ROOT}/ui/widgets"
    "${NIUMA_LEGACY_ROOT}/database"
    "${NIUMA_LEGACY_ROOT}/service"
    "${NIUMA_LEGACY_ROOT}/models"
    "${NIUMA_LEGACY_ROOT}/utils")

target_include_directories(NiumaV3TaskDialogContractTest PRIVATE
    "${NIUMA_LEGACY_ROOT}"
    "${NIUMA_LEGACY_ROOT}/src"
    "${NIUMA_LEGACY_ROOT}/ui"
    "${NIUMA_LEGACY_ROOT}/ui/widgets"
    "${NIUMA_LEGACY_ROOT}/database"
    "${NIUMA_LEGACY_ROOT}/service"
    "${NIUMA_LEGACY_ROOT}/models"
    "${NIUMA_LEGACY_ROOT}/utils")

target_include_directories(NiumaV3TaskApplicationContractTest PRIVATE
    "${NIUMA_LEGACY_ROOT}"
    "${NIUMA_LEGACY_ROOT}/database"
    "${NIUMA_LEGACY_ROOT}/service"
    "${NIUMA_LEGACY_ROOT}/models"
    "${NIUMA_LEGACY_ROOT}/utils")

target_include_directories(NiumaV3PeriodicTaskContractTest PRIVATE
    "${NIUMA_LEGACY_ROOT}"
    "${NIUMA_LEGACY_ROOT}/src"
    "${NIUMA_LEGACY_ROOT}/ui"
    "${NIUMA_LEGACY_ROOT}/ui/widgets"
    "${NIUMA_LEGACY_ROOT}/database"
    "${NIUMA_LEGACY_ROOT}/service"
    "${NIUMA_LEGACY_ROOT}/models"
    "${NIUMA_LEGACY_ROOT}/utils")

target_include_directories(NiumaV3ReferenceContractTest PRIVATE
    "${NIUMA_LEGACY_ROOT}"
    "${NIUMA_LEGACY_ROOT}/src"
    "${NIUMA_LEGACY_ROOT}/ui"
    "${NIUMA_LEGACY_ROOT}/ui/widgets"
    "${NIUMA_LEGACY_ROOT}/database"
    "${NIUMA_LEGACY_ROOT}/service"
    "${NIUMA_LEGACY_ROOT}/models"
    "${NIUMA_LEGACY_ROOT}/utils")

target_include_directories(NiumaV3AchievementContractTest PRIVATE
    "${NIUMA_LEGACY_ROOT}"
    "${NIUMA_LEGACY_ROOT}/src"
    "${NIUMA_LEGACY_ROOT}/ui"
    "${NIUMA_LEGACY_ROOT}/ui/widgets"
    "${NIUMA_LEGACY_ROOT}/database"
    "${NIUMA_LEGACY_ROOT}/service"
    "${NIUMA_LEGACY_ROOT}/models"
    "${NIUMA_LEGACY_ROOT}/utils")

target_include_directories(NiumaV3StatsContractTest PRIVATE
    "${NIUMA_LEGACY_ROOT}"
    "${NIUMA_LEGACY_ROOT}/src"
    "${NIUMA_LEGACY_ROOT}/ui"
    "${NIUMA_LEGACY_ROOT}/ui/widgets"
    "${NIUMA_LEGACY_ROOT}/database"
    "${NIUMA_LEGACY_ROOT}/service"
    "${NIUMA_LEGACY_ROOT}/models"
    "${NIUMA_LEGACY_ROOT}/utils")

target_include_directories(NiumaV3RecycleBinContractTest PRIVATE
    "${NIUMA_LEGACY_ROOT}"
    "${NIUMA_LEGACY_ROOT}/src"
    "${NIUMA_LEGACY_ROOT}/ui"
    "${NIUMA_LEGACY_ROOT}/ui/widgets"
    "${NIUMA_LEGACY_ROOT}/database"
    "${NIUMA_LEGACY_ROOT}/service"
    "${NIUMA_LEGACY_ROOT}/models"
    "${NIUMA_LEGACY_ROOT}/utils")

target_include_directories(NiumaV3SettingsContractTest PRIVATE
    "${NIUMA_LEGACY_ROOT}"
    "${NIUMA_LEGACY_ROOT}/src"
    "${NIUMA_LEGACY_ROOT}/ui"
    "${NIUMA_LEGACY_ROOT}/ui/widgets"
    "${NIUMA_LEGACY_ROOT}/database"
    "${NIUMA_LEGACY_ROOT}/service"
    "${NIUMA_LEGACY_ROOT}/models"
    "${NIUMA_LEGACY_ROOT}/utils")

target_include_directories(NiumaV3SettingsMaintenanceContractTest PRIVATE
    "${NIUMA_LEGACY_ROOT}"
    "${NIUMA_LEGACY_ROOT}/src"
    "${NIUMA_LEGACY_ROOT}/ui"
    "${NIUMA_LEGACY_ROOT}/ui/widgets"
    "${NIUMA_LEGACY_ROOT}/database"
    "${NIUMA_LEGACY_ROOT}/service"
    "${NIUMA_LEGACY_ROOT}/models"
    "${NIUMA_LEGACY_ROOT}/utils")

target_include_directories(NiumaV3StyledMessageBoxContractTest PRIVATE
    "${NIUMA_LEGACY_ROOT}"
    "${NIUMA_LEGACY_ROOT}/src"
    "${NIUMA_LEGACY_ROOT}/ui"
    "${NIUMA_LEGACY_ROOT}/ui/widgets"
    "${NIUMA_LEGACY_ROOT}/database"
    "${NIUMA_LEGACY_ROOT}/service"
    "${NIUMA_LEGACY_ROOT}/models"
    "${NIUMA_LEGACY_ROOT}/utils")

target_include_directories(NiumaV3NativeVisualCaptureContractTest PRIVATE
    "${NIUMA_LEGACY_ROOT}"
    "${NIUMA_LEGACY_ROOT}/src"
    "${NIUMA_LEGACY_ROOT}/ui"
    "${NIUMA_LEGACY_ROOT}/ui/widgets"
    "${NIUMA_LEGACY_ROOT}/database"
    "${NIUMA_LEGACY_ROOT}/service"
    "${NIUMA_LEGACY_ROOT}/models"
    "${NIUMA_LEGACY_ROOT}/utils")

if(MINGW)
    target_link_libraries(NiumaV3LegacyBaseline PRIVATE dwmapi)
    target_link_libraries(NiumaV3ShellContractTest PRIVATE dwmapi)
    target_link_libraries(NiumaV3HomeContractTest PRIVATE dwmapi)
    target_link_libraries(NiumaV3TaskListContractTest PRIVATE dwmapi)
    target_link_libraries(NiumaV3TaskDialogContractTest PRIVATE dwmapi)
    target_link_options(NiumaV3TaskApplicationContractTest PRIVATE -mconsole)
    target_link_libraries(NiumaV3PeriodicTaskContractTest PRIVATE dwmapi)
    target_link_libraries(NiumaV3ReferenceContractTest PRIVATE dwmapi)
    target_link_libraries(NiumaV3AchievementContractTest PRIVATE dwmapi)
    target_link_libraries(NiumaV3StatsContractTest PRIVATE dwmapi)
    target_link_libraries(NiumaV3RecycleBinContractTest PRIVATE dwmapi)
    target_link_libraries(NiumaV3SettingsContractTest PRIVATE dwmapi)
    target_link_libraries(NiumaV3SettingsMaintenanceContractTest PRIVATE dwmapi)
    target_link_libraries(NiumaV3NativeVisualCaptureContractTest PRIVATE dwmapi)
    target_link_options(NiumaV3LegacyBaseline PRIVATE -mwindows)
    target_compile_options(NiumaV3LegacyBaseline PRIVATE -Wall -Wextra)
    target_link_options(NiumaV3Phase3StagingTest PRIVATE -mconsole)
    target_link_options(NiumaV3StagingPathTest PRIVATE -mconsole)
    target_link_options(NiumaV3ShellContractTest PRIVATE -mconsole)
    target_link_options(NiumaV3HomeContractTest PRIVATE -mconsole)
    target_link_options(NiumaV3TaskListContractTest PRIVATE -mconsole)
    target_link_options(NiumaV3TaskDialogContractTest PRIVATE -mconsole)
    target_link_options(NiumaV3PeriodicTaskContractTest PRIVATE -mconsole)
    target_link_options(NiumaV3ReferenceContractTest PRIVATE -mconsole)
    target_link_options(NiumaV3AchievementContractTest PRIVATE -mconsole)
    target_link_options(NiumaV3StatsContractTest PRIVATE -mconsole)
    target_link_options(NiumaV3RecycleBinContractTest PRIVATE -mconsole)
    target_link_options(NiumaV3SettingsContractTest PRIVATE -mconsole)
    target_link_options(NiumaV3SettingsMaintenanceContractTest PRIVATE -mconsole)
    target_link_options(NiumaV3NativeVisualCaptureContractTest PRIVATE -mconsole)
endif()

if(WIN32)
    add_executable(NiumaV3WindowChromeContractTest
        tests/window_chrome_contract_test.cpp legacy_v2/ui/widgets/TitleBar.cpp
        legacy_v2/resources/resources.qrc)
    target_link_libraries(NiumaV3WindowChromeContractTest PRIVATE Qt5::Widgets Qt5::Svg)
    target_include_directories(NiumaV3WindowChromeContractTest PRIVATE "${NIUMA_LEGACY_ROOT}/ui")
    foreach(chrome_target IN ITEMS NiumaV3LegacyBaseline NiumaV3ShellContractTest
        NiumaV3HomeContractTest NiumaV3TaskListContractTest NiumaV3TaskDialogContractTest
        NiumaV3PeriodicTaskContractTest NiumaV3ReferenceContractTest NiumaV3AchievementContractTest
        NiumaV3StatsContractTest NiumaV3RecycleBinContractTest NiumaV3SettingsContractTest
        NiumaV3SettingsMaintenanceContractTest NiumaV3StyledMessageBoxContractTest
        NiumaV3NativeVisualCaptureContractTest NiumaV3WindowChromeContractTest)
        target_link_libraries(${chrome_target} PRIVATE NiumaV3WindowsChromeSupport)
    endforeach()
    add_test(NAME niuma_v3_legacy_window_chrome_contract COMMAND NiumaV3WindowChromeContractTest)
    add_test(NAME niuma_v3_legacy_window_chrome_dpi_200 COMMAND NiumaV3WindowChromeContractTest)
    add_test(NAME niuma_v3_legacy_navigation_contract COMMAND NiumaV3ShellContractTest --legacy-input)
    add_test(NAME niuma_v3_legacy_visual_capture_contract COMMAND NiumaV3NativeVisualCaptureContractTest --legacy-chrome)
    add_test(NAME niuma_v3_legacy_visual_capture_dpi_200 COMMAND NiumaV3NativeVisualCaptureContractTest --legacy-chrome)
    find_program(NIUMA_POWERSHELL NAMES powershell REQUIRED)
    get_target_property(NIUMA_WINDOWS_PLUGIN Qt5::QWindowsIntegrationPlugin IMPORTED_LOCATION_RELEASE)
    get_filename_component(NIUMA_WINDOWS_PLUGIN_DIR "${NIUMA_WINDOWS_PLUGIN}" DIRECTORY)
    get_filename_component(NIUMA_PLUGINS_DIR "${NIUMA_WINDOWS_PLUGIN_DIR}" DIRECTORY)
    add_test(NAME niuma_v3_windows7_import_audit COMMAND "${NIUMA_POWERSHELL}" -NoProfile
        -ExecutionPolicy Bypass -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/windows7_import_audit.ps1"
        -Application $<TARGET_FILE:NiumaV3LegacyBaseline> -RuntimeDirectory $<TARGET_FILE_DIR:Qt5::Core>
        -PluginsDirectory "${NIUMA_PLUGINS_DIR}" -Objdump "${CMAKE_OBJDUMP}"
        -Report "${CMAKE_SOURCE_DIR}/06_test/reports/${CMAKE_BUILD_TYPE}-windows7-imports.json")
    add_executable(NiumaV3InstanceContractTest tests/instance_contract_test.cpp)
    target_link_libraries(NiumaV3InstanceContractTest PRIVATE Qt5::Core user32)
    add_test(NAME niuma_v3_instance_contract COMMAND NiumaV3InstanceContractTest $<TARGET_FILE:NiumaV3LegacyBaseline>)
    set_tests_properties(niuma_v3_instance_contract PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
    add_test(NAME niuma_v3_native_input_contract COMMAND NiumaV3InstanceContractTest $<TARGET_FILE:NiumaV3LegacyBaseline> --input-probe)
    set_tests_properties(niuma_v3_native_input_contract PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
    add_test(NAME niuma_v3_native_navigation_contract COMMAND NiumaV3ShellContractTest --native-input)
    set_tests_properties(niuma_v3_native_navigation_contract PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
endif()
add_test(NAME niuma_v3_phase3_staging COMMAND NiumaV3Phase3StagingTest)
add_test(NAME niuma_v3_staging_path COMMAND NiumaV3StagingPathTest)
add_test(NAME niuma_v3_shell_contract COMMAND NiumaV3ShellContractTest)
add_test(NAME niuma_v3_home_contract COMMAND NiumaV3HomeContractTest)
add_test(NAME niuma_v3_task_list_contract COMMAND NiumaV3TaskListContractTest)
add_test(NAME niuma_v3_task_dialog_contract COMMAND NiumaV3TaskDialogContractTest)
add_test(NAME niuma_v3_task_application_contract COMMAND NiumaV3TaskApplicationContractTest)
add_test(NAME niuma_v3_periodic_task_contract COMMAND NiumaV3PeriodicTaskContractTest)
add_test(NAME niuma_v3_reference_contract COMMAND NiumaV3ReferenceContractTest)
add_test(NAME niuma_v3_achievement_contract COMMAND NiumaV3AchievementContractTest)
add_test(NAME niuma_v3_stats_contract COMMAND NiumaV3StatsContractTest)
add_test(NAME niuma_v3_recycle_bin_contract COMMAND NiumaV3RecycleBinContractTest)
add_test(NAME niuma_v3_settings_contract COMMAND NiumaV3SettingsContractTest)
add_test(NAME niuma_v3_settings_maintenance_contract COMMAND NiumaV3SettingsMaintenanceContractTest)
add_test(NAME niuma_v3_styled_message_box_contract COMMAND NiumaV3StyledMessageBoxContractTest)
add_test(NAME niuma_v3_native_visual_capture_contract COMMAND NiumaV3NativeVisualCaptureContractTest)
add_test(NAME niuma_v3_native_visual_capture_dpi_125 COMMAND NiumaV3NativeVisualCaptureContractTest)
add_test(NAME niuma_v3_native_visual_capture_dpi_150 COMMAND NiumaV3NativeVisualCaptureContractTest)
add_test(NAME niuma_v3_native_visual_capture_dpi_200 COMMAND NiumaV3NativeVisualCaptureContractTest)
get_filename_component(NIUMA_BUILD_PROFILE "${CMAKE_BINARY_DIR}" NAME)
set(NIUMA_TEST_DATA_ROOT "${CMAKE_SOURCE_DIR}/06_test/data/${NIUMA_BUILD_PROFILE}")
set(NIUMA_TEST_EVIDENCE_ROOT "${CMAKE_SOURCE_DIR}/06_test/evidence/${NIUMA_BUILD_PROFILE}")
if(WIN32)
    set_tests_properties(niuma_v3_instance_contract PROPERTIES
        ENVIRONMENT "NIUMA_TEST_ROOT=${NIUMA_TEST_DATA_ROOT}/instance")
    set_tests_properties(niuma_v3_native_input_contract PROPERTIES
        ENVIRONMENT "NIUMA_TEST_ROOT=${NIUMA_TEST_DATA_ROOT}/native-input")
    set_tests_properties(niuma_v3_native_navigation_contract PROPERTIES
        ENVIRONMENT "NIUMA_DATA_DIR=${NIUMA_TEST_DATA_ROOT}/native-navigation/Data")
    set_tests_properties(niuma_v3_legacy_window_chrome_contract PROPERTIES
        ENVIRONMENT "NIUMA_SCREENSHOT_DIR=${NIUMA_TEST_EVIDENCE_ROOT}/legacy-chrome")
    set_tests_properties(niuma_v3_legacy_window_chrome_dpi_200 PROPERTIES
        ENVIRONMENT "NIUMA_SCREENSHOT_DIR=${NIUMA_TEST_EVIDENCE_ROOT}/legacy-chrome-dpi-200;QT_SCALE_FACTOR=2")
    set_tests_properties(niuma_v3_legacy_navigation_contract PROPERTIES
        ENVIRONMENT "NIUMA_DATA_DIR=${NIUMA_TEST_DATA_ROOT}/legacy-navigation/Data")
    set_tests_properties(niuma_v3_legacy_visual_capture_contract PROPERTIES
        ENVIRONMENT "NIUMA_TEST_ROOT=${NIUMA_TEST_DATA_ROOT}/legacy-visual;NIUMA_SCREENSHOT_DIR=${NIUMA_TEST_EVIDENCE_ROOT}/legacy-visual")
    set_tests_properties(niuma_v3_legacy_visual_capture_dpi_200 PROPERTIES
        ENVIRONMENT "NIUMA_TEST_ROOT=${NIUMA_TEST_DATA_ROOT}/legacy-visual-dpi-200;NIUMA_SCREENSHOT_DIR=${NIUMA_TEST_EVIDENCE_ROOT}/legacy-visual-dpi-200;QT_SCALE_FACTOR=2")
    set_tests_properties(niuma_v3_legacy_window_chrome_contract niuma_v3_legacy_window_chrome_dpi_200
        niuma_v3_legacy_navigation_contract niuma_v3_legacy_visual_capture_contract
        niuma_v3_legacy_visual_capture_dpi_200 PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
endif()
set(NIUMA_STAGING_TEST_PATH "C:\\msys64\\mingw64\\bin;C:\\msys64\\usr\\bin;$ENV{PATH}")
string(REPLACE ";" "\\;" NIUMA_STAGING_TEST_PATH_ESCAPED "${NIUMA_STAGING_TEST_PATH}")
set_tests_properties(niuma_v3_phase3_staging PROPERTIES
    ENVIRONMENT "NIUMA_TEST_ROOT=${NIUMA_TEST_DATA_ROOT}/phase3;NIUMA_DATA_DIR=${NIUMA_TEST_DATA_ROOT}/phase3/Data;PATH=${NIUMA_STAGING_TEST_PATH_ESCAPED}")
set_tests_properties(niuma_v3_staging_path PROPERTIES
    ENVIRONMENT "NIUMA_DATA_DIR=${NIUMA_TEST_DATA_ROOT}/Data;PATH=${NIUMA_STAGING_TEST_PATH_ESCAPED}")
set_tests_properties(niuma_v3_shell_contract PROPERTIES
    ENVIRONMENT "NIUMA_DATA_DIR=${NIUMA_TEST_DATA_ROOT}/shell/Data;PATH=${NIUMA_STAGING_TEST_PATH_ESCAPED}")
set_tests_properties(niuma_v3_home_contract PROPERTIES
    ENVIRONMENT "NIUMA_TEST_ROOT=${NIUMA_TEST_DATA_ROOT}/home;PATH=${NIUMA_STAGING_TEST_PATH_ESCAPED}")
set_tests_properties(niuma_v3_task_list_contract PROPERTIES
    ENVIRONMENT "NIUMA_TEST_ROOT=${NIUMA_TEST_DATA_ROOT}/task-list;PATH=${NIUMA_STAGING_TEST_PATH_ESCAPED}")
set_tests_properties(niuma_v3_task_dialog_contract PROPERTIES
    ENVIRONMENT "NIUMA_TEST_ROOT=${NIUMA_TEST_DATA_ROOT}/task-dialog;PATH=${NIUMA_STAGING_TEST_PATH_ESCAPED}")
set_tests_properties(niuma_v3_task_application_contract PROPERTIES
    ENVIRONMENT "NIUMA_TEST_ROOT=${NIUMA_TEST_DATA_ROOT}/task-application;PATH=${NIUMA_STAGING_TEST_PATH_ESCAPED}")
set_tests_properties(niuma_v3_stats_contract PROPERTIES
    ENVIRONMENT "NIUMA_TEST_ROOT=${NIUMA_TEST_DATA_ROOT}/stats;PATH=${NIUMA_STAGING_TEST_PATH_ESCAPED}")
set_tests_properties(niuma_v3_recycle_bin_contract PROPERTIES
    ENVIRONMENT "NIUMA_TEST_ROOT=${NIUMA_TEST_DATA_ROOT}/recycle-bin;PATH=${NIUMA_STAGING_TEST_PATH_ESCAPED}")
set_tests_properties(niuma_v3_settings_contract PROPERTIES
    ENVIRONMENT "NIUMA_TEST_ROOT=${NIUMA_TEST_DATA_ROOT}/settings;PATH=${NIUMA_STAGING_TEST_PATH_ESCAPED}")
set_tests_properties(niuma_v3_settings_maintenance_contract PROPERTIES
    ENVIRONMENT "NIUMA_TEST_ROOT=${NIUMA_TEST_DATA_ROOT}/settings-maintenance;PATH=${NIUMA_STAGING_TEST_PATH_ESCAPED}")
set_tests_properties(niuma_v3_styled_message_box_contract PROPERTIES
    ENVIRONMENT "PATH=${NIUMA_STAGING_TEST_PATH_ESCAPED}")
set_tests_properties(niuma_v3_native_visual_capture_contract PROPERTIES
    ENVIRONMENT "NIUMA_TEST_ROOT=${NIUMA_TEST_DATA_ROOT}/native-visual-capture;NIUMA_SCREENSHOT_DIR=${NIUMA_TEST_EVIDENCE_ROOT}/qt-native-captures;PATH=${NIUMA_STAGING_TEST_PATH_ESCAPED}")
set_tests_properties(niuma_v3_native_visual_capture_dpi_125 PROPERTIES
    ENVIRONMENT "NIUMA_TEST_ROOT=${NIUMA_TEST_DATA_ROOT}/native-visual-dpi-125;NIUMA_SCREENSHOT_DIR=${NIUMA_TEST_EVIDENCE_ROOT}/qt-native-captures/dpi-125;QT_SCALE_FACTOR=1.25;PATH=${NIUMA_STAGING_TEST_PATH_ESCAPED}")
set_tests_properties(niuma_v3_native_visual_capture_dpi_150 PROPERTIES
    ENVIRONMENT "NIUMA_TEST_ROOT=${NIUMA_TEST_DATA_ROOT}/native-visual-dpi-150;NIUMA_SCREENSHOT_DIR=${NIUMA_TEST_EVIDENCE_ROOT}/qt-native-captures/dpi-150;QT_SCALE_FACTOR=1.5;PATH=${NIUMA_STAGING_TEST_PATH_ESCAPED}")
set_tests_properties(niuma_v3_native_visual_capture_dpi_200 PROPERTIES
    ENVIRONMENT "NIUMA_TEST_ROOT=${NIUMA_TEST_DATA_ROOT}/native-visual-dpi-200;NIUMA_SCREENSHOT_DIR=${NIUMA_TEST_EVIDENCE_ROOT}/qt-native-captures/dpi-200;QT_SCALE_FACTOR=2;PATH=${NIUMA_STAGING_TEST_PATH_ESCAPED}")
set_tests_properties(niuma_v3_periodic_task_contract PROPERTIES
    ENVIRONMENT "NIUMA_TEST_ROOT=${NIUMA_TEST_DATA_ROOT}/periodic-task;PATH=${NIUMA_STAGING_TEST_PATH_ESCAPED}")
set_tests_properties(niuma_v3_reference_contract PROPERTIES
    ENVIRONMENT "NIUMA_TEST_ROOT=${NIUMA_TEST_DATA_ROOT}/reference;PATH=${NIUMA_STAGING_TEST_PATH_ESCAPED}")
set_tests_properties(niuma_v3_achievement_contract PROPERTIES
    ENVIRONMENT "NIUMA_TEST_ROOT=${NIUMA_TEST_DATA_ROOT}/achievement;PATH=${NIUMA_STAGING_TEST_PATH_ESCAPED}")
