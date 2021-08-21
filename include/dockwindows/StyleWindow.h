#pragma once
#include <QWidget>
#include <QProxyStyle>
#include <QStyleOption>
#include "ROI/ROIStyle.h"

class StyleWindow : public QWidget
{
    Q_OBJECT
    public:
        explicit StyleWindow(QWidget* parent);
                
    signals:
        void ROIColorChanged(const QColor& clr);
        void ROIStyleChanged(ROIStyle style);
        void ChartStyleChanged();

    public slots:
        void selectionChange(const QColor& clr);

    private slots:
        void ROIColorChange();
        void ROIStyleChange();
        void ChartStyleChange();
        void LineChartStyleChange();
        void RidgeChartStyleChange();
    
    private:
        struct pimpl;
        std::unique_ptr<pimpl>impl = std::make_unique<pimpl>();
};

class CustomTabStyle : public QProxyStyle {
public:
  QSize sizeFromContents(ContentsType type, const QStyleOption* option,
                         const QSize& size, const QWidget* widget) const {
    QSize s = QProxyStyle::sizeFromContents(type, option, size, widget);
    if (type == QStyle::CT_TabBarTab) {
      s.transpose();
    }
    return s;
  }

  void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const {
    if (element == CE_TabBarTabLabel) {
      if (const QStyleOptionTab* tab = qstyleoption_cast<const QStyleOptionTab*>(option)) {
        QStyleOptionTab opt(*tab);
        opt.shape = QTabBar::RoundedNorth;
        QProxyStyle::drawControl(element, &opt, painter, widget);
        return;
      }
    }
    QProxyStyle::drawControl(element, option, painter, widget);
  }
};
