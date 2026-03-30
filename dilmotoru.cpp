#include "dilmotoru.h"

DilMotoru::DilMotoru() {
    sozlukHazirla();
}

void DilMotoru::sozlukHazirla() {
    // Veri Tipleri
    sozluk["tamsayi"] = "int";
    sozluk["ondalik"] = "float";
    sozluk["metin"] = "string";
    sozluk["mantiksal"] = "bool";
    sozluk["donmeyen"] = "void";

    // Karar ve Döngü Yapıları
    sozluk["eger"] = "if";
    sozluk["degilse"] = "else";
    sozluk["tekrarla"] = "for";
    sozluk["olanadek"] = "while";
    sozluk["herbir"] = "for";
    sozluk["icin"] = ":";

    // Fonksiyon ve Akış Kontrolü
    sozluk["geridondur"] = "return";
    sozluk["dkir"] = "break";
    sozluk["secim"] = "switch";
    sozluk["durum"] = "case";
    sozluk["varsayilan"] = "default";
    sozluk["cekirdek"] = "main";

    // Mantıksal Operatörler
    sozluk["ve"] = "&&";
    sozluk["veya"] = "||";
    sozluk["dogru"] = "true";
    sozluk["yanlis"] = "false";

    // Not: "yazdir" ve "oku" kelimelerini sözlüğe eklemiyoruz.
    // Çünkü cevirici.cpp içerisinde bunlara özel '<<' ve '>>' operatörleri ekleniyor.
}
static QString yorumlariTemizleAmaStringiKoru(const QString &satir)
{
    QString sonuc;
    bool stringIcinde = false;
    bool escapeAktif = false;

    for (int i = 0; i < satir.length(); ++i)
    {
        QChar ch = satir[i];

        if (stringIcinde)
        {
            sonuc += ch;

            if (escapeAktif)
            {
                escapeAktif = false;
            }
            else if (ch == '\\')
            {
                escapeAktif = true;
            }
            else if (ch == '"')
            {
                stringIcinde = false;
            }

            continue;
        }

        // string dışındayız
        if (ch == '"')
        {
            stringIcinde = true;
            sonuc += ch;
            continue;
        }

        // string dışında // gördüysek yorum başlar
        if (ch == '/' && i + 1 < satir.length() && satir[i + 1] == '/')
        {
            break;
        }

        sonuc += ch;
    }

    return sonuc;
}

QVector<Parca> DilMotoru::parcalaraAyir(const QString &kod) {
    QVector<Parca> parcalar;

    QRegularExpression re("(\"(?:\\\\.|[^\"])*\"|==|!=|<=|>=|=|&&|\\|\\||\\d+\\.\\d+|\\d+|[a-zA-ZğüşöçıİĞÜŞÖÇ_]+\\w*|[\\+\\-\\*\\/\\%\\=\\;\\,\\(\\)\\{\\}\\[\\]\\.]|>|<)");

    QStringList satirlarListesi = kod.split('\n');

    for (int i = 0; i < satirlarListesi.size(); ++i) {
        QString gecerliSatir = yorumlariTemizleAmaStringiKoru(satirlarListesi[i]);

        if (gecerliSatir.trimmed().isEmpty())
            continue;

        QRegularExpressionMatchIterator it = re.globalMatch(gecerliSatir);

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString kelime = match.captured(1);

            Parca p;
            p.icerik = kelime;
            p.satir = i + 1;

            if (sozluk.contains(kelime)) {
                p.tip = ParcaTipi::AnahtarKelime;
            }
            else if (kelime.startsWith("\"")) {
                p.tip = ParcaTipi::Metin;
            }
            else if (kelime.contains(QRegularExpression("^\\d"))) {
                p.tip = ParcaTipi::Sayi;
            }
            else if (kelime.contains(QRegularExpression("^[\\+\\-\\*\\/\\%\\=\\>\\<\\!\\&\\|]+$"))) {
                p.tip = ParcaTipi::Operator;
            }
            else {
                p.tip = ParcaTipi::Tanimlayici;
            }

            parcalar.append(p);
        }
    }

    return parcalar;
}
