#pragma once

#include <QString>

struct FilterCondition {
    QString column;
    QString op;
    QString value;
};
