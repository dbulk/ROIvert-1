#pragma once
#include <QObject>
#include <QRect>
class VideoData;
class TraceView;
class ChartStyle;
class TraceChartWidget;
class TraceChartSeries;


namespace ROIVert {
    enum class SHAPE;
}

class ROITrace : public QObject
{
    Q_OBJECT

public:
    ROITrace(TraceView*, VideoData*, std::shared_ptr<ChartStyle>);
    ~ROITrace();
    void update();

    
    TraceChartWidget* getTraceChart() const noexcept;
    TraceChartSeries* getLineSeries() const noexcept;
    TraceChartSeries* getRidgeSeries() const noexcept;
    

    std::vector<float> getTrace() const;

public slots:
    void updateTrace(ROIVert::SHAPE, QRect, std::vector<QPoint>);

private:
    struct pimpl;
    std::unique_ptr<pimpl> impl = std::make_unique<pimpl>();
};

