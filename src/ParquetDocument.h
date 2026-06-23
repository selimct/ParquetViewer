#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

struct ColumnInfo {
    QString name;
    QString type;
};

struct QueryPage {
    QStringList columns;
    QVector<QVector<QString>> rows;
};

class ParquetDocument final : public QObject {
    Q_OBJECT

public:
    explicit ParquetDocument(QObject *parent = nullptr);
    ~ParquetDocument() override;

    bool open(const QString &filePath, QString *errorMessage);
    bool isOpen() const;

    QString filePath() const;
    QVector<ColumnInfo> schema() const;
    QStringList columnNames() const;

    bool rowCount(const QString &whereClause, qsizetype *count, QString *errorMessage);
    bool queryPage(const QString &whereClause, qsizetype limit, qsizetype offset, QueryPage *page, QString *errorMessage);

private:
    bool runJsonQuery(const QString &sql, class QJsonArray *rows, QString *errorMessage);
    bool loadSchema(QString *errorMessage);
    QString parquetSourceSql() const;
    QString duckdbExecutable() const;

    QString filePath_;
    QVector<ColumnInfo> schema_;
};

QString quoteSqlString(const QString &value);
QString quoteSqlIdentifier(const QString &identifier);
