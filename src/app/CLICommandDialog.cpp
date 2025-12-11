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

#include "CLICommandDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QClipboard>
#include <QApplication>
#include <QFont>
#include <QTimer>

CLICommandDialog::CLICommandDialog(QString const& command, QWidget* parent)
    : QDialog(parent)
    , m_command(command)
    , m_commandEdit(nullptr)
    , m_copyButton(nullptr)
    , m_closeButton(nullptr)
    , m_statusLabel(nullptr)
{
    setupUi();
}

void CLICommandDialog::setupUi()
{
    setWindowTitle(tr("CLI Command"));
    setMinimumSize(600, 250);
    resize(700, 300);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Description label
    QLabel* descLabel = new QLabel(
        tr("The following command can be used to process files with the same settings from the command line:"),
        this);
    descLabel->setWordWrap(true);
    mainLayout->addWidget(descLabel);

    // Command text area
    m_commandEdit = new QPlainTextEdit(this);
    m_commandEdit->setPlainText(m_command);
    m_commandEdit->setReadOnly(true);
    m_commandEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);

    // Use monospace font for command
    QFont monoFont("Monospace");
    monoFont.setStyleHint(QFont::TypeWriter);
    monoFont.setPointSize(10);
    m_commandEdit->setFont(monoFont);

    mainLayout->addWidget(m_commandEdit, 1);

    // Status label (for copy confirmation)
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: green;");
    mainLayout->addWidget(m_statusLabel);

    // Button layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_copyButton = new QPushButton(tr("Copy to Clipboard"), this);
    m_copyButton->setDefault(true);
    connect(m_copyButton, &QPushButton::clicked, this, &CLICommandDialog::copyToClipboard);
    buttonLayout->addWidget(m_copyButton);

    m_closeButton = new QPushButton(tr("Close"), this);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_closeButton);

    mainLayout->addLayout(buttonLayout);
}

void CLICommandDialog::copyToClipboard()
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(m_command);

    m_statusLabel->setText(tr("Command copied to clipboard!"));

    // Clear the status message after 3 seconds
    QTimer::singleShot(3000, this, [this]() {
        m_statusLabel->clear();
    });
}
