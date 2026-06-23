#pragma once

#include <QAbstractTableModel>
#include <QVector>

class ParquetTableModel final : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit ParquetTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void setResult(QStringList columns, QVector<QVector<QString>> rows, qsizetype absoluteOffset);
    void clear();

private:
    QStringList columns_;
    QVector<QVector<QString>> rows_;
    qsizetype absoluteOffset_ = 0;
};
