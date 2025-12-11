/*
    Scan Tailor Universal - Interactive post-processing tool for scanned pages.
    Copyright (C) 2024

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

#include "CLICommandGenerator.h"
#include "StageSequence.h"
#include "ProjectPages.h"
#include "PageId.h"
#include "filters/page_split/Filter.h"
#include "filters/page_split/Settings.h"
#include "filters/page_split/LayoutType.h"
#include "filters/page_layout/Filter.h"
#include "filters/page_layout/Settings.h"
#include "filters/page_layout/Alignment.h"
#include "filters/output/Filter.h"
#include "filters/output/Settings.h"
#include "filters/output/Params.h"
#include "filters/output/ColorParams.h"
#include "filters/output/DespeckleLevel.h"
#include "filters/output/DewarpingMode.h"
#include "filters/output/DepthPerception.h"
#include "Margins.h"
#include <QFileInfo>

CLICommandGenerator::CLICommandGenerator(
    IntrusivePtr<StageSequence> const& stages,
    QString const& projectFile,
    QString const& outputDir)
    : m_stages(stages)
    , m_projectFile(projectFile)
    , m_outputDir(outputDir)
{
}

QString CLICommandGenerator::generateCommand(PageId const* pageId) const
{
    QStringList args = generateArguments(pageId);
    return args.join(" ");
}

QStringList CLICommandGenerator::generateArguments(PageId const* pageId) const
{
    QStringList args;

    // Start with the executable name
    args << "scantailor-cli";

    // Get a reference page ID
    PageId refPageId;
    if (pageId && pageId->imageId().filePath().length() > 0) {
        refPageId = *pageId;
    }

    // Get output settings if available
    if (m_stages.get() && m_stages->outputFilter()) {
        output::Settings* outputSettings = m_stages->outputFilter()->getSettings();
        if (outputSettings) {
            // Get params from reference page or use defaults
            output::Params params;
            if (refPageId.imageId().filePath().length() > 0) {
                params = outputSettings->getParams(refPageId);
            }

            // Color mode
            QString colorMode = getColorModeArg(params);
            if (!colorMode.isEmpty()) {
                args << colorMode;
            }

            // Output DPI
            QString dpi = getOutputDpiArg(params);
            if (!dpi.isEmpty()) {
                args << dpi;
            }

            // Despeckle
            QString despeckle = getDespeckleArg(params);
            if (!despeckle.isEmpty()) {
                args << despeckle;
            }

            // Dewarping
            QString dewarping = getDewarpingArg(params);
            if (!dewarping.isEmpty()) {
                args << dewarping;
            }

            // Depth perception
            QString depth = getDepthPerceptionArg(params);
            if (!depth.isEmpty()) {
                args << depth;
            }
        }
    }

    // Get page layout settings (margins, alignment)
    if (m_stages.get() && m_stages->pageLayoutFilter()) {
        page_layout::Settings* layoutSettings = m_stages->pageLayoutFilter()->getSettings();

        if (layoutSettings && refPageId.imageId().filePath().length() > 0) {
            QString margins = getMarginsArgs(layoutSettings, refPageId);
            if (!margins.isEmpty()) {
                args << margins;
            }

            page_layout::Alignment alignment = layoutSettings->getPageAlignment(refPageId);
            QString alignArgs = getAlignmentArgs(alignment);
            if (!alignArgs.isEmpty()) {
                args << alignArgs;
            }
        }
    }

    // Get page split layout type
    QString layout = getLayoutArg();
    if (!layout.isEmpty()) {
        args << layout;
    }

    // Add the project file
    if (!m_projectFile.isEmpty()) {
        // Quote the path if it contains spaces
        QString projectPath = m_projectFile;
        if (projectPath.contains(' ')) {
            projectPath = "\"" + projectPath + "\"";
        }
        args << projectPath;
    }

    // Add output directory if different from project default
    if (!m_outputDir.isEmpty()) {
        QString outDir = m_outputDir;
        if (outDir.contains(' ')) {
            outDir = "\"" + outDir + "\"";
        }
        args << outDir;
    }

    return args;
}

QString CLICommandGenerator::getColorModeArg(output::Params const& params) const
{
    output::ColorParams::ColorMode mode = params.colorParams().colorMode();

    switch (mode) {
        case output::ColorParams::BLACK_AND_WHITE:
            return "--color-mode=black_and_white";
        case output::ColorParams::COLOR_GRAYSCALE:
            return "--color-mode=color_grayscale";
        case output::ColorParams::MIXED:
            return "--color-mode=mixed";
        default:
            return QString();
    }
}

QString CLICommandGenerator::getDespeckleArg(output::Params const& params) const
{
    output::DespeckleLevel level = params.despeckleLevel();

    switch (level) {
        case output::DESPECKLE_OFF:
            return "--despeckle=off";
        case output::DESPECKLE_CAUTIOUS:
            return "--despeckle=cautious";
        case output::DESPECKLE_NORMAL:
            return "--despeckle=normal";
        case output::DESPECKLE_AGGRESSIVE:
            return "--despeckle=aggressive";
        default:
            return QString();
    }
}

QString CLICommandGenerator::getDewarpingArg(output::Params const& params) const
{
    output::DewarpingMode mode = params.dewarpingMode();

    switch (static_cast<output::DewarpingMode::Mode>(mode)) {
        case output::DewarpingMode::OFF:
            return "--dewarping=off";
        case output::DewarpingMode::AUTO:
            return "--dewarping=auto";
        case output::DewarpingMode::MANUAL:
            return "--dewarping=manual";
        case output::DewarpingMode::MARGINAL:
            return "--dewarping=marginal";
        default:
            return QString();
    }
}

QString CLICommandGenerator::getDepthPerceptionArg(output::Params const& params) const
{
    double depth = params.depthPerception().value();

    // Only include if not default (2.0)
    if (qAbs(depth - 2.0) > 0.01) {
        return QString("--depth-perception=%1").arg(depth, 0, 'f', 1);
    }
    return QString();
}

QString CLICommandGenerator::getOutputDpiArg(output::Params const& params) const
{
    Dpi dpi = params.outputDpi();

    if (dpi.horizontal() == dpi.vertical()) {
        // Same DPI for both axes
        if (dpi.horizontal() != 600) { // Only if not default
            return QString("--output-dpi=%1").arg(dpi.horizontal());
        }
    } else {
        // Different DPI for each axis
        return QString("--output-dpi-x=%1 --output-dpi-y=%2")
            .arg(dpi.horizontal())
            .arg(dpi.vertical());
    }
    return QString();
}

QString CLICommandGenerator::getMarginsArgs(
    page_layout::Settings const* settings, PageId const& pageId) const
{
    if (!settings) return QString();

    MarginsWithAuto margins = settings->getHardMarginsMM(pageId);
    QStringList args;

    // Check if all margins are the same
    if (qAbs(margins.left() - margins.right()) < 0.01 &&
        qAbs(margins.left() - margins.top()) < 0.01 &&
        qAbs(margins.left() - margins.bottom()) < 0.01) {

        if (qAbs(margins.left()) > 0.01) {
            args << QString("--margins=%1").arg(margins.left(), 0, 'f', 1);
        }
    } else {
        // Different margins for each side
        if (qAbs(margins.left()) > 0.01) {
            args << QString("--margins-left=%1").arg(margins.left(), 0, 'f', 1);
        }
        if (qAbs(margins.right()) > 0.01) {
            args << QString("--margins-right=%1").arg(margins.right(), 0, 'f', 1);
        }
        if (qAbs(margins.top()) > 0.01) {
            args << QString("--margins-top=%1").arg(margins.top(), 0, 'f', 1);
        }
        if (qAbs(margins.bottom()) > 0.01) {
            args << QString("--margins-bottom=%1").arg(margins.bottom(), 0, 'f', 1);
        }
    }

    return args.join(" ");
}

QString CLICommandGenerator::getAlignmentArgs(page_layout::Alignment const& alignment) const
{
    QStringList args;

    // Vertical alignment
    switch (alignment.vertical()) {
        case page_layout::Alignment::TOP:
            args << "--alignment-vertical=top";
            break;
        case page_layout::Alignment::VCENTER:
            // Default, don't add
            break;
        case page_layout::Alignment::BOTTOM:
            args << "--alignment-vertical=bottom";
            break;
        case page_layout::Alignment::VAUTO:
            args << "--alignment-vertical=auto";
            break;
        case page_layout::Alignment::VORIGINAL:
            args << "--alignment-vertical=original";
            break;
    }

    // Horizontal alignment
    switch (alignment.horizontal()) {
        case page_layout::Alignment::LEFT:
            args << "--alignment-horizontal=left";
            break;
        case page_layout::Alignment::HCENTER:
            // Default, don't add
            break;
        case page_layout::Alignment::RIGHT:
            args << "--alignment-horizontal=right";
            break;
        case page_layout::Alignment::HAUTO:
            args << "--alignment-horizontal=auto";
            break;
        case page_layout::Alignment::HORIGINAL:
            args << "--alignment-horizontal=original";
            break;
    }

    // Match layout (null alignment)
    if (alignment.isNull()) {
        args << "--match-layout=false";
    }

    // Tolerance
    if (alignment.tolerance() > 0.01 && qAbs(alignment.tolerance() - 0.2) > 0.01) {
        args << QString("--alignment-tolerance=%1").arg(alignment.tolerance(), 0, 'f', 2);
    }

    return args.join(" ");
}

QString CLICommandGenerator::getLayoutArg() const
{
    if (!m_stages.get() || !m_stages->pageSplitFilter()) {
        return QString();
    }

    // Get default layout type from the filter
    // Note: Layout type is typically set per-page, but we return the common setting
    // The CLI will apply this to all pages

    return QString(); // Layout is usually auto-detected or set per-page
}
