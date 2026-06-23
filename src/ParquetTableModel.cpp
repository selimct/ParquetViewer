#include "ParquetTableModel.h"

ParquetTableModel::ParquetTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int ParquetTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : rows_.size();
}

int ParquetTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : columns_.size();
}

QVariant ParquetTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole) {
        return {};
    }

    if (index.row() < 0 || index.row() >= rows_.size()) {
        return {};
    }

    const auto &row = rows_.at(index.row());
    if (index.column() < 0 || index.column() >= row.size()) {
        return {};
    }

    return row.at(index.column());
}

QVariant ParquetTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return {};
    }

    if (orientation == Qt::Horizontal) {
        if (section >= 0 && section < columns_.size()) {
            return columns_.at(section);
        }
        return {};
    }

    return absoluteOffset_ + section + 1;
}

void ParquetTableModel::setResult(QStringList columns, QVector<QVector<QString>> rows, qsizetype absoluteOffset)
{
    beginResetModel();
    columns_ = std::move(columns);
    rows_ = std::move(rows);
    absoluteOffset_ = absoluteOffset;
    endResetModel();
}

void ParquetTableModel::clear()
{
    beginResetModel();
    columns_.clear();
    rows_.clear();
    absoluteOffset_ = 0;
    endResetModel();
}
