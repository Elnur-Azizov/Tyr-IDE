#ifndef CEVIRICI_H
#define CEVIRICI_H

#include <QString>
#include <QVector>
#include "dilmotoru.h"

class Cevirici {
public:
    Cevirici();
    QString turkcedenCppye(const QVector<Parca> &parcalar, DilMotoru *motor);
};

#endif
