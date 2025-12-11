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

#ifndef CLI_COMMAND_GENERATOR_H_
#define CLI_COMMAND_GENERATOR_H_

#include <QString>
#include <QStringList>
#include "IntrusivePtr.h"

class StageSequence;
class ProjectPages;
class PageId;

namespace page_split { class Settings; }
namespace page_layout { class Settings; class Alignment; }
namespace output { class Settings; class Params; }

/**
 * @brief Generates CLI command from current project settings
 *
 * This class extracts settings from the GUI project and generates
 * an equivalent CLI command that can be used for batch processing.
 */
class CLICommandGenerator
{
public:
    CLICommandGenerator(IntrusivePtr<StageSequence> const& stages,
                        QString const& projectFile,
                        QString const& outputDir);

    /**
     * @brief Generate the complete CLI command
     * @param pageId Optional reference page to extract settings from.
     *               If invalid, uses the first page.
     * @return The generated CLI command as a string
     */
    QString generateCommand(PageId const* pageId = nullptr) const;

    /**
     * @brief Generate CLI command as a list of arguments
     * @param pageId Optional reference page
     * @return List of command arguments
     */
    QStringList generateArguments(PageId const* pageId = nullptr) const;

private:
    QString getColorModeArg(output::Params const& params) const;
    QString getDespeckleArg(output::Params const& params) const;
    QString getDewarpingArg(output::Params const& params) const;
    QString getDepthPerceptionArg(output::Params const& params) const;
    QString getOutputDpiArg(output::Params const& params) const;
    QString getMarginsArgs(page_layout::Settings const* settings, PageId const& pageId) const;
    QString getAlignmentArgs(page_layout::Alignment const& alignment) const;
    QString getLayoutArg() const;

    IntrusivePtr<StageSequence> m_stages;
    QString m_projectFile;
    QString m_outputDir;
};

#endif // CLI_COMMAND_GENERATOR_H_
