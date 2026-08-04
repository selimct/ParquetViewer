// Copyright (C) 2026 Furkan Selim Cetin
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MainWindow.h"

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Parquet Viewer"));
    QApplication::setOrganizationName(QStringLiteral("ParquetViewer"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Desktop Parquet file viewer"));
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("files"), QStringLiteral("Parquet files to open"), QStringLiteral("[files...]"));
    parser.process(app);

    MainWindow window;
    window.show();
    window.openFiles(parser.positionalArguments());

    return app.exec();
}
