#include "FilterBuilderWidget.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

namespace {

QStringList operators()
{
    return {
        QStringLiteral("equals"),
        QStringLiteral("does not equal"),
        QStringLiteral("greater than"),
        QStringLiteral("less than"),
        QStringLiteral("at least"),
        QStringLiteral("at most"),
        QStringLiteral("contains text"),
        QStringLiteral("starts with"),
        QStringLiteral("ends with"),
        QStringLiteral("is true"),
        QStringLiteral("is false"),
        QStringLiteral("is empty"),
        QStringLiteral("is not empty"),
    };
}

bool operatorNeedsValue(const QString &op)
{
    return op != QStringLiteral("is true")
        && op != QStringLiteral("is false")
        && op != QStringLiteral("is empty")
        && op != QStringLiteral("is not empty");
}

} // namespace

FilterBuilderWidget::FilterBuilderWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(6);

    rowsLayout_ = new QVBoxLayout();
    rowsLayout_->setContentsMargins(0, 0, 0, 0);
    rowsLayout_->setSpacing(6);
    root->addLayout(rowsLayout_);

    auto *actions = new QHBoxLayout();
    actions->setContentsMargins(0, 0, 0, 0);

    addButton_ = new QPushButton(tr("Add Filter"));
    addButton_->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    connect(addButton_, &QPushButton::clicked, this, &FilterBuilderWidget::addConditionRow);
    actions->addWidget(addButton_);

    auto *clearButton = new QPushButton(tr("Clear"));
    clearButton->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
    connect(clearButton, &QPushButton::clicked, this, [this]() {
        while (!rows_.isEmpty()) {
            removeConditionRow(rows_.last().container);
        }
        emit filtersApplied();
    });
    actions->addWidget(clearButton);

    auto *applyButton = new QPushButton(tr("Apply"));
    applyButton->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    connect(applyButton, &QPushButton::clicked, this, &FilterBuilderWidget::filtersApplied);
    actions->addWidget(applyButton);
    actions->addStretch(1);
    root->addLayout(actions);
}

void FilterBuilderWidget::setColumns(const QStringList &columns)
{
    columns_ = columns;
    addButton_->setEnabled(!columns_.isEmpty());

    for (auto &row : rows_) {
        const QString current = row.column->currentText();
        row.column->clear();
        row.column->addItems(columns_);
        const int index = row.column->findText(current);
        if (index >= 0) {
            row.column->setCurrentIndex(index);
        }
    }
}

QVector<FilterCondition> FilterBuilderWidget::conditions() const
{
    QVector<FilterCondition> result;
    result.reserve(rows_.size());

    for (const auto &row : rows_) {
        FilterCondition condition;
        condition.column = row.column->currentText();
        condition.op = row.op->currentText();
        condition.value = row.value->text();
        if (!condition.column.isEmpty()) {
            result.push_back(std::move(condition));
        }
    }

    return result;
}

void FilterBuilderWidget::addConditionRow()
{
    if (columns_.isEmpty()) {
        return;
    }

    Row row;
    row.container = new QWidget(this);

    auto *layout = new QHBoxLayout(row.container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    row.column = new QComboBox(row.container);
    row.column->addItems(columns_);
    row.column->setMinimumWidth(180);
    layout->addWidget(row.column, 2);

    row.op = new QComboBox(row.container);
    row.op->addItems(operators());
    row.op->setMinimumWidth(145);
    layout->addWidget(row.op, 1);

    row.value = new QLineEdit(row.container);
    row.value->setPlaceholderText(tr("value"));
    layout->addWidget(row.value, 2);

    row.remove = new QPushButton(row.container);
    row.remove->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    row.remove->setToolTip(tr("Remove filter"));
    row.remove->setFixedWidth(34);
    layout->addWidget(row.remove);

    rowsLayout_->addWidget(row.container);
    rows_.push_back(row);

    connect(row.op, &QComboBox::currentTextChanged, this, [this, container = row.container]() {
        for (auto &candidate : rows_) {
            if (candidate.container == container) {
                refreshValueEditor(candidate);
                break;
            }
        }
    });
    connect(row.remove, &QPushButton::clicked, this, [this, container = row.container]() {
        removeConditionRow(container);
        emit filtersApplied();
    });
    connect(row.value, &QLineEdit::returnPressed, this, &FilterBuilderWidget::filtersApplied);

    refreshValueEditor(rows_.last());
}

void FilterBuilderWidget::removeConditionRow(QWidget *container)
{
    for (qsizetype i = 0; i < rows_.size(); ++i) {
        if (rows_.at(i).container != container) {
            continue;
        }

        rowsLayout_->removeWidget(rows_[i].container);
        rows_[i].container->deleteLater();
        rows_.removeAt(i);
        break;
    }
}

void FilterBuilderWidget::refreshValueEditor(Row &row)
{
    const bool enabled = operatorNeedsValue(row.op->currentText());
    row.value->setEnabled(enabled);
    if (!enabled) {
        row.value->clear();
    }
}
