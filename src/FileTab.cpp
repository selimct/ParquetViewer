#include "FileTab.h"

#include "FilterBuilderWidget.h"
#include "ParquetTableModel.h"

#include <QComboBox>
#include <QAbstractItemView>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStyle>
#include <QTableView>
#include <QToolBar>
#include <QVBoxLayout>

#include <algorithm>

namespace {

struct QuickOperator {
    QString token;
    QString filterOperator;
};

bool isNumericType(const QString &type)
{
    const QString upper = type.toUpper();
    return upper.contains(QStringLiteral("INT"))
        || upper == QStringLiteral("FLOAT")
        || upper == QStringLiteral("DOUBLE")
        || upper == QStringLiteral("REAL")
        || upper.startsWith(QStringLiteral("DECIMAL"));
}

bool isBooleanType(const QString &type)
{
    return type.compare(QStringLiteral("BOOLEAN"), Qt::CaseInsensitive) == 0
        || type.compare(QStringLiteral("BOOL"), Qt::CaseInsensitive) == 0;
}

bool parseBoolean(const QString &value, QString *sql)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("true") || normalized == QStringLiteral("1")
        || normalized == QStringLiteral("yes")) {
        *sql = QStringLiteral("TRUE");
        return true;
    }
    if (normalized == QStringLiteral("false") || normalized == QStringLiteral("0")
        || normalized == QStringLiteral("no")) {
        *sql = QStringLiteral("FALSE");
        return true;
    }
    return false;
}

QString escapeLikePattern(QString value)
{
    value.replace('\\', QStringLiteral("\\\\"));
    value.replace('%', QStringLiteral("\\%"));
    value.replace('_', QStringLiteral("\\_"));
    value.replace('\'', QStringLiteral("''"));
    return value;
}

QString literalForValue(const QString &type, const QString &rawValue, QString *errorMessage)
{
    const QString trimmed = rawValue.trimmed();
    if (trimmed.compare(QStringLiteral("null"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("NULL");
    }

    QString booleanSql;
    if (isBooleanType(type)) {
        if (parseBoolean(trimmed, &booleanSql)) {
            return booleanSql;
        }
        *errorMessage = QObject::tr("Expected a boolean value like true, false, 1, or 0.");
        return {};
    }

    if (isNumericType(type)) {
        bool ok = false;
        trimmed.toDouble(&ok);
        if (!ok) {
            *errorMessage = QObject::tr("Expected a numeric value.");
            return {};
        }
        return trimmed;
    }

    return quoteSqlString(rawValue);
}

QString unquoteQuickValue(QString value)
{
    value = value.trimmed();
    if (value.size() >= 2) {
        const QChar first = value.front();
        const QChar last = value.back();
        if ((first == '\'' && last == '\'') || (first == '"' && last == '"')) {
            value = value.sliced(1, value.size() - 2);
        }
    }
    return value;
}

QVector<QuickOperator> quickOperators()
{
    return {
        {QStringLiteral("!="), QStringLiteral("does not equal")},
        {QStringLiteral("<>"), QStringLiteral("does not equal")},
        {QStringLiteral(">="), QStringLiteral("at least")},
        {QStringLiteral("<="), QStringLiteral("at most")},
        {QStringLiteral("="), QStringLiteral("equals")},
        {QStringLiteral(">"), QStringLiteral("greater than")},
        {QStringLiteral("<"), QStringLiteral("less than")},
    };
}

bool tryParseQuickFilter(const QString &text, const QStringList &columns, FilterCondition *condition)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    QStringList sortedColumns = columns;
    std::sort(sortedColumns.begin(), sortedColumns.end(), [](const QString &left, const QString &right) {
        return left.size() > right.size();
    });

    for (const QString &column : sortedColumns) {
        if (!trimmed.startsWith(column, Qt::CaseInsensitive)) {
            continue;
        }

        qsizetype cursor = column.size();
        while (cursor < trimmed.size() && trimmed.at(cursor).isSpace()) {
            ++cursor;
        }

        const QString remaining = trimmed.sliced(cursor);
        for (const auto &op : quickOperators()) {
            if (!remaining.startsWith(op.token)) {
                continue;
            }

            const QString value = unquoteQuickValue(remaining.sliced(op.token.size()));
            condition->column = column;
            condition->op = op.filterOperator;
            condition->value = value;
            return true;
        }
    }

    return false;
}

} // namespace

FileTab::FileTab(QString filePath, QWidget *parent)
    : QWidget(parent)
    , filePath_(std::move(filePath))
    , document_(std::make_unique<ParquetDocument>())
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(8);

    auto *toolbar = new QToolBar(this);
    toolbar->setIconSize({18, 18});

    previousButton_ = new QPushButton(this);
    previousButton_->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    previousButton_->setToolTip(tr("Previous page"));
    connect(previousButton_, &QPushButton::clicked, this, &FileTab::previousPage);
    toolbar->addWidget(previousButton_);

    nextButton_ = new QPushButton(this);
    nextButton_->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    nextButton_->setToolTip(tr("Next page"));
    connect(nextButton_, &QPushButton::clicked, this, &FileTab::nextPage);
    toolbar->addWidget(nextButton_);

    auto *refreshButton = new QPushButton(this);
    refreshButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    refreshButton->setToolTip(tr("Reload"));
    connect(refreshButton, &QPushButton::clicked, this, &FileTab::reload);
    toolbar->addWidget(refreshButton);

    pageLabel_ = new QLabel(this);
    toolbar->addWidget(pageLabel_);

    toolbar->addSeparator();
    toolbar->addWidget(new QLabel(tr("Rows"), this));

    pageSize_ = new QSpinBox(this);
    pageSize_->setRange(100, 100000);
    pageSize_->setSingleStep(500);
    pageSize_->setValue(1000);
    connect(pageSize_, &QSpinBox::valueChanged, this, [this]() {
        offset_ = 0;
        reload();
    });
    toolbar->addWidget(pageSize_);

    toolbar->addSeparator();
    toolbar->addWidget(new QLabel(tr("Find"), this));

    searchBox_ = new QLineEdit(this);
    searchBox_->setPlaceholderText(tr("value or target = 1"));
    searchBox_->setMinimumWidth(260);
    connect(searchBox_, &QLineEdit::returnPressed, this, [this]() {
        offset_ = 0;
        reload();
    });
    toolbar->addWidget(searchBox_);

    searchMode_ = new QComboBox(this);
    searchMode_->addItems({tr("contains"), tr("exact")});
    connect(searchMode_, &QComboBox::currentTextChanged, this, [this]() {
        offset_ = 0;
        reload();
    });
    toolbar->addWidget(searchMode_);

    root->addWidget(toolbar);

    filterBuilder_ = new FilterBuilderWidget(this);
    connect(filterBuilder_, &FilterBuilderWidget::filtersApplied, this, [this]() {
        offset_ = 0;
        reload();
    });
    root->addWidget(filterBuilder_);

    model_ = new ParquetTableModel(this);
    table_ = new QTableView(this);
    table_->setModel(model_);
    table_->setAlternatingRowColors(true);
    table_->setSortingEnabled(false);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table_->horizontalHeader()->setSectionsMovable(true);
    table_->horizontalHeader()->setStretchLastSection(false);
    table_->verticalHeader()->setDefaultSectionSize(24);
    root->addWidget(table_, 1);

    statusLabel_ = new QLabel(this);
    statusLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    root->addWidget(statusLabel_);
}

bool FileTab::load(QString *errorMessage)
{
    if (!document_->open(filePath_, errorMessage)) {
        return false;
    }

    columnTypes_.clear();
    for (const auto &column : document_->schema()) {
        columnTypes_.insert(column.name, column.type);
    }
    filterBuilder_->setColumns(document_->columnNames());
    reload();
    return true;
}

QString FileTab::displayName() const
{
    return QFileInfo(filePath_).fileName();
}

QString FileTab::filePath() const
{
    return filePath_;
}

void FileTab::reload()
{
    QString error;
    const QString whereClause = buildWhereClause(&error);
    if (!error.isEmpty()) {
        statusLabel_->setText(error);
        return;
    }

    qsizetype filteredRows = 0;
    if (!document_->rowCount(whereClause, &filteredRows, &error)) {
        statusLabel_->setText(error);
        return;
    }

    QueryPage page;
    if (!document_->queryPage(whereClause, pageSize_->value(), offset_, &page, &error)) {
        statusLabel_->setText(error);
        return;
    }

    currentPageRows_ = page.rows.size();
    model_->setResult(std::move(page.columns), std::move(page.rows), offset_);
    table_->resizeColumnsToContents();
    updatePageControls(filteredRows);
}

void FileTab::previousPage()
{
    offset_ = std::max<qsizetype>(0, offset_ - pageSize_->value());
    reload();
}

void FileTab::nextPage()
{
    offset_ += pageSize_->value();
    reload();
}

void FileTab::updatePageControls(qsizetype filteredRows)
{
    const qsizetype firstRow = filteredRows == 0 ? 0 : offset_ + 1;
    const qsizetype lastRow = filteredRows == 0 ? 0 : std::min(offset_ + currentPageRows_, filteredRows);

    pageLabel_->setText(tr("%1-%2 of %3").arg(firstRow).arg(lastRow).arg(filteredRows));
    previousButton_->setEnabled(offset_ > 0);
    nextButton_->setEnabled(offset_ + currentPageRows_ < filteredRows);

    statusLabel_->setText(tr("%1 columns. Showing %2 rows from %3.")
        .arg(document_->columnNames().size())
        .arg(currentPageRows_)
        .arg(filePath_));
}

QString FileTab::buildWhereClause(QString *errorMessage) const
{
    QStringList clauses;

    const QString searchClause = searchToSql(errorMessage);
    if (!errorMessage->isEmpty()) {
        return {};
    }
    if (!searchClause.isEmpty()) {
        clauses.push_back(QStringLiteral("(%1)").arg(searchClause));
    }

    for (const auto &condition : filterBuilder_->conditions()) {
        const QString clause = conditionToSql(condition, errorMessage);
        if (!errorMessage->isEmpty()) {
            return {};
        }
        if (!clause.isEmpty()) {
            clauses.push_back(QStringLiteral("(%1)").arg(clause));
        }
    }

    return clauses.join(QStringLiteral(" AND "));
}

QString FileTab::conditionToSql(const FilterCondition &condition, QString *errorMessage) const
{
    const QString identifier = quoteSqlIdentifier(condition.column);
    const QString type = columnType(condition.column);
    const QString op = condition.op;

    if (op == QStringLiteral("is true")) {
        return QStringLiteral("%1 = TRUE").arg(identifier);
    }
    if (op == QStringLiteral("is false")) {
        return QStringLiteral("%1 = FALSE").arg(identifier);
    }
    if (op == QStringLiteral("is empty")) {
        return QStringLiteral("(%1 IS NULL OR CAST(%1 AS VARCHAR) = '')").arg(identifier);
    }
    if (op == QStringLiteral("is not empty")) {
        return QStringLiteral("(%1 IS NOT NULL AND CAST(%1 AS VARCHAR) <> '')").arg(identifier);
    }

    if (condition.value.trimmed().isEmpty()) {
        *errorMessage = tr("A filter value is empty.");
        return {};
    }

    if (op == QStringLiteral("contains text")
        || op == QStringLiteral("starts with")
        || op == QStringLiteral("ends with")) {
        QString pattern = escapeLikePattern(condition.value);
        if (op == QStringLiteral("contains text")) {
            pattern = QStringLiteral("%") + pattern + QStringLiteral("%");
        } else if (op == QStringLiteral("starts with")) {
            pattern = pattern + QStringLiteral("%");
        } else {
            pattern = QStringLiteral("%") + pattern;
        }
        return QStringLiteral("CAST(%1 AS VARCHAR) ILIKE '%2' ESCAPE '\\'").arg(identifier, pattern);
    }

    const QString literal = literalForValue(type, condition.value, errorMessage);
    if (!errorMessage->isEmpty()) {
        return {};
    }

    if (literal == QStringLiteral("NULL")) {
        if (op == QStringLiteral("equals")) {
            return QStringLiteral("%1 IS NULL").arg(identifier);
        }
        if (op == QStringLiteral("does not equal")) {
            return QStringLiteral("%1 IS NOT NULL").arg(identifier);
        }
        *errorMessage = tr("Only equals / does not equal can be used with NULL.");
        return {};
    }

    if (op == QStringLiteral("equals")) {
        return QStringLiteral("%1 = %2").arg(identifier, literal);
    }
    if (op == QStringLiteral("does not equal")) {
        return QStringLiteral("%1 <> %2").arg(identifier, literal);
    }
    if (op == QStringLiteral("greater than")) {
        return QStringLiteral("%1 > %2").arg(identifier, literal);
    }
    if (op == QStringLiteral("less than")) {
        return QStringLiteral("%1 < %2").arg(identifier, literal);
    }
    if (op == QStringLiteral("at least")) {
        return QStringLiteral("%1 >= %2").arg(identifier, literal);
    }
    if (op == QStringLiteral("at most")) {
        return QStringLiteral("%1 <= %2").arg(identifier, literal);
    }

    *errorMessage = tr("Unknown filter operator: %1").arg(op);
    return {};
}

QString FileTab::searchToSql(QString *errorMessage) const
{
    const QString value = searchBox_->text().trimmed();
    if (value.trimmed().isEmpty()) {
        return {};
    }

    FilterCondition quickFilter;
    if (tryParseQuickFilter(value, document_->columnNames(), &quickFilter)) {
        return conditionToSql(quickFilter, errorMessage);
    }

    QStringList clauses;
    for (const QString &column : document_->columnNames()) {
        const QString identifier = quoteSqlIdentifier(column);
        if (searchMode_->currentText() == tr("exact")) {
            clauses.push_back(QStringLiteral("CAST(%1 AS VARCHAR) = %2").arg(identifier, quoteSqlString(value)));
        } else {
            const QString pattern = QStringLiteral("%") + escapeLikePattern(value) + QStringLiteral("%");
            clauses.push_back(QStringLiteral("CAST(%1 AS VARCHAR) ILIKE '%2' ESCAPE '\\'").arg(identifier, pattern));
        }
    }

    return clauses.join(QStringLiteral(" OR "));
}

QString FileTab::columnType(const QString &column) const
{
    return columnTypes_.value(column);
}
