#include "ParquetDocument.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QProcess>

#ifndef DUCKDB_EXECUTABLE
#define DUCKDB_EXECUTABLE "duckdb"
#endif

namespace {

QString jsonValueToString(const QJsonValue &value)
{
    if (value.isNull() || value.isUndefined()) {
        return QStringLiteral("NULL");
    }
    if (value.isBool()) {
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    }
    if (value.isDouble()) {
        return QString::number(value.toDouble(), 'g', 16);
    }
    return value.toString();
}

} // namespace

ParquetDocument::ParquetDocument(QObject *parent)
    : QObject(parent)
{
}

ParquetDocument::~ParquetDocument()
{
}

bool ParquetDocument::open(const QString &filePath, QString *errorMessage)
{
    QFileInfo info(filePath);
    if (!info.exists() || !info.isFile()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("File does not exist: %1").arg(filePath);
        }
        return false;
    }

    filePath_ = info.absoluteFilePath();
    return loadSchema(errorMessage);
}

bool ParquetDocument::isOpen() const
{
    return !filePath_.isEmpty();
}

QString ParquetDocument::filePath() const
{
    return filePath_;
}

QVector<ColumnInfo> ParquetDocument::schema() const
{
    return schema_;
}

QStringList ParquetDocument::columnNames() const
{
    QStringList names;
    names.reserve(schema_.size());
    for (const auto &column : schema_) {
        names.push_back(column.name);
    }
    return names;
}

bool ParquetDocument::rowCount(const QString &whereClause, qsizetype *count, QString *errorMessage)
{
    const QString sql = QStringLiteral("SELECT count(*) FROM %1 %2")
        .arg(parquetSourceSql(), whereClause.isEmpty() ? QString() : QStringLiteral("WHERE %1").arg(whereClause));

    QJsonArray rows;
    if (!runJsonQuery(sql, &rows, errorMessage)) {
        return false;
    }

    if (rows.isEmpty() || !rows.first().isObject()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("DuckDB returned no row count.");
        }
        return false;
    }

    const QJsonObject object = rows.first().toObject();
    if (object.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("DuckDB returned an empty row count.");
        }
        return false;
    }

    *count = static_cast<qsizetype>(object.constBegin().value().toDouble());
    return true;
}

bool ParquetDocument::queryPage(const QString &whereClause, qsizetype limit, qsizetype offset, QueryPage *page, QString *errorMessage)
{
    const QString sql = QStringLiteral("SELECT * FROM %1 %2 LIMIT %3 OFFSET %4")
        .arg(parquetSourceSql(),
             whereClause.isEmpty() ? QString() : QStringLiteral("WHERE %1").arg(whereClause))
        .arg(limit)
        .arg(offset);

    QJsonArray rows;
    if (!runJsonQuery(sql, &rows, errorMessage)) {
        return false;
    }

    QueryPage nextPage;
    nextPage.columns = columnNames();
    nextPage.rows.reserve(rows.size());

    for (const QJsonValue &rowValue : rows) {
        if (!rowValue.isObject()) {
            continue;
        }

        const QJsonObject object = rowValue.toObject();
        QVector<QString> values;
        values.reserve(nextPage.columns.size());
        for (const QString &column : nextPage.columns) {
            values.push_back(jsonValueToString(object.value(column)));
        }
        nextPage.rows.push_back(std::move(values));
    }

    *page = std::move(nextPage);
    return true;
}

bool ParquetDocument::runJsonQuery(const QString &sql, QJsonArray *rows, QString *errorMessage)
{
    QProcess process;
    process.setProgram(duckdbExecutable());
    process.setArguments({
        QStringLiteral("-no-init"),
        QStringLiteral("-json"),
        QStringLiteral("-c"),
        sql,
    });
    process.start();

    if (!process.waitForStarted()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Could not start DuckDB CLI: %1").arg(duckdbExecutable());
        }
        return false;
    }

    process.waitForFinished(-1);

    const QString stderrText = QString::fromUtf8(process.readAllStandardError()).trimmed();
    const QByteArray stdoutBytes = process.readAllStandardOutput();

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (errorMessage != nullptr) {
            *errorMessage = stderrText.isEmpty()
                ? QStringLiteral("DuckDB CLI failed.")
                : stderrText;
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(stdoutBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isArray()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Could not parse DuckDB JSON output: %1").arg(parseError.errorString());
        }
        return false;
    }

    *rows = document.array();
    return true;
}

bool ParquetDocument::loadSchema(QString *errorMessage)
{
    const QString sql = QStringLiteral("DESCRIBE SELECT * FROM %1").arg(parquetSourceSql());

    QJsonArray rows;
    if (!runJsonQuery(sql, &rows, errorMessage)) {
        return false;
    }

    QVector<ColumnInfo> nextSchema;
    nextSchema.reserve(rows.size());
    for (const QJsonValue &rowValue : rows) {
        if (!rowValue.isObject()) {
            continue;
        }
        const QJsonObject object = rowValue.toObject();
        nextSchema.push_back({
            object.value(QStringLiteral("column_name")).toString(),
            object.value(QStringLiteral("column_type")).toString(),
        });
    }

    schema_ = std::move(nextSchema);
    return true;
}

QString ParquetDocument::parquetSourceSql() const
{
    return QStringLiteral("read_parquet(%1)").arg(quoteSqlString(filePath_));
}

QString ParquetDocument::duckdbExecutable() const
{
    return QString::fromUtf8(DUCKDB_EXECUTABLE);
}

QString quoteSqlString(const QString &value)
{
    QString escaped = value;
    escaped.replace('\'', QStringLiteral("''"));
    return QStringLiteral("'%1'").arg(escaped);
}

QString quoteSqlIdentifier(const QString &identifier)
{
    QString escaped = identifier;
    escaped.replace('"', QStringLiteral("\"\""));
    return QStringLiteral("\"%1\"").arg(escaped);
}
