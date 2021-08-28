#pragma once
#include <QDialog>
#include <QBitArray>

namespace ROIVert {
    enum class RESET {
        WINDOW,
        ROICOLOR,
        ROISTYLE,
        CHARTSTYLE,
        IMAGEDATA
    };
}

class ResetDialog : public QDialog
{
public:
    ResetDialog(QWidget* parent = nullptr);
    QBitArray getResult();

private:
    QBitArray res = QBitArray(5);
};

