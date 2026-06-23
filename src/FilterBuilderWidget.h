#pragma once

#include "FilterCondition.h"

#include <QWidget>

class QComboBox;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

class FilterBuilderWidget final : public QWidget {
    Q_OBJECT

public:
    explicit FilterBuilderWidget(QWidget *parent = nullptr);

    void setColumns(const QStringList &columns);
    QVector<FilterCondition> conditions() const;

signals:
    void filtersApplied();

private:
    struct Row {
        QWidget *container = nullptr;
        QComboBox *column = nullptr;
        QComboBox *op = nullptr;
        QLineEdit *value = nullptr;
        QPushButton *remove = nullptr;
    };

    void addConditionRow();
    void removeConditionRow(QWidget *container);
    void refreshValueEditor(Row &row);

    QStringList columns_;
    QVector<Row> rows_;
    QVBoxLayout *rowsLayout_ = nullptr;
    QPushButton *addButton_ = nullptr;
};
