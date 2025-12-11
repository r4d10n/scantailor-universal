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

#ifndef CLI_COMMAND_DIALOG_H_
#define CLI_COMMAND_DIALOG_H_

#include <QDialog>

class QPlainTextEdit;
class QPushButton;
class QLabel;

/**
 * @brief Dialog to display and copy the generated CLI command
 *
 * This dialog shows the CLI command equivalent to the current project
 * settings and allows the user to copy it to the clipboard.
 */
class CLICommandDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CLICommandDialog(QString const& command, QWidget* parent = nullptr);
    ~CLICommandDialog() override = default;

private slots:
    void copyToClipboard();

private:
    void setupUi();

    QString m_command;
    QPlainTextEdit* m_commandEdit;
    QPushButton* m_copyButton;
    QPushButton* m_closeButton;
    QLabel* m_statusLabel;
};

#endif // CLI_COMMAND_DIALOG_H_
