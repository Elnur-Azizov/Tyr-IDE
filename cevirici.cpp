#include "cevirici.h"

Cevirici::Cevirici() {}

QString Cevirici::turkcedenCppye(const QVector<Parca> &parcalar, DilMotoru *motor) {
    QString cppKod =
        "#include <iostream>\n"
        "#include <string>\n"
        "#include <vector>\n"
        "using namespace std;\n\n";

    bool yazdirmaModu = false;
    int aktifSatir = -1;

    for (int i = 0; i < parcalar.size(); ++i) {
        QString icerik = parcalar[i].icerik.simplified();
        int satirNo = parcalar[i].satir;

        if (icerik.isEmpty())
            continue;

        // Yeni kaynak satıra geçildiyse C++ tarafına satır eşleme koy
        if (satirNo != aktifSatir) {
            aktifSatir = satirNo;
            cppKod += "\n#line " + QString::number(satirNo) + " \"kaynak.tr\"\n";
        }

        // --- 1. ÖZEL KOMUTLAR ---
        if (icerik == "yazdir") {
            cppKod += "cout << ";
            yazdirmaModu = true;
            continue;
        }

        if (icerik == "oku") {
            cppKod += "cin >> ";
            continue;
        }

        // --- 2. NOKTALI VİRGÜL KONTROLÜ ---
        if (icerik == ";") {
            if (yazdirmaModu) {
                cppKod += " << endl;\n";
                yazdirmaModu = false;
            } else {
                cppKod += ";\n";
            }
            continue;
        }

        // --- 3. SÖZLÜK KONTROLÜ ---
        if (motor->sozluk.contains(icerik)) {
            cppKod += motor->sozluk[icerik] + " ";
            continue;
        }

        // --- 4. DİĞER HER ŞEY ---
        cppKod += icerik + " ";
    }

    return cppKod;
}
