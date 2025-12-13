/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"
#include "Application.h"
#include "MainWindow.h"
#include "PngMetadataLoader.h"
#include "TiffMetadataLoader.h"
#include "JpegMetadataLoader.h"
#ifdef ENABLE_OPENJPEG
#include "Jp2MetadataLoader.h"
#endif
#include "GenericMetadataLoader.h"
#ifdef ENABLE_MUPDF
#include "PdfMetadataLoader.h"
#endif
#include "settings/ini_keys.h"
#include <QMetaType>
#include <QtPlugin>
#include <QLocale>
#include <QDir>
#include <QString>
#include <QStringList>
#include <QTranslator>
#include "settings/globalstaticsettings.h"
#include "PerformanceStats.h"
#include "SIMDUtils.h"
#include "gpu/CUDAUtils.h"
#include <Qt>
#include <string.h>
#include <iostream>
#include <iomanip>

#include "CommandLine.h"

static void printSystemInfo()
{
    std::cout << "\n";
    std::cout << "========================================" << std::endl;
    std::cout << "  Scan Tailor Universal - System Info  " << std::endl;
    std::cout << "========================================" << std::endl;

    // SIMD capabilities
    std::cout << "SIMD: ";
#if SIMD_AVX2_AVAILABLE
    std::cout << "AVX2 ";
#endif
#if SIMD_SSE4_1_AVAILABLE
    std::cout << "SSE4.1 ";
#endif
#if SIMD_SSE2_AVAILABLE
    std::cout << "SSE2 ";
#endif
#if SIMD_NEON_AVAILABLE
    std::cout << "NEON ";
#endif
#if !SIMD_SSE2_AVAILABLE && !SIMD_AVX2_AVAILABLE && !SIMD_NEON_AVAILABLE
    std::cout << "None (scalar fallback)";
#endif
    std::cout << std::endl;

    // CUDA/GPU info
    using namespace imageproc::gpu;
    if (isCUDAAvailable()) {
        CUDADeviceInfo info = getCUDADeviceInfo();
        std::cout << "GPU: " << info.deviceName << std::endl;
        std::cout << "  Compute: " << info.major << "." << info.minor << std::endl;
        std::cout << "  Memory: " << (info.totalMemory / (1024*1024)) << " MB" << std::endl;
        std::cout << "  SMs: " << info.multiprocessorCount << std::endl;
    } else {
        std::cout << "GPU: Not available (CPU processing)" << std::endl;
    }

    std::cout << "========================================\n" << std::endl;
}

int main(int argc, char** argv)
{

    Application app(argc, argv);

#ifdef _WIN32
    // Get rid of all references to Qt's installation directory.
    app.setLibraryPaths(QStringList(app.applicationDirPath()));
#endif

    // This information is used by QSettings.
    // Must be done before CommandLine created
    app.setApplicationName(APPLICATION_NAME);
    app.setOrganizationName(ORGANIZATION_NAME);
    app.setOrganizationDomain(ORGANIZATION_DOMAIN);
    QSettings::setDefaultFormat(QSettings::IniFormat);

    // parse command line arguments
    CommandLine cli(app.arguments());
    CommandLine::set(cli);

    if (cli.isError()) {
        cli.printHelp();
        return 1;
    }

    if (cli.hasHelp()) {
        cli.printHelp();
        return 0;
    }

    QSettings settings;
    GlobalStaticSettings::applyAppStyle(settings);

    // Print system info (GPU/SIMD capabilities) at startup
    printSystemInfo();

    // Enable performance stats collection
    PerformanceStats::instance().setEnabled(true);

    PngMetadataLoader::registerMyself();
    TiffMetadataLoader::registerMyself();
    JpegMetadataLoader::registerMyself();
#ifdef ENABLE_OPENJPEG
    Jp2MetadataLoader::registerMyself();
#endif
#ifdef ENABLE_MUPDF
    PdfMetadataLoader::registerMyself();
#endif
    // should be the last one as the most dumb and loads whole image into mem
    GenericMetadataLoader::registerMyself();

    MainWindow* main_wnd = new MainWindow();
    main_wnd->setAttribute(Qt::WA_DeleteOnClose);

    QObject::connect(main_wnd, &MainWindow::settingsUpdateRequest, CommandLine::updateSettings);

    if (cli.hasLanguage()) {
        main_wnd->changeLanguage(cli.getLanguage(), true);
    }

    if (settings.value(_key_app_maximized, _key_app_maximized_def) == false) {
        main_wnd->show();
    } else {
        //main_wnd->showMaximized(); // didn't work for some Win machines.
        QTimer::singleShot(0, main_wnd, &QMainWindow::showMaximized);
    }

    if (!cli.projectFile().isEmpty()) {
        main_wnd->openProject(cli.projectFile());
    }

    return app.exec();
}
