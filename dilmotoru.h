#ifndef DILMOTORU_H
#define DILMOTORU_H

#include <QString>
#include <QVector>
#include <QMap>
#include <QRegularExpression>

enum class ParcaTipi {
    AnahtarKelime,
    Tanimlayici,
    Sayi,
    Metin,
    Operator,
    Noktalama,
    Bilinmeyen
};

struct Parca {
    ParcaTipi tip;
    QString icerik;
    int satir;
};

class DilMotoru {
public:
    DilMotoru();
    QVector<Parca> parcalaraAyir(const QString &kod);
    QMap<QString, QString> sozluk; // Cevirici'nin erişmesi için public

private:
    struct Degisken {
        QString isim;
        QString tip; // tamsayi, metin vb.
        int satir;
    };
    QMap<QString, Degisken> sembolTablosu;
    void sozlukHazirla();
};

#endif
