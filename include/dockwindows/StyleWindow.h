#pragma once
#include <QDockWidget>
#include <QProxyStyle>
#include <QStyleOption>

class ROIs;
class TraceView;

class StyleWindow : public QDockWidget
{
    Q_OBJECT
    public:
        explicit StyleWindow(QWidget* parent);
        void setROIs(ROIs* rois);
        void setTraceView(TraceView* traceview);
        void loadSettings();

    public slots:
        void selectionChange(std::vector<size_t> inds);

        void ROIColorChange();
        void ROIStyleChange();
        void ChartStyleChange();
        void LineChartStyleChange();
        void RidgeChartStyleChange();
        void RidgeOverlapChange();
        void LineMatchyChange();

    private:
        struct pimpl;
        std::unique_ptr<pimpl>impl = std::make_unique<pimpl>();
};

class CustomTabStyle : public QProxyStyle {
public:
    QSize sizeFromContents(ContentsType, const QStyleOption*, const QSize&, const QWidget*) const;
    void drawControl(ControlElement, const QStyleOption*, QPainter*, const QWidget*) const;
};
