#pragma once
#include <QString>
#include <QVector>
#include <QColor>
#include "opencv2/opencv.hpp"

// misc shared data types?
namespace ROIVert {
    struct imgsettings {
        double contrastMin = 0.;
        double contrastMax = 1.;
        double contrastGamma = 1.;

        int projectionType = 0;
        int cmap = -1;

        int smoothType = 0;
        int smoothSize = 5;
        double smoothSigma = 0.;
        double smoothSimgaI = 0.;
    };

    static const QString dffstring() {
        return(QString::fromWCharArray(L"\x03B4\xD835\xDC53/\xD835\xDC53"));
    }
    

    static const QVector<QColor> colors() {
        QVector<QColor>ret = {
        QColor("#2264A5"),
        QColor("#F75C03"),
        QColor("#F1C40F"),
        QColor("#D90368"),
        QColor("#00CC66")
        };
        return ret;
    }

    static const cv::Rect QRect2CVRect(const QRect &bb) {
        return cv::Rect(static_cast<size_t>(bb.x()),
            static_cast<size_t>(bb.y()),
            static_cast<size_t>(bb.width()),
            static_cast<size_t>(bb.height()));
    }
}
