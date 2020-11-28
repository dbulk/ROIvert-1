#include "traceviewer.h"
#include "qvalueaxis.h"
#include "qpoint.h"
#include "qgridlayout.h"
#include "qapplication.h"
#include "qscrollbar.h"
#include "qdebug.h"
#include "qabstractseries.h"
#include "qgraphicslayout.h"
#include "roivertcore.h"

using QtCharts::QValueAxis;

TraceViewer::TraceViewer(QWidget* parent)
    : QWidget(parent)
{
    QGridLayout* glay = new QGridLayout;
    this->setLayout(glay);

    lay = new QVBoxLayout();
    scrollArea = new QScrollArea(this);
    QWidget* scrollAreaContent = new QWidget;
    scrollAreaContent->setLayout(lay);
    scrollArea->setWidget(scrollAreaContent);
    scrollArea->setWidgetResizable(true);
    lay->setAlignment(Qt::AlignTop);
    lay->setContentsMargins(0, 0, 0, 0);
    scrollArea->setMinimumHeight(302);
    
    glay->addWidget(scrollArea);
}

TraceViewer::~TraceViewer()
{
}
void TraceViewer::setTrace(const int roiid, const std::vector<double> trace) {
    if (roiid < 1 || roiid > charts.size() + 1) {
        return;
    }
    size_t ind = (size_t)roiid - 1;
    if (charts.size() == ind) {
        push_chart(roiid);
    }

    // build trace, will need time here
    QVector<QPointF> pts;
    double coeff = maxtime/trace.size();
    for (size_t i = 0; i < trace.size(); i++) {
        pts.push_back(QPointF(i*coeff, trace[i]));
    }
    
    QLineSeries* series = qobject_cast<QLineSeries*>(charts[ind]->series()[0]);
    series->replace(pts);

    charts[ind]->axes(Qt::Horizontal)[0]->setMax(pts.size() * coeff);
    charts[ind]->axes(Qt::Horizontal)[0]->setMin(0.);
    double ymin = *std::min_element(trace.begin(), trace.end());
    double ymax = *std::max_element(trace.begin(), trace.end());
    charts[ind]->axes(Qt::Vertical)[0]->setMin(ymin);
    charts[ind]->axes(Qt::Vertical)[0]->setMax(ymax);
    
    chartviews[ind]->show();
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    scrollArea->verticalScrollBar()->setValue(chartviews[ind]->y() + chartviews[ind]->height());
}

void TraceViewer::setmaxtime(double t_secs) {
    maxtime = t_secs;

    // need to update existing charts
    for (size_t i = 0; i < charts.size(); i++) { // for each chart
        //qobject_cast<QLineSeries*>charts[i]->series()[0];
        QLineSeries* series = qobject_cast<QLineSeries*>(charts[i]->series()[0]);
        QVector<QPointF> data = series->pointsVector();

        double coeff = maxtime / data.size();
        for (int ii = 0; ii < data.size(); ii++) {
            data[ii].setX(ii * coeff);
        }
        series->replace(data);
        charts[i]->axes(Qt::Horizontal)[0]->setMax(data.size() * coeff);
    }
}

void TraceViewer::push_chart(int roiid) {

    QChart* chart = new QChart;
    QLineSeries* series = new QLineSeries;
    ChartViewClick* chartView = new ChartViewClick;
    chartView->setChart(chart);
    chartView->setParent(this);

    chart->legend()->hide();
    QString title = "ROI " + QString::number(roiid);
    chart->setTitle(title);
    chart->layout()->setContentsMargins(1, 0, 1, 0);

    chartView->setFixedHeight(300);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setContentsMargins(0, 0, 0, 0);

    series->append(0, 0);
    chart->addSeries(series);
    chart->createDefaultAxes();
    QValueAxis* x = (QValueAxis*)chart->axes(Qt::Horizontal)[0];
    QValueAxis* y = (QValueAxis*)chart->axes(Qt::Vertical)[0];
    
    x->setTitleText("Time (s)");
    x->setGridLineColor(gridclr);
    y->setTitleText(ROIVert::dffstring);
    y->setGridLineColor(gridclr);

    QPen seriespen(selclr, 3);
    series->setPen(seriespen);
    chart->setBackgroundBrush(QBrush(backclr));

    x->setLabelsColor(foreclr);
    y->setLabelsColor(foreclr);
    x->setTitleBrush(foreclr);
    y->setTitleBrush(foreclr);
    chart->setTitleBrush(foreclr);

    charts.push_back(chart);
    chartviews.push_back(chartView);
    lay->addWidget(chartView);

    connect(chartView, &ChartViewClick::clicked, this, [=]() {emit chartClicked(roiid); });
    connect(chartView, &ChartViewClick::keypressed, this, [=](size_t key) {
        if (key == Qt::Key_Delete) {
            emit(deleteroi(roiid));
        }
    });
}

void TraceViewer::setSelectedTrace(int oldind, int newind) {
    if (oldind > 0 && oldind-1 < charts.size()) {
        QLineSeries* series = qobject_cast<QLineSeries*>(charts[oldind-1]->series()[0]);
        series->setColor(unselclr);
    }
    if (newind > 0 && newind-1 < charts.size()) {
        QLineSeries* series = qobject_cast<QLineSeries*>(charts[newind-1]->series()[0]);
        series->setColor(selclr);

        // ensure bottom is visible, then ensure top is visible, 
        int top = chartviews[newind - 1]->y();
        int bottom = top + chartviews[newind - 1]->height();
        scrollArea->ensureVisible(0, bottom, 0, 0);
        scrollArea->ensureVisible(0, top, 0, 0);

    }
    selectedind = newind;
}

void TraceViewer::roideleted(size_t roiind) {
    if (roiind > 0 && roiind - 1 < charts.size()) {
        delete(charts[roiind - 1]);
        delete(chartviews[roiind - 1]);

        charts.erase(charts.begin() + roiind - 1);
        chartviews.erase(chartviews.begin() + roiind - 1);

        // need to retitle and reconnect all the charts and chartviews
        // This is cheap enough, just go through everything:
        for (int i = 0; i < charts.size(); i++) {
            QString title = "ROI " + QString::number(i+1);
            charts[i]->setTitle(title);
            
            chartviews[i]->disconnect();
            connect(chartviews[i], &ChartViewClick::clicked, this, [=]() {emit chartClicked(i + 1); });
            connect(chartviews[i], &ChartViewClick::keypressed, this, [=](size_t key) {
                if (key == Qt::Key_Delete) {
                    emit(deleteroi(i + 1));
                }
            });
        }
    }
}

const std::vector<std::vector<float>> TraceViewer::getAllTraces() {
    std::vector<std::vector<float>> ret;
    ret.reserve(charts.size());// number of traces

    // for each chart
    for each (QChart *chart in charts) {
        QLineSeries* series = qobject_cast<QLineSeries*>(chart->series()[0]);
        QVector<QPointF> data = series->pointsVector();
        std::vector<float> trace;
        trace.reserve(data.size());
        for each (QPointF datum in data) {
            trace.push_back(datum.y());
        }
        ret.push_back(trace);
    }
    return ret;
}

const std::vector<float> TraceViewer::getTVec() {
    std::vector<float> ret;
    if (!charts.empty()) {
        QLineSeries* series = qobject_cast<QLineSeries*>(charts[0]->series()[0]);
        QVector<QPointF> data = series->pointsVector();
        ret.reserve(data.size());
        for each (QPointF datum in data) {
            ret.push_back(datum.x());
        }
    }
    return ret;
}



void TraceViewer::setSelectedColor(QColor clr){
    selclr = clr;
    if (selectedind > 0) {
        QLineSeries* series = qobject_cast<QLineSeries*>(charts[selectedind - 1]->series()[0]);
        QPen seriespen(selclr, 3);
        series->setPen(seriespen);
    }
    //selectedind 
}
void TraceViewer::setUnselectedColor(QColor clr){
    unselclr = clr;
    for (int i = 0; i < charts.size(); i++) {
        if (i + 1 != selectedind) {
            QLineSeries* series = qobject_cast<QLineSeries*>(charts[i]->series()[0]);
            QPen seriespen(unselclr, 3);
            series->setPen(seriespen);
        }
    }
}

void TraceViewer::setBackgroundColor(QColor clr) {
    backclr = clr;

    for each (QChart* chart  in charts)
    {
        chart->setBackgroundBrush(QBrush(backclr));
    }
}

void TraceViewer::setForegroundColor(QColor clr) {

    foreclr = clr;
    for each (QChart * chart  in charts)
    {
        QValueAxis* x = (QValueAxis*)chart->axes(Qt::Horizontal)[0];
        QValueAxis* y = (QValueAxis*)chart->axes(Qt::Vertical)[0];

        x->setLabelsColor(foreclr);
        y->setLabelsColor(foreclr);
        x->setTitleBrush(foreclr);
        y->setTitleBrush(foreclr);
        chart->setTitleBrush(foreclr);
    }
}
void TraceViewer::setGridColor(QColor clr) {
    gridclr = clr;

    for each (QChart * chart  in charts)
    {
        QValueAxis* x = (QValueAxis*)chart->axes(Qt::Horizontal)[0];
        QValueAxis* y = (QValueAxis*)chart->axes(Qt::Vertical)[0];
        x->setGridLineColor(gridclr);
        y->setGridLineColor(gridclr);
    }
}